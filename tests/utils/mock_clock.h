/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_clock.h                                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:47:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • ca44511ba  2026-02-07  Add dynamic cache routing, injectable clock abstraction, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/clock.h"

namespace themis {
namespace utils {

/**
 * @brief Mock clock for deterministic testing
 * 
 * Provides controllable time for testing time-dependent behavior:
 * - Manually advance time with advance()
 * - Sleep operations immediately advance mock time
 * - No actual sleeping or waiting
 * 
 * Thread Safety: This mock is NOT thread-safe. It is designed for
 * single-threaded test scenarios where time control is explicit.
 * Do not share MockClock instances across threads without external
 * synchronization.
 */
class MockClock : public Clock {
public:
    MockClock() : current_time_(std::chrono::system_clock::now()) {}
    
    explicit MockClock(std::chrono::system_clock::time_point start_time)
        : current_time_(start_time) {}
    
    std::chrono::system_clock::time_point now() const override {
        return current_time_;
    }
    
    void sleep_for(std::chrono::milliseconds duration) override {
        // Mock sleep: instantly advance time without blocking
        advance(duration);
    }
    
    void sleep_until(std::chrono::system_clock::time_point time_point) override {
        // Mock sleep: instantly set time without blocking
        if (time_point > current_time_) {
            current_time_ = time_point;
        }
    }
    
    /**
     * @brief Manually advance mock time
     * @param duration Duration to advance
     */
    void advance(std::chrono::milliseconds duration) {
        current_time_ += duration;
    }
    
    /**
     * @brief Set absolute time
     * @param time_point Time to set
     */
    void set_time(std::chrono::system_clock::time_point time_point) {
        current_time_ = time_point;
    }
    
    /**
     * @brief Reset to current system time
     */
    void reset() {
        current_time_ = std::chrono::system_clock::now();
    }
    
private:
    std::chrono::system_clock::time_point current_time_;
};

} // namespace utils
} // namespace themis
