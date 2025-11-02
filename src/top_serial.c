#include "ga.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <dataset>\n", argv[0]);
        return 1;
    }
    const char *path = argv[1];
    TSP *t = tsp_load_from_file(path);
    if (!t) return 1;

    GAParams P = { .pop_size = 200, .generations = 1000, .cx_rate = 0.8, .mut_rate = 0.05, .tournament_k = 4, .use_two_opt = 1 };
    unsigned int rng = (unsigned int)time(NULL);

    Population pop = pop_create(P.pop_size, t->n);
    for (int i = 0; i < pop.pop_size; i++) indiv_randomize(&pop.inds[i], &rng);
    pop_eval(&pop, t);

    double bestL = 1e9;
    int bestIdx = 0;

    for (int g = 1; g <= P.generations; g++) {
        ga_step(&pop, t, &P, &rng);
        int idx = pop_argmin(&pop);
        if (pop.inds[idx].fitness < bestL) {
            bestL = pop.inds[idx].fitness;
            bestIdx = idx;
        }
        if (g % 100 == 0)
            printf("Gen %d best=%.4f\n", g, bestL);
    }

    printf("Best tour length (serial): %.6f\n", bestL);
    tsp_free(t);
    pop_free(&pop);
    return 0;
}
