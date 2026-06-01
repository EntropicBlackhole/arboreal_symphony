#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>

#include "HumanMatrix.hpp"
#include "MiniMidi.hpp"

enum class Predicate {
  IsUnison,
  IsMinorSecond,
  IsMajorSecond,
  IsMinorThird,
  IsMajorThird,
  IsPerfectFourth,
  IsTritone,
  IsPerfectFifth,
  IntervalLargerThanFifth,
  PitchIsInCMajor,
  MelodicDirectionUp,
  MelodicDirectionDown,
  DurationShorter,
  DurationLonger,
  DurationEquals,
  BassMatchesHarmonyRoot,
  BassConsonantWithHarmony,
  BassClashesWithHarmony,
  VarianceIsHigh,
  IsLocalMaximum,
  DensityIsSparse,
  MatchesHistoricalMotif,
  IsArpeggiatingUp,
  IsArpeggiatingDown,
  IsSyncopated,
  IsDownbeat,
  MatchesHarmonyContour,
  IsOctaveLeap,
  NUM_PREDICATES
};

struct Node {
  bool is_leaf;
  Predicate predicate;
  float prob_pitch;
  float prob_duration;
  int left_index;
  int right_index;

  static Node makeLeaf(float p_pitch, float p_dur) {
    Node n;
    n.is_leaf = true;
    n.prob_pitch = p_pitch;
    n.prob_duration = p_dur;
    n.left_index = -1;
    n.right_index = -1;
    return n;
  }
  static Node makeMain(Predicate pred, int left, int right) {
    Node n;
    n.is_leaf = false;
    n.predicate = pred;
    n.prob_pitch = 0;
    n.prob_duration = 0;
    n.left_index = left;
    n.right_index = right;
    return n;
  }
};

class Tree {
 public:
  std::vector<Node> nodes;
  int root_index = 0;
  float fitness_score = 0.0f;

  Tree() {}

  void generateRandomBranch(int max_depth = 4, int current_depth = 0) {
    nodes.clear();
    root_index = buildSubtree(max_depth, current_depth);
  }

  void mutate(float temp) {
    if (nodes.empty()) return;
    std::uniform_int_distribution<int> dist(0, nodes.size() - 1);
    int idx = dist(rng);

    if (nodes[idx].is_leaf) {
      std::uniform_real_distribution<float> fdist(-temp, temp);
      nodes[idx].prob_pitch =
          std::clamp(nodes[idx].prob_pitch + fdist(rng), 0.0f, 1.0f);
      nodes[idx].prob_duration =
          std::clamp(nodes[idx].prob_duration + fdist(rng), 0.0f, 1.0f);
    } else {
      std::uniform_real_distribution<float> chance(0.0f, 1.0f);
      if (chance(rng) < temp) {
        std::uniform_int_distribution<int> pdist(
            0, static_cast<int>(Predicate::NUM_PREDICATES) - 1);
        nodes[idx].predicate = static_cast<Predicate>(pdist(rng));
      }
    }
  }

  std::pair<float, float> evaluate(const std::vector<Note>& V,
                                   const std::vector<Note>& context) const {
    if (nodes.empty()) return {0.5f, 0.5f};
    int current_idx = root_index;
    int steps = 0;

    while (current_idx >= 0 && current_idx < nodes.size() && steps < 50) {
      const Node& current_node = nodes[current_idx];
      if (current_node.is_leaf)
        return {current_node.prob_pitch, current_node.prob_duration};
      bool result = evaluateLogic(current_node.predicate, V, context);
      current_idx = result ? current_node.left_index : current_node.right_index;
      steps++;
    }
    return {0.5f, 0.5f};
  }

  std::vector<Note> generateMelody(const std::vector<Note>& seed,
                                   float target_duration_beats,
                                   const std::vector<Note>& ref_track = {},
                                   float temp = 0.1f) const {
    std::vector<Note> W;
    std::vector<Note> current_V = seed;
    int prev_pitch = current_V.back().pitch;

    float current_time = 0.0f;

    while (current_time < target_duration_beats) {
      std::vector<Note> current_context;

      if (!ref_track.empty()) {
        float ref_time = 0.0f;
        Note active_harmony = ref_track[0];
        for (const auto& n : ref_track) {
          active_harmony = n;
          ref_time += n.duration;
          if (ref_time > current_time) break;
        }
        current_context.push_back(active_harmony);
      }

      auto [prob_pitch, prob_duration] = evaluate(current_V, current_context);
      Note new_note =
          mapProbabilityToNote(prob_pitch, prob_duration, prev_pitch, temp);

      if (current_time + new_note.duration > target_duration_beats) {
        new_note.duration = target_duration_beats - current_time;
      }

      W.push_back(new_note);
      current_V.push_back(new_note);
      prev_pitch = new_note.pitch;
      current_time += new_note.duration;
    }
    return W;
  }

 private:
  mutable std::mt19937 rng{std::random_device{}()};

  int buildSubtree(int max_depth, int current_depth) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (current_depth >= max_depth || (current_depth > 0 && dist(rng) < 0.3f)) {
      nodes.push_back(Node::makeLeaf(dist(rng), dist(rng)));
      return nodes.size() - 1;
    }

    int node_idx = nodes.size();
    nodes.push_back(Node::makeLeaf(0.0f, 0.0f));

    int left = buildSubtree(max_depth, current_depth + 1);
    int right = buildSubtree(max_depth, current_depth + 1);

    std::uniform_int_distribution<int> p_dist(
        0, static_cast<int>(Predicate::NUM_PREDICATES) - 1);
    Predicate rand_pred = static_cast<Predicate>(p_dist(rng));

    nodes[node_idx] = Node::makeMain(rand_pred, left, right);
    return node_idx;
  }

  bool evaluateLogic(Predicate p, const std::vector<Note>& V,
                     const std::vector<Note>& context) const {
    int len = V.size();
    if (len < 2) return false;
    int p1 = V[len - 1].pitch;
    int p2 = V[len - 2].pitch;

    switch (p) {
      case Predicate::IsUnison:
        return std::abs(p1 - p2) == 0;
      case Predicate::IsMajorThird:
        return std::abs(p1 - p2) == 4;
      case Predicate::IsPerfectFifth:
        return std::abs(p1 - p2) == 7;
      case Predicate::PitchIsInCMajor: {
        int mod = p1 % 12;
        return mod == 0 || mod == 2 || mod == 4 || mod == 5 || mod == 7 ||
               mod == 9 || mod == 11;
      }
      case Predicate::MelodicDirectionUp:
        return p1 > p2;
      case Predicate::DurationShorter:
        return V[len - 1].duration < V[len - 2].duration;
      case Predicate::BassMatchesHarmonyRoot: {
        if (context.empty()) return false;
        return (p1 % 12) == (context[0].pitch % 12);
      }
      case Predicate::BassConsonantWithHarmony: {
        if (context.empty()) return false;
        int interval = std::abs(p1 - context[0].pitch) % 12;
        return interval == 0 || interval == 3 || interval == 4 || interval == 7;
      }
      case Predicate::BassClashesWithHarmony: {
        if (context.empty()) return false;
        int interval = std::abs(p1 - context[0].pitch) % 12;
        return interval == 1 || interval == 2 || interval == 6 ||
               interval == 10 || interval == 11;
      }
      case Predicate::VarianceIsHigh: {
        if (len < 16) return false;
        float sum = 0;
        for (int i = len - 16; i < len; i++) sum += V[i].pitch;
        float mean = sum / 16.0f, var = 0;
        for (int i = len - 16; i < len; i++)
          var += (V[i].pitch - mean) * (V[i].pitch - mean);
        return (var / 16.0f) > 12.0f;
      }
      case Predicate::IsLocalMaximum: {
        if (len < 16) return false;
        int max_p = p1;
        for (int i = len - 16; i < len - 1; i++)
          if (V[i].pitch > max_p) return false;
        return true;
      }
      case Predicate::DensityIsSparse: {
        if (len < 8) return false;
        float total_dur = 0;
        for (int i = len - 8; i < len; i++) total_dur += V[i].duration;
        return total_dur > 6.0f;
      }
      case Predicate::MatchesHistoricalMotif: {
        if (len < 18) return false;
        return (V[len - 1].pitch - V[len - 2].pitch) ==
               (V[len - 17].pitch - V[len - 18].pitch);
      }
      case Predicate::IsArpeggiatingUp: {
        if (len < 3) return false;
        int d1 = V[len - 2].pitch - V[len - 3].pitch;
        int d2 = V[len - 1].pitch - V[len - 2].pitch;
        return (d1 == 3 || d1 == 4) && (d2 == 3 || d2 == 4);
      }
      case Predicate::IsArpeggiatingDown: {
        if (len < 3) return false;
        int d1 = V[len - 2].pitch - V[len - 3].pitch;
        int d2 = V[len - 1].pitch - V[len - 2].pitch;
        return (d1 == -3 || d1 == -4) && (d2 == -3 || d2 == -4);
      }
      case Predicate::IsSyncopated: {
        float total_time = 0;
        for (const auto& n : V) total_time += n.duration;
        return std::fmod(total_time, 1.0f) != 0.0f;
      }
      case Predicate::IsDownbeat: {
        float total_time = 0;
        for (const auto& n : V) total_time += n.duration;
        return std::fmod(total_time, 1.0f) == 0.0f;
      }
      case Predicate::MatchesHarmonyContour: {
        if (len < 2 || context.empty()) return false;
        int mel_delta = p1 - p2;
        int har_delta =
            context[0].pitch - p1;  // aproximacion para musica clasica
        return (mel_delta > 0 && har_delta > 0) ||
               (mel_delta < 0 && har_delta < 0);
      }
      case Predicate::IsOctaveLeap: {
        return std::abs(p1 - p2) == 12;
      }
      default:
        return false;
    }
  }

  Note mapProbabilityToNote(float prob_pitch, float prob_dur,
                            int previous_pitch, float temp) const {
    const std::vector<int> intervals = {-12, -9, -7, -5, -4, -3, -2, -1, 0,
                                        1,   2,  3,  4,  5,  7,  9,  12};

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    float pitchProbWithTemp = prob_pitch + (chance(rng) * (temp * 2) - temp);
    int index = static_cast<int>(pitchProbWithTemp * intervals.size());
    index = std::clamp(index, 0, static_cast<int>(intervals.size()) - 1);

    int new_pitch = previous_pitch + intervals[index];
    if (new_pitch > 84) new_pitch -= 12;
    if (new_pitch < 48) new_pitch += 12;

    float durProbWithTemp = prob_dur + (chance(rng) * (temp * 2) - temp);
    float duration;
    if (durProbWithTemp < 0.25f)
      duration = 0.25f;
    else if (durProbWithTemp < 0.5f)
      duration = 0.5f;
    else if (durProbWithTemp < 0.85f)
      duration = 1.0f;
    else
      duration = 2.0f;

    return Note(new_pitch, duration, 80);
  }
};

class Evolver {
 public:
  static inline float PARSIMONY_LAMBDA = 0.5f;
  static inline int PARSIMONY_THRESHOLD = 30;

  static Note getNoteAtTime(const std::vector<Note>& track,
                            float time_in_beats) {
    if (track.empty()) return Note(60, 1.0f, 80);  // porsiaca
    float current_t = 0.0f;
    for (const auto& n : track) {
      current_t += n.duration;
      if (current_t > time_in_beats) return n;
    }
    return track.back();
  }

  static int copySubtree(const Tree& source, int source_idx, Tree& dest) {
    if (source_idx < 0 || source_idx >= source.nodes.size()) return -1;
    const Node& s_node = source.nodes[source_idx];
    int dest_idx = dest.nodes.size();
    dest.nodes.push_back(s_node);
    if (!s_node.is_leaf) {
      int left = copySubtree(source, s_node.left_index, dest);
      int right = copySubtree(source, s_node.right_index, dest);
      dest.nodes[dest_idx].left_index = left;
      dest.nodes[dest_idx].right_index = right;
    }
    return dest_idx;
  }

  static Tree crossover(const Tree& parentA, const Tree& parentB) {
    Tree child = parentA;
    if (child.nodes.empty() || parentB.nodes.empty()) return child;

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> distA(0, child.nodes.size() - 1);
    std::uniform_int_distribution<int> distB(0, parentB.nodes.size() - 1);

    int replace_idx = distA(rng);
    int b_source_idx = distB(rng);
    int new_subtree_root = copySubtree(parentB, b_source_idx, child);

    if (child.root_index == replace_idx)
      child.root_index = new_subtree_root;
    else {
      for (auto& n : child.nodes) {
        if (!n.is_leaf) {
          if (n.left_index == replace_idx) n.left_index = new_subtree_root;
          if (n.right_index == replace_idx) n.right_index = new_subtree_root;
        }
      }
    }
    if (child.nodes.size() > 250) return parentA;
    return child;
  }

  static float calculateMatrixFitness(const std::vector<Note>& W) {
    float score = 0.0f;

    for (size_t i = 2; i < W.size(); i++) {
      int prev_jump = W[i - 1].pitch - W[i - 2].pitch;
      int curr_jump = W[i].pitch - W[i - 1].pitch;

      int prev_idx = std::clamp(prev_jump, -12, 12) + 12;
      int curr_idx = std::clamp(curr_jump, -12, 12) + 12;

      float human_prob = HUMAN_MATRIX[prev_idx][curr_idx];

      if (human_prob == 0.0f) {
        // si no existe en el dataset, castigar
        score -= 10.0f;
      } else {
        // escalar la probabilidad directamente a puntos de aptitud
        score += (human_prob * 100.0f);
      }
    }

    return score;
  }

  static float calculateHarmonyFitness(const Tree& tree,
                                       const std::vector<Note>& seed) {
    auto W = tree.generateMelody(seed, 64.0f, {}, 0.0f);

    float score = calculateMatrixFitness(W);

    std::unordered_set<int> unique_pitches;
    for (const auto& n : W) unique_pitches.insert(n.pitch);
    score += unique_pitches.size() * 5.0f;

    for (const auto& n : W) {
      int pClass = n.pitch % 12;
      if (pClass == 0 || pClass == 4 || pClass == 7) score += 5.0f;
    }

    int rhythmic_changes = 0;
    int consecutive_16ths = 0, max_consecutive_16ths = 0;

    for (size_t i = 1; i < W.size(); i++) {
      if (W[i].duration != W[i - 1].duration) {
        rhythmic_changes++;
        consecutive_16ths = (W[i].duration == 0.25f) ? 1 : 0;
      } else {
        if (W[i].duration == 0.25f) {
          consecutive_16ths++;
          if (consecutive_16ths > max_consecutive_16ths)
            max_consecutive_16ths = consecutive_16ths;
        }
      }
    }

    if (rhythmic_changes < 6) score -= 100.0f;
    if (max_consecutive_16ths > 8) score -= 200.0f;

    int node_count = tree.nodes.size();
    if (node_count > PARSIMONY_THRESHOLD)
      score -= ((node_count - PARSIMONY_THRESHOLD) * PARSIMONY_LAMBDA);
    return score;
  }

  static float calculateBassFitness(const Tree& tree,
                                    const std::vector<Note>& seed,
                                    const std::vector<Note>& harmony) {
    float score = 0.0f;
    auto W = tree.generateMelody(seed, 64.0f, harmony, 0.0f);

    int consecutive_16ths = 0;
    int max_consecutive_16ths = 0;
    float current_time = 0.0f;

    for (size_t i = 0; i < W.size(); i++) {
      Note current_harmony = getNoteAtTime(harmony, current_time);

      if (W[i].pitch % 12 == current_harmony.pitch % 12) {
        score += 20.0f;
      } else {
        int interval = std::abs(W[i].pitch - current_harmony.pitch) % 12;
        if (interval == 3 || interval == 4 || interval == 7)
          score += 5.0f;
        else
          score -= 15.0f;
      }
      if (W[i].pitch < 48) score += 10.0f;

      if (W[i].duration == 0.25f) {
        consecutive_16ths++;
        if (consecutive_16ths > max_consecutive_16ths)
          max_consecutive_16ths = consecutive_16ths;
      } else {
        consecutive_16ths = 0;
      }

      current_time += W[i].duration;
    }

    if (max_consecutive_16ths > 4) score -= 150.0f;

    int node_count = tree.nodes.size();
    if (node_count > PARSIMONY_THRESHOLD) {
      score -= ((node_count - PARSIMONY_THRESHOLD) * PARSIMONY_LAMBDA);
    }
    return score;
  }

  static float calculateMelodyFitness(const Tree& tree,
                                      const std::vector<Note>& seed,
                                      const std::vector<Note>& harmony) {
    float score = 0.0f;
    auto W = tree.generateMelody(seed, 64.0f, harmony, 0.0f);

    int inflection_points = 0;
    int rhythmic_changes = 0;
    int consecutive_16ths = 0;
    int max_consecutive_16ths = 0;

    float current_time = 0.0f;
    Note prev_harmony = getNoteAtTime(harmony, 0.0f);

    for (size_t i = 0; i < W.size(); i++) {
      Note current_harmony = getNoteAtTime(harmony, current_time);

      int interval = std::abs(W[i].pitch - current_harmony.pitch) % 12;
      if (interval == 0 || interval == 3 || interval == 4 || interval == 7 ||
          interval == 8 || interval == 9)
        score += 15.0f;
      else if (interval == 1 || interval == 2 || interval == 10 ||
               interval == 11)
        score -= 15.0f;

      if (i > 0) {
        if (W[i].duration != W[i - 1].duration) {
          rhythmic_changes++;
          consecutive_16ths = (W[i].duration == 0.25f) ? 1 : 0;
        } else {
          if (W[i].duration == 0.25f) {
            consecutive_16ths++;
            if (consecutive_16ths > max_consecutive_16ths)
              max_consecutive_16ths = consecutive_16ths;
          }
        }
      }

      if (i > 1) {
        int delta1 = W[i - 1].pitch - W[i - 2].pitch;
        int delta2 = W[i].pitch - W[i - 1].pitch;
        if ((delta1 > 0 && delta2 < 0) || (delta1 < 0 && delta2 > 0))
          inflection_points++;

        int mel_delta = W[i].pitch - W[i - 1].pitch;
        int har_delta = current_harmony.pitch - prev_harmony.pitch;
        if ((mel_delta > 0 && har_delta < 0) ||
            (mel_delta < 0 && har_delta > 0))
          score += 5.0f;
      }

      prev_harmony = current_harmony;
      current_time += W[i].duration;
    }

    if (inflection_points < 10) score -= 200.0f;
    if (rhythmic_changes < 8) score -= 150.0f;
    if (max_consecutive_16ths > 6) score -= 220.0f;

    int node_count = tree.nodes.size();
    if (node_count > PARSIMONY_THRESHOLD) {
      score -= ((node_count - PARSIMONY_THRESHOLD) * PARSIMONY_LAMBDA);
    }
    return score;
  }
};