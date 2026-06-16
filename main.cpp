#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "Tree.hpp"
#include "matrices/abyss_matrix.hpp"
#include "matrices/aristocrat_matrix.hpp"
#include "matrices/duelist_matrix.hpp"
// #include "matrices/upbeat_matrix.hpp"

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

const std::vector<SymphonyBlueprint> MAESTRO_REGISTRY = {
    {
        "The_Hollow_Knight_Duel",
        140,  
        {LOG_ARISTOCRAT_LEFT, LOG_ARISTOCRAT_RIGHT, 0.1f,
         0.6f},  
        {LOG_DUELIST_LEFT, LOG_DUELIST_RIGHT, 0.4f,
         0.2f}  
    },
    {
        "The_Abyssal_Waltz",
        85,  
        {LOG_ABYSS_LEFT, LOG_ABYSS_RIGHT, 0.05f,
         0.8f},  
        {LOG_DUELIST_LEFT, LOG_DUELIST_RIGHT, 0.15f,
         0.5f}  
    }};


void appendPerfectCadence(std::vector<Note>& harmony, std::vector<Note>& bass,
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

template <typename FitnessFunc>
Tree runGA(const std::string& phaseName, int popSize, int generations,
           float temp, FitnessFunc fitnessFunc) {
  std::vector<Tree> population(popSize);
  for (auto& t : population) t.generateRandomBranch(4, 0);

  Tree bestTree;
  std::uniform_int_distribution<int> parent_dist(0, 9);

  auto chunkStartTime = std::chrono::high_resolution_clock::now();

  for (int gen = 0; gen < generations; ++gen) {
    for (auto& t : population) t.fitness_score = fitnessFunc(t);

    std::sort(population.begin(), population.end(),
              [](const Tree& a, const Tree& b) {
                return a.fitness_score > b.fitness_score;
              });

    bestTree = population[0];
    if (gen > 0 && gen % (std::max(1, generations / 10)) == 0) {
      auto now = std::chrono::high_resolution_clock::now();
      std::cout << phaseName << " | gen " << gen
                << " | puntaje: " << bestTree.fitness_score
                << " | size: " << bestTree.nodes.size() << " | time: "
                << std::chrono::duration<double>(now - chunkStartTime).count()
                << "s" << std::endl;

      chunkStartTime = std::chrono::high_resolution_clock::now();
    }

    std::vector<Tree> nextGen;
    nextGen.reserve(popSize);
    nextGen.push_back(population[0]);
    nextGen.push_back(population[1]);

    while (nextGen.size() < popSize) {
      const Tree& mom = population[parent_dist(getGlobalRNG())];
      const Tree& dad = population[parent_dist(getGlobalRNG())];
      Tree child = Evolver::crossover(mom, dad);
      child.mutate(temp);
      nextGen.push_back(child);
    }
    population = std::move(nextGen);
  }
  return bestTree;
}

int main(int argc, char* argv[]) {
  int popSize = 1000;
  int generations = 10000;  // 5000 seems more optimal
  float temp = 0.1f;
  float playback_temp = 0.15f;
  int threshold = 30;
  float lambda = 0.5f;
  int theme_index = 0;

  std::string output_file;

  uint32_t master_seed = 0;
  bool seed_provided = false;
  bool output_file_provided = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--gens=") == 0)
      generations = std::stoi(arg.substr(7));
    else if (arg.find("--temp=") == 0)
      temp = std::stof(arg.substr(7));
    else if (arg.find("--pop=") == 0)
      popSize = std::stoi(arg.substr(6));
    else if (arg.find("--threshold=") == 0)
      threshold = std::stoi(arg.substr(12));
    else if (arg.find("--lambda=") == 0)
      lambda = std::stof(arg.substr(9));
    else if (arg.find("--out=") == 0) {
      output_file = arg.substr(6);
      output_file_provided = true;
    } else if (arg.find("--seed=") == 0) {
      master_seed = std::stoul(arg.substr(7));
      seed_provided = true;
    } else if (arg.find("--theme=") == 0) {
      theme_index = std::stoi(arg.substr(8));
    }
  }

  if (!seed_provided) {
    master_seed = std::random_device{}();
  }

  if (!output_file_provided) {
    output_file = "./symphonies/G" + std::to_string(generations);
    output_file += "_T" + std::to_string(temp);
    output_file += "_P" + std::to_string(popSize);
    output_file += "_Th" + std::to_string(threshold);
    output_file += "_L" + std::to_string(lambda);
    output_file += "_S" + std::to_string(master_seed) + ".mid";
  }

  getGlobalRNG().seed(master_seed);

  Evolver::PARSIMONY_THRESHOLD = threshold;
  Evolver::PARSIMONY_LAMBDA = lambda;

  std::cout << "--pop=" << popSize << " --gens=" << generations
            << " --temp=" << temp << " --threshold=" << threshold
            << " --lambda=" << lambda << " --seed=" << master_seed
            << "--theme=" << theme_index << "\n";

  std::vector<Note> seedV = {
      Note(60, 1.0f, 80), Note(64, 0.5f, 80), Note(67, 0.5f, 80),
      Note(72, 1.0f, 80), Note(67, 0.5f, 80), Note(64, 0.5f, 80),
      Note(60, 1.0f, 80), Note(62, 0.5f, 80), Note(64, 0.5f, 80),
      Note(65, 0.5f, 80), Note(67, 1.0f, 80), Note(69, 0.5f, 80),
      Note(71, 0.5f, 80), Note(72, 1.0f, 80), Note(67, 0.5f, 80),
      Note(64, 0.5f, 80), Note(60, 1.0f, 80), Note(60, 1.0f, 80)};

  auto start_time = std::chrono::high_resolution_clock::now();

  theme_index =
      std::clamp(theme_index, 0, static_cast<int>(MAESTRO_REGISTRY.size()) - 1);
  const SymphonyBlueprint& active_theme = MAESTRO_REGISTRY[theme_index];

  std::cout << "tema: " << active_theme.theme_name
            << " a " << active_theme.target_bpm << " bpm\n";

  
  std::cout << "\n[Training Motif A - Harmony]\n";
  Tree harmTreeA = runGA("HarmA", popSize, generations,
                         active_theme.verse_chip.optimal_temperature,
                         [&seedV, &active_theme](const Tree& t) {
                           return Evolver::calculateHarmonyFitness(
                               t, seedV, active_theme.verse_chip.left_matrix);
                         });
  std::vector<Note> harmA =
      harmTreeA.generateMelody(seedV, 16.0f, {}, playback_temp);

  std::cout << "\n[Training Motif A - Bass]\n";
  Tree bassTreeA =
      runGA("BassA", popSize, generations,
            active_theme.verse_chip.optimal_temperature,
            [&seedV, &harmA, &active_theme](const Tree& t) {
              return Evolver::calculateBassFitness(
                  t, seedV, harmA, active_theme.verse_chip.left_matrix);
            });
  std::vector<Note> bassA =
      bassTreeA.generateMelody(seedV, 16.0f, harmA, playback_temp);
  for (auto& n : bassA) n.pitch -= 12;

  std::cout << "\n[Training Motif A - Melody]\n";
  Tree melTreeA = runGA(
      "MelA", popSize, generations, active_theme.verse_chip.optimal_temperature,
      [&seedV, &bassA, &active_theme](const Tree& t) {
        return Evolver::calculateMelodyFitness(
            t, seedV, bassA, active_theme.verse_chip.right_matrix);
      });
  std::vector<Note> melA =
      melTreeA.generateMelody(seedV, 16.0f, bassA, playback_temp);

  
  std::cout << "\n[Training Bridge B - Chaos]\n";

  Tree harmTreeB = runGA("HarmB", popSize, generations,
                         active_theme.bridge_chip.optimal_temperature,
                         [&seedV, &active_theme](const Tree& t) {
                           return Evolver::calculateHarmonyFitness(
                               t, seedV, active_theme.bridge_chip.left_matrix);
                         });
  std::vector<Note> harmB =
      harmTreeB.generateMelody(seedV, 16.0f, {}, playback_temp * 1.5f);

  Tree bassTreeB =
      runGA("BassB", popSize, generations,
            active_theme.bridge_chip.optimal_temperature,
            [&seedV, &harmB, &active_theme](const Tree& t) {
              return Evolver::calculateBassFitness(
                  t, seedV, harmB, active_theme.bridge_chip.left_matrix);
            });
  std::vector<Note> bassB =
      bassTreeB.generateMelody(seedV, 16.0f, harmB, playback_temp * 1.5f);
  for (auto& n : bassB) n.pitch -= 12;

  Tree melTreeB =
      runGA("MelB", popSize, generations,
            active_theme.bridge_chip.optimal_temperature,
            [&seedV, &bassB, &active_theme](const Tree& t) {
              return Evolver::calculateMelodyFitness(
                  t, seedV, bassB, active_theme.bridge_chip.right_matrix);
            });
  std::vector<Note> melB =
      melTreeB.generateMelody(seedV, 16.0f, bassB, playback_temp * 1.5f);

  std::cout << "\n[Assembling Symphony]\n";

  std::vector<Note> final_harmony, final_bass, final_melody;

  appendTrack(final_harmony, harmA, 0);
  appendTrack(final_bass, bassA, 0);
  appendTrack(final_melody, melA, 0);

  appendTrack(final_harmony, shiftPitch(harmA, 7),
              16 * PPQ);
  appendTrack(final_bass, shiftPitch(bassA, 7), 16 * PPQ);
  appendTrack(final_melody, shiftPitch(melA, 7), 16 * PPQ);

  appendTrack(final_harmony, harmB, 32 * PPQ);
  appendTrack(final_bass, bassB, 32 * PPQ);
  appendTrack(final_melody, melB, 32 * PPQ);

  appendTrack(final_harmony, harmA, 48 * PPQ);
  appendTrack(final_bass, bassA, 48 * PPQ);
  appendTrack(final_melody, melA, 48 * PPQ);

  appendPerfectCadence(final_harmony, final_bass, final_melody, 64 * PPQ, 60);

  applyHumanDynamics(final_harmony, false);
  applyHumanDynamics(final_bass, false);
  applyHumanDynamics(final_melody, false);

  auto end_time = std::chrono::high_resolution_clock::now();
  std::cout << "\n completado en "
            << std::chrono::duration<double>(end_time - start_time).count()
            << " s\n";

  MiniMidi midiFile;
  midiFile.setBPM(active_theme.target_bpm);
  midiFile.addTrack(final_harmony, 0, 0);
  midiFile.addTrack(final_bass, 1, 0);
  midiFile.addTrack(final_melody, 2, 0);

  // midiFile.save(output_file);

  std::string hex_seed = SeedCrypto::encodeSeed(seedV);

  auto epoch = std::chrono::system_clock::now().time_since_epoch();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
  std::string runID = hex_seed.substr(0, 8) + "_" + std::to_string(popSize) +
                      "_" + std::to_string(generations) + "G_" +
                      std::to_string(static_cast<int>(temp * 100)) + "T_" +
                      std::to_string(threshold) + "Th" +
                      std::to_string(timestamp);

  std::string base_dir = "trees/" + runID;
  std::string models_dir = base_dir + "/models";
  fs::create_directories(models_dir);

  harmTreeA.saveToFile(models_dir + "/harmA.tree");
  bassTreeA.saveToFile(models_dir + "/bassA.tree");
  melTreeA.saveToFile(models_dir + "/melA.tree");

  harmTreeB.saveToFile(models_dir + "/harmB.tree");
  bassTreeB.saveToFile(models_dir + "/bassB.tree");
  melTreeB.saveToFile(models_dir + "/melB.tree");

  std::ofstream report(base_dir + "/report.txt");
  if (report.is_open()) {
    report << "training time: "
           << std::chrono::duration<double>(end_time - start_time).count()
           << " s" << "\n";
    // report << "training data: mixed corpus\n";
    report << "theme: " << active_theme.theme_name << "\n";
    report << "bpm: " << active_theme.target_bpm << "\n";
    report << "generations: " << generations << "\n";
    report << "temperature: " << temp << "\n";
    report << "population: " << popSize << "\n";
    report << "threshold: " << threshold << "\n";
    report << "lambda: " << lambda << "\n";
    report << "seed: " << master_seed << "\n";
    report << "hex_seed: " << hex_seed << "\n";
    report.close();
  }

  std::string output_midi = base_dir + "/" + runID + ".mid";
  midiFile.save(output_midi);
  std::cout << "guardado en " << output_file << std::endl;

  std::cout << "[IO] Triggering Telemetry Daemon for RunID: " << runID
            << std::endl;
  fs::create_directories("json_symphonies");

  std::string command = "python midi_to_json.py " + output_midi;
  std::system(command.c_str());
  return 0;
}
