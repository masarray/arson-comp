#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace arson::ui {
// Canonical vector geometry for the six horizontal faders.
// No bitmap aspect ratios, resampling or stretched controls.
struct FaderGeometry {
    static constexpr float railHeight=6.0f;
    juce::Rectangle<float> track, thumb;
    float left=0.0f, right=0.0f, position=0.0f;
    static FaderGeometry calculate(float width,float height,float sliderPos) noexcept {
        const float inset=juce::jlimit(8.0f,11.0f,width*0.10f);
        const float capW=juce::jlimit(12.0f,15.0f,height*0.40f);
        const float capH=juce::jlimit(22.0f,29.0f,height-5.0f);
        const float cy=height*0.5f;
        FaderGeometry g;
        g.track={inset,cy-railHeight*0.5f,width-2.0f*inset,railHeight};
        g.left=inset+capW*0.5f;
        g.right=width-inset-capW*0.5f;
        g.position=juce::jlimit(g.left,g.right,sliderPos);
        g.thumb={g.position-capW*0.5f,cy-capH*0.5f,capW,capH};
        return g;
    }
};
} // namespace arson::ui
