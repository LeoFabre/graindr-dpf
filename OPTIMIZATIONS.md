# Optimization notes

This document describes the CPU optimization work done on the Graindr DSP for the
production setup on Bela (PocketBeagle2, Cortex-A53, aarch64). Builds use
`-O3 -march=armv8-a -mtune=cortex-a53`. All optimizations were validated with an
offline A/B regression gate (the same audio rendered through two git refs, float
dumps compared in dBFS), unit tests, and real-device benchmarks on the Cortex-A53.

**Headline numbers (Cortex-A53):**

- Tiers 1–3 + fastTanh: 3271 → 2565 ns/sample = **1.27×**
- PS2 removal (digital path): 1765 → 914 ns/sample = **1.93×** (xRT 11.8 → 22.8)

**Branch reality:** `main` carries Tiers 1–3 plus the PS2 removal. The fastTanh
soft-clipper (`cecc43b`) currently lives on the `opt/tier4-graindr` branch, which
diverged before the PS2 removal and is not merged into `main`.

## What was removed

### The second granular line (PS2) — `d39558e`

This is the biggest win in the repo, and it is a removal, not an optimization.

The original container always processed **two** granular pitch-shifter lines
(PS1 + PS2) plus the PS1/PS2 routing, mixed 50/50 by default
(`ps12OutBalance = 0.5`). In real use only one line is ever driven — and with
identical default parameters the two lines produced the same output (one line's
sound, computed twice). That is pure wasted CPU on every sample.

PS2 was ripped out entirely — DSP, parameters, and UI — rather than skipped with a
per-sample conditional. Full removal was chosen deliberately for real-time
determinism: constant cost, lowest worst-case execution time, and no branch in the
audio callback.

- Output = PS1 post-delay; the PS1 signal path is untouched, so PS1-only output is
  **bit-identical** to the previous build. Only the unused second line and its
  50/50 mix are gone.
- Parameters were renumbered (DryWet = 0, PS1 = 1..10, Mod = 11..14). PS1/Mod
  symbols are kept stable (`ps1*` / `psMod*`) so name-based hosts are unaffected;
  index-based hosts realign.

Measured on the Cortex-A53 (digital path): 1765 → 914 ns/sample = **1.93×**,
real-time headroom (xRT) 11.8 → 22.8.

## What was optimized

### Tier 1 — flush-to-zero denormals (`5235276`)

`dsp/DenormalGuard.hpp` arms FPCR.FZ (flush-to-zero) once on the audio thread at
the top of `Plugin::run()`. aarch64-only, no-op on host builds. Protects the
feedback-delay decay tails from the Cortex-A53 denormal penalty.

### Tier 2 — numerically-equivalent per-sample wins (`5235276`)

Safe rewrites (same math, cheaper form): `pow(x, 2)` → `x*x`,
reciprocal-multiply, hoisted loop-invariant coefficients, `__restrict` on hot
buffers. Validated bit-equivalent (or below −80 dB residual) by the offline gate.

### Tier 3 — fast-math on the DSP objects only (`8f462d3`)

`-funsafe-math-optimizations` applied to the Graindr DSP compilation units only.
Gated by the same A/B comparison.

### Tier 4a — fastTanh soft-clipper, ANALOG mode (`cecc43b`, branch `opt/tier4-graindr`)

The ANALOG-mode soft clipper called `std::tanh` twice per sample. It was replaced
by a [7/6] Padé `fastTanh` clamped to ±1 at |x| ≥ 4.9 (just below where the raw
rational creeps above 1 and then sags) — bounded, monotonic, saturating, no
NaN/Inf.

The clipper sits inside a ×3.98 feedback loop, so accuracy matters: the Padé
matches `std::tanh` to < 1.1e-4, and tanh is contractive so the small error does
not grow in the loop.

- Gate vs `std::tanh`: analog −138.5 dB, digital **bit-identical**, and a
  dedicated hard-driven worst-case preset (95% feedback + full shimmer, tanh deep
  in saturation) −117.2 dB — all far below the −80 dB tolerance.
- **Per-mode caveat:** this approximation is only ever executed in ANALOG mode;
  DIGITAL mode never calls the clipper and is untouched.

## What was tried and rejected

Nothing was fully abandoned in this repo; the noteworthy non-obvious decision is
documented above — removing PS2 outright instead of branching around it, traded
flexibility for worst-case-execution-time determinism in the audio callback.

## Validation

- **Offline A/B regression gate**: identical input rendered through two git refs,
  float output dumps compared in dBFS. Thresholds are calibrated: bit-identical
  reads as ≈ −999 dB, sub-audible drift ≈ −85 dB, a real regression shows up
  around −14 dB.
- **Unit tests**: the FastMath tests assert accuracy, monotonicity, boundedness
  and hard-saturation behavior of the Padé tanh.
- **Real-device benchmarks**: ns/sample and xRT measured on the Cortex-A53 itself.
