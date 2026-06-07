#pragma once
#include "DspMath.hpp"
#include "FastMath.hpp"
#include "SmoothedValue.hpp"

namespace graindr {

inline float peakGainForQ(float q) {
    if (q <= 0.707f) return 1.0f;
    auto q2 = q * q;
    return q2 / std::pow(q2 - 0.25f, 0.5f);
}

class StaticVASVFilter {
public:
    StaticVASVFilter() = default;
    void reset(float sampleRate) { fs = sampleRate; sn_1 = 0.f; sn_2 = 0.f; }

    void setParameters(float _fc, float _q, bool _enableGainComp, bool _enableSoftClipper,
                       float _bsfMix, float _bpfMix, float _hpfMix, float _lpfMix,
                       bool _matchAnalogNyquistLPF) {
        fc=_fc; q=_q; enableGainComp=_enableGainComp; enableSoftClipper=_enableSoftClipper;
        bsfMix=_bsfMix; bpfMix=_bpfMix; hpfMix=_hpfMix; lpfMix=_lpfMix;
        matchAnalogNyquistLPF=_matchAnalogNyquistLPF;
        calcCoeffs();
    }
    float processSample(float x);

private:
    float fs=44100.f, fc=1000.f, q=0.707f;
    bool enableGainComp=false, enableSoftClipper=false;
    float halfPeak=1.f, alpha=0.f, alpha_0=0.f, r=0.707f, rho=1.414f, sigma=0.f;
    float sn_1=0.f, sn_2=0.f;
    float bsfMix=0.f, bpfMix=0.f, hpfMix=0.f, lpfMix=0.f;
    bool matchAnalogNyquistLPF=true;
    void calcCoeffs();
};

inline void StaticVASVFilter::calcCoeffs() {
    alpha = FastMath::tan((kPi * fc) / fs);
    r = 1.0f / (2.0f * q);
    rho = 2.0f * r + alpha;
    alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
    sigma = (4.0f * fc * fc) / (alpha * fs * fs);
    halfPeak = 1.0f;
    auto peak_dB = gainToDb(peakGainForQ(q));
    if (peak_dB > 0.0f) halfPeak = dbToGain(-peak_dB * 0.5f);
}

inline float StaticVASVFilter::processSample(float x) {
    if (enableGainComp) x *= halfPeak;
    auto hpf = alpha_0 * (x - rho * sn_1 - sn_2);
    auto bpf = alpha * hpf + sn_1;
    if (enableSoftClipper) bpf = std::tanh(bpf);
    auto lpf = alpha * bpf + sn_2;
    auto bsf = hpf + lpf;
    auto lpf2 = matchAnalogNyquistLPF ? lpf + sigma * sn_1 : lpf;
    sn_1 = alpha * hpf + bpf;
    sn_2 = alpha * bpf + lpf;
    return bsfMix * bsf + bpfMix * bpf + hpfMix * hpf + lpfMix * lpf2;
}

class VASVFilter {
public:
    VASVFilter() = default;
    void reset(float sampleRate) {
        fs = sampleRate;
        fc.reset(fs, 0.1); q.reset(fs, 0.1);
        bpfMix.reset(fs,0.1); bsfMix.reset(fs,0.1); hpfMix.reset(fs,0.1); lpfMix.reset(fs,0.1);
        sn_1=0.f; sn_2=0.f;
        calcCoeffs(true);
    }
    void setParameters(float _fc, float _q, float _bpfMix, float _bsfMix, float _hpfMix, float _lpfMix) {
        fc.setTargetValue(_fc); q.setTargetValue(_q);
        bpfMix.setTargetValue(_bpfMix); bsfMix.setTargetValue(_bsfMix);
        hpfMix.setTargetValue(_hpfMix); lpfMix.setTargetValue(_lpfMix);
    }
    float processSample(float x);
private:
    float fs=44100.f;
    SmoothedValueMultiplicative fc{1000.f};
    SmoothedValueLinear         q{0.707f};
    float halfPeak=1.f, alpha=0.f, alpha_0=0.f, r=0.707f, rho=1.414f, sigma=0.f;
    SmoothedValueLinear bsfMix{0.f}, bpfMix{0.f}, hpfMix{0.f}, lpfMix{0.f};
    float sn_1=0.f, sn_2=0.f;
    void calcCoeffs(bool force=false);
};

inline void VASVFilter::calcCoeffs(bool force) {
    if (!force && !fc.isSmoothing() && !q.isSmoothing()) return;
    if (force || fc.isSmoothing()) {
        auto currentFc = fc.getNextValue();
        alpha = FastMath::tan((kPi * currentFc) / fs);
        sigma = (4.0f * currentFc * currentFc) / (alpha * fs * fs);
    }
    if (force || q.isSmoothing()) {
        auto currentQ = q.getNextValue();
        auto peak_dB = gainToDb(peakGainForQ(currentQ));
        if (peak_dB > 0.0f) halfPeak = dbToGain(-peak_dB * 0.5f);
        r = 1.0f / (2.0f * currentQ);
    }
    rho = 2.0f * r + alpha;
    alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
}

inline float VASVFilter::processSample(float x) {
    calcCoeffs();
    auto hpf = alpha_0 * (x - rho * sn_1 - sn_2);
    auto bpf = alpha * hpf + sn_1;
    auto lpf = alpha * bpf + sn_2;
    auto bsf = hpf + lpf;
    auto lpf2 = lpf + sigma * sn_1;
    sn_1 = alpha * hpf + bpf;
    sn_2 = alpha * bpf + lpf;
    return bpfMix.getNextValue()*bpf + bsfMix.getNextValue()*bsf
         + hpfMix.getNextValue()*hpf + lpfMix.getNextValue()*lpf2;
}

} // namespace graindr
