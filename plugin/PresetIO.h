#pragma once
#include "Processor.h"
#include <juce_gui_extra/juce_gui_extra.h>

// UI-thread-only file I/O. Never called from processBlock.
// Use the same versioned host-state schema for project and user preset recall.
namespace arson::preset {
inline bool save(Processor& processor, const juce::File& file) {
    juce::MemoryBlock bytes;
    processor.getStateInformation(bytes);
    auto xml=juce::getXmlFromBinary(bytes.getData(),static_cast<int>(bytes.getSize()));
    return xml!=nullptr && file.hasFileExtension(".arsoncomp") && xml->writeTo(file);
}
inline bool load(Processor& processor, const juce::File& file) {
    if(!file.existsAsFile() || !file.hasFileExtension(".arsoncomp")
        || file.getSize()>1024*1024) return false;
    auto xml=juce::XmlDocument::parse(file);
    if(!xml || !xml->hasTagName("ArSonCompState")
        || xml->getIntAttribute("schema",1)!=1) return false;
    juce::MemoryBlock bytes;
    juce::copyXmlToBinary(*xml,bytes);
    processor.setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
    return true;
}
}
