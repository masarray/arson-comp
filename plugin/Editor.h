#pragma once
#include "Processor.h"
#include <juce_gui_extra/juce_gui_extra.h>
class CompLook final : public juce::LookAndFeel_V4 {
public:
    CompLook();
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class Editor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void applyPreset(int);
    void setValue(std::size_t,float);
    void drawDisplay(juce::Graphics&);
    Processor& processor;
    CompLook look;
    juce::TooltipWindow tooltip{this,650};
    std::array<juce::Slider,arson::Detector> knobs;
    std::array<juce::Label,arson::Detector> labels;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<std::unique_ptr<SliderAttachment>,arson::Detector> attachments;
    juce::ComboBox presets,detector,style;
    juce::TextButton compare{"A / B"};
    juce::ToggleButton autoRelease{"Auto release"},external{"External SC"},listen{"Listen SC"},bypass{"Bypass"};
    std::array<std::unique_ptr<ButtonAttachment>,4> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectorAttachment,styleAttachment;
    arson::Parameters alternate, presetSnapshot;
    bool alternateActive=false;
    std::array<arson::MeterFrame,400> history{};
    std::size_t head=0;
    float inputDb=-120,outputDb=-120,gr=0;
    juce::Rectangle<int> display;
    juce::Image grid;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
