#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace arson::ui {

// One canonical geometry/contrast contract for the compact live compressor.
// Keep actual controls comfortably sized; remove blank canvas before scaling
// text/knobs down. Reused by the editor renderer and the Windows smoke tests.
struct EditorMetrics {
    static constexpr int minWidth=660, minHeight=490;
    static constexpr int defaultWidth=704, defaultHeight=520;
    static constexpr int maxWidth=1040, maxHeight=780;
    static constexpr int miniHeight=330; // preserve MINI meter readability
    static constexpr int headerHeight=62, displayTop=64;
    static constexpr int displayBottomReserve=260;
    static constexpr int bottomControlsReserve=92;
    static constexpr juce::uint32 captionRgb=0xffd9e0e3;
    static constexpr juce::uint32 controlCaptionRgb=0xffe4e9ec;
    static constexpr juce::uint32 headerBackgroundRgb=0xff292d30;
    static constexpr juce::uint32 metalBackgroundRgb=0xff454a4d;
};

struct HeaderLayout {
    juce::Rectangle<int> logo,mini,model,preset,compare,file,bypass;

    static HeaderLayout calculate(int width) noexcept {
        constexpr int inset=12,gap=8;
        constexpr int logoW=126,miniW=42,modelW=96,compareW=40,fileW=44,bypassW=110;
        constexpr int fixed=logoW+miniW+modelW+compareW+fileW+bypassW;
        const int available=width-2*inset-6*gap-fixed;
        const int presetW=juce::jlimit(124,154,available);
        constexpr int top=22,height=27;
        HeaderLayout h;
        int x=inset;
        h.logo={x,17,logoW,29};x+=logoW+gap;
        h.mini={x,top,miniW,height};x+=miniW+gap;
        h.model={x,top,modelW,height};x+=modelW+gap;
        h.preset={x,top,presetW,height};
        // Reserve the right actions; extra width becomes breathing room.
        h.bypass={width-inset-bypassW,top,bypassW,height};
        h.file={h.bypass.getX()-gap-fileW,top,fileW,height};
        h.compare={h.file.getX()-gap-compareW,top,compareW,height};
        return h;
    }
};
} // namespace arson::ui
