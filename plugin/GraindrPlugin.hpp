#pragma once
#include "DistrhoPlugin.hpp"
#include "ParameterMetadata.hpp"
#include "PitchShifterContainer.hpp"
#include "DryWetMixer.hpp"

START_NAMESPACE_DISTRHO

class GraindrPlugin : public Plugin {
public:
    GraindrPlugin();

    const char* getLabel()       const override { return "Graindr"; }
    const char* getDescription() const override { return "Dual granular pitch-shifter / time-stretch / delay"; }
    const char* getMaker()       const override { return "Nexus"; }
    const char* getHomePage()    const override { return "https://github.com/lfabre/graindr-dpf"; }
    const char* getLicense()     const override { return "GPL-3.0-or-later"; }
    uint32_t    getVersion()     const override { return d_version(0, 1, 0); }
    int64_t     getUniqueId()    const override { return d_cconst('G','r','n','d'); }

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:
    graindr::PitchShifterContainer container_[2];
    graindr::DryWetMixer dwMixer_[2];
    float paramValues_[graindr::kNumParameters] {};
    bool  prepared_ = false;
    void pushParamsToDsp();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraindrPlugin)
};

END_NAMESPACE_DISTRHO
