#include "Editor.h"
#include "PresetIO.h"
#include "GraphPaint.h"
#include <cmath>
#include <algorithm>

namespace {
const juce::Colour bg{0xff101315}, panel{0xff14191c},
    line{0xff343c41}, text{0xffe3e6e5}, muted{0xff9ea6a7},
    accent{0xffdf5860}, accentBright{0xfff4797d}, green{0xff83cfa1},
    yellow{0xffe9c978};

float clamp01(float x) noexcept { return juce::jlimit(0.0f,1.0f,x); }
float dbFraction(float x) noexcept { return clamp01((x+60.0f)/60.0f); }
// Visual ballistics only: no effect on detector, gain or audio latency.
float approachVisual(float current,float target,double elapsedSeconds,
                     double riseSeconds,double fallSeconds) noexcept {
    const double tau=target>current ? riseSeconds : fallSeconds;
    const float alpha=static_cast<float>(1.0-std::exp(-elapsedSeconds/tau));
    return current+alpha*(target-current);
}
float graphY(const juce::Rectangle<float>& p,float db) noexcept {
    return p.getBottom()-dbFraction(db)*p.getHeight();
}
juce::String levelText(float v) {
    return v<=-100.0f ? juce::String("-inf") : juce::String(v,1);
}
}

// All painting belongs to JUCE's message thread. No GUI object or
// gradient/path/cache construction is reachable from processBlock.
CompLook::CompLook() {
    setColour(juce::Slider::textBoxTextColourId,text);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour{0xff111417});
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colour{0xff31383e});
    setColour(juce::ComboBox::backgroundColourId,juce::Colour{0xff181d22});
    setColour(juce::ComboBox::textColourId,text);
    setColour(juce::ComboBox::outlineColourId,juce::Colour{0xff374047});
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour{0xff161b20});
    setColour(juce::PopupMenu::textColourId,text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour{0xff602b32});
    setColour(juce::TextButton::buttonColourId,juce::Colour{0xff1a1f24});
    setColour(juce::TextButton::textColourOffId,text);
    setColour(juce::ToggleButton::textColourId,text);
    setColour(juce::ToggleButton::tickColourId,accent);
    setColour(juce::Label::textColourId,muted);
}
juce::Label* CompLook::createSliderTextBox(juce::Slider& s) {
    auto* label=juce::LookAndFeel_V4::createSliderTextBox(s);
    label->setFont(juce::FontOptions(12.0f));
    label->setColour(juce::Label::outlineColourId,juce::Colour{0xff323a40});
    label->setColour(juce::Label::textColourId,text);
    label->setColour(juce::Label::backgroundColourId,juce::Colour{0xff101419});
    label->setColour(juce::TextEditor::backgroundColourId,juce::Colour{0xff101419});
    label->setColour(juce::TextEditor::textColourId,text);
    label->setColour(juce::TextEditor::outlineColourId,line);
    label->setColour(juce::TextEditor::focusedOutlineColourId,accent);
    return label;
}
void CompLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                                 float pos,float start,float end,juce::Slider& slider) {
    const bool primary=slider.getName()=="Threshold";
    const float radiusScale=primary?0.47f:0.36f;
    const float r=std::max(12.0f,std::min(static_cast<float>(w)*radiusScale,
        static_cast<float>(h)*(primary?0.47f:0.36f)));
    const float cx=static_cast<float>(x)+w*0.5f,cy=static_cast<float>(y)+h*0.51f;
    g.setColour(juce::Colours::black.withAlpha(0.62f));
    g.fillEllipse(cx-r-3.0f,cy-r-1.0f,2*r+6.0f,2*r+6.0f);
    g.setColour(juce::Colour{0xff3e4147});
    g.fillEllipse(cx-r,cy-r,2*r,2*r);
    const float ringInset=primary?6.8f:5.0f;
    const float rr=r-ringInset;
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff363a40},
        cx-rr*0.65f,cy-rr*0.9f,juce::Colour{0xff0b0e11},
        cx+rr*0.85f,cy+rr*0.95f,true));
    g.fillEllipse(cx-rr,cy-rr,2*rr,2*rr);
    g.setColour(juce::Colour{0xff15191d}.withAlpha(0.8f));
    g.drawEllipse(cx-rr+2.0f,cy-rr+2.0f,2*rr-4.0f,2*rr-4.0f,1.2f);
    juce::Path inactive,active;
    const float arcR=r+1.6f;
    inactive.addCentredArc(cx,cy,arcR,arcR,0.0f,start,end,true);
    active.addCentredArc(cx,cy,arcR,arcR,0.0f,start,start+clamp01(pos)*(end-start),true);
    g.setColour(juce::Colour{0xff43494f});
    g.strokePath(inactive,juce::PathStrokeType(primary?4.8f:4.0f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(accent);
    g.strokePath(active,juce::PathStrokeType(primary?5.4f:4.3f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    const float theta=start+clamp01(pos)*(end-start);
    g.setColour(juce::Colour{0xfffff5f5});
    g.drawLine(cx+std::sin(theta)*rr*0.27f,cy-std::cos(theta)*rr*0.27f,
               cx+std::sin(theta)*rr*0.78f,cy-std::cos(theta)*rr*0.78f,
               primary?2.7f:2.1f);
    g.setColour(juce::Colour{0xff6d7278});
    g.fillEllipse(cx-1.5f,cy-1.5f,3.0f,3.0f);
}
void CompLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                 float pos,float,float,
                                 const juce::Slider::SliderStyle,juce::Slider&) {
    const float left=static_cast<float>(x)+9, right=static_cast<float>(x+w)-9;
    const float cy=static_cast<float>(y)+h*0.51f;
    g.setColour(juce::Colour{0xff3d444a});
    g.fillRoundedRectangle(left,cy-3,right-left,6,3);
    g.setColour(juce::Colour{0xffe33c49});
    const float end=juce::jlimit(left,right,pos);
    if(end>left)g.fillRoundedRectangle(left,cy-3,end-left,6,3);
    g.setColour(juce::Colour{0xffff7380});
    g.fillEllipse(end-5.0f,cy-5.0f,10.0f,10.0f);
}

void CompLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,
                                 bool highlighted,bool down) {
    if(button.getButtonText()!="Bypass") {
        juce::LookAndFeel_V4::drawToggleButton(g,button,highlighted,down);
        return;
    }
    const bool on=button.getToggleState();
    const auto box=button.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(on?juce::Colour{0xff3d2027}:juce::Colour{0xff1b2026});
    g.fillRoundedRectangle(box,5.0f);
    g.setColour(on?accent.withAlpha(0.8f):line);
    g.drawRoundedRectangle(box,5.0f,0.9f);
    const float cx=box.getX()+19.0f,cy=box.getCentreY();
    juce::Path ring;
    ring.addCentredArc(cx,cy,9.5f,9.5f,0.0f,0.63f,5.65f,true);
    g.setColour(on?accentBright:muted);
    g.strokePath(ring,juce::PathStrokeType(2.2f));
    g.drawLine(cx,cy-13.0f,cx,cy-1.0f,2.2f);
    g.setColour(on?accentBright:text);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(on?"BYPASS":"ACTIVE",33,0,box.getWidth()-35,
        button.getHeight(),juce::Justification::centredLeft);
}

void CompLook::drawTooltip(juce::Graphics& g,const juce::String& value,int w,int h) {
    const auto r=juce::Rectangle<float>(1.0f,1.0f,static_cast<float>(w-2),static_cast<float>(h-2));
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRoundedRectangle(r.translated(0.0f,1.0f),6.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff283037},0,0,
         juce::Colour{0xff15191d},static_cast<float>(w),static_cast<float>(h),false));
    g.fillRoundedRectangle(r,6.0f);
    g.setColour(juce::Colour{0xff5b6267});
    g.drawRoundedRectangle(r,6.0f,0.8f);
    g.setColour(accent.withAlpha(0.78f));
    g.fillRoundedRectangle(7.0f,8.0f,2.0f,static_cast<float>(std::max(4,h-16)),1.0f);
    g.setColour(text);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText(value,15,5,w-24,h-10,juce::Justification::centredLeft,3);
}

Editor::Editor(Processor& p):AudioProcessorEditor(p),processor(p),alternate(p.snapshot()) {
    setLookAndFeel(&look);
    setOpaque(true);
    const std::array<std::size_t,6> bars{{arson::Range,arson::Hold,arson::Mix,
        arson::Link,arson::Highpass,arson::Lowpass}};
    for(std::size_t i=0;i<knobs.size();++i) {
        const auto& spec=arson::specs[i];
        auto& k=knobs[i];
        k.setLookAndFeel(&look);
        k.setSliderStyle(std::find(bars.begin(),bars.end(),i)!=bars.end()
            ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,96,20);
        k.setTextValueSuffix(spec.unit);
        k.setNumDecimalPlacesToDisplay(i==arson::Attack || i==arson::Ratio ? 2 : 1);
        k.setDoubleClickReturnValue(true,spec.initial);
        k.setScrollWheelEnabled(false);
        k.setName(spec.name);
        k.setComponentID("param."+juce::String(spec.id));
        k.setTooltip(juce::String(spec.name)+": drag to change; double click resets; type exact values.");
        juce::String shortLabel=juce::String(spec.name).toUpperCase();
        if(i==arson::Highpass) shortLabel="HP";
        if(i==arson::Lowpass) shortLabel="LP";
        if(i==arson::Link) shortLabel="LINK";
        labels[i].setText(shortLabel,juce::dontSendNotification);
        labels[i].setTooltip(juce::String(spec.name)+": adjust the compressor's "+spec.name);
        labels[i].setFont(juce::FontOptions(11.0f));
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        addAndMakeVisible(k);
        attachments[i]=std::make_unique<SliderAttachment>(p.state,spec.id,k);
    }
    style.addItemList({"Clean - feedforward","Feedback - classic","Smooth - dual envelope"},1);
    style.setComponentID("view.model");
    style.setTooltip("Three independent zero-added-sample-latency compressor models.");
    addAndMakeVisible(style);
    styleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.state,"style",style);
    presets.addItemList({"Default","Vocal | gentle","Vocal | level",
        "Mix bus | glue","Drums | punch","Podcast | steady"},1);
    presets.setComponentID("view.presets");
    presets.setTooltip("Choose a factory starting point; threshold depends on input level.");
    presets.onChange=[this] {applyPreset(presets.getSelectedId());};
    addAndMakeVisible(presets);
    detector.addItemList({"Peak","RMS"},1);
    detector.setComponentID("view.detector");
    detector.setTooltip("Peak: transients; RMS: smoothed 10 ms energy, with zero lookahead.");
    addAndMakeVisible(detector);
    detectorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.state,"detector",detector);
    juce::ToggleButton* toggles[]{&autoRelease,&external,&listen,&bypass};
    const char* toggleIds[]{"autoRelease","external","listen","bypass"};
    for(std::size_t i=0;i<4;++i) {
        addAndMakeVisible(*toggles[i]);
        buttons[i]=std::make_unique<ButtonAttachment>(p.state,toggleIds[i],*toggles[i]);
    }
    autoRelease.setComponentID("view.auto");
    external.setComponentID("view.external");
    listen.setComponentID("view.listen");
    autoRelease.setButtonText("AUTO");
    external.setButtonText("EXT SC");
    listen.setButtonText("LISTEN");
    autoRelease.setTooltip("Auto Release - adapts recovery time to signal activity.");
    external.setTooltip("External Sidechain - routes the host's auxiliary detector bus.");
    listen.setTooltip("Listen Sidechain - audition the filtered detector signal.");
    compare.setTooltip("A/B snapshots apply to actual host parameters.");
    compare.onClick=[this] {
        const auto current=processor.snapshot();
        for(std::size_t i=0;i<arson::Count;++i) setValue(i,alternate[i]);
        alternate=current;alternateActive=!alternateActive;
        compare.setButtonText(alternateActive?"B / A":"A / B");
        presets.setSelectedId(0,juce::dontSendNotification);
        presets.setText("Custom",juce::dontSendNotification);
    };
    addAndMakeVisible(compare);
    savePreset.setComponentID("preset.save");
    loadPreset.setComponentID("preset.load");
    savePreset.setTooltip("Export all current parameters to a .arsoncomp user preset.");
    loadPreset.setTooltip("Open a validated versioned .arsoncomp user preset.");
    savePreset.onClick=[this] {
        presetChooser=std::make_unique<juce::FileChooser>(
            "Save ArSonComp preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("ArSonComp-Preset.arsoncomp"),"*.arsoncomp");
        juce::Component::SafePointer<Editor> safe(this);
        presetChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [safe](const juce::FileChooser& chooser) {
                if(auto* editor=safe.getComponent()) {
                    auto file=chooser.getResult();
                    if(file==juce::File{})return;
                    if(!file.hasFileExtension(".arsoncomp"))
                        file=file.withFileExtension(".arsoncomp");
                    if(!arson::preset::save(editor->processor,file))
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Save failed","Could not write the selected preset.");
                }
            });
    };
    loadPreset.onClick=[this] {
        presetChooser=std::make_unique<juce::FileChooser>(
            "Open ArSonComp preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.arsoncomp");
        juce::Component::SafePointer<Editor> safe(this);
        presetChooser->launchAsync(juce::FileBrowserComponent::openMode |
                juce::FileBrowserComponent::canSelectFiles,
            [safe](const juce::FileChooser& chooser) {
                if(auto* editor=safe.getComponent()) {
                    const auto file=chooser.getResult();
                    if(file==juce::File{})return;
                    if(!arson::preset::load(editor->processor,file)) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Preset rejected","This is not a compatible ArSonComp preset.");
                        return;
                    }
                    editor->presetSnapshot=editor->processor.snapshot();
                    editor->presets.setSelectedId(0,juce::dontSendNotification);
                    editor->presets.setText(file.getFileNameWithoutExtension(),juce::dontSendNotification);
                    editor->cachedCurveKey[0]=999;
                    editor->repaint();
                }
            });
    };
    addAndMakeVisible(savePreset);
    addAndMakeVisible(loadPreset);
    compact.setComponentID("view.compact");
    compact.setTooltip("MINI shows hardware controls and horizontal stereo meters.");
    compact.onClick=[this] {
        compactMode=!compactMode;
        if(compactMode) {
            expandedWidth=getWidth();expandedHeight=getHeight();
            setResizeLimits(860,380,1200,380);
            compact.setButtonText("FULL");
            setSize(expandedWidth,380);
        } else {
            setResizeLimits(860,700,1200,965);
            compact.setButtonText("MINI");
            setSize(expandedWidth,expandedHeight);
        }
    };
    addAndMakeVisible(compact);
    arson::MeterFrame discard;
    for(std::uint32_t i=0;i<arson::MeterQueue::capacity && p.meters.pop(discard);++i){}
    presets.setSelectedId(0,juce::dontSendNotification);
    presets.setText("Custom / restored",juce::dontSendNotification);
    setResizable(true,true);
    setResizeLimits(860,700,1200,965);
    setSize(900,765);
    startTimerHz(30);
}
Editor::~Editor() {
    stopTimer();
    presetChooser.reset();
    if(thresholdDragging)
        if(auto* parameter=processor.state.getParameter("threshold"))
            parameter->endChangeGesture();
    setLookAndFeel(nullptr);
}
void Editor::setValue(std::size_t i,float value) {
    if(auto* parameter=processor.state.getParameter(arson::specs[i].id)) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}
void Editor::applyPreset(int id) {
    if(id<1)return;
    arson::Parameters p;
    if(id==2) {p.value[arson::Ratio]=2.5f;p.value[arson::Attack]=15;
        p.value[arson::Release]=180;p.value[arson::Knee]=12;p.value[arson::Style]=2;}
    if(id==3) {p.value[arson::Threshold]=-24;p.value[arson::Ratio]=3;
        p.value[arson::Detector]=1;p.value[arson::AutoRelease]=1;
        p.value[arson::Highpass]=100;p.value[arson::Style]=1;}
    if(id==4) {p.value[arson::Ratio]=2;p.value[arson::Attack]=30;
        p.value[arson::Release]=250;p.value[arson::Knee]=12;
        p.value[arson::Range]=6;p.value[arson::Highpass]=80;p.value[arson::Style]=2;}
    if(id==5) {p.value[arson::Ratio]=5;p.value[arson::Attack]=25;
        p.value[arson::Release]=80;p.value[arson::Knee]=3;p.value[arson::Mix]=75;}
    if(id==6) {p.value[arson::Threshold]=-24;p.value[arson::Ratio]=4;
        p.value[arson::Attack]=5;p.value[arson::Release]=180;
        p.value[arson::Knee]=12;p.value[arson::Highpass]=100;
        p.value[arson::AutoRelease]=1;p.value[arson::Style]=2;}
    presetSnapshot=p;
    for(std::size_t i=0;i<arson::Count;++i)
        if(i!=arson::External && i!=arson::Listen && i!=arson::Bypass)setValue(i,p[i]);
}
juce::Rectangle<float> Editor::plotBounds() const noexcept {
    if(compactMode || display.isEmpty())return {};
    return {static_cast<float>(display.getX()+82),
        static_cast<float>(display.getY()+30),
        static_cast<float>(display.getWidth()-206),
        static_cast<float>(display.getHeight()-60)};
}
void Editor::resized() {
    const int w=getWidth(),h=getHeight();
    // Narrow portrait-weight layout: fixed utility budget + elastic control cells.
    // No label or toggle is allowed to overlap its neighbor at minimum size.
    compact.setBounds(166,23,47,28);
    style.setBounds(220,22,146,29);
    presets.setBounds(w-467,22,151,29);
    savePreset.setBounds(w-308,22,47,29);
    loadPreset.setBounds(w-257,22,47,29);
    compare.setBounds(w-206,22,64,29);
    bypass.setBounds(w-135,23,122,27);
    display=compactMode?juce::Rectangle<int>{}:
        juce::Rectangle<int>{8,73,w-16,h-333};
    const int mainY=compactMode?68:display.getBottom()+8;
    const int cell=(w-28)/7;
    constexpr std::array<std::size_t,7> main{arson::Input,arson::Attack,
        arson::Release,arson::Threshold,arson::Ratio,arson::Knee,arson::Makeup};
    for(std::size_t k=0;k<main.size();++k) {
        const auto i=main[k];const int x=14+static_cast<int>(k)*cell;
        labels[i].setBounds(x,mainY+3,cell,17);
        knobs[i].setBounds(x+3,mainY+20,cell-6,105);
    }
    const int bottomY=compactMode?202:h-119;
    const int small=(w-245)/6;
    constexpr std::array<std::size_t,6> secondary{arson::Range,arson::Hold,
        arson::Mix,arson::Link,arson::Highpass,arson::Lowpass};
    for(std::size_t k=0;k<secondary.size();++k) {
        const auto i=secondary[k];const int x=14+static_cast<int>(k)*small;
        labels[i].setBounds(x,bottomY+1,small,16);
        knobs[i].setBounds(x+6,bottomY+18,small-12,55);
    }
    const int opts=w-220;
    detector.setBounds(opts,bottomY+1,84,26);
    autoRelease.setBounds(opts+90,bottomY+1,112,26);
    external.setBounds(opts,bottomY+37,94,24);
    listen.setBounds(opts+102,bottomY+37,100,24);
    if(compactMode){grid=juce::Image();return;}

    // This cache owns only immutable decoration. Animated history paths are
    // separate message-thread caches rebuilt on new telemetry, not paint().
    cachedCurveWidth=-1;
    grid=juce::Image(juce::Image::RGB,display.getWidth(),display.getHeight(),true);
    juce::Graphics g(grid);
    const float width=static_cast<float>(display.getWidth());
    const float height=static_cast<float>(display.getHeight());
    g.setGradientFill(juce::ColourGradient(panel,0.0f,0.0f,bg,width,height,false));
    g.fillAll();
    const auto plot=plotBounds().translated(-static_cast<float>(display.getX()),
                                            -static_cast<float>(display.getY()));
    g.setColour(juce::Colour{0xff20262b}.withAlpha(0.34f));
    g.fillRect(plot);
    const float bottom=plot.getBottom(),left=plot.getX(),right=plot.getRight();
    for(int db=-60;db<=0;db+=6) {
        const float yy=graphY(plot,static_cast<float>(db));
        g.setColour(juce::Colour{0xff58616a}.withAlpha(db%12==0?0.18f:0.07f));
        g.drawHorizontalLine(static_cast<int>(yy),left,right);
        if(db%12==0) {
            g.setColour(muted.withAlpha(0.62f));
            g.setFont(juce::FontOptions(9.0f));
            g.drawText(juce::String(db),static_cast<int>(left)-37,
                static_cast<int>(yy)-8,31,17,juce::Justification::centredRight);
        }
    }
    for(int db=-60;db<=0;db+=10) {
        const float xx=left+(static_cast<float>(db+60)/60.0f)*plot.getWidth();
        g.setColour(juce::Colour{0xff64707a}.withAlpha(db%20==0?0.18f:0.08f));
        g.drawVerticalLine(static_cast<int>(xx),plot.getY(),bottom);
    }
    g.setColour(juce::Colour{0xff51585f}.withAlpha(0.52f));
    g.drawRect(plot,0.8f);
    g.setColour(juce::Colour{0xff454b50}.withAlpha(0.75f));
    g.drawVerticalLine(static_cast<int>(left)-45,18.0f,height-20.0f);
    g.drawVerticalLine(static_cast<int>(right)+12,18.0f,height-20.0f);
    // Fully reusable gradient stops; preserve transfer readability at
    // near-silent endpoints while moving traces dissolve to transparent.
    const float x0=plot.getX(),x1=plot.getRight();
    edgeFades[0]=arson::ui::makeEdgeFade(juce::Colours::white,x0,x1,0.19f);
    edgeFades[1]=arson::ui::makeEdgeFade(juce::Colour{0xffd8d9d8},x0,x1,0.52f);
    edgeFades[2]=arson::ui::makeEdgeFade(accentBright,x0,x1,0.91f);
    edgeFades[3]=arson::ui::makeEdgeFade(accentBright,x0,x1,0.97f,0.18f);
    edgeFades[4]=arson::ui::makeEdgeFade(accent,x0,x1,0.14f,0.10f);
    edgeFades[5]=arson::ui::makeEdgeFade(accent,x0,x1,0.105f);
    // Pre-rasterize inactive LED rails once with grid; runtime paints
    // active segments only, avoiding ~230 inactive rect draws per frame.
    arson::ui::paintMeterRail(g,{14.0f,plot.getY(),33.0f,plot.getHeight()},false);
    arson::ui::paintMeterRail(g,{width-110.0f,plot.getY(),34.0f,plot.getHeight()},true);
    arson::ui::paintMeterRail(g,{width-61.0f,plot.getY(),42.0f,plot.getHeight()},false);
    rebuildHistoryPaths();
}
void Editor::timerCallback() {
    if(!isShowing())return;
    if(presets.getSelectedId()>0) {
        const auto current=processor.snapshot();
        for(std::size_t i=0;i<arson::Count;++i)
            if(i!=arson::External && i!=arson::Listen && i!=arson::Bypass &&
               std::abs(current[i]-presetSnapshot[i])>0.001f) {
                presets.setSelectedId(0,juce::dontSendNotification);
                presets.setText("Custom",juce::dontSendNotification);
                break;
            }
    }
    arson::MeterFrame frame;
    float in=0,out=0,reduction=0;
    std::array<float,2> inPeaks{},outPeaks{};
    int received=0;
    // Coalesce the bounded SPSC queue exactly once per GUI tick.
    while(received<static_cast<int>(arson::MeterQueue::capacity) &&
          processor.meters.pop(frame)) {
        history[head]=frame;
        head=(head+1)%history.size();
        ++received;
        in=std::max(in,frame.input);
        out=std::max(out,frame.output);
        reduction=std::max(reduction,frame.reduction);
        currentChannels=frame.channels;
        for(std::size_t i=0;i<2;++i) {
            inPeaks[i]=std::max(inPeaks[i],frame.inputChannel[i]);
            outPeaks[i]=std::max(outPeaks[i],frame.outputChannel[i]);
        }
    }
    // Time-based visual smoothing: attack remains responsive (~18 ms) while
    // falloff is calm (~210 ms). No change to real detector or DSP latency.
    const double now=juce::Time::getMillisecondCounterHiRes();
    const double elapsed=lastUiTickMs>0.0 ?
        std::clamp((now-lastUiTickMs)*0.001,0.008,0.120):1.0/30.0;
    lastUiTickMs=now;
    inputDb=approachVisual(inputDb,arson::gainToDb(in),elapsed,0.018,0.210);
    outputDb=approachVisual(outputDb,arson::gainToDb(out),elapsed,0.018,0.210);
    gr=approachVisual(gr,reduction,elapsed,0.022,0.185);
    for(std::size_t i=0;i<2;++i) {
        inChannelDb[i]=approachVisual(inChannelDb[i],
            arson::gainToDb(inPeaks[i]),elapsed,0.018,0.210);
        outChannelDb[i]=approachVisual(outChannelDb[i],
            arson::gainToDb(outPeaks[i]),elapsed,0.018,0.210);
    }
    if(!compactMode && received>0)rebuildHistoryPaths();
    if(compactMode)repaint(14,298,getWidth()-28,57);
    else repaint(display);
}

void Editor::rebuildHistoryPaths() {
    inputHistory.clear();
    outputHistory.clear();
    reductionHistory.clear();
    reductionArea.clear();
    const auto plot=plotBounds().translated(-static_cast<float>(display.getX()),
                                            -static_cast<float>(display.getY()));
    if(plot.isEmpty())return;
    float inY=plot.getBottom(),outY=plot.getBottom(),grY=plot.getY();
    constexpr float inputEase=0.52f, outputEase=0.52f, grEase=0.42f;
    for(std::size_t i=0;i<history.size();++i) {
        const auto& f=history[(head+i)%history.size()];
        const float x=plot.getX()+plot.getWidth()*static_cast<float>(i)/
            static_cast<float>(history.size()-1);
        const float targetIn=graphY(plot,arson::gainToDb(f.input));
        const float targetOut=graphY(plot,arson::gainToDb(f.output));
        const float targetGr=plot.getY()+clamp01(f.reduction/30.0f)*plot.getHeight()*0.40f;
        if(i==0){inY=targetIn;outY=targetOut;grY=targetGr;}
        else {
            inY+=(targetIn-inY)*inputEase;
            outY+=(targetOut-outY)*outputEase;
            grY+=(targetGr-grY)*grEase;
        }
        if(i==0) {
            inputHistory.startNewSubPath(x,inY);
            outputHistory.startNewSubPath(x,outY);
            reductionHistory.startNewSubPath(x,grY);
            reductionArea.startNewSubPath(x,plot.getY());
            reductionArea.lineTo(x,grY);
        } else {
            inputHistory.lineTo(x,inY);
            outputHistory.lineTo(x,outY);
            reductionHistory.lineTo(x,grY);
            reductionArea.lineTo(x,grY);
        }
    }
    reductionArea.lineTo(plot.getRight(),plot.getY());
    reductionArea.closeSubPath();
}
void Editor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff202528},
        0.0f,0.0f,bg,0.0f,69.0f,false));
    g.fillRect(0,0,getWidth(),68);
    g.setColour(line.withAlpha(0.67f));
    g.drawHorizontalLine(68,0.0f,static_cast<float>(getWidth()));
    g.setFont(juce::FontOptions(20.5f));
    g.setColour(text);
    g.drawText("ArSon",16,11,81,29,juce::Justification::centredLeft);
    g.setColour(accent);
    g.drawText("Comp",89,11,71,29,juce::Justification::centredLeft);
    g.fillRoundedRectangle(16.0f,49.0f,42.0f,2.0f,1.0f);
    g.setColour(muted.withAlpha(0.86f));
    g.setFont(juce::FontOptions(8.7f));
    g.drawText("LIVE DYNAMICS",67,46,99,16,juce::Justification::centredLeft);
    g.drawText("MODEL",221,5,90,16,juce::Justification::left);
    g.drawText("PRESET",getWidth()-466,5,140,16,juce::Justification::left);
    const int mainY=compactMode?68:display.getBottom()+8;
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff1b2023},
        0.0f,static_cast<float>(mainY),bg,0.0f,static_cast<float>(mainY+141),false));
    g.fillRect(8,mainY,getWidth()-16,132);
    g.setColour(line.withAlpha(0.58f));
    g.drawHorizontalLine(mainY,12.0f,static_cast<float>(getWidth()-12));
    g.drawHorizontalLine(compactMode?198:getHeight()-123,
        12.0f,static_cast<float>(getWidth()-12));
    const int cell=(getWidth()-28)/7;
    for(int i=1;i<7;++i) {
        const float x=14.0f+static_cast<float>(i*cell);
        g.setColour(line.withAlpha(0.16f));
        g.drawVerticalLine(static_cast<int>(x),static_cast<float>(mainY+16),
            static_cast<float>(mainY+117));
    }
    if(!compactMode) {
        if(grid.isValid())g.drawImageAt(grid,display.getX(),display.getY());
        g.setColour(juce::Colour{0xff555d60}.withAlpha(0.62f));
        g.drawRoundedRectangle(display.toFloat(),5.0f,0.8f);
        drawDisplay(g);
    } else paintCompactMeters(g);
    g.setColour(muted.withAlpha(0.67f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("0 SAMPLES ADDED   /   VST3",14,getHeight()-19,250,15,
        juce::Justification::left);
    g.drawText("Drag / double-click reset / type exact values",
        getWidth()-313,getHeight()-19,299,15,juce::Justification::centredRight);
}
void Editor::refreshTransfer(const arson::Parameters& p,juce::Rectangle<float> plot) {
    const std::array<float,4> key{{p[arson::Threshold],p[arson::Ratio],
        p[arson::Knee],p[arson::Range]}};
    if(key==cachedCurveKey && plot.getWidth()==cachedCurveWidth)return;
    cachedCurveKey=key;
    cachedCurveWidth=plot.getWidth();
    transferCurve.clear();
    transferShade.clear();
    const int steps=std::max(100,static_cast<int>(plot.getWidth()));
    for(int i=0;i<=steps;++i) {
        const float db=-60.0f+60.0f*static_cast<float>(i)/steps;
        const float y=graphY(plot,db-arson::reduction(db,key[0],key[1],key[2],key[3]));
        const float x=plot.getX()+plot.getWidth()*static_cast<float>(i)/steps;
        if(i==0){transferCurve.startNewSubPath(x,y);transferShade.startNewSubPath(x,y);}
        else {transferCurve.lineTo(x,y);transferShade.lineTo(x,y);}
    }
    transferShade.lineTo(plot.getRight(),plot.getBottom());
    transferShade.closeSubPath();
}
void Editor::drawMeter(juce::Graphics& g,juce::Rectangle<float> bounds,
                       float dB1,float dB2,const juce::String& title,bool gainReduction) {
    const auto x=bounds.getX(),y=bounds.getY(),h=bounds.getHeight();
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(title,static_cast<int>(x)-3,static_cast<int>(y)-22,
        static_cast<int>(bounds.getWidth())+6,18,juce::Justification::centred);
    // Cached dark LED rails are already part of Editor::grid.
    // Draw only the illuminated pixels, no 46x redraw of dark segments.
    const int bars=gainReduction?1:2;
    const float bw=bars==1?13.0f:9.0f;
    const float spacing=3.0f;
    const float total=bw*bars+spacing*(bars-1);
    const float start=x+(bounds.getWidth()-total)*0.5f;
    constexpr int segments=46;
    const float segmentH=h/static_cast<float>(segments);
    for(int c=0;c<bars;++c) {
        const float db=(c==0?dB1:(currentChannels==1?-120.0f:dB2));
        const float fraction=gainReduction?clamp01(db/30.0f):dbFraction(db);
        const int active=juce::jlimit(0,segments,
            static_cast<int>(std::ceil(fraction*static_cast<float>(segments))));
        const float bx=start+c*(bw+spacing);
        for(int step=0;step<active;++step) {
            // GR grows downwards; input/output grow upwards.
            const float yy=gainReduction?y+step*segmentH:
                y+h-(step+1)*segmentH;
            juce::Colour colour=gainReduction?accent:
                (static_cast<float>(step+1)/segments>0.90f?
                    juce::Colour{0xffff5c63}:
                (static_cast<float>(step+1)/segments>0.71f?
                    yellow:green));
            g.setColour(colour);
            g.fillRect(bx,yy,bw,std::max(1.0f,segmentH-1.2f));
        }
    }
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(text);
    g.drawText(levelText(dB1),static_cast<int>(x)-10,static_cast<int>(y+h+6),
               static_cast<int>(bounds.getWidth())+20,18,juce::Justification::centred);
}
void Editor::drawDisplay(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(display);
    g.setOrigin(display.getPosition());
    const auto plot=plotBounds().translated(-static_cast<float>(display.getX()),
                                             -static_cast<float>(display.getY()));
    if(plot.isEmpty())return;
    const auto p=processor.snapshot();
    const float w=static_cast<float>(display.getWidth()),h=static_cast<float>(display.getHeight());
    refreshTransfer(p,plot);
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(muted);
    g.drawText("DYNAMICS  /  4.0 s",static_cast<int>(plot.getX())+7,8,185,17,
        juce::Justification::left);
    g.setColour(accent.withAlpha(0.82f));
    g.drawText("TRANSFER  /  DRAG THRESHOLD",static_cast<int>(plot.getRight())-225,8,219,17,
        juce::Justification::centredRight);

    // Graph and metering share a -60 to 0 dBFS scale. The transfer line is a
    // real response curve; the animated lines are real per-block telemetry.
    // The two data types never claim to share an x-axis: static transfer uses
    // input level on x, dynamic history uses time.
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.14f),
        plot.getX(),plot.getY()+plot.getHeight()*0.28f,
        accent.withAlpha(0.005f),plot.getX(),plot.getBottom(),false));
    g.fillPath(transferShade);
    g.setGradientFill(edgeFades[4]);
    g.strokePath(transferCurve,juce::PathStrokeType(6.0f));
    g.setGradientFill(edgeFades[3]);
    g.strokePath(transferCurve,juce::PathStrokeType(2.1f));

    const float mx=plot.getX()+dbFraction(p[arson::Threshold])*plot.getWidth();
    const float my=graphY(plot,p[arson::Threshold]-
        arson::reduction(p[arson::Threshold],p[arson::Threshold],
            p[arson::Ratio],p[arson::Knee],p[arson::Range]));
    g.setColour(accent.withAlpha(0.28f));
    g.drawVerticalLine(static_cast<int>(mx),plot.getY(),plot.getBottom());
    g.setColour(accent.withAlpha(0.24f));
    g.fillEllipse(mx-9,my-9,18,18);
    g.setColour(juce::Colours::white);
    g.fillEllipse(mx-4.0f,my-4.0f,8.0f,8.0f);

    // These 400-point paths are computed once when fresh telemetry arrives,
    // never rebuilt by repaint or a host redraw. All buffers live on GUI.
    g.setGradientFill(edgeFades[5]);
    g.fillPath(reductionArea);
    g.setGradientFill(edgeFades[0]);
    g.strokePath(inputHistory,juce::PathStrokeType(1.05f));
    g.setGradientFill(edgeFades[1]);
    g.strokePath(outputHistory,juce::PathStrokeType(1.1f));
    g.setGradientFill(edgeFades[2]);
    g.strokePath(reductionHistory,juce::PathStrokeType(1.55f));

    drawMeter(g,{14.0f,plot.getY(),33.0f,plot.getHeight()},
        inChannelDb[0],inChannelDb[1],"IN",false);
    drawMeter(g,{w-110.0f,plot.getY(),34.0f,plot.getHeight()},
        gr,gr,"GR",true);
    drawMeter(g,{w-61.0f,plot.getY(),42.0f,plot.getHeight()},
        outChannelDb[0],outChannelDb[1],"OUT",false);
    g.setColour(muted.withAlpha(0.88f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("-60 dB",static_cast<int>(plot.getX()),
        static_cast<int>(h)-27,60,15,juce::Justification::left);
    g.drawText("0 dBFS",static_cast<int>(plot.getRight())-60,
        static_cast<int>(h)-27,60,15,juce::Justification::right);
    if(p[arson::Listen]>0.5f || p[arson::Bypass]>0.5f) {
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(accentBright);
        g.drawText(p[arson::Bypass]>0.5f?"BYPASSED":"SIDECHAIN MONITOR",
            static_cast<int>(plot.getCentreX())-95,8,190,19,juce::Justification::centred);
    }
}
void Editor::paintCompactMeters(juce::Graphics& g) {
    g.setColour(juce::Colour{0xff151a1e});
    g.fillRoundedRectangle(13.0f,301.0f,static_cast<float>(getWidth()-26),49.0f,4.0f);
    const float values[]{inputDb,gr,outputDb};
    const char* labelsText[]{"INPUT","GAIN REDUCTION","OUTPUT"};
    const int segment=(getWidth()-42)/3;
    for(int i=0;i<3;++i) {
        const float x=20.0f+static_cast<float>(i*segment);
        g.setColour(muted);g.setFont(juce::FontOptions(10.0f));
        g.drawText(labelsText[i],static_cast<int>(x),304,109,16,juce::Justification::left);
        g.setColour(text);g.setFont(juce::FontOptions(11.0f));
        g.drawText(levelText(values[i])+" dB",static_cast<int>(x)+segment-85,304,78,16,
            juce::Justification::centredRight);
        g.setColour(line);
        const float bw=static_cast<float>(segment-16), y=330.0f;
        g.fillRoundedRectangle(x,y,bw,6.0f,3.0f);
        const float filled=i==1?clamp01(values[i]/30.0f):dbFraction(values[i]);
        g.setColour(i==1?accent:green);
        if(filled>0.0f)g.fillRoundedRectangle(x,y,bw*filled,6.0f,3.0f);
    }
}
void Editor::mouseDown(const juce::MouseEvent& event) {
    if(compactMode || !plotBounds().contains(event.position))return;
    const auto p=processor.snapshot();
    const auto plot=plotBounds();
    const float mx=plot.getX()+dbFraction(p[arson::Threshold])*plot.getWidth();
    const float my=graphY(plot,p[arson::Threshold]-
        arson::reduction(p[arson::Threshold],p[arson::Threshold],
            p[arson::Ratio],p[arson::Knee],p[arson::Range]));
    if(std::abs(event.position.x-mx)>16.0f ||
       std::abs(event.position.y-my)>18.0f)return;
    if(auto* parameter=processor.state.getParameter("threshold")) {
        thresholdDragging=true;
        parameter->beginChangeGesture();
        mouseDrag(event);
    }
}
void Editor::mouseDrag(const juce::MouseEvent& event) {
    if(!thresholdDragging)return;
    const auto plot=plotBounds();
    const float threshold=-60.0f+60.0f*clamp01(
        (event.position.x-plot.getX())/plot.getWidth());
    if(auto* parameter=processor.state.getParameter("threshold"))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(threshold));
    repaint(display);
}
void Editor::mouseUp(const juce::MouseEvent&) {
    if(!thresholdDragging)return;
    thresholdDragging=false;
    if(auto* parameter=processor.state.getParameter("threshold"))
        parameter->endChangeGesture();
    setMouseCursor(juce::MouseCursor::NormalCursor);
}
void Editor::mouseMove(const juce::MouseEvent& event) {
    if(compactMode || !plotBounds().contains(event.position)) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }
    const auto p=processor.snapshot();
    const auto plot=plotBounds();
    const float mx=plot.getX()+dbFraction(p[arson::Threshold])*plot.getWidth();
    const float my=graphY(plot,p[arson::Threshold]-
        arson::reduction(p[arson::Threshold],p[arson::Threshold],
            p[arson::Ratio],p[arson::Knee],p[arson::Range]));
    const bool active=std::abs(event.position.x-mx)<=16.0f &&
                      std::abs(event.position.y-my)<=18.0f;
    setMouseCursor(active?juce::MouseCursor::PointingHandCursor:
                          juce::MouseCursor::NormalCursor);
}
