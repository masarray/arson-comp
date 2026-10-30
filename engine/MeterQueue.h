#pragma once
#include <array>
#include <atomic>
#include <cstdint>
namespace arson {
struct MeterFrame { float input=0, output=0, reduction=0; };
// One audio producer, one editor consumer. Full queue drops visualization only.
class MeterQueue {
public:
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    bool push(MeterFrame frame) noexcept {
        const auto w=write.load(std::memory_order_relaxed), next=(w+1)%capacity;
        if (next==read.load(std::memory_order_acquire)) return false;
        frames[w]=frame; write.store(next,std::memory_order_release); return true;
    }
    bool pop(MeterFrame& frame) noexcept {
        const auto r=read.load(std::memory_order_relaxed);
        if (r==write.load(std::memory_order_acquire)) return false;
        frame=frames[r]; read.store((r+1)%capacity,std::memory_order_release); return true;
    }
    static constexpr std::uint32_t capacity=512;
private:
    std::array<MeterFrame,capacity> frames{};
    alignas(64) std::atomic<std::uint32_t> write{0};
    alignas(64) std::atomic<std::uint32_t> read{0};
};
}
