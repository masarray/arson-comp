#include "Editor.h"
#include "PresetIO.h"
#include "FactoryPresetBank.h"
#include "GraphPaint.h"
#include "SurfacePaint.h"
#include "GraphHistory.h"
#include "HeaderLayout.h"
#include "FaderGeometry.h"
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
    // Decode state sprites once on the GUI thread, never from processBlock().
    toggleOn=juce::ImageFileFormat::loadFrom(
        BinaryData::arson_toggle_on_png,
        static_cast<std::size_t>(BinaryData::arson_toggle_on_pngSize));
    toggleOff=juce::ImageFileFormat::loadFrom(
        BinaryData::arson_toggle_off_png,
        static_cast<std::size_t>(BinaryData::arson_toggle_off_pngSize));
    jassert(toggleOn.isValid() && toggleOff.isValid());
    jassert(toggleOn.getBounds()==toggleOff.getBounds());
    // Two light-weight, transparent gunmetal assets are decoded once,
    // exclusively on the GUI thread; no file I/O or decoding in paint().
    sliderTrack=juce::ImageFileFormat::loadFrom(
        BinaryData::gunmetal_slider_track_png,
        static_cast<std::size_t>(BinaryData::gunmetal_slider_track_pngSize));
    sliderThumb=juce::ImageFileFormat::loadFrom(
        BinaryData::gunmetal_slider_thumb_red_png,
        static_cast<std::size_t>(BinaryData::gunmetal_slider_thumb_red_pngSize));
    jassert(sliderTrack.isValid() && sliderThumb.isValid());
    setColour(juce::Slider::textBoxTextColourId,text);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour{0xff111417});
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colour{0xff31383e});
    setColour(juce::ComboBox::backgroundColourId,juce::Colour{0xff181d22});
    setColour(juce::ComboBox::textColourId,text);
    setColour(juce::ComboBox::outlineColourId,juce::Colour{0xff374047});
    setColour(juce::ComboBox::arrowColourId,juce::Colour{0xffb0b8bb});
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour{0xff161b20});
    setColour(juce::PopupMenu::textColourId,text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour{0xff602b32});
    setColour(juce::TextButton::buttonColourId,juce::Colour{0xff1a1f24});
    setColour(juce::TextButton::textColourOffId,text);
    setColour(juce::ToggleButton::textColourId,text);
    setColour(juce::ToggleButton::tickColourId,accent);
    // Metal panel text uses a high-contrast neutral; muted gray on metal
    // was unreadable in real 780px screenshots.
    setColour(juce::Label::textColourId,
        juce::Colour{arson::ui::EditorMetrics::controlCaptionRgb});
}
// Satin-finished combo surfaces: recessed but legible on a brushed chassis.
juce::Font CompLook::getTextButtonFont(juce::TextButton&,int) {
    return juce::Font(juce::FontOptions(arson::ui::EditorMetrics::buttonFont));
}
void CompLook::drawComboBox(juce::Graphics& g,int width,int height,bool down,
                            int buttonX,int buttonY,int buttonW,int buttonH,
                            juce::ComboBox& box) {
    juce::ignoreUnused(buttonX,buttonY,buttonW,buttonH);
    auto r=box.getLocalBounds().toFloat().reduced(0.5f);
    const bool hovered=box.isMouseOverOrDragging();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour{0xff262e32},0.0f,0.0f,
        juce::Colour{0xff151a1e},0.0f,static_cast<float>(height),false));
    g.fillRoundedRectangle(r,4.5f);
    g.setColour(down?juce::Colour{0xffa95b60}:
        hovered?juce::Colour{0xff78858a}:juce::Colour{0xff3c474c});
    g.drawRoundedRectangle(r,4.5f,1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(2,5.0f,static_cast<float>(width-5));
    // One discreet chevron; long preset titles receive the remaining width.
    juce::Path caret;
    const float cx=static_cast<float>(width)-15.0f;
    const float cy=static_cast<float>(height)*0.50f;
    caret.startNewSubPath(cx-3.5f,cy-1.5f);
    caret.lineTo(cx,cy+2.0f);
    caret.lineTo(cx+3.5f,cy-1.5f);
    g.setColour(hovered?juce::Colour{0xffe0e5e5}:juce::Colour{0xffa2afb1});
    g.strokePath(caret,juce::PathStrokeType(1.5f));
}
void CompLook::positionComboBoxText(juce::ComboBox& box,juce::Label& label) {
    label.setBounds(11,1,juce::jmax(0,box.getWidth()-36),box.getHeight()-2);
    label.setFont(juce::FontOptions(arson::ui::EditorMetrics::buttonFont));
    label.setJustificationType(juce::Justification::centredLeft);
}
void CompLook::drawButtonBackground(juce::Graphics& g,juce::Button& button,
                                   const juce::Colour& background,
                                   bool hovered,bool down) {
    const auto id=button.getComponentID();
    if(id!="view.compact" && id!="view.compare" && id!="view.file") {
        juce::LookAndFeel_V4::drawButtonBackground(g,button,background,hovered,down);
        return;
    }
    auto r=button.getLocalBounds().toFloat().reduced(0.5f);
    g.setGradientFill(juce::ColourGradient(
        down?juce::Colour{0xff30373b}:juce::Colour{0xff293035},0.0f,0.0f,
        juce::Colour{0xff151b1e},0.0f,r.getHeight(),false));
    g.fillRoundedRectangle(r,4.5f);
    g.setColour(down?juce::Colour{0xffa95b60}:
        hovered?juce::Colour{0xff7b8588}:juce::Colour{0xff485156});
    g.drawRoundedRectangle(r,4.5f,1.0f);
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
    // Draw the *whole* 340x57 track and the 58x124 thumb at the SAME
    // proportional scale. A resized window changes size, not aspect ratio.
    // All geometry is float arithmetic; there is no allocation/cache churn.
    const auto geometry=arson::ui::FaderGeometry::calculate(
        static_cast<float>(w),static_cast<float>(h),pos);
    const auto track=geometry.track.translated(static_cast<float>(x),
                                               static_cast<float>(y));
    const auto thumb=geometry.thumb.translated(static_cast<float>(x),
                                               static_cast<float>(y));
    const float cy=static_cast<float>(y)+static_cast<float>(h)*0.5f;
    const float left=geometry.left+static_cast<float>(x);
    const float right=geometry.right+static_cast<float>(x);
    const float position=geometry.position+static_cast<float>(x);
    // The native track art is intentionally ~6:1. At an 85px control
    // it becomes just 14px tall. A bounded *independent* metal housing
    // restores the physical fader's depth without stretching either PNG.
    // All geometry stays within the component; no image/cache allocation.
    const float housingH=juce::jmin(23.0f,static_cast<float>(h)-6.0f);
    const auto housing=juce::Rectangle<float>{
        track.getX()-3.0f,cy-housingH*0.5f,track.getWidth()+6.0f,housingH};
    g.setColour(juce::Colour{0xff262c31});
    g.fillRoundedRectangle(housing,4.5f);
    g.setColour(juce::Colour{0xff717a80}.withAlpha(0.42f));
    g.drawRoundedRectangle(housing.reduced(0.5f),4.5f,0.9f);
    g.setColour(juce::Colours::white.withAlpha(0.085f));
    g.drawHorizontalLine(juce::roundToInt(housing.getY()+2.5f),
        housing.getX()+5.0f,housing.getRight()-5.0f);
    if(sliderTrack.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(sliderTrack,track);
    } else {
        g.setColour(juce::Colour{0xff292f33});
        g.fillRoundedRectangle(track,track.getHeight()*0.35f);
    }
    // Recessed LED level remains clipped inside the proportional track
    // and always uses the same thumb travel. Minimum has no red fill.
    if(position>left+0.3f) {
        juce::Graphics::ScopedSaveState scope(g);
        const float ledH=juce::jmax(3.0f,track.getHeight()*0.29f);
        const float ledY=cy-ledH*0.5f;
        juce::Path recess;
        recess.addRoundedRectangle(left,ledY,right-left,ledH,ledH*0.45f);
        g.reduceClipRegion(recess);
        g.setGradientFill(juce::ColourGradient(
            juce::Colour{0xffff6b79},left,ledY,
            juce::Colour{0xff9e2433},left,ledY+ledH,false));
        g.fillRect(left,ledY,position-left,ledH);
        g.setColour(juce::Colour{0xffffb0b6}.withAlpha(0.46f));
        g.fillRect(left,cy-0.55f,position-left,1.1f);
    }
    // A proportional photographic thumb is too dark against the new chassis
    // at 580-612px. Give it a bright metal silhouette and a tactile centre
    // marker UNDER/OVER the original asset (never stretch or replace it).
    // This makes the current value visible even when the LED fill is zero.
    const auto thumbBack=thumb.expanded(1.2f,0.4f);
    g.setColour(juce::Colour{0xffabb6bb});
    g.fillRoundedRectangle(thumbBack,2.5f);
    g.setColour(juce::Colour{0xff11181c});
    g.fillRoundedRectangle(thumbBack.reduced(1.1f),2.0f);
    if(sliderThumb.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(sliderThumb,thumb);
    } else {
        g.setColour(juce::Colour{0xff929fa5});
        g.fillRoundedRectangle(thumb,2.5f);
    }
    // Draw a narrow light-catching ridge over the image. The centre marker
    // must move with thumb at both extremes and at all supported widths.
    const float ridgeX=thumb.getCentreX();
    g.setColour(juce::Colour{0xffc8d0d2}.withAlpha(0.9f));
    g.drawVerticalLine(juce::roundToInt(ridgeX),thumb.getY()+3.0f,
                       thumb.getBottom()-3.0f);
    g.setColour(juce::Colour{0xff5f6a70}.withAlpha(0.8f));
    g.drawVerticalLine(juce::roundToInt(ridgeX+2.0f),
                       thumb.getY()+4.0f,thumb.getBottom()-4.0f);
}
void CompLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,
                                 bool highlighted,bool down) {
    const bool bypassControl=button.getButtonText()=="Bypass";
    // In the DSP, bypass=1 means processing OFF. All other controls use
    // the natural mapping: parameter=1 means this feature is ON.
    const bool active=bypassControl?!button.getToggleState():button.getToggleState();
    constexpr float trackW=56.0f,trackH=23.0f;
    const float tx=static_cast<float>(button.getWidth())-trackW-4.0f;
    const float ty=(static_cast<float>(button.getHeight())-trackH)*0.5f;
    const auto track=juce::Rectangle<float>{tx,ty,trackW,trackH};
    const auto& sprite=active?toggleOn:toggleOff;
    if(sprite.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(sprite,track);
    } else {
        // Keep all four toggles legible even if binary data is missing.
        g.setColour(active?juce::Colour{0xff4aaf61}:juce::Colour{0xffa83439});
        g.fillRoundedRectangle(track,trackH*0.5f);
    }
    // The sprite already says ON/OFF; the adjacent caption identifies
    // the affected compressor function without repeating that state.
    const juce::String caption=bypassControl
        ? (active?"Active":"Bypass") : button.getButtonText();
    g.setColour(juce::Colour{0xffe3e6e5});
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(caption,0,0,juce::jmax(0,static_cast<int>(tx)-5),
        button.getHeight(),juce::Justification::centredRight,false);
    if(highlighted||down) {
        g.setColour(juce::Colours::white.withAlpha(down?0.28f:0.10f));
        g.drawRoundedRectangle(track.reduced(1.0f),trackH*0.5f,0.7f);
    }
}
Editor::Editor(Processor& p):AudioProcessorEditor(p),processor(p),alternate(p.snapshot()) {
    setLookAndFeel(&look);
    setOpaque(true);
    // Crisp vector-font wordmark replaces a low-contrast bitmap in the
    // tight 88px header; photo knob/slider assets remain predecoded.
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
            if(i==arson::Threshold || i==arson::Ratio ||
               i==arson::Knee || i==arson::Range)
                graphCanvas.repaint();
        };
        juce::String shortLabel=juce::String(spec.name).toUpperCase();
        if(i==arson::Highpass) shortLabel="HP";
        if(i==arson::Lowpass) shortLabel="LP";
        if(i==arson::Link) shortLabel="LINK";
        labels[i].setText(shortLabel,juce::dontSendNotification);
        labels[i].setComponentID("caption."+juce::String(spec.id));
        labels[i].setFont(juce::FontOptions(arson::ui::EditorMetrics::controlCaptionFont));
        labels[i].setColour(juce::Label::textColourId,
            juce::Colour{arson::ui::EditorMetrics::controlCaptionRgb});
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
    // The header stays small; factory bank is ordered into real categories.
    // Search / favorites / details live in a transient overlay, not in toolbar.
    presets.setComponentID("view.presets");
    presets.addItem("SEARCH / BROWSE PRESETS...",1000);
    const auto& bank=arson::preset::factoryBank();
    juce::String lastCategory;
    for(std::size_t i=0;i<bank.size();++i) {
        const juce::String cat(bank[i].category);
        if(cat!=lastCategory) {
            presets.addSectionHeading(cat.toUpperCase());
            lastCategory=cat;
        }
        presets.addItem(bank[i].name,static_cast<int>(i)+1);
    }
    presets.onChange=[this] {
        const int id=presets.getSelectedId();
        if(id==1000) {
            // Opening and dismissing a browser must NOT lose the currently
            // loaded preset title (or a user-restored custom name).
            presets.setSelectedId(0,juce::dontSendNotification);
            presets.setText(currentPresetTitle,juce::dontSendNotification);
            // Open after the native combo popup has completed its selection
            // event, so the host cannot immediately reclaim focus/z-order.
            // When rendered offscreen (JUCE smoke/headless host), no
            // desktop popup exists to dismiss; open synchronously. Real
            // visible hosts keep the deferred call to avoid popup z-order.
            if(!isShowing()) {
                openPresetBrowser();
            } else {
                juce::Component::SafePointer<Editor> safe(this);
                juce::MessageManager::callAsync([safe] {
                    if(auto* editor=safe.getComponent())
                        editor->openPresetBrowser();
                });
            }
        } else if(id>0) applyPreset(id);
    };
    addAndMakeVisible(presets);
    presetBrowser.setComponentID("view.preset.browser");
    addChildComponent(presetBrowser);
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
        currentPresetTitle="Custom";
        presets.setSelectedId(0,juce::dontSendNotification);
        presets.setText(currentPresetTitle,juce::dontSendNotification);
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
                    editor->currentPresetTitle=file.getFileNameWithoutExtension();
                    editor->presets.setText(editor->currentPresetTitle,juce::dontSendNotification);
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
        menu.addSeparator();
        menu.addItem(3,"Browse factory bank...");
        juce::Component::SafePointer<Editor> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&fileMenu),
            [safe](int item) {
                if(auto* editor=safe.getComponent()) {
                    if(item==1 && editor->savePreset.onClick)editor->savePreset.onClick();
                    else if(item==2 && editor->loadPreset.onClick)editor->loadPreset.onClick();
                    else if(item==3)editor->openPresetBrowser();
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
    compact.setButtonText("Mini");
    fileMenu.setButtonText("File");
    dynamicsTab.setButtonText("Dynamics");
    sidechainTab.setButtonText("Sidechain");
    compact.setComponentID("view.compact");
    compact.onClick=[this] {
        compactMode=!compactMode;
        if(compactMode) {
            expandedWidth=getWidth();expandedHeight=getHeight();
            setResizeLimits(arson::ui::EditorMetrics::minWidth,
                arson::ui::EditorMetrics::miniHeight,
                arson::ui::EditorMetrics::maxWidth,
                arson::ui::EditorMetrics::miniHeight);
            compact.setButtonText("Full");
            setSize(expandedWidth,arson::ui::EditorMetrics::miniHeight);
        } else {
            setResizeLimits(arson::ui::EditorMetrics::minWidth,
                arson::ui::EditorMetrics::minHeight,
                arson::ui::EditorMetrics::maxWidth,
                arson::ui::EditorMetrics::maxHeight);
            compact.setButtonText("Mini");
            setSize(expandedWidth,expandedHeight);
        }
    };
    addAndMakeVisible(compact);
    addAndMakeVisible(graphCanvas);
    hoverReadout.setComponentID("view.value.readout");
    hoverReadout.setFont(juce::FontOptions(11.0f));
    hoverReadout.setColour(juce::Label::textColourId,accentBright);
    hoverReadout.setJustificationType(juce::Justification::centred);
    hoverReadout.setInterceptsMouseClicks(false,false);
    addAndMakeVisible(hoverReadout);
    updateHoverReadout(hoveredParameter);
    arson::MeterFrame discard;
    for(std::uint32_t i=0;i<arson::MeterQueue::capacity && p.meters.pop(discard);++i){}
    presetSnapshot=processor.snapshot();
    presets.setSelectedId(0,juce::dontSendNotification);
    presets.setText(currentPresetTitle,juce::dontSendNotification);
    setResizable(true,true);
    setResizeLimits(arson::ui::EditorMetrics::minWidth,
        arson::ui::EditorMetrics::minHeight,
        arson::ui::EditorMetrics::maxWidth,
        arson::ui::EditorMetrics::maxHeight);
    setSize(arson::ui::EditorMetrics::defaultWidth,
            arson::ui::EditorMetrics::defaultHeight);
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
void Editor::GraphCanvas::paint(juce::Graphics& g) {
    if(owner.compactMode) {
        if(owner.metalSurface.isValid())
            g.drawImageAt(owner.metalSurface,-getX(),-getY());
        else g.fillAll(juce::Colour{0xff0b1014});
        juce::Graphics::ScopedSaveState saved(g);
        g.setOrigin(-getX(),-getY());
        owner.paintCompactMeters(g);
    } else {
        if(owner.grid.isValid())g.drawImageAt(owner.grid,0,0);
        else g.fillAll(juce::Colour{0xff070b0d});
        owner.drawDisplay(g);
    }
}
void Editor::GraphCanvas::mouseDown(const juce::MouseEvent& e) {
    owner.mouseDown(e.getEventRelativeTo(&owner));
}
void Editor::GraphCanvas::mouseDrag(const juce::MouseEvent& e) {
    owner.mouseDrag(e.getEventRelativeTo(&owner));
}
void Editor::GraphCanvas::mouseUp(const juce::MouseEvent& e) {
    owner.mouseUp(e.getEventRelativeTo(&owner));
}
void Editor::GraphCanvas::mouseMove(const juce::MouseEvent& e) {
    owner.mouseMove(e.getEventRelativeTo(&owner));
    setMouseCursor(owner.getMouseCursor());
}
Editor::PresetBrowser::PresetBrowser(Editor& editor):owner(editor) {
    setOpaque(true);
    heading.setText("PRESET LIBRARY",juce::dontSendNotification);
    heading.setFont(juce::FontOptions(14.0f));
    heading.setColour(juce::Label::textColourId,juce::Colour{0xffe5eaee});
    addAndMakeVisible(heading);
    search.setMultiLine(false);
    search.setTextToShowWhenEmpty("Search names, categories, tags, descriptions...",
                                  juce::Colour{0xffaab6bb});
    search.setColour(juce::TextEditor::backgroundColourId,juce::Colour{0xff182027});
    search.setColour(juce::TextEditor::textColourId,juce::Colours::white);
    search.setColour(juce::TextEditor::outlineColourId,juce::Colour{0xff536068});
    search.onTextChange=[this] {rebuildFilter();};
    search.addKeyListener(this);
    addAndMakeVisible(search);
    resultList.setModel(this);
    resultList.setRowHeight(28);
    resultList.setColour(juce::ListBox::backgroundColourId,juce::Colour{0xff11191e});
    resultList.addKeyListener(this);
    addAndMakeVisible(resultList);
    detail.setMultiLine(true);
    detail.setReadOnly(true);
    detail.setCaretVisible(false);
    detail.setScrollbarsShown(false);
    detail.setColour(juce::TextEditor::backgroundColourId,juce::Colour{0xff1c252a});
    detail.setColour(juce::TextEditor::textColourId,juce::Colour{0xffe2e8eb});
    detail.setColour(juce::TextEditor::outlineColourId,juce::Colour{0xff35424a});
    addAndMakeVisible(detail);
    subheading.setFont(juce::FontOptions(10.0f));
    subheading.setColour(juce::Label::textColourId,juce::Colour{0xffa4b0b6});
    addAndMakeVisible(subheading);
    for(auto* button:{&favoritesOnly,&star,&use,&close})addAndMakeVisible(*button);
    favorites.assign(arson::preset::factoryBank().size(),false);
    const auto file=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ArSonAudio/ArSonComp/favorites.txt");
    if(file.existsAsFile()) {
        juce::StringArray lines;
        lines.addLines(file.loadFileAsString());
        for(const auto& id:lines) {
            const int index=arson::preset::findFactoryId(id.toStdString());
            if(index>=0)favorites[static_cast<std::size_t>(index)]=true;
        }
    }
    favoritesOnly.onClick=[this] {
        showOnlyFavorites=!showOnlyFavorites;
        favoritesOnly.setButtonText(showOnlyFavorites?"ALL":"FAVORITES");
        rebuildFilter();
    };
    star.onClick=[this] {
        const int row=resultList.getSelectedRow();
        if(row<0 || static_cast<std::size_t>(row)>=matches.size())return;
        const auto i=static_cast<std::size_t>(matches[static_cast<std::size_t>(row)]);
        favorites[i]=!favorites[i];
        persistFavorites();
        if(showOnlyFavorites && !favorites[i])rebuildFilter();
        else {resultList.repaint();showDetails();}
    };
    use.onClick=[this]{applySelected(false);};
    close.onClick=[this]{setVisible(false);};
    rebuildFilter();
}
Editor::PresetBrowser::~PresetBrowser(){resultList.setModel(nullptr);}
void Editor::PresetBrowser::open() {
    search.clear();
    showOnlyFavorites=false;
    favoritesOnly.setButtonText("FAVORITES");
    rebuildFilter();
    setVisible(true);
    toFront(true);
    search.grabKeyboardFocus();
}
void Editor::PresetBrowser::persistFavorites() const {
    const auto file=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ArSonAudio/ArSonComp/favorites.txt");
    if(file.getParentDirectory().createDirectory().failed())return;
    juce::String data;
    const auto& bank=arson::preset::factoryBank();
    for(std::size_t i=0;i<bank.size();++i)
        if(favorites[i])data+=juce::String(bank[i].id)+"\n";
    file.replaceWithText(data);
}
void Editor::PresetBrowser::rebuildFilter(){
    matches.clear();
    const auto query=search.getText().trim().toLowerCase();
    const auto& bank=arson::preset::factoryBank();
    for(std::size_t i=0;i<bank.size();++i) {
        if(showOnlyFavorites && !favorites[i])continue;
        const auto& p=bank[i];
        const auto haystack=(juce::String(p.category)+" "+p.name+" "+p.tags+
                              " "+p.description).toLowerCase();
        if(query.isEmpty()||haystack.contains(query))matches.push_back(static_cast<int>(i));
    }
    resultList.updateContent();
    if(!matches.empty())resultList.selectRow(0);
    else resultList.deselectAllRows();
    subheading.setText(juce::String(matches.size())+" of "+
        juce::String(bank.size())+" presets",juce::dontSendNotification);
    showDetails();
}
int Editor::PresetBrowser::getNumRows(){return static_cast<int>(matches.size());}
void Editor::PresetBrowser::paintListBoxItem(int row,juce::Graphics& g,
                                             int width,int height,bool selected) {
    if(row<0||static_cast<std::size_t>(row)>=matches.size())return;
    const auto index=static_cast<std::size_t>(matches[static_cast<std::size_t>(row)]);
    const auto& p=arson::preset::factoryBank()[index];
    if(selected)g.fillAll(juce::Colour{0xff344b56});
    else if(row%2==0)g.fillAll(juce::Colour{0xff172025});
    g.setFont(juce::FontOptions(10.5f));
    g.setColour(juce::Colour{0xffb9c5cb});
    g.drawText(p.category,9,0,93,height,juce::Justification::centredLeft,true);
    g.setColour(juce::Colour{0xffeef2f3});
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(p.name,106,0,juce::jmax(0,width-130),height,
               juce::Justification::centredLeft,true);
    if(favorites[index]){
        g.setColour(juce::Colour{0xffedc579});
        g.drawText("*",width-21,0,15,height,juce::Justification::centred);
    }
}
void Editor::PresetBrowser::selectedRowsChanged(int){showDetails();}
void Editor::PresetBrowser::showDetails(){
    const int row=resultList.getSelectedRow();
    if(row<0||static_cast<std::size_t>(row)>=matches.size()){
        detail.setText("No matching presets",false);
        star.setEnabled(false);use.setEnabled(false);return;
    }
    const auto i=static_cast<std::size_t>(matches[static_cast<std::size_t>(row)]);
    const auto& p=arson::preset::factoryBank()[i];
    detail.setText(juce::String(p.name)+"\n"+p.category+
        "  |  ArSon Audio\n\n"+p.description+
        "\n\nTags: "+p.tags+
        "\n\nStart here; adjust threshold for the source level.",false);
    star.setButtonText(favorites[i]?"UNSTAR":"STAR");
    star.setEnabled(true);use.setEnabled(true);
}
void Editor::PresetBrowser::applySelected(bool closeAfter){
    const int row=resultList.getSelectedRow();
    if(row<0||static_cast<std::size_t>(row)>=matches.size())return;
    owner.applyPreset(matches[static_cast<std::size_t>(row)]+1);
    if(closeAfter)setVisible(false);
}
void Editor::PresetBrowser::listBoxItemDoubleClicked(int row,const juce::MouseEvent&) {
    if(row>=0&&static_cast<std::size_t>(row)<matches.size()){
        resultList.selectRow(row);
        applySelected(true);
    }
}
bool Editor::PresetBrowser::keyPressed(const juce::KeyPress& key,juce::Component*){
    if(key==juce::KeyPress::escapeKey){setVisible(false);return true;}
    if(key==juce::KeyPress::returnKey){applySelected(false);return true;}
    return false;
}
void Editor::PresetBrowser::paint(juce::Graphics& g){
    g.fillAll(juce::Colour{0xff20282c});
    const auto r=getLocalBounds().toFloat().reduced(0.75f);
    g.setColour(juce::Colour{0xff63747d});
    g.drawRoundedRectangle(r,6.0f,1.2f);
}
void Editor::PresetBrowser::resized(){
    const int w=getWidth(),h=getHeight();
    const int rightW=juce::jmax(165,w*34/100);
    const int leftW=w-rightW-26;
    heading.setBounds(14,8,w-130,25);
    close.setBounds(w-77,10,63,24);
    search.setBounds(14,39,leftW,29);
    favoritesOnly.setBounds(14,75,91,23);
    subheading.setBounds(115,75,leftW-111,23);
    const int listY=105, listH=juce::jmax(60,h-listY-14);
    resultList.setBounds(14,listY,leftW,listH);
    detail.setBounds(leftW+23,39,rightW-8,juce::jmax(80,h-131));
    star.setBounds(leftW+23,h-80,juce::jmax(70,(rightW-15)/2),27);
    use.setBounds(leftW+30+(rightW-15)/2,h-80,juce::jmax(68,(rightW-15)/2-7),27);
}
void Editor::setValue(std::size_t i,float value) {
    if(auto* parameter=processor.state.getParameter(arson::specs[i].id)) {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}
void Editor::applyPreset(int id) {
    const auto& bank=arson::preset::factoryBank();
    if(id<1 || static_cast<std::size_t>(id)>bank.size())return;
    const auto& p=bank[static_cast<std::size_t>(id)-1];
    // Keep the safety-critical live routing/bypass state unchanged. These
    // user switches must never be set merely by auditioning a factory sound.
    for(std::size_t i=0;i<arson::Count;++i) {
        if(i==arson::External || i==arson::Listen || i==arson::Bypass)continue;
        setValue(i,p.settings[i]);
    }
    presetSnapshot=p.settings;
    currentPresetTitle=p.name;
    presets.setSelectedId(id,juce::dontSendNotification);
    presets.setText(currentPresetTitle,juce::dontSendNotification);
    cachedCurveKey[0]=999;
    graphCanvas.repaint();
}
void Editor::openPresetBrowser() {
    presetBrowser.setBounds(juce::Rectangle<int>{12,61,getWidth()-24,
        juce::jmax(220,getHeight()-73)});
    presetBrowser.open();
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
    // Compact header grid (124..154px preset); same baseline at 660..1040px.
    const auto header=arson::ui::HeaderLayout::calculate(w);
    compact.setBounds(header.mini);
    style.setBounds(header.model);
    presets.setBounds(header.preset);
    presetBrowser.setBounds(12,61,w-24,juce::jmax(220,h-73));
    compare.setBounds(header.compare);
    fileMenu.setBounds(header.file);
    bypass.setBounds(header.bypass);
    // Compact responsive hardware chassis; LCD still has its own display.
    display=compactMode?juce::Rectangle<int>{}:
        juce::Rectangle<int>{8,arson::ui::EditorMetrics::displayTop,
            w-16,h-arson::ui::EditorMetrics::displayBottomReserve};
    const int mainY=compactMode?66:display.getBottom()+5;
    const int cell=(w-28)/7;
    constexpr std::array<std::size_t,7> main{arson::Input,arson::Attack,
        arson::Release,arson::Threshold,arson::Ratio,arson::Knee,arson::Makeup};
    for(std::size_t k=0;k<main.size();++k) {
        const auto i=main[k];const int x=14+static_cast<int>(k)*cell;
        labels[i].setBounds(x,mainY+arson::ui::EditorMetrics::knobCaptionOffset,
                            cell,15);
        knobs[i].setBounds(x+3,mainY+arson::ui::EditorMetrics::knobTopOffset,
                           cell-6,arson::ui::EditorMetrics::knobHeight);
    }

    const int bottomY=compactMode?166:
        h-arson::ui::EditorMetrics::bottomControlsReserve;
    dynamicsTab.setBounds(16,bottomY+1,92,23);
    sidechainTab.setBounds(114,bottomY+1,94,23);
    // Main tab: four comfortable controls, plus a dedicated AUTO control.
    const int slot=(w-176)/4;
    constexpr std::array<std::size_t,4> dynamics{arson::Range,arson::Hold,
        arson::Mix,arson::Link};
    for(std::size_t k=0;k<dynamics.size();++k) {
        const auto i=dynamics[k];const int x=14+static_cast<int>(k)*slot;
        labels[i].setBounds(x,bottomY+arson::ui::EditorMetrics::faderCaptionOffset,
                            slot-5,14);
        knobs[i].setBounds(x+3,bottomY+arson::ui::EditorMetrics::faderTopOffset,
                           slot-10,arson::ui::EditorMetrics::faderHeight);
    }
    autoRelease.setBounds(w-145,bottomY+46,132,27);

    // Sidechain tab: only detector-specific controls are painted.
    constexpr std::array<std::size_t,2> filters{arson::Highpass,arson::Lowpass};
    // Responsive five-column sidechain panel, no fixed overflow positions.
    const int available=w-32;
    // Allocate all SIDECHAIN columns inside the 580px minimum width.
    const int usable=available-3*8;
    const int filterSlot=usable*23/100;
    const int detW=usable*17/100;
    const int extW=usable*19/100;
    const int listenW=usable-2*filterSlot-detW-extW;
    const int filterStart=16;
    for(std::size_t k=0;k<filters.size();++k) {
        const auto i=filters[k];const int x=filterStart+static_cast<int>(k)*filterSlot;
        labels[i].setBounds(x,bottomY+arson::ui::EditorMetrics::faderCaptionOffset,
                            filterSlot-8,14);
        knobs[i].setBounds(x+2,bottomY+arson::ui::EditorMetrics::faderTopOffset,
                           filterSlot-8,arson::ui::EditorMetrics::faderHeight);
    }
    const int detX=filterStart+filterSlot*2+8;
    detector.setBounds(detX,bottomY+47,detW,27);
    external.setBounds(detX+detW+8,bottomY+46,extW,28);
    listen.setBounds(detX+detW+extW+16,bottomY+46,listenW,28);
    const int readoutW=juce::jmin(254,w-40);
    graphCanvas.setBounds(compactMode
        ? juce::Rectangle<int>{13,258,w-26,47} : display);
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
                currentPresetTitle="Custom";
                presets.setSelectedId(0,juce::dontSendNotification);
                presets.setText(currentPresetTitle,juce::dontSendNotification);
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
    const auto previousInput=inputDb,previousOutput=outputDb,previousGr=gr;
    const auto previousInChannels=inChannelDb,previousOutChannels=outChannelDb;
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
    bool meterChanged=std::abs(inputDb-previousInput)>0.03f ||
        std::abs(outputDb-previousOutput)>0.03f ||
        std::abs(gr-previousGr)>0.015f;
    for(std::size_t i=0;i<2;++i)
        meterChanged=meterChanged ||
            std::abs(inChannelDb[i]-previousInChannels[i])>0.03f ||
            std::abs(outChannelDb[i]-previousOutChannels[i])>0.03f;
    if(received>0 || aged>0 || meterChanged)graphCanvas.repaint();
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
    // Quiet header group: contrast by surface tone, not divider strokes.
    g.setColour(juce::Colours::black.withAlpha(0.23f));
    g.fillRect(0,0,getWidth(),arson::ui::EditorMetrics::headerHeight);
    // Single-line embossed brand and a quiet, consistent overline hierarchy.
    const auto header=arson::ui::HeaderLayout::calculate(getWidth());
    // Same crisp native font for the brand, toolbar and controls.
    // The previous dark embossed bitmap became illegible at 612px.
    g.setFont(juce::FontOptions(14.0f));
    g.setColour(juce::Colours::black.withAlpha(0.48f));
    g.drawText("ArSonComp",header.logo.translated(0,1),
               juce::Justification::centredLeft,true);
    g.setColour(juce::Colour{0xffd6dee1});
    g.drawText("ArSonComp",header.logo,juce::Justification::centredLeft,true);
    // Labels only for dropdowns; the other buttons identify themselves.
    g.setColour(juce::Colour{arson::ui::EditorMetrics::captionRgb});
    g.setFont(juce::FontOptions(arson::ui::EditorMetrics::headerCaptionFont));
    const auto caption=[&g](const char* title,const juce::Rectangle<int>& r) {
        g.drawText(title,r.getX()+1,7,r.getWidth()-2,12,
            juce::Justification::centredLeft,false);
    };
    caption("MODEL",header.model);
    caption("PRESET",header.preset);

    const int mainY=compactMode?66:display.getBottom()+5;
    const int bottomY=compactMode?166:
        getHeight()-arson::ui::EditorMetrics::bottomControlsReserve;
    g.setColour(juce::Colours::black.withAlpha(0.105f));
    g.fillRect(8,mainY,getWidth()-16,bottomY-mainY);
    // Remove six knob-cell fences and doubled chrome rules: whitespace
    // and dark surfaces provide grouping with less visual interference.
    g.setColour(accent.withAlpha(0.68f));
    g.fillRoundedRectangle(sidechainPanel?115.0f:17.0f,
        static_cast<float>(bottomY+22),sidechainPanel?91.0f:88.0f,1.8f,1.0f);
    // Isolated canvas owns meter and graph painting; chassis stays cached.
    // The footer is intentionally clear: the LCD already shows exact values.
    // Eliminating the tiny technical slogan avoids visual competition.
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
    g.reduceClipRegion(juce::Rectangle<int>{0,0,
        display.getWidth(),display.getHeight()});
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
    graphCanvas.repaint();
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
