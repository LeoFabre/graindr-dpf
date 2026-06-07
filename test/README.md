# Graindr-DPF tests

## Unit tests (CTest)

The DSP test suite is standalone (no DPF) and builds on its own:

```bash
cmake -S . -B build-test          # from plugins/graindr-dpf/test
cmake --build build-test -j
ctest --test-dir build-test --output-on-failure
```

Or as part of the full project (`-DGRAINDR_BUILD_TESTS=ON`, default):

```bash
cmake -S . -B build -DGRAINDR_BUILD_UI=OFF   # from plugins/graindr-dpf
cmake --build build -j
ctest --test-dir build --output-on-failure
```

11 standalone executables exercise the DSP modules in isolation
(`test_dsp_math`, `test_fastmath`, `test_smoothed`, `test_circular`,
`test_vasv_filter`, `test_envelope`, `test_lfo`, `test_granular`,
`test_container`, `test_drywet`, `test_plugin_offline`).

## Null-test against Graindr-JUCE (offline)

`null_test_vs_juce.sh` renders `fixtures/input.wav` through both plugins and
compares the outputs. Acceptance: peak residual ≤ -80 dB (relaxable to -60 dB
with a documented reason — see spec §4).

```bash
# Build the JUCE original first (host VST3):
cmake -S /Users/lfabre/nexus-workdir/Graindr -B /Users/lfabre/nexus-workdir/Graindr/build-host -DCMAKE_BUILD_TYPE=Release
cmake --build /Users/lfabre/nexus-workdir/Graindr/build-host --target Graindr_VST3 -j

export GRAINDR_JUCE_VST3=/Users/lfabre/nexus-workdir/Graindr/build-host/Graindr_artefacts/Release/VST3/Graindr.vst3
./null_test_vs_juce.sh default     # both at factory defaults
./null_test_vs_juce.sh stress      # see note below
```

Default host: `carla-single`. If you use a different offline VST3 host, edit the
script. Both presets use `psModDepth=0` so the (non-bit-faithful) S&H RNG path is
never exercised in the null-test.

**Stress preset:** applying it requires a host that can set VST3 parameters before
rendering (carla-single alone cannot). The parameter values are in spec §4; wire
them to your host's automation mechanism.

The fixture is **not committed** — it is regenerated on demand by
`fixtures/generate_input.py` (pure-stdlib Python, no numpy).
