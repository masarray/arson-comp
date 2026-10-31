#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

namespace arson::ui {
// Canonical visual bounds for the satin metal rotary face AND its red arc.
// Never permit an arc stroke to be cropped by a JUCE Slider component.
struct KnobGeometry final {
    static constexpr float arcOutsideFace=2.2f;
    static constexpr float widestArcStroke=5.2f;
    static constexpr float safeInset=1.25f;
    static int facePixels(int width,int height,bool threshold) noexcept {
        const float fraction=threshold?0.400f:0.385f;
        const float suggestedRadius=std::max(13.0f,
            std::min(static_cast<float>(width)*fraction,
                     static_cast<float>(height)*fraction));
        const int desired=juce::jmax(28,
            static_cast<int>(std::ceil(suggestedRadius*2.0f+7.0f)));
        const int available=static_cast<int>(std::floor(
            static_cast<float>(std::min(width,height))-
            2.0f*(arcOutsideFace+widestArcStroke*0.5f+safeInset)));
        return std::min(desired,std::max(24,available));
    }
    static float outerPaintRadius(int faceDiameter) noexcept {
        return static_cast<float>(faceDiameter)*0.5f+
            arcOutsideFace+widestArcStroke*0.5f;
    }
};
} // namespace arson::ui
