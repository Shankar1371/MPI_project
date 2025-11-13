#include "ga.h"
#include "parallel_ga.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GAParams params;
    const char *dataset_path;
    const char *save_route_path; // optional: where to dump best tour indices
} CLI;

static void parse_args(int argc, char **argv, CLI *cli) {
    // Defaults (tuned for this project)
    GAParams *P = &cli->params;
    P->pop_size           = 200;
    P->generations        = 50;     // default now 50 (project cap)
    P->cx_rate            = 0.80;
    P->mut_rate           = 0.05;
    P->tournament_k       = 4;
    P->migration_interval = 50;
    P->migration_elites   = 1;
    P->use_two_opt        = 1;
    P->seed               = 42UL;

    cli->dataset_path    = (argc > 1) ? argv[1] : "data/cities.txt";
    cli->save_route_path = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--pop") == 0 && i + 1 < argc) {
            P->pop_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--generations") == 0 && i + 1 < argc) {
            P->generations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--cx") == 0 && i + 1 < argc) {
            P->cx_rate = atof(argv[++i]);
        } else if (strcmp(argv[i], "--mut") == 0 && i + 1 < argc) {
            P->mut_rate = atof(argv[++i]);
        } else if (strcmp(argv[i], "--k") == 0 && i + 1 < argc) {
            P->tournament_k = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--mig-int") == 0 && i + 1 < argc) {
            P->migration_interval = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-twoopt") == 0) {
            P->use_two_opt = 0;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            P->seed = (unsigned long)atol(argv[++i]);
        } else if (strcmp(argv[i], "--save-route") == 0 && i + 1 < argc) {
            cli->save_route_path = argv[++i];
        }
    }
    
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    CLI cli;
    parse_args(argc, argv, &cli);
    GAParams *P = &cli.params;

    // Enforce the project rule: max 50 generations
    int requested_gens = P->generations;
    if (P->generations > 50) {
        P->generations = 50;
        if (rank == 0) {
            printf("[note] generations requested=%d capped to 50 per project spec\n", requested_gens);
            fflush(stdout);
        }
    }

    if (rank == 0) {
        printf("Parallel GA TSP | islands=%d, pop/island=%d, gens=%d, cx=%.2f, mut=%.2f, k=%d, migInt=%d, twoopt=%d\n",
               size, P->pop_size, P->generations, P->cx_rate, P->mut_rate,
               P->tournament_k, P->migration_interval, P->use_two_opt);
        printf("Dataset: %s\n", cli.dataset_path);
        fflush(stdout);
    }

    TSP *t = tsp_load_from_file(cli.dataset_path);
    if (!t) {
        if (rank == 0) fprintf(stderr, "Failed to load TSP file: %s\n", cli.dataset_path);
        MPI_Finalize();
        return 1;
    }

    MPIInfo mpi = { rank, size };
    Individual best = indiv_create(t->n);
    double elapsed = 0.0;

    parallel_ga_run(t, P, &mpi, &best, &elapsed);

    if (rank == 0) {
        // Final report
        printf("Best tour length: %.6f\n", best.fitness);
        printf("Elapsed (parallel, p=%d): %.4f s\n", size, elapsed);
        printf("Best tour: ");
        for (int i = 0; i < t->n; i++) printf("%d ", best.perm[i]);
        printf("\n");

        // Optional: dump permutation to a file for plotting (one line of indices)
        if (cli.save_route_path && cli.save_route_path[0]) {
            FILE *rf = fopen(cli.save_route_path, "w");
            if (rf) {
                for (int i = 0; i < t->n; i++) {
                    if (i) fputc(' ', rf);
                    fprintf(rf, "%d", best.perm[i]);
                }
                fputc('\n', rf);
                fclose(rf);
                printf("[info] saved best route to: %s\n", cli.save_route_path);
            } else {
                fprintf(stderr, "[warn] could not open --save-route path: %s\n", cli.save_route_path);
            }
        }

        // One-line summary (easy to grep from bench scripts)
        printf("[summary] dataset=%s p=%d gens=%d pop=%d twoopt=%d best_len=%.6f elapsed=%.4f\n",
               cli.dataset_path, size, P->generations, P->pop_size, P->use_two_opt,
               best.fitness, elapsed);
        fflush(stdout);
    }

    indiv_free(&best);
    tsp_free(t);
    MPI_Finalize();
    return 0;
}
