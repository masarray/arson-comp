#pragma once
#include "Processor.h"
#include "FactoryPresetBank.h"
#include <vector>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
class CompLook final : public juce::LookAndFeel_V4 {
public:
    CompLook();
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,
                              const juce::Colour&,bool,bool) override;
    juce::Font getTextButtonFont(juce::TextButton&,int) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
                          const juce::Slider::SliderStyle,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
private:
    struct FaceCache { int pixels=0; juce::Image face; };
    std::array<FaceCache,2> faces; // normal + oversized Threshold only
    juce::Image sourceFace; // decoded once on GUI initialization
    juce::Image toggleOn, toggleOff; // matched, predecoded state sprites
    juce::Image sliderTrack, sliderThumb, sliderThumbCompact; // native-resolution prefiltered fader cap
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
    void mouseEnter(const juce::MouseEvent&) override;
private:
    // Compact multi-category audition/search browser on the message thread.
    // Owns only GUI state and a bounded factory index; never touches DSP.
    class PresetBrowser final : public juce::Component,
                                private juce::ListBoxModel,
                                private juce::KeyListener {
    public:
        explicit PresetBrowser(Editor&);
        ~PresetBrowser() override;
        void paint(juce::Graphics&) override;
        void resized() override;
        void open();
        int getNumRows() override;
        void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
        void selectedRowsChanged(int) override;
        void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override;
        bool keyPressed(const juce::KeyPress&,juce::Component*) override;
    private:
        void rebuildFilter();
        void showDetails();
        void applySelected(bool closeAfter);
        void persistFavorites() const;
        Editor& owner;
        juce::TextEditor search,detail;
        juce::Label heading,subheading;
        juce::ListBox resultList;
        juce::TextButton favoritesOnly{"FAVORITES"},
                         star{"STAR"},use{"APPLY"},close{"CLOSE"};
        std::vector<int> matches;
        std::vector<bool> favorites;
        bool showOnlyFavorites=false;
    };
    class GraphCanvas final : public juce::Component {
    public:
        explicit GraphCanvas(Editor& p) : owner(p) {
            setComponentID("view.graph.canvas");
            setOpaque(true);
        }
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseMove(const juce::MouseEvent&) override;
    private:
        Editor& owner;
    };
    void timerCallback() override;
    void applyPreset(int);
    void openPresetBrowser();
    void setValue(std::size_t,float);
    void drawDisplay(juce::Graphics&);
    juce::Rectangle<float> plotBounds() const noexcept;
    void refreshTransfer(const arson::Parameters&,juce::Rectangle<float>);
    void drawMeter(juce::Graphics&,juce::Rectangle<float>,float,float,
                   const juce::String&,bool);
    void paintCompactMeters(juce::Graphics&);
    void rebuildHistoryPaths();
    void syncPanelVisibility();
    void updateHoverReadout(std::size_t);
    bool focusHoveredSlider(const juce::MouseEvent&);
    Processor& processor;
    CompLook look;
    std::array<juce::Slider,arson::Detector> knobs;
    std::array<juce::Label,arson::Detector> labels;
    GraphCanvas graphCanvas{*this};
    PresetBrowser presetBrowser{*this};
    juce::Label hoverReadout;
    std::size_t hoveredParameter=arson::Threshold;
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
    juce::String currentPresetTitle{"Custom / restored"};
    bool alternateActive=false;
    bool compactMode=false,thresholdDragging=false,sidechainPanel=false;
    int expandedWidth=612,expandedHeight=500;
    std::array<arson::MeterFrame,400> history{};
    std::size_t head=0;
    float inputDb=-120,outputDb=-120,gr=0;
    double lastUiTickMs=0.0,lastTelemetryMs=0.0;
    std::size_t idleHistoryFrames=0;
    std::array<float,2> inChannelDb{{-120.0f,-120.0f}},outChannelDb{{-120.0f,-120.0f}};
    int currentChannels=2;
    juce::Rectangle<int> display;
    juce::Image grid,metalSurface; // cached full chassis and black glass LCD
    juce::Path transferCurve,transferShade;
    juce::Path inputHistory,outputHistory,reductionHistory,reductionArea;
    // Six cached horizontal gradient brushes: input, output, GR, transfer,
    // transfer glow and reduction-area fade. Built only on full-view resize.
    std::array<juce::ColourGradient,6> edgeFades;
    std::array<float,4> cachedCurveKey{{999,999,999,999}};
    float cachedCurveWidth=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
