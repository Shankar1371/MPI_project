#include "ga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>


// --- tiny portable RNG ---
static inline unsigned rng_next_u32(unsigned *state) {
    // xorshift32
    unsigned x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x ? x : 0x9E3779B1u;
    return *state;
}
static inline int rng_randint(unsigned *state, int hi_inclusive) {
    return (int)(rng_next_u32(state) % (unsigned)(hi_inclusive + 1));
}
static inline double rng_rand01(unsigned *state) {
    return (rng_next_u32(state) / 4294967296.0); // [0,1)
}




static inline double euclid(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2, dy = y1 - y2;
    return sqrt(dx*dx + dy*dy);
}

// Robust loader with progress + clear errors.
 

TSP *tsp_load_from_file(const char *path) {
    setvbuf(stdout, NULL, _IOLBF, 0);  // line-buffer stdout for immediate prints
    fprintf(stdout, "[load] opening %s\n", path);

    errno = 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[load][ERROR] fopen(%s): %s\n", path, strerror(errno));
        return NULL;
    }

    // --- read city count n, skipping blank/comment lines ---
    int n = -1;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        // skip leading whitespace
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        // skip comments and blank lines
        if (*p == '\0' || *p == '#') continue;
        if (sscanf(p, "%d", &n) == 1) break;
        fprintf(stderr, "[load][ERROR] first non-comment token must be an integer city count\n");
        fclose(f);
        return NULL;
    }
    if (n <= 0) {
        fprintf(stderr, "[load][ERROR] invalid or missing city count\n");
        fclose(f);
        return NULL;
    }
    fprintf(stdout, "[load] n=%d\n", n);

    // --- allocate TSP ---
    TSP *t = calloc(1, sizeof(TSP));
    if (!t) { fprintf(stderr, "[load][ERROR] OOM TSP\n"); fclose(f); return NULL; }
    t->n = n;

    t->coords = malloc(sizeof(double) * 2 * n);
    if (!t->coords) { fprintf(stderr, "[load][ERROR] OOM coords\n"); fclose(f); free(t); return NULL; }

    // --- read n coordinate lines (x y), skipping blank/comment lines ---
    int read_pts = 0;
    while (read_pts < n && fgets(line, sizeof line, f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;  // skip blank/comment
        double x, y;
        if (sscanf(p, "%lf %lf", &x, &y) == 2) {
            t->coords[2*read_pts]   = x;
            t->coords[2*read_pts+1] = y;
            read_pts++;
        } else {
            fprintf(stderr, "[load][ERROR] malformed coord line near city %d (expected: \"x y\")\n",
                    read_pts + 1);
            fclose(f); free(t->coords); free(t); return NULL;
        }
    }
    fclose(f);

    if (read_pts != n) {
        fprintf(stderr, "[load][ERROR] expected %d coordinates, read %d\n", n, read_pts);
        free(t->coords); free(t); return NULL;
    }
    fprintf(stdout, "[load] coords read ok; building dist matrix...\n");

    // --- build distance matrix ---
    t->dist = calloc((size_t)n * (size_t)n, sizeof(double));
    if (!t->dist) {
        fprintf(stderr, "[load][ERROR] OOM distance matrix %dx%d\n", n, n);
        free(t->coords); free(t); return NULL;
    }
    for (int i = 0; i < n; i++) {
        double x1 = t->coords[2*i], y1 = t->coords[2*i+1];
        for (int j = 0; j < n; j++) {
            double x2 = t->coords[2*j], y2 = t->coords[2*j+1];
            double dx = x1 - x2, dy = y1 - y2;
            t->dist[(size_t)i * (size_t)n + (size_t)j] = sqrt(dx*dx + dy*dy);
        }
    }
    fprintf(stdout, "[load] done\n");
    return t;
}



void tsp_free(TSP *t){
    if(!t) return;
    free(t->coords);
    free(t->dist);
    free(t);
}

double tsp_tour_length(const TSP *t, const int *perm){
    double L=0.0; int n=t->n;
    for(int i=0;i<n;i++){
        int a=perm[i], b=perm[(i+1)%n];
        L += t->dist[(size_t)a*t->n + b];
    }
    return L;
}

// ---------------- Individual/Population ----------------
Individual indiv_create(int n){
    Individual x; 
    x.n=n; 
    x.perm = malloc(sizeof(int)*n); 
    x.fitness = INFINITY; 
    return x;
}
void indiv_free(Individual *ind){ 
    if(ind && ind->perm){ free(ind->perm); ind->perm=NULL; }
}
void indiv_copy(Individual *d, const Individual *s){
    memcpy(d->perm, s->perm, sizeof(int)*s->n); 
    d->fitness=s->fitness; 
    d->n=s->n;
}

static void shuffle(int *a, int n, unsigned int *rng){
    for(int i=n-1;i>0;i--){
        int j = (int)(rand_r(rng) % (unsigned)(i+1));
        int tmp=a[i]; a[i]=a[j]; a[j]=tmp;
    }
}
void indiv_randomize(Individual *ind, unsigned int *rng){
    for(int i=0;i<ind->n;i++) ind->perm[i]=i;
    shuffle(ind->perm, ind->n, rng);
}

Population pop_create(int pop_size, int n){
    Population p; 
    p.n=n; 
    p.pop_size=pop_size; 
    p.inds = malloc(sizeof(Individual)*pop_size);
    for(int i=0;i<pop_size;i++){ p.inds[i]=indiv_create(n); }
    return p;
}
void pop_free(Population *p){
    for(int i=0;i<p->pop_size;i++) indiv_free(&p->inds[i]);
    free(p->inds); 
    p->inds=NULL;
}
void pop_eval(Population *p, const TSP *t){
    for(int i=0;i<p->pop_size;i++){
        p->inds[i].fitness = tsp_tour_length(t, p->inds[i].perm);
    }
}
int pop_argmin(const Population *p){
    int b=0; 
    for(int i=1;i<p->pop_size;i++) 
        if(p->inds[i].fitness < p->inds[b].fitness) b=i; 
    return b;
}
int pop_argmax(const Population *p){
    int w=0; 
    for(int i=1;i<p->pop_size;i++) 
        if(p->inds[i].fitness > p->inds[w].fitness) w=i; 
    return w;
}

// insertion sort by fitness (ascending)

static int cmp_indiv(const void *a, const void *b) {
    const Individual *ia = (const Individual*)a;
    const Individual *ib = (const Individual*)b;
    if (ia->fitness < ib->fitness) return -1;
    if (ia->fitness > ib->fitness) return 1;
    return 0;
}

void pop_sort_by_fitness(Population *p){
    qsort(p->inds, (size_t)p->pop_size, sizeof(Individual), cmp_indiv);
}


// ---------------- Operators ----------------
int tournament_select(const Population *p, int k, unsigned int *rng){
    int best = (int)(rand_r(rng) % (unsigned)p->pop_size);
    for(int i=1;i<k;i++){
        int r = (int)(rand_r(rng) % (unsigned)p->pop_size);
        if(p->inds[r].fitness < p->inds[best].fitness) best = r;
    }
    return best;
}

// helpers (top of ga.c if not present)
static int find_pos(const int *perm, int n, int val) {
    for (int i = 0; i < n; i++) if (perm[i] == val) return i;
    return -1;
}

// Safe PMX: maps until index leaves [a,b], avoids infinite cycles and OOB
void pmx(const int *p1, const int *p2, int n, int *child, unsigned int *rng){
    // choose cut points (use your rng; or replace rng_randint with your rand/rand_r)
    int a = (int)(rng ? (unsigned)rng[0] % n : rand() % n);
    int b = (int)(rng ? ((unsigned)rng[0] * 1103515245u + 12345u) % n : rand() % n);
    if (a > b) { int t = a; a = b; b = t; }

    for (int i = 0; i < n; i++) child[i] = -1;

    // 1) copy segment from p1
    for (int i = a; i <= b; i++) child[i] = p1[i];

    // 2) place p2’s genes: map until pos exits [a,b]
    for (int i = a; i <= b; i++) {
        int val = p2[i];

        // if already present in the segment, skip
        int present = 0;
        for (int k = a; k <= b; k++) if (child[k] == val) { present = 1; break; }
        if (present) continue;

        int pos = find_pos(p1, n, val);
        // follow mapping while pos inside [a,b]
        while (pos >= a && pos <= b) {
            val = p2[pos];
            pos = find_pos(p1, n, val);
            if (pos < 0 || pos >= n) break; // safety
        }
        if (pos >= 0 && pos < n && child[pos] == -1) {
            child[pos] = p2[i];
        }
    }

    // 3) fill remaining from p2
    for (int i = 0; i < n; i++) {
        if (child[i] == -1) child[i] = p2[i];
    }
}



void mutate_swap(int *perm, int n, double mut_rate, unsigned int *rng){
    if(((double)rand_r(rng) / RAND_MAX) < mut_rate){
        int i = (int)(rand_r(rng) % (unsigned)n);
        int j = (int)(rand_r(rng) % (unsigned)n);
        int t=perm[i]; perm[i]=perm[j]; perm[j]=t;
    }
}

// --------- 2-opt local search ----------
static inline double edge(const TSP *t, int a, int b){ return t->dist[(size_t)a*t->n + b]; }

void indiv_two_opt(Individual *ind, const TSP *t){
    int n = ind->n;
    int improved = 1;
    while(improved){
        improved = 0;
        for(int i=0;i<n-1;i++){
            int a = ind->perm[i];
            int b = ind->perm[(i+1)%n];
            for(int j=i+2;j<n;j++){
                int c = ind->perm[j];
                int d = ind->perm[(j+1)%n];
                if (j == i) continue;
                double delta = (edge(t,a,c) + edge(t,b,d)) - (edge(t,a,b) + edge(t,c,d));
                if(delta < -1e-9){
                    // reverse segment (i+1 .. j)
                    for(int u=i+1,v=j; u<v; u++,v--){
                        int tmp=ind->perm[u]; ind->perm[u]=ind->perm[v]; ind->perm[v]=tmp;
                    }
                    improved = 1;
                }
            }
        }
    }
    ind->fitness = tsp_tour_length(t, ind->perm);
}

// One generational step (µ+λ style with elitism)
void ga_step(Population *pop, const TSP *t, const GAParams *P, unsigned int *rng){
    // evaluate & sort by fitness
    pop_eval(pop, t);
    pop_sort_by_fitness(pop);

    // keep top 2 elites
    int n = pop->n;
    Population next = pop_create(pop->pop_size, n);
    indiv_copy(&next.inds[0], &pop->inds[0]);
    indiv_copy(&next.inds[1], &pop->inds[1]);

    // produce children
    for(int i=2;i<pop->pop_size;i++){
        int p1 = tournament_select(pop, P->tournament_k, rng);
        int p2 = tournament_select(pop, P->tournament_k, rng);
        Individual child = indiv_create(n);

        double r = (double)rand_r(rng)/RAND_MAX;
        if(r < P->cx_rate){
            pmx(pop->inds[p1].perm, pop->inds[p2].perm, n, child.perm, rng);
        } else {
            memcpy(child.perm, pop->inds[p1].perm, sizeof(int)*n);
        }

        mutate_swap(child.perm, n, P->mut_rate, rng);

        if(P->use_two_opt) {
            child.fitness = tsp_tour_length(t, child.perm);
            indiv_two_opt(&child, t);
        } else {
            child.fitness = tsp_tour_length(t, child.perm);
        }

        indiv_copy(&next.inds[i], &child);
        indiv_free(&child);
    }

    // replace
    for(int i=0;i<pop->pop_size;i++) indiv_copy(&pop->inds[i], &next.inds[i]);
    pop_free(&next);
}
