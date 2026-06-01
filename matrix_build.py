import mido
import glob
import numpy as np
import os

MATRIX_SIZE = 25

def clamp(val, min_val, max_val):
    return max(min_val, min(val, max_val))

def build_markov_matrix(midi_folder):
    transition_counts = np.zeros((MATRIX_SIZE, MATRIX_SIZE))
    midi_files = glob.glob(os.path.join(midi_folder, "*.mid"))
    if not midi_files:
        print("CRITICAL: No MIDI files found in", midi_folder)
        return None
    print(f"Processing {len(midi_files)} files...")
    for file in midi_files:
        try:
            mid = mido.MidiFile(file)
            for track in mid.tracks:
                pitches = []
                for msg in track:
                    if msg.type == 'note_on' and msg.velocity > 0:
                        pitches.append(msg.note)
                
                if len(pitches) < 3:
                    continue
                    
                for i in range(2, len(pitches)):
                    prev_jump = pitches[i-1] - pitches[i-2]
                    curr_jump = pitches[i] - pitches[i-1]
                    
                    prev_idx = clamp(prev_jump, -12, 12) + 12
                    curr_idx = clamp(curr_jump, -12, 12) + 12
                    
                    transition_counts[prev_idx][curr_idx] += 1
        except Exception as e:
            print(f"Failed parsing {file}: {e}")

    row_sums = transition_counts.sum(axis=1)
    
    row_sums[row_sums == 0] = 1 
    transition_matrix = transition_counts / row_sums[:, np.newaxis]
    
    return transition_matrix

def export_to_hpp(matrix, filename="HumanMatrix.hpp"):
    with open(filename, 'w') as f:
        f.write("#pragma once\n")
        f.write("#include <array>\n\n")
        f.write("// matriz markov de transicion de midis de data humana autogenerada\n")
        f.write("constexpr std::array<std::array<float, 25>, 25> HUMAN_MATRIX = {{\n")
        for row in matrix:
            f.write("    {")
            f.write(", ".join(f"{val:.4f}f" for val in row))
            f.write("},\n")
        f.write("}};\n")
    print(f"matriz exportada a {filename}")

if __name__ == "__main__":
    folder = "training_datasets/bach" # descargue todo de bach xd
    if not os.path.exists(folder):
        os.makedirs(folder)
        print(f"'{folder}' creado, coloca los midis en este folder")
    else:
        matrix = build_markov_matrix(folder)
        if matrix is not None:
            export_to_hpp(matrix)