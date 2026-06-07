#!/usr/bin/env bash
# Render the same input wav through Graindr-JUCE and Graindr-DPF using an offline
# VST3 host, then compare the outputs sample-by-sample.
#
# Requirements:
#   - An offline VST3 host capable of file-in/file-out rendering. carla-single is
#     the default; replace with whatever you have (pluginval, a bespoke renderer).
#   - GRAINDR_JUCE_VST3 pointing at the Graindr-JUCE bundle (built natively from
#     /Users/lfabre/nexus-workdir/Graindr — see Step 3 in the plan).
#
# Presets (spec §4 — both use psModDepth=0 so the S&H RNG path is never taken):
#   default : both plugins at their factory defaults (directly comparable).
#   stress  : PS1 pitch -12 + stretch 2, PS2 pitch +7 / fineTune +50c, feedback 50%,
#             shimmer 40%, ANALOG tone, cross-feed balances non-zero.
#             NOTE: applying the stress preset requires a host that can set VST3
#             parameters before rendering (carla-single alone cannot). Wire it to
#             your host's parameter-automation mechanism; the values are in the spec.
#
# Acceptance: peak residual ≤ -80 dB (relaxable to -60 dB with a documented reason).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
PRESET="${1:-default}"
FIXTURE="${HERE}/fixtures/input.wav"
DPF_VST3="${HERE}/../build/bin/Graindr.vst3"
JUCE_VST3="${GRAINDR_JUCE_VST3:?Set GRAINDR_JUCE_VST3 to the Graindr-JUCE VST3 bundle}"
THRESHOLD_DB="${THRESHOLD_DB:--80}"

OUT_JUCE="${HERE}/fixtures/out_juce_${PRESET}.wav"
OUT_DPF="${HERE}/fixtures/out_dpf_${PRESET}.wav"

if [[ ! -f "${FIXTURE}" ]]; then
    echo "Generating test fixture..."
    python3 "${HERE}/fixtures/generate_input.py" "${FIXTURE}"
fi

if [[ "${PRESET}" != "default" ]]; then
    echo "WARNING: preset '${PRESET}' requires a host that sets VST3 parameters" >&2
    echo "         before rendering. Ensure your host applies the spec §4 values." >&2
fi

if command -v carla-single >/dev/null 2>&1; then
    carla-single vst3 "${JUCE_VST3}" "${FIXTURE}" "${OUT_JUCE}"
    carla-single vst3 "${DPF_VST3}"  "${FIXTURE}" "${OUT_DPF}"
else
    echo "ERROR: no carla-single found. Install Carla or adapt this script to" >&2
    echo "       your local offline VST3 host (pluginval, etc.)." >&2
    exit 2
fi

python3 "${HERE}/compare_wavs.py" "${OUT_JUCE}" "${OUT_DPF}" --threshold-db "${THRESHOLD_DB}"
