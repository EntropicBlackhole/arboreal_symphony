#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "Maestro.hpp"
#include "Tree.hpp"
#include "matrices/abyss_matrix.hpp"
#include "matrices/aristocrat_matrix.hpp"
#include "matrices/duelist_matrix.hpp"
// #include "matrices/upbeat_matrix.hpp"

namespace fs = std::filesystem;

using LogMatrixType = const std::array<std::array<float, 25>, 25>&;

template <typename FitnessFunc>
Tree runGA(const std::string& phaseName, int popSize, int generations,
           float temp, FitnessFunc fitnessFunc) {
  std::vector<Tree> population(popSize);
  for (auto& t : population) t.generateRandomBranch(4, 0);

  Tree bestTree;
  std::uniform_int_distribution<int> parent_dist(0, 9);

  auto chunkStartTime = std::chrono::high_resolution_clock::now();

  float previous_best_fitness = -99999.0f;
  int stagnation_counter = 0;

  for (int gen = 0; gen < generations; ++gen) {
    for (auto& t : population) t.fitness_score = fitnessFunc(t);

    std::sort(population.begin(), population.end(),
              [](const Tree& a, const Tree& b) {
                return a.fitness_score > b.fitness_score;
              });

    bestTree = population[0];

    if (std::abs(bestTree.fitness_score - previous_best_fitness) < 0.1f) {
      stagnation_counter++;
    } else {
      stagnation_counter = 0;
      previous_best_fitness = bestTree.fitness_score;
    }

    // --- MASS EXTINCTION EVENT ---
    if (stagnation_counter >= 50) {
      std::cout << "\n[" << phaseName
                << "] alerta: estancamiento genetico en gen " << gen
                << ". purgando 80% de la poblacion...\n";

      int survival_cutoff = static_cast<int>(popSize * 0.2f);
      for (int i = survival_cutoff; i < popSize; ++i) {
        population[i].generateRandomBranch(4, 0);
      }

      stagnation_counter = 0;
      previous_best_fitness = -99999.0f; 
    }

    std::vector<Tree> next_gen;
    next_gen.reserve(popSize);

    int elite_count = popSize * 0.1;
    for (int i = 0; i < elite_count; ++i) next_gen.push_back(population[i]);

    while (next_gen.size() < popSize) {
      int p1 = parent_dist(getGlobalRNG());
      int p2 = parent_dist(getGlobalRNG());
      Tree child = Evolver::crossover(population[p1], population[p2]);
      child.mutate(temp);
      next_gen.push_back(child);
    }

    population = std::move(next_gen);

    if (gen % (generations / 10) == 0 || gen == generations - 1) {
      std::cout << "[" << phaseName << "] gen " << gen
                << " | puntaje: " << bestTree.fitness_score << "\n";
    }
  }

  return bestTree;
}

int main(int argc, char* argv[]) {
  int popSize = 1000;
  int generations = 2000;  // 5000 seems more optimal
  float temp = 0.1f;
  float playback_temp = 0.15f;
  int threshold = 30;
  float lambda = 0.5f;
  // int theme_index = 0; //?? DEPRECATED
  std::string theme_name;
  std::string input_seed_str = "duel";

  std::string output_file;

  // uint32_t master_seed = 0;
  std::string master_seed = input_seed_str;
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
      input_seed_str = arg.substr(7);
      seed_provided = true;
    } else if (arg.find("--theme=") == 0) {
      theme_name = arg.substr(8);
    }
  }

  if (!seed_provided) {
    master_seed = std::random_device{}();
  } else {
    master_seed = input_seed_str;
  }

  if (!output_file_provided) {
    output_file = "./symphonies/G" + std::to_string(generations);
    output_file += "_T" + std::to_string(temp);
    output_file += "_P" + std::to_string(popSize);
    output_file += "_Th" + std::to_string(threshold);
    output_file += "_L" + std::to_string(lambda);
    output_file += "_S" + master_seed + ".mid";
  }

  std::string hex_seed = SeedRegistry::resolve(input_seed_str);
  std::vector<Note> seedV;

  try {
    seedV = SeedCrypto::decodeSeed(hex_seed);
  } catch (...) {
    std::cerr
        << "[error] fallo al decodificar la semilla\n";
    return 1;
  }

  uint32_t numeric_seed =
      static_cast<uint32_t>(std::hash<std::string>{}(master_seed));
  getGlobalRNG().seed(numeric_seed);

  Evolver::PARSIMONY_THRESHOLD = threshold;
  Evolver::PARSIMONY_LAMBDA = lambda;

  std::cout << "--pop=" << popSize << " --gens=" << generations
            << " --temp=" << temp << " --threshold=" << threshold
            << " --lambda=" << lambda << " --seed=" << master_seed
            << " --theme=" << theme_name << "\n";

  // std::vector<Note> seedV = {
  //     Note(60, 1.0f, 80), Note(64, 0.5f, 80), Note(67, 0.5f, 80),
  //     Note(72, 1.0f, 80), Note(67, 0.5f, 80), Note(64, 0.5f, 80),
  //     Note(60, 1.0f, 80), Note(62, 0.5f, 80), Note(64, 0.5f, 80),
  //     Note(65, 0.5f, 80), Note(67, 1.0f, 80), Note(69, 0.5f, 80),
  //     Note(71, 0.5f, 80), Note(72, 1.0f, 80), Note(67, 0.5f, 80),
  //     Note(64, 0.5f, 80), Note(60, 1.0f, 80), Note(60, 1.0f, 80)};

  auto start_time = std::chrono::high_resolution_clock::now();

  const SymphonyBlueprint* active_theme_ptr = &Maestro::REGISTRY[0];

  for (const auto& bp : Maestro::REGISTRY) {
    if (bp.theme_name == theme_name) {
      active_theme_ptr = &bp;
      break;
    }
  }

  const SymphonyBlueprint& active_theme = *active_theme_ptr;

  std::cout << "[maestro] tema: " << active_theme.theme_name << " a "
            << active_theme.target_bpm << " bpm\n";

  std::cout << "\n[entrenando motivo A - armonia]\n";
  Tree harmTreeA = runGA("HarmA", popSize, generations,
                         active_theme.verse_chip.optimal_temperature,
                         [&seedV, &active_theme](const Tree& t) {
                           return Evolver::calculateHarmonyFitness(
                               t, seedV, active_theme.verse_chip.left_matrix);
                         });
  std::vector<Note> harmA =
      harmTreeA.generateMelody(seedV, 16.0f, {}, playback_temp);

  std::cout << "\n[entrenando motivo A - base]\n";
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

  std::cout << "\n[entrenando motivo A - melodia]\n";
  Tree melTreeA = runGA(
      "MelA", popSize, generations, active_theme.verse_chip.optimal_temperature,
      [&seedV, &bassA, &active_theme](const Tree& t) {
        return Evolver::calculateMelodyFitness(
            t, seedV, bassA, active_theme.verse_chip.right_matrix);
      });
  std::vector<Note> melA =
      melTreeA.generateMelody(seedV, 16.0f, bassA, playback_temp);

  std::cout << "\n[entrenando puente B - chaos]\n";

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

  auto end_time = std::chrono::high_resolution_clock::now();
  std::cout << "\n[Entrenamiento Completado en "
            << std::chrono::duration<double>(end_time - start_time).count()
            << " s]\n";

  hex_seed = SeedCrypto::encodeSeed(seedV);

  auto epoch = std::chrono::system_clock::now().time_since_epoch();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(epoch).count();

  std::string runID = hex_seed.substr(0, 8) + "_" + std::to_string(popSize) +
                      "_" + std::to_string(generations) + "G_" +
                      std::to_string(static_cast<int>(temp * 100)) + "T_" +
                      std::to_string(threshold) + "Th_" +
                      std::to_string(timestamp);

  std::string base_dir = "trees/" + runID;
  std::string models_dir = base_dir + "/models";
  fs::create_directories(models_dir);
  fs::create_directories("json_symphonies");

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
           << " s\n";
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

  Maestro::generate(harmTreeA, bassTreeA, melTreeA, harmTreeB, bassTreeB,
                    melTreeB, seedV, hex_seed, active_theme, output_midi);

  return 0;
}