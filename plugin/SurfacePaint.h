#pragma once
#include <juce_graphics/juce_graphics.h>
#include <algorithm>
#include <cstdint>

namespace arson::ui {
// Resizable brushed gunmetal chassis, painted ONCE into Editor::metalSurface
// when the plugin dimensions change. No noise, blur, bitmap IO or shader in
// timerCallback / audio callback. Deterministic scan lines avoid RNG state.
inline void paintBrushedChassis(juce::Graphics& g, int width, int height) {
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    const juce::Colour light{0xff65696b}, dark{0xff383c40};
    g.setGradientFill(juce::ColourGradient(light,0.0f,0.0f,
                                            dark,w,h,false));
    g.fillRect(0.0f,0.0f,w,h);
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::white.withAlpha(0.09f),w*0.16f,0.0f,
        juce::Colours::black.withAlpha(0.08f),w*0.88f,0.0f,false));
    g.fillRect(0.0f,0.0f,w,h);
    // Fine horizontally brushed metal grain. Baked into a single image.
    for(int y=0;y<height;y+=2) {
        const std::uint32_t k=static_cast<std::uint32_t>(y)*2654435761u+1013904223u;
        const float a=0.026f+static_cast<float>((k>>25)&15u)/1400.0f;
        g.setColour((k&1u)?juce::Colours::white.withAlpha(a):
                            juce::Colours::black.withAlpha(a*1.45f));
        g.drawHorizontalLine(y,0.0f,w);
    }
    g.setColour(juce::Colours::white.withAlpha(0.085f));
    g.drawHorizontalLine(1,0.0f,w);
    g.setColour(juce::Colours::black.withAlpha(0.32f));
    g.drawHorizontalLine(height-2,0.0f,w);
}

inline void paintInsetGlass(juce::Graphics& g,juce::Rectangle<float> bounds) {
    const auto r=bounds.reduced(1.5f);
    g.setColour(juce::Colours::black.withAlpha(0.58f));
    g.fillRoundedRectangle(r.translated(0.0f,2.0f),6.0f);
    g.setColour(juce::Colour{0xff101316});
    g.fillRoundedRectangle(r,6.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff0d1114},
        r.getX(),r.getY(),juce::Colour{0xff030607},r.getRight(),r.getBottom(),false));
    g.fillRoundedRectangle(r.reduced(2.5f),4.8f);
    g.setColour(juce::Colours::black.withAlpha(0.76f));
    g.drawRoundedRectangle(r.reduced(1.0f),5.7f,2.1f);
    g.setColour(juce::Colour{0xff8a9295}.withAlpha(0.42f));
    g.drawRoundedRectangle(r,6.0f,0.9f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine(r.getX()+7.0f,r.getY()+3.0f,
               r.getRight()-7.0f,r.getY()+3.0f,0.8f);
}
} // namespace arson::ui
