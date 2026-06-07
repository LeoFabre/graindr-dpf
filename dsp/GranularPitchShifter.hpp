#pragma once

#include <cmath>
#include "DspMath.hpp"
#include "CircularBuffer.hpp"
#include "SmoothedValue.hpp"
#include "VASVFilter.hpp"

namespace graindr {

constexpr int MAX_GRAIN_SIZE_SEC = 1;

constexpr int MIN_PITCH_SHIFT = -12;
constexpr int MAX_PITCH_SHIFT = 12;

constexpr float MIN_PITCH_SHIFT_FACTOR = 0.5f;
constexpr float MAX_PITCH_SHIFT_FACTOR = 2.0f;

constexpr float MIN_LFO_FREQ = 0.02f;
constexpr float MAX_LFO_FREQ = 10.0f;

constexpr float MAX_MOD_DEPTH_SECS = 0.03f;

constexpr float SMOOTHED_VAL_RAMP_LEN_SEC = 0.1f;

inline float pitchShift2Factor(float shift) { return std::pow(2.0f, shift / 12.0f); }

enum PlaybackDirection
{
    FORWARD,
    REVERSE,
    ALTERNATE
};

enum ToneType
{
    ANALOG,
    DIGITAL
};

class GranularPitchShifter
{
public:
    GranularPitchShifter() {}

    bool isStreching() { return stretch > 1; }

    void reset(float sampleRate)
    {
        fs = sampleRate;

        int maxOutputBufferSize = ((int) fs) * MAX_GRAIN_SIZE_SEC;
        int maxInputBufferSize = ((int) MAX_PITCH_SHIFT_FACTOR) * maxOutputBufferSize;
        inputBuffer.createCircularBuffer(maxInputBufferSize * 2);
        outputBuffer.createCircularBuffer(maxOutputBufferSize * 4);
        modulationBuffer.createCircularBuffer((unsigned int)(((int) fs) * MAX_MOD_DEPTH_SECS * 2));
        postDelayBuffer.createCircularBuffer(maxOutputBufferSize);

        writtenSmplsCtr = 0;
        stretchCtr = 0;

        output1ReadIdx = 0;
        output1ReadOffset = 0;
        output1StretchMultiplier = 1;
        output1TriggerStretchOnNext = false;

        output2ReadIdx = 0;
        output2ReadOffset = 0;
        output2StretchMultiplier = 1;
        output2TriggerStretchOnNext = false;

        grainSize_smpls.reset(fs, 1.0f);
        pitchShiftFactor.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        fineTuneFactor.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        texture.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        nextStretch = stretch;
        feedback.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        shimmer.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        psBandpass.reset(fs);
        psBandpass.setParameters(725.0f, 0.33f, false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);

        fdbkHpf.reset(fs);
        fdbkHpf.setParameters(100.0f, 0.707f, false, false, 0.0f, 0.0f, 1.0f, 0.0f, false);

        postDelayBandpass.reset(fs);
        postDelayBandpass.setParameters(725.0f, 0.33f, false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);

        postDelayHpf.reset(fs);
        postDelayHpf.setParameters(100.0f, 0.707f, false, false, 0.0f, 0.0f, 1.0f, 0.0f, false);

        fdbkLpf.reset(fs);

//        nDelayApf.reset(fs, 149.0f, 239.0f);
//        nDelayApf.setParams(0.9f, 0.5f);
    }

    void setParameters(float grainSize_ms, float _pitchShift, float _fineTune, float _texture, float _strech, float _feedback, float _shimmer, float shimmerHiCut_Hz, PlaybackDirection _playbackDir, ToneType _toneType)
    {
        grainSize_smpls.setTargetValue(grainSize_ms * 0.001f * fs);

        if (_pitchShift != pitchShift)
        {
            pitchShift = _pitchShift;
            pitchShiftFactor.setTargetValue(pitchShift2Factor(pitchShift));
        }
        if (_fineTune != fineTune)
        {
            fineTune = _fineTune;
            fineTuneFactor.setTargetValue(pitchShift2Factor(fineTune * 0.01f));
        }

        texture.setTargetValue(_texture);
        nextStretch = (int) _strech;
        feedback.setTargetValue(jlimit(0.0f, 1.0f, _feedback * 0.01f));
        shimmer.setTargetValue(jlimit(0.0f, 1.0f, _shimmer * 0.01f));
        fdbkLpf.setParameters(shimmerHiCut_Hz, 0.707f, 0.0f, 0.0f, 0.0f, 1.0f);
        playbackDir = _playbackDir;
        toneType = _toneType;
    }

    void processBlock(float* buffer, const int numSamples);

    float processSample(float x, float modulation = 0.0f, bool triggerStrech = false);

    float processOutputBuffers(float modulation_smpls, bool triggerStrech);
    float processPostDelay(float x, float modulation_smpls);
    void writeInputBuffer(float x, float feedbackIn);

private:
    static constexpr float ANALOG_POST_DEL_LOOP_GAIN = 3.98f;
    float fs = 44100.0f;

    SmoothedValueMultiplicative grainSize_smpls{2205.0f};

    float pitchShift = 0;
    SmoothedValueMultiplicative pitchShiftFactor{1.0f};
    float fineTune = 0;
    SmoothedValueMultiplicative fineTuneFactor{1.0f};
    SmoothedValueLinear texture{0.5f};

    int stretch = 1;
    int nextStretch = 1;

    SmoothedValueLinear feedback{0.0f};
    SmoothedValueLinear shimmer{0.0f};
    PlaybackDirection playbackDir = PlaybackDirection::FORWARD;
    ToneType toneType = ToneType::DIGITAL;

    int writtenSmplsCtr = 0;
    int stretchCtr = 0;

    int output1ReadIdx = 0;
    int output1ReadOffset = 0;
    int output1StretchMultiplier = 1;
    bool output1TriggerStretchOnNext = false;

    int output2ReadIdx = 0;
    int output2ReadOffset = 0;
    int output2StretchMultiplier = 1;
    bool output2TriggerStretchOnNext = false;

    float currentInputSize = 0.0f;
    int currentOutputSize = 0;

    CircularBuffer inputBuffer;
    CircularBuffer outputBuffer;
    CircularBuffer modulationBuffer;
    CircularBuffer postDelayBuffer;

    StaticVASVFilter psBandpass;
    StaticVASVFilter postDelayBandpass;
    StaticVASVFilter fdbkHpf;
    StaticVASVFilter postDelayHpf;

    VASVFilter fdbkLpf;

    inline float softClipper(float x)
    {
        return std::tanh(x);
    }

    inline void resample(float inputSize, int outputSize, float factor);
    inline float windowFunc(int n, int outputSize, int overlapWidth);
};

inline void GranularPitchShifter::processBlock(float *buffer, const int numSamples)
{
    for (int n = 0; n < numSamples; ++n)
    {
        buffer[n] = processSample(buffer[n]);
    }
}

inline float GranularPitchShifter::processOutputBuffers(float modulation_smpls, bool triggerStrech)
{
    auto pFactor = pitchShiftFactor.getNextValue();
    auto ftFactor = fineTuneFactor.getNextValue();
    auto totalPitchFactor = pFactor * ftFactor;

    currentOutputSize = (int) (grainSize_smpls.getNextValue());
    currentInputSize = static_cast<float>(currentOutputSize) * totalPitchFactor;

    auto txt = texture.getNextValue();
    int overlapWidth = (int) (currentOutputSize * txt * 0.5f);
    int stride = currentOutputSize - overlapWidth - 1;

    if (triggerStrech)
    {
        output1TriggerStretchOnNext = true;
        output2TriggerStretchOnNext = true;
    }

    if (writtenSmplsCtr > stride) {
        writtenSmplsCtr = 0;

        int multiplierIncr = 0;
        if (++stretchCtr >= stretch)
        {
            stretchCtr = 0;
        }
        else
        {
            multiplierIncr = 1;
        }
        stretch = nextStretch;

        resample(currentInputSize, currentOutputSize, totalPitchFactor);
        if (output1ReadIdx >= currentOutputSize)
        {
            output1ReadIdx = 0;
            output1ReadOffset = 0;
            output2ReadOffset = currentOutputSize;
            auto writeIdx = outputBuffer.getWriteIndex();
            if (output1TriggerStretchOnNext || (writeIdx > 0 && writeIdx < currentOutputSize))
            {
                output1StretchMultiplier = 1;
            }
            else
            {
                output1StretchMultiplier = output2StretchMultiplier + multiplierIncr;
            }

            output1TriggerStretchOnNext = false;
        }
        else if (output2ReadIdx >= currentOutputSize)
        {
            output2ReadIdx = 0;
            output2ReadOffset = 0;
            output1ReadOffset = currentOutputSize;
            auto writeIdx = outputBuffer.getWriteIndex();
            if (output2TriggerStretchOnNext || (writeIdx > 0 && writeIdx < currentOutputSize))
            {
                output2StretchMultiplier = 1;
            }
            else
            {
                output2StretchMultiplier = output1StretchMultiplier + multiplierIncr;
            }

            output2TriggerStretchOnNext = false;
        }
    }

    auto o1OutRev = 0.0f;
    auto o1OutFwd = 0.0f;
    if (output1ReadIdx < currentOutputSize)
    {
        auto win = windowFunc(output1ReadIdx, currentOutputSize, overlapWidth);
        auto grainStart = output1StretchMultiplier * currentOutputSize + output1ReadOffset;
        o1OutRev = outputBuffer.readBuffer(grainStart - currentOutputSize + output1ReadIdx + 1) * win;
        o1OutFwd = outputBuffer.readBuffer(grainStart - output1ReadIdx) * win;
        output1ReadIdx++;
    }
    auto o1Out = (playbackDir == PlaybackDirection::REVERSE ? o1OutRev : o1OutFwd);

    auto o2OutRev = 0.0f;
    auto o2OutFwd = 0.0f;
    if (output2ReadIdx < currentOutputSize)
    {
        auto win = windowFunc(output2ReadIdx, currentOutputSize, overlapWidth);
        auto grainStart = output2StretchMultiplier * currentOutputSize + output2ReadOffset;
        o2OutRev = outputBuffer.readBuffer(grainStart - currentOutputSize + output2ReadIdx + 1) * win;
        o2OutFwd = outputBuffer.readBuffer(grainStart - output2ReadIdx) * win;
        output2ReadIdx++;
    }
    auto o2Out = (playbackDir == PlaybackDirection::REVERSE || playbackDir == PlaybackDirection::ALTERNATE ? o2OutRev : o2OutFwd);

    auto sumOut = o1Out + o2Out;

    if (toneType == ToneType::ANALOG)
    {
        sumOut = softClipper(sumOut);
        sumOut = ANALOG_POST_DEL_LOOP_GAIN * psBandpass.processSample(sumOut);
    }
    sumOut = fdbkLpf.processSample(sumOut);

    modulationBuffer.writeBuffer(sumOut);

    return modulationBuffer.readBuffer(1.0f + modulation_smpls, true);
}

inline float GranularPitchShifter::processPostDelay(float x, float modulation_smpls)
{
    auto delayLineOut = postDelayBuffer.readBuffer(currentInputSize + modulation_smpls);
    auto fb = feedback.getNextValue();

    if (toneType == ToneType::ANALOG)
    {
        delayLineOut = softClipper(delayLineOut);
        delayLineOut = ANALOG_POST_DEL_LOOP_GAIN * postDelayBandpass.processSample(delayLineOut);
    }
    delayLineOut = postDelayHpf.processSample(delayLineOut);

    auto y = x + fb * delayLineOut;
    postDelayBuffer.writeBuffer(y);

    return y;
}

inline float GranularPitchShifter::processSample(float x, float modulation_smpls, bool triggerStrech)
{
    auto outputBuffersOut = processOutputBuffers(modulation_smpls, triggerStrech);
    auto y = processPostDelay(outputBuffersOut, modulation_smpls);

    writeInputBuffer(x, fdbkHpf.processSample(outputBuffersOut));

    return y;
}

inline void GranularPitchShifter::resample(float inputSize, int outputSize, float factor)
{
    float phasor = 0.0f;
    int currPos = 0;
    for (int i = 0; i < outputSize; i++)
    {
        float delay = inputSize - static_cast<float>(currPos);
        float y = inputBuffer.readBuffer(delay);

        if (phasor != 0.0f)
        {
            float y0 = inputBuffer.readBuffer(delay + 1.0f);
            float y1 = y;
            float y2 = inputBuffer.readBuffer(delay - 1.0f);
            float y3 = inputBuffer.readBuffer(delay - 2.0f);

            y = cubicInterpolation(y0, y1, y2, y3, phasor);
        }

        outputBuffer.writeBuffer(y);
        phasor += factor;
        while (phasor >= 1.0f)
        {
            phasor -= 1.0f;
            currPos++;
        }
    }
}

inline float GranularPitchShifter::windowFunc(int n, int outputSize, int overlapWidth)
{
    float w = 1.0f;
    if (n < overlapWidth)
    {
        w = static_cast<float>(n) / static_cast<float>(overlapWidth);
    }
    else if (n >= outputSize - overlapWidth)
    {
        w = static_cast<float>(outputSize - n - 1) / static_cast<float>(overlapWidth);
    }
    return w;
}

inline void GranularPitchShifter::writeInputBuffer(float x, float feedbackIn)
{
    inputBuffer.writeBuffer(x + shimmer.getNextValue() * feedbackIn);
    writtenSmplsCtr++;
}

} // namespace graindr
