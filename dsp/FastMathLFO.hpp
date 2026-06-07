#pragma once
#include <cmath>
#include <cstdint>
#include "DspMath.hpp"
#include "FastMath.hpp"
#include "SmoothedValue.hpp"

namespace graindr {

constexpr float oneOverTwoPi = 1.0f / kTwoPi;

struct NormalisedPhase {
    void reset() noexcept { phase = 0.0f; }
    float advance(float increment, float phaseShift = 0.0f) noexcept {
        auto offset = phaseShift * oneOverTwoPi;
        auto last = phase;
        auto next = last + increment;
        while ((next + offset) >= 1.0f) next -= 1.0f;
        phase = next;
        return last + offset;
    }
    float phase = 0.0f;
};

inline float polyBlep(float arg, float increment, float modFactor = 1.0f) {
    auto incr = modFactor * increment;
    if (arg < incr)        { auto t = arg / incr;          return t + t - t * t - 1.0f; }
    if (arg > 1.0f - incr) { auto t = (arg - 1.0f) / incr; return t + t + t * t + 1.0f; }
    return 0.0f;
}

// Deterministic replacement for juce::Random (nextFloat() -> [0,1)).
class Lcg {
public:
    float nextFloat() {
        state = state * 1664525u + 1013904223u;
        return float(state >> 8) / float(1u << 24); // [0,1)
    }
private:
    uint32_t state = 0x12345678u;
};

class FastMathLFO {
public:
    enum LFOWave { SIN, TRI, SAW, RMP, SQR, RSH };
    enum LFOPolarity { UNIPOLAR, BIPOLAR };
    LFOPolarity polarity = UNIPOLAR;

    FastMathLFO() = default;

    void reset(float sampleRate) {
        fs = sampleRate;
        depth.reset(sampleRate, 0.05);
        rshState.reset(sampleRate, 0.001);
        resetPhase();
    }
    void resetPhase() {
        phase.reset();
        rshCounter = 0;
        rshState.setCurrentAndTargetValue(0.0f);
        phaseIncrement.setCurrentAndTargetValue(phaseIncrement.getTargetValue());
    }
    void setParams(float freq, float depth_pct, LFOWave _waveform, LFOPolarity _polarity) {
        phaseIncrement.setTargetValue(freq / fs);
        depth.setTargetValue(jlimit(0.0f, 1.0f, depth_pct * 0.01f));
        waveform = _waveform;
        polarity = _polarity;
    }

    float getNextSample(float phaseShift) {
        float bipolarSample = 0.0f;
        auto nextIncr = phaseIncrement.getNextValue();
        if (waveform == RSH) {
            int thresh = (int)(1.0f / (2.0f * nextIncr));
            if (rshCounter >= thresh) { rshState.setTargetValue(rng.nextFloat()*2.0f-1.0f); rshCounter=0; }
            else rshCounter++;
            bipolarSample = rshState.getNextValue();
            phase.advance(nextIncr);
        } else {
            bipolarSample = waveSample(waveform, nextIncr, phaseShift);
        }
        auto halfDepth = 0.5f * depth.getNextValue();
        if (polarity == UNIPOLAR) return halfDepth * (bipolarSample + 1.0f);
        return halfDepth * bipolarSample;
    }

private:
    float fs = 44100.0f;
    SmoothedValueLinear phaseIncrement{0.0f};
    SmoothedValueLinear depth{0.0f};
    LFOWave waveform = SIN;
    NormalisedPhase phase;
    int rshCounter = 0;
    Lcg rng;
    SmoothedValueLinear rshState{0.0f};

    // Direct port of the JUCE lambdas (Utils.h getWaveFunc), as a switch.
    float waveSample(LFOWave wave, float increment, float phaseShift) {
        switch (wave) {
            case SIN: {
                auto arg = phase.advance(increment, 0.25f + phaseShift);
                if (arg < 0.5f) return FastMath::sin(kTwoPi * (arg - 0.25f));
                return 0.0f - FastMath::sin(kTwoPi * (arg - 0.75f));
            }
            case TRI: {
                auto arg = phase.advance(increment, 0.25f + phaseShift);
                return 1.0f - 2.0f * std::fabs(2.0f * arg - 1.0f);
            }
            case SAW: {
                auto arg = phase.advance(increment, phaseShift);
                return 1.0f - 2.0f * arg + polyBlep(arg, increment);
            }
            case RMP: {
                auto arg = phase.advance(increment, phaseShift);
                return 2.0f * arg - 1.0f - polyBlep(arg, increment);
            }
            case SQR: {
                auto arg = phase.advance(increment, phaseShift);
                float val = arg < 0.5f ? 1.0f : -1.0f;
                val += polyBlep(arg, increment);
                auto argModulo = (arg - 0.5f) - std::floor(arg - 0.5f);
                val -= polyBlep(argModulo, increment);
                return val;
            }
            default: return 0.0f;
        }
    }
};

} // namespace graindr
