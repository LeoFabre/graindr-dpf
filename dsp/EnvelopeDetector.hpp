#pragma once
#include <cmath>
#include <algorithm>

namespace graindr {

enum DetectionMode { MS, PEAK, RMS };

struct EnvelopeDetectorOutput {
    EnvelopeDetectorOutput() = default;
    EnvelopeDetectorOutput(float e, bool a) : envelope(e), onAttack(a) {}
    float envelope = 0.0f;
    bool  onAttack = false;
};

class EnvelopeDetector {
public:
    EnvelopeDetector() = default;

    void reset(float sampleRate) {
        fs = sampleRate;
        lastEnvelope = 0.0f;
        lastState = State::REL;
        expFactor = TLD_AUDIO_ENVELOPE_ANALOG_TC * 1000.0f / fs;
    }

    void setParams(DetectionMode _dMode, float attackTime_ms, float releaseTime_ms, bool clamp) {
        dMode = _dMode;
        attackTime  = std::exp(expFactor / attackTime_ms);
        releaseTime = std::exp(expFactor / releaseTime_ms);
        clampToOne = clamp;
    }

    EnvelopeDetectorOutput processSample(float x) {
        bool onAttack = false;
        x = std::fabs(x);
        if (dMode == MS || dMode == RMS) x *= x;
        float currentEnvelope = 0.0f;
        if (x > lastEnvelope) {
            currentEnvelope = attackTime * (lastEnvelope - x) + x;
            onAttack = (lastState == State::REL);
            lastState = State::ATK;
        } else {
            currentEnvelope = releaseTime * (lastEnvelope - x) + x;
            lastState = State::REL;
        }
        if (clampToOne) currentEnvelope = std::min(currentEnvelope, 1.0f);
        currentEnvelope = std::max(currentEnvelope, 0.0f);
        lastEnvelope = currentEnvelope;
        if (dMode == RMS) currentEnvelope = std::sqrt(currentEnvelope);
        return EnvelopeDetectorOutput(currentEnvelope, onAttack);
    }

private:
    enum State { ATK, REL };
    const float TLD_AUDIO_ENVELOPE_ANALOG_TC = std::log(0.368f);
    float fs = 44100.0f;
    float lastEnvelope = 0.0f;
    State lastState = State::REL;
    float expFactor = TLD_AUDIO_ENVELOPE_ANALOG_TC * 1000.0f / 44100.0f;
    DetectionMode dMode = PEAK;
    float attackTime = 0.0f, releaseTime = 0.0f;
    bool  clampToOne = true;
};

} // namespace graindr
