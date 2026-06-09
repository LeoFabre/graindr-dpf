#pragma once

#include "GranularPitchShifter.hpp"
#include "EnvelopeDetector.hpp"
#include "FastMathLFO.hpp"
#include "VASVFilter.hpp"
#include "DspMath.hpp"

namespace graindr {

// Single granular delay line. (A second line / PS2 + the PS1/PS2 routing was
// removed — production only ever used one line, and processing a second was
// pure wasted CPU. ~2x cheaper, deterministic cost, no per-sample branch.)
class PitchShifterContainer
{
public:
    PitchShifterContainer() {}

    void reset(float sampleRate)
    {
        fs = sampleRate;

        ps1.reset(fs);

        lfoPhaseShift.reset(fs, 0.1f);
        lfo.reset(fs);
        maxModDepth_smpls = MAX_MOD_DEPTH_SECS * fs;

        resetEnvelopeDetectorBlock();
        envelopeDetectorActive = ps1.isStreching();
    }

    void processBlock(float* buffer, const int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            auto x = buffer[n];

            bool triggerStrech = false;
            updateEnvelopeDetectorState(ps1.isStreching());
            if (envelopeDetectorActive)
            {
                auto envlpIn = preDetectorFilter.processSample(x);
                auto envlpOut = envelopeDetector.processSample(envlpIn);
                auto delta = envlpOut.envelope - TRACKING_THRESHOLD;
                float modVal = 0.0f;
                if (delta > 0)
                    modVal = jlimit(0.0f, 1.0f, delta * TRACKING_SENSITIVITY);

                if (!noteOn && modVal >= 0.001f)
                {
                    noteOn = true;
                    triggerStrech = true;
                }
                else if (noteOn && modVal < 0.001f)
                {
                    noteOn = false;
                }
            }

            auto modulation_smpls = lfo.getNextSample(lfoPhaseShift.getNextValue()) * maxModDepth_smpls;

            auto ps1OutputBuffersOut = ps1.processOutputBuffers(modulation_smpls, triggerStrech);
            auto ps1PostDelOut = ps1.processPostDelay(ps1OutputBuffersOut, modulation_smpls);
            ps1.writeInputBuffer(x, ps1OutputBuffersOut);

            buffer[n] = ps1PostDelOut;
        }
    }

    void setPs1Parameters(float grainSize_ms, float pitchShift, float fineTune, float texture, float strech, float feedback, float shimmer, float shimmerHiCut_Hz, PlaybackDirection playbackDir, ToneType toneType)
    {
        ps1.setParameters(grainSize_ms, pitchShift, fineTune, texture, strech, feedback, shimmer, shimmerHiCut_Hz, playbackDir, toneType);
    }

    void setModParams(float freq, float depth, FastMathLFO::LFOWave wave, float phaseShift)
    {
        lfo.setParams(freq, depth, wave, FastMathLFO::LFOPolarity::UNIPOLAR);
        lfoPhaseShift.setTargetValue(degreesToRadians(phaseShift));
    }

private:
    static constexpr float TRACKING_THRESHOLD = 0.001f;
    static constexpr float TRACKING_SENSITIVITY = 0.25f;

    float fs = 44100.0f;

    float maxModDepth_smpls = MAX_MOD_DEPTH_SECS * 44100.0f;

    GranularPitchShifter ps1;

    bool envelopeDetectorActive = false;

    FastMathLFO lfo;
    SmoothedValueLinear lfoPhaseShift{0.0f};

    StaticVASVFilter preDetectorFilter;
    EnvelopeDetector envelopeDetector;
    bool noteOn = false;

    void resetEnvelopeDetectorBlock()
    {
        preDetectorFilter.reset(fs);
        preDetectorFilter.setParameters(1500.0f, 0.707f, false, false, 0.0f, 0.0f, 0.0f, 1.0f, false);
        envelopeDetector.reset(fs);
        envelopeDetector.setParams(DetectionMode::RMS, 1.5f, 5.0f, false);
        noteOn = false;
    }

    void updateEnvelopeDetectorState(bool activate)
    {
        if (envelopeDetectorActive == activate)
            return;

        if (envelopeDetectorActive && !activate)
        {
            envelopeDetectorActive = false;
            return;
        }

        resetEnvelopeDetectorBlock();
        envelopeDetectorActive = true;
    }
};

} // namespace graindr
