#include "PitchShifterContainer.hpp"
#include "DryWetMixer.hpp"
#include <cmath>
#include <vector>
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
    const float fs = 48000.f; const int N = 4096;
    PitchShifterContainer c[2]; DryWetMixer m[2];
    for (int ch=0;ch<2;++ch){
        c[ch].reset(fs); m[ch].prepare(fs); m[ch].setWetProportion(0.5f);
        c[ch].setPs1Parameters(50.f,-12.f,0.f,0.5f,1.f,30.f,0.f,22000.f,FORWARD,DIGITAL);
        c[ch].setModParams(0.02f,0.f,FastMathLFO::SIN,0.f);
    }
    std::vector<float> inb((size_t)N), wb((size_t)N);
    for (int n=0;n<N;++n) inb[n]=std::sin(2*kPi*440.f*n/fs);
    double energy=0;
    for (int ch=0;ch<2;++ch){
        for (int n=0;n<N;++n) wb[n]=inb[n];
        c[ch].processBlock(wb.data(), N);
        for (int n=0;n<N;++n){ float y=m[ch].mix(inb[n], wb[n]); if(!std::isfinite(y)){std::fprintf(stderr,"FAIL nan\n");return 1;} energy+=double(y)*y; }
    }
    if(!(energy>1.0)){ std::fprintf(stderr,"FAIL silent %.4f\n",energy); return 1; }
    std::puts("OK test_plugin_offline");
    return 0;
}
