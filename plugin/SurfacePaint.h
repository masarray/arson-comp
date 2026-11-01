#pragma once
#include <juce_graphics/juce_graphics.h>
#include <algorithm>
#include <cstdint>

namespace arson::ui {
// Resizable brushed gunmetal chassis, painted ONCE into Editor::metalSurface
// when the plugin dimensions change. No noise, blur, bitmap IO or shader in
// timerCallback / audio callback. Deterministic scan lines avoid RNG state.
// Realistic flat brushed gunmetal. Grain is HORIZONTAL and deliberately
// flat: no radial/diagonal sheen, no waves and no separate photo overlay.
// This runs once at Editor::resized(), not in the meter timer or audio callback.
inline void paintBrushedChassis(juce::Graphics& g,int width,int height) {
    g.fillAll(juce::Colour{0xff4b4e50});
    for(int y=0;y<height;++y) {
        // Pure deterministic row-coherent micro-striations, no RNG state.
        std::uint32_t k=static_cast<std::uint32_t>(y)+0x9e3779b9u;
        k^=k>>16; k*=0x7feb352du;
        k^=k>>15; k*=0x846ca68bu; k^=k>>16;
        const float a=0.045f+static_cast<float>((k>>25)&31u)*0.0015f;
        g.setColour((k&1u) ? juce::Colours::white.withAlpha(a) :
                              juce::Colours::black.withAlpha(a*1.10f));
        g.drawHorizontalLine(y,0.0f,static_cast<float>(width));
        // Short directionally brushed scratches without large-scale banding.
        if(y%3==0) {
            for(int cell=0;cell<(width+63)/64;++cell) {
                const std::uint32_t t=k^(static_cast<std::uint32_t>(cell+1)*2246822519u);
                const int x=cell*64+static_cast<int>((t>>22)&15u);
                const int right=std::min(width,x+15+static_cast<int>((t>>16)&31u));
                if(right<=x)continue;
                g.setColour((t&1u) ? juce::Colours::white.withAlpha(0.040f) :
                                    juce::Colours::black.withAlpha(0.050f));
                g.drawHorizontalLine(y,static_cast<float>(x),static_cast<float>(right));
            }
        }
    }
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(0,0.0f,static_cast<float>(width));
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawHorizontalLine(height-1,0.0f,static_cast<float>(width));
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
