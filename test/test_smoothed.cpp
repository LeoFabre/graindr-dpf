#include "SmoothedValue.hpp"
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
    // Linear: reaches target in exactly N steps, linear ramp.
    {
        SmoothedValueLinear v(0.0f);
        v.reset(1000.0, 0.01); // 10 steps
        v.setTargetValue(1.0f);
        for (int i = 1; i <= 10; ++i) {
            float cur = v.getNextValue();
            EXPECT_NEAR(cur, float(i) / 10.0f, 1e-5); // 0.1,0.2,...,1.0
        }
        EXPECT_NEAR(v.getNextValue(), 1.0f, 1e-9); // holds target
    }
    // Multiplicative: geometric ramp, reaches target in N steps.
    {
        SmoothedValueMultiplicative v(1.0f);
        v.reset(1000.0, 0.01); // 10 steps
        v.setTargetValue(8.0f);
        float cur = 1.0f;
        for (int i = 0; i < 10; ++i) cur = v.getNextValue();
        EXPECT_NEAR(cur, 8.0f, 1e-4);
        SmoothedValueMultiplicative w(1.0f);
        w.reset(1000.0, 0.01);
        w.setTargetValue(8.0f);
        float a = w.getNextValue(), b = w.getNextValue();
        EXPECT_NEAR(b / a, std::pow(8.0f, 0.1f), 1e-4);
    }
    // No smoothing when rampLength 0: jumps immediately.
    {
        SmoothedValueLinear v(0.0f);
        v.reset(1000.0, 0.0);
        v.setTargetValue(2.0f);
        EXPECT_NEAR(v.getNextValue(), 2.0f, 1e-9);
    }
    std::puts("OK test_smoothed");
    return 0;
}
