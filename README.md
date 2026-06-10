# graindr-dpf

A DPF (DISTRHO Plugin Framework) port of [Graindr](https://github.com/LeoFabre/Graindr)
— a dual granular pitch-shifter / time-stretcher / feedback-delay — built to run headless
in [Sushi](https://github.com/elk-audio/sushi) on Bela (PocketBeagle2) with a far smaller
footprint than the original JUCE build. Stereo in / stereo out, VST3 + LV2.

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

## Parameters (28)

4 routing (dry/wet, PS1/PS2 in-balance, PS balance) · 10 per pitch-shifter ×2
(pitch, fine-tune, grain size, texture, stretch, feedback, shimmer, shimmer hi-cut,
playback dir, tone) · 4 modulation (freq, depth, wave, stereo phase). State is auto-saved
by DPF (no XML).

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
