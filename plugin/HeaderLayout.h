#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace arson::ui {

// Single geometry source for JUCE resized(), brand painting and smoke tests.
// Compact presets never displace the model, A/B, file menu or main bypass.
struct HeaderLayout {
    juce::Rectangle<int> logo,mini,model,preset,compare,file,bypass;

    static HeaderLayout calculate(int width) noexcept {
        constexpr int inset=14,gap=10;
        constexpr int logoW=137,miniW=46,modelW=112,compareW=48,fileW=50,bypassW=114;
        constexpr int fixed=logoW+miniW+modelW+compareW+fileW+bypassW;
        const int presetW=juce::jlimit(144,176,width-2*inset-6*gap-fixed);
        constexpr int top=24,height=28;
        HeaderLayout h;
        int x=inset;
        h.logo={x,17,logoW,32};x+=logoW+gap;
        h.mini={x,top,miniW,height};x+=miniW+gap;
        h.model={x,top,modelW,height};x+=modelW+gap;
        h.preset={x,top,presetW,height};

        // The right-side actions remain right-aligned when the editor grows.
        // Any additional width becomes intentional whitespace, not a huge preset.
        h.bypass={width-inset-bypassW,top,bypassW,height};
        h.file={h.bypass.getX()-gap-fileW,top,fileW,height};
        h.compare={h.file.getX()-gap-compareW,top,compareW,height};
        return h;
    }
};
} // namespace arson::ui
