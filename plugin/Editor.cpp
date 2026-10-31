#include "Editor.h"
#include "PresetIO.h"
#include "GraphPaint.h"
#include "SurfacePaint.h"
#include "GraphHistory.h"
#include "BinaryData.h"
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
    // Decode the single bundled brushed-metal photo on the message thread.
    // It is never decoded, resized or accessed by the audio callback.
    sourceFace=juce::ImageFileFormat::loadFrom(
        BinaryData::black_brushed_knob_jpg,
        static_cast<std::size_t>(BinaryData::black_brushed_knob_jpgSize));
    jassert(sourceFace.isValid());
    // Two light-weight, transparent gunmetal assets are decoded once,
    // exclusively on the GUI thread; no file I/O or decoding in paint().
    sliderTrack=juce::ImageFileFormat::loadFrom(
        BinaryData::gunmetal_slider_track_png,
        static_cast<std::size_t>(BinaryData::gunmetal_slider_track_pngSize));
    sliderThumb=juce::ImageFileFormat::loadFrom(
        BinaryData::gunmetal_slider_thumb_red_png,
        static_cast<std::size_t>(BinaryData::gunmetal_slider_thumb_red_pngSize));
    jassert(sliderTrack.isValid() && sliderThumb.isValid());
    if(sliderTrack.isValid()) {
        constexpr int cap=37;
        const int width=sliderTrack.getWidth(),height=sliderTrack.getHeight();
        sliderCapLeft=sliderTrack.getClippedImage({0,0,cap,height});
        sliderCentre=sliderTrack.getClippedImage({cap,0,width-2*cap,height});
        sliderCapRight=sliderTrack.getClippedImage({width-cap,0,cap,height});
    }
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
const juce::Image& CompLook::knobFace(int diameter,bool primary) {
    auto& slot=faces[primary?1:0];
    if(slot.pixels==diameter && slot.face.isValid())return slot.face;
    slot.pixels=diameter;
    slot.face=juce::Image(juce::Image::ARGB,diameter,diameter,true);
    if(sourceFace.isValid()) {
        // One-time circular alpha mask + high-quality scale at each dial size.
        // The compressed JPEG is bundled with the binary; JPEG pixels outside
        // the circular metal face never appear against the editor background.
        juce::Graphics g(slot.face);
        juce::Path circularClip;
        constexpr float inset=1.25f;
        const float size=static_cast<float>(diameter)-2.0f*inset;
        circularClip.addEllipse(inset,inset,size,size);
        g.reduceClipRegion(circularClip);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(sourceFace,0,0,diameter,diameter,
                          juce::RectanglePlacement::stretchToFit);
    }
    return slot.face;
}
void CompLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                                 float pos,float start,float end,juce::Slider& slider) {
    const bool primary=slider.getName()=="Threshold";
    const float r=std::max(13.0f,std::min(
        static_cast<float>(w)*(primary?0.400f:0.385f),
        static_cast<float>(h)*(primary?0.400f:0.385f)));
    const int pixels=juce::jmax(28,static_cast<int>(std::ceil(r*2.0f+7.0f)));
    const float cx=x+w*0.5f,cy=y+h*0.50f;
    const float angle=start+clamp01(pos)*(end-start);
    const float bodyRadius=pixels*0.5f;
    // Draw the level RING FIRST. Its radius is larger than the black metal
    // photograph, so the dial occludes the inner arc naturally, as a physical
    // knob would. Nothing gets painted on top of the metal except the dot.
    const float trackRadius=bodyRadius+2.2f;
    juce::Path inactive,active;
    inactive.addCentredArc(cx,cy,trackRadius,trackRadius,0.0f,start,end,true);
    active.addCentredArc(cx,cy,trackRadius,trackRadius,0.0f,start,angle,true);
    g.setColour(juce::Colour{0xff42474b}.withAlpha(0.92f));
    g.strokePath(inactive,juce::PathStrokeType(5.2f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(accentBright.withAlpha(0.98f));
    g.strokePath(active,juce::PathStrokeType(primary?5.2f:4.7f,
        juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    // Cached source face never gets recomputed during drag or telemetry paint.
    const auto& face=knobFace(pixels,primary);
    g.drawImageAt(face,juce::roundToInt(cx-bodyRadius),
                       juce::roundToInt(cy-bodyRadius));
    // Red positioning dot at 79% of the actual knob radius, near the rim.
    // A tiny dark bezel keeps it readable on specular metal highlights.
    const float dotRadius=bodyRadius*0.79f;
    const float dx=cx+std::sin(angle)*dotRadius;
    const float dy=cy-std::cos(angle)*dotRadius;
    g.setColour(juce::Colour{0xff0a0e11}.withAlpha(0.88f));
    g.fillEllipse(dx-4.0f,dy-4.0f,8.0f,8.0f);
    g.setColour(accentBright);
    g.fillEllipse(dx-2.75f,dy-2.75f,5.5f,5.5f);
    g.setColour(juce::Colours::white.withAlpha(0.40f));
    g.fillEllipse(dx-1.25f,dy-1.6f,1.5f,1.5f);
}
void CompLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
                                 float pos,float,float,
                                 const juce::Slider::SliderStyle,juce::Slider&) {
    const float cy=static_cast<float>(y)+static_cast<float>(h)*0.50f;
    const float plateX=static_cast<float>(x)+2.0f;
    const float plateW=static_cast<float>(w)-4.0f;
    constexpr float plateH=30.0f;
    // Photo plate is stationary. Nine-slice horizontally (3 segments)
    // preserves realistic endcaps at both the short and 170px SC sliders.
    if(sliderTrack.isValid()) {
        constexpr float dstCap=18.0f;
        const float top=cy-plateH*0.5f;
        const float midW=juce::jmax(0.0f,plateW-2.0f*dstCap);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        // JUCE 8's Graphics::drawImage takes a target Rectangle and optional
        // RectanglePlacement, NOT a source Rectangle. Preclip these three
        // views once in CompLook constructor, never inside paint().
        g.drawImage(sliderCapLeft,
            juce::Rectangle<float>{plateX,top,dstCap,plateH});
        g.drawImage(sliderCentre,
            juce::Rectangle<float>{plateX+dstCap,top,midW,plateH});
        g.drawImage(sliderCapRight,
            juce::Rectangle<float>{plateX+plateW-dstCap,top,dstCap,plateH});
    } else {
        g.setColour(juce::Colour{0xff282d31});
        g.fillRoundedRectangle(plateX,cy-14.0f,plateW,28.0f,7.0f);
    }

    // Use the same travel limits for the bright level and moving thumb.
    // The LED is INSIDE the dark photo slot, not a red slider drawn
    // over the brushed metal. At minimum it is genuinely extinguished.
    const float left=static_cast<float>(x)+21.0f;
    const float right=static_cast<float>(x+w)-21.0f;
    const float cx=juce::jlimit(left,right,pos);
    if(cx>left+0.3f) {
        juce::Graphics::ScopedSaveState scope(g);
        juce::Path slot;
        slot.addRoundedRectangle(left,cy-4.0f,right-left,8.0f,3.2f);
        g.reduceClipRegion(slot);
        g.setColour(juce::Colour{0xff922731}.withAlpha(0.40f));
        g.fillRoundedRectangle(left,cy-4.0f,cx-left,8.0f,3.5f);
        g.setGradientFill(juce::ColourGradient(
            juce::Colour{0xffff6774}.withAlpha(0.98f),left,cy-2.6f,
            juce::Colour{0xffa62038}.withAlpha(0.90f),left,cy+3.5f,false));
        g.fillRoundedRectangle(left,cy-2.8f,cx-left,5.6f,2.8f);
        g.setColour(juce::Colour{0xffff9197}.withAlpha(0.55f));
        g.fillRect(left,cy-0.7f,cx-left,1.4f);
    }
    // The thumb is the FRONT layer. Only its x position moves with the
    // JUCE slider value; the chassis, glass and audio thread never move.
    if(sliderThumb.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(sliderThumb,
            juce::Rectangle<float>{cx-9.0f,cy-18.5f,18.0f,37.0f});
    } else {
        g.setColour(juce::Colour{0xffa8afb1});
        g.fillRoundedRectangle(cx-8.0f,cy-17.0f,16.0f,34.0f,4.0f);
    }
}
void CompLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,
                                 bool highlighted,bool down) {
    const bool header=button.getButtonText()=="Bypass";
    const bool checked=button.getToggleState();
    const bool enabled=header?!checked:checked;
    const float trackW=header?33.0f:32.0f;
    const float trackH=header?16.0f:15.0f;
    // Caption LEFT of the switch in both normal and header layouts.
    const float tx=static_cast<float>(button.getWidth())-trackW-6.0f;
    const float ty=(static_cast<float>(button.getHeight())-trackH)*0.50f;
    const auto track=juce::Rectangle<float>{tx,ty,trackW,trackH};
    g.setColour(juce::Colours::black.withAlpha(0.29f));
    g.fillRoundedRectangle(track.translated(0.0f,0.9f),trackH*0.5f);
    g.setColour(enabled?juce::Colour{0xff592c37}:juce::Colour{0xff252b30});
    g.fillRoundedRectangle(track,trackH*0.5f);
    g.setColour(enabled?accent.withAlpha(0.72f):
                juce::Colour{0xff636b70}.withAlpha(0.76f));
    g.drawRoundedRectangle(track,trackH*0.5f,0.8f);
    const float knobR=trackH*0.5f-2.25f;
    const float cx=enabled?tx+trackW-trackH*0.5f:tx+trackH*0.5f;
    const float cy=ty+trackH*0.5f;
    g.setColour(enabled?juce::Colour{0xffff8c92}:juce::Colour{0xffb7bbbc});
    g.fillEllipse(cx-knobR,cy-knobR,knobR*2.0f,knobR*2.0f);
    const auto label=header?(enabled?"ACTIVE":"BYPASS"):button.getButtonText();
    g.setColour(enabled?juce::Colour{0xfff0efed}:juce::Colour{0xffc5c8c8});
    g.setFont(juce::FontOptions(header?10.0f:9.5f));
    g.drawText(label,0,0,juce::jmax(0,static_cast<int>(tx)-7),
        button.getHeight(),juce::Justification::centredRight,false);
    if(highlighted||down) {
        g.setColour(accent.withAlpha(down?0.32f:0.13f));
        g.drawRoundedRectangle(track.expanded(1.6f),trackH*0.5f,0.7f);
    }
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
        // A single bottom-of-LCD value readout replaces all popup overlays.
        k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        k.setPopupDisplayEnabled(false,false,nullptr);
        k.addMouseListener(this,false);
        k.setTextValueSuffix(spec.unit);
        k.setNumDecimalPlacesToDisplay(i==arson::Attack || i==arson::Ratio ? 2 : 1);
        k.setDoubleClickReturnValue(true,spec.initial);
        k.setScrollWheelEnabled(false);
        k.setName(spec.name);
        k.setComponentID("param."+juce::String(spec.id));
        k.onValueChange=[this,i] {
            if(hoveredParameter==i)updateHoverReadout(i);
        };
        juce::String shortLabel=juce::String(spec.name).toUpperCase();
        if(i==arson::Highpass) shortLabel="HP";
        if(i==arson::Lowpass) shortLabel="LP";
        if(i==arson::Link) shortLabel="LINK";
        labels[i].setText(shortLabel,juce::dontSendNotification);
        labels[i].setFont(juce::FontOptions(11.0f));
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        addAndMakeVisible(k);
        attachments[i]=std::make_unique<SliderAttachment>(p.state,spec.id,k);
    }
    style.addItemList({"Clean","Feedback","Smooth"},1);
    style.setComponentID("view.model");
    addAndMakeVisible(style);
    styleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.state,"style",style);
    presets.addItemList({"Default","Vocal | gentle","Vocal | level",
        "Mix bus | glue","Drums | punch","Podcast | steady"},1);
    presets.setComponentID("view.presets");
    presets.onChange=[this] {applyPreset(presets.getSelectedId());};
    addAndMakeVisible(presets);
    detector.addItemList({"Peak","RMS"},1);
    detector.setComponentID("view.detector");
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
    compare.setComponentID("view.compare");
    bypass.setComponentID("view.bypass");
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
    // Preset file operations stay accessible through one small FILE menu.
    // Legacy controls remain attached for host-independent smoke tests,
    // but are not painted in the crowded toolbar.
    addChildComponent(savePreset);
    addChildComponent(loadPreset);
    fileMenu.setComponentID("view.file");
    fileMenu.onClick=[this] {
        juce::PopupMenu menu;
        menu.addItem(1,"Save preset...");
        menu.addItem(2,"Open preset...");
        juce::Component::SafePointer<Editor> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&fileMenu),
            [safe](int item) {
                if(auto* editor=safe.getComponent()) {
                    if(item==1 && editor->savePreset.onClick)editor->savePreset.onClick();
                    else if(item==2 && editor->loadPreset.onClick)editor->loadPreset.onClick();
                }
            });
    };
    addAndMakeVisible(fileMenu);
    dynamicsTab.setComponentID("view.dynamics.tab");
    sidechainTab.setComponentID("view.sidechain.tab");
    dynamicsTab.onClick=[this]{sidechainPanel=false;syncPanelVisibility();};
    sidechainTab.onClick=[this]{sidechainPanel=true;syncPanelVisibility();};
    addAndMakeVisible(dynamicsTab);
    addAndMakeVisible(sidechainTab);
    compact.setComponentID("view.compact");
    compact.onClick=[this] {
        compactMode=!compactMode;
        if(compactMode) {
            expandedWidth=getWidth();expandedHeight=getHeight();
            setResizeLimits(740,330,1200,330);
            compact.setButtonText("FULL");
            setSize(expandedWidth,330);
        } else {
            setResizeLimits(740,585,1200,965);
            compact.setButtonText("MINI");
            setSize(expandedWidth,expandedHeight);
        }
    };
    addAndMakeVisible(compact);
    hoverReadout.setComponentID("view.value.readout");
    hoverReadout.setFont(juce::FontOptions(11.0f));
    hoverReadout.setColour(juce::Label::textColourId,accentBright);
    hoverReadout.setJustificationType(juce::Justification::centred);
    hoverReadout.setInterceptsMouseClicks(false,false);
    addAndMakeVisible(hoverReadout);
    updateHoverReadout(hoveredParameter);
    arson::MeterFrame discard;
    for(std::uint32_t i=0;i<arson::MeterQueue::capacity && p.meters.pop(discard);++i){}
    presets.setSelectedId(0,juce::dontSendNotification);
    presets.setText("Custom / restored",juce::dontSendNotification);
    setResizable(true,true);
    setResizeLimits(860,700,1200,965);
    setSize(780,640);
    syncPanelVisibility();
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
void Editor::syncPanelVisibility() {
    constexpr std::array<std::size_t,4> mainControls{
        arson::Range,arson::Hold,arson::Mix,arson::Link};
    constexpr std::array<std::size_t,2> sideControls{
        arson::Highpass,arson::Lowpass};
    for(auto i:mainControls) {
        knobs[i].setVisible(!sidechainPanel);
        labels[i].setVisible(!sidechainPanel);
    }
    for(auto i:sideControls) {
        knobs[i].setVisible(sidechainPanel);
        labels[i].setVisible(sidechainPanel);
    }
    autoRelease.setVisible(!sidechainPanel);
    detector.setVisible(sidechainPanel);
    external.setVisible(sidechainPanel);
    listen.setVisible(sidechainPanel);
    dynamicsTab.setColour(juce::TextButton::buttonColourId,
        sidechainPanel?juce::Colour{0xff1b2025}:juce::Colour{0xff413037});
    sidechainTab.setColour(juce::TextButton::buttonColourId,
        sidechainPanel?juce::Colour{0xff413037}:juce::Colour{0xff1b2025});
    repaint(0,compactMode?177:getHeight()-108,getWidth(),106);
}
void Editor::resized() {
    const int w=getWidth(),h=getHeight();
    // The utility menu replaces two permanent buttons. Presets and model
    // have measurable reserved space at both 860 and 900 pixel layouts.
    compact.setBounds(147,21,45,28);
    style.setBounds(198,20,125,30);
    presets.setBounds(333,20,w-580,30);
    compare.setBounds(w-237,20,49,30);
    fileMenu.setBounds(w-180,20,57,30);
    bypass.setBounds(w-115,21,105,28);
    // Compact responsive hardware chassis; LCD still has its own display.
    display=compactMode?juce::Rectangle<int>{}:
        juce::Rectangle<int>{8,70,w-16,h-268};
    const int mainY=compactMode?68:display.getBottom()+5;
    const int cell=(w-28)/7;
    constexpr std::array<std::size_t,7> main{arson::Input,arson::Attack,
        arson::Release,arson::Threshold,arson::Ratio,arson::Knee,arson::Makeup};
    for(std::size_t k=0;k<main.size();++k) {
        const auto i=main[k];const int x=14+static_cast<int>(k)*cell;
        labels[i].setBounds(x,mainY+3,cell,17);
        knobs[i].setBounds(x+3,mainY+20,cell-6,70);
    }

    const int bottomY=compactMode?166:h-92;
    dynamicsTab.setBounds(16,bottomY+1,92,23);
    sidechainTab.setBounds(114,bottomY+1,94,23);
    // Main tab: four comfortable controls, plus a dedicated AUTO control.
    const int slot=(w-190)/4;
    constexpr std::array<std::size_t,4> dynamics{arson::Range,arson::Hold,
        arson::Mix,arson::Link};
    for(std::size_t k=0;k<dynamics.size();++k) {
        const auto i=dynamics[k];const int x=16+static_cast<int>(k)*slot;
        labels[i].setBounds(x,bottomY+25,slot-10,15);
        knobs[i].setBounds(x+5,bottomY+38,slot-18,40);
    }
    autoRelease.setBounds(w-153,bottomY+42,139,28);

    // Sidechain tab: only detector-specific controls are painted.
    constexpr std::array<std::size_t,2> filters{arson::Highpass,arson::Lowpass};
    // Responsive five-column sidechain panel, no fixed overflow positions.
    const int available=w-32;
    const int filterSlot=juce::jlimit(136,174,available*23/100);
    const int detW=juce::jlimit(92,108,available*14/100);
    const int extW=juce::jlimit(108,118,available*17/100);
    const int listenW=juce::jlimit(105,118,available*16/100);
    const int filterStart=16;
    for(std::size_t k=0;k<filters.size();++k) {
        const auto i=filters[k];const int x=filterStart+static_cast<int>(k)*filterSlot;
        labels[i].setBounds(x,bottomY+25,filterSlot-8,15);
        knobs[i].setBounds(x+2,bottomY+38,filterSlot-8,40);
    }
    const int detX=filterStart+filterSlot*2+8;
    detector.setBounds(detX,bottomY+43,detW,27);
    external.setBounds(detX+detW+8,bottomY+42,extW,30);
    listen.setBounds(detX+detW+extW+16,bottomY+42,listenW,30);
    const int readoutW=juce::jmin(254,w-40);
    hoverReadout.setBounds(compactMode?w-readoutW-14:
        display.getCentreX()-readoutW/2,
        compactMode?h-21:display.getBottom()-27,readoutW,18);
    // Old file buttons are intentionally non-visible.
    savePreset.setBounds(0,0,0,0);
    loadPreset.setBounds(0,0,0,0);
    // Generate the brushed metal once on resize, not at 30Hz in paint().
    metalSurface=juce::Image(juce::Image::RGB,w,h,true);
    {
        juce::Graphics chassis(metalSurface);
        arson::ui::paintBrushedChassis(chassis,w,h);
        // MINI meter glass is a static bezel; paint only live bar data
        // above this cached surface during normal timer repaints.
        if(compactMode) arson::ui::paintInsetGlass(chassis,
            {13.0f,258.0f,static_cast<float>(w-26),47.0f});
    }
    if(compactMode){grid=juce::Image();return;}

    cachedCurveWidth=-1;
    grid=juce::Image(juce::Image::RGB,display.getWidth(),display.getHeight(),true);
    juce::Graphics g(grid);
    const float width=static_cast<float>(display.getWidth());
    const float height=static_cast<float>(display.getHeight());
    // Independent black glass LCD covers both graph and IN/GR/OUT meter
    // cavities. All glass effects are baked into this cached grid bitmap.
    arson::ui::paintInsetGlass(g,{0.0f,0.0f,width,height});
    const auto plot=plotBounds().translated(-static_cast<float>(display.getX()),
                                            -static_cast<float>(display.getY()));
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff070b0d},
        plot.getX(),plot.getY(),juce::Colour{0xff020506},
        plot.getRight(),plot.getBottom(),false));
    g.fillRect(plot);
    // A very faint diagonal reflection, not an opaque gray graph panel.
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::white.withAlpha(0.034f),plot.getX(),plot.getY(),
        juce::Colours::transparentWhite,plot.getCentreX(),plot.getBottom(),false));
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
    const float x0=plot.getX(),x1=plot.getRight();
    edgeFades[0]=arson::ui::makeEdgeFade(juce::Colours::white,x0,x1,0.19f);
    edgeFades[1]=arson::ui::makeEdgeFade(juce::Colour{0xffd8d9d8},x0,x1,0.52f);
    edgeFades[2]=arson::ui::makeEdgeFade(accentBright,x0,x1,0.91f);
    edgeFades[3]=arson::ui::makeEdgeFade(accentBright,x0,x1,0.97f,0.18f);
    edgeFades[4]=arson::ui::makeEdgeFade(accent,x0,x1,0.14f,0.10f);
    edgeFades[5]=arson::ui::makeEdgeFade(accent,x0,x1,0.105f);
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
    // A stopped host produces no meter frames. Without a bounded idle
    // drain, the last four seconds of trace remain frozen forever.
    // Advance at the original 100-Hz history rate using GUI time only.
    std::size_t aged=0;
    if(received>0) {
        lastTelemetryMs=now;
        idleHistoryFrames=0;
    } else if(lastTelemetryMs>0.0 && now-lastTelemetryMs>150.0) {
        aged=arson::ui::ageStoppedHistory(history,head,idleHistoryFrames,
            std::max(1,static_cast<int>(std::round(elapsed*100.0))));
    }
    if(!compactMode && (received>0 || aged>0))rebuildHistoryPaths();
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
    if(metalSurface.isValid())g.drawImageAt(metalSurface,0,0);
    else g.fillAll(juce::Colour{0xff454a4d});
    // Soft satin chassis divisions remain visible over fine brushed grain.
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRect(0,0,getWidth(),68);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawHorizontalLine(1,0.0f,static_cast<float>(getWidth()));
    g.setColour(juce::Colours::black.withAlpha(0.58f));
    g.drawHorizontalLine(67,0.0f,static_cast<float>(getWidth()));
    g.setFont(juce::FontOptions(17.5f));
    g.setColour(text);
    g.drawText("ArSon",16,11,73,28,juce::Justification::centredLeft);
    g.setColour(accent);
    g.drawText("Comp",84,11,61,28,juce::Justification::centredLeft);
    g.fillRoundedRectangle(17.0f,49.0f,37.0f,2.0f,1.0f);
    g.setColour(text.withAlpha(0.73f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("MODEL",198,5,84,13,juce::Justification::left);
    g.drawText("PRESET",333,5,88,13,juce::Justification::left);

    const int mainY=compactMode?68:display.getBottom()+5;
    const int bottomY=compactMode?166:getHeight()-92;
    g.setColour(juce::Colours::black.withAlpha(0.105f));
    g.fillRect(8,mainY,getWidth()-16,bottomY-mainY);
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.drawHorizontalLine(mainY,12.0f,static_cast<float>(getWidth()-12));
    g.drawHorizontalLine(bottomY,12.0f,static_cast<float>(getWidth()-12));
    g.setColour(juce::Colours::white.withAlpha(0.09f));
    g.drawHorizontalLine(mainY+1,12.0f,static_cast<float>(getWidth()-12));
    g.drawHorizontalLine(bottomY+1,12.0f,static_cast<float>(getWidth()-12));
    const int cell=(getWidth()-28)/7;
    for(int i=1;i<7;++i) {
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        const float x=14.0f+static_cast<float>(i*cell);
        g.drawVerticalLine(static_cast<int>(x),static_cast<float>(mainY+17),
                           static_cast<float>(mainY+90));
    }
    g.setColour(accent.withAlpha(0.68f));
    g.fillRoundedRectangle(sidechainPanel?115.0f:17.0f,
        static_cast<float>(bottomY+22),sidechainPanel?91.0f:88.0f,1.8f,1.0f);
    if(!compactMode) {
        if(grid.isValid())g.drawImageAt(grid,display.getX(),display.getY());
        drawDisplay(g);
    } else paintCompactMeters(g);
    g.setColour(text.withAlpha(0.66f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("0 SAMPLES ADDED / REALTIME DSP",
        14,getHeight()-18,232,14,juce::Justification::left);
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
    // The black LCD bezel and glass are baked into metalSurface on resize.
    const float values[]{inputDb,gr,outputDb};
    const char* labelsText[]{"INPUT","GAIN REDUCTION","OUTPUT"};
    const int segment=(getWidth()-42)/3;
    for(int i=0;i<3;++i) {
        const float x=20.0f+static_cast<float>(i*segment);
        g.setColour(muted);g.setFont(juce::FontOptions(10.0f));
        g.drawText(labelsText[i],static_cast<int>(x),261,109,16,juce::Justification::left);
        g.setColour(text);g.setFont(juce::FontOptions(11.0f));
        g.drawText(levelText(values[i])+" dB",static_cast<int>(x)+segment-85,261,78,16,
            juce::Justification::centredRight);
        g.setColour(line);
        const float bw=static_cast<float>(segment-16), y=289.0f;
        g.fillRoundedRectangle(x,y,bw,6.0f,3.0f);
        const float filled=i==1?clamp01(values[i]/30.0f):dbFraction(values[i]);
        g.setColour(i==1?accent:green);
        if(filled>0.0f)g.fillRoundedRectangle(x,y,bw*filled,6.0f,3.0f);
    }
}
void Editor::updateHoverReadout(std::size_t index) {
    if(index>=knobs.size())return;
    const auto& spec=arson::specs[index];
    const int digits=(index==arson::Attack || index==arson::Ratio)?2:
        ((index==arson::Highpass || index==arson::Lowpass ||
          index==arson::Mix || index==arson::Link || index==arson::Hold)?0:1);
    const juce::String value=juce::String(spec.name).toUpperCase()+"    "+
        juce::String(knobs[index].getValue(),digits)+juce::String(spec.unit);
    if(hoverReadout.getText()!=value)
        hoverReadout.setText(value,juce::dontSendNotification);
}
bool Editor::focusHoveredSlider(const juce::MouseEvent& event) {
    for(std::size_t i=0;i<knobs.size();++i) {
        if(event.eventComponent==&knobs[i]) {
            if(hoveredParameter!=i) {
                hoveredParameter=i;
                updateHoverReadout(i);
            }
            return true;
        }
    }
    return false;
}
void Editor::mouseEnter(const juce::MouseEvent& event) {
    focusHoveredSlider(event);
}
void Editor::mouseDown(const juce::MouseEvent& event) {
    if(focusHoveredSlider(event))return;
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
    if(focusHoveredSlider(event))return;
    if(!thresholdDragging)return;
    const auto plot=plotBounds();
    const float threshold=-60.0f+60.0f*clamp01(
        (event.position.x-plot.getX())/plot.getWidth());
    if(auto* parameter=processor.state.getParameter("threshold"))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(threshold));
    hoveredParameter=arson::Threshold;
    updateHoverReadout(hoveredParameter);
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
    if(focusHoveredSlider(event))return;
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
