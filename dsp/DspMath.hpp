#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace graindr {

inline float dbToGain(float db)        { return std::pow(10.0f, 0.05f * db); }
inline float gainToDb(float gain)      { return 20.0f * std::log10(std::max(gain, 1e-9f)); }

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float jlimit(float lo, float hi, float x) { return clampf(x, lo, hi); } // JUCE arg order

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
inline float degreesToRadians(float deg) { return deg * (kPi / 180.0f); }

inline uint32_t nextPowerOfTwo(uint32_t n) {
    uint32_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// Catmull-Rom style cubic (matches Graindr Utils.h::cubicInterpolation exactly).
inline float cubicInterpolation(float y0, float y1, float y2, float y3, float fraction) {
    float fraction2 = fraction * fraction;
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;
    return a0 * fraction * fraction2 + a1 * fraction2 + a2 * fraction + a3;
}

} // namespace graindr
