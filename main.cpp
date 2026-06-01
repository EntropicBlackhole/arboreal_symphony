#include <chrono>
#include <iostream>
#include <string>
#include "Tree.hpp"

template <typename FitnessFunc>
Tree runGA(const std::string& phaseName, int popSize, int generations,
           float temp, FitnessFunc fitnessFunc) {
  std::vector<Tree> population(popSize);
  for (auto& t : population) t.generateRandomBranch(4, 0);

  Tree bestTree;
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> parent_dist(0, 9);

  for (int gen = 0; gen < generations; ++gen) {
    for (auto& t : population) t.fitness_score = fitnessFunc(t);

    std::sort(population.begin(), population.end(),
              [](const Tree& a, const Tree& b) {
                return a.fitness_score > b.fitness_score;
              });

    bestTree = population[0];

    if (gen % (std::max(1, generations / 10)) == 0) {
      std::cout << phaseName << " | gen " << gen
                << " | puntaje: " << bestTree.fitness_score
                << " | t: " << bestTree.nodes.size() << "\n";
    }

    std::vector<Tree> nextGen;
    nextGen.reserve(popSize);
    nextGen.push_back(population[0]);
    nextGen.push_back(population[1]);

    while (nextGen.size() < popSize) {
      const Tree& mom = population[parent_dist(rng)];
      const Tree& dad = population[parent_dist(rng)];
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
  std::string output_file = "symphony_cpp.mid";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--generations=") == 0)
      generations = std::stoi(arg.substr(14));
    else if (arg.find("--temp=") == 0)
      temp = std::stof(arg.substr(7));
    else if (arg.find("--pop=") == 0)
      popSize = std::stoi(arg.substr(6));
    else if (arg.find("--threshold=") == 0)
      threshold = std::stoi(arg.substr(12));
    else if (arg.find("--lambda=") == 0)
      lambda = std::stof(arg.substr(9));
    else if (arg.find("--out=") == 0)
      output_file = arg.substr(6);
  }

  Evolver::PARSIMONY_THRESHOLD = threshold;
  Evolver::PARSIMONY_LAMBDA = lambda;

  std::cout << "pop=" << popSize << ", gens=" << generations
            << ", temp=" << temp << ", threshold=" << threshold
            << ", lambda=" << lambda << "\n";

  std::vector<Note> seedV = {
      Note(60, 1.0f, 80), Note(64, 0.5f, 80), Note(67, 0.5f, 80),
      Note(72, 1.0f, 80), Note(67, 0.5f, 80), Note(64, 0.5f, 80),
      Note(60, 1.0f, 80), Note(62, 0.5f, 80), Note(64, 0.5f, 80),
      Note(65, 0.5f, 80), Note(67, 1.0f, 80), Note(69, 0.5f, 80),
      Note(71, 0.5f, 80), Note(72, 1.0f, 80), Note(67, 0.5f, 80),
      Note(64, 0.5f, 80), Note(60, 1.0f, 80), Note(60, 1.0f, 80)};

  auto start_time = std::chrono::high_resolution_clock::now();

  std::cout << "\narmonia\n";
  Tree harmonyTree =
      runGA("armonia", popSize, generations, temp, [&seedV](const Tree& t) {
        return Evolver::calculateHarmonyFitness(t, seedV);
      });
  std::vector<Note> harmonyW = harmonyTree.generateMelody(
      seedV, 64.0f, {}, playback_temp);

  std::cout << "\nbase\n";
  Tree bassTree = runGA(
      "base", popSize, generations, temp, [&seedV, &harmonyW](const Tree& t) {
        return Evolver::calculateBassFitness(t, seedV, harmonyW);
      });
  std::vector<Note> bassW =
      bassTree.generateMelody(seedV, 64.0f, harmonyW, playback_temp);

  for (auto& n : bassW) n.pitch -= 12;

  std::cout << "\nmelodia\n";
  Tree melodyTree =
      runGA("melodia", popSize, generations, temp,
            [&seedV, &harmonyW](const Tree& t) {
              return Evolver::calculateMelodyFitness(t, seedV, harmonyW);
            });
  std::vector<Note> melodyW =
      melodyTree.generateMelody(seedV, 64.0f, harmonyW, playback_temp);

  for (auto& n : melodyW) n.pitch += 12;

  auto end_time = std::chrono::high_resolution_clock::now();
  std::cout << "\n completado en "
            << std::chrono::duration<double>(end_time - start_time).count()
            << " s\n";

  MiniMidi midiFile;
  midiFile.addTrack(harmonyW, 0, 1);  // canal 1, piano
  midiFile.addTrack(bassW, 1, 36);    // canal 2, base
  midiFile.addTrack(melodyW, 2, 74);  // canal 3, flauta

  midiFile.save(output_file);
  std::cout << "guardado en " << output_file << std::endl;

  return 0;
}