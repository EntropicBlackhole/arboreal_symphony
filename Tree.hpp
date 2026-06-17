#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <unordered_map>

#include "MiniMidi.hpp"

constexpr int PPQ = 480;

struct TemporalContext {
  Note note;
  int start_tick;
  int end_tick;
};

// --- DIATONIC SET THEORY (SCALE QUANTIZATION) ---
// Standard Minor Scale (Aeolian): Root, +2, +3, +5, +7, +8, +10
const std::vector<int> MINOR_SCALE = {0, 2, 3, 5, 7, 8, 10};

// Snaps any 12-tone pitch to the nearest valid 7-tone scale degree
inline int snapToScale(int pitch, int root_note,
                       const std::vector<int>& scale_intervals) {
  int octave_base = (pitch / 12) * 12;
  int relative_pitch = pitch % 12;
  int root_offset = root_note % 12;

  // Calculate distance from the root note
  int interval_from_root = (relative_pitch - root_offset + 12) % 12;

  int closest_dist = 100;
  int best_interval = 0;

  // Find the nearest mathematically legal note in the 7-note subset
  for (int valid_interval : scale_intervals) {
    int dist = std::abs(interval_from_root - valid_interval);
    // Handle wrap-around (e.g., comparing 11 to 0)
    dist = std::min(dist, 12 - dist);
    if (dist < closest_dist) {
      closest_dist = dist;
      best_interval = valid_interval;
    }
  }

  int snapped_pitch = octave_base + root_offset + best_interval;

  // Fix octave boundary shifts
  if (std::abs(snapped_pitch - pitch) > 6) {
    snapped_pitch += (snapped_pitch < pitch) ? 12 : -12;
  }
  return snapped_pitch;
}

inline std::vector<Note> getNotesInWindow(const std::vector<Note>& track,
                                          int start_tick, int end_tick) {
  std::vector<Note> window;
  int current_t = 0;
  for (const auto& n : track) {
    int note_ticks = static_cast<int>(std::round(n.duration * PPQ));
    if (current_t >= start_tick && current_t < end_tick) {
      window.push_back(n);
    }
    current_t += note_ticks;
    if (current_t >= end_tick) break;
  }
  return window;
}

inline std::vector<int> extractContour(const std::vector<Note>& notes) {
  std::vector<int> contour;
  if (notes.size() < 2) return contour;
  for (size_t i = 1; i < notes.size(); ++i) {
    int delta = notes[i].pitch - notes[i - 1].pitch;
    if (delta > 0)
      contour.push_back(1);
    else if (delta < 0)
      contour.push_back(-1);
    else
      contour.push_back(0);
  }
  return contour;
}

// based on levenshtein ddit distance
inline int calculateEditDistance(const std::vector<int>& v1,
                                 const std::vector<int>& v2) {
  int m = v1.size();
  int n = v2.size();
  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
  for (int i = 0; i <= m; i++) {
    for (int j = 0; j <= n; j++) {
      if (i == 0)
        dp[i][j] = j;
      else if (j == 0)
        dp[i][j] = i;
      else if (v1[i - 1] == v2[j - 1])
        dp[i][j] = dp[i - 1][j - 1];
      else
        dp[i][j] = 1 + std::min({dp[i][j - 1], dp[i - 1][j], dp[i - 1][j - 1]});
    }
  }
  return dp[m][n];
}

inline TemporalContext getTemporalContext(const std::vector<Note>& track,
                                          int time_in_ticks) {
  if (track.empty()) return {Note(60, 1.0f, 80), 0, PPQ * 100};
  for (const auto& n : track) {
    int note_ticks = static_cast<int>(std::round(n.duration * PPQ));
    if (time_in_ticks >= n.absolute_tick &&
        time_in_ticks < (n.absolute_tick + note_ticks)) {
      return {n, n.absolute_tick, n.absolute_tick + note_ticks};
    }
  }
  int last_ticks = static_cast<int>(std::round(track.back().duration * PPQ));
  return {track.back(), track.back().absolute_tick,
          track.back().absolute_tick + last_ticks};
}

inline std::mt19937& getGlobalRNG() {
  static std::mt19937 rng(1337);
  return rng;
}

inline Note getNoteAtTime(const std::vector<Note>& track, int time_in_ticks) {
  if (track.empty()) return Note(60, 1.0f, 80);
  for (const auto& n : track) {
    int note_ticks = static_cast<int>(std::round(n.duration * PPQ));
    if (time_in_ticks >= n.absolute_tick &&
        time_in_ticks < (n.absolute_tick + note_ticks)) {
      return n;
    }
  }
  return track.back();
}

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
  ContextStartsSimultaneously,
  ContextIsEndingSoon,
  ContextIsSustaining,
  ContextStartedBeforeUs,
  IsHeavyDownbeat,
  IsOffbeatSyncopation,
  NUM_PREDICATES
};

struct alignas(32) Node {
  bool is_leaf;
  Predicate predicate;
  float prob_pitch;
  float prob_duration;
  float prob_chord;
  int left_index;
  int right_index;

  static Node makeLeaf(float p_pitch, float p_dur, float p_chord) {
    Node n;
    n.is_leaf = true;
    n.prob_pitch = p_pitch;
    n.prob_duration = p_dur;
    n.prob_chord = p_chord;
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

  void compact() {
    if (nodes.empty()) return;

    std::vector<Node> active_nodes;
    // index_map[old_index] = new_index. // initialize with -1
    std::vector<int> index_map(nodes.size(), -1);

    auto copy_reachable = [&](auto& self, int old_idx) -> int {
      if (old_idx < 0 || old_idx >= nodes.size()) return -1;

      int new_idx = active_nodes.size();
      active_nodes.push_back(nodes[old_idx]);
      index_map[old_idx] = new_idx;

      if (!active_nodes[new_idx].is_leaf) {
        int old_left = active_nodes[new_idx].left_index;
        int old_right = active_nodes[new_idx].right_index;

        active_nodes[new_idx].left_index = self(self, old_left);
        active_nodes[new_idx].right_index = self(self, old_right);
      }

      return new_idx;
    };

    copy_reachable(copy_reachable, 0);

    nodes = std::move(active_nodes);
  }

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
      nodes[idx].prob_chord =
          std::clamp(nodes[idx].prob_chord + fdist(rng), 0.0f, 1.0f);
    } else {
      std::uniform_real_distribution<float> chance(0.0f, 1.0f);
      if (chance(rng) < temp) {
        std::uniform_int_distribution<int> pdist(
            0, static_cast<int>(Predicate::NUM_PREDICATES) - 1);
        nodes[idx].predicate = static_cast<Predicate>(pdist(rng));
      }
    }
    this->compact();
  }

  std::tuple<float, float, float> evaluate(
      const std::vector<Note>& V, const std::vector<Note>& context,
      int current_tick, const TemporalContext& ref_ctx) const {
    if (nodes.empty()) return {0.5f, 0.5f, 0.5f};
    int current_idx = root_index;
    int steps = 0;

    while (current_idx >= 0 && current_idx < nodes.size() && steps < 50) {
      const Node& current_node = nodes[current_idx];
      if (current_node.is_leaf)
        return {current_node.prob_pitch, current_node.prob_duration,
                current_node.prob_chord};
      bool result = evaluateLogic(current_node.predicate, V, context,
                                  current_tick, ref_ctx);
      current_idx = result ? current_node.left_index : current_node.right_index;
      steps++;
    }
    return {0.5f, 0.5f, 0.5f};
  }

  std::vector<Note> generateMelody(const std::vector<Note>& seed,
                                   float target_duration_beats,
                                   const std::vector<Note>& ref_track = {},
                                   float temp = 0.1f,
                                   bool is_pad = false) const {
    std::vector<Note> W;
    std::vector<Note> current_V = seed;
    int prev_pitch = current_V.back().pitch;

    int target_duration_ticks =
        static_cast<int>(std::round(target_duration_beats * PPQ));
    int current_tick = 0;

    while (current_tick < target_duration_ticks) {
      std::vector<Note> current_context;
      TemporalContext ref_ctx = {Note(60, 1.0f, 80), 0, PPQ * 100};

      if (!ref_track.empty()) {
        current_context.push_back(getNoteAtTime(ref_track, current_tick));
      }

      float progress = static_cast<float>(current_tick) / target_duration_ticks;
      float current_temp = temp;

      if (progress >= 0.5f && progress < 0.75f)
        current_temp = std::min(1.0f, temp * 4.0f);
      else if (progress >= 0.75f)
        current_temp = temp * 1.2f;
      else if (progress >= 0.25f && progress < 0.5f)
        current_temp = temp;

      auto [prob_pitch, prob_duration, prob_chord] =
          evaluate(current_V, current_context, current_tick, ref_ctx);
      Note new_note = mapProbabilityToNote(prob_pitch, prob_duration,
                                           prev_pitch, current_temp);
      // !! i think prob_chord should also be added to mapProbabilityToNote
      new_note.absolute_tick = current_tick;

      int note_ticks = static_cast<int>(std::round(new_note.duration * PPQ));
      if (current_tick + note_ticks > target_duration_ticks) {
        note_ticks = target_duration_ticks - current_tick;
        new_note.duration = static_cast<float>(note_ticks) / PPQ;
      }

      new_note.pitch = snapToScale(new_note.pitch, 60, MINOR_SCALE);

      if (is_pad) {
        new_note.duration = std::max(new_note.duration, 2.0f);
        new_note.intensity = static_cast<int>(new_note.intensity * 0.70f);
      }

      W.push_back(new_note);

      static int chord_cooldown = 0;

      if (chord_cooldown > 0) {
        prob_chord = 0.0f;
        chord_cooldown--;
      }

      if (prob_chord > 0.6f) {
        Note fifth = new_note;
        fifth.pitch += 7;
        fifth.intensity = static_cast<int>(fifth.intensity * 0.85f);
        W.push_back(fifth);
        chord_cooldown = 1;
      }

      if (prob_chord > 0.85f) {
        Note octave = new_note;
        octave.pitch += 12;
        octave.intensity = static_cast<int>(octave.intensity * 0.70f);
        W.push_back(octave);
        chord_cooldown = 3;
      }

      current_V.push_back(new_note);
      prev_pitch = new_note.pitch;

      // advance the clock
      current_tick += note_ticks;
    }
    return W;
  }

  void saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    out << root_index << " " << nodes.size() << "\n";

    for (const auto& n : nodes) {
      out << n.is_leaf << " " << static_cast<int>(n.predicate) << " "
          << n.prob_pitch << " " << n.prob_duration << " " << n.prob_chord
          << " " << n.left_index << " " << n.right_index << "\n";
    }
    out.close();
  }

  void loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return;

    nodes.clear();
    size_t node_count;
    in >> root_index >> node_count;

    nodes.reserve(node_count);
    for (size_t i = 0; i < node_count; ++i) {
      Node n;
      int pred_int;
      in >> n.is_leaf >> pred_int >> n.prob_pitch >> n.prob_duration >>
          n.prob_chord >> n.left_index >> n.right_index;

      n.predicate = static_cast<Predicate>(pred_int);
      nodes.push_back(n);
    }
    in.close();
  }

 private:
  mutable std::mt19937 rng{getGlobalRNG()};

  int buildSubtree(int max_depth, int current_depth) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (current_depth >= max_depth || (current_depth > 0 && dist(rng) < 0.3f)) {
      nodes.push_back(Node::makeLeaf(dist(rng), dist(rng), dist(rng)));
      return nodes.size() - 1;
    }

    int node_idx = nodes.size();
    nodes.push_back(Node::makeLeaf(0.0f, 0.0f, 0.0f));

    int left = buildSubtree(max_depth, current_depth + 1);
    int right = buildSubtree(max_depth, current_depth + 1);

    std::uniform_int_distribution<int> p_dist(
        0, static_cast<int>(Predicate::NUM_PREDICATES) - 1);
    Predicate rand_pred = static_cast<Predicate>(p_dist(rng));

    nodes[node_idx] = Node::makeMain(rand_pred, left, right);
    return node_idx;
  }

  bool evaluateLogic(Predicate p, const std::vector<Note>& V,
                     const std::vector<Note>& context, int current_tick,
                     const TemporalContext& ref_ctx) const {
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
        if (V.size() < 3) return false;
        int current_delta = p1 - p2;
        int motif_delta = V[1].pitch - V[0].pitch;

        // trajectory
        return (current_delta > 0 && motif_delta > 0) ||
               (current_delta < 0 && motif_delta < 0) ||
               (current_delta == 0 && motif_delta == 0);
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
        //!! might have to change this, we're training on other datasets now
        return (mel_delta > 0 && har_delta > 0) ||
               (mel_delta < 0 && har_delta < 0);
      }
      case Predicate::IsOctaveLeap: {
        return std::abs(p1 - p2) == 12;
      }
      case Predicate::ContextStartsSimultaneously: {
        return current_tick == ref_ctx.start_tick;
      }
      case Predicate::ContextIsEndingSoon: {
        return (ref_ctx.end_tick - current_tick) <=
               (PPQ / 2);  // switching from PPQ / 4 to PPQ / 2
      }
      case Predicate::ContextIsSustaining: {
        return (ref_ctx.end_tick - ref_ctx.start_tick) > PPQ &&
               current_tick > ref_ctx.start_tick;
      }
      case Predicate::ContextStartedBeforeUs: {
        return ref_ctx.start_tick < current_tick;
      }
      case Predicate::IsHeavyDownbeat: {
        return (current_tick % (PPQ * 2)) == 0;
      }
      case Predicate::IsOffbeatSyncopation: {
        return (current_tick % (PPQ / 2)) != 0;
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
  using MatrixType = const std::array<std::array<float, 25>, 25>&;

 public:
  static inline float PARSIMONY_LAMBDA = 0.5f;
  static inline int PARSIMONY_THRESHOLD = 30;

  static std::vector<Note> extractRoots(const std::vector<Note>& track) {
    std::vector<Note> roots;
    for (const auto& n : track) {
      if (roots.empty() || roots.back().absolute_tick != n.absolute_tick) {
        roots.push_back(n);
      }
    }
    return roots;
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

    std::mt19937 rng{getGlobalRNG()};
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
    child.compact();
    return child;
  }

  static float calculateMatrixFitness(const std::vector<Note>& W,
                                      MatrixType target_log_matrix) {
    if (W.size() < 3) return -1000.0f;
    float score = 0.0f;
    const Note* __restrict data = W.data();
    size_t sz = W.size();

    for (size_t i = 2; i < sz; i++) {
      int prev_jump = data[i - 1].pitch - data[i - 2].pitch;
      int curr_jump = data[i].pitch - data[i - 1].pitch;

      int prev_idx = std::clamp(prev_jump, -12, 12) + 12;
      int curr_idx = std::clamp(curr_jump, -12, 12) + 12;

      score += target_log_matrix[prev_idx][curr_idx] * 10.0f;
    }

    // if (human_prob == 0.0f) {
    //   // si no existe en el dataset, castigar
    //   score -= 10.0f;
    // } else {
    //   // escalar la probabilidad directamente a puntos de aptitud
    //   score += (human_prob * 100.0f);
    // }

    float average_score = score / static_cast<float>(sz - 2);

    if (sz < 16) {
      average_score -= (16.0f - sz) * 5.0f;
    }

    return average_score;
  }

  static float calculateHarmonyFitness(const Tree& tree,
                                       const std::vector<Note>& seed,
                                       MatrixType log_matrix) {
    auto W_raw = tree.generateMelody(seed, 64.0f, {}, 0.0f);
    auto W = extractRoots(W_raw);

    float score = calculateMatrixFitness(W, log_matrix);

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
                                    const std::vector<Note>& harmony,
                                    MatrixType log_matrix) {
    auto W_raw = tree.generateMelody(seed, 16.0f, harmony, 0.0f);
    auto W = extractRoots(W_raw);
    float score = calculateMatrixFitness(W, log_matrix);

    int consecutive_16ths = 0;
    int max_consecutive_16ths = 0;
    int current_tick = 0;

    for (size_t i = 0; i < W.size(); i++) {
      Note current_harmony = getNoteAtTime(harmony, current_tick);

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

      current_tick += std::round(W[i].duration * PPQ);
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
                                      const std::vector<Note>& harmony,
                                      MatrixType log_matrix) {
    auto W_raw = tree.generateMelody(seed, 16.0f, harmony, 0.0f);
    auto W = extractRoots(W_raw);
    float score = calculateMatrixFitness(W, log_matrix);

    int inflection_points = 0;
    int rhythmic_changes = 0;
    int consecutive_16ths = 0;
    int max_consecutive_16ths = 0;

    int current_tick = 0;
    Note prev_harmony = getNoteAtTime(harmony, 0.0f);

    for (size_t i = 0; i < W.size(); i++) {
      Note current_harmony = getNoteAtTime(harmony, current_tick);

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
      current_tick += std::round(W[i].duration * PPQ);
    }

    float range_penalty = 0.0f;
    for (const auto& n : W) {
      if (n.pitch > 81) {
        range_penalty -= (n.pitch - 81) * 20.0f;
      }
      if (n.pitch < 60) {
        range_penalty -= (60 - n.pitch) * 20.0f;
      }
    }
    score += range_penalty;

    if (inflection_points < 10) score -= 200.0f;
    if (rhythmic_changes < 8) score -= 150.0f;
    if (max_consecutive_16ths > 6) score -= 220.0f;

    std::vector<Note> motif_notes = getNotesInWindow(W, 0, 4 * PPQ);
    std::vector<int> motif_contour = extractContour(motif_notes);

    std::vector<Note> a2_notes = getNotesInWindow(W, 8 * PPQ, 12 * PPQ);
    std::vector<int> a2_contour = extractContour(a2_notes);

    if (!motif_contour.empty() && !a2_contour.empty()) {
      int dist_a2 = calculateEditDistance(motif_contour, a2_contour);
      score += std::max(0.0f, 50.0f - (dist_a2 * 10.0f));
    }

    int node_count = tree.nodes.size();
    if (node_count > PARSIMONY_THRESHOLD) {
      score -= ((node_count - PARSIMONY_THRESHOLD) * PARSIMONY_LAMBDA);
    }
    return score;
  }
};

namespace SeedCrypto {

inline uint32_t encodeNote(const Note& n) {
  uint32_t p = n.pitch & 0x7F;
  uint32_t i = n.intensity & 0x7F;

  uint32_t d = static_cast<uint32_t>(n.duration * 4.0f) & 0x1F;

  return (p << 12) | (i << 5) | d;
}

inline Note decodeNote(uint32_t data) {
  int p = (data >> 12) & 0x7F;
  int i = (data >> 5) & 0x7F;
  float d = static_cast<float>(data & 0x1F) / 4.0f;
  return Note(p, d, i, 0);  // absolute_tick starts at 0 for decoded seeds
}

inline std::string encodeSeed(const std::vector<Note>& seed) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (const auto& n : seed) {
    ss << std::setw(8) << encodeNote(n);
  }
  return ss.str();
}

inline std::vector<Note> decodeSeed(const std::string& hex_str) {
  std::vector<Note> seed;
  int current_t = 0;
  for (size_t i = 0; i < hex_str.length(); i += 8) {
    uint32_t data;
    std::stringstream ss;
    ss << std::hex << hex_str.substr(i, 8);
    ss >> data;
    Note n = decodeNote(data);
    n.absolute_tick =
        current_t;
    current_t +=
        static_cast<int>(std::round(n.duration * PPQ));
    seed.push_back(n);
  }
  return seed;
}
}  // namespace SeedCrypto

namespace SeedRegistry {
const std::unordered_map<std::string, std::string> DICTIONARY = {
    {"duel",
     "0003ca0400040a0200043a0200048a0400043a0200040a020003ca040003ea0200040a020"
     "0041a0200043a0400045a0200047a0200048a0400043a0200040a020003ca040003ca04"},

    {"melancholy", "0003c5040003f5040003c5040003f504"},

    {"war", "0003000200033002000360020003900200030002"},

    {"cinematic", "0003c002000400020004300200048004"}};


inline std::string resolve(const std::string& input) {
  auto it = DICTIONARY.find(input);
  if (it != DICTIONARY.end()) {
    return it->second;
  }

  return input;
}
}  // namespace SeedRegistry

void appendTrack(std::vector<Note>& dest, const std::vector<Note>& src,
                 int time_offset_ticks) {
  for (Note n : src) {
    n.absolute_tick += time_offset_ticks;
    dest.push_back(n);
  }
}

std::vector<Note> shiftPitch(std::vector<Note> track, int semitones) {
  for (Note& n : track) {
    n.pitch += semitones;

    if (n.pitch > 108) n.pitch -= 12;
    if (n.pitch < 21) n.pitch += 12;
  }
  return track;
}

inline void applyHumanDynamics(std::vector<Note>& track,
                               bool is_chaos_section = false) {
  std::mt19937& rng = getGlobalRNG();
  std::normal_distribution<float> jitter(0.0f, 10.0f);

  for (Note& n : track) {
    // currently commented cause i dont think it betters the output, but its worth enough to not delete it
    // int shift = static_cast<int>(std::round(jitter(rng)));
    // n.absolute_tick += shift;

    // prevent time-travel lol (cannot play before tick 0)
    if (n.absolute_tick < 0) n.absolute_tick = 0;
    if (n.absolute_tick % (PPQ * 4) <= 15) {
      n.intensity = 100;
    } else if (n.absolute_tick % PPQ <= 15) {
      n.intensity = 85;
    } else if (n.absolute_tick % (PPQ / 2) <= 15) {
      n.intensity = 70;
    } else {
      n.intensity = 55;
    }

    if (is_chaos_section) {
      n.intensity = std::min(127, static_cast<int>(n.intensity * 1.25f));
    }
  }
}