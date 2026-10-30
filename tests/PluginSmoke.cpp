#include "../plugin/Processor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    Processor processor;processor.prepareToPlay(48000,128);
    if(processor.getLatencySamples()!=0) return 1;
    auto* parameter=processor.state.getParameter("threshold");parameter->setValueNotifyingHost(parameter->convertTo0to1(-27));
    juce::MemoryBlock state;processor.getStateInformation(state);parameter->setValueNotifyingHost(parameter->convertTo0to1(-5));processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    if(std::abs(processor.snapshot()[arson::Threshold]+27)>0.01f)return 2;
    juce::AudioBuffer<float> buffer(2,128);juce::MidiBuffer midi;
    for(int j=0;j<1000;++j) {buffer.clear();buffer.setSample(0,0,0.5f);processor.processBlock(buffer,midi);}
    for(int iteration=0;iteration<8;++iteration) {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        editor->setSize(iteration%2 ? 860 : 960,iteration%2 ? 570 : 600);
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        if(image.isNull())return 3;
        if(iteration==0 && argc>1) {juce::File file(juce::String::fromUTF8(argv[1]));file.deleteFile();auto stream=file.createOutputStream();if(!stream || !juce::PNGImageFormat{}.writeImageToStream(image,*stream))return 4;}
    }
    processor.releaseResources();std::cout<<"PASS: state round-trip, audio, zero latency, editor resize/reopen/render\n";
}
