import subprocess
import os
import itertools
import time
from pathlib import Path

POPULATIONS = [150, 500, 1000]
GENERATIONS = [2000, 5000, 10000]
TEMPERATURES = [0.1, 0.3, 0.6]
THRESHOLDS = [20, 50]
LAMBDAS = [0.1, 0.5, 1.0]

OUTPUT_DIR = "arboreal_grid_search"

def run_grid_search():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # crea todas las combinaciones posibles de variables
    parameter_combinations = list(itertools.product(
        POPULATIONS, GENERATIONS, TEMPERATURES, THRESHOLDS, LAMBDAS
    ))

    total_runs = len(parameter_combinations)
    print(f"Starting Grid Search: {total_runs} permutations queued.\n")

    for idx, (pop, gens, temp, thresh, lam) in enumerate(parameter_combinations):
        
        filename = f"P{pop}_G{gens}_T{temp}_Th{thresh}_L{lam}.mid"
        filepath = os.path.join(OUTPUT_DIR, filename)

        file = Path(filepath)

        if file.is_file():
            print(f"{filepath} exists, skipping")
            continue

        exe_name = "./train_music.exe" if os.name == 'nt' else "./train_music"
        
        deterministic_seed = 42069 + idx

        command = [
            exe_name,
            f"--pop={pop}",
            f"--gens={gens}",
            f"--threshold={thresh}",
            f"--lambda={lam}",
            f"--seed={deterministic_seed}",
            f"--out={filepath}"
        ]

        print(f"[{idx + 1}/{total_runs}] corriendo {' '.join(command)}")
        
        start_time = time.time()
        try:
            subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            elapsed = time.time() - start_time
            print(f"  -> terminado en {elapsed:.2f}s")
            
        except subprocess.CalledProcessError:
            print(f"  -> error, fallo en permutacion {filename}")


if __name__ == "__main__":
    run_grid_search()