# graindr-dpf

A DPF (DISTRHO Plugin Framework) port of [Graindr](https://github.com/LeoFabre/Graindr)
— a single granular pitch-shifter / time-stretcher / feedback-delay — built to run headless
in [Sushi](https://github.com/elk-audio/sushi) on Bela (PocketBeagle2) with a far smaller
footprint than the original JUCE build. Stereo in / stereo out, VST3 + LV2.

The second granular line (PS2) was removed for real-time determinism; see
[OPTIMIZATIONS.md](OPTIMIZATIONS.md) for details.

## Layout

```
graindr-dpf/
├── dpf/      DISTRHO/DPF submodule (+ recursive pugl)
├── dsp/      pure C++17 inline-header DSP (zero external deps)
├── plugin/   DPF facade (GraindrPlugin) + ParameterMetadata + optional NanoVG UI
└── test/     unit tests (CTest) + null-test harness
```

The DSP is a faithful port of the JUCE original. Fidelity-critical pieces are replicated
exactly: `FastMath` (JUCE Padé sin/tan), `SmoothedValue` (Linear + Multiplicative ramps),
and `DryWetMixer` (sin3dB equal-power crossfade). See
`docs/superpowers/specs/2026-06-07-graindr-juce-to-dpf-design.md` in the parent repo.

## Parameters (15)

1 dry/wet · 10 PS1 (single granular line) · 4 modulation. State is auto-saved by DPF (no XML).

| Index | Symbol | Name | Range | Default | Unit |
|------:|--------|------|-------|---------|------|
| 0 | `dryWet` | Dry/Wet | 0 – 1 | 0.5 | — |
| 1 | `ps1PitchShift` | PS1 Pitch Shift | −12 – 12 | 0 | semitones |
| 2 | `ps1FineTune` | PS1 Fine Tune | −100 – 100 | 0 | cents |
| 3 | `ps1GrainSize` | PS1 Grain Size | 1 – 1000 | 50 | ms |
| 4 | `ps1Texture` | PS1 Texture | 0 – 1 | 0.5 | — |
| 5 | `ps1Stretch` | PS1 Stretch | 1 – 4 | 1 | — |
| 6 | `ps1Feedback` | PS1 Feedback | 0 – 100 | 0 | % |
| 7 | `ps1Shimmer` | PS1 Shimmer | 0 – 100 | 0 | % |
| 8 | `ps1ShimmerHiCut` | PS1 Hi Cut | 20 – 22000 | 22000 | Hz |
| 9 | `ps1PlaybackDir` | PS1 Playback | 0 FORWARD / 1 REVERSE / 2 ALTERNATE | 0 | — |
| 10 | `ps1ToneType` | PS1 Tone | 0 ANALOG / 1 DIGITAL | 1 | — |
| 11 | `psModFreq` | Mod Freq | 0.02 – 10 | 0.02 | Hz |
| 12 | `psModDepth` | Mod Depth | 0 – 100 | 0 | % |
| 13 | `psModWave` | Mod Wave | 0 SIN / 1 TRI / 2 SAW / 3 RMP / 4 SQR / 5 S&H | 0 | — |
| 14 | `psModStereoPhase` | Mod Stereo Phase | 0 – 180 | 0 | deg |

## Build

### Host (desktop), headless — for tests and offline VST3

```bash
cmake -S . -B build -DGRAINDR_BUILD_UI=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure        # 11 unit tests
```

Headless VST3 bundle: `build/bin/Graindr.vst3` (~248 KB).

### Host, with the optional NanoVG debug UI

```bash
cmake -S . -B build-ui -DGRAINDR_BUILD_UI=ON
cmake --build build-ui -j
```

The UI is a minimal grid-of-knobs debug aid, not a reproduction of the JUCE GUI.

### Cross-compile for Bela (aarch64), headless

From the parent integration repo:

```bash
./cross-build-dpf-plugin.sh plugins/graindr-dpf          # headless (default)
./cross-build-dpf-plugin.sh plugins/graindr-dpf --ui     # with UI
```

Output: `build-arm64/graindr-dpf/Graindr.vst3` and `Graindr.lv2`. Requires Docker and the
`elk-crossbuild-bookworm` image.

## Deploy to Bela

```bash
./deploy-plugin.sh Graindr --config bela-project/sushi-config-graindr.json
```

## Status

| Gate | State |
|------|-------|
| DSP unit tests (host) | ✅ 11/11 pass |
| Headless VST3 < 1 MB | ✅ ~248 KB |
| UI build (host) | ✅ links with `GRAINDR_BUILD_UI=ON` |
| Null-test ≤ −80 dB vs JUCE | ⏳ harness ready; needs an offline VST3 host (e.g. Carla) + the JUCE build |
| ARM64 cross-build | ⏳ needs Docker + elk image |
| Bela load + RSS delta | ⏳ needs a reachable Bela board |

## Null-test

See [`test/README.md`](test/README.md). Acceptance: peak residual ≤ −80 dB vs the JUCE
original (relaxable to −60 dB with a documented reason). Both presets use `psModDepth=0`
so the deliberately non-bit-faithful S&H RNG path is never exercised in the comparison.

## Performance

See [OPTIMIZATIONS.md](OPTIMIZATIONS.md) for the Cortex-A53 optimization work, the PS2 removal, and measured gains.

## License

GPL-3.0-or-later (matches DPF and the JUCE original).
