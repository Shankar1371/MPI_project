#include "ga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

/* ===========================
   Tiny portable RNG helpers
   =========================== */
static inline unsigned rng_next_u32(unsigned *state) {
    unsigned x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x ? x : 0x9E3779B1u;
    return *state;
}
static inline int rng_randint_inclusive(unsigned *state, int hi_inclusive) {
    return (int)(rng_next_u32(state) % (unsigned)(hi_inclusive + 1));
}
static inline int rng_randint(unsigned *state, int n) { // [0, n)
    return (int)(rng_next_u32(state) % (unsigned)n);
}
static inline double rng_rand01(unsigned *state) {      // [0,1)
    return rng_next_u32(state) / 4294967296.0;
}

/* ===========================
   TSP I/O and utilities
   =========================== */
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
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
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
        if (*p == '\0' || *p == '#') continue;
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

/* ===========================
   Individual / Population
   =========================== */
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
    for (int i = n - 1; i > 0; i--) {
        int j = rng_randint(rng, i + 1);   // 0..i
        int tmp = a[i]; a[i] = a[j]; a[j] = tmp;
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

/* sort by fitness (ascending) */
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

/* ===========================
   Operators
   =========================== */
int tournament_select(const Population *p, int k, unsigned int *rng){
    int best = rng_randint(rng, p->pop_size);
    for(int i=1;i<k;i++){
        int r = rng_randint(rng, p->pop_size);
        if(p->inds[r].fitness < p->inds[best].fitness) best = r;
    }
    return best;
}

static inline int find_pos(const int *perm, int n, int val) {
    for (int i = 0; i < n; i++) if (perm[i] == val) return i;
    return -1;
}

/* Proper PMX: copy [a,b] from p1; map p2 genes until landing outside [a,b]; fill holes with p2 */
void pmx(const int *p1, const int *p2, int n, int *child, unsigned int *rng){
    int a = rng_randint(rng, n);
    int b = rng_randint(rng, n);
    if (a > b) { int t = a; a = b; b = t; }

    for (int i = 0; i < n; i++) child[i] = -1;

    /* 1) copy segment from p1 */
    for (int i = a; i <= b; i++) child[i] = p1[i];

    /* 2) map p2 segment genes into holes outside [a,b] */
    for (int i = a; i <= b; i++) {
        int g = p2[i];

        /* skip if already present in copied segment */
        int present = 0;
        for (int k = a; k <= b; k++) { if (child[k] == g) { present = 1; break; } }
        if (!present) {
            /* find where g ultimately maps to */
            int pos = i;
            while (pos >= a && pos <= b) {
                int p1_val_at_pos = p1[pos];
                pos = find_pos(p2, n, p1_val_at_pos);
                if (pos < 0) break; /* safety */
            }
            if (pos >= 0 && pos < n && child[pos] == -1) child[pos] = g;
        }
    }

    /* 3) fill remaining holes with p2 in order */
    for (int i = 0; i < n; i++) {
        if (child[i] == -1) child[i] = p2[i];
    }
}

/* Inversion mutation (rubric requirement) */
void mutate_inversion(int *perm, int n, double mut_rate, unsigned int *rng) {
    if (rng_rand01(rng) < mut_rate) {
        int i = rng_randint(rng, n);
        int j = rng_randint(rng, n);
        if (i > j) { int tmp = i; i = j; j = tmp; }
        while (i < j) {
            int tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
            i++; j--;
        }
    }
}

/* --------- 2-opt local search ---------- */
static inline double edge(const TSP *t, int a, int b){ return t->dist[(size_t)a*t->n + b]; }

void indiv_two_opt(Individual *ind, const TSP *t){
    const int n = ind->n;
    const double EPS = 1e-12;

    int improved = 1;
    while (improved) {
        improved = 0;

        for (int i = 0; i < n - 1; i++) {
            int a = ind->perm[i];
            int b = ind->perm[(i + 1) % n];

            for (int j = i + 2; j < n; j++) {
                int c = ind->perm[j];
                int d = ind->perm[(j + 1) % n];
                if (j == i) continue;

                double old_len = t->dist[(size_t)a*t->n + b] + t->dist[(size_t)c*t->n + d];
                double new_len = t->dist[(size_t)a*t->n + c] + t->dist[(size_t)b*t->n + d];
                double delta = new_len - old_len;

                if (delta < -EPS) {
                    /* reverse segment (i+1 .. j) */
                    for (int u = i + 1, v = j; u < v; u++, v--) {
                        int tmp = ind->perm[u]; ind->perm[u] = ind->perm[v]; ind->perm[v] = tmp;
                    }
                    improved = 1;
                    goto restart_scan;   /* first-improvement: restart outer loop */
                }
            }
        }
        restart_scan: ;
    }
    ind->fitness = tsp_tour_length(t, ind->perm);
}



/* ===========================
   Validation helpers
   =========================== */
static int perm_is_valid(const int *perm, int n) {
    int *seen = (int*)calloc((size_t)n, sizeof(int));
    if (!seen) return 0;
    for (int i = 0; i < n; i++) {
        int v = perm[i];
        if (v < 0 || v >= n || seen[v]) { free(seen); return 0; }
        seen[v] = 1;
    }
    free(seen);
    return 1;
}

/* ===========================
   GA step (µ+λ with elitism)
   =========================== */
void ga_step(Population *pop, const TSP *t, const GAParams *P, unsigned int *rng){
    /* evaluate & sort by fitness */
    pop_eval(pop, t);
    pop_sort_by_fitness(pop);

    /* keep top 2 elites */
    int n = pop->n;
    Population next = pop_create(pop->pop_size, n);
    indiv_copy(&next.inds[0], &pop->inds[0]);
    indiv_copy(&next.inds[1], &pop->inds[1]);

    /* produce children */
    for(int i=2;i<pop->pop_size;i++){
        int pidx1 = tournament_select(pop, P->tournament_k, rng);
        int pidx2 = tournament_select(pop, P->tournament_k, rng);
        const int *P1 = pop->inds[pidx1].perm;
        const int *P2 = pop->inds[pidx2].perm;

        Individual child = indiv_create(n);

        double r = rng_rand01(rng);
        if(r < P->cx_rate){
            pmx(P1, P2, n, child.perm, rng);
        } else {
            memcpy(child.perm, P1, sizeof(int)*n);
        }

        mutate_inversion(child.perm, n, P->mut_rate, rng);

        /* ensure permutation validity BEFORE 2-opt / fitness */
        if (!perm_is_valid(child.perm, n)) {
            /* repair: shuffle a parent copy */
            memcpy(child.perm, P1, sizeof(int)*n);
            shuffle(child.perm, n, rng);
        }

        child.fitness = tsp_tour_length(t, child.perm);

/* Apply 2-opt only to elites to keep runtime reasonable */
    int elite_cap = pop->pop_size / 10;         /* top 10% */
    if (elite_cap < 1) elite_cap = 1;
    if (P->use_two_opt && i < 2 + elite_cap) {  /* keep 2 hard elites, optimize next best */
    indiv_two_opt(&child, t);
    } else {
    /* keep the computed fitness */
    }


        indiv_copy(&next.inds[i], &child);
        indiv_free(&child);
    }

    /* replace */
    for(int i=0;i<pop->pop_size;i++) indiv_copy(&pop->inds[i], &next.inds[i]);
    pop_free(&next);
}
