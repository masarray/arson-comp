#include "Compressor.h"
namespace arson {
namespace {
float clean(float x) noexcept { return std::isfinite(x) ? std::clamp(x,-32.0f,32.0f) : 0.0f; }
float flush(float x) noexcept { return std::abs(x)<1.0e-20f ? 0.0f : x; }
}
void Compressor::prepare(double rate,const Parameters& initial) noexcept {
    fs=std::isfinite(rate) ? std::clamp(rate,8000.0,384000.0) : 48000;
    rampLength=std::max(1,static_cast<int>(fs*0.005));
    meterPeriod=std::max(1,static_cast<int>(fs*0.01));
    hpState={}; lpState={}; rms={}; envelope={}; holdRemaining={}; meter={}; meterCount=0;
    rmsCoeff=static_cast<float>(std::exp(-1.0/(0.01*fs)));
    p=initial; p.sanitize();
    for(std::size_t i=0;i<Count;++i) smooth[i].reset(i==Input || i==Makeup ? dbToGain(p[i]) : p[i]);
    // Force coefficient calculation on prepare, including when defaults are unchanged.
    const auto actual=p; p.value.fill(-999); setParameters(actual);
}
void Compressor::setParameters(Parameters next) noexcept {
    next.sanitize();
    if(next[Attack]!=p[Attack]) attackCoeff=static_cast<float>(std::exp(-1000.0/(next[Attack]*fs)));
    if(next[Release]!=p[Release]) {
        releaseCoeff=static_cast<float>(std::exp(-1000.0/(next[Release]*fs)));
        slowReleaseCoeff=static_cast<float>(std::exp(-1000.0/(next[Release]*4.0*fs)));
    }
    if(next[Highpass]!=p[Highpass]) hpCoeff=static_cast<float>(std::exp(-6.28318530718*next[Highpass]/fs));
    if(next[Lowpass]!=p[Lowpass]) lpCoeff=static_cast<float>(std::exp(-6.28318530718*std::min<double>(next[Lowpass],fs*0.45)/fs));
    holdSamples=static_cast<int>(next[Hold]*0.001*fs);
    for(std::size_t i=0;i<Count;++i) smooth[i].set(i==Input || i==Makeup ? dbToGain(next[i]) : next[i],rampLength);
    p=next;
}
void Compressor::process(float* const* audio,int channels,int samples,const float* const* sc,int scChannels,MeterQueue* meters) noexcept {
    if(!audio || channels<1 || channels>2 || samples<=0) return;
    const bool external=p[External]>0.5f;
    for(int n=0;n<samples;++n) {
        std::array<float,Count> v{};
        for(std::size_t i=0;i<Count;++i) v[i]=smooth[i].next();
        const float inputGain=v[Input],makeupGain=v[Makeup];
        float original[2]{},dry[2]{},detected[2]{},listen[2]{};
        for(int c=0;c<channels;++c) {
            original[c]=clean(audio[c][n]); dry[c]=original[c]*inputGain;
            float x=external ? ((sc && scChannels>0) ? clean(sc[std::min(c,scChannels-1)][n]) : 0.0f) : dry[c];
            hpState[c]=flush(hpCoeff*hpState[c]+(1-hpCoeff)*x);
            if(p[Highpass]>20.01f) x-=hpState[c];
            lpState[c]=flush(lpCoeff*lpState[c]+(1-lpCoeff)*x);
            if(p[Lowpass]<19999.0f) x=lpState[c];
            listen[c]=x;
            rms[c]=flush(rmsCoeff*rms[c]+(1-rmsCoeff)*x*x);
            // Crossfade detector modes to avoid a discontinuity on automation.
            detected[c]=std::abs(x)+(std::sqrt(rms[c])-std::abs(x))*v[Detector];
        }
        const float linked=std::max(detected[0],detected[channels-1]);
        for(int c=0;c<channels;++c) {
            const float detector=detected[c]+(linked-detected[c])*(v[Link]*0.01f);
            const float target=reduction(gainToDb(detector),v[Threshold],v[Ratio],v[Knee],v[Range]);
            auto& env=envelope[c];
            if(target>env) { env=target+attackCoeff*(env-target); holdRemaining[c]=holdSamples; }
            else if(holdRemaining[c]>0) --holdRemaining[c];
            else {
                const float adaptive=std::clamp(env/24.0f,0.0f,1.0f)*v[AutoRelease];
                const float coeff=releaseCoeff+(slowReleaseCoeff-releaseCoeff)*adaptive;
                env=target+coeff*(env-target);
            }
            env=flush(env);
            const float wet=dry[c]*dbToGain(-env)*makeupGain;
            float out=dry[c]+(wet-dry[c])*(v[Mix]*0.01f);
            out+=(listen[c]-out)*v[Listen];
            out=v[Bypass]>=1.0f ? original[c] : out+(original[c]-out)*v[Bypass];
            audio[c][n]=out;
            meter.input=std::max(meter.input,std::abs(dry[c]));
            meter.output=std::max(meter.output,std::abs(out));
            meter.reduction=std::max(meter.reduction,env*(1-v[Bypass]));
        }
        if(++meterCount>=meterPeriod) {
            if(meters) meters->push(meter);
            meter={}; meterCount=0;
        }
    }
}
}
