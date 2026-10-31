#pragma once
#include "Parameters.h"
#include "MeterQueue.h"
#include <cstdint>
namespace arson {
// Framework-free, audio-thread-owned state. Planar mono/stereo, arbitrary block size.
class Compressor {
public:
    void prepare(double sampleRate, const Parameters& initial=Parameters{}) noexcept;
    void setParameters(Parameters parameters) noexcept;
    void process(float* const* audio, int channels, int samples,
                 const float* const* sidechain=nullptr, int sidechainChannels=0,
                 MeterQueue* meters=nullptr) noexcept;
    static constexpr int latencySamples() noexcept { return 0; }
private:
    struct Ramp {
        float current=0,target=0,step=0; int remaining=0;
        void reset(float x) noexcept { current=target=x; remaining=0; step=0; }
        void set(float x,int length) noexcept {
            if(x==target) return;
            target=x; remaining=length; step=(target-current)/static_cast<float>(length);
        }
        float next() noexcept { if(remaining>0 && --remaining==0) current=target; else if(remaining>0) current+=step; return current; }
    };
    Parameters p;
    std::array<Ramp,Count> smooth{};
    // Canonical smoothed values and sparse ramp indexes. Audio calls never
    // allocate; stable controls cause zero per-sample ramp work.
    std::array<float,Count> currentValues{};
    std::array<std::uint8_t,Count> activeRampIndices{};
    std::size_t activeRampCount=0;
    // Preconverted lower knee edge for the steady-parameter no-compression path.
    // Disabled while threshold/knee are ramping to retain exact automation.
    float lowerKneeLinear=0.0f;
    bool staticKneeGate=false;
    // Filter on/off is blended separately from cutoff coefficients to avoid endpoint steps.
    std::array<Ramp,2> filterWet{};
    std::array<float,2> hpState{}, lpState{}, rms{}, envelope{}, smoothEnvelope{}, previousGain{1.0f,1.0f};
    std::array<int,2> holdRemaining{};
    double fs=48000;
    float attackCoeff=0,releaseCoeff=0,slowReleaseCoeff=0,rmsCoeff=0;
    int rampLength=240, meterPeriod=480, meterCount=0,holdSamples=0;
    MeterFrame meter{};
};
}
