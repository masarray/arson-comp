#include "../plugin/Processor.h"
#include "../plugin/PresetIO.h"
#include "../plugin/GraphPaint.h"
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
    // Validate the supplied transparent PAN-SLIDER asset is embedded in
    // JUCE BinaryData and will not require an external install path.
    {
        const auto image=juce::ImageFileFormat::loadFrom(
            BinaryData::sliderthumb_png,
            static_cast<std::size_t>(BinaryData::sliderthumb_pngSize));
        if(!image.isValid() || image.getWidth()!=18 || image.getHeight()!=40 ||
           image.getPixelAt(0,0).getFloatAlpha()>0.01f ||
           image.getPixelAt(9,20).getFloatAlpha()<0.9f) return 47;
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
    for(int iteration=0;iteration<8;++iteration) {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        if(!editor->findChildWithID("preset.save") || !editor->findChildWithID("preset.load"))
            return 18;
        editor->setSize(iteration%2 ? 860 : 900,iteration%2 ? 700 : 765);
        if(editor->getWidth()!=(iteration%2 ? 860 : 900) ||
           editor->getHeight()!=(iteration%2 ? 700 : 765))return 9;
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
            // Value remains available by accessible popup/tooltips even
            // though boxed numeric labels no longer crowd the control strip.
            if(slider->getTooltip().isEmpty())return 49;
        }
        if(threshold->getBounds().intersects(ratio->getBounds()))return 20;
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
        // File controls are deliberately not mounted in the visible header.
        // Both independent tab views must have unclipped, non-overlapping
        // active controls at 860 and 900px.
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
            if(editor->getHeight()!=380)return 11;
            auto compactImage=editor->createComponentSnapshot(editor->getLocalBounds());
            if(compactImage.isNull() || compactImage.getHeight()!=380)return 12;
            if(argc>1) {
                juce::File file(juce::String::fromUTF8(argv[1]));
                file=file.getSiblingFile(file.getFileNameWithoutExtension()+"-compact.png");
                auto stream=file.createOutputStream();
                if(!stream || !juce::PNGImageFormat{}.writeImageToStream(compactImage,*stream))return 13;
            }
            mini->onClick();
            if(editor->getHeight()!=765)return 14;
        }
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        if(image.isNull())return 3;
        if(iteration<2 && argc>1) {juce::File file(juce::String::fromUTF8(argv[1]));
            if(iteration==1)file=file.getSiblingFile(file.getFileNameWithoutExtension()+"-minimum.png");
            file.deleteFile();auto stream=file.createOutputStream();if(!stream || !juce::PNGImageFormat{}.writeImageToStream(image,*stream))return 4;}
    }
    processor.releaseResources();std::cout<<"PASS: state round-trip, audio, zero latency, editor resize/reopen/render\n";
}
