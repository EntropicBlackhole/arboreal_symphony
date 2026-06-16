#include <vector>
#include <string>
#include <iostream>
#include "Tree.hpp"

void generate(Tree harmTreeA, Tree bassTreeA, Tree melTreeA, Tree harmTreeB, Tree bassTreeB, Tree melTreeB, std::vector<Note> seedV, std::string hexSeed) {
  std::string output_file = hexSeed + ".mid";
  float playback_temp = 0.15f;
  float target_duration_beats = 128.0f;

  std::vector<Note> harmA =
      harmTreeA.generateMelody(seedV, target_duration_beats, {},
                               playback_temp); 

  std::vector<Note> bassA =
      bassTreeA.generateMelody(seedV, target_duration_beats, harmA, playback_temp);
  for (auto& n : bassA) n.pitch -= 12;


  std::vector<Note> melA =
      melTreeA.generateMelody(seedV, target_duration_beats, bassA, playback_temp);


  std::vector<Note> harmB =
      harmTreeB.generateMelody(seedV, target_duration_beats, {}, playback_temp * 1.5f);

  std::vector<Note> bassB =
      bassTreeB.generateMelody(seedV, target_duration_beats, harmB, playback_temp * 1.5f);
  for (auto& n : bassB) n.pitch -= 12;

  std::vector<Note> melB =
      melTreeB.generateMelody(seedV, target_duration_beats, bassB, playback_temp * 1.5f);


  std::vector<Note> final_harmony, final_bass, final_melody;

  // section 1: intro (motif A) -> tick offset: 0
  appendTrack(final_harmony, harmA, 0);
  appendTrack(final_bass, bassA, 0);
  appendTrack(final_melody, melA, 0);

  // section 2: verse (mtif A transposed) -> tick offset: 16 beats
  appendTrack(final_harmony, shiftPitch(harmA, 7),
              16 * PPQ);  // +7 = Perfect 5th shift
  appendTrack(final_bass, shiftPitch(bassA, 7), 16 * PPQ);
  appendTrack(final_melody, shiftPitch(melA, 7), 16 * PPQ);

  // section 3: bridge (chaos B) -> tick offset: 32 beats
  appendTrack(final_harmony, harmB, 32 * PPQ);
  appendTrack(final_bass, bassB, 32 * PPQ);
  appendTrack(final_melody, melB, 32 * PPQ);

  // section 4: resolution (motif A original) -> tick offset: 48 beats
  appendTrack(final_harmony, harmA, 48 * PPQ);
  appendTrack(final_bass, bassA, 48 * PPQ);
  appendTrack(final_melody, melA, 48 * PPQ);

  applyHumanDynamics(final_harmony, false);
  applyHumanDynamics(final_bass, false);
  applyHumanDynamics(final_melody, false);

  MiniMidi midiFile;
  midiFile.addTrack(final_harmony, 0, 0);
  midiFile.addTrack(final_bass, 1, 0);
  midiFile.addTrack(final_melody, 2, 0);

  midiFile.save(output_file);
  std::cout << "guardado en " << output_file << std::endl;
}

int main(int argc, char* argv[]) {
  std::string hex_seed = "0003c5040004050200043502"; 
  if (argc > 1) hex_seed = argv[1];
  std::vector<Note> custom_seed = SeedCrypto::decodeSeed(hex_seed);

  Tree harmA, bassA, melA;
  harmA.loadFromFile("models/harmA.tree");
  bassA.loadFromFile("models/bassA.tree");
  melA.loadFromFile("models/melA.tree");

  Tree harmB, bassB, melB;
  harmB.loadFromFile("models/harmB.tree");
  bassB.loadFromFile("models/bassB.tree");
  melB.loadFromFile("models/melB.tree");

  generate(harmA, bassA, melA, harmB, bassB, melB, custom_seed, hex_seed);
}

// int main(int argc, char* argv[]) {
//   float temp = 0.15f;
//   std::string output_file;
//   uint32_t master_seed = 0;
//   bool seed_provided = false;
//   bool output_file_provided = false;

//   for (int i = 1; i < argc; ++i) {
//     std::string arg = argv[i];
//     if (arg.find("--temp=") == 0)
//       temp = std::stof(arg.substr(7));
//     else if (arg.find("--out=") == 0) {
//       output_file = arg.substr(6);
//       output_file_provided = true;
//     } else if (arg.find("--seed=") == 0) {
//       master_seed = std::stoul(arg.substr(7));
//       seed_provided = true;
//     }
//   }