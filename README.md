This document explains, in simple words, everything contained in the project’s README. It describes the project goal, structure, datasets, how to run the code, how to benchmark it, how to generate plots, and how each script is used.

1. Project Overview

This project implements a Parallel Hybrid Genetic Algorithm (GA) for solving the Traveling Salesman Problem (TSP).
It uses MPI (Message Passing Interface) to run the GA across multiple processes using an island model.

Each process maintains its own population and periodically exchanges elite individuals with other processes (migration).
The goal is to test the scalability of the GA with different numbers of processes:

p = 1

p = 2

p = 4

p = 8

p = 16

p = 32

The algorithm is tested using four TSPLIB datasets:

berlin52 (52 cities)

d198 (198 cities)

pr439 (439 cities)

pr1002 (1002 cities)

The primary evaluation metrics include:

Best tour length (solution quality)

Runtime for 50 generations

Speedup

Efficiency

Time per generation

The results are visualized using various plots.

2. Key Features of the Algorithm

The project’s GA implements:

Tournament selection (k = 4)

PMX crossover (permutation-safe)

Inversion mutation

Optional 2-opt local search

Island-based parallelism using MPI

Migration of elite tours every few generations

Configurable population size, generations, crossover rate, mutation rate

3. Project Folder Structure
mpi-project/
├── src/
│   ├── main.c
│   ├── ga.c
│   ├── ga.h
│   ├── parallel_ga.c
│   ├── parallel_ga.h
├── data/
│   ├── berlin52.txt
│   ├── d198.txt
│   ├── pr439.txt
│   ├── pr1002.txt
├── bench.sh
├── report_best.sh
├── plot_speedup.py
├── plot_gen_time.py
├── plot_tour.py
├── Makefile
└── README.md


Explanation:

main.c
Handles MPI initialization, argument parsing, running the GA, printing the best tour.

ga.c / ga.h
Implements the serial GA operators (selection, crossover, mutation, 2-opt).

parallel_ga.c / parallel_ga.h
Implements parallel execution, migration, and global reduction.

data/
Contains converted TSPLIB datasets in simple "N + coordinates" format.

bench.sh
Runs the GA for p = 1,2,4,8,16,32 (50 generations), writes CSV results.

plot_speedup.py
Creates speedup and efficiency graphs.

plot_gen_time.py
Creates time-per-generation vs processor-count graphs.

plot_tour.py
Draws the best tour’s route with coordinates.

Makefile
Builds the project.

4. Dataset Format

Your GA uses a simple input format:

N
x0 y0
x1 y1
x2 y2
...


where N = number of cities.

All TSPLIB .tsp files must be converted to this format.

Example:

52
565 575
25 185
345 750
...


You now have:

berlin52.txt

d198.txt

pr439.txt

pr1002.txt

in the correct format.

5. How to Build the Project

Inside your WSL terminal:

make clean && make


This creates the executable:

./tsp

6. Running the Genetic Algorithm

General usage:

mpirun --oversubscribe -np P ./tsp data/<dataset.txt> [options]


Important options:

--pop N → population per island

--generations G → number of generations

--mig-int K → migration interval

--no-twoopt → disable 2-opt

--save-route file → save best tour permutation to a file

Example:

mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt \
  --generations 50 --pop 200 --mig-int 50 --save-route route_berlin52.txt

7. Script: report_best.sh

This script runs each dataset once (gens=50) and prints:

Progress best every 10 generations

Final best tour length

Best tour permutation

It covers:

berlin52

d198

pr439

pr1002

Run it with:

./report_best.sh

8. Script: bench.sh (Updated)

This script generates the data required for graphs by running:

p = 1, 2, 4, 8, 16, 32


for each dataset with 50 generations and writing:

p,elapsed


to a CSV file.

Example:

./bench.sh data/berlin52.txt 400 results_berlin52.csv
./bench.sh data/pr1002.txt 800 results_pr1002.csv

9. Python Environment for Plotting

Create a virtual environment called .plots:

python3 -m venv .plots
source .plots/bin/activate
pip install --upgrade pip
pip install pandas matplotlib

10. Speedup & Efficiency Plots

Convert the benchmark CSVs into graphs:

python3 plot_speedup.py results_berlin52.csv berlin52
python3 plot_speedup.py results_d198.csv d198
python3 plot_speedup.py results_pr439.csv pr439
python3 plot_speedup.py results_pr1002.csv pr1002


Generates:

<dataset>_speedup.png

<dataset>_efficiency.png

11. Time-per-Generation vs Processes Plot

To study scalability deeper:

python3 plot_gen_time.py results_berlin52.csv 50 berlin52
python3 plot_gen_time.py results_pr1002.csv 50 pr1002


Generates:

<dataset>_gen_time.png

12. Plotting Best Tour Paths

After saving the route:

mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt \
  --generations 50 --pop 200 --mig-int 50 --save-route route_berlin52.txt


Now draw the actual tour:

python3 plot_tour.py data/berlin52.txt route_berlin52.txt berlin52_tour.png


Works for all datasets.

13. Known Best (Reference) Tour Lengths

For comparison with assignment values:

berlin52: 7542

d198: 15780

pr439: 107217

pr1002: 259045

Your GA may not reach perfect optimal values with only 50 generations — that is expected.

14. Rebuilding

After modifying any .c or .h file:

make clean && make

15. Requirements and Notes

Must run inside WSL/Linux filesystem (not /mnt/c/)

Requires OpenMPI

Python plotting done inside .plots venv

2-opt is expensive for large datasets (e.g., pr1002)

End of README Explanation

If you want, I can generate this as an actual .txt file using the file generator — just say:

"Create the README explanation txt file"
