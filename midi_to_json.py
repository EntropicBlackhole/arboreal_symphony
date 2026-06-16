import mido
import json
import sys
import os

def parse_telemetry(midi_path, custom_out_path=None):
    dir_name = os.path.dirname(midi_path)
    base_name = os.path.basename(midi_path)
    run_id = os.path.splitext(base_name)[0]

    report_data = {}
    report_path = os.path.join(dir_name, "report.txt")
    if os.path.exists(report_path):
        with open(report_path, 'r') as f:
            for line in f:
                if ':' in line:
                    key, val = line.split(':', 1)
                    report_data[key.strip()] = val.strip()

    try:
        mid = mido.MidiFile(midi_path)
    except Exception as e:
        print(f"Error loading MIDI: {e}")
        return

    notes_data = []
    for track_idx, track in enumerate(mid.tracks):
        absolute_tick = 0
        for msg in track:
            absolute_tick += msg.time
            if msg.type == 'note_on' and msg.velocity > 0:
                notes_data.append({
                    "track": track_idx,
                    "absoluteTick": absolute_tick,
                    "noteNumber": msg.note,
                    "velocity": msg.velocity
                })
    notes_data = sorted(notes_data, key=lambda x: x['absoluteTick'])

    treelist_path = "trees/treelist.json"
    treelist = {}
    if os.path.exists(treelist_path):
        with open(treelist_path, 'r') as f:
            try:
                treelist = json.load(f)
            except json.JSONDecodeError:
                pass

    treelist[run_id] = report_data
    with open(treelist_path, 'w') as f:
        json.dump(treelist, f, indent=4)

    if not custom_out_path:
        custom_out_path = os.path.join("json_symphonies", f"{run_id}.json")
    
    os.makedirs(os.path.dirname(custom_out_path), exist_ok=True)
    
    final_payload = {
        "run_id": run_id,
        "telemetry": report_data,
        "notes": notes_data
    }

    with open(custom_out_path, 'w') as f:
        json.dump(final_payload, f, indent=4)
        
    print(f"{run_id} guardado a treelist.json")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("uso: python midi_to_json.py <ubicacion_de_midi> [ubicacion de salida opcional]")
    else:
        out_path = sys.argv[2] if len(sys.argv) > 2 else None
        parse_telemetry(sys.argv[1], out_path)