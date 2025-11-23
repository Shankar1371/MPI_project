# Parallel Hybrid Genetic Algorithm for the Traveling Salesman Problem (TSP)

This project implements an MPI-based parallel Hybrid Genetic Algorithm (GA) for the Traveling Salesman Problem.  
It follows an island model where each MPI process (island) evolves its own population and periodically migrates elite tours to the other islands.  
A 2-opt local search step can be enabled to refine tours.

The implementation is designed for experiments with different process counts (p = 1, 2, 4, 8, 16, 32) and fixed generation count (typically 50), measuring:

- Best tour length (solution quality)
- Runtime and time per generation
- Parallel speedup and efficiency

---

## Features

- Island-model parallel GA using MPI
- Operators:
  - Tournament selection (k = 4)
  - PMX crossover (robust, permutation-safe)
  - Inversion mutation
  - Optional 2-opt local search
- Configurable parameters via command line:
  - Population size per island
  - Number of generations
  - Crossover and mutation rates
  - Migration interval
  - 2-opt on/off
  - Random seed
- Robust TSP loader:
  - Simple “N + coordinates” format
  - Works with converted TSPLIB instances
- Benchmark scripts:
  - `bench.sh` automatically runs p = 1,2,4,8,16,32 and logs runtimes to CSV
  - `plot_speedup.py` plots speedup and efficiency
  - `plot_gen_time.py` plots **time per generation vs processes**
- Tour plotting:
  - `plot_tour.py` draws the best tour route over the city coordinates
- Multi-dataset support:
  - `berlin52`, `d198`, `pr439`, `pr1002`

---


## project overview
 This project implements:

- Genetic Operators

- Tournament selection (k = 4)

- PMX crossover (permutation-safe)

- Inversion mutation

- Optional 2-opt local search

Parallelization Model

- MPI island model

- Processes evolve independently

- Every --mig-int generations, top 5% - migrate

- Global best is synchronized

Datasets used

All converted from TSPLIB:

- berlin52 — 52 cities

- d198 — 198 cities

- pr439 — 439 cities

- pr1002 — 1002 cities

All datasets run for 50 generations, as required.

## Project Structure

mpi-project/
├── src/
│   ├── main.c            # argument parsing, MPI setup, top-level driver
│   ├── ga.c              # serial GA logic and operators (PMX, mutation, 2-opt)
│   ├── ga.h
│   ├── parallel_ga.c     # MPI island model, migration, global best reduction
│   └── parallel_ga.h
├── data/
│   ├── berlin52.txt      # 52-city instance (converted from TSPLIB)
│   ├── d198.txt          # 198-city instance
│   ├── pr439.txt         # 439-city instance
│   └── pr1002.txt        # 1002-city instance (converted from TSPLIB)
├── Makefile
├── bench.sh              # benchmark script, p = 1,2,4,8,16,32 → CSV
├── report_best.sh        # runs each dataset once (gens=50) and prints best tours
├── plot_speedup.py       # speedup / efficiency plots from CSV
├── plot_gen_time.py      # time-per-generation vs processes plots from CSV
├── plot_tour.py          # draw best tour for a dataset
└── README.md


## Dataset Format
The GA uses a simple coordinate format:


Copy code
N
x0 y0
x1 y1
...
x(N-1) y(N-1)
Example (berlin52.txt):

text
Copy code
52
565 575
25 185
345 750
945 685
...
All TSPLIB .tsp instances (e.g., berlin52.tsp, d198.tsp, pr439.tsp, pr1002.tsp) are first converted to this format and stored as .txt inside data/.

### Best known tour lengths (from the assignment):

berlin52.tsp: 7542

d198.tsp: 15780

pr439.tsp: 107217

pr1002.tsp: 259045

The GA’s results can be compared against these reference values.


## 2 How to build the project

### Install dependecies

sudo apt update
sudo apt install -y build-essential openmpi-bin libopenmpi-dev python3-venv

### compile

make clean && make

#### this creates the executable:

./tsp

## How to Run the Algorithm (Simple Example)

mpirun --oversubscribe -np <p> ./tsp data/<dataset>.txt --generations 50 --pop <size> --mig-int 50

### example
mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt --generations 50 --pop 200 --mig-int 50

#### this prints:
Generation progress (10,20,...50)

Best tour length

Best tour (permutation)

Execution time

## run all the p=1,2,4,8,16,32 for every dataset

./report_best.sh

#### It generates:

output_berlin52.txt

output_d198.txt

output_pr439.txt

output_pr1002.txt

Each contains:

All generations

Best tour length

Best tour route

Runtime for each p

## benchmarking (CSV file)
for speedup and  efficeincy plots:

### Commands:
./bench.sh data/berlin52.txt 400  results_berlin52.csv
./bench.sh data/d198.txt     800  results_d198.csv
./bench.sh data/pr439.txt    800  results_pr439.csv
./bench.sh data/pr1002.txt   1200 results_pr1002.csv

###### each csv contains
p,elapsed
1,0.1285
2,0.0701
4,0.0332
8,0.0475
16,...
32,...


## Enable the plotting environment

python3 -m venv .plots
source .plots/bin/activate
pip install pandas matplotlib


## speedup &  effieciency plots

python3 plot_speedup.py results_berlin52.csv berlin52
python3 plot_speedup.py results_d198.csv     d198
python3 plot_speedup.py results_pr439.csv    pr439
python3 plot_speedup.py results_pr1002.csv   pr1002

### generated ouputs

berlin52_speedup.png

berlin52_efficiency.png

...(same for others)

## generation time vs processort plot

python3 plot_gen_time.py results_berlin52.csv 50 berlin52
python3 plot_gen_time.py results_d198.csv     50 d198
python3 plot_gen_time.py results_pr439.csv    50 pr439
python3 plot_gen_time.py results_pr1002.csv   50 pr1002

##### Generated:

<prefix>_gen_time.png

## plotting Best Tour routes

mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt \
  --generations 50 --pop 200 --mig-int 50 \
  --save-route route_berlin52.txt

python3 plot_tour.py data/berlin52.txt route_berlin52.txt berlin52_tour.png


## file output Overview

### Run logs

output_berlin52.txt

output_d198.txt

output_pr439.txt

output_pr1002.txt

### Population logs

_run.txt

### Benchmark CSV

results_*.csv

### Plots

*_speedup.png

*_efficiency.png

*_gen_time.png

*_tour.png

### Routes

route_*.txt

## Clean and Rebuild

make clean && make
