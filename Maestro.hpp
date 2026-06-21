#pragma once
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Tree.hpp"
#include "matrices/abyss_matrix.hpp"
#include "matrices/aristocrat_matrix.hpp"
#include "matrices/classical_matrix.hpp"
#include "matrices/duelist_matrix.hpp"

namespace fs = std::filesystem;

using LogMatrixType = const std::array<std::array<float, 25>, 25>&;

struct EmotionChip {
  LogMatrixType left_matrix;
  LogMatrixType right_matrix;
  float optimal_temperature;
  float optimal_lambda;
};

struct SymphonyBlueprint {
  std::string theme_name;
  int target_bpm;
  EmotionChip verse_chip;
  EmotionChip bridge_chip;
};

namespace Maestro {

const std::vector<SymphonyBlueprint> REGISTRY = {
    {"The_Hollow_Knight_Duel",
     140,
     {LOG_DUELIST_LEFT, LOG_DUELIST_RIGHT, 0.1f, 0.6f},
     {LOG_DUELIST_LEFT, LOG_DUELIST_RIGHT, 0.4f, 0.2f}},
    {"The_Abyssal_Waltz",
     85,
     {LOG_ABYSS_LEFT, LOG_ABYSS_RIGHT, 0.05f, 0.8f},
     {LOG_DUELIST_LEFT, LOG_DUELIST_RIGHT, 0.15f, 0.5f}},
    {"The_Baroque_Invention",
     110,
     {LOG_CLASSICAL_LEFT, LOG_CLASSICAL_RIGHT, 0.02f, 0.85f},
     {LOG_CLASSICAL_LEFT, LOG_CLASSICAL_RIGHT, 0.1f, 0.6f}}};

inline void appendPerfectCadence(std::vector<Note>& harmony,
                                 std::vector<Note>& bass,
                                 std::vector<Note>& melody, int start_tick,
                                 int root_pitch) {
  int dom = root_pitch + 7;

  bass.push_back(Note(dom - 12, 2.0f, 95, start_tick));
  harmony.push_back(Note(dom, 2.0f, 85, start_tick));
  harmony.push_back(Note(dom + 4, 2.0f, 85, start_tick));
  melody.push_back(Note(dom + 12, 2.0f, 100, start_tick));

  int res_tick = start_tick + (PPQ * 2);

  bass.push_back(Note(root_pitch - 24, 4.0f, 110, res_tick));
  bass.push_back(Note(root_pitch - 12, 4.0f, 100, res_tick));

  harmony.push_back(Note(root_pitch, 4.0f, 85, res_tick));
  harmony.push_back(Note(root_pitch + 7, 4.0f, 85, res_tick));

  melody.push_back(Note(root_pitch + 12, 4.0f, 110, res_tick + (PPQ / 4)));
}

inline void appendStingerCadence(std::vector<Note>& harmony,
                                 std::vector<Note>& bass,
                                 std::vector<Note>& melody, int start_tick,
                                 int root_pitch) {
  int dom = root_pitch + 7;

  bass.push_back(Note(dom - 12, 0.5f, 100, start_tick));
  harmony.push_back(Note(dom, 0.5f, 90, start_tick));
  melody.push_back(Note(dom + 12, 0.5f, 100, start_tick));

  int tick_2 = start_tick + (PPQ / 2);
  bass.push_back(Note(dom - 12, 0.5f, 100, tick_2));
  harmony.push_back(Note(dom, 0.5f, 90, tick_2));

  int tick_3 = start_tick + PPQ;
  bass.push_back(Note(dom - 12, 0.5f, 110, tick_3));
  harmony.push_back(Note(dom, 0.5f, 95, tick_3));
  melody.push_back(Note(dom + 12, 0.5f, 110, tick_3));

  int final_tick = start_tick + (PPQ * 2);
  bass.push_back(Note(root_pitch - 24, 1.0f, 127, final_tick));
  bass.push_back(Note(root_pitch - 12, 1.0f, 127, final_tick));

  harmony.push_back(Note(root_pitch, 1.0f, 110, final_tick));
  harmony.push_back(Note(root_pitch + 7, 1.0f, 110, final_tick));

  melody.push_back(Note(root_pitch + 12, 1.0f, 127, final_tick));
  melody.push_back(Note(root_pitch + 24, 1.0f, 127, final_tick));
}

inline void generate(Tree& harmTreeA, Tree& bassTreeA, Tree& melTreeA,
                     Tree& harmTreeB, Tree& bassTreeB, Tree& melTreeB,
                     const std::vector<Note>& seedV, const std::string& hexSeed,
                     const SymphonyBlueprint& active_theme,
                     const std::string& output_file) {
  float tempV = active_theme.verse_chip.optimal_temperature;
  float tempB = active_theme.bridge_chip.optimal_temperature;

  auto getSeed = [](const std::vector<Note>& track,
                    const std::vector<Note>& fallback) {
    if (track.size() >= 3)
      return std::vector<Note>(track.end() - 3, track.end());
    return fallback;
  };

  auto scaleVelocity = [](std::vector<Note> track, float multiplier) {
    for (auto& n : track) {
      n.intensity =
          std::clamp(static_cast<int>(n.intensity * multiplier), 0, 127);
    }
    return track;
  };

  std::cout << "\n[maestro] ejecutando ensamble continuo (80 beats)..."
            << std::endl;

  std::vector<Note> h1 =
      harmTreeA.generateMelody(seedV, 16.0f, {}, tempV, true);
  std::vector<Note> b1 =
      bassTreeA.generateMelody(seedV, 16.0f, h1, tempV, false);
  for (auto& n : b1) n.pitch -= 12;
  std::vector<Note> m1 =
      melTreeA.generateMelody(seedV, 16.0f, b1, tempV, false);

  std::vector<Note> h2 =
      harmTreeA.generateMelody(getSeed(h1, seedV), 16.0f, {}, tempV, true);
  std::vector<Note> b2 =
      bassTreeA.generateMelody(getSeed(b1, seedV), 16.0f, h2, tempV, false);
  for (auto& n : b2) n.pitch -= 12;
  std::vector<Note> m2 =
      melTreeA.generateMelody(getSeed(m1, seedV), 16.0f, b2, tempV, false);

  std::vector<Note> h3 =
      harmTreeA.generateMelody(getSeed(h2, seedV), 16.0f, {}, tempV, true);
  std::vector<Note> b3 =
      bassTreeA.generateMelody(getSeed(b2, seedV), 16.0f, h3, tempV, false);
  for (auto& n : b3) n.pitch -= 12;
  std::vector<Note> m3 =
      melTreeA.generateMelody(getSeed(m2, seedV), 16.0f, b3, tempV, false);

  std::vector<Note> h4 =
      harmTreeB.generateMelody(getSeed(h3, seedV), 16.0f, {}, tempB, true);
  std::vector<Note> b4 =
      bassTreeB.generateMelody(getSeed(b3, seedV), 16.0f, h4, tempB, false);
  for (auto& n : b4) n.pitch -= 12;
  std::vector<Note> m4 =
      melTreeB.generateMelody(getSeed(m3, seedV), 16.0f, b4, tempB, false);

  std::vector<Note> h5 =
      harmTreeA.generateMelody(getSeed(h4, seedV), 16.0f, {}, tempV, true);
  std::vector<Note> b5 =
      bassTreeA.generateMelody(getSeed(b4, seedV), 16.0f, h5, tempV, false);
  for (auto& n : b5) n.pitch -= 12;
  std::vector<Note> m5 =
      melTreeA.generateMelody(getSeed(m4, seedV), 16.0f, b5, tempV, false);

  std::vector<Note> final_harmony, final_bass, final_melody;

  appendTrack(final_melody, scaleVelocity(m1, 0.7f), 0);

  appendTrack(final_bass, scaleVelocity(b2, 0.9f), 16 * PPQ);
  appendTrack(final_melody, scaleVelocity(m2, 0.9f), 16 * PPQ);

  appendTrack(final_harmony, scaleVelocity(h3, 0.85f), 32 * PPQ);
  appendTrack(final_bass, b3, 32 * PPQ);
  appendTrack(final_melody, m3, 32 * PPQ);

  appendTrack(final_harmony, scaleVelocity(shiftPitch(h4, 7), 1.15f), 48 * PPQ);
  appendTrack(final_bass, scaleVelocity(shiftPitch(b4, 7), 1.15f), 48 * PPQ);
  appendTrack(final_melody, scaleVelocity(shiftPitch(m4, 7), 1.15f), 48 * PPQ);

  appendTrack(final_harmony, scaleVelocity(h5, 0.6f), 64 * PPQ);
  appendTrack(final_melody, scaleVelocity(m5, 0.8f), 64 * PPQ);

  if (active_theme.target_bpm > 110) {
    appendStingerCadence(final_harmony, final_bass, final_melody, 80 * PPQ, 60);
  } else {
    appendPerfectCadence(final_harmony, final_bass, final_melody, 80 * PPQ, 60);
  }

  applyHumanDynamics(final_harmony, false);
  applyHumanDynamics(final_bass, false);
  applyHumanDynamics(final_melody, false);

  MiniMidi midiFile;
  midiFile.setBPM(active_theme.target_bpm);

  midiFile.addTrack(final_harmony, 0, 0);
  midiFile.addTrack(final_bass, 1, 0);
  midiFile.addTrack(final_melody, 2, 0);

  int total_duration = 82 * PPQ;
  midiFile.addSustainPedal(total_duration, 0);
  midiFile.addSustainPedal(total_duration, 1);
  midiFile.addSustainPedal(total_duration, 2);

  midiFile.save(output_file);
  midiFile.save("current.mid");
  std::cout << "[maestro] guardado en " << output_file << std::endl;

  std::string command = "python midi_to_json.py " + output_file;
  std::system(command.c_str());
}
}  // namespace Maestro