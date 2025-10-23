#ifndef PARALLEL_GA_H
#define PARALLEL_GA_H

#include "ga.h"

typedef struct {
    int rank, size;  // MPI rank/size
} MPIInfo;

typedef struct {
    Individual best; // best tour on this island
    // buffer used for migrations
    int *buf_perm;
} Island;

void parallel_ga_run(const TSP *t, const GAParams *P, const MPIInfo *mpi, Individual *global_best, double *elapsed);

#endif
