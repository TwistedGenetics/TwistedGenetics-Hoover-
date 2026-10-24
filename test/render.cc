// Desktop tests + preview renderer for the Hoover DSP core (core/hoover_dsp.h).
// Run via test/run.sh. Writes preview WAVs to test/out/ and exits non-zero on failure.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "hoover_dsp.h"
#include "wav.h"

using namespace hoover;

struct NoteEv { float startSec, lenSec; float note; };

// Render a sequence of notes with a simple per-voice gate (stand-in for the mk2 amp EG)
static std::vector<float> renderSeq(Engine &e, const std::vector<NoteEv> &seq, float totalSec) {
  const int total = (int)(totalSec * kSampleRate);
  std::vector<float> out(total, 0.f);
  float buf[kMaxBlock];
  float amp[kMaxVoices] = {0};
  for (int pos = 0; pos < total; pos += kMaxBlock) {
    const int n = std::min(kMaxBlock, total - pos);
    const float t0 = pos / kSampleRate;
    for (size_t v = 0; v < seq.size() && v < (size_t)kMaxVoices; v++) {
      const NoteEv &ev = seq[v];
      if (t0 >= ev.startSec && t0 - kMaxBlock / kSampleRate < ev.startSec) e.trigger((int)v);
      const bool on = t0 >= ev.startSec && t0 < ev.startSec + ev.lenSec;
      e.render((int)v, ev.note, buf, n);
      for (int i = 0; i < n; i++) {
        const float target = on ? 1.f : 0.f;
        amp[v] += (target - amp[v]) * (on ? 0.01f : 0.0008f);  // fast attack, ~25 ms release
        out[pos + i] += buf[i] * amp[v];
      }
    }
  }
  return out;
}

// Crude aliasing probe: energy above 16 kHz for a high note, via Goertzel-free DFT bins
static double hfRatio(const std::vector<float> &x) {
  const int N = 4096;
  if ((int)x.size() < N + 48000) return 0;
  const float *s = x.data() + 24000;
  double hi = 0, all = 0;
  for (int k = 1; k < N / 2; k += 3) {
    double re = 0, im = 0;
    for (int n = 0; n < N; n++) {
      const double w = 0.5 - 0.5 * std::cos(2 * M_PI * n / (N - 1));
      re += s[n] * w * std::cos(2 * M_PI * k * n / N);
      im -= s[n] * w * std::sin(2 * M_PI * k * n / N);
    }
    const double p = re * re + im * im;
    all += p;
    if (k * 48000.0 / N > 16000) hi += p;
  }
  return all > 0 ? hi / all : 0;
}

int main(int argc, char **argv) {
  const char *outDir = argc > 1 ? argv[1] : "test/out";
  const char *modeNames[kNumModes] = {"hoover", "mentasm", "reese", "rave"};
  int failures = 0;

  // 1) Each mode: a jungle-ish riff, default knobs
  for (int m = 0; m < kNumModes; m++) {
    Engine e;
    e.params.mode = m;
    if (m == kModeReese) { e.params.dive = 0.f; e.params.detune = 0.35f; e.params.sub = 0.5f; }
    std::vector<NoteEv> seq = {{0.0f, 0.9f, 43}, {1.0f, 0.4f, 46}, {1.5f, 0.4f, 41}, {2.0f, 1.2f, 43}};
    if (m != kModeReese) seq = {{0.0f, 1.4f, 55}, {1.5f, 0.6f, 58}, {2.2f, 1.2f, 53}};
    auto out = renderSeq(e, seq, 3.6f);
    Stats s = stats(out);
    std::printf("%-8s peak %.3f  rms %.3f  dc %+.4f  %s\n", modeNames[m], s.peak, s.rms, s.dc,
                s.finite ? "finite" : "NaN/INF!");
    if (!s.finite || s.peak > 1.0 || std::fabs(s.dc) > 0.02 || s.rms < 0.02) failures++;
    write_wav((std::string(outDir) + "/hoover_" + modeNames[m] + ".wav").c_str(), out, out);
  }

  // 2) Stress: every knob at max / min, extreme notes, all 8 voices
  for (int corner = 0; corner < 2; corner++) {
    for (int m = 0; m < kNumModes; m++) {
      Engine e;
      Params &p = e.params;
      const float k = corner ? 1.f : 0.f;
      p.mode = m; p.detune = k; p.dive = corner ? 1.f : -1.f; p.diveTime = k; p.pwm = k;
      p.pwmRate = k; p.chorus = k; p.sub = k; p.drive = k; p.drift = k; p.transpose = corner ? 24.f : -24.f;
      for (int d = 0; d < kNumModDest; d++)
        for (int v = 0; v < kMaxVoices; v++) e.mod[d][v] = corner ? 1.f : -1.f;
      std::vector<NoteEv> seq;
      for (int v = 0; v < kMaxVoices; v++) seq.push_back({0.f, 2.f, (float)(corner ? 100 + v * 3 : v * 3)});
      auto out = renderSeq(e, seq, 2.f);
      Stats s = stats(out);
      // 8 voices summed, so peak can exceed 1 here; only flag non-finite or runaway
      if (!s.finite || s.peak > 8.0) {
        std::printf("STRESS FAIL corner=%d mode=%d peak=%.3f finite=%d\n", corner, m, s.peak, s.finite);
        failures++;
      }
    }
  }
  std::printf("stress corners: %s\n", failures ? "see above" : "all finite, bounded");

  // 3) Aliasing probe: C7 (2093 Hz) saw stack, drive off. Energy above 16k should be small.
  {
    Engine e;
    e.params.mode = kModeHoover; e.params.drive = 0.f; e.params.dive = 0.f; e.params.sub = 0.f;
    auto out = renderSeq(e, {{0.f, 2.f, 96}}, 2.f);
    const double r = hfRatio(out);
    std::printf("alias probe (C7, >16kHz energy share): %.4f %s\n", r, r < 0.05 ? "ok" : "HIGH");
    if (r >= 0.05) failures++;
  }


  // 4b) Pitch dive: +12 should start ~an octave up and settle on pitch; -12 mirrors it
  {
    static float x[48000];
    auto pitchAt = [&](int a, int len) {
      int best = 0; double bv = -1e9;
      for (int lag = 40; lag < 800; lag++) {
        double s = 0, n0 = 0, n1 = 0;
        for (int i = a; i < a + len; i++) { s += x[i] * x[i + lag]; n0 += x[i] * x[i]; n1 += x[i + lag] * x[i + lag]; }
        const double r = s / std::sqrt(n0 * n1 + 1e-12);
        if (r > bv + 0.02) { bv = r; best = lag; }
      }
      return 48000.0 / best;
    };
    for (int dv : {12, -12}) {
      Engine e; Params &p = e.params;
      p.mode = kModeReese; p.detune = 0; p.chorus = 0; p.drift = 0; p.drive = 0; p.sub = 0;
      p.dive = dv / 24.f; p.diveTime = 0.35f;
      e.trigger(0, true);
      float buf[kMaxBlock];
      for (int i = 0; i < 48000; i += kMaxBlock) { e.render(0, 57.f, buf, kMaxBlock); for (int j = 0; j < kMaxBlock; j++) x[i + j] = buf[j]; }
      const double start = pitchAt(96, 480), settled = pitchAt(38400, 480);
      const bool ok = (dv > 0 ? start > 360 : start < 140) && std::fabs(settled - 220.0) < 6.0;
      std::printf("dive %+d: starts %.0f Hz, settles %.0f Hz (A3 = 220) %s\n", dv, start, settled, ok ? "ok" : "FAIL");
      if (!ok) failures++;
    }
  }

  // 4c) Drive: 0 is clean (high crest), 100 % is heavily clipped (crest near 1), monotonic in between
  {
    double prev = 1e9; bool mono = true, ok = true;
    std::printf("drive crest:");
    for (float d : {0.f, 0.25f, 0.5f, 0.75f, 1.f}) {
      Engine e; e.params.mode = kModeHoover; e.params.dive = 0.f; e.params.drive = d; e.trigger(0, true);
      float buf[kMaxBlock]; double sq = 0, pk = 0; const int N = 48000;
      for (int i = 0; i < N; i += kMaxBlock) { e.render(0, 45.f, buf, kMaxBlock); for (float v : buf) { sq += v * v; pk = std::max(pk, (double)std::fabs(v)); } }
      const double crest = pk / std::sqrt(sq / N);
      std::printf(" %.0f%%=%.2f", d * 100, crest);
      if (crest > prev + 0.05) mono = false;
      if (d == 0.f && crest < 2.0) ok = false;
      if (d == 1.f && crest > 1.2) ok = false;
      prev = crest;
    }
    std::printf("  %s\n", (mono && ok) ? "ok" : "FAIL");
    if (!(mono && ok)) failures++;
  }

  // 4) Throughput: 8 voices, heaviest mode (4 members with twins)
  {
    Engine e;
    e.params.mode = kModeMentasm;
    float buf[kMaxBlock];
    const int blocks = 48000 * 10 / kMaxBlock;  // 10 s of audio
    auto t0 = std::chrono::steady_clock::now();
    float sink = 0;
    for (int b = 0; b < blocks; b++)
      for (int v = 0; v < kMaxVoices; v++) {
        e.render(v, 48.f + v, buf, kMaxBlock);
        sink += buf[0];
      }
    auto t1 = std::chrono::steady_clock::now();
    const double sec = std::chrono::duration<double>(t1 - t0).count();
    std::printf("throughput: 10 s x 8 voices rendered in %.3f s on host (%.1f%% realtime) [sink %g]\n",
                sec, sec * 10.0, sink);
  }

  std::printf(failures ? "FAILURES: %d\n" : "ALL CHECKS PASSED\n", failures);
  return failures ? 1 : 0;
}
