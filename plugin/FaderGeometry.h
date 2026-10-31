#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace arson::ui {
// One uniform image scale for BOTH components of the physical fader.
// The artwork is 340x57 (track) and 58x124 (thumb). Never compress the
// track height or stretch the thumb just to fill a short JUCE slot.
struct FaderGeometry {
    static constexpr float trackSourceW=340.0f,trackSourceH=57.0f;
    static constexpr float thumbSourceW=58.0f,thumbSourceH=124.0f;
    juce::Rectangle<float> track,thumb;
    float left=0.0f,right=0.0f,position=0.0f;
    static FaderGeometry calculate(float width,float height,float sliderPos) noexcept {
        const float scale=juce::jlimit(0.01f,1.0f,
            std::min((width-8.0f)/trackSourceW,(height-4.0f)/thumbSourceH));
        const float trackW=trackSourceW*scale,trackH=trackSourceH*scale;
        const float thumbW=thumbSourceW*scale,thumbH=thumbSourceH*scale;
        const float centerY=height*0.5f;
        const float trackX=(width-trackW)*0.5f;
        FaderGeometry g;
        g.track={trackX,centerY-trackH*0.5f,trackW,trackH};
        g.left=trackX+thumbW*0.5f+2.0f;
        g.right=trackX+trackW-thumbW*0.5f-2.0f;
        g.position=juce::jlimit(g.left,g.right,sliderPos);
        g.thumb={g.position-thumbW*0.5f,centerY-thumbH*0.5f,thumbW,thumbH};
        return g;
    }
};
} // namespace arson::ui
