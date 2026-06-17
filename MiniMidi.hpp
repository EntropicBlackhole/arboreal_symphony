#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct Note {
  int pitch;
  float duration;
  int intensity;
  int absolute_tick;

  Note(int p, float d, int i, int tick = 0)
      : pitch(p), duration(d), intensity(i), absolute_tick(tick) {}
};

struct MidiEvent {
  int absolute_tick;
  uint8_t status_byte;
  uint8_t pitch;
  uint8_t velocity;

  bool operator<(const MidiEvent& other) const {
    if (absolute_tick == other.absolute_tick) {
      return status_byte < other.status_byte;
    }
    return absolute_tick < other.absolute_tick;
  }
};

class MiniMidi {
 private:
  std::vector<uint8_t> buffer;
  int ticks_per_quarter = 480;
  int current_bpm = 120;

  void write32(uint32_t v) {
    buffer.push_back((v >> 24) & 0xFF);
    buffer.push_back((v >> 16) & 0xFF);
    buffer.push_back((v >> 8) & 0xFF);
    buffer.push_back(v & 0xFF);
  }

  void write16(uint16_t v) {
    buffer.push_back((v >> 8) & 0xFF);
    buffer.push_back(v & 0xFF);
  }

  void writeVarLen(std::vector<uint8_t>& track, uint32_t value) {
    uint32_t buf = value & 0x7F;
    while ((value >>= 7)) {
      buf <<= 8;
      buf |= ((value & 0x7F) | 0x80);
    }
    while (true) {
      track.push_back(buf & 0xFF);
      if (buf & 0x80)
        buf >>= 8;
      else
        break;
    }
  }

  std::vector<std::vector<uint8_t>> tracks;

 public:
  void setBPM(int bpm) { current_bpm = bpm; }
  void addTrack(const std::vector<Note>& notes, uint8_t channel,
                uint8_t instrument) {
    std::vector<uint8_t> trk;

    writeVarLen(trk, 0);
    trk.push_back(0xC0 | channel);
    trk.push_back(instrument);

    std::vector<MidiEvent> events;
    for (const auto& note : notes) {
      uint32_t duration_ticks =
          static_cast<uint32_t>(std::round(note.duration * ticks_per_quarter));

      events.push_back({note.absolute_tick,
                        static_cast<uint8_t>(0x90 | channel),
                        static_cast<uint8_t>(note.pitch),
                        static_cast<uint8_t>(note.intensity)});

      events.push_back({note.absolute_tick + static_cast<int>(duration_ticks),
                        static_cast<uint8_t>(0x80 | channel),
                        static_cast<uint8_t>(note.pitch), 0});
    }

    std::sort(events.begin(), events.end());

    int current_midi_tick = 0;
    for (const auto& ev : events) {
      int delta_time = ev.absolute_tick - current_midi_tick;

      writeVarLen(trk, delta_time);
      trk.push_back(ev.status_byte);
      trk.push_back(ev.pitch);
      trk.push_back(ev.velocity);

      current_midi_tick = ev.absolute_tick;
    }

    writeVarLen(trk, 0);
    trk.push_back(0xFF);
    trk.push_back(0x2F);
    trk.push_back(0x00);

    tracks.push_back(trk);
  }

  void addSustainPedal(int total_ticks, uint8_t channel) {
    std::vector<uint8_t> pedal_track;

    pedal_track.push_back(0x00);
    pedal_track.push_back(0xB0 | channel);
    pedal_track.push_back(64);
    pedal_track.push_back(127);

    int current_tick = 0;
    int measure_ticks = 480 * 2;

    while (current_tick + measure_ticks < total_ticks) {
      current_tick += measure_ticks;

      writeVarLen(pedal_track, measure_ticks - 10);
      pedal_track.push_back(0xB0 | channel);
      pedal_track.push_back(64);
      pedal_track.push_back(0); 

      writeVarLen(pedal_track, 10);
      pedal_track.push_back(0xB0 | channel);
      pedal_track.push_back(64);
      pedal_track.push_back(127);
    }

    pedal_track.push_back(0x00);
    pedal_track.push_back(0xFF);
    pedal_track.push_back(0x2F);
    pedal_track.push_back(0x00);

    tracks.push_back(pedal_track);
  }

  void save(const std::string& filename)  // midi
  {
    buffer.clear();
    buffer.insert(buffer.end(), {'M', 'T', 'h', 'd'});
    write32(6);
    write16(1);
    write16(tracks.size() + 1);
    write16(ticks_per_quarter);

    buffer.insert(buffer.end(), {'M', 'T', 'r', 'k'});
    std::vector<uint8_t> tempo_track;

    tempo_track.insert(tempo_track.end(), {0x00, 0xFF, 0x51, 0x03});

    uint32_t mpqn = 60000000 / current_bpm;
    tempo_track.push_back((mpqn >> 16) & 0xFF);
    tempo_track.push_back((mpqn >> 8) & 0xFF);
    tempo_track.push_back(mpqn & 0xFF);

    tempo_track.insert(tempo_track.end(), {0x00, 0xFF, 0x2F, 0x00});

    write32(tempo_track.size());
    buffer.insert(buffer.end(), tempo_track.begin(), tempo_track.end());

    for (const auto& trk : tracks) {
      buffer.insert(buffer.end(), {'M', 'T', 'r', 'k'});
      write32(trk.size());
      buffer.insert(buffer.end(), trk.begin(), trk.end());
    }

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  }
};