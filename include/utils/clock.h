/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            clock.h                                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:10:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <memory>
#include <thread>

namespace themis {
namespace utils {

/**
 * @brief Abstract clock interface for time operations
 * 
 * Provides an abstraction over std::chrono to enable deterministic testing
 * through clock injection. Tests can use MockClock while production code
 * uses SystemClock.
 */
class Clock {
public:
    virtual ~Clock() = default;
    
    /**
     * @brief Get current time point
     * @return Current time as system_clock::time_point
     */
    virtual std::chrono::system_clock::time_point now() const = 0;
    
    /**
     * @brief Sleep for specified duration
     * @param duration Duration to sleep
     */
    virtual void sleep_for(std::chrono::milliseconds duration) = 0;
    
    /**
     * @brief Sleep until specified time point
     * @param time_point Time to sleep until
     */
    virtual void sleep_until(std::chrono::system_clock::time_point time_point) = 0;
};

/**
 * @brief System clock implementation using real time
 * 
 * Production implementation that delegates to std::chrono
 */
class SystemClock : public Clock {
public:
    std::chrono::system_clock::time_point now() const override {
        return std::chrono::system_clock::now();
    }
    
    void sleep_for(std::chrono::milliseconds duration) override {
        std::this_thread::sleep_for(duration);
    }
    
    void sleep_until(std::chrono::system_clock::time_point time_point) override {
        std::this_thread::sleep_until(time_point);
    }
};

/**
 * @brief Get default system clock instance
 * @return Shared pointer to system clock
 */
inline std::shared_ptr<Clock> getSystemClock() {
    static auto clock = std::make_shared<SystemClock>();
    return clock;
}

} // namespace utils
} // namespace themis
