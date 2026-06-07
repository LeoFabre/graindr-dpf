#pragma once
#include <cstdint>
#include <initializer_list>

namespace graindr {

enum ParamId : uint32_t {
    kParamDryWet = 0,
    kParamPs1InBalance,
    kParamPs2InBalance,
    kParamPsBalance,
    kParamPs1Base,                       // PS1 block: 10 params
    kParamPs2Base = kParamPs1Base + 10,  // PS2 block: 10 params
    kParamModFreq = kParamPs2Base + 10,
    kParamModDepth,
    kParamModWave,
    kParamModStereoPhase,
    kNumParameters
};

enum PsFieldOffset : uint32_t {
    kPsPitchShift = 0,
    kPsFineTune,
    kPsGrainSize,
    kPsTexture,
    kPsStretch,
    kPsFeedback,
    kPsShimmer,
    kPsShimmerHiCut,
    kPsPlaybackDir,
    kPsToneType,
};

struct ParamRange { float min, max, def; bool isInt; bool isLog; int scalePoints; };

inline ParamRange rangeFor(uint32_t id) {
    switch (id) {
        case kParamDryWet:        return {0.f, 1.f, 0.5f, false, false, 0};
        case kParamPs1InBalance:  return {0.f, 1.f, 0.0f, false, false, 0};
        case kParamPs2InBalance:  return {0.f, 1.f, 0.0f, false, false, 0};
        case kParamPsBalance:     return {0.f, 1.f, 0.5f, false, false, 0};
        case kParamModFreq:       return {0.02f, 10.f, 0.02f, false, false, 0};
        case kParamModDepth:      return {0.f, 100.f, 0.f, false, false, 0};
        case kParamModWave:       return {0.f, 5.f, 0.f, true, false, 6};
        case kParamModStereoPhase:return {0.f, 180.f, 0.f, false, false, 0};
        default: break;
    }
    for (uint32_t base : { (uint32_t)kParamPs1Base, (uint32_t)kParamPs2Base }) {
        if (id >= base && id < base + 10) {
            switch (id - base) {
                case kPsPitchShift:   return {-12.f, 12.f, 0.f, true, false, 0};
                case kPsFineTune:     return {-100.f, 100.f, 0.f, false, false, 0};
                case kPsGrainSize:    return {1.f, 1000.f, 50.f, false, false, 0};
                case kPsTexture:      return {0.f, 1.f, 0.5f, false, false, 0};
                case kPsStretch:      return {1.f, 4.f, 1.f, true, false, 0};
                case kPsFeedback:     return {0.f, 100.f, 0.f, false, false, 0};
                case kPsShimmer:      return {0.f, 100.f, 0.f, false, false, 0};
                case kPsShimmerHiCut: return {20.f, 22000.f, 22000.f, false, true, 0};
                case kPsPlaybackDir:  return {0.f, 2.f, 0.f, true, false, 3};
                case kPsToneType:     return {0.f, 1.f, 1.f, true, false, 2};
            }
        }
    }
    return {0.f, 1.f, 0.f, false, false, 0};
}

} // namespace graindr
