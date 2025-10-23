#include "ga.h"
#include "parallel_ga.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parse_args(int argc, char **argv, GAParams *P, const char **path){
    // defaults (you can tune these)
    P->pop_size = 200;
    P->generations = 500;
    P->cx_rate = 0.8;
    P->mut_rate = 0.05;
    P->tournament_k = 4;
    P->migration_interval = 50;
    P->migration_elites = 1; // we migrate 1 best via gather/bcast in this skeleton
    P->use_two_opt = 1;
    P->seed = 42;

    *path = (argc>1)? argv[1] : "data/cities.txt";
    for (int i=2;i<argc;i++){
        if (strcmp(argv[i],"--pop")==0 && i+1<argc) P->pop_size = atoi(argv[++i]);
        else if (strcmp(argv[i],"--generations")==0 && i+1<argc) P->generations = atoi(argv[++i]);
        else if (strcmp(argv[i],"--cx")==0 && i+1<argc) P->cx_rate = atof(argv[++i]);
        else if (strcmp(argv[i],"--mut")==0 && i+1<argc) P->mut_rate = atof(argv[++i]);
        else if (strcmp(argv[i],"--k")==0 && i+1<argc) P->tournament_k = atoi(argv[++i]);
        else if (strcmp(argv[i],"--mig-int")==0 && i+1<argc) P->migration_interval = atoi(argv[++i]);
        else if (strcmp(argv[i],"--no-twoopt")==0) P->use_two_opt = 0;
        else if (strcmp(argv[i],"--seed")==0 && i+1<argc) P->seed = (unsigned long)atol(argv[++i]);
    }
}

int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    GAParams P; const char *path=NULL;
    parse_args(argc, argv, &P, &path);

    if (rank==0) {
        printf("Parallel GA TSP | islands=%d, pop/island=%d, gens=%d, cx=%.2f, mut=%.2f, k=%d, migInt=%d, twoopt=%d\n",
            size, P.pop_size, P.generations, P.cx_rate, P.mut_rate, P.tournament_k, P.migration_interval, P.use_two_opt);
        printf("Dataset: %s\n", path);
        fflush(stdout);
    }

    TSP *t = tsp_load_from_file(path);
    if(!t){
        if(rank==0) fprintf(stderr, "Failed to load TSP file: %s\n", path);
        MPI_Finalize();
        return 1;
    }

    MPIInfo mpi = { rank, size };
    Individual best = indiv_create(t->n);
    double elapsed=0.0;

    parallel_ga_run(t, &P, &mpi, &best, &elapsed);

    // root prints final best + time
    if(rank==0){
        printf("Best tour length: %.6f\n", best.fitness);
        printf("Elapsed (parallel, p=%d): %.4f s\n", size, elapsed);
        printf("Best tour: ");
        for(int i=0;i<t->n;i++) printf("%d ", best.perm[i]);
        printf("\n");
    }

    indiv_free(&best);
    tsp_free(t);
    MPI_Finalize();
    return 0;
}
