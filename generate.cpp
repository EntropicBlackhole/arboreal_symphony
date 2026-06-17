#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Maestro.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Uso: ./generate <RunID> [hex_seed_opcional]\n";
    return 1;
  }

  std::string runID = argv[1];
  std::string base_dir = "trees/" + runID;
  std::string models_dir = base_dir + "/models";

  if (!fs::exists(models_dir)) {
    std::cerr << "[error] no se encontro el directorio: " << models_dir << "\n";
    return 1;
  }

  std::string theme_name = "The_Hollow_Knight_Duel";
  std::string hex_seed = "0003c5040004050200043502";

  std::ifstream report(base_dir + "/report.txt");
  if (report.is_open()) {
    std::string line;
    while (std::getline(report, line)) {
      if (line.find("theme:") != std::string::npos) {
        theme_name = line.substr(line.find(":") + 1);
        theme_name.erase(0, theme_name.find_first_not_of(" \t"));
      }
      if (line.find("hex_seed:") != std::string::npos) {
        hex_seed = line.substr(line.find(":") + 1);
        hex_seed.erase(0, hex_seed.find_first_not_of(" \t"));
      }
    }
    report.close();
  }

  std::string input_seed_str = hex_seed;
  if (argc > 2) {
    input_seed_str = argv[2];
    std::cout << "[generador] resolviendo clave semantica: " << input_seed_str
              << "\n";
  }

  std::string resolved_hex = SeedRegistry::resolve(input_seed_str);
  std::vector<Note> custom_seed = SeedCrypto::decodeSeed(resolved_hex);

  const SymphonyBlueprint* active_theme_ptr = &Maestro::REGISTRY[0];
  for (const auto& bp : Maestro::REGISTRY) {
    if (bp.theme_name == theme_name) {
      active_theme_ptr = &bp;
      break;
    }
  }

  Tree harmA, bassA, melA, harmB, bassB, melB;
  try {
    harmA.loadFromFile(models_dir + "/harmA.tree");
    bassA.loadFromFile(models_dir + "/bassA.tree");
    melA.loadFromFile(models_dir + "/melA.tree");

    harmB.loadFromFile(models_dir + "/harmB.tree");
    bassB.loadFromFile(models_dir + "/bassB.tree");
    melB.loadFromFile(models_dir + "/melB.tree");
  } catch (...) {
    std::cerr << "[error] corrupcion al leer los archivos .tree\n";
    return 1;
  }

  std::string output_midi = resolved_hex + "_generated.mid";

  Maestro::generate(harmA, bassA, melA, harmB, bassB, melB, custom_seed,
                    resolved_hex, *active_theme_ptr, output_midi);

  return 0;
}