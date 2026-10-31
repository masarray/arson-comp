#pragma once
#include <juce_graphics/juce_graphics.h>
#include <algorithm>

namespace arson::ui {
// A four-stop horizontal opacity envelope. Same RGB at every stop avoids
// muddy colour mixing, and keeps line edges organically dissolved.
// Only called on the message thread on component resize, never during audio.
inline juce::ColourGradient makeEdgeFade(juce::Colour colour,float x0,float x1,
                                         float opacity,float edgeOpacity=0.0f) {
    const float visible=juce::jlimit(0.0f,1.0f,opacity);
    const float edge=visible*juce::jlimit(0.0f,1.0f,edgeOpacity);
    juce::ColourGradient gradient(colour.withAlpha(edge),x0,0.0f,
                                  colour.withAlpha(edge),x1,0.0f,false);
    gradient.addColour(0.135,colour.withAlpha(visible));
    gradient.addColour(0.865,colour.withAlpha(visible));
    return gradient;
}

// Static segmented meter rails go in Editor's cached grid image, not its
// 30Hz paint path. Active LEDs alone are painted over the rails at runtime.
inline void paintMeterRail(juce::Graphics& g,juce::Rectangle<float> bounds,
                           bool gainReduction) {
    const int bars=gainReduction?1:2;
    const float bw=bars==1?13.0f:9.0f;
    const float spacing=3.0f, total=bw*bars+spacing*(bars-1);
    const float start=bounds.getX()+(bounds.getWidth()-total)*0.5f;
    constexpr int segments=46;
    const float segmentH=bounds.getHeight()/static_cast<float>(segments);
    for(int channel=0;channel<bars;++channel) {
        const float bx=start+channel*(bw+spacing);
        g.setColour(juce::Colour{0xff20262a});
        g.fillRect(bx,bounds.getY(),bw,bounds.getHeight());
        g.setColour(juce::Colour{0xff313637}.withAlpha(0.52f));
        for(int step=0;step<segments;++step) {
            const float yy=bounds.getY()+step*segmentH;
            g.fillRect(bx,yy,bw,std::max(1.0f,segmentH-1.2f));
        }
        g.setColour(juce::Colour{0xff535b60});
        g.drawRect(bx-1.0f,bounds.getY()-1.0f,bw+2.0f,
                   bounds.getHeight()+2.0f,0.7f);
    }
}
} // namespace arson::ui
