#include "FastMath.hpp"
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
    for (int i = -30; i <= 30; ++i) {
        float x = float(i) / 10.0f; // -3.0 .. 3.0  (within [-pi,pi])
        EXPECT_NEAR(FastMath::sin(x), std::sin(x), 1e-4);
    }
    for (int i = -15; i <= 15; ++i) {
        float x = float(i) / 10.0f; // -1.5 .. 1.5  (within (-pi/2, pi/2))
        EXPECT_NEAR(FastMath::tan(x), std::tan(x), 1e-3);
    }
    // tanh softClipper: accurate vs libm, and beyond the clamp it must saturate
    // to ±1 (never diverge/sag), staying monotonic and bounded — the loop relies
    // on it. Worst case is ~1.1e-4 near the |x|=4.9 clamp.
    float prev = FastMath::tanh(-50.0f);
    for (int i = -2000; i <= 2000; ++i) {
        float x = float(i) / 100.0f; // -20 .. 20, well past the clamp
        float v = FastMath::tanh(x);
        EXPECT_NEAR(v, std::tanh(x), 2e-4);
        if (v < prev - 1e-6f) { std::fprintf(stderr, "FAIL tanh not monotonic at x=%.3f\n", x); std::exit(1); }
        if (v > 1.0f || v < -1.0f) { std::fprintf(stderr, "FAIL tanh unbounded at x=%.3f -> %.6f\n", x, v); std::exit(1); }
        prev = v;
    }
    EXPECT_NEAR(FastMath::tanh(100.0f),  1.0f, 0.0);   // hard saturation
    EXPECT_NEAR(FastMath::tanh(-100.0f), -1.0f, 0.0);
    std::puts("OK test_fastmath");
    return 0;
}
