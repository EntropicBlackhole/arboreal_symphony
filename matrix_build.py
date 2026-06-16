import mido
import glob
import numpy as np
import os

MATRIX_SIZE = 25

def clamp(val, min_val, max_val):
    return max(min_val, min(val, max_val))

def extract_matrices_from_theme(midi_folder):
    left_counts = np.zeros((MATRIX_SIZE, MATRIX_SIZE))
    right_counts = np.zeros((MATRIX_SIZE, MATRIX_SIZE))
    
    midi_files = glob.glob(os.path.join(midi_folder, "*.mid"))
    if not midi_files:
        return None, None
        
    print(f"procesando {len(midi_files)} archivos en {midi_folder}")
    for file in midi_files:
        try:
            mid = mido.MidiFile(file)
            for track in mid.tracks:
                left_pitches = []
                right_pitches = []
                
                for msg in track:
                    if msg.type == 'note_on' and msg.velocity > 0:
                        if msg.note < 62: 
                            left_pitches.append(msg.note)
                        else:
                            right_pitches.append(msg.note)
                
                if len(left_pitches) >= 3:
                    for i in range(2, len(left_pitches)):
                        p_jump = clamp(left_pitches[i-1] - left_pitches[i-2], -12, 12) + 12
                        c_jump = clamp(left_pitches[i] - left_pitches[i-1], -12, 12) + 12
                        left_counts[p_jump][c_jump] += 1
                        
                if len(right_pitches) >= 3:
                    for i in range(2, len(right_pitches)):
                        p_jump = clamp(right_pitches[i-1] - right_pitches[i-2], -12, 12) + 12
                        c_jump = clamp(right_pitches[i] - right_pitches[i-1], -12, 12) + 12
                        right_counts[p_jump][c_jump] += 1
                        
        except Exception as e:
            print(f"error en {file}: {e}")

    epsilon = 0.1 
    left_counts += epsilon
    right_counts += epsilon

    left_matrix = left_counts / left_counts.sum(axis=1)[:, np.newaxis]
    right_matrix = right_counts / right_counts.sum(axis=1)[:, np.newaxis]
    
    return left_matrix, right_matrix

def export_theme_to_hpp(theme_name, left_matrix, right_matrix, out_folder="matrices"):
    os.makedirs(out_folder, exist_ok=True)
    filename = os.path.join(out_folder, f"{theme_name}_matrix.hpp")
    
    safe_name = theme_name.upper().replace(" ", "_")
    
    with open(filename, 'w') as f:
        f.write("#pragma once\n")
        f.write("#include <array>\n\n")
        
        f.write(f"constexpr std::array<std::array<float, 25>, 25> LOG_{safe_name}_LEFT = {{{{\n")
        for row in left_matrix:
            f.write("    {" + ", ".join(f"{np.log(val):.4f}f" for val in row) + "},\n")
        f.write("}};\n\n")
        
        f.write(f"constexpr std::array<std::array<float, 25>, 25> LOG_{safe_name}_RIGHT = {{{{\n")
        for row in right_matrix:
            f.write("    {" + ", ".join(f"{np.log(val):.4f}f" for val in row) + "},\n")
        f.write("}};\n")
        
    print(f"exportando headers de {safe_name} a {filename}")

if __name__ == "__main__":
    themes_dir = "training_datasets/themes"
    if not os.path.exists(themes_dir):
        os.makedirs(themes_dir)
        print(f"'{themes_dir}' fue creado, crea subfolderes para cada tema y coloca midis");
    else:
        for item in os.listdir(themes_dir):
            theme_path = os.path.join(themes_dir, item)
            if os.path.isdir(theme_path):
                left_mat, right_mat = extract_matrices_from_theme(theme_path)
                if left_mat is not None:
                    export_theme_to_hpp(item, left_mat, right_mat)