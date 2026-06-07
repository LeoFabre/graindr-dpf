#include "FastMathLFO.hpp"
#include <cmath>
#include <algorithm>
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
    auto sweep = [&](FastMathLFO::LFOWave w){
        FastMathLFO lfo; lfo.reset(fs);
        lfo.setParams(2.0f, 100.f, w, FastMathLFO::UNIPOLAR); // depth 100% -> output in [0,1]
        float mn=1e9f, mx=-1e9f;
        for (int n=0;n<int(fs);++n){ float v=lfo.getNextSample(0.0f); mn=std::min(mn,v); mx=std::max(mx,v); }
        if (mn < -0.001f || mx > 1.001f) { std::fprintf(stderr,"FAIL wave %d range [%.3f,%.3f]\n",(int)w,mn,mx); std::exit(1); }
    };
    sweep(FastMathLFO::SIN); sweep(FastMathLFO::TRI); sweep(FastMathLFO::SAW);
    sweep(FastMathLFO::RMP); sweep(FastMathLFO::SQR); sweep(FastMathLFO::RSH);
    // depth 0 -> output identically 0 (this is what the null-test relies on).
    {
        FastMathLFO lfo; lfo.reset(fs);
        lfo.setParams(2.0f, 0.f, FastMathLFO::SIN, FastMathLFO::UNIPOLAR);
        for (int n=0;n<1000;++n) EXPECT_NEAR(lfo.getNextSample(0.0f), 0.0f, 1e-7);
    }
    std::puts("OK test_lfo");
    return 0;
}
