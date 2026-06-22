#pragma once
#include <array>
#include <mutex>
#include <atomic>
 
namespace oscilleon {
 
constexpr int kCurveSampleCount = 512;
 

class SharedLfoState
{
public:
    static SharedLfoState& instance()
    {
        static SharedLfoState s;
        return s;
    }
 
    void setCurve(const std::array<float, kCurveSampleCount>& samples)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending = samples;
        m_dirty.store(true, std::memory_order_release);
    }
 

    float getPhase() const
    {
        return m_phase.load(std::memory_order_relaxed);
    }

    bool tryUpdateCurve(std::array<float, kCurveSampleCount>& dest)
    {
        if (!m_dirty.load(std::memory_order_acquire)) return false;
        if (m_mutex.try_lock()) {
            dest = m_pending;
            m_dirty.store(false, std::memory_order_release);
            m_mutex.unlock();
            return true;
        }
        return false;
    }
 
    void setPhase(float phase)    { m_phase.store(phase, std::memory_order_relaxed); }
    void setIsPlaying(bool playing) { m_playing.store(playing, std::memory_order_relaxed); }
    bool isPlaying() const          { return m_playing.load(std::memory_order_relaxed); }

private:
    SharedLfoState() { m_pending.fill(0.0f); }
    std::atomic<bool> m_playing{ false };
    std::mutex                              m_mutex;
    std::atomic<bool>                       m_dirty{ false };
    std::array<float, kCurveSampleCount>    m_pending{};
    std::atomic<float>                      m_phase{ 0.0f };
};
 
} // namespace oscilleon