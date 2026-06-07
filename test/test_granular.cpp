#include "GranularPitchShifter.hpp"
#include "FastMathLFO.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

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
    EXPECT_NEAR(pitchShift2Factor(12.0f), 2.0f, 1e-5);
    EXPECT_NEAR(pitchShift2Factor(-12.0f), 0.5f, 1e-5);
    EXPECT_NEAR(pitchShift2Factor(0.0f), 1.0f, 1e-6);

    const float fs = 48000.f;
    // (a) unity-ish pass: finite, eventually non-silent.
    {
        GranularPitchShifter ps; ps.reset(fs);
        ps.setParameters(50.f, 0.f, 0.f, 0.5f, 1.f, 0.f, 0.f, 22000.f, FORWARD, DIGITAL);
        std::vector<float> buf((size_t)fs); // 1 s
        for (int n=0;n<(int)buf.size();++n) buf[n] = std::sin(2*kPi*220.f*n/fs);
        ps.processBlock(buf.data(), (int)buf.size());
        float energy=0.f; bool finite=true;
        for (float v : buf){ if(!std::isfinite(v)) finite=false; energy+=v*v; }
        if(!finite){ std::fprintf(stderr,"FAIL non-finite unity\n"); return 1; }
        if(!(energy > 1.0f)){ std::fprintf(stderr,"FAIL silent output %.4f\n", energy); return 1; }
    }
    // (b) stress: no NaN/Inf with feedback+shimmer+ANALOG.
    {
        GranularPitchShifter ps; ps.reset(fs);
        ps.setParameters(80.f, -12.f, 7.f, 0.7f, 2.f, 80.f, 50.f, 8000.f, ALTERNATE, ANALOG);
        Lcg rng; std::vector<float> buf((size_t)fs);
        for (int n=0;n<(int)buf.size();++n) buf[n] = rng.nextFloat()*2.f-1.f;
        ps.processBlock(buf.data(), (int)buf.size());
        for (float v : buf) if(!std::isfinite(v)){ std::fprintf(stderr,"FAIL non-finite stress\n"); return 1; }
    }
    std::puts("OK test_granular");
    return 0;
}
