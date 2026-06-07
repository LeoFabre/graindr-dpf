#pragma once
#include "DistrhoUI.hpp"
#include "ParameterMetadata.hpp"
#include <vector>

START_NAMESPACE_DISTRHO

class GraindrUI : public UI
{
public:
    GraindrUI();

protected:
    void onNanoDisplay() override;
    void parameterChanged(uint32_t index, float value) override;
    bool onMouse(const MouseEvent& ev) override;
    bool onMotion(const MotionEvent& ev) override;

private:
    struct KnobRect { float x, y, r; uint32_t paramId; };

    void drawKnob(const KnobRect& k);
    bool hitTestKnob(const KnobRect& k, float mx, float my, float fudge = 1.2f);

    static constexpr float kWidth  = 840.f;
    static constexpr float kHeight = 520.f;

    float values_[graindr::kNumParameters] {};
    std::vector<KnobRect> knobs_;
    int   draggingIdx_ = -1;
    float dragStartY_  = 0.f;
    float dragStartV_  = 0.f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraindrUI)
};

END_NAMESPACE_DISTRHO
