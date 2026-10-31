#include "../plugin/Processor.h"
#include "../plugin/Editor.h"
#include "../plugin/HeaderLayout.h"
#include "../plugin/PresetIO.h"
#include "../plugin/GraphPaint.h"
#include "../plugin/SurfacePaint.h"
#include "BinaryData.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>
#include <initializer_list>
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    // Quantitative edge-dissolve contract: endpoints transparent, the
    // central ~73% clearly visible, symmetric fades and stable RGB.
    {
        const auto coral=juce::Colour{0xffdf5860};
        const auto fade=arson::ui::makeEdgeFade(coral,0.0f,239.0f,0.9f);
        const float head=fade.getColourAtPosition(0.0).getFloatAlpha();
        const float tail=fade.getColourAtPosition(1.0).getFloatAlpha();
        const float middle=fade.getColourAtPosition(0.5).getFloatAlpha();
        const float nearStart=fade.getColourAtPosition(0.02).getFloatAlpha();
        const float innerStart=fade.getColourAtPosition(0.10).getFloatAlpha();
        const float mirrorStart=fade.getColourAtPosition(0.07).getFloatAlpha();
        const float mirrorEnd=fade.getColourAtPosition(0.93).getFloatAlpha();
        if(fade.getNumColours()!=4 || head>0.015f || tail>0.015f ||
           middle<0.85f || nearStart>=innerStart ||
           std::abs(mirrorStart-mirrorEnd)>0.035f) {
            std::cerr<<"edge-stop test: count="<<fade.getNumColours()
                <<" alpha={ "<<head<<", "<<nearStart<<", "<<innerStart
                <<", "<<middle<<", "<<mirrorStart<<", "<<mirrorEnd
                <<", "<<tail<<" }\\n";
            return 24;
        }
        const auto transfer=arson::ui::makeEdgeFade(coral,0.0f,239.0f,1.0f,0.18f);
        const float transferEdge=transfer.getColourAtPosition(0.0).getFloatAlpha();
        const float transferMiddle=transfer.getColourAtPosition(0.5).getFloatAlpha();
        if(transferEdge<0.15f || transferMiddle<0.98f) {
            std::cerr<<"transfer edge="<<transferEdge<<" middle="<<transferMiddle<<"\\n";
            return 25;
        }

        // Exercise the same API as the real scope: Graphics::strokePath
        // with a cached multi-stop brush, NOT Graphics::drawLine.
        juce::Image canvas(juce::Image::ARGB,240,16,true);
        {
            // JUCE may defer its raster writes until Graphics is destroyed.
            // Reading the image while Graphics still owns its renderer can
            // return the untouched, all-transparent pixels on Windows.
            juce::Graphics graph(canvas);
            graph.setGradientFill(fade);
            juce::Path stroke;
            stroke.startNewSubPath(0.0f,8.0f);
            stroke.lineTo(239.0f,8.0f);
            graph.strokePath(stroke,juce::PathStrokeType(2.0f));
            // Solid control verifies that the raster backend committed its
            // writes; it is outside the sampled gradient row.
            graph.setColour(juce::Colours::white);
            graph.fillRect(115,12,10,2);
        } // complete paint/flush before inspecting canvas
        const float control=canvas.getPixelAt(120,12).getFloatAlpha();
        const float left=canvas.getPixelAt(5,8).getFloatAlpha();
        const float centre=canvas.getPixelAt(120,8).getFloatAlpha();
        const float right=canvas.getPixelAt(234,8).getFloatAlpha();
        if(control<0.95f) {
            std::cerr<<"raster-backend: control pixel="<<control
                <<" (expected opaque after Graphics scope)"<<'\n';
            return 27;
        }
        if(!(centre>0.55f && left<centre*0.55f && right<centre*0.55f)) {
            std::cerr<<"raster-fade: alpha left="<<left
                <<" centre="<<centre<<" right="<<right<<'\n';
            return 26;
        }
    }
    // The black-metal knob must be an actual bundled image, not a fallback
    // vector approximation or an external file that breaks in host sandbox.
    {
        const auto base=juce::ImageFileFormat::loadFrom(
            BinaryData::black_brushed_knob_jpg,
            static_cast<std::size_t>(BinaryData::black_brushed_knob_jpgSize));
        if(!base.isValid() || base.getWidth()!=96 || base.getHeight()!=96)
            return 33;
        if(base.getPixelAt(48,48).getBrightness()<0.08f ||
           base.getPixelAt(48,48).getBrightness()>0.90f)return 34;
    }
    const auto failHost=[](int code) {
        std::cerr<<"HOST_BUS_INTEGRATION_FAILED "<<code<<'\n';
        return code;
    };
    // True JUCE bus-routing contract: test the processor entrypoint, not
    // just the DSP engine, using 2 main channels and a host mono SC bus.
    // Covers 32/64/128-frame hardware-like blocks and host bypass.
    for(const int blockSize : {32,64,128}) {
        Processor live;
        auto buses=live.getBusesLayout();
        if(buses.inputBuses.size()<2) return failHost(35);
        buses.inputBuses.set(1,juce::AudioChannelSet::mono());
        if(!live.setBusesLayout(buses) ||
           live.getTotalNumInputChannels()!=3 ||
           live.getTotalNumOutputChannels()!=2)return failHost(36);
        live.prepareToPlay(48000.0,blockSize);
        auto set=[&](const char* id,float value) {
            auto* parameter=live.state.getParameter(id);
            if(!parameter)return false;
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
            return true;
        };
        if(!set("threshold",-20.0f)||!set("ratio",4.0f)||
           !set("knee",0.0f)||!set("external",1.0f))return failHost(37);

        juce::AudioBuffer<float> signal(3,blockSize);
        juce::MidiBuffer midi;
        const auto process=[&](bool bypassed) {
            for(int i=0;i<blockSize;++i) {
                signal.setSample(0,i,0.05f);
                signal.setSample(1,i,0.025f);
                signal.setSample(2,i,1.0f); // mono external detector
            }
            if(bypassed) live.processBlockBypassed(signal,midi);
            else live.processBlock(signal,midi);
        };
        const int warmup=48000/blockSize;
        for(int b=0;b<warmup;++b)process(false);
        const float gain=arson::dbToGain(-15.0f);
        if(std::abs(signal.getSample(0,blockSize-1)-0.05f*gain)>0.002f ||
           std::abs(signal.getSample(1,blockSize-1)-0.025f*gain)>0.002f ||
           signal.getSample(2,blockSize-1)!=1.0f) {
            std::cerr<<"sidechain output @"<<blockSize<<" frames "
                <<signal.getSample(0,blockSize-1)<<", "
                <<signal.getSample(1,blockSize-1)<<", side="
                <<signal.getSample(2,blockSize-1)<<" expectedGain="
                <<gain<<'\n';
            return failHost(38);
        }

        // The sidechain is still physically present; switching to internal
        // detection must recover naturally without rewiring the host buffer.
        if(!set("external",0.0f))return failHost(39);
        for(int b=0;b<warmup;++b)process(false);
        if(std::abs(signal.getSample(0,blockSize-1)-0.05f)>0.002f ||
           std::abs(signal.getSample(1,blockSize-1)-0.025f)>0.002f)return failHost(40);

        // Host bypass path must ignore Input/Makeup and output the exact
        // input waveform after the finite click-avoidance transition.
        if(!set("input",12.0f)||!set("makeup",12.0f))return failHost(41);
        for(int b=0;b<12;++b)process(true);
        if(std::abs(signal.getSample(0,blockSize-1)-0.05f)>1.0e-5f ||
           std::abs(signal.getSample(1,blockSize-1)-0.025f)>1.0e-5f)
            return failHost(42);
        if(live.getLatencySamples()!=0)return failHost(43);
    }
    {
        // The optional SC bus must not break mono, sidechain-disabled
        // host layouts (live speech / karaoke mic use case).
        Processor mono;
        auto buses=mono.getBusesLayout();
        buses.inputBuses.set(0,juce::AudioChannelSet::mono());
        buses.outputBuses.set(0,juce::AudioChannelSet::mono());
        if(!mono.setBusesLayout(buses) || mono.getTotalNumInputChannels()!=1)
            return failHost(44);
        // Configure unity before prepareToPlay so the test explicitly
        // measures an initially-unity mono instance rather than a 5ms
        // smoothed transition from its factory 4:1 default.
        auto* unity=mono.state.getParameter("ratio");
        if(!unity)return failHost(45);
        unity->setValueNotifyingHost(unity->convertTo0to1(1.0f));
        mono.prepareToPlay(48000,32);
        juce::AudioBuffer<float> audio(1,32);
        juce::MidiBuffer midi;
        for(int i=0;i<32;++i)audio.setSample(0,i,0.123f);
        mono.processBlock(audio,midi);
        if(std::abs(audio.getSample(0,0)-0.123f)>1.0e-6f ||
           std::abs(audio.getSample(0,31)-0.123f)>1.0e-6f)return failHost(46);
    }
    // Hardware fader layer contract: independent transparent photo plate
    // (stationary), photo thumb (red aperture) and moving red LED level.
    // Confirm real JUCE BinaryData resources, not external installation paths.
    {
        const auto plate=juce::ImageFileFormat::loadFrom(
            BinaryData::gunmetal_slider_track_png,
            static_cast<std::size_t>(BinaryData::gunmetal_slider_track_pngSize));
        const auto thumb=juce::ImageFileFormat::loadFrom(
            BinaryData::gunmetal_slider_thumb_red_png,
            static_cast<std::size_t>(BinaryData::gunmetal_slider_thumb_red_pngSize));
        if(!plate.isValid() || plate.getWidth()!=340 || plate.getHeight()!=57 ||
           !thumb.isValid() || thumb.getWidth()!=58 || thumb.getHeight()!=124 ||
           plate.getPixelAt(0,0).getFloatAlpha()>0.04f ||
           thumb.getPixelAt(0,0).getFloatAlpha()>0.04f ||
           plate.getPixelAt(170,29).getFloatAlpha()<0.92f ||
           thumb.getPixelAt(29,62).getFloatAlpha()<0.92f) {
            std::cerr<<"GUNMETAL_RESOURCE_47 track="<<plate.getWidth()
                     <<"x"<<plate.getHeight()<<" thumb="<<thumb.getWidth()
                     <<"x"<<thumb.getHeight()<<" cornerAlpha="
                     <<thumb.getPixelAt(0,0).getFloatAlpha()<<" centreAlpha="
                     <<thumb.getPixelAt(29,62).getFloatAlpha()<<'\n';
            return 47;
        }

        // Raster-render all three slider positions via the *production*
        // LookAndFeel, then inspect the LED at a track location untouched
        // by the thumb. Red must be off at minimum and lit at maximum.
        CompLook look;
        juce::Slider slider;
        constexpr int w=160,h=40;
        const auto render=[&](float position) {
            juce::Image canvas(juce::Image::RGB,w,h,true);
            {
                juce::Graphics g(canvas);
                g.fillAll(juce::Colour{0xff44494d});
                look.drawLinearSlider(g,0,0,w,h,position,0.0f,0.0f,
                                      juce::Slider::LinearHorizontal,slider);
            }
            return canvas;
        };
        const auto low=render(21.0f);
        const auto mid=render(80.0f);
        const auto high=render(139.0f);
        const auto off=low.getPixelAt(61,20);
        const auto on=high.getPixelAt(61,20);
        if(on.getRed()<=off.getRed()+35 ||
           on.getRed()<=on.getGreen()+24) {
            std::cerr<<"GUNMETAL_LED_54 off="<<off.toDisplayString(true)
                     <<" on="<<on.toDisplayString(true)<<'\n';
            return 54;
        }
        // Photo thumb travels across the *same* stationary plate.
        // The centre thumbnail area is metal at 50%, but at 100% is
        // dark track; use contrast not a fragile exact RGB value.
        float diff=0.0f;
        for(int y=6;y<36;y+=4)
            for(int x=74;x<=86;x+=4)
                diff+=std::abs(mid.getPixelAt(x,y).getBrightness()-
                               high.getPixelAt(x,y).getBrightness());
        if(diff<0.25f) {
            std::cerr<<"GUNMETAL_THUMB_TRAVEL_55 image difference="
                     <<diff<<'\n';
            return 55;
        }
    }
    // Production ON/OFF resources and Bypass inversion contract.
    {
        const auto on=juce::ImageFileFormat::loadFrom(
            BinaryData::arson_toggle_on_png,
            static_cast<std::size_t>(BinaryData::arson_toggle_on_pngSize));
        const auto off=juce::ImageFileFormat::loadFrom(
            BinaryData::arson_toggle_off_png,
            static_cast<std::size_t>(BinaryData::arson_toggle_off_pngSize));
        if(!on.isValid()||!off.isValid()||
           on.getWidth()!=150||on.getHeight()!=62||
           on.getBounds()!=off.getBounds()||
           on.getPixelAt(0,0).getFloatAlpha()>0.02f||
           off.getPixelAt(0,0).getFloatAlpha()>0.02f||
           on.getPixelAt(75,31).getFloatAlpha()<0.85f||
           off.getPixelAt(75,31).getFloatAlpha()<0.85f)return 56;
        CompLook look;
        juce::ToggleButton toggle;
        toggle.setSize(126,30);
        const auto render=[&](const char* caption,bool state) {
            toggle.setButtonText(caption);
            toggle.setToggleState(state,juce::dontSendNotification);
            juce::Image canvas(juce::Image::RGB,126,30,true);
            {
                juce::Graphics g(canvas);
                g.fillAll(juce::Colours::black);
                look.drawToggleButton(g,toggle,false,false);
            }
            return canvas;
        };
        const auto countHue=[](const juce::Image& img,bool green) {
            int total=0;
            for(int y=4;y<26;++y)
                for(int x=70;x<120;++x) {
                    const auto p=img.getPixelAt(x,y);
                    if(green?(p.getGreen()>p.getRed()+25 &&
                              p.getGreen()>p.getBlue()+16):
                             (p.getRed()>p.getGreen()+40 &&
                              p.getRed()>p.getBlue()+25))
                        ++total;
                }
            return total;
        };
        if(countHue(render("AUTO",true),true)<70||
           countHue(render("AUTO",false),false)<70)return 57;
        // Bypass checkbox is DSP inverted: checked=red OFF,
        // unchecked=green ON. This must also survive the GUI theme.
        if(countHue(render("Bypass",true),false)<70||
           countHue(render("Bypass",false),true)<70)return 58;
    }
    // The small header uses the real metallic EMBOSS PNG, not a scaled-down
    // two-line neon image. RGBA, bright metal and no red subtitle are required.
    {
        const auto logo=juce::ImageFileFormat::loadFrom(
            BinaryData::arsoncomp_header_logo_png,
            static_cast<std::size_t>(BinaryData::arsoncomp_header_logo_pngSize));
        if(!logo.isValid()||logo.getWidth()!=360||logo.getHeight()!=72||
           logo.getPixelAt(0,0).getFloatAlpha()>0.02f)return 59;
        int brightMetal=0,redPixels=0;
        for(int y=0;y<logo.getHeight();++y)
            for(int x=0;x<logo.getWidth();++x) {
                const auto p=logo.getPixelAt(x,y);
                if(p.getAlpha()>170 && p.getRed()>145 &&
                   std::abs(int(p.getRed())-int(p.getGreen()))<30 &&
                   std::abs(int(p.getGreen())-int(p.getBlue()))<35)
                    ++brightMetal;
                if(p.getAlpha()>120 && p.getRed()>p.getGreen()+45)
                    ++redPixels;
            }
        if(brightMetal<600||redPixels>10)return 60;
        // Real 131px GUI header footprint: verify highlights survive downscale.
        juce::Image header(juce::Image::RGB,148,66,true);
        {
            juce::Graphics g(header);
            g.fillAll(juce::Colour{0xff282c2f});
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImageWithin(logo,13,17,131,31,juce::RectanglePlacement::centred);
        }
        int visibleHighlight=0;
        for(int y=19;y<46;++y)
            for(int x=14;x<144;++x)
                if(header.getPixelAt(x,y).getBrightness()>0.54f)
                    ++visibleHighlight;
        if(visibleHighlight<80)return 61;
    }
    // Cached chassis must be genuinely brushed grey, while the LCD
    // must remain deep black. Probe actual JUCE image rasterization after
    // the Graphics lifetime ends (Windows may defer image writes).
    {
        juce::Image metal(juce::Image::RGB,320,220,true);
        {
            juce::Graphics g(metal);
            arson::ui::paintBrushedChassis(g,320,220);
        }
        const auto value=metal.getPixelAt(30,25).getBrightness();
        const auto lower=metal.getPixelAt(100,180).getBrightness();
        // Relative reflectance is portable across Windows Direct2D and
        // software-raster paths; absolute brightness varies by backend.
        if(!(value>0.11f && value<0.88f &&
             lower>0.11f && lower<0.88f)) {
            std::cerr<<"SURFACE_CONTRAST_50 metal="<<value
                     <<" lower="<<lower<<'\n';
            return 50;
        }
        juce::Image lcd(juce::Image::RGB,320,220,true);
        {
            juce::Graphics g(lcd);
            arson::ui::paintInsetGlass(g,{0.0f,0.0f,320.0f,220.0f});
            arson::ui::paintMeterRail(g,{24.0f,21.0f,34.0f,170.0f},false);
        }
        const auto glass=lcd.getPixelAt(160,110).getBrightness();
        const auto rail=lcd.getPixelAt(42,60).getBrightness();
        if(!(glass<0.25f && rail<0.29f && value>glass+0.075f)) {
            std::cerr<<"SURFACE_CONTRAST_51 metal="<<value
                     <<" glass="<<glass<<" rail="<<rail<<'\n';
            return 51;
        }
    }
    Processor processor;processor.prepareToPlay(48000,128);
    if(processor.getLatencySamples()!=0) return 1;
    auto* parameter=processor.state.getParameter("threshold");parameter->setValueNotifyingHost(parameter->convertTo0to1(-27));
    if(parameter->getText(parameter->convertTo0to1(-27),32)!="-27.0")return 5;
    auto* frequency=processor.state.getParameter("lowpass");
    if(frequency->getText(frequency->convertTo0to1(20000),32)!="20000")return 6;
    auto* model=processor.state.getParameter("style");
    if(!model || model->getNumSteps()!=3)return 7;
    model->setValueNotifyingHost(model->convertTo0to1(2));
    juce::MemoryBlock state;processor.getStateInformation(state);parameter->setValueNotifyingHost(parameter->convertTo0to1(-5));processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    if(std::abs(processor.snapshot()[arson::Threshold]+27)>0.01f)return 2;
    if(std::abs(processor.snapshot()[arson::Style]-2.0f)>0.01f)return 8;
    // Real portable preset export/import roundtrip, including new model ID.
    const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("ArSonComp-Smoke",".arsoncomp",false);
    if(!arson::preset::save(processor,file))return 15;
    parameter->setValueNotifyingHost(parameter->convertTo0to1(-7));
    model->setValueNotifyingHost(model->convertTo0to1(0));
    if(!arson::preset::load(processor,file))return 16;
    if(std::abs(processor.snapshot()[arson::Threshold]+27)>0.01f ||
       std::abs(processor.snapshot()[arson::Style]-2.0f)>0.01f)return 17;
    file.deleteFile();
    juce::AudioBuffer<float> buffer(2,128);juce::MidiBuffer midi;
    for(int j=0;j<1000;++j) {buffer.clear();buffer.setSample(0,0,0.5f);processor.processBlock(buffer,midi);}
    // Full toolbar geometry across minimum/default/wide editor widths:
    // consistent 10px gaps, uniform baseline and bounded preset dropdown.
    for(const int width : {740,780,900,1200}) {
        const auto h=arson::ui::HeaderLayout::calculate(width);
        const std::array<juce::Rectangle<int>,7> fields{
            h.logo,h.mini,h.model,h.preset,h.compare,h.file,h.bypass};
        if(h.preset.getWidth()<144||h.preset.getWidth()>176||
           h.bypass.getRight()!=width-14||h.logo.getX()!=14)return 62;
        for(std::size_t i=1;i<fields.size();++i) {
            if(fields[i].getX()-fields[i-1].getRight()<10 ||
               fields[i].getRight()>width-14 ||
               fields[i].getWidth()<=0)return 63;
            if(fields[i].getY()!=24||fields[i].getHeight()!=28)return 64;
        }
    }
    for(int iteration=0;iteration<8;++iteration) {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        if(!editor->findChildWithID("preset.save") || !editor->findChildWithID("preset.load"))
            return 18;
        editor->setSize(iteration%2 ? 740 : 780,iteration%2 ? 585 : 640);
        if(editor->getWidth()!=(iteration%2 ? 740 : 780) ||
           editor->getHeight()!=(iteration%2 ? 585 : 640))return 9;
        auto* threshold=editor->findChildWithID("param.threshold");
        auto* input=editor->findChildWithID("param.input");
        auto* ratio=editor->findChildWithID("param.ratio");
        auto* attack=editor->findChildWithID("param.attack");
        if(!threshold || !input || !ratio || !attack ||
           !(input->getX()<attack->getX() && attack->getX()<threshold->getX() &&
             threshold->getX()<ratio->getX()))return 19;
        for(auto* candidate : {threshold,input,ratio,attack}) {
            auto* slider=dynamic_cast<juce::Slider*>(candidate);
            if(!slider || slider->getTextBoxPosition()!=juce::Slider::NoTextBox)
                return 48;
            // No floating tooltip obscures the hardware control strip.
            if(!slider->getTooltip().isEmpty())return 49;
        }
        auto* readout=dynamic_cast<juce::Label*>(
            editor->findChildWithID("view.value.readout"));
        if(!readout || !readout->isVisible() ||
           !readout->getText().containsIgnoreCase("THRESHOLD") ||
           readout->getBounds().intersects(threshold->getBounds()))return 56;
        auto* thresholdSlider=dynamic_cast<juce::Slider*>(threshold);
        if(!thresholdSlider)return 57;
        thresholdSlider->setValue(-23.0,juce::sendNotificationSync);
        if(!readout->getText().contains("-23.0"))return 57;
        if(threshold->getBounds().intersects(ratio->getBounds()))return 20;
        // New compact primary knob row: height budget for larger black LCD
        // without collapsing control interaction areas at minimum size.
        if(threshold->getHeight()!=70 || input->getHeight()!=70 ||
           attack->getHeight()!=70 || ratio->getHeight()!=70) {
            std::cerr<<"COMPACT_KNOB_HEIGHT_52 "<<threshold->getHeight()
                <<", "<<input->getHeight()<<", "<<attack->getHeight()
                <<", "<<ratio->getHeight()<<'\n';
            return 52;
        }
        // Enforce responsive layout invariants at both default and minimum.
        // This fails CI before a clipped/colliding UI ships to Windows users.
        const auto* hp=editor->findChildWithID("param.highpass");
        const auto* lp=editor->findChildWithID("param.lowpass");
        const auto* detector=editor->findChildWithID("view.detector");
        const auto* autoMode=editor->findChildWithID("view.auto");
        const auto* external=editor->findChildWithID("view.external");
        const auto* listen=editor->findChildWithID("view.listen");
        const auto* style=editor->findChildWithID("view.model");
        const auto* presets=editor->findChildWithID("view.presets");
        const auto* save=editor->findChildWithID("preset.save");
        const auto* load=editor->findChildWithID("preset.load");
        const auto* file=editor->findChildWithID("view.file");
        const auto* compare=editor->findChildWithID("view.compare");
        const auto* bypass=editor->findChildWithID("view.bypass");
        const auto* dynTab=editor->findChildWithID("view.dynamics.tab");
        const auto* scTab=editor->findChildWithID("view.sidechain.tab");
        const auto* range=editor->findChildWithID("param.range");
        const auto* mix=editor->findChildWithID("param.mix");
        if(!hp||!lp||!detector||!autoMode||!external||!listen||
           !style||!presets||!save||!load||!file||!compare||!bypass||
           !dynTab||!scTab||!range||!mix)return 21;
        // Actual JUCE component positions must match the shared header grid;
        // this catches collisions and regressions in MINI/FULL resizing.
        const auto header=arson::ui::HeaderLayout::calculate(editor->getWidth());
        const auto* mini=editor->findChildWithID("view.compact");
        if(!mini || mini->getBounds()!=header.mini ||
           style->getBounds()!=header.model ||
           presets->getBounds()!=header.preset ||
           compare->getBounds()!=header.compare ||
           file->getBounds()!=header.file ||
           bypass->getBounds()!=header.bypass)return 65;
        // File controls are deliberately not mounted in the visible header.
        // Both independent tab views must have unclipped, non-overlapping
        // active controls at 740 and 780px.
        if(save->isVisible()||load->isVisible()||
           !dynTab->isVisible()||!scTab->isVisible()||
           !range->isVisible()||!mix->isVisible()||!autoMode->isVisible()||
           hp->isVisible()||lp->isVisible()||detector->isVisible()||
           external->isVisible()||listen->isVisible())return 22;
        if(style->getBounds().intersects(presets->getBounds()) ||
           presets->getBounds().intersects(compare->getBounds()) ||
           compare->getBounds().intersects(file->getBounds()) ||
           file->getBounds().intersects(bypass->getBounds()) ||
           range->getBounds().intersects(mix->getBounds()) ||
           dynTab->getBounds().intersects(scTab->getBounds()) ||
           autoMode->getBounds().intersects(mix->getBounds()))return 28;
        // Toggle labels now sit LEFT of their switch tracks: verify the
        // small control is still at least 100px wide and never clips.
        if(autoMode->getWidth()<112 || bypass->getWidth()<100 ||
           autoMode->getHeight()<27 || bypass->getHeight()<27) {
            std::cerr<<"TOGGLE_GEOMETRY_53 auto="<<autoMode->getBounds().toString()
                     <<" bypass="<<bypass->getBounds().toString()<<'\n';
            return 53;
        }
        // Explicit const base pointer type: MSVC cannot deduce one
        // initializer_list type from mixed const/non-const Component*.
        for(const juce::Component* widget :
            std::initializer_list<const juce::Component*>{
                threshold,input,ratio,attack,hp,lp,detector,autoMode,
                external,listen,style,presets,save,load,file,compare,
                bypass,dynTab,scTab,range,mix}) {
            if(!widget->isVisible())continue;
            if(widget->getX()<0 || widget->getRight()>editor->getWidth() ||
               widget->getY()<0 || widget->getBottom()>editor->getHeight())return 23;
        }
        if(iteration==1) {
            auto* tab=dynamic_cast<juce::TextButton*>(
                editor->findChildWithID("view.sidechain.tab"));
            auto* mainTab=dynamic_cast<juce::TextButton*>(
                editor->findChildWithID("view.dynamics.tab"));
            if(!tab||!mainTab||!tab->onClick||!mainTab->onClick)return 29;
            tab->onClick();
            if(!hp->isVisible()||!lp->isVisible()||!detector->isVisible()||
               !external->isVisible()||!listen->isVisible()||
               range->isVisible()||mix->isVisible()||autoMode->isVisible()||
               lp->getBounds().intersects(detector->getBounds())||
               detector->getBounds().intersects(external->getBounds())||
               external->getBounds().intersects(listen->getBounds()))return 30;
            mainTab->onClick();
            if(!range->isVisible()||hp->isVisible()||!autoMode->isVisible())
                return 32;
        }
        editor->addToDesktop(0); editor->setVisible(true);
        for(int j=0;j<1500;++j) {
            for(int i=0;i<128;++i) {
                const float level=0.15f+0.6f*(0.5f+0.5f*std::sin(j*0.013f));
                const float sample=level*std::sin((j*128+i)*0.057f);
                buffer.setSample(0,i,sample);buffer.setSample(1,i,sample*0.8f);
            }
            processor.processBlock(buffer,midi);
        }
        if(iteration==0) {
            juce::Timer::callAfterDelay(120,[]{juce::MessageManager::getInstance()->stopDispatchLoop();});
            juce::MessageManager::getInstance()->runDispatchLoop();
        }
        if(iteration==1 && argc>1) {
            // Use the populated signal history and active desktop renderer
            // for a useful visual review of the Sidechain page.
            auto* tab=dynamic_cast<juce::TextButton*>(
                editor->findChildWithID("view.sidechain.tab"));
            auto* mainTab=dynamic_cast<juce::TextButton*>(
                editor->findChildWithID("view.dynamics.tab"));
            if(!tab || !mainTab || !tab->onClick || !mainTab->onClick)return 29;
            tab->onClick();
            const auto picture=editor->createComponentSnapshot(editor->getLocalBounds());
            juce::File fileName(juce::String::fromUTF8(argv[1]));
            fileName=fileName.getSiblingFile(
                fileName.getFileNameWithoutExtension()+"-sidechain.png");
            auto stream=fileName.createOutputStream();
            if(!stream || !juce::PNGImageFormat{}.writeImageToStream(picture,*stream))
                return 31;
            mainTab->onClick();
        }
        // Verify the real MINI/FULL UI transformation on the message thread.
        if(iteration==2) {
            auto* mini=dynamic_cast<juce::TextButton*>(editor->findChildWithID("view.compact"));
            if(!mini || !mini->onClick)return 10;
            mini->onClick();
            if(editor->getHeight()!=330)return 11;
            auto compactImage=editor->createComponentSnapshot(editor->getLocalBounds());
            if(compactImage.isNull() || compactImage.getHeight()!=330)return 12;
            if(argc>1) {
                juce::File file(juce::String::fromUTF8(argv[1]));
                file=file.getSiblingFile(file.getFileNameWithoutExtension()+"-compact.png");
                auto stream=file.createOutputStream();
                if(!stream || !juce::PNGImageFormat{}.writeImageToStream(compactImage,*stream))return 13;
            }
            mini->onClick();
            if(editor->getHeight()!=640)return 14;
        }
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        if(image.isNull())return 3;
        if(iteration<2 && argc>1) {juce::File file(juce::String::fromUTF8(argv[1]));
            if(iteration==1)file=file.getSiblingFile(file.getFileNameWithoutExtension()+"-minimum.png");
            file.deleteFile();auto stream=file.createOutputStream();if(!stream || !juce::PNGImageFormat{}.writeImageToStream(image,*stream))return 4;}
    }
    processor.releaseResources();std::cout<<"PASS: state round-trip, audio, zero latency, editor resize/reopen/render\n";
}
