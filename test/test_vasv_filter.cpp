#include "VASVFilter.hpp"
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

static float rms(float* b, int n){ double s=0; for(int i=0;i<n;i++) s+=double(b[i])*b[i]; return (float)std::sqrt(s/n); }

int main() {
    using namespace graindr;
    const float fs = 48000.f; const int N = 4096;
    // LPF at 500 Hz: passes a 50 Hz sine, attenuates a 12 kHz sine.
    auto runLPF = [&](float freqHz){
        StaticVASVFilter f; f.reset(fs);
        f.setParameters(500.f, 0.707f, false, false, 0.f, 0.f, 0.f, 1.f, false);
        std::vector<float> out(N);
        for (int n=0;n<N;n++){ float x=std::sin(2*kPi*freqHz*n/fs); out[n]=f.processSample(x); }
        return rms(out.data()+N/2, N/2); // ignore transient
    };
    float lowPass = runLPF(50.f);
    float highCut = runLPF(12000.f);
    if (!(lowPass > 0.6f))  { std::fprintf(stderr,"FAIL LPF passband %.4f\n", lowPass); return 1; }
    if (!(highCut < 0.1f))  { std::fprintf(stderr,"FAIL LPF stopband %.4f\n", highCut); return 1; }
    // HPF at 5 kHz: attenuates 50 Hz.
    {
        StaticVASVFilter f; f.reset(fs);
        f.setParameters(5000.f, 0.707f, false, false, 0.f, 0.f, 1.f, 0.f, false);
        std::vector<float> out(N);
        for (int n=0;n<N;n++){ float x=std::sin(2*kPi*50.f*n/fs); out[n]=f.processSample(x); }
        float r = rms(out.data()+N/2, N/2);
        if (!(r < 0.1f)) { std::fprintf(stderr,"FAIL HPF stopband %.4f\n", r); return 1; }
    }
    std::puts("OK test_vasv_filter");
    return 0;
}
