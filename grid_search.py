import subprocess
import os
import itertools
import time
from pathlib import Path
import concurrent.futures

# --- NEW HYPERPARAMETERS ---
THEMES = [
    "The_Hollow_Knight_Duel", 
    "The_Abyssal_Waltz", 
    "The_Baroque_Invention"
]

POPULATIONS = [150, 500, 1000]
GENERATIONS = [2000, 5000, 10000]
TEMPERATURES = [0.1, 0.3, 0.6]
THRESHOLDS = [20, 50]
LAMBDAS = [0.1, 0.5, 1.0]

OUTPUT_DIR = "arboreal_grid_search"

# Dynamically determine the maximum number of concurrent threads 
# based on the host machine's CPU cores to prevent bottlenecking.
MAX_WORKERS = os.cpu_count() or 4 

def run_single_permutation(args):
    """
    Worker function to execute a single C++ subprocess.
    """
    idx, theme, pop, gens, temp, thresh, lam = args
    
    # We prefix the filename with the theme to keep outputs organized
    filename = f"{theme}_P{pop}_G{gens}_T{temp}_Th{thresh}_L{lam}.mid"
    filepath = os.path.join(OUTPUT_DIR, filename)

    if Path(filepath).is_file():
        return f"[{idx}] {filename} exists, skipping."

    exe_name = "./train_music.exe" if os.name == 'nt' else "./train_music"
    
    # Deterministic seed shifts per run to ensure unique procedural generation
    deterministic_seed = 42069 + idx

    # FIXED: Added --theme and --temperature to the command structure
    command = [
        exe_name,
        f"--theme={theme}",
        f"--pop={pop}",
        f"--gens={gens}",
        f"--temperature={temp}", 
        f"--threshold={thresh}",
        f"--lambda={lam}",
        f"--seed={deterministic_seed}",
        f"--out={filepath}"
    ]

    start_time = time.time()
    try:
        # Run the C++ binary. stdout/stderr are routed to DEVNULL to keep the console clean 
        # while multiple threads run concurrently.
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elapsed = time.time() - start_time
        return f"[{idx}] Success: {filename} in {elapsed:.2f}s"
    except subprocess.CalledProcessError:
        return f"[{idx}] ERROR: Failed on {filename}"

def run_grid_search():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # Cartesian product of all hyperparameters
    parameter_combinations = list(itertools.product(
        THEMES, POPULATIONS, GENERATIONS, TEMPERATURES, THRESHOLDS, LAMBDAS
    ))

    # Package the index with the parameters so the worker function can use it for the seed
    tasks = [(idx + 1, *params) for idx, params in enumerate(parameter_combinations)]
    total_runs = len(tasks)
    
    print(f"Starting Optimized Grid Search: {total_runs} permutations queued.")
    print(f"Allocating {MAX_WORKERS} concurrent CPU threads...\n")

    # Execute the subprocesses in parallel
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        # executor.map handles the queue and returns results as they finish
        for result in executor.map(run_single_permutation, tasks):
            print(result)
            
    print("\nGrid search complete.")

if __name__ == "__main__":
    run_grid_search()