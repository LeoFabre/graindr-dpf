#include "DryWetMixer.hpp"
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
    DryWetMixer m; m.prepare(48000.f);
    m.setWetProportion(1.0f); // fully wet
    for (int i=0;i<10000;++i){ (void)m.mix(0.3f, 0.9f);} // settle
    EXPECT_NEAR(m.mix(0.3f, 0.9f), 0.9f, 1e-3);
    m.setWetProportion(0.0f); // fully dry
    for (int i=0;i<10000;++i){ (void)m.mix(0.3f, 0.9f);}
    EXPECT_NEAR(m.mix(0.3f, 0.9f), 0.3f, 1e-3);
    m.setWetProportion(0.5f); // equal power: each gain = sin(pi/4)=0.7071
    for (int i=0;i<10000;++i){ (void)m.mix(1.0f,1.0f);}
    EXPECT_NEAR(m.mix(1.0f, 0.0f), 0.7071f, 1e-3); // dry only -> dryGain
    std::puts("OK test_drywet");
    return 0;
}
