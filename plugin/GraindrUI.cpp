#include "GraindrUI.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;

// Short labels for all params, in ParamId order (single granular line).
static const char* kLabels[graindr::kNumParameters] = {
    // 0: kParamDryWet
    "Dry/Wet",
    // 1-10: PS1 block
    "P1 Pitch", "P1 Fine", "P1 Grain", "P1 Tex",
    "P1 Str", "P1 Fbk", "P1 Shim", "P1 ShCut",
    "P1 Dir", "P1 Tone",
    // 11: kParamModFreq
    "Mod Frq",
    // 12: kParamModDepth
    "Mod Dep",
    // 13: kParamModWave
    "Mod Wav",
    // 14: kParamModStereoPhase
    "Mod Ph",
};

GraindrUI::GraindrUI() : UI(kWidth, kHeight)
{
    setSize(kWidth, kHeight);

    // Layout: 7 columns x 4 rows of knobs.
    // Row 0: global params (4) + mod params (4) = 8, but we use 7 cols so wrap.
    // Simpler approach: flat grid, 7 columns.
    // 28 params = 4 rows × 7 columns.
    const float marginX = 60.f;
    const float marginY = 80.f;
    const float colSpacing = (kWidth - 2.f * marginX) / 6.f;  // 7 cols → 6 gaps
    const float rowSpacing = (kHeight - marginY - 40.f) / 3.f; // 4 rows → 3 gaps
    const float kR = 20.f;

    for (uint32_t i = 0; i < graindr::kNumParameters; ++i) {
        const uint32_t col = i % 7;
        const uint32_t row = i / 7;
        const float x = marginX + col * colSpacing;
        const float y = marginY + row * rowSpacing;
        knobs_.push_back({x, y, kR, i});
    }

    // Seed UI values from the parameter defaults.
    for (uint32_t i = 0; i < graindr::kNumParameters; ++i) {
        values_[i] = graindr::rangeFor(i).def;
    }
}

void GraindrUI::parameterChanged(uint32_t index, float value)
{
    if (index < graindr::kNumParameters) {
        values_[index] = value;
        repaint();
    }
}

void GraindrUI::onNanoDisplay()
{
    // Background
    beginPath();
    rect(0.f, 0.f, kWidth, kHeight);
    fillColor(Color(24, 24, 30));
    fill();

    // Title
    fontSize(16.f);
    textAlign(ALIGN_LEFT | ALIGN_BASELINE);
    fillColor(Color(200, 200, 210));
    text(14.f, 24.f, "Graindr  [debug UI]", nullptr);

    // Section labels
    fontSize(11.f);
    fillColor(Color(140, 140, 160));
    text(14.f, 44.f, "Global (0)  |  PS1 (1-10)  |  Mod (11-14)", nullptr);

    // Draw all knobs
    for (const auto& k : knobs_)
        drawKnob(k);
}

void GraindrUI::drawKnob(const KnobRect& k)
{
    const graindr::ParamRange r = graindr::rangeFor(k.paramId);
    const float value = values_[k.paramId];
    const float range = r.max - r.min;
    const float n = (range > 1e-9f)
                    ? std::clamp((value - r.min) / range, 0.f, 1.f)
                    : 0.f;

    // Knob background circle
    beginPath();
    circle(k.x, k.y, k.r);
    if (r.isInt && r.max - r.min <= 1.f) {
        // Bool/toggle: green if on
        fillColor(value > 0.5f ? Color(80, 200, 100) : Color(50, 50, 60));
    } else {
        fillColor(Color(42, 42, 52));
    }
    fill();
    strokeColor(Color(130, 130, 150));
    strokeWidth(1.f);
    stroke();

    // Indicator line
    if (!(r.isInt && r.max - r.min <= 1.f)) {
        const float ang = -2.356f + n * 4.712f;  // -135° to +135°
        beginPath();
        moveTo(k.x, k.y);
        lineTo(k.x + std::cos(ang) * k.r * 0.85f,
               k.y + std::sin(ang) * k.r * 0.85f);
        strokeColor(Color(230, 230, 240));
        strokeWidth(2.f);
        stroke();
    }

    // Label (short, below knob)
    char buf[32];
    fontSize(9.f);
    textAlign(ALIGN_CENTER | ALIGN_BASELINE);
    fillColor(Color(180, 180, 200));
    const char* label = (k.paramId < graindr::kNumParameters) ? kLabels[k.paramId] : "?";
    text(k.x, k.y + k.r + 12.f, label, nullptr);

    // Value (numeric, below label)
    std::snprintf(buf, sizeof(buf), "%.2f", value);
    fontSize(8.f);
    fillColor(Color(120, 120, 140));
    text(k.x, k.y + k.r + 22.f, buf, nullptr);
}

bool GraindrUI::hitTestKnob(const KnobRect& k, float mx, float my, float fudge)
{
    const float dx = mx - k.x, dy = my - k.y;
    return (dx * dx + dy * dy) <= (k.r * k.r * fudge * fudge);
}

bool GraindrUI::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1) return false;

    if (ev.press) {
        for (std::size_t i = 0; i < knobs_.size(); ++i) {
            if (hitTestKnob(knobs_[i], ev.pos.getX(), ev.pos.getY())) {
                const graindr::ParamRange r = graindr::rangeFor(knobs_[i].paramId);
                // Bool toggle on click
                if (r.isInt && r.max - r.min <= 1.f) {
                    values_[knobs_[i].paramId] = (values_[knobs_[i].paramId] > 0.5f) ? 0.f : 1.f;
                    setParameterValue(knobs_[i].paramId, values_[knobs_[i].paramId]);
                    repaint();
                    return true;
                }
                draggingIdx_ = int(i);
                dragStartY_  = ev.pos.getY();
                dragStartV_  = values_[knobs_[i].paramId];
                return true;
            }
        }
        return false;
    }

    // Button release
    draggingIdx_ = -1;
    return true;
}

bool GraindrUI::onMotion(const MotionEvent& ev)
{
    if (draggingIdx_ < 0) return false;

    auto& k = knobs_[draggingIdx_];
    const graindr::ParamRange r = graindr::rangeFor(k.paramId);
    const float range = r.max - r.min;
    const float dy    = dragStartY_ - ev.pos.getY();
    float v = dragStartV_ + (dy / 200.f) * range;
    v = std::clamp(v, r.min, r.max);
    values_[k.paramId] = v;
    setParameterValue(k.paramId, v);
    repaint();
    return true;
}

UI* createUI() { return new GraindrUI(); }

END_NAMESPACE_DISTRHO
