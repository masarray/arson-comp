#include "../plugin/Processor.h"
#include "../plugin/PresetIO.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
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
        editor->setSize(iteration%2 ? 960 : 1120,iteration%2 ? 670 : 760);
        if(editor->getWidth()!=(iteration%2 ? 960 : 1120) ||
           editor->getHeight()!=(iteration%2 ? 670 : 760))return 9;
        const auto* threshold=editor->findChildWithID("param.threshold");
        const auto* input=editor->findChildWithID("param.input");
        const auto* ratio=editor->findChildWithID("param.ratio");
        const auto* attack=editor->findChildWithID("param.attack");
        if(!threshold || !input || !ratio || !attack ||
           !(input->getX()<attack->getX() && attack->getX()<threshold->getX() &&
             threshold->getX()<ratio->getX()))return 19;
        if(threshold->getBounds().intersects(ratio->getBounds()))return 20;
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
            if(editor->getHeight()!=760)return 14;
        }
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        if(image.isNull())return 3;
        if(iteration<2 && argc>1) {juce::File file(juce::String::fromUTF8(argv[1]));
            if(iteration==1)file=file.getSiblingFile(file.getFileNameWithoutExtension()+"-minimum.png");
            file.deleteFile();auto stream=file.createOutputStream();if(!stream || !juce::PNGImageFormat{}.writeImageToStream(image,*stream))return 4;}
    }
    processor.releaseResources();std::cout<<"PASS: state round-trip, audio, zero latency, editor resize/reopen/render\n";
}
