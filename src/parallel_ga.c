#include "parallel_ga.h"
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>

// Migrate the single best tour across islands (gather -> pick best -> bcast -> replace local worst).
static void migrate_best(const TSP *t, const GAParams *P, const MPIInfo *mpi,
                         Population *pop, unsigned int *rng)
{
    (void)P;   // not used in this simple migration
    (void)rng; // not used here

    const int n = t->n;

    // Pack my best tour
    const int my_best_idx = pop_argmin(pop);
    int *sendbuf = pop->inds[my_best_idx].perm;

    // Root receives all best tours
    int *allbuf = NULL;
    if (mpi->rank == 0) {
        allbuf = (int*)malloc(sizeof(int) * n * mpi->size);
        if (!allbuf) {
            fprintf(stderr, "[migrate][ERROR] OOM for allbuf\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // Gather best perms at root
    MPI_Gather(sendbuf, n, MPI_INT, allbuf, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Root picks global best
    int *global_best_perm = (int*)malloc(sizeof(int) * n);  // allocate on ALL ranks (needed for Bcast)
    if (!global_best_perm) {
        fprintf(stderr, "[migrate][ERROR] OOM for global_best_perm\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (mpi->rank == 0) {
        double bestL = DBL_MAX;
        // default to rank 0's best
        memcpy(global_best_perm, allbuf, sizeof(int) * n);

        for (int r = 0; r < mpi->size; r++) {
            const double L = tsp_tour_length(t, allbuf + r * n);
            if (L < bestL) {
                bestL = L;
                memcpy(global_best_perm, allbuf + r * n, sizeof(int) * n);
            }
        }
    }

    // Broadcast the global best to everyone
    MPI_Bcast(global_best_perm, n, MPI_INT, 0, MPI_COMM_WORLD);

    // Replace local worst with the global best (elitism-based migration)
    const int worst = pop_argmax(pop);
    memcpy(pop->inds[worst].perm, global_best_perm, sizeof(int) * n);
    pop->inds[worst].fitness = tsp_tour_length(t, pop->inds[worst].perm);

    // Cleanup
    if (mpi->rank == 0) free(allbuf);
    free(global_best_perm);
}

void parallel_ga_run(const TSP *t, const GAParams *P, const MPIInfo *mpi,
                     Individual *global_best, double *elapsed)
{
    // Per-island population
    unsigned int rng = (unsigned int)(P->seed + (unsigned)mpi->rank * 1337u);

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

        // Periodic migration (only if enabled)
        if (P->migration_interval > 0 && (gen % P->migration_interval) == 0) {
            migrate_best(t, P, mpi, &pop, &rng);
        }

        // Lightweight progress (rank 0 only)
        if (mpi->rank == 0 && (gen % 10) == 0) {
            const int bidx = pop_argmin(&pop);
            fprintf(stdout, "[run] gen=%d best=%.6f\n", gen, pop.inds[bidx].fitness);
            fflush(stdout);
        }
    }

    const double t1 = MPI_Wtime();
    *elapsed = t1 - t0;

    // Find island best
    const int b = pop_argmin(&pop);
    Individual mybest = indiv_create(t->n);
    indiv_copy(&mybest, &pop.inds[b]);

    // Reduce to global best (by fitness) across all ranks
    struct { double fitness; int rank; } in, out;
    in.fitness = mybest.fitness;
    in.rank    = mpi->rank;

    MPI_Allreduce(&in, &out, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    // Broadcast the best perm from the winning rank
    if (mpi->rank == out.rank) {
        memcpy(global_best->perm, mybest.perm, sizeof(int) * t->n);
        global_best->fitness = mybest.fitness;
    }
    MPI_Bcast(global_best->perm, t->n, MPI_INT, out.rank, MPI_COMM_WORLD);

    // Re-evaluate to be safe
    global_best->fitness = tsp_tour_length(t, global_best->perm);

    indiv_free(&mybest);
    pop_free(&pop);
}
