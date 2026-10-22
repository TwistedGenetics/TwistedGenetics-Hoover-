/*
 *  File: unit.cc
 *
 *  Hoover - microKORG 2 oscillator unit: SDK glue around hoover::Engine (core/hoover_dsp.h).
 *
 *  Up to 8 voices per render call. Voices the synth has deallocated are skipped
 *  (silence) to save CPU; if the firmware never sends voice events, every voice renders.
 *  The mk2 mod matrix can target Detune, Dive, PWM, Chorus, Sub and Drive per voice.
 *
 *  Built with -DHOOVER_LOADMETER (tools/mk2-meter), the MODE display also
 *  shows the CPU load of this unit, e.g. "HOOV 23%".
 */

#include "unit.h"
#include "runtime.h"
#include "unit_osc.h"
#include "utils/io_ops.h"
#include "utils/mk2_utils.h"
#include "hoover_dsp.h"
#ifdef HOOVER_LOADMETER
#include <stdio.h>
#include <time.h>
#endif

namespace {

enum {
  kParamMode = 0,
  kParamDetune,
  kParamDive,
  kParamDiveTime,
  kParamPwm,
  kParamPwmRate,
  kParamChorus,
  kParamSub,
  kParamDrive,
  kParamDrift,
  kParamSemi,
  kParamFine,
  kNumParams
};

unit_runtime_desc_t s_desc;
hoover::Engine s_engine;
int32_t s_params[kNumParams];
float s_buf[hoover::kMaxBlock];

// Voice activity from unit_osc_voice_event; idle voices output silence.
bool s_active[kMk2MaxVoices];
bool s_events_seen = false;

#ifdef HOOVER_LOADMETER
// Time spent in unit_render as a share of the buffer's real-time length.
// Peak over ~0.5 s. 100 % = no time left (crackles).
float s_load_peak = 0.f, s_load_show = 0.f;
uint32_t s_load_frames = 0;
char s_mode_str[16];
inline double nowNs() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

inline const unit_runtime_osc_context_t *context() {
  return static_cast<const unit_runtime_osc_context_t *>(s_desc.hooks.runtime_context);
}

void applyParam(uint8_t id, int32_t value) {
  s_params[id] = value;
  hoover::Params &p = s_engine.params;
  switch (id) {
    case kParamMode:     p.mode = clipminmaxi32(0, value, hoover::kNumModes - 1); break;
    case kParamDetune:   p.detune = value * 0.01f; break;
    case kParamDive:     p.dive = value * (1.f / 24.f); break;
    case kParamDiveTime: p.diveTime = value * 0.01f; break;
    case kParamPwm:      p.pwm = value * 0.01f; break;
    case kParamPwmRate:  p.pwmRate = value * 0.01f; break;
    case kParamChorus:   p.chorus = value * 0.01f; break;
    case kParamSub:      p.sub = value * 0.01f; break;
    case kParamDrive:    p.drive = value * 0.01f; break;
    case kParamDrift:    p.drift = value * 0.01f; break;
    case kParamSemi:
    case kParamFine:
      p.transpose = s_params[kParamSemi] + s_params[kParamFine] * 0.01f;
      break;
    default: break;
  }
}

void resetVoices() {
  s_engine.reset();
  s_events_seen = false;
  for (int v = 0; v < kMk2MaxVoices; v++) s_active[v] = false;
}

}  // namespace

__attribute__((used)) int8_t unit_init(const unit_runtime_desc_t *desc) {
  if (!desc) return k_unit_err_undef;
  if (desc->target != unit_header.target) return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;
  if (desc->samplerate != 48000) return k_unit_err_samplerate;

  s_desc = *desc;
  resetVoices();
  for (int i = 0; i < kNumParams; i++) applyParam(i, unit_header.params[i].init);
  return k_unit_err_none;
}

__attribute__((used)) void unit_teardown() {}

__attribute__((used)) void unit_reset() { resetVoices(); }

__attribute__((used)) void unit_resume() {}

__attribute__((used)) void unit_suspend() {}

__attribute__((used)) void unit_render(const float *in, float *out, uint32_t frames) {
  (void)in;
#ifdef HOOVER_LOADMETER
  const double t_start = nowNs();
#endif
  const unit_runtime_osc_context_t *ctx = context();
  if (frames > (uint32_t)hoover::kMaxBlock) frames = hoover::kMaxBlock;  // mk2 renders 64-frame blocks

  const int n = ctx->voiceLimit;
  for (int v = 0; v < n; v++) {
    const bool live = !s_events_seen || s_active[v];
    if (live) {
      s_engine.render(v, ctx->pitch[v], s_buf, (int)frames);
    } else {
      for (uint32_t i = 0; i < frames; i++) s_buf[i] = 0.f;
    }
    const uint32_t offset = GetBufferOffset(ctx, v, (int32_t)frames);
    const uint32_t lane = (uint32_t)(v + ctx->voiceOffset);
    for (uint32_t i = 0; i < frames; i++)
      write_oscillator_output_x1(out, s_buf[i], offset, ctx->outputStride, i, lane);
  }

#ifdef HOOVER_LOADMETER
  const double used = nowNs() - t_start;
  const float load = (float)(used / ((double)frames * (1e9 / 48000.0)));
  if (load > s_load_peak) s_load_peak = load;
  s_load_frames += frames;
  if (s_load_frames >= 24000) {
    s_load_show = s_load_peak;
    s_load_peak = 0.f;
    s_load_frames = 0;
  }
#endif
}

__attribute__((used)) void unit_set_param_value(uint8_t id, int32_t value) {
  if (id >= kNumParams) return;
  applyParam(id, value);
}

__attribute__((used)) int32_t unit_get_param_value(uint8_t id) {
  return (id < kNumParams) ? s_params[id] : 0;
}

__attribute__((used)) const char *unit_get_param_str_value(uint8_t id, int32_t value) {
  switch (id) {
    case kParamMode: {
      if (value < 0 || value >= hoover::kNumModes) return nullptr;
#ifdef HOOVER_LOADMETER
      static const char *kShort[hoover::kNumModes] = {"HOOV", "MENT", "REES", "RAVE"};
      snprintf(s_mode_str, sizeof s_mode_str, "%s %d%%", kShort[value], (int)(s_load_show * 100.f + 0.5f));
      return s_mode_str;
#else
      static const char *kNames[hoover::kNumModes] = {"HOOVER", "MENTASM", "REESE", "RAVE"};
      return kNames[value];
#endif
    }
    default:
      return nullptr;
  }
}

__attribute__((used)) void unit_set_tempo(uint32_t tempo) { (void)tempo; }

// mk2 mod matrix: per-voice offsets for Detune, Dive, PWM, Chorus, Sub, Drive.
__attribute__((used)) void unit_platform_exclusive(uint8_t messageId, void *data, uint32_t dataSize) {
  (void)dataSize;
  const unit_runtime_osc_context_t *ctx = context();
  switch (messageId) {
    case kMk2PlatformExclusiveModData: {
      const int32_t *index = GetModIndex(data);
      const float *depth = GetModDepth(data);
      for (int d = 0; d < hoover::kNumModDest; d++)
        for (int v = 0; v < hoover::kMaxVoices; v++) s_engine.mod[d][v] = 0.f;

      const int voices = ctx->voiceLimit;
      for (int src = 0; src < kNumMk2ModSrc; src++) {
        const int32_t dest = index[src];
        if (dest < 0 || dest >= hoover::kNumModDest) continue;
        // Dive is bipolar over its full range, so scale its mod up to match
        const float scale = (dest == hoover::kModDive) ? 2.f : 1.f;
        for (int v = 0; v < voices; v++) {
          const float *srcData = GetModSourceData(data, src, voices, v);
          s_engine.mod[dest][v] += srcData[0] * depth[src] * scale;
        }
      }
      break;
    }

    case kMk2PlatformExclusiveModDestName: {
      static const char *kNames[hoover::kNumModDest] = {"Detune", "Dive", "PWM", "Chorus", "Sub", "Drive"};
      char *name = GetModDestNameData(data);
      const uint8_t dest = (uint8_t)name[0];
      if (dest < hoover::kNumModDest) {
        const char *s = kNames[dest];
        int i = 0;
        for (; s[i] && i < 8; i++) name[1 + i] = s[i];
        if (i < 8) name[1 + i] = '\0';
      }
      break;
    }

    default:
      break;
  }
}

__attribute__((used)) void unit_osc_voice_event(uint8_t event, uint8_t voice, uint8_t note, uint8_t velocity) {
  (void)note;
  (void)velocity;
  if (voice >= kMk2MaxVoices) return;
  s_events_seen = true;
  switch (event) {
    case k_voice_event_allocation:
      s_engine.trigger(voice, !s_active[voice]);  // idle voice: snap to the new pitch
      s_active[voice] = true;
      break;
    case k_voice_event_steal:
      s_engine.trigger(voice, false);  // keep phase and glide: no click
      s_active[voice] = true;
      break;
    case k_voice_event_release:
      break;  // keep rendering through the amp release
    case k_voice_event_deallocation:
      s_active[voice] = false;
      break;
    default:
      break;
  }
}
