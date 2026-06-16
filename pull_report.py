import json
import os
import sys

def build_audit(run_id):
    symphony_path = os.path.join("json_symphonies", f"{run_id}.json")
    if not os.path.exists(symphony_path):
        print(f"[error] {run_id} no fue encontrado")
        return

    with open(symphony_path, 'r') as f:
        data = json.load(f)

    telemetry = data.get("telemetry", {})
    notes = data.get("notes", [])

    os.makedirs("reports", exist_ok=True)
    out_file = os.path.join("reports", f"{run_id}_audit.txt")
    
    with open(out_file, 'w') as f:
        f.write(f"=== REPORTE {run_id} ===\n")
        f.write("-" * 45 + "\n")
        f.write(">>> PARAMETROS <<<\n")
        for k, v in telemetry.items():
            f.write(f"{k.ljust(20)}: {v}\n")
            
        f.write("-" * 45 + "\n")
        f.write(">>> RESUMEN DE NOTAS <<<\n")
        f.write(f"total de notas       : {len(notes)}\n")
        
        if notes:
            f.write(f"rango de tessitura  : {min(n['noteNumber'] for n in notes)} to {max(n['noteNumber'] for n in notes)}\n")
            f.write(f"duracion en ticks  : {notes[-1]['absoluteTick']}\n")

            tick_counts = {}
            for n in notes:
                tick_counts[n['absoluteTick']] = tick_counts.get(n['absoluteTick'], 0) + 1
            
            chord_events = sum(1 for v in tick_counts.values() if v > 1)
            f.write(f"polifonia  : {chord_events} (ticks con notas sobrepuestas)\n")

            beats = notes[-1]['absoluteTick'] / 480
            density = len(notes) / beats if beats > 0 else 0
            f.write(f"densidad ritmica  : {density:.2f} notas por beat\n")

        f.write("-" * 45 + "\n")
        f.write(">>> datos adicionales <<<\n")
        f.write(json.dumps(notes, indent=4))

    print(f"reporte escrito a {out_file}")

if __name__ == "__main__":
    if len(sys.argv) == 2:
        build_audit(sys.argv[1])