#pragma once
namespace graindr {
struct FastMath {
    // juce::dsp::FastMathApproximations::sin — valid for x in [-pi, pi].
    static float sin(float x) noexcept {
        auto x2 = x * x;
        auto numerator   = -x * (-11511339840.0f + x2 * (1640635920.0f + x2 * (-52785432.0f + x2 * 479249.0f)));
        auto denominator =  11511339840.0f + x2 * (277920720.0f + x2 * (3177720.0f + x2 * 18361.0f));
        return numerator / denominator;
    }
    // juce::dsp::FastMathApproximations::tan — valid for x in (-pi/2, pi/2).
    static float tan(float x) noexcept {
        auto x2 = x * x;
        auto numerator   = x * (-135135.0f + x2 * (17325.0f + x2 * (-378.0f + x2)));
        auto denominator = -135135.0f + x2 * (62370.0f + x2 * (-3150.0f + 28.0f * x2));
        return numerator / denominator;
    }
};
} // namespace graindr
