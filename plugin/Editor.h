#pragma once
#include "Processor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
class CompLook final : public juce::LookAndFeel_V4 {
public:
    CompLook();
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
                          const juce::Slider::SliderStyle,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawTooltip(juce::Graphics&,const juce::String&,int,int) override;
private:
    struct FaceCache { int pixels=0; juce::Image face; };
    std::array<FaceCache,2> faces; // normal + oversized Threshold only
    juce::Image sourceFace; // decoded once on GUI initialization
    const juce::Image& knobFace(int diameter,bool primary);
};
class Editor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void applyPreset(int);
    void setValue(std::size_t,float);
    void drawDisplay(juce::Graphics&);
    juce::Rectangle<float> plotBounds() const noexcept;
    void refreshTransfer(const arson::Parameters&,juce::Rectangle<float>);
    void drawMeter(juce::Graphics&,juce::Rectangle<float>,float,float,
                   const juce::String&,bool);
    void paintCompactMeters(juce::Graphics&);
    void rebuildHistoryPaths();
    void syncPanelVisibility();
    Processor& processor;
    CompLook look;
    juce::TooltipWindow tooltip{this,650};
    std::array<juce::Slider,arson::Detector> knobs;
    std::array<juce::Label,arson::Detector> labels;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<std::unique_ptr<SliderAttachment>,arson::Detector> attachments;
    juce::ComboBox presets,detector,style;
    juce::TextButton compare{"A/B"},compact{"MINI"},savePreset{"SAVE"},loadPreset{"OPEN"};
    juce::TextButton fileMenu{"FILE"},dynamicsTab{"DYNAMICS"},sidechainTab{"SIDECHAIN"};
    juce::ToggleButton autoRelease{"Auto release"},external{"External SC"},listen{"Listen SC"},bypass{"Bypass"};
    std::array<std::unique_ptr<ButtonAttachment>,4> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectorAttachment,styleAttachment;
    std::unique_ptr<juce::FileChooser> presetChooser;
    arson::Parameters alternate, presetSnapshot;
    bool alternateActive=false;
    bool compactMode=false,thresholdDragging=false,sidechainPanel=false;
    int expandedWidth=900,expandedHeight=765;
    std::array<arson::MeterFrame,400> history{};
    std::size_t head=0;
    float inputDb=-120,outputDb=-120,gr=0;
    double lastUiTickMs=0.0;
    std::array<float,2> inChannelDb{{-120.0f,-120.0f}},outChannelDb{{-120.0f,-120.0f}};
    int currentChannels=2;
    juce::Rectangle<int> display;
    juce::Image grid;
    juce::Path transferCurve,transferShade;
    juce::Path inputHistory,outputHistory,reductionHistory,reductionArea;
    // Six cached horizontal gradient brushes: input, output, GR, transfer,
    // transfer glow and reduction-area fade. Built only on full-view resize.
    std::array<juce::ColourGradient,6> edgeFades;
    std::array<float,4> cachedCurveKey{{999,999,999,999}};
    float cachedCurveWidth=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
