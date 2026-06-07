#pragma once
#include <memory>
#include <cstring>
#include "DspMath.hpp"

namespace graindr {

class CircularBuffer {
public:
    CircularBuffer() = default;

    void flushBuffer() { std::memset(&buffer[0], 0, bufferLength * sizeof(float)); }

    void createCircularBuffer(unsigned int len) {
        writeIndex = 0;
        bufferLength = nextPowerOfTwo(len);
        wrapMask = bufferLength - 1;
        buffer.reset(new float[bufferLength]);
        flushBuffer();
    }

    void writeBuffer(float input) {
        buffer[writeIndex++] = input;
        writeIndex &= wrapMask;
    }

    float readBuffer(int delayInSamples) {
        int readIndex = (int) writeIndex - delayInSamples;
        readIndex &= (int) wrapMask;
        return buffer[readIndex];
    }

    float readBuffer(float delayInFractionalSamples, bool linearInterpolation = false) {
        float fraction = delayInFractionalSamples - (int) delayInFractionalSamples;
        float y1 = readBuffer((int) delayInFractionalSamples);
        float y2 = readBuffer((int) delayInFractionalSamples + 1);
        if (linearInterpolation)
            return (1.0f - fraction) * y1 + fraction * y2;
        float y0 = readBuffer((int) delayInFractionalSamples - 1);
        float y3 = readBuffer((int) delayInFractionalSamples + 2);
        return cubicInterpolation(y0, y1, y2, y3, fraction);
    }

    int getWriteIndex() { return (int) writeIndex; }

private:
    std::unique_ptr<float[]> buffer = nullptr;
    unsigned int writeIndex = 0;
    unsigned int bufferLength = 1024;
    unsigned int wrapMask = 1023;
};

} // namespace graindr
