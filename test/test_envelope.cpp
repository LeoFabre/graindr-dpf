#include "EnvelopeDetector.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

#define EXPECT_NEAR(a, b, eps) do {                                          \
    const double _a = double(a), _b = double(b), _e = double(eps);           \
    if (std::abs(_a - _b) > _e) {                                            \
        std::fprintf(stderr, "FAIL %s:%d: %.6f != %.6f (eps=%.6g)\n",        \
            __FILE__, __LINE__, _a, _b, _e);                                 \
        std::exit(1);                                                        \
    }                                                                        \
} while (0)

int main() {
    using namespace graindr;
    const float fs = 48000.f;
    EnvelopeDetector env; env.reset(fs);
    env.setParams(DetectionMode::PEAK, 1.5f, 50.f, false);
    float v = 0.f;
    for (int n = 0; n < int(0.02f * fs); ++n) v = env.processSample(1.0f).envelope; // 20 ms attack
    if (!(v > 0.5f)) { std::fprintf(stderr, "FAIL attack rise %.4f\n", v); return 1; }
    float afterAttack = v;
    for (int n = 0; n < int(0.2f * fs); ++n) v = env.processSample(0.0f).envelope; // 200 ms release
    if (!(v < afterAttack * 0.5f)) { std::fprintf(stderr, "FAIL release decay %.4f\n", v); return 1; }
    // RMS mode: constant 0.5 input -> envelope approaches 0.5.
    EnvelopeDetector e2; e2.reset(fs);
    e2.setParams(DetectionMode::RMS, 1.f, 1.f, false);
    float r = 0.f;
    for (int n = 0; n < int(0.5f * fs); ++n) r = e2.processSample(0.5f).envelope;
    EXPECT_NEAR(r, 0.5f, 0.02f);
    std::puts("OK test_envelope");
    return 0;
}
