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
    // [7/6] Padé approximant of tanh (juce::dsp::FastMathApproximations::tanh).
    // Matches std::tanh to < ~1.1e-4. The rational form creeps above 1 near
    // x~4.97 then falls back toward 0 for large x, so clamp to ±1 at 4.9 (just
    // below the overshoot, Padé(4.9)=0.99998): keeps the softClipper bounded,
    // monotonic and saturating when driven hard inside the feedback loop,
    // instead of blowing up or sagging. No NaN/Inf.
    static float tanh(float x) noexcept {
        if (x >=  4.9f) return  1.0f;
        if (x <= -4.9f) return -1.0f;
        auto x2 = x * x;
        auto numerator   = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        auto denominator = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + 28.0f * x2));
        return numerator / denominator;
    }
};
} // namespace graindr
