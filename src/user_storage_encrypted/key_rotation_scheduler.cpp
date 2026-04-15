/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_rotation_scheduler.cpp                         ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:45:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     327                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "key_rotation_scheduler.hpp"
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <nlohmann/json.hpp>

namespace themis {
namespace plugins {
namespace user_storage {

struct RotationSchedule {
    SecurityLevel level;
    int interval_days;
    bool auto_rotate;
    KeyRotationScheduler::RotationCallback callback;
    int64_t last_check_ms;
    
    RotationSchedule()
        : level(SecurityLevel::OFFEN)
        , interval_days(90)
        , auto_rotate(false)
        , last_check_ms(0)
    {}
};

struct KeyRotationScheduler::Impl {
    std::map<SecurityLevel, RotationSchedule> schedules;
    std::mutex mutex;
    std::atomic<bool> running;
    std::thread scheduler_thread;
    int check_interval_seconds;
    std::shared_ptr<IRotationStore> store;
    
    Impl() : running(false), check_interval_seconds(3600),
             store(std::make_shared<NullRotationStore>()) {}
    std::shared_ptr<IRotationStore> store;  // Feature 3: optional persistence backend

    // Condition variable used by shutdown() to interrupt the sleep in the loop.
    std::condition_variable cv;
    std::mutex cv_mutex;

    Impl() : running(false), check_interval_seconds(3600) {}
};

KeyRotationScheduler::KeyRotationScheduler()
    : impl_(std::make_unique<Impl>()) {
}

KeyRotationScheduler::~KeyRotationScheduler() {
    shutdown();
}

void KeyRotationScheduler::setRotationStore(std::shared_ptr<IRotationStore> store) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (store) {
        impl_->store = std::move(store);
    }
}

Result<void> KeyRotationScheduler::initialize(int check_interval_seconds) {
Result<void> KeyRotationScheduler::initialize(
    int check_interval_seconds,
    std::shared_ptr<IRotationStore> store
) {
    if (impl_->running) {
        return Result<void>::error("Scheduler already running");
    }

    impl_->check_interval_seconds = check_interval_seconds;
    impl_->store = std::move(store);
    impl_->running = true;

    // Start scheduler thread
    impl_->scheduler_thread = std::thread([this]() {
        schedulerLoop();
    });

    return Result<void>();
}

void KeyRotationScheduler::shutdown() {
    if (impl_->running) {
        impl_->running = false;
        // Wake the scheduler thread so it exits promptly.
        impl_->cv.notify_all();
        if (impl_->scheduler_thread.joinable()) {
            impl_->scheduler_thread.join();
        }
    }
}

Result<void> KeyRotationScheduler::scheduleRotation(
    SecurityLevel level,
    int interval_days,
    bool auto_rotate,
    RotationCallback callback
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    RotationSchedule schedule;
    schedule.level = level;
    schedule.interval_days = interval_days;
    schedule.auto_rotate = auto_rotate;
    schedule.callback = callback;

    // Load persisted last_check_ms (0 if not found)
    auto loaded = impl_->store->load(level);
    schedule.last_check_ms = loaded.isSuccess() ? loaded.value() : getCurrentTimeMs();
    
    // If no persisted state, initialise to now
    if (schedule.last_check_ms == 0) {
        schedule.last_check_ms = getCurrentTimeMs();
    }

    impl_->schedules[level] = schedule;

    // Feature 3: load previously persisted last_check_ms if available.
    if (impl_->store) {
        const std::string key =
            "user_storage:rotation_state:" + securityLevelToString(level);
        std::string json_value;
        if (impl_->store->get(key, json_value)) {
            try {
                auto j = nlohmann::json::parse(json_value);
                if (j.contains("last_check_ms")) {
                    impl_->schedules[level].last_check_ms =
                        j["last_check_ms"].get<int64_t>();
                }
                if (j.contains("interval_days")) {
                    impl_->schedules[level].interval_days =
                        j["interval_days"].get<int>();
                }
            } catch (...) {
                // Corrupted state; proceed with current time
            }
        }
    }

    return Result<void>();
}

void KeyRotationScheduler::cancelRotation(SecurityLevel level) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->schedules.erase(level);
}

bool KeyRotationScheduler::isRotationDue(SecurityLevel level, int64_t last_rotation_ms) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->schedules.find(level);
    if (it == impl_->schedules.end()) {
        return false;
    }
    
    const auto& schedule = it->second;
    int64_t interval_ms = static_cast<int64_t>(schedule.interval_days) * 24 * 3600 * 1000;
    int64_t elapsed_ms = getCurrentTimeMs() - last_rotation_ms;
    
    return elapsed_ms >= interval_ms;
}

int64_t KeyRotationScheduler::getNextRotationTime(SecurityLevel level) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->schedules.find(level);
    if (it == impl_->schedules.end()) {
        return 0;
    }
    
    const auto& schedule = it->second;
    int64_t interval_ms = static_cast<int64_t>(schedule.interval_days) * 24 * 3600 * 1000;
    
    return schedule.last_check_ms + interval_ms;
}

void KeyRotationScheduler::schedulerLoop() {
    while (impl_->running) {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);

            int64_t now_ms = getCurrentTimeMs();
            for (auto& pair : impl_->schedules) {
                auto& schedule = pair.second;
                
                if (!schedule.auto_rotate) {
                    continue;
                }

                int64_t now = getCurrentTimeMs();
                int64_t interval_ms = static_cast<int64_t>(schedule.interval_days) * 24 * 3600 * 1000;
                if (now - schedule.last_check_ms >= interval_ms) {
                    schedule.last_check_ms = now;
                    // Persist updated timestamp
                    impl_->store->save(schedule.level, now);

                    if (schedule.callback) {
                        schedule.callback(schedule.level, true, "");

                if (!schedule.auto_rotate || !schedule.callback) {
                    continue;
                }

                int64_t interval_ms =
                    static_cast<int64_t>(schedule.interval_days) * 24 * 3600 * 1000;

                if (now_ms - schedule.last_check_ms >= interval_ms) {
                    // Invoke the rotation callback.
                    try {
                        schedule.callback(schedule.level, true, "");
                    } catch (...) {
                        // Callback must not propagate exceptions.
                    }

                    // Update last_check_ms and persist (Feature 3).
                    schedule.last_check_ms = now_ms;
                    if (impl_->store) {
                        persistRotationState(schedule.level);
                    }
                }
            }
        }

        std::unique_lock<std::mutex> cv_lock(impl_->cv_mutex);
        impl_->cv.wait_for(
            cv_lock,
            std::chrono::seconds(impl_->check_interval_seconds),
            [this]() { return !impl_->running.load(); }
        );
    }
}

void KeyRotationScheduler::triggerRotation(SecurityLevel level) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->schedules.find(level);
    if (it == impl_->schedules.end()) {
        return;
    }
    auto& schedule = it->second;
    int64_t now = getCurrentTimeMs();
    schedule.last_check_ms = now;
    impl_->store->save(level, now);
    if (schedule.callback) {
        schedule.callback(level, true, "");
    }
}

int64_t KeyRotationScheduler::getCurrentTimeMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ---------------------------------------------------------------------------
// Feature 3: Rotation state persistence helpers (called with mutex held)
// ---------------------------------------------------------------------------

void KeyRotationScheduler::persistRotationState(SecurityLevel level) {
    if (!impl_->store) return;

    auto it = impl_->schedules.find(level);
    if (it == impl_->schedules.end()) return;

    const auto& sched = it->second;
    nlohmann::json j;
    j["last_check_ms"] = sched.last_check_ms;
    j["interval_days"] = sched.interval_days;

    const std::string key =
        "user_storage:rotation_state:" + securityLevelToString(level);
    impl_->store->put(key, j.dump());
}

void KeyRotationScheduler::loadRotationState(SecurityLevel level) {
    if (!impl_->store) return;

    auto it = impl_->schedules.find(level);
    if (it == impl_->schedules.end()) return;

    const std::string key =
        "user_storage:rotation_state:" + securityLevelToString(level);
    std::string json_value;
    if (!impl_->store->get(key, json_value)) return;

    try {
        auto j = nlohmann::json::parse(json_value);
        if (j.contains("last_check_ms")) {
            it->second.last_check_ms = j["last_check_ms"].get<int64_t>();
        }
        if (j.contains("interval_days")) {
            it->second.interval_days = j["interval_days"].get<int>();
        }
    } catch (...) {
        // Ignore corrupted persisted state.
    }
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
