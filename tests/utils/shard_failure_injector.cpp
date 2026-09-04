#include "shard_failure_injector.h"
#include <random>
#include <algorithm>

namespace themis {
namespace test {

ShardFailureInjector::ShardFailureInjector() : next_scenario_id_(1) {}

ShardFailureInjector::~ShardFailureInjector() = default;

int ShardFailureInjector::injectFailure(int shard_id, FailureType type,
                                        std::chrono::milliseconds duration) {
    int scenario_id = generateScenarioId();

    FailureScenario scenario;
    scenario.type = type;
    scenario.shard_id = shard_id;
    scenario.duration = duration;
    scenario.probability = 1.0f;
    scenario.active = true;
    scenario.start_time = std::chrono::steady_clock::now();

    scenarios_[scenario_id] = scenario;

    if (failure_callback_) {
        failure_callback_(shard_id, type);
    }

    return scenario_id;
}

std::vector<int> ShardFailureInjector::injectRandomFailures(
    int num_failures, int max_shard_id, bool transient,
    std::chrono::milliseconds max_duration) {
    std::vector<int> scenario_ids;
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> shard_dist(0, max_shard_id);
    std::uniform_int_distribution<> type_dist(0, 3);
    std::uniform_int_distribution<> duration_dist(1000, max_duration.count());

    for (int i = 0; i < num_failures; ++i) {
        int shard_id = shard_dist(gen);
        auto type = static_cast<FailureType>(type_dist(gen));
        auto duration = transient ? std::chrono::milliseconds(duration_dist(gen))
                                   : std::chrono::milliseconds{0};

        int scenario_id = injectFailure(shard_id, type, duration);
        scenario_ids.push_back(scenario_id);
    }

    return scenario_ids;
}

std::vector<int> ShardFailureInjector::injectCascadingFailures(
    int initial_shard, int cascade_count, std::chrono::milliseconds cascade_delay) {
    std::vector<int> scenario_ids;

    // Initial failure
    int scenario_id = injectFailure(initial_shard, FailureType::PERMANENT);
    scenario_ids.push_back(scenario_id);

    // Cascading failures
    for (int i = 0; i < cascade_count; ++i) {
        int next_shard = initial_shard + i + 1;
        auto delay_duration = cascade_delay * (i + 1);

        scenario_id = injectFailure(next_shard, FailureType::TRANSIENT, delay_duration);
        scenario_ids.push_back(scenario_id);
    }

    return scenario_ids;
}

void ShardFailureInjector::clearAllFailures() {
    // Trigger recovery callbacks
    if (recovery_callback_) {
        for (const auto& [id, scenario] : scenarios_) {
            if (scenario.active) {
                recovery_callback_(scenario.shard_id);
            }
        }
    }

    scenarios_.clear();
}

bool ShardFailureInjector::recoverScenario(int scenario_id) {
    auto it = scenarios_.find(scenario_id);
    if (it == scenarios_.end() || !it->second.active) {
        return false;
    }

    it->second.active = false;

    if (recovery_callback_) {
        recovery_callback_(it->second.shard_id);
    }

    return true;
}

bool ShardFailureInjector::isShardFailed(int shard_id) const {
    for (const auto& [id, scenario] : scenarios_) {
        if (scenario.active && scenario.shard_id == shard_id) {
            return true;
        }
    }
    return false;
}

std::vector<ShardFailureInjector::FailureType> ShardFailureInjector::getActiveFailures(
    int shard_id) const {
    std::vector<FailureType> failures;

    for (const auto& [id, scenario] : scenarios_) {
        if (scenario.active && scenario.shard_id == shard_id) {
            failures.push_back(scenario.type);
        }
    }

    return failures;
}

std::vector<int> ShardFailureInjector::getActiveScenarios() const {
    std::vector<int> active;

    for (const auto& [id, scenario] : scenarios_) {
        if (scenario.active) {
            active.push_back(id);
        }
    }

    return active;
}

const ShardFailureInjector::FailureScenario* ShardFailureInjector::getScenario(
    int scenario_id) const {
    auto it = scenarios_.find(scenario_id);
    if (it != scenarios_.end()) {
        return &it->second;
    }
    return nullptr;
}

void ShardFailureInjector::setFailureCallback(FailureCallback callback) {
    failure_callback_ = callback;
}

void ShardFailureInjector::setRecoveryCallback(RecoveryCallback callback) {
    recovery_callback_ = callback;
}

void ShardFailureInjector::update() {
    auto now = std::chrono::steady_clock::now();

    std::vector<int> to_recover;

    for (auto& [id, scenario] : scenarios_) {
        if (!scenario.active) {
            continue;
        }

        // Check for auto-recovery (transient failures with duration)
        if (scenario.type == FailureType::TRANSIENT && scenario.duration.count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - scenario.start_time);

            if (elapsed >= scenario.duration) {
                to_recover.push_back(id);
            }
        }
    }

    // Recover scenarios
    for (int id : to_recover) {
        recoverScenario(id);
    }
}

std::map<std::string, int> ShardFailureInjector::getStatistics() const {
    std::map<std::string, int> stats = {};

    stats["total_scenarios"] = static_cast<int>(scenarios_.size());
    stats["active_scenarios"] = 0;
    stats["transient_failures"] = 0;
    stats["permanent_failures"] = 0;
    stats["slow_failures"] = 0;
    stats["corrupt_failures"] = 0;

    for (const auto& [id, scenario] : scenarios_) {
        if (scenario.active) {
            stats["active_scenarios"]++;

            switch (scenario.type) {
                case FailureType::TRANSIENT:
                    stats["transient_failures"]++;
                    break;
                case FailureType::PERMANENT:
                    stats["permanent_failures"]++;
                    break;
                case FailureType::SLOW:
                    stats["slow_failures"]++;
                    break;
                case FailureType::CORRUPT:
                    stats["corrupt_failures"]++;
                    break;
            }
        }
    }

    return stats;
}

int ShardFailureInjector::generateScenarioId() {
    return next_scenario_id_++;
}

} // namespace test
} // namespace themis
