#include "Processor.h"
#include "Editor.h"
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for(std::size_t i=0;i<arson::Count;++i) {
        const auto& s=arson::specs[i]; const juce::ParameterID id{s.id,1};
        if(i==arson::Detector) result.add(std::make_unique<juce::AudioParameterChoice>(id,s.name,juce::StringArray{"Peak","RMS"},0));
        else if(i>arson::Detector) result.add(std::make_unique<juce::AudioParameterBool>(id,s.name,s.initial>0.5f));
        else result.add(std::make_unique<juce::AudioParameterFloat>(id,s.name,
            juce::NormalisableRange<float>{s.min,s.max,0.0f,s.skew},s.initial,
            juce::AudioParameterFloatAttributes{}.withLabel(s.unit)));
    }
    return result;
}
Processor::Processor() : AudioProcessor(BusesProperties()
    .withInput("Input",juce::AudioChannelSet::stereo(),true)
    .withInput("Sidechain",juce::AudioChannelSet::stereo(),false)
    .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
    state(*this,nullptr,"ArSonCompState",layout()) {
    static_assert(std::atomic<float>::is_always_lock_free);
    for(std::size_t i=0;i<arson::Count;++i) raw[i]=state.getRawParameterValue(arson::specs[i].id);
    setLatencySamples(0);
}
arson::Parameters Processor::snapshot() const noexcept {
    arson::Parameters p;
    for(std::size_t i=0;i<arson::Count;++i) p.value[i]=raw[i]->load(std::memory_order_relaxed);
    return p;
}
void Processor::prepareToPlay(double rate,int) { engine.prepare(rate,snapshot()); setLatencySamples(0); }
bool Processor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto main=b.getMainInputChannelSet();
    if(main!=b.getMainOutputChannelSet() || (main!=juce::AudioChannelSet::mono() && main!=juce::AudioChannelSet::stereo())) return false;
    const auto side=b.getChannelSet(true,1);
    return side.isDisabled() || side==juce::AudioChannelSet::mono() || side==juce::AudioChannelSet::stereo();
}
void Processor::run(juce::AudioBuffer<float>& buffer,bool hostBypass) {
    juce::ScopedNoDenormals noDenormals;
    auto p=snapshot(); if(hostBypass) p.value[arson::Bypass]=1;
    engine.setParameters(p);
    auto main=getBusBuffer(buffer,false,0);
    auto side=getBusBuffer(buffer,true,1);
    engine.process(main.getArrayOfWritePointers(),main.getNumChannels(),main.getNumSamples(),
        side.getArrayOfReadPointers(),side.getNumChannels(),&meters);
}
void Processor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { run(b,false); }
void Processor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { run(b,true); }
juce::AudioProcessorParameter* Processor::getBypassParameter() const { return state.getParameter("bypass"); }
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
void Processor::getStateInformation(juce::MemoryBlock& dest) {
    auto copy=state.copyState(); copy.setProperty("schema",1,nullptr);
    if(auto xml=copy.createXml()) copyXmlToBinary(*xml,dest);
}
void Processor::setStateInformation(const void* data,int size) {
    if(size<=0 || size>1024*1024) return;
    if(auto xml=getXmlFromBinary(data,size)) {
        auto incoming=juce::ValueTree::fromXml(*xml);
        if(!incoming.hasType("ArSonCompState") || static_cast<int>(incoming.getProperty("schema",1))!=1) return;
        // Rebuild known parameter values; reject unknown IDs and non-finite payloads.
        auto safe=state.copyState();
        for(int i=0;i<safe.getNumChildren();++i) {
            auto child=safe.getChild(i);
            const auto id=child.getProperty("id").toString();
            auto source=incoming.getChildWithProperty("id",id);
            if(!source.isValid()) continue;
            for(std::size_t j=0;j<arson::Count;++j) if(id==arson::specs[j].id) {
                const float value=static_cast<float>(source.getProperty("value"));
                if(std::isfinite(value)) child.setProperty("value",std::clamp(value,arson::specs[j].min,arson::specs[j].max),nullptr);
            }
        }
        state.replaceState(safe);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor(); }
