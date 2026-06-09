#include "GraindrPlugin.hpp"
#include "DenormalGuard.hpp"
#include <cstdio>
#include <cstring>

START_NAMESPACE_DISTRHO
using namespace graindr;

static const char* const kWaveLabels[6] = {"SIN","TRI","SAW","RMP","SQR","S&H"};
static const char* const kDirLabels[3]  = {"FORWARD","REVERSE","ALTERNATE"};
static const char* const kToneLabels[2] = {"ANALOG","DIGITAL"};

GraindrPlugin::GraindrPlugin() : Plugin(kNumParameters, 0, 0) {
    Parameter tmp;
    for (uint32_t i = 0; i < kNumParameters; ++i) { initParameter(i, tmp); paramValues_[i] = tmp.ranges.def; }
}

void GraindrPlugin::initParameter(uint32_t index, Parameter& p) {
    const ParamRange r = rangeFor(index);
    p.hints = kParameterIsAutomatable;
    if (r.isInt) p.hints |= kParameterIsInteger;
    if (r.isLog) p.hints |= kParameterIsLogarithmic;
    p.ranges.min = r.min; p.ranges.max = r.max; p.ranges.def = r.def;

    char name[64], sym[32];
    auto setNS = [&](const char* n, const char* s, const char* unit) {
        p.name = n; p.symbol = s; p.unit = unit;
    };
    switch (index) {
        case kParamDryWet:        setNS("Dry/Wet","dryWet",""); return;
        case kParamPs1InBalance:  setNS("PS 1 In","ps1InBalance",""); return;
        case kParamPs2InBalance:  setNS("PS 2 In","ps2InBalance",""); return;
        case kParamPsBalance:     setNS("PS Balance","psBalance",""); return;
        case kParamModFreq:       setNS("Mod Freq","psModFreq","Hz"); return;
        case kParamModDepth:      setNS("Mod Depth","psModDepth","%"); return;
        case kParamModStereoPhase:setNS("Mod Stereo Phase","psModStereoPhase","Deg"); return;
        case kParamModWave: {
            setNS("Mod Wave","psModWave","");
            p.enumValues.count = 6; p.enumValues.restrictedMode = true;
            p.enumValues.values = new ParameterEnumerationValue[6];
            for (int i=0;i<6;++i){ p.enumValues.values[i].value=(float)i; p.enumValues.values[i].label=kWaveLabels[i]; }
            return;
        }
        default: break;
    }
    for (uint32_t psn = 0; psn < 2; ++psn) {
        const uint32_t base = (psn==0)? (uint32_t)kParamPs1Base : (uint32_t)kParamPs2Base;
        if (index >= base && index < base + 10) {
            const uint32_t f = index - base;
            const int n = (int)psn + 1;
            auto mk = [&](const char* label, const char* symbase, const char* unit){
                std::snprintf(name,sizeof name,"PS%d %s", n, label);
                std::snprintf(sym, sizeof sym, "ps%d%s", n, symbase);
                p.name = name; p.symbol = sym; p.unit = unit;
            };
            switch (f) {
                case kPsPitchShift:   mk("Pitch Shift","PitchShift",""); return;
                case kPsFineTune:     mk("Fine Tune","FineTune",""); return;
                case kPsGrainSize:    mk("Grain Size","GrainSize","ms"); return;
                case kPsTexture:      mk("Texture","Texture",""); return;
                case kPsStretch:      mk("Stretch","Stretch",""); return;
                case kPsFeedback:     mk("Feedback","Feedback","%"); return;
                case kPsShimmer:      mk("Shimmer","Shimmer","%"); return;
                case kPsShimmerHiCut: mk("Hi Cut","ShimmerHiCut","Hz"); return;
                case kPsPlaybackDir: {
                    mk("Playback","PlaybackDir","");
                    p.enumValues.count=3; p.enumValues.restrictedMode=true;
                    p.enumValues.values=new ParameterEnumerationValue[3];
                    for(int i=0;i<3;++i){ p.enumValues.values[i].value=(float)i; p.enumValues.values[i].label=kDirLabels[i]; }
                    return;
                }
                case kPsToneType: {
                    mk("Tone","ToneType","");
                    p.enumValues.count=2; p.enumValues.restrictedMode=true;
                    p.enumValues.values=new ParameterEnumerationValue[2];
                    for(int i=0;i<2;++i){ p.enumValues.values[i].value=(float)i; p.enumValues.values[i].label=kToneLabels[i]; }
                    return;
                }
            }
        }
    }
}

float GraindrPlugin::getParameterValue(uint32_t index) const {
    return index < kNumParameters ? paramValues_[index] : 0.f;
}
void GraindrPlugin::setParameterValue(uint32_t index, float value) {
    if (index >= kNumParameters) return;
    paramValues_[index] = value;
    if (prepared_) pushParamsToDsp();
}

void GraindrPlugin::activate() {
    const float fs = float(getSampleRate());
    for (int ch=0; ch<2; ++ch) { container_[ch].reset(fs); dwMixer_[ch].prepare(fs); }
    prepared_ = true;
    pushParamsToDsp();
}

void GraindrPlugin::pushParamsToDsp() {
    for (int ch=0; ch<2; ++ch) {
        dwMixer_[ch].setWetProportion(paramValues_[kParamDryWet]);
        container_[ch].setRouting(paramValues_[kParamPs1InBalance],
                                  paramValues_[kParamPs2InBalance],
                                  paramValues_[kParamPsBalance]);
        auto setPs = [&](uint32_t base, auto setter){
            setter(paramValues_[base+kPsGrainSize], paramValues_[base+kPsPitchShift],
                   paramValues_[base+kPsFineTune],  paramValues_[base+kPsTexture],
                   paramValues_[base+kPsStretch],   paramValues_[base+kPsFeedback],
                   paramValues_[base+kPsShimmer],   paramValues_[base+kPsShimmerHiCut],
                   (PlaybackDirection)(int)paramValues_[base+kPsPlaybackDir],
                   (ToneType)(int)paramValues_[base+kPsToneType]);
        };
        setPs(kParamPs1Base, [&](float a,float b,float c,float d,float e,float f,float g,float h,PlaybackDirection i,ToneType j){
            container_[ch].setPs1Parameters(a,b,c,d,e,f,g,h,i,j); });
        setPs(kParamPs2Base, [&](float a,float b,float c,float d,float e,float f,float g,float h,PlaybackDirection i,ToneType j){
            container_[ch].setPs2Parameters(a,b,c,d,e,f,g,h,i,j); });
        const float phase = (ch == 0) ? 0.0f : paramValues_[kParamModStereoPhase];
        container_[ch].setModParams(paramValues_[kParamModFreq], paramValues_[kParamModDepth],
                                    (FastMathLFO::LFOWave)(int)paramValues_[kParamModWave], phase);
    }
}

void GraindrPlugin::run(const float** inputs, float** outputs, uint32_t frames) {
    ftz::armOnce();
    for (int ch=0; ch<2; ++ch) {
        const float* __restrict in = inputs[ch];
        float* __restrict out = outputs[ch];
        for (uint32_t n=0; n<frames; ++n) out[n] = in[n];   // copy dry into work buffer
        container_[ch].processBlock(out, (int)frames);       // wet in place
        for (uint32_t n=0; n<frames; ++n) out[n] = dwMixer_[ch].mix(in[n], out[n]);
    }
}

Plugin* createPlugin() { return new GraindrPlugin(); }

END_NAMESPACE_DISTRHO
