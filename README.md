# Parallel Hybrid Genetic Algorithm for the Traveling Salesman Problem (TSP)

This project implements an MPI-based parallel **Hybrid Genetic Algorithm (GA)** for the Traveling Salesman Problem.  
It follows an island model where each MPI process (island) evolves its own population and periodically migrates elite tours to the other islands.  
A 2-opt local search step can be enabled to refine tours.

The implementation is designed for experiments with different process counts (p = 1, 2, 4, 8, 16, 32) and fixed generation count (typically 50), measuring:

- Best tour length (solution quality)
- Runtime and time per generation
- Parallel speedup and efficiency

---

## Features

- Island-model parallel GA using **MPI**
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

### Building
Inside WSL/Ubuntu:


sudo apt update
sudo apt install -y build-essential openmpi-bin libopenmpi-dev python3-venv


### Clone and build:
make clean && make

### This produces the executable:

./tsp


### Running the Parallel GA
General form:


mpirun --oversubscribe -np <p> ./tsp <dataset> [options...]
Important options:

--pop <N> population size per island

--generations <G> number of generations

--mig-int <k> migration interval in generations (0 = disabled)

--no-twoopt disable 2-opt local search

--cx <rate> crossover rate (default 0.8)

--mut <rate> mutation rate (default 0.05)

--seed <s> random seed

--save-route <file> write the best tour permutation to a file

### Example: small run on berlin52:


mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt \
  --generations 50 \
  --pop 200 \
  --mig-int 50 \
  --save-route route_berlin52.txt


### Typical console output:


Parallel GA TSP | islands=4, pop/island=200, gens=50, cx=0.80, mut=0.05, k=4, migInt=50, twoopt=1
Dataset: data/berlin52.txt
[load] opening data/berlin52.txt
[load] n=52
[load] coords read ok; building dist matrix...
[load] done
[run] islands=4 pop/island=200 gens=50 twoopt=1 migInt=50
[run] gen=10 best=7544.365902
[run] gen=20 best=7544.365902
...
[run] gen=50 best=7544.365902
Best tour length: 7544.365902
Elapsed (parallel, p=4): 0.0277 s
Best tour: 21 0 48 31 44 18 40 ...
One-Shot Best-Tour Experiments (gens = 50)
To match the project requirement of 50 generations per dataset, the script report_best.sh runs:

pr439.txt with gens=50

berlin52.txt with gens=50

d198.txt with gens=50

pr1002.txt with gens=50

# to see all the outputs see the output.... .txt file that has the data from the terminal

#command

bash -c '
GENS=50

run_dataset () {
  dataset=$1
  pop=$2
  outfile=$3

  echo "=== Running $dataset ==="
  echo "" > "$outfile"

  for p in 1 2 4 8 16 32; do
    echo "---------------------------------------" | tee -a "$outfile"
    echo ">>> DATASET: $dataset   (p=$p)" | tee -a "$outfile"
    echo "---------------------------------------" | tee -a "$outfile"

    mpirun --oversubscribe -np "$p" ./tsp "data/$dataset.txt" \
      --generations "$GENS" \
      --pop "$pop" \
      --mig-int 50 \
      --cx 0.8 \
      --mut 0.05 \
      --k 4 \
      2>&1 | tee -a "$outfile"

    echo "" | tee -a "$outfile"
  done
}

run_dataset "berlin52" 200 "output_berlin52.txt"
run_dataset "d198" 400 "output_d198.txt"
run_dataset "pr439" 800 "output_pr439.txt"
run_dataset "pr1002" 1200 "output_pr1002.txt"

echo "===================================="
echo "All datasets complete!"
echo "Generated:"
echo " output_berlin52.txt"
echo " output_d198.txt"
echo " output_pr439.txt"
echo " output_pr1002.txt"
echo "===================================="
'



### Example:

chmod +x report_best.sh
./report_best.sh


This prints, for each dataset:

Dataset name

Number of cities

Progress of best fitness every 10 generations

Final best tour length and best tour permutation

You can compare these lengths with the known best values given above.

#### Benchmarking: p = 1, 2, 4, 8, 16, 32 (gens = 50)
To measure scaling, use the updated bench.sh:


chmod +x bench.sh

# 50 generations are hard-coded inside bench.sh
./bench.sh data/berlin52.txt 400  results_berlin52.csv
./bench.sh data/d198.txt     800  results_d198.csv
./bench.sh data/pr439.txt    800  results_pr439.csv
./bench.sh data/pr1002.txt   800  results_pr1002.csv


#### Each CSV will contain:

text
Copy code
p,elapsed
1,0.1285
2,0.0701
4,0.0332
8,0.0475
16, ...
32, ...
where elapsed is the total runtime (seconds) for 50 generations.

Python Environment for Plots
Create a small plotting virtual environment:

bash
Copy code
python3 -m venv .plots
source .plots/bin/activate
pip install --upgrade pip
pip install pandas matplotlib
All plotting commands below assume this environment is activated ((.plots) in the shell prompt).

Speedup and Efficiency Plots
You can turn the results_*.csv files into speedup and efficiency plots:

bash
Copy code
python3 plot_speedup.py results_berlin52.csv berlin52
python3 plot_speedup.py results_d198.csv     d198
python3 plot_speedup.py results_pr439.csv    pr439
python3 plot_speedup.py results_pr1002.csv   pr1002
Each call will print the derived columns and save:

<prefix>_speedup.png

<prefix>_efficiency.png

Speedup is defined as:

text
Copy code
S(p)  = T1 / Tp
Efficiency:

text
Copy code
E(p)  = S(p) / p
Time-Per-Generation vs Processes (New Plot)
To directly show how time per generation changes with the number of processes, use:

bash
Copy code
python3 plot_gen_time.py results_berlin52.csv 50 berlin52
python3 plot_gen_time.py results_d198.csv     50 d198
python3 plot_gen_time.py results_pr439.csv    50 pr439
python3 plot_gen_time.py results_pr1002.csv   50 pr1002
This script:

Reads p,elapsed from the CSV

Computes gen_time = elapsed / generations

Produces <prefix>_gen_time.png with:

x-axis: processes p (1, 2, 4, 8, 16, 32)

y-axis: time per generation in seconds

You can use these plots to analyze scalability for each dataset separately.

Plotting the Best Tour Route
After running ./tsp with --save-route, you can visualize the tour:


# Example for berlin52
mpirun --oversubscribe -np 4 ./tsp data/berlin52.txt \
  --generations 50 --pop 200 --mig-int 50 \
  --save-route route_berlin52.txt

python3 plot_tour.py data/berlin52.txt route_berlin52.txt berlin52_tour.png


### The resulting image shows:

City coordinates as points

The best tour as a polygonal path

Reported tour length in the title

Repeat for d198, pr439, and pr1002 using their respective route files.

Implementation Notes
The TSP loader is robust to comments and blank lines, and precomputes a full distance matrix for speed.

The PMX crossover and inversion mutation are implemented in a permutation-safe way and include checks against invalid offspring.

Migration uses an island model:

Each island evolves independently for several generations.

Every migration_interval generations, the top 5% individuals are gathered and the global best tour is broadcast back to all islands.

2-opt can be computationally expensive for large instances (especially pr1002), so population sizes and process counts may need tuning.

### How to Rebuild After Code Changes
Whenever you update *.c or *.h files:


make clean && make
Environment and Assumptions
Tested on WSL2 / Ubuntu with OpenMPI.

Source and data files live inside the Linux filesystem (e.g., /home/.../mpi-project), not under /mnt/c/... to avoid MPI I/O overhead.

Plots are generated using a small local Python venv (.plots) to keep dependencies isolated.