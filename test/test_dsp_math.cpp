#include "DspMath.hpp"
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
    EXPECT_NEAR(dbToGain(0.0f), 1.0f, 1e-6);
    EXPECT_NEAR(dbToGain(-6.0206f), 0.5f, 1e-3);
    EXPECT_NEAR(gainToDb(1.0f), 0.0f, 1e-4);
    EXPECT_NEAR(gainToDb(0.5f), -6.0206f, 1e-3);
    EXPECT_NEAR(clampf(5.0f, 0.0f, 1.0f), 1.0f, 1e-9);
    EXPECT_NEAR(clampf(-5.0f, 0.0f, 1.0f), 0.0f, 1e-9);
    EXPECT_NEAR(degreesToRadians(180.0f), 3.14159265f, 1e-5);
    if (nextPowerOfTwo(1000u) != 1024u) { std::fprintf(stderr,"FAIL npot\n"); return 1; }
    if (nextPowerOfTwo(1024u) != 1024u) { std::fprintf(stderr,"FAIL npot2\n"); return 1; }
    EXPECT_NEAR(cubicInterpolation(0.f,2.f,4.f,6.f,0.0f), 2.0f, 1e-5);
    EXPECT_NEAR(cubicInterpolation(0.f,2.f,4.f,6.f,1.0f), 4.0f, 1e-5);
    std::puts("OK test_dsp_math");
    return 0;
}
