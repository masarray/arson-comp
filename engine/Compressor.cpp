#include "Compressor.h"
namespace arson {
namespace {
float clean(float x) noexcept { return std::isfinite(x) ? std::clamp(x,-32.0f,32.0f) : 0.0f; }
float flush(float x) noexcept { return std::abs(x)<1.0e-20f ? 0.0f : x; }
// Coefficients are calculated on control updates, not in the sample loop.
// Existing parameter ramps carry the one-pole coefficient for the two filters.
float rampTarget(std::size_t index,float value,double fs) noexcept {
    if(index==Input || index==Makeup) return dbToGain(value);
    if(index==Highpass) return static_cast<float>(std::exp(-6.28318530718*value/fs));
    if(index==Lowpass) return static_cast<float>(std::exp(-6.28318530718*std::min<double>(value,fs*0.45)/fs));
    return value;
}
}
void Compressor::prepare(double rate,const Parameters& initial) noexcept {
    fs=std::isfinite(rate) ? std::clamp(rate,8000.0,384000.0) : 48000;
    rampLength=std::max(1,static_cast<int>(fs*0.005));
    meterPeriod=std::max(1,static_cast<int>(fs*0.01));
    hpState={}; lpState={}; rms={}; envelope={}; smoothEnvelope={}; previousGain={1.0f,1.0f}; holdRemaining={}; meter={}; meterCount=0;
    rmsCoeff=static_cast<float>(std::exp(-1.0/(0.01*fs)));
    p=initial; p.sanitize();
    for(std::size_t i=0;i<Count;++i) smooth[i].reset(rampTarget(i,p[i],fs));
    filterWet[0].reset(p[Highpass]>20.01f ? 1.0f : 0.0f);
    filterWet[1].reset(p[Lowpass]<19999.0f ? 1.0f : 0.0f);
    attackCoeff=static_cast<float>(std::exp(-1000.0/(p[Attack]*fs)));
    releaseCoeff=static_cast<float>(std::exp(-1000.0/(p[Release]*fs)));
    slowReleaseCoeff=static_cast<float>(std::exp(-1000.0/(p[Release]*4.0*fs)));
    holdSamples=static_cast<int>(p[Hold]*0.001*fs);
}
void Compressor::setParameters(Parameters next) noexcept {
    next.sanitize();
    if(next[Attack]!=p[Attack]) attackCoeff=static_cast<float>(std::exp(-1000.0/(next[Attack]*fs)));
    if(next[Release]!=p[Release]) {
        releaseCoeff=static_cast<float>(std::exp(-1000.0/(next[Release]*fs)));
        slowReleaseCoeff=static_cast<float>(std::exp(-1000.0/(next[Release]*4.0*fs)));
    }
    // Host snapshots arrive every audio block, usually without parameter
    // changes. Coalesce all target calculations; avoid exp() when idle.
    if(next[Hold]!=p[Hold]) holdSamples=static_cast<int>(next[Hold]*0.001*fs);
    for(std::size_t i=0;i<Count;++i) {
        if(next[i]!=p[i]) smooth[i].set(rampTarget(i,next[i],fs),rampLength);
    }
    if(next[Highpass]!=p[Highpass])
        filterWet[0].set(next[Highpass]>20.01f ? 1.0f : 0.0f,rampLength);
    if(next[Lowpass]!=p[Lowpass])
        filterWet[1].set(next[Lowpass]<19999.0f ? 1.0f : 0.0f,rampLength);
    p=next;
}
void Compressor::process(float* const* audio,int channels,int samples,const float* const* sc,int scChannels,MeterQueue* meters) noexcept {
    if(!audio || channels<1 || channels>2 || samples<=0) return;
    for(int n=0;n<samples;++n) {
        std::array<float,Count> v{};
        for(std::size_t i=0;i<Count;++i) v[i]=smooth[i].next();
        const float inputGain=v[Input],makeupGain=v[Makeup];
        const float hpCoeff=v[Highpass],lpCoeff=v[Lowpass];
        // Clean feedforward; Feedback senses prior actual attenuation;
        // Smooth cascades the envelope. 5-ms ramps interpolate models.
        const float feedbackWeight=std::clamp(std::min(v[Style],2.0f-v[Style]),0.0f,1.0f);
        const float smoothWeight=std::clamp(v[Style]-1.0f,0.0f,1.0f);
        const float hpWet=filterWet[0].next(),lpWet=filterWet[1].next();
        float original[2]{},dry[2]{},detected[2]{},listen[2]{};
        for(int c=0;c<channels;++c) {
            original[c]=clean(audio[c][n]); dry[c]=original[c]*inputGain;
            const float side=(sc && scChannels>0) ? clean(sc[std::min(c,scChannels-1)][n]) : 0.0f;
            const float internal=dry[c]*(1.0f-feedbackWeight*(1.0f-previousGain[c]));
            float x=internal+(side-internal)*v[External];
            hpState[c]=flush(hpCoeff*hpState[c]+(1-hpCoeff)*x);
            x-=hpWet*hpState[c];
            lpState[c]=flush(lpCoeff*lpState[c]+(1-lpCoeff)*x);
            x+=(lpState[c]-x)*lpWet;
            listen[c]=x;
            rms[c]=flush(rmsCoeff*rms[c]+(1-rmsCoeff)*x*x);
            // Crossfade detector modes to avoid a discontinuity on automation.
            const float absLevel=std::abs(x);
            const float rmsMix=v[Detector];
            // Maintain RMS state when inactive for click-free detector
            // switching, but skip sqrt() for the common Peak-only mode.
            detected[c]=rmsMix<=0.0f ? absLevel
                : absLevel+(std::sqrt(rms[c])-absLevel)*rmsMix;
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
            // Distinct cascaded reduction pole, active only in Smooth mode.
            if(smoothWeight>0.0f) {
                const float coeff=env>smoothEnvelope[c] ? attackCoeff : releaseCoeff;
                smoothEnvelope[c]=flush(env+coeff*(smoothEnvelope[c]-env));
            } else smoothEnvelope[c]=env;
            const float effectiveEnv=env+(smoothEnvelope[c]-env)*smoothWeight;
            const float attenuation=dbToGain(-effectiveEnv);
            previousGain[c]=attenuation;
            const float wet=dry[c]*attenuation*makeupGain;
            float out=dry[c]+(wet-dry[c])*(v[Mix]*0.01f);
            out+=(listen[c]-out)*v[Listen];
            out=v[Bypass]>=1.0f ? original[c] : out+(original[c]-out)*v[Bypass];
            audio[c][n]=out;
            meter.input=std::max(meter.input,std::abs(dry[c]));
            meter.output=std::max(meter.output,std::abs(out));
            meter.reduction=std::max(meter.reduction,effectiveEnv*(1-v[Bypass]));
        }
        if(++meterCount>=meterPeriod) {
            if(meters) meters->push(meter);
            meter={}; meterCount=0;
        }
    }
}
}
