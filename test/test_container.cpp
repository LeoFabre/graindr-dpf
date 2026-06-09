#include "PitchShifterContainer.hpp"
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

static void configDefault(graindr::PitchShifterContainer& c){
    using namespace graindr;
    c.setPs1Parameters(50.f,0.f,0.f,0.5f,1.f,0.f,0.f,22000.f, FORWARD, DIGITAL);
}
int main() {
    using namespace graindr;
    const float fs = 48000.f;
    // (a) finite, non-silent
    {
        PitchShifterContainer c; c.reset(fs); configDefault(c);
        c.setModParams(0.02f, 0.f, FastMathLFO::SIN, 0.f);
        std::vector<float> buf((size_t)fs);
        for (int n=0;n<(int)buf.size();++n) buf[n]=std::sin(2*kPi*330.f*n/fs);
        c.processBlock(buf.data(),(int)buf.size());
        float e=0; for(float v:buf){ if(!std::isfinite(v)){std::fprintf(stderr,"FAIL nan\n");return 1;} e+=v*v; }
        if(!(e>1.f)){ std::fprintf(stderr,"FAIL silent %.3f\n",e); return 1; }
    }
    // (b) stereo phase divergence with modulation on
    {
        PitchShifterContainer c0; c0.reset(fs); configDefault(c0);
        PitchShifterContainer c1; c1.reset(fs); configDefault(c1);
        c0.setModParams(2.0f, 100.f, FastMathLFO::SIN, 0.f);
        c1.setModParams(2.0f, 100.f, FastMathLFO::SIN, 180.f);
        std::vector<float> b0((size_t)(fs/4)), b1((size_t)(fs/4));
        for (int n=0;n<(int)b0.size();++n){ float x=std::sin(2*kPi*330.f*n/fs); b0[n]=x; b1[n]=x; }
        c0.processBlock(b0.data(),(int)b0.size());
        c1.processBlock(b1.data(),(int)b1.size());
        float diff=0; for(size_t i=0;i<b0.size();++i) diff+=std::abs(b0[i]-b1[i]);
        if(!(diff>0.0f)){ std::fprintf(stderr,"FAIL phase no divergence\n"); return 1; }
    }
    std::puts("OK test_container");
    return 0;
}
