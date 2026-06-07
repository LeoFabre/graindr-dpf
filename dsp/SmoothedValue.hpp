#pragma once
#include <cmath>

namespace graindr {

// Mirrors juce::SmoothedValue<float, ValueSmoothingTypes::Linear>.
class SmoothedValueLinear {
public:
    SmoothedValueLinear() = default;
    explicit SmoothedValueLinear(float initial) : currentValue(initial), target(initial) {}

    void reset(double sampleRate, double rampLengthSeconds) {
        reset((int) std::floor(rampLengthSeconds * sampleRate));
    }
    void reset(int numSteps) { stepsToTarget = numSteps; setCurrentAndTargetValue(target); }

    void setCurrentAndTargetValue(float v) { target = currentValue = v; countdown = 0; }

    void setTargetValue(float newValue) {
        if (newValue == target) return;
        if (stepsToTarget <= 0) { setCurrentAndTargetValue(newValue); return; }
        target = newValue;
        countdown = stepsToTarget;
        step = (target - currentValue) / (float) countdown;
    }

    bool  isSmoothing()    const { return countdown > 0; }
    float getCurrentValue() const { return currentValue; }
    float getTargetValue()  const { return target; }

    float getNextValue() {
        if (!isSmoothing()) return target;
        --countdown;
        if (isSmoothing()) currentValue += step;
        else               currentValue = target;
        return currentValue;
    }
private:
    float currentValue = 0.0f, target = 0.0f, step = 0.0f;
    int   countdown = 0, stepsToTarget = 0;
};

// Mirrors juce::SmoothedValue<float, ValueSmoothingTypes::Multiplicative>.
// Precondition: values are never 0 (matches JUCE's assertion).
class SmoothedValueMultiplicative {
public:
    SmoothedValueMultiplicative() = default;
    explicit SmoothedValueMultiplicative(float initial) : currentValue(initial), target(initial) {}

    void reset(double sampleRate, double rampLengthSeconds) {
        reset((int) std::floor(rampLengthSeconds * sampleRate));
    }
    void reset(int numSteps) { stepsToTarget = numSteps; setCurrentAndTargetValue(target); }

    void setCurrentAndTargetValue(float v) { target = currentValue = v; countdown = 0; }

    void setTargetValue(float newValue) {
        if (newValue == target) return;
        if (stepsToTarget <= 0) { setCurrentAndTargetValue(newValue); return; }
        target = newValue;
        countdown = stepsToTarget;
        step = std::exp((std::log(std::abs(target)) - std::log(std::abs(currentValue)))
                        / (float) countdown);
    }

    bool  isSmoothing()    const { return countdown > 0; }
    float getCurrentValue() const { return currentValue; }
    float getTargetValue()  const { return target; }

    float getNextValue() {
        if (!isSmoothing()) return target;
        --countdown;
        if (isSmoothing()) currentValue *= step;
        else               currentValue = target;
        return currentValue;
    }
private:
    float currentValue = 1.0f, target = 1.0f, step = 1.0f;
    int   countdown = 0, stepsToTarget = 0;
};

} // namespace graindr
