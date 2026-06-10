#pragma once

#include <cmath>
#include <memory>
#ifndef NDEBUG
#include <cassert>
#endif
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

        grainProducing = false;
        grainReverseOrder = false;
        grainElapsed = 0;
        grainQuota = 0;
        grainFrontIdx = 0;
        grainBackIdx = -1;
        grainPhasor = 0.0f;
        grainCurrPos = 0;
        grainSeqCapacity = maxOutputBufferSize + kGrainHazard;
        grainSeqPhasor.reset(new float[grainSeqCapacity]);
        grainSeqPos.reset(new int[grainSeqCapacity]);

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

    // ---- Amortized grain production (flat WCET) -------------------------------------
    //
    // The original resample() rebuilt the whole grain (outputSize cubic reads, ~2205 at
    // defaults) inside ONE sample tick, a ~300 us burst every stride+1 samples on the
    // A53. It is replaced by incremental production: the event LATCHES everything the
    // grain depends on (sizes, factor, the input write-head position) and the grain is
    // then produced K samples per tick into the same outputBuffer slots, with identical
    // float operations in identical order, so the result is bit-identical.
    //
    // Source reads: the input head advances by 1 per tick after the event, so a read
    // that resample() would have done at integer delay d is done at d + grainElapsed —
    // the same buffer cell with unchanged content (the cell is only rewritten after
    // bufferLength = nextPow2(4*fs) further writes; max d + elapsed <= 2.12*fs + 3 + fs
    // < 4*fs, so it never is). The fractional part is computed from the SAME float
    // expression as before (never from d + elapsed, which could round differently).
    // The only cells that DO change under the head are the ones resample() read at
    // non-positive event-time delay (the "future wrap" reads of the cubic tail, integer
    // delays >= -3): those are latched into grainHazard[] at the event.
    //
    // Safety of deferred output writes (readers must never see an unproduced slot):
    // the fresh grain occupies head-relative delays [1, outputSize] (slot i at delay
    // outputSize - i; the head is advanced by outputSize at the event, exactly where
    // the original left it, so getWriteIndex() and all reader delays are unchanged).
    //  * The only output that touches that region immediately is the one reset at this
    //    event with stretch multiplier 1: it reads slot e (forward) or slot
    //    outputSize-1-e (reverse) at tick e after the event. Production runs before
    //    the readers each tick, in the matching direction, so after tick e it has
    //    produced at least K*(e+1) >= e+1 slots from that end: always ahead for K >= 1.
    //  * The non-reset output reads delays >= outputSize + 1 (its readOffset is
    //    outputSize), and a reset output with multiplier >= 2 reads delays
    //    >= outputSize + 1 too: both outside the fresh region (at event-time sizes).
    //  * K = ceil(outputSize/(stride+1)) + 1 completes production strictly before the
    //    next event (period stride+1 ticks at event-time parameters), so back-to-back
    //    grains never overlap in steady state. Defaults: ceil(2205/1654)+1 = 3.
    //  * Parameter smoothing can break the static assumptions (stride shrinking ->
    //    earlier event; outputSize drifting -> reader delays slide into the fresh
    //    region). Two fallbacks keep bit-identity unconditionally: a new event first
    //    burst-completes any in-flight grain (production time does not affect values,
    //    only the latched state does), and a per-tick guard recomputes the exact
    //    delays the SELECTED reads will use this tick and catch-up produces if one
    //    lands beyond the produced range (covers playbackDir flips mid-grain too).
    //
    // Reverse-order production needs the per-iteration (phasor, currPos) recurrence
    // state at arbitrary iterations; the recurrence is float-rounding-sensitive and only
    // steps forward, so it is precomputed into grainSeq* at the event (a few cheap ops
    // per iteration, ~50x lighter than the cubic reads it replaces).
    static constexpr int kGrainHazard = 8;

    bool grainProducing = false;
    bool grainReverseOrder = false;
    float grainInputSize = 0.0f;
    int grainOutputSize = 0;
    float grainFactor = 0.0f;
    int grainQuota = 0;            // K = per-tick production quota
    int grainElapsed = 0;          // input writes since the latch event
    unsigned int grainBase = 0;    // outputBuffer slot of grain sample 0
    int grainFrontIdx = 0;         // forward order: next slot to produce
    float grainPhasor = 0.0f;      // forward-order recurrence cursor
    int grainCurrPos = 0;
    int grainBackIdx = -1;         // reverse order: next slot to produce (descending)
    float grainHazard[kGrainHazard] = {};
    int grainSeqCapacity = 0;
    std::unique_ptr<float[]> grainSeqPhasor;
    std::unique_ptr<int[]> grainSeqPos;

    inline void startGrainProduction(float inputSize, int outputSize, float factor, int stride);
    inline void prepareReverseOrderProduction();
    inline void produceGrainStep();
    inline float produceGrainValue(float phasorVal, int currPosVal);
    inline float readGrainSource(float delay);
    inline float readGrainSourceInt(int delaySmpls);
    inline void guardFreshRead(int readIdx, int multiplier, int readOffset, bool readsReverse);

    inline float windowFunc(int n, int outputSize, int overlapWidth, float invOverlap);
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
    float invOverlap = overlapWidth != 0 ? 1.0f / static_cast<float>(overlapWidth) : 0.0f;
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

        startGrainProduction(currentInputSize, currentOutputSize, totalPitchFactor, stride);

        int resetOutput = 0;
        if (output1ReadIdx >= currentOutputSize)
        {
            resetOutput = 1;
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
            resetOutput = 2;
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

        // Produce back-to-front iff the output that will start reading the fresh grain
        // immediately (just reset, multiplier 1 -> grainStart == currentOutputSize)
        // consumes it in reverse; otherwise nothing reads the fresh region before the
        // per-tick guards below would catch it, and forward order is cheapest.
        bool freshReadReverse = false;
        if (resetOutput == 1 && output1StretchMultiplier == 1)
            freshReadReverse = (playbackDir == PlaybackDirection::REVERSE);
        else if (resetOutput == 2 && output2StretchMultiplier == 1)
            freshReadReverse = (playbackDir != PlaybackDirection::FORWARD);
        if (freshReadReverse)
            prepareReverseOrderProduction();
    }

    if (grainProducing)
    {
        for (int k = 0; k < grainQuota && grainProducing; ++k)
            produceGrainStep();

        // Guard: if a SELECTED read this tick lands in the not-yet-produced part of the
        // fresh grain (possible only under parameter drift or a playbackDir change
        // mid-production), catch up to it now. Recomputes exactly the delays used by
        // the read code below.
        if (grainProducing)
            guardFreshRead(output1ReadIdx, output1StretchMultiplier, output1ReadOffset,
                           playbackDir == PlaybackDirection::REVERSE);
        if (grainProducing)
            guardFreshRead(output2ReadIdx, output2StretchMultiplier, output2ReadOffset,
                           playbackDir != PlaybackDirection::FORWARD);
    }

    auto o1OutRev = 0.0f;
    auto o1OutFwd = 0.0f;
    if (output1ReadIdx < currentOutputSize)
    {
        auto win = windowFunc(output1ReadIdx, currentOutputSize, overlapWidth, invOverlap);
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
        auto win = windowFunc(output2ReadIdx, currentOutputSize, overlapWidth, invOverlap);
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

inline void GranularPitchShifter::startGrainProduction(float inputSize, int outputSize, float factor, int stride)
{
    // A previous grain still in flight (possible when parameter ramps shrink the stride
    // below the production window) is completed now: the produced values depend only on
    // the latched event state, never on WHEN the production steps run.
    while (grainProducing)
        produceGrainStep();

    // Advance the head as if the whole grain had been written here (the original
    // resample() did exactly that), so getWriteIndex() and every head-relative reader
    // delay are unchanged. The reserved slots are filled incrementally below/later.
    grainBase = (unsigned int) outputBuffer.getWriteIndex();
    outputBuffer.advanceWriteIndex(outputSize);

    if (outputSize <= 0)
        return;

    grainInputSize = inputSize;
    grainOutputSize = outputSize;
    grainFactor = factor;
    grainElapsed = 0;
    grainFrontIdx = 0;
    grainPhasor = 0.0f;
    grainCurrPos = 0;
    grainReverseOrder = false;
    grainProducing = true;

    // K: completes the grain strictly before the next event (period = stride+1 ticks at
    // event-time parameters), +1 margin. Defaults: ceil(2205/1654) + 1 = 3 steps/tick.
    const int period = stride + 1 > 0 ? stride + 1 : 1;
    grainQuota = (outputSize + period - 1) / period + 1;

    // Latch the input cells the grain tail reads at non-positive event-time delay (the
    // cubic neighbourhood can reach integer delay -3); post-event writes overwrite
    // those cells within a few ticks, so they must be captured now.
    for (int k = 0; k < kGrainHazard; ++k)
        grainHazard[k] = inputBuffer.readBuffer(-k);
}

inline void GranularPitchShifter::prepareReverseOrderProduction()
{
    if (!grainProducing || grainOutputSize > grainSeqCapacity)
        return; // oversized grain: stay in forward order (guards burst if ever read early)

    // The (phasor, currPos) recurrence is float-rounding-sensitive and only steps
    // forward; precompute it so slots can be produced back-to-front bit-identically.
    float ph = 0.0f;
    int cp = 0;
    for (int i = 0; i < grainOutputSize; ++i)
    {
        grainSeqPhasor[i] = ph;
        grainSeqPos[i] = cp;
        ph += grainFactor;
        while (ph >= 1.0f)
        {
            ph -= 1.0f;
            cp++;
        }
    }
    grainReverseOrder = true;
    grainBackIdx = grainOutputSize - 1;
}

inline void GranularPitchShifter::produceGrainStep()
{
    if (grainReverseOrder)
    {
        const int j = grainBackIdx;
        outputBuffer.writeAtIndex(grainBase + (unsigned int) j,
                                  produceGrainValue(grainSeqPhasor[j], grainSeqPos[j]));
        if (--grainBackIdx < 0)
            grainProducing = false;
    }
    else
    {
        outputBuffer.writeAtIndex(grainBase + (unsigned int) grainFrontIdx,
                                  produceGrainValue(grainPhasor, grainCurrPos));
        grainPhasor += grainFactor;
        while (grainPhasor >= 1.0f)
        {
            grainPhasor -= 1.0f;
            grainCurrPos++;
        }
        if (++grainFrontIdx >= grainOutputSize)
            grainProducing = false;
    }
}

// One grain sample, bit-identical to the original resample() loop body.
inline float GranularPitchShifter::produceGrainValue(float phasorVal, int currPosVal)
{
    float delay = grainInputSize - static_cast<float>(currPosVal);
    float y = readGrainSource(delay);

    if (phasorVal != 0.0f)
    {
        float y0 = readGrainSource(delay + 1.0f);
        float y1 = y;
        float y2 = readGrainSource(delay - 1.0f);
        float y3 = readGrainSource(delay - 2.0f);

        y = cubicInterpolation(y0, y1, y2, y3, phasorVal);
    }
    return y;
}

// Replicates CircularBuffer::readBuffer(float) against the EVENT-TIME buffer content.
// The fraction comes from the same float expression as the original (never recomputed
// from an elapsed-compensated value, which could round differently).
inline float GranularPitchShifter::readGrainSource(float delay)
{
    const int delayInt = (int) delay;
    const float fraction = delay - static_cast<float>(delayInt);
    if (fraction == 0.0f) // fast path: Catmull-Rom at t=0 is exactly y1
        return readGrainSourceInt(delayInt);

    float y1 = readGrainSourceInt(delayInt);
    float y2 = readGrainSourceInt(delayInt + 1);
    float y0 = readGrainSourceInt(delayInt - 1);
    float y3 = readGrainSourceInt(delayInt + 2);
    return cubicInterpolation(y0, y1, y2, y3, fraction);
}

inline float GranularPitchShifter::readGrainSourceInt(int delaySmpls)
{
    if (delaySmpls <= 0)
    {
        // Event-time "future wrap" cells: already overwritten by post-event input
        // writes, served from the latch instead.
#ifndef NDEBUG
        assert(-delaySmpls < kGrainHazard);
#endif
        return grainHazard[-delaySmpls];
    }
    // The head advanced by grainElapsed since the event; same cell, unchanged content.
    return inputBuffer.readBuffer(delaySmpls + grainElapsed);
}

inline void GranularPitchShifter::guardFreshRead(int readIdx, int multiplier, int readOffset, bool readsReverse)
{
    if (readIdx >= currentOutputSize)
        return; // this output reads nothing this tick

    const int grainStart = multiplier * currentOutputSize + readOffset;
    const int d = readsReverse ? (grainStart - currentOutputSize + readIdx + 1)
                               : (grainStart - readIdx);
    if (d < 1 || d > grainOutputSize)
        return; // the read lands outside the fresh grain region

    const int slot = grainOutputSize - d;
    if (grainReverseOrder)
        while (grainProducing && grainBackIdx >= slot)
            produceGrainStep();
    else
        while (grainProducing && grainFrontIdx <= slot)
            produceGrainStep();
}

inline float GranularPitchShifter::windowFunc(int n, int outputSize, int overlapWidth, float invOverlap)
{
    float w = 1.0f;
    if (n < overlapWidth)
    {
        w = static_cast<float>(n) * invOverlap;
    }
    else if (n >= outputSize - overlapWidth)
    {
        w = static_cast<float>(outputSize - n - 1) * invOverlap;
    }
    return w;
}

inline void GranularPitchShifter::writeInputBuffer(float x, float feedbackIn)
{
    inputBuffer.writeBuffer(x + shimmer.getNextValue() * feedbackIn);
    writtenSmplsCtr++;
    if (grainProducing)
        grainElapsed++;
}

} // namespace graindr
