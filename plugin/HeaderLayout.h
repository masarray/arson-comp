#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace arson::ui {

// One canonical geometry/contrast contract for the compact live compressor.
// Keep actual controls comfortably sized; remove blank canvas before scaling
// text/knobs down. Reused by the editor renderer and the Windows smoke tests.
struct EditorMetrics {
    // Dense-but-usable hardware UI: 612px default instead of 704px.
    // Main knobs retain 58px hit targets and readable 10.4pt captions.
    static constexpr int minWidth=580, minHeight=480;
    static constexpr int defaultWidth=612, defaultHeight=500;
    static constexpr int maxWidth=1000, maxHeight=760;
    static constexpr int miniHeight=320;
    static constexpr int headerHeight=62, displayTop=64;
    static constexpr int displayBottomReserve=254;
    static constexpr int bottomControlsReserve=86;
    // Single control geometry/typography contract; JUCE smoke uses these.
    static constexpr float headerCaptionFont=9.5f;
    static constexpr float buttonFont=11.0f;
    static constexpr float controlCaptionFont=10.4f;
    static constexpr int knobCaptionOffset=2, knobTopOffset=26, knobHeight=66;
    static constexpr int faderCaptionOffset=25, faderTopOffset=44, faderHeight=34;
    static constexpr juce::uint32 captionRgb=0xffd9e0e3;
    static constexpr juce::uint32 controlCaptionRgb=0xffe4e9ec;
    static constexpr juce::uint32 headerBackgroundRgb=0xff292d30;
    static constexpr juce::uint32 metalBackgroundRgb=0xff454a4d;
};

struct HeaderLayout {
    juce::Rectangle<int> logo,mini,model,preset,compare,file,bypass;

    static HeaderLayout calculate(int width) noexcept {
        // One-row responsive grid with a compact logo and right-anchored
        // actions, so long preset names remain accessible via the browser.
        constexpr int inset=10,gap=6;
        constexpr int logoW=88,miniW=50,modelW=86;
        constexpr int compareW=38,fileW=38,bypassW=100;
        constexpr int fixed=logoW+miniW+modelW+compareW+fileW+bypassW;
        const int available=width-2*inset-6*gap-fixed;
        const int presetW=juce::jlimit(124,148,available);
        constexpr int top=22,height=27;
        HeaderLayout h;
        int x=inset;
        h.logo={x,17,logoW,28};x+=logoW+gap;
        h.mini={x,top,miniW,height};x+=miniW+gap;
        h.model={x,top,modelW,height};x+=modelW+gap;
        h.preset={x,top,presetW,height};
        h.bypass={width-inset-bypassW,top,bypassW,height};
        h.file={h.bypass.getX()-gap-fileW,top,fileW,height};
        h.compare={h.file.getX()-gap-compareW,top,compareW,height};
        return h;
    }
};
} // namespace arson::ui
