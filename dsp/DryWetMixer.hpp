#pragma once
#include <cmath>
#include "DspMath.hpp"
#include "SmoothedValue.hpp"

namespace graindr {

class DryWetMixer {
public:
    DryWetMixer() = default;
    void prepare(float sampleRate) {
        dryGain.reset(sampleRate, 0.05);
        wetGain.reset(sampleRate, 0.05);
        setWetProportion(wetProp, true);
    }
    void setWetProportion(float p, bool force = false) {
        wetProp = clampf(p, 0.0f, 1.0f);
        const float dg = std::sin(0.5f * kPi * (1.0f - wetProp));
        const float wg = std::sin(0.5f * kPi * wetProp);
        if (force) { dryGain.setCurrentAndTargetValue(dg); wetGain.setCurrentAndTargetValue(wg); }
        else       { dryGain.setTargetValue(dg);           wetGain.setTargetValue(wg); }
    }
    float mix(float dry, float wet) {
        return dryGain.getNextValue() * dry + wetGain.getNextValue() * wet;
    }
private:
    float wetProp = 0.5f;
    SmoothedValueLinear dryGain{0.7071068f}, wetGain{0.7071068f};
};

} // namespace graindr
