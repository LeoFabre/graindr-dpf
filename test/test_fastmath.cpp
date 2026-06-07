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
    std::puts("OK test_fastmath");
    return 0;
}
