# Parallel Genetic Algorithm for the Traveling Salesman Problem (TSP)

This project implements a **Parallel Genetic Algorithm (GA)** to solve the Traveling Salesman Problem using **MPI**.  
It follows an island model where each MPI process (island) evolves its own population and periodically migrates its best individuals to other islands.  
Optional **2-opt local search** improves solution quality.

---

## Features

- Parallel execution with MPI (island model)
- Robust PMX crossover and swap mutation
- 2-opt local optimization (optional)
- Configurable parameters for population, generations, migration, crossover, and mutation
- Real-time progress logging
- Benchmark and plotting scripts to measure speedup and efficiency

---

## Project Structure

mpi-project/
├── src/
│ ├── main.c
│ ├── ga.c
│ ├── ga.h
│ ├── parallel_ga.c
│ └── parallel_ga.h
├── data/
│ └── cities.txt
├── Makefile
├── bench.sh
├── plot_speedup.py
└── README.md


---

## Prerequisites

Install the following on Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y build-essential openmpi-bin libopenmpi-dev python3-pip
pip3 install pandas matplotlib

Building

Compile the project using the Makefile:

make clean && make


This creates the executable tsp in the project root.

Running
Example 1: Single process, no 2-opt, no migration
mpirun -np 1 ./tsp data/cities.txt --generations 5 --pop 20 --no-twoopt --mig-int 0

Example 2: Four processes (parallel)
mpirun -np 4 ./tsp data/cities.txt --generations 20 --pop 10 --no-twoopt --mig-int 0

Example 3: With migration
mpirun -np 4 ./tsp data/cities.txt --generations 100 --pop 30 --no-twoopt --mig-int 50

Example 4: With 2-opt enabled
mpirun -np 1 ./tsp data/cities.txt --generations 50 --pop 40

Data File Format

Each dataset file should begin with the number of cities n, followed by n lines of coordinates:

8
0 0
1 5
5 2
6 6
2 3
8 3
7 8
3 7

Output Example
Parallel GA TSP | islands=4, pop/island=10, gens=20, cx=0.80, mut=0.05, k=4, migInt=50, twoopt=0
Dataset: data/cities.txt
[load] opening data/cities.txt
[load] n=8
[load] coords read ok; building dist matrix...
[load] done
[run] islands=4 pop/island=10 gens=20 twoopt=0 migInt=50
[run] gen=10 best=23.970563
[run] gen=20 best=23.455844
Best tour length: 23.455844
Elapsed (parallel, p=4): 0.0049 s
Best tour: 5 2 0 4 1 7 6 3

Benchmarking and Plots

The project includes a benchmarking script and a plotting utility to measure performance scaling.

Run Benchmarks
chmod +x bench.sh
./bench.sh data/cities.txt 300 800 3 results.csv

Generate Speedup and Efficiency Plots
python3 plot_speedup.py results.csv


Expected outputs:

speedup.png

efficiency.png

Evaluating Performance
Parameter	Description	Typical Value
--pop	Population size per island	50–200
--generations	Number of generations	100–1000
--cx	Crossover rate	0.7–0.9
--mut	Mutation rate	0.03–0.08
--mig-int	Migration interval (generations)	50–150
--no-twoopt	Disable local 2-opt search	optional

Use different process counts (-np 1, 2, 4, 8, ...) to observe parallel speedup.

Clean Up

To remove build files:

make clean

Notes

Run inside WSL or a native Linux environment.

Avoid /mnt/c/... Windows paths, as they block MPI execution.

Always rebuild after modifying ga.c, parallel_ga.c, or the Makefile.

Author

Developed by Shankar Punati as part of a parallel programming project at Texas A&M University–Corpus Christi.


---