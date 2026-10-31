#pragma once
#include "Parameters.h"
#include <vector>
#include <utility>
#include <initializer_list>
#include <string_view>

namespace arson::preset {
struct FactoryPreset {
    const char* id;
    const char* category;
    const char* name;
    const char* tags;
    const char* description;
    Parameters settings;
};
inline FactoryPreset make(const char* id,const char* group,const char* name,
                           const char* tags,const char* description,
                           std::initializer_list<std::pair<Param,float>> changes) {
    FactoryPreset v{id,group,name,tags,description,Parameters{}};
    for(const auto& entry:changes)v.settings.value[entry.first]=entry.second;
    // Factory programs must never silently turn on external sidechain,
    // monitoring or bypass (surprising silent/misrouted live audio).
    v.settings.value[External]=0.0f;
    v.settings.value[Listen]=0.0f;
    v.settings.value[Bypass]=0.0f;
    // Factory makeup is an intentional *starting* compensation, not
    // automatic loudness normalization and never a limiter. For a nominal
    // -18 dBFS reference, estimate static GR with the actual compressor
    // transfer law, scale by wet mix, and add a conservative audible lift.
    // All work happens once when the factory catalog is first built on
    // the message thread, not in the audio callback.
    const std::string_view category(group);
    const bool mastering=category=="Mastering";
    const bool voice=category=="Vocals" || category=="Live & Broadcast";
    const bool effects=category=="Special";
    const float referenceGr=reduction(-18.0f,v.settings[Threshold],
        v.settings[Ratio],v.settings[Knee],v.settings[Range]);
    const float wet=v.settings[Mix]*0.01f;
    const float lift=mastering ? 0.85f : voice ? 1.55f : effects ? 1.30f : 1.35f;
    const float maximum=mastering ? 3.2f : effects ? 8.0f : voice ? 6.0f : 5.8f;
    v.settings.value[Makeup]=std::clamp(lift + 0.82f*referenceGr*wet,
        lift,maximum);
    v.settings.sanitize();
    return v;
}
inline const std::vector<FactoryPreset>& factoryBank() {
    static const std::vector<FactoryPreset> bank={
        make("neutral-start","Clean","Neutral Start","clean neutral transparent","A clear, audible starter with moderate leveling and compensated output.", {{Threshold,-24.0f},{Ratio,2.5f},{Attack,16.0f},{Release,170.0f},{Knee,10.0f},{Range,12.0f}}),
        make("smooth-transparent","Clean","Transparent Control","clean smooth subtle","Low-ratio, wide-knee compression for natural level regulation.", {{Threshold,-21.0f}, {Ratio,1.8f}, {Attack,28.0f}, {Release,240.0f}, {Knee,18.0f}, {Range,12.0f}, {Style,2.0f}}),
        make("peak-catcher","Clean","Peak Catch","clean peaks transient","Fast peak containment with limited reduction; not a true-peak limiter.", {{Threshold,-16.0f}, {Ratio,5.0f}, {Attack,1.5f}, {Release,75.0f}, {Knee,3.0f}, {Range,10.0f}, {Detector,0.0f}}),
        make("parallel-preserve","Clean","Parallel Control","parallel clean transient","Parallel compression that retains part of the unprocessed dynamics.", {{Threshold,-25.0f}, {Ratio,5.0f}, {Attack,12.0f}, {Release,140.0f}, {Knee,8.0f}, {Mix,50.0f}, {Range,18.0f}}),
        make("vocal-natural","Vocals","Natural Vocal","vocal singer smooth","Gentle vocal leveling that preserves breath and consonants.", {{Threshold,-25.0f}, {Ratio,2.4f}, {Attack,13.0f}, {Release,150.0f}, {Knee,12.0f}, {Range,10.0f}, {Style,2.0f}, {AutoRelease,1.0f}}),
        make("vocal-forward","Vocals","Forward Lead","vocal lead presence","Assertive lead vocal control without an excessively fast attack.", {{Threshold,-28.0f}, {Ratio,3.5f}, {Attack,7.0f}, {Release,110.0f}, {Knee,9.0f}, {Range,16.0f}, {Highpass,95.0f}, {Style,0.0f}}),
        make("vocal-broadcast","Vocals","Broadcast Voice","vocal podcast speech","Consistent spoken-word loudness with program-dependent release.", {{Threshold,-30.0f}, {Ratio,3.2f}, {Attack,4.0f}, {Release,180.0f}, {Knee,10.0f}, {Range,16.0f}, {Detector,1.0f}, {AutoRelease,1.0f}, {Highpass,90.0f}, {Style,2.0f}}),
        make("vocal-intimate","Vocals","Intimate Vocal","vocal intimate soft","Soft program leveling for quiet intimate performances.", {{Threshold,-29.0f}, {Ratio,2.2f}, {Attack,18.0f}, {Release,250.0f}, {Knee,20.0f}, {Range,9.0f}, {Detector,1.0f}, {Style,2.0f}}),
        make("vocal-rock","Vocals","Rock Lead","vocal rock firm","Firm vocal dynamics for dense guitar-driven arrangements.", {{Threshold,-25.0f}, {Ratio,4.5f}, {Attack,5.0f}, {Release,90.0f}, {Knee,6.0f}, {Range,20.0f}, {Highpass,110.0f}, {Style,1.0f}}),
        make("vocal-backing","Vocals","Backing Blend","vocal harmony bus","Gentle glue for stacked backing vocals and harmonies.", {{Threshold,-23.0f}, {Ratio,2.5f}, {Attack,23.0f}, {Release,220.0f}, {Knee,13.0f}, {Range,10.0f}, {Style,2.0f}, {Link,100.0f}}),
        make("drum-kick","Drums","Kick Weight","drums kick body","Maintain kick impact while smoothing excessive body resonance.", {{Threshold,-20.0f}, {Ratio,4.0f}, {Attack,20.0f}, {Release,95.0f}, {Knee,5.0f}, {Range,14.0f}, {Highpass,35.0f}, {Style,1.0f}}),
        make("drum-snare","Drums","Snare Snap","drums snare attack","Retain snare crack with a medium attack and lively release.", {{Threshold,-22.0f}, {Ratio,4.0f}, {Attack,16.0f}, {Release,85.0f}, {Knee,4.0f}, {Range,15.0f}, {Style,0.0f}}),
        make("drum-parallel","Drums","Parallel Smash","drums parallel heavy","Dense parallel drum energy, dry/wet blended for punch.", {{Threshold,-34.0f}, {Ratio,9.0f}, {Attack,3.0f}, {Release,65.0f}, {Knee,5.0f}, {Range,30.0f}, {Mix,45.0f}, {Style,1.0f}}),
        make("drum-room","Drums","Room Pump","drums room character","Pronounced room-compressor movement for deliberate effect.", {{Threshold,-31.0f}, {Ratio,6.0f}, {Attack,8.0f}, {Release,170.0f}, {Knee,4.0f}, {Range,24.0f}, {Style,1.0f}}),
        make("drum-bus","Drums","Drum Bus Glue","drums bus glue","Moderate drum-bus cohesion with transient headroom.", {{Threshold,-21.0f}, {Ratio,2.5f}, {Attack,28.0f}, {Release,140.0f}, {Knee,7.0f}, {Range,10.0f}, {Link,90.0f}, {Style,0.0f}}),
        make("drum-perc","Drums","Percussion Tame","drums percussion peak","Control uneven percussion peaks without flattening expression.", {{Threshold,-23.0f}, {Ratio,3.3f}, {Attack,4.0f}, {Release,110.0f}, {Knee,10.0f}, {Range,12.0f}, {Style,2.0f}}),
        make("bass-steady","Bass","Steady Bass","bass controlled even","Control low-note variation and sustain with smooth recovery.", {{Threshold,-25.0f}, {Ratio,3.5f}, {Attack,16.0f}, {Release,180.0f}, {Knee,10.0f}, {Range,14.0f}, {Detector,1.0f}, {Style,2.0f}}),
        make("bass-punch","Bass","Bass Attack","bass attack punch","Preserve front-edge impact with medium attack and release.", {{Threshold,-21.0f}, {Ratio,4.0f}, {Attack,25.0f}, {Release,125.0f}, {Knee,6.0f}, {Range,16.0f}, {Style,0.0f}}),
        make("bass-synth","Bass","Synth Bass Control","bass synth electronic","Dense synth bass level containment with a smooth knee.", {{Threshold,-28.0f}, {Ratio,4.5f}, {Attack,9.0f}, {Release,160.0f}, {Knee,12.0f}, {Range,20.0f}, {Detector,1.0f}, {Style,2.0f}}),
        make("bass-slap","Bass","Slap Peak Tame","bass slap transient","Control slap and pop spikes while retaining some articulation.", {{Threshold,-20.0f}, {Ratio,5.0f}, {Attack,3.0f}, {Release,90.0f}, {Knee,4.0f}, {Range,14.0f}, {Style,0.0f}}),
        make("guitar-acoustic","Guitar","Acoustic Gentle","guitar acoustic natural","Smooth acoustic strumming dynamics without pumping.", {{Threshold,-24.0f}, {Ratio,2.1f}, {Attack,23.0f}, {Release,180.0f}, {Knee,15.0f}, {Range,10.0f}, {Style,2.0f}}),
        make("guitar-electric","Guitar","Electric Rhythm","guitar electric rhythm","Subtle rhythm guitar leveling for consistent mixing.", {{Threshold,-22.0f}, {Ratio,2.7f}, {Attack,15.0f}, {Release,135.0f}, {Knee,9.0f}, {Range,10.0f}, {Style,0.0f}}),
        make("guitar-lead","Guitar","Lead Sustain","guitar solo sustain","Extend lead-line sustain with more assertive compression.", {{Threshold,-28.0f}, {Ratio,4.0f}, {Attack,6.0f}, {Release,220.0f}, {Knee,8.0f}, {Range,19.0f}, {Style,1.0f}}),
        make("guitar-clean","Guitar","Clean Picking","guitar clean fingerstyle","Keep picked notes even and articulate.", {{Threshold,-26.0f}, {Ratio,2.8f}, {Attack,11.0f}, {Release,150.0f}, {Knee,14.0f}, {Range,12.0f}, {Style,2.0f}}),
        make("piano-classical","Piano","Piano Natural","piano classical transparent","Very gentle dynamics for exposed acoustic piano.", {{Threshold,-19.0f}, {Ratio,1.6f}, {Attack,37.0f}, {Release,310.0f}, {Knee,22.0f}, {Range,7.0f}, {Style,2.0f}}),
        make("piano-pop","Piano","Pop Piano","piano pop even","Even pop piano levels while keeping chord attacks lively.", {{Threshold,-25.0f}, {Ratio,2.5f}, {Attack,20.0f}, {Release,180.0f}, {Knee,12.0f}, {Range,12.0f}, {Style,0.0f}}),
        make("piano-keys","Piano","Electric Keys","piano keys electric","Controlled sustain for modern electric keyboards.", {{Threshold,-27.0f}, {Ratio,3.0f}, {Attack,12.0f}, {Release,195.0f}, {Knee,11.0f}, {Range,14.0f}, {Style,2.0f}}),
        make("piano-percussive","Piano","Percussive Keys","piano keyboard transient","Let piano transients speak through moderate attack.", {{Threshold,-20.0f}, {Ratio,3.0f}, {Attack,31.0f}, {Release,125.0f}, {Knee,8.0f}, {Range,12.0f}, {Style,0.0f}}),
        make("mix-glue","Mixbus","Gentle Glue","mixbus stereo glue","2:1 stereo bus glue with conservative reduction.", {{Threshold,-18.0f}, {Ratio,2.0f}, {Attack,30.0f}, {Release,260.0f}, {Knee,13.0f}, {Range,6.0f}, {Link,100.0f}, {Style,2.0f}}),
        make("mix-forward","Mixbus","Forward Mix","mixbus pop rock","Tighter mix cohesion without crushing the transients.", {{Threshold,-21.0f}, {Ratio,2.5f}, {Attack,22.0f}, {Release,180.0f}, {Knee,9.0f}, {Range,9.0f}, {Link,100.0f}, {Style,1.0f}}),
        make("mix-parallel","Mixbus","Parallel Mix","mixbus parallel energy","Blend denser bus energy underneath the original.", {{Threshold,-27.0f}, {Ratio,4.0f}, {Attack,14.0f}, {Release,140.0f}, {Knee,8.0f}, {Range,16.0f}, {Mix,55.0f}, {Link,100.0f}}),
        make("mix-smooth","Mixbus","Smooth Program","mixbus smooth balance","Smooth overall dynamic movement for changing material.", {{Threshold,-20.0f}, {Ratio,1.7f}, {Attack,37.0f}, {Release,350.0f}, {Knee,20.0f}, {Range,6.0f}, {Detector,1.0f}, {AutoRelease,1.0f}, {Style,2.0f}}),
        make("master-gentle","Mastering","Gentle Program","mastering transparent low-gr","Subtle final program control; start with only 1–2 dB reduction.", {{Threshold,-15.0f}, {Ratio,1.4f}, {Attack,45.0f}, {Release,450.0f}, {Knee,24.0f}, {Range,4.0f}, {Style,2.0f}, {Link,100.0f}}),
        make("master-dynamic","Mastering","Dynamic Preserve","mastering transient preserve","Protect program transients and limit gain reduction.", {{Threshold,-17.0f}, {Ratio,1.7f}, {Attack,55.0f}, {Release,340.0f}, {Knee,15.0f}, {Range,5.0f}, {Style,0.0f}, {Link,100.0f}}),
        make("master-density","Mastering","Density Lift","mastering parallel density","Moderate parallel density; not a brickwall limiter.", {{Threshold,-22.0f}, {Ratio,2.6f}, {Attack,26.0f}, {Release,290.0f}, {Knee,14.0f}, {Range,8.0f}, {Mix,65.0f}, {Style,2.0f}}),
        make("master-smooth","Mastering","Slow Program","mastering rms long","Slow RMS-based envelope for wide program changes.", {{Threshold,-17.0f}, {Ratio,1.5f}, {Attack,65.0f}, {Release,600.0f}, {Knee,22.0f}, {Range,5.0f}, {Detector,1.0f}, {AutoRelease,1.0f}, {Style,2.0f}}),
        make("special-pumping","Special","Rhythmic Pump","effect electronic pumping","Audible rhythmic compression for electronic sources.", {{Threshold,-34.0f}, {Ratio,7.0f}, {Attack,1.0f}, {Release,300.0f}, {Knee,2.0f}, {Range,26.0f}, {Style,1.0f}}),
        make("special-crush","Special","Hard Clamp","effect aggressive hard","Aggressive peak reduction; reduce input if distortion appears.", {{Threshold,-35.0f}, {Ratio,12.0f}, {Attack,0.3f}, {Release,60.0f}, {Knee,0.0f}, {Range,35.0f}, {Style,0.0f}}),
        make("special-movement","Special","Slow Breathing","effect evolving ambient","Slow rise and recovery for dramatic ambience.", {{Threshold,-30.0f}, {Ratio,4.0f}, {Attack,150.0f}, {Release,950.0f}, {Knee,20.0f}, {Range,22.0f}, {Style,2.0f}}),
        make("special-ny-parallel","Special","NY Parallel","effect drums ny","Dense parallel compression for energetic drums.", {{Threshold,-31.0f}, {Ratio,7.0f}, {Attack,5.0f}, {Release,95.0f}, {Knee,7.0f}, {Range,25.0f}, {Mix,35.0f}, {Style,1.0f}}),
        make("live-speech","Live & Broadcast","Live Speech","live speech mic low-latency","Low-latency microphone dynamics. No added plugin samples.", {{Threshold,-24.0f}, {Ratio,3.0f}, {Attack,8.0f}, {Release,140.0f}, {Knee,12.0f}, {Range,12.0f}, {Highpass,110.0f}, {Style,2.0f}, {AutoRelease,1.0f}}),
        make("live-vocal","Live & Broadcast","Live Vocal","live vocal karaoke mic","Performance vocal control with conservative artifacts.", {{Threshold,-24.0f}, {Ratio,3.0f}, {Attack,12.0f}, {Release,150.0f}, {Knee,10.0f}, {Range,14.0f}, {Highpass,100.0f}, {Style,2.0f}, {AutoRelease,1.0f}}),
        make("live-podcast","Live & Broadcast","Podcast Voice","broadcast podcast conversation","Natural spoken-word consistency; monitor for pumping.", {{Threshold,-29.0f}, {Ratio,3.5f}, {Attack,5.0f}, {Release,200.0f}, {Knee,12.0f}, {Range,16.0f}, {Detector,1.0f}, {AutoRelease,1.0f}, {Highpass,95.0f}, {Style,2.0f}}),
        make("live-stream","Live & Broadcast","Streaming Bus","broadcast stream stereo","Gentle stereo bus level management for online streaming.", {{Threshold,-20.0f}, {Ratio,2.1f}, {Attack,25.0f}, {Release,230.0f}, {Knee,15.0f}, {Range,9.0f}, {Style,2.0f}, {Link,100.0f}}),
        make("live-instrument","Live & Broadcast","Stage Instrument","live instrument guitar","Transient-preserving compression for a stage instrument.", {{Threshold,-23.0f}, {Ratio,3.0f}, {Attack,20.0f}, {Release,135.0f}, {Knee,8.0f}, {Range,13.0f}, {Style,0.0f}}),
        make("live-radio","Live & Broadcast","Radio Presenter","broadcast radio voice","Firm broadcast speech dynamics with smooth release.", {{Threshold,-30.0f}, {Ratio,4.0f}, {Attack,4.0f}, {Release,210.0f}, {Knee,14.0f}, {Range,20.0f}, {Detector,1.0f}, {Highpass,120.0f}, {AutoRelease,1.0f}, {Style,2.0f}})
    };
    return bank;
}
inline int findFactoryId(std::string_view id) {
    const auto& presets=factoryBank();
    for(std::size_t i=0;i<presets.size();++i)
        if(presets[i].id==id)return static_cast<int>(i);
    return -1;
}
} // namespace arson::preset
