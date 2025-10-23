#ifndef GA_H
#define GA_H

#include <stddef.h>

typedef struct {
    int n;              // number of cities
    double *coords;     // [n*2] x,y
    double *dist;       // [n*n] distance matrix
} TSP;

typedef struct {
    int n;              // chromosome length (n cities)
    int *perm;          // permutation of 0..n-1
    double fitness;     // total tour length
} Individual;

typedef struct {
    int n;
    int pop_size;
    Individual *inds;   // [pop_size]
} Population;

typedef struct {
    int pop_size;       // total pop per island (per rank)
    int generations;
    double cx_rate;     // crossover probability
    double mut_rate;    // mutation probability
    int tournament_k;   // tournament size
    int migration_interval; // generations between migrations
    int migration_elites;   // how many elites per island to exchange
    int use_two_opt;        // 0/1
    unsigned long seed;     // base seed
} GAParams;

// ---------- TSP helpers ----------
TSP *tsp_load_from_file(const char *path);
void tsp_free(TSP *t);
void tsp_build_dist(TSP *t);
double tsp_tour_length(const TSP *t, const int *perm);

// ---------- GA core (serial) ----------
Individual indiv_create(int n);
void indiv_free(Individual *ind);
void indiv_randomize(Individual *ind, unsigned int *rng);
void indiv_copy(Individual *dst, const Individual *src);
void indiv_two_opt(Individual *ind, const TSP *t);

Population pop_create(int pop_size, int n);
void pop_free(Population *p);
void pop_eval(Population *p, const TSP *t);
int pop_argmin(const Population *p);
int pop_argmax(const Population *p);
void pop_sort_by_fitness(Population *p); // ascending

// selection / crossover / mutation
int tournament_select(const Population *p, int k, unsigned int *rng);
void pmx(const int *p1, const int *p2, int n, int *child, unsigned int *rng);
void mutate_swap(int *perm, int n, double mut_rate, unsigned int *rng);

// one generation (serial)
void ga_step(Population *pop, const TSP *t, const GAParams *P, unsigned int *rng);

#endif // GA_H
