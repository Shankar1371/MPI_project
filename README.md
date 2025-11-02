Parallel Genetic Algorithm for the Traveling Salesman Problem (TSP)

This project implements a Parallel Hybrid Genetic Algorithm (GA) to solve the Traveling Salesman Problem (TSP) using MPI.
It follows an island model, where each MPI process evolves an independent sub-population and periodically migrates its best tours to others.
The algorithm integrates Partially Mapped Crossover (PMX), inversion mutation, and an optional 2-opt local search for improved solution quality.


#### Features

Parallel Execution (MPI Island Model) — distributed populations across multiple MPI ranks.

Top-5% Elite Migration — best individuals broadcast every fixed interval.

PMX Crossover + Inversion Mutation — robust GA operators for permutation problems.

2-Opt Local Search (optional) — refines local routes for shorter tours.

Dynamic Parameters — user-configurable population, generations, crossover/mutation rates.

Benchmark & Plot Scripts — measure runtime, speedup, and parallel efficiency automatically.

Compatible with TSPLIB datasets (berlin52, d198, pr439).


###  Project Structure
mpi-project/
├── src/
│   ├── main.c
│   ├── ga.c
│   ├── ga.h
│   ├── parallel_ga.c
│   └── parallel_ga.h
├── data/
│   ├── berlin52.txt
│   ├── d198.txt
│   └── pr439.txt
├── Makefile
├── bench.sh
├── plot_speedup.py
├── results_berlin52.csv
├── results_d198.csv
├── results_pr439.csv
├── berlin52_speedup.png
├── d198_speedup.png
├── pr439_speedup.png
└── README.md

### Prerequisites

Install dependencies on Ubuntu or WSL:

sudo apt update
sudo apt install -y build-essential openmpi-bin libopenmpi-dev python3-venv python3-pip


Create a Python virtual environment (recommended):

python3 -m venv .venv
source .venv/bin/activate
pip install pandas matplotlib


If you prefer system-wide installs:

sudo apt install -y python3-matplotlib python3-pandas

### Building

Compile using the provided Makefile:

make clean && make


This builds the tsp executable in the project root.

####
 Running Examples
######
Example 1: Serial (no 2-opt, no migration)
mpirun -np 1 ./tsp data/berlin52.txt --generations 200 --pop 100 --no-twoopt --mig-int 0

Example 2: Parallel (4 processes)
mpirun -np 4 ./tsp data/berlin52.txt --generations 400 --pop 200 --no-twoopt --mig-int 0

Example 3: With Migration
mpirun -np 4 ./tsp data/berlin52.txt --generations 400 --pop 200 --mig-int 50

Example 4: With 2-Opt Optimization
mpirun -np 4 ./tsp data/berlin52.txt --generations 400 --pop 200

Example 5: Large Dataset (pr439)
mpirun --oversubscribe -np 8 ./tsp data/pr439.txt --generations 1000 --pop 400 --mig-int 50

### Data File Format

Each dataset must start with the number of cities n, followed by n lines of coordinates:

8
0 0
1 5
5 2
6 6
2 3
8 3
7 8
3 7


#### Datasets like berlin52.txt, d198.txt, and pr439.txt follow this format, converted from TSPLIB.

### Output Example
Parallel GA TSP | islands=4, pop/island=10, gens=200, cx=0.80, mut=0.05, k=4, migInt=50, twoopt=1
Dataset: data/berlin52.txt
[load] n=52
[run] gen=10 best=18940.657075
[run] gen=200 best=10532.739401
Best tour length: 10532.739401
Elapsed (parallel, p=4): 0.0331 s
Best tour: 9 32 50 11 27 26 10 51 13 12 46 ...

#### Benchmarking

Run automated benchmarks for multiple process counts (1, 2, 4, 8, 16, 32):

chmod +x bench.sh
./bench.sh data/berlin52.txt 600 200 results_berlin52.csv
./bench.sh data/d198.txt     1200 400 results_d198.csv
./bench.sh data/pr439.txt    2000 800 results_pr439.csv


If you have fewer cores, add --oversubscribe in bench.sh.

#### Plotting Performance

Generate speedup and efficiency plots:

python3 plot_speedup.py results_berlin52.csv berlin52_
python3 plot_speedup.py results_d198.csv    d198_
python3 plot_speedup.py results_pr439.csv   pr439_


#### Outputs:

berlin52_speedup.png      berlin52_efficiency.png
d198_speedup.png          d198_efficiency.png
pr439_speedup.png         pr439_efficiency.png

Example Results Table
Processes (p)	Elapsed (s)	Speedup	Efficiency
1	0.1285	1.00	1.00
2	0.0701	1.83	0.91
4	0.0332	3.87	0.97
8	0.0475	2.70	0.34
### Evaluating Solution Quality
Dataset	Optimal (TSPLIB)	Best Found	% Error
berlin52	7542	~7700	2.1%
d198	15780	~16150	2.3%
pr439	107217	~110800	3.3%
### Parameters and Tuning
Parameter	Description	Typical Value
--pop	Population size per island	50–200
--generations	Number of generations	500–3000
--cx	Crossover rate	0.7–0.9
--mut	Mutation rate	0.03–0.08
--mig-int	Migration interval (generations)	50–150
--no-twoopt	Disable 2-opt	optional
### Clean Up
make clean


Removes build artifacts and the executable.

### Notes

Run inside WSL2 or a native Linux environment (MPI doesn’t work on Windows paths).

Always rebuild after modifying any .c file.

The GA can be extended to include:

Adaptive migration frequency

Load-balanced subpopulations

Hybrid MPI + OpenMP model

#### Author

Developed by Shankar Punati
Texas A&M University – Corpus Christi
as part of the Parallel Programming Graduate Project (Fall 2025)