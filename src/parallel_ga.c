#include "parallel_ga.h"
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>

/* Migrate the top 5% elites across islands:
   - ensure population is sorted by fitness first
   - GATHER all elites at root
   - root picks global best tour
   - BCAST best tour to all ranks
   - each rank replaces its local worst with the global best
*/
static void migrate_best(const TSP *t, const GAParams *P, const MPIInfo *mpi,
                         Population *pop, unsigned int *rng)
{
    (void)P;   /* not used here; silence -Wunused-parameter */
    (void)rng; /* migration is deterministic w.r.t. current populations */

    const int n = t->n;

    /* Make sure we’re actually taking the best individuals */
    pop_eval(pop, t);
    pop_sort_by_fitness(pop);

    int top = (int)(0.05 * pop->pop_size);
    if (top < 1) top = 1;
    if (top > pop->pop_size) top = pop->pop_size;

    const int send_count = top * n;
    int *sendbuf = (int*)malloc((size_t)send_count * sizeof(int));
    if (!sendbuf) return; /* out-of-memory: skip this migration safely */

    for (int i = 0; i < top; i++) {
        memcpy(sendbuf + i * n, pop->inds[i].perm, (size_t)n * sizeof(int));
    }

    int *allbuf = NULL;
    if (mpi->rank == 0) {
        allbuf = (int*)malloc((size_t)send_count * (size_t)mpi->size * sizeof(int));
        if (!allbuf) { free(sendbuf); return; }
    }

    /* Gather top-5% permutations from all islands */
    MPI_Gather(sendbuf, send_count, MPI_INT,
               allbuf,  send_count, MPI_INT,
               0, MPI_COMM_WORLD);

    int *global_best_perm = (int*)malloc((size_t)n * sizeof(int));
    if (!global_best_perm) {
        free(sendbuf);
        if (mpi->rank == 0) free(allbuf);
        return;
    }

    if (mpi->rank == 0) {
        double bestL = DBL_MAX;
        for (int r = 0; r < mpi->size; r++) {
            const int *block = allbuf + r * send_count;
            for (int e = 0; e < top; e++) {
                const int *cand = block + e * n;
                double L = tsp_tour_length(t, cand);
                if (L < bestL) {
                    bestL = L;
                    memcpy(global_best_perm, cand, (size_t)n * sizeof(int));
                }
            }
        }
    }

    /* Broadcast the global best permutation to all islands */
    MPI_Bcast(global_best_perm, n, MPI_INT, 0, MPI_COMM_WORLD);

    /* Replace the local worst with the global best */
    int worst = pop_argmax(pop);
    memcpy(pop->inds[worst].perm, global_best_perm, (size_t)n * sizeof(int));
    pop->inds[worst].fitness = tsp_tour_length(t, pop->inds[worst].perm);

    free(sendbuf);
    free(global_best_perm);
    if (mpi->rank == 0) free(allbuf);
}

void parallel_ga_run(const TSP *t, const GAParams *P, const MPIInfo *mpi,
                     Individual *global_best, double *elapsed)
{
    /* Per-island RNG seed */
    unsigned int rng = (unsigned int)(P->seed + (unsigned)mpi->rank * 1337u);

    /* Initialize island population */
    Population pop = pop_create(P->pop_size, t->n);
    for (int i = 0; i < pop.pop_size; i++) {
        indiv_randomize(&pop.inds[i], &rng);
    }
    pop_eval(&pop, t);

    if (mpi->rank == 0) {
        fprintf(stdout, "[run] islands=%d pop/island=%d gens=%d twoopt=%d migInt=%d\n",
                mpi->size, P->pop_size, P->generations, P->use_two_opt, P->migration_interval);
        fflush(stdout);
    }

    const double t0 = MPI_Wtime();

    for (int gen = 1; gen <= P->generations; gen++) {
        ga_step(&pop, t, P, &rng);

        /* Periodic migration */
        if (P->migration_interval > 0 && (gen % P->migration_interval) == 0) {
            migrate_best(t, P, mpi, &pop, &rng);
        }

        /* Lightweight progress (rank 0 only) */
        if (mpi->rank == 0 && (gen % 10) == 0) {
            const int bidx = pop_argmin(&pop);
            fprintf(stdout, "[run] gen=%d best=%.6f\n", gen, pop.inds[bidx].fitness);
            fflush(stdout);
        }
    }

    const double t1 = MPI_Wtime();
    *elapsed = t1 - t0;

    /* Find island-local best */
    const int b = pop_argmin(&pop);
    Individual mybest = indiv_create(t->n);
    indiv_copy(&mybest, &pop.inds[b]);

    /* Reduce to global best across all ranks by fitness (MINLOC) */
    struct { double fitness; int rank; } in, out;
    in.fitness = mybest.fitness;
    in.rank    = mpi->rank;

    MPI_Allreduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    /* Broadcast the best permutation from the winning rank */
    if (mpi->rank == out.rank) {
        memcpy(global_best->perm, mybest.perm, (size_t)t->n * sizeof(int));
        global_best->fitness = mybest.fitness;
    }
    MPI_Bcast(global_best->perm, t->n, MPI_INT, out.rank, MPI_COMM_WORLD);

    /* Re-evaluate to be safe */
    global_best->fitness = tsp_tour_length(t, global_best->perm);

    indiv_free(&mybest);
    pop_free(&pop);
}
