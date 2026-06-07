#include "CircularBuffer.hpp"

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
    CircularBuffer cb;
    cb.createCircularBuffer(16); // rounds to 16
    for (int i = 1; i <= 5; ++i) cb.writeBuffer(float(i)); // wrote 1,2,3,4,5
    EXPECT_NEAR(cb.readBuffer(1), 5.0f, 1e-6);
    EXPECT_NEAR(cb.readBuffer(2), 4.0f, 1e-6);
    EXPECT_NEAR(cb.readBuffer(5), 1.0f, 1e-6);
    // linear fractional read halfway between delay 1 (=5) and delay 2 (=4) -> 4.5
    EXPECT_NEAR(cb.readBuffer(1.5f, true), 4.5f, 1e-5);
    std::puts("OK test_circular");
    return 0;
}
