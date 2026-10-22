#pragma once
// Minimal 16-bit stereo WAV writer for host-side previews
#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>

static inline void write_wav(const char *path, const std::vector<float> &l, const std::vector<float> &r,
                             int sr = 48000) {
  FILE *f = std::fopen(path, "wb");
  if (!f) return;
  const uint32_t n = (uint32_t)l.size();
  const uint32_t dataBytes = n * 4;
  auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
  auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
  std::fwrite("RIFF", 1, 4, f); w32(36 + dataBytes); std::fwrite("WAVE", 1, 4, f);
  std::fwrite("fmt ", 1, 4, f); w32(16); w16(1); w16(2); w32(sr); w32(sr * 4); w16(4); w16(16);
  std::fwrite("data", 1, 4, f); w32(dataBytes);
  for (uint32_t i = 0; i < n; i++) {
    for (float s : {l[i], r[i]}) {
      float c = s > 1.f ? 1.f : (s < -1.f ? -1.f : s);
      int16_t q = (int16_t)std::lrint(c * 32767.f);
      std::fwrite(&q, 2, 1, f);
    }
  }
  std::fclose(f);
}

struct Stats {
  double peak = 0, rms = 0, dc = 0;
  bool finite = true;
};

static inline Stats stats(const std::vector<float> &x) {
  Stats s;
  double sum = 0, sq = 0;
  for (float v : x) {
    if (!std::isfinite(v)) s.finite = false;
    double a = std::fabs(v);
    if (a > s.peak) s.peak = a;
    sum += v;
    sq += (double)v * v;
  }
  if (!x.empty()) {
    s.dc = sum / x.size();
    s.rms = std::sqrt(sq / x.size());
  }
  return s;
}
