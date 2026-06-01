#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct Note {
  int pitch;
  float duration;
  int intensity;
  Note(int p, float d, int i) : pitch(p), duration(d), intensity(i) {}
};

class MiniMidi {
 private:
  std::vector<uint8_t> buffer;
  int ticks_per_quarter = 480;

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
  void addTrack(const std::vector<Note>& notes, uint8_t channel,
                uint8_t instrument) {
    std::vector<uint8_t> trk;

    // instrumental
    writeVarLen(trk, 0);
    trk.push_back(0xC0 | channel);
    trk.push_back(instrument);

    for (const auto& note : notes) {
      uint32_t ticks = static_cast<uint32_t>(note.duration * ticks_per_quarter);

      writeVarLen(trk, 0);
      trk.push_back(0x90 | channel);
      trk.push_back(note.pitch);
      trk.push_back(note.intensity);

      writeVarLen(trk, ticks);
      trk.push_back(0x80 | channel);
      trk.push_back(note.pitch);
      trk.push_back(0);
    }

    writeVarLen(trk, 0);
    trk.push_back(0xFF);
    trk.push_back(0x2F);
    trk.push_back(0x00);

    tracks.push_back(trk);
  }

  void save(const std::string& filename)  // midi
  {
    buffer.clear();
    buffer.insert(buffer.end(), {'M', 'T', 'h', 'd'});
    write32(6);
    write16(1);
    write16(tracks.size());
    write16(ticks_per_quarter);

    for (const auto& trk : tracks) {
      buffer.insert(buffer.end(), {'M', 'T', 'r', 'k'});
      write32(trk.size());
      buffer.insert(buffer.end(), trk.begin(), trk.end());
    }

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  }
};