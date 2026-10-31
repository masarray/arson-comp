#pragma once
#include "../engine/MeterQueue.h"
#include <algorithm>
#include <array>
#include <cstddef>

namespace arson::ui {
// Silence visually ages only the GUI's bounded history, not any audio data.
// Ring producer = JUCE message thread; never used by DSP thread.
template<std::size_t N>
std::size_t ageStoppedHistory(std::array<MeterFrame,N>& frames,
                              std::size_t& head,std::size_t& injected,
                              int requested) noexcept {
    static_assert(N>0);
    const auto available=N-std::min(N,injected);
    const auto count=std::min(available,
        static_cast<std::size_t>(std::max(0,requested)));
    for(std::size_t i=0;i<count;++i) {
        frames[head]=MeterFrame{};
        head=(head+1)%N;
    }
    injected+=count;
    return count;
}
}
