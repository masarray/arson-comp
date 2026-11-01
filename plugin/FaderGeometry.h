#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace arson::ui {
// Cropped, transparent PNG source dimensions: 480x66 track, 96x96 thumb.
// The track remains stationary; the round silver cap slides on its axis.
struct FaderGeometry {
    static constexpr float trackAspect=480.0f/66.0f;
    juce::Rectangle<float> track,thumb;
    float left=0.0f,right=0.0f,position=0.0f;
    static FaderGeometry calculate(float width,float height,float sliderPos) noexcept {
        // Prevent image stretching at any supported Dynamics/Sidechain width.
        const float trackWidth=juce::jmax(1.0f,
            std::min(width-4.0f,(height-3.0f)*trackAspect));
        const float trackHeight=trackWidth/trackAspect;
        // Smaller VISUAL thumb while keeping the 34px interactive Slider bounds.
        const float thumbSize=juce::jlimit(20.0f,25.0f,height*0.70f);
        const float railX=(width-trackWidth)*0.5f;
        const float cy=height*0.5f;
        FaderGeometry g;
        g.track={railX,cy-trackHeight*0.5f,trackWidth,trackHeight};
        g.left=railX+thumbSize*0.5f+2.0f;
        g.right=railX+trackWidth-thumbSize*0.5f-2.0f;
        g.position=juce::jlimit(g.left,g.right,sliderPos);
        g.thumb={g.position-thumbSize*0.5f,cy-thumbSize*0.5f,
                 thumbSize,thumbSize};
        return g;
    }
};
} // namespace arson::ui
