#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Compressor.h"
class Processor final : public juce::AudioProcessor {
public:
    Processor();
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ArSonComp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    arson::Parameters snapshot() const noexcept;
    juce::AudioProcessorValueTreeState state;
    arson::MeterQueue meters;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void run(juce::AudioBuffer<float>&,bool hostBypass);
    std::array<std::atomic<float>*,arson::Count> raw{};
    arson::Compressor engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};
