/**
 * @file fault_injector.cpp
 * @brief Implementation of fault injection framework
 */

#include "tests/utils/fault_injector.h"

#include <iostream>
#include <sstream>

namespace themis {
namespace test {

// ============================================================================
// CRASH INJECTOR IMPLEMENTATION
// ============================================================================

FaultInjector::InjectionResult CrashInjector::inject() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::INACTIVE) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Already injected or recovered"};
    }

    // Check probability
    if (randomDouble() >= config_.probability) {
        return InjectionResult{
            false, InjectionState::INACTIVE, config_.target_component,
            config_.type, std::chrono::steady_clock::now(),
            std::chrono::steady_clock::now(),
            "Fault skipped due to probability"};
    }

    start_time_ = std::chrono::steady_clock::now();
    crash_cfg_.should_crash_flag = true;
    state_ = InjectionState::ACTIVE;

    std::cout << "[CrashInjector] INJECTED crash at " << config_.target_component
              << " (point=" << static_cast<int>(crash_cfg_.crash_point) << ")"
              << std::endl;

    return InjectionResult{
        true, InjectionState::ACTIVE, config_.target_component, config_.type,
        start_time_, start_time_ + config_.duration, ""};
}

FaultInjector::InjectionResult CrashInjector::recover() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE && state_ != InjectionState::RECOVERING) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Not currently active"};
    }

    crash_cfg_.should_crash_flag = false;
    state_ = InjectionState::RECOVERED;

    std::cout << "[CrashInjector] RECOVERED from crash at " << config_.target_component
              << std::endl;

    return InjectionResult{
        true, InjectionState::RECOVERED, config_.target_component, config_.type,
        start_time_, std::chrono::steady_clock::now(), ""};
}

void CrashInjector::update() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE) {
        return;
    }

    if (config_.auto_recover && isDurationElapsed()) {
        std::cout << "[CrashInjector] Auto-recovering from crash at " << config_.target_component
                  << std::endl;
        crash_cfg_.should_crash_flag = false;
        state_ = InjectionState::RECOVERING;
    }
}

// ============================================================================
// NETWORK INJECTOR IMPLEMENTATION
// ============================================================================

FaultInjector::InjectionResult NetworkInjector::inject() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::INACTIVE) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Already injected or recovered"};
    }

    // Check probability
    if (randomDouble() >= config_.probability) {
        return InjectionResult{
            false, InjectionState::INACTIVE, config_.target_component,
            config_.type, std::chrono::steady_clock::now(),
            std::chrono::steady_clock::now(),
            "Fault skipped due to probability"};
    }

    start_time_ = std::chrono::steady_clock::now();
    state_ = InjectionState::ACTIVE;

    std::string fault_desc;
    switch (net_cfg_.network_type) {
        case NetworkFaultType::PARTITION:
            fault_desc = "PARTITION";
            break;
        case NetworkFaultType::DELAY:
            fault_desc = "DELAY (" + std::to_string(net_cfg_.latency.count()) + "ms)";
            break;
        case NetworkFaultType::PACKET_LOSS:
            fault_desc = "PACKET_LOSS (" + std::to_string(static_cast<int>(net_cfg_.packet_loss_rate * 100)) + "%)";
            break;
        case NetworkFaultType::CORRUPTION:
            fault_desc = "CORRUPTION";
            break;
    }

    std::cout << "[NetworkInjector] INJECTED " << fault_desc << " for " << config_.target_component
              << std::endl;

    return InjectionResult{
        true, InjectionState::ACTIVE, config_.target_component, config_.type,
        start_time_, start_time_ + config_.duration, ""};
}

FaultInjector::InjectionResult NetworkInjector::recover() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE && state_ != InjectionState::RECOVERING) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Not currently active"};
    }

    state_ = InjectionState::RECOVERED;

    std::cout << "[NetworkInjector] RECOVERED network for " << config_.target_component
              << std::endl;

    return InjectionResult{
        true, InjectionState::RECOVERED, config_.target_component, config_.type,
        start_time_, std::chrono::steady_clock::now(), ""};
}

void NetworkInjector::update() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE) {
        return;
    }

    if (config_.auto_recover && isDurationElapsed()) {
        std::cout << "[NetworkInjector] Auto-recovering network for " << config_.target_component
                  << std::endl;
        state_ = InjectionState::RECOVERING;
    }
}

// ============================================================================
// TIMEOUT INJECTOR IMPLEMENTATION
// ============================================================================

FaultInjector::InjectionResult TimeoutInjector::inject() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::INACTIVE) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Already injected or recovered"};
    }

    // Check probability
    if (randomDouble() >= config_.probability) {
        return InjectionResult{
            false, InjectionState::INACTIVE, config_.target_component,
            config_.type, std::chrono::steady_clock::now(),
            std::chrono::steady_clock::now(),
            "Fault skipped due to probability"};
    }

    start_time_ = std::chrono::steady_clock::now();
    timeout_cfg_.trigger_immediately = true;
    state_ = InjectionState::ACTIVE;

    std::cout << "[TimeoutInjector] INJECTED timeout for " << config_.target_component
              << " (delay=" << timeout_cfg_.timeout_delay.count() << "ms)" << std::endl;

    return InjectionResult{
        true, InjectionState::ACTIVE, config_.target_component, config_.type,
        start_time_, start_time_ + config_.duration, ""};
}

FaultInjector::InjectionResult TimeoutInjector::recover() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE && state_ != InjectionState::RECOVERING) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Not currently active"};
    }

    timeout_cfg_.trigger_immediately = false;
    state_ = InjectionState::RECOVERED;

    std::cout << "[TimeoutInjector] RECOVERED timeout for " << config_.target_component
              << std::endl;

    return InjectionResult{
        true, InjectionState::RECOVERED, config_.target_component, config_.type,
        start_time_, std::chrono::steady_clock::now(), ""};
}

void TimeoutInjector::update() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE) {
        return;
    }

    if (config_.auto_recover && isDurationElapsed()) {
        std::cout << "[TimeoutInjector] Auto-recovering timeout for " << config_.target_component
                  << std::endl;
        timeout_cfg_.trigger_immediately = false;
        state_ = InjectionState::RECOVERING;
    }
}

// ============================================================================
// CORRUPTION INJECTOR IMPLEMENTATION
// ============================================================================

FaultInjector::InjectionResult CorruptionInjector::inject() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::INACTIVE) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Already injected or recovered"};
    }

    // Check probability
    if (randomDouble() >= config_.probability) {
        return InjectionResult{
            false, InjectionState::INACTIVE, config_.target_component,
            config_.type, std::chrono::steady_clock::now(),
            std::chrono::steady_clock::now(),
            "Fault skipped due to probability"};
    }

    start_time_ = std::chrono::steady_clock::now();
    state_ = InjectionState::ACTIVE;

    std::cout << "[CorruptionInjector] INJECTED data corruption in " << config_.target_component
              << " (field=" << corr_cfg_.data_field
              << ", rate=" << static_cast<int>(corr_cfg_.corruption_rate * 100) << "%)" << std::endl;

    return InjectionResult{
        true, InjectionState::ACTIVE, config_.target_component, config_.type,
        start_time_, start_time_ + config_.duration, ""};
}

FaultInjector::InjectionResult CorruptionInjector::recover() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE && state_ != InjectionState::RECOVERING) {
        return InjectionResult{
            false, state_, config_.target_component, config_.type,
            start_time_, std::chrono::steady_clock::now(),
            "Not currently active"};
    }

    state_ = InjectionState::RECOVERED;

    std::cout << "[CorruptionInjector] RECOVERED from corruption in " << config_.target_component
              << std::endl;

    return InjectionResult{
        true, InjectionState::RECOVERED, config_.target_component, config_.type,
        start_time_, std::chrono::steady_clock::now(), ""};
}

void CorruptionInjector::update() {
    std::lock_guard<std::mutex> lk(mutex_);

    if (state_ != InjectionState::ACTIVE) {
        return;
    }

    if (config_.auto_recover && isDurationElapsed()) {
        std::cout << "[CorruptionInjector] Auto-recovering from corruption in "
                  << config_.target_component << std::endl;
        state_ = InjectionState::RECOVERING;
    }
}

size_t CorruptionInjector::corruptData(uint8_t* buffer, size_t size) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (!isActive()) {
        return 0;
    }

    size_t corrupted_count = 0;
    for (size_t i = 0; i < size; ++i) {
        if (randomDouble() < corr_cfg_.corruption_rate) {
            buffer[i] ^= (randomInt(1, 255) & 0xFF);
            corrupted_count++;
        }
    }

    return corrupted_count;
}

// ============================================================================
// FAULT ORCHESTRATOR IMPLEMENTATION
// ============================================================================

int FaultOrchestrator::addInjector(std::unique_ptr<FaultInjector> injector) {
    std::lock_guard<std::mutex> lk(mutex_);
    int id = generateId();
    injectors_[id] = std::move(injector);
    return id;
}

void FaultOrchestrator::removeInjector(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    injectors_.erase(id);
}

void FaultOrchestrator::injectAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [id, injector] : injectors_) {
        auto result = injector->inject();
        if (!result.success) {
            std::cerr << "[FaultOrchestrator] Failed to inject fault: " << result.error_message
                      << std::endl;
        }
    }
}

void FaultOrchestrator::recoverAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [id, injector] : injectors_) {
        auto result = injector->recover();
        if (!result.success) {
            std::cerr << "[FaultOrchestrator] Failed to recover from fault: "
                      << result.error_message << std::endl;
        }
    }
}

void FaultOrchestrator::updateAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [id, injector] : injectors_) {
        injector->update();
    }
}

bool FaultOrchestrator::hasActiveFaults() const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& [id, injector] : injectors_) {
        if (injector->isActive()) {
            return true;
        }
    }
    return false;
}

size_t FaultOrchestrator::getActiveFaultCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    size_t count = 0;
    for (const auto& [id, injector] : injectors_) {
        if (injector->isActive()) {
            count++;
        }
    }
    return count;
}

FaultInjector* FaultOrchestrator::getInjector(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = injectors_.find(id);
    if (it != injectors_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<int> FaultOrchestrator::getAllInjectorIds() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<int> ids;
    for (const auto& [id, injector] : injectors_) {
        ids.push_back(id);
    }
    return ids;
}

void FaultOrchestrator::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    injectors_.clear();
}

std::map<std::string, int> FaultOrchestrator::getStatistics() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::map<std::string, int> stats;
    stats["total_injectors"] = injectors_.size();
    
    int active_count = 0;
    int recovered_count = 0;
    for (const auto& [id, injector] : injectors_) {
        if (injector->isActive()) {
            active_count++;
        } else if (injector->getState() == FaultInjector::InjectionState::RECOVERED) {
            recovered_count++;
        }
    }
    
    stats["active_faults"] = active_count;
    stats["recovered_faults"] = recovered_count;
    
    return stats;
}

// ============================================================================
// CHAOS SCENARIO BUILDER IMPLEMENTATION
// ============================================================================

ChaosScenario& ChaosScenario::crash(
    const std::string& target,
    std::chrono::milliseconds duration,
    CrashInjector::CrashPoint point) {
    
    CrashInjector::CrashConfig cfg;
    cfg.type = FaultInjector::FaultType::CRASH;
    cfg.target_component = target;
    cfg.duration = duration;
    cfg.crash_point = point;
    
    auto injector = std::make_unique<CrashInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

ChaosScenario& ChaosScenario::networkPartition(
    const std::string& target_node,
    const std::vector<std::string>& partition_members,
    std::chrono::milliseconds duration) {
    
    NetworkInjector::NetworkConfig cfg;
    cfg.type = FaultInjector::FaultType::NETWORK_PARTITION;
    cfg.target_component = target_node;
    cfg.duration = duration;
    cfg.network_type = NetworkInjector::NetworkFaultType::PARTITION;
    cfg.target_node = target_node;
    cfg.partition_members = partition_members;
    
    auto injector = std::make_unique<NetworkInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

ChaosScenario& ChaosScenario::networkDelay(
    const std::string& target_node,
    std::chrono::milliseconds latency,
    std::chrono::milliseconds duration) {
    
    NetworkInjector::NetworkConfig cfg;
    cfg.type = FaultInjector::FaultType::NETWORK_DELAY;
    cfg.target_component = target_node;
    cfg.duration = duration;
    cfg.network_type = NetworkInjector::NetworkFaultType::DELAY;
    cfg.target_node = target_node;
    cfg.latency = latency;
    
    auto injector = std::make_unique<NetworkInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

ChaosScenario& ChaosScenario::packetLoss(
    const std::string& target_node,
    float loss_rate,
    std::chrono::milliseconds duration) {
    
    NetworkInjector::NetworkConfig cfg;
    cfg.type = FaultInjector::FaultType::NETWORK_PACKET_LOSS;
    cfg.target_component = target_node;
    cfg.duration = duration;
    cfg.network_type = NetworkInjector::NetworkFaultType::PACKET_LOSS;
    cfg.target_node = target_node;
    cfg.packet_loss_rate = loss_rate;
    
    auto injector = std::make_unique<NetworkInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

ChaosScenario& ChaosScenario::timeout(
    const std::string& target,
    std::chrono::milliseconds duration) {
    
    TimeoutInjector::TimeoutConfig cfg;
    cfg.type = FaultInjector::FaultType::TIMEOUT;
    cfg.target_component = target;
    cfg.duration = duration;
    cfg.trigger_immediately = true;
    
    auto injector = std::make_unique<TimeoutInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

ChaosScenario& ChaosScenario::corruption(
    const std::string& target,
    const std::string& field,
    float corruption_rate) {
    
    CorruptionInjector::CorruptionConfig cfg;
    cfg.type = FaultInjector::FaultType::CORRUPTION;
    cfg.target_component = target;
    cfg.data_field = field;
    cfg.corruption_rate = corruption_rate;
    
    auto injector = std::make_unique<CorruptionInjector>(cfg);
    orchestrator_->addInjector(std::move(injector));
    
    return *this;
}

FaultOrchestrator* ChaosScenario::build() {
    orchestrator_->injectAll();
    return orchestrator_.get();
}

}  // namespace test
}  // namespace themis
