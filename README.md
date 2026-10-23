# Hoover

Hoover, Mentasm, Reese and rave stacks for the **Korg microKORG 2** oscillator slot.

Each voice is a stack of detuned **PWM-saws**: a band-limited saw plus a twin whose phase offset is swept by an LFO. That's the Alpha Juno pulse-saw motion that sits under the classic hoover. On top you get a note-on pitch dive, chorus smear across the stack, analogue drift, a sine sub and soft-clip drive. It's fully polyphonic (up to 8 voices), and the mk2 mod matrix can reach six of its parameters per voice.

It goes much deeper than the HOOV mode in [Rollers](https://github.com/TwistedGenetics/Rollers_Bass), which stays the go-to for sub-weighted bass.

## Download & install

Go to [**Releases**](https://github.com/TwistedGenetics/TwistedGenetics-Hoover-/releases/latest) and download `Hoover.mk2unit`.

Needs microKORG 2 firmware **2.0.0 or later** (the version that added logue SDK support).

- **KORG Sound Librarian** (Korg's official app for v2.0): connect the microKORG 2 by USB, add `Hoover.mk2unit` as a user **oscillator** and send it to the synth.
- **Or USB mass storage:** hold the first option button while powering on, then copy the file into `Units/Oscs/` (inside a `SlotN` folder) on the drive that appears.

Hoover then appears at the end of the oscillator list.

**`HVmeter.mk2unit`** (optional, also in each Release) is the same oscillator with its CPU load shown next to the mode name, e.g. `HOOV 23%` (peak over the last half second; 100 % = out of time, crackles). Load it if you hear crackles when playing big chords.

## Controls

| Page | Knob | Range | What it does |
|---|---|---|---|
| 1 | **Mode** | 4 modes | Stack layout (below) |
| 1 | **Detune** | 0–100 % | Stack spread. Squared curve, so low values give slow Reese beating |
| 1 | **Dive** | −24…+24 st | Pitch offset at note-on that glides back to pitch. + dives down, − swoops up |
| 2 | **DiveTime** | 0–100 % | How long the dive takes to settle, from 5 ms to 2 s |
| 2 | **PWM** | 0–100 % | Twin-saw offset sweep depth. High values push toward an octave-up edge |
| 2 | **PWM Rate** | 0–100 % | PWM LFO, 0.1–8 Hz |
| 2 | **Chorus** | 0–100 % | Pitch smear across the stack (up to ±14 cents, 0.55 Hz) |
| 2 | **Sub** | 0–100 % | Sine sub one octave down (follows the dive) |
| 3 | **Drive** | 0–100 % | Soft-clip saturation with level make-up |
| 3 | **Drift** | 0–100 % | Slow random pitch walk per stack member |
| 3 | **Semi / Fine** | ±24 st / ±50 ct | Transpose |

**Mod matrix destinations:** Detune, Dive, PWM, Chorus, Sub, Drive (per voice). Try EG → Dive or LFO → PWM.

### Modes

| Mode | Stack | Character |
|---|---|---|
| **HOOVER** | 3 PWM-saws | The classic. Wide, moving, Juno-ish |
| **MENTASM** | 3 PWM-saws + octave-down PWM-saw | Darker growl, more weight |
| **REESE** | 2 plain saws | Pure beating Reese |
| **RAVE** | 3 PWM-saws + octave-up PWM-saw | Screamier, cuts through a mix |

## Starter settings

- **Mentasm hoover:** MENTASM, Detune 45, Dive +12, DiveTime 35, PWM 60, Chorus 50. Add portamento and a slow filter opening on the mk2.
- **Rolling Reese:** REESE, Dive 0, Detune 25–40, Chorus 0–20, Sub 50, Drift 30. Keep the mk2 filter low.
- **Rave stab:** RAVE, Dive −7, DiveTime 15, Drive 60, short amp EG.
- **Mod-matrix wobble:** HOOVER, LFO → PWM at about 50 %, slow LFO → Detune at about 20 %.

## Test checklist (by ear on the hardware)

- [ ] All four modes sound distinct, with no clicks on note-on
- [ ] Dive +12 and −12 glide in and settle on pitch; DiveTime 0 is near-instant
- [ ] 8-note chord with MENTASM: no crackles (check `HVmeter`)
- [ ] Mod matrix shows Detune / Dive / PWM / Chorus / Sub / Drive as destinations, and they respond
- [ ] Mono and unison voice modes on the mk2 still sound right

## CPU and memory

The DSP is scalar C++, with roughly 2–8 polyBLEP saws per voice depending on mode (MENTASM and RAVE are heaviest). Voices the synth has released are skipped. The unit is well under the 48 KB osc load limit; CI fails the build if it ever isn't.

## Building

GitHub Actions builds every push with Korg's own toolchain and publishes a Release when the `.version` in `microkorg2/header.c` changes.

Locally (Linux):

```
./build.sh        # fetches Korg's toolchain + logue SDK into .deps/, builds dist/Hoover.mk2unit and dist/HVmeter.mk2unit
./test/run.sh     # desktop DSP tests, no toolchain needed; preview WAVs land in test/out/
```

Or with Korg's Docker setup: copy `core/hoover_dsp.h` next to the files in `microkorg2/` and build it as a microkorg2 project.

## Layout

```
core/hoover_dsp.h       DSP engine (plain C++, no SDK dependencies)
microkorg2/             unit glue, header, Makefile, config.mk
tools/mk2-meter/        HVmeter: same unit with a CPU meter in the MODE display
test/                   desktop tests + preview renders
build.sh                toolchain + SDK fetch and build
```

## Unit ID

dev_id `0x5477476E` ('TwGn'), unit_id `7` (HVmeter: `0x107`).
