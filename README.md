# Parallel Genetic Algorithm for TSP (Open MPI)

- **Island model** with periodic **migration** (best individual gathered and broadcast).
- **GA ops**: tournament selection (k=4), **PMX** crossover, swap mutation, optional **2-opt** local search.
- Measures total runtime via `MPI_Wtime()`.

## Build
```bash
make
