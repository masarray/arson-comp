#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace arson {
enum Param : std::size_t { Threshold, Ratio, Attack, Release, Knee, Range, Hold,
    Makeup, Mix, Input, Link, Highpass, Lowpass, Detector, AutoRelease, External, Listen, Bypass, Count };
struct Spec { const char* id; const char* name; const char* unit; float min, max, initial, skew; };
inline constexpr std::array<Spec, Count> specs {{
    {"threshold", "Threshold", " dB", -60, 0, -18, 1},
    {"ratio", "Ratio", ":1", 1, 20, 4, 0.45f},
    {"attack", "Attack", " ms", 0.05f, 250, 10, 0.3f},
    {"release", "Release", " ms", 10, 2000, 150, 0.35f},
    {"knee", "Knee", " dB", 0, 48, 6, 1},
    {"range", "Range", " dB", 0, 60, 36, 1},
    {"hold", "Hold", " ms", 0, 500, 0, 0.5f},
    {"makeup", "Makeup", " dB", -24, 24, 0, 1},
    {"mix", "Mix", " %", 0, 100, 100, 1},
    {"input", "Input", " dB", -24, 24, 0, 1},
    {"link", "Stereo link", " %", 0, 100, 100, 1},
    {"highpass", "SC high-pass", " Hz", 20, 1000, 20, 0.35f},
    {"lowpass", "SC low-pass", " Hz", 1000, 20000, 20000, 0.4f},
    {"detector", "Detector", "", 0, 1, 0, 1},
    {"autoRelease", "Auto release", "", 0, 1, 0, 1},
    {"external", "External SC", "", 0, 1, 0, 1},
    {"listen", "SC listen", "", 0, 1, 0, 1},
    {"bypass", "Bypass", "", 0, 1, 0, 1}
}};
struct Parameters {
    std::array<float, Count> value{};
    Parameters() noexcept { for (std::size_t i=0; i<Count; ++i) value[i]=specs[i].initial; }
    float operator[](std::size_t i) const noexcept { return value[i]; }
    void sanitize() noexcept {
        for (std::size_t i=0; i<Count; ++i) {
            auto& v=value[i];
            v=std::isfinite(v) ? std::clamp(v,specs[i].min,specs[i].max) : specs[i].initial;
            if (i>=Detector) v=std::round(v);
        }
    }
};
inline float gainToDb(float gain) noexcept { return 20.0f*std::log10(std::max(gain,1.0e-6f)); }
inline float dbToGain(float db) noexcept { return std::exp(db*0.1151292546497023f); }
// Positive gain reduction; C1-continuous at both soft-knee boundaries.
inline float reduction(float db,float threshold,float ratio,float knee,float range) noexcept {
    const float x=db-threshold, slope=1.0f-1.0f/ratio;
    float gr=0;
    if (knee<=0) gr=std::max(0.0f,x)*slope;
    else if (x>=knee*0.5f) gr=x*slope;
    else if (x>-knee*0.5f) { const float t=x+knee*0.5f; gr=slope*t*t/(2*knee); }
    return std::min(gr,range);
}
}
