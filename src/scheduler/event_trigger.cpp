#include "scheduler/event_trigger.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {

// ===== CDCTriggerConfig Implementation =====

bool CDCTriggerConfig::isValid() const {
    return !key_prefix.empty() && !event_types.empty();
}

std::string CDCTriggerConfig::getValidationError() const {
    if (key_prefix.empty()) {
        return "Key prefix cannot be empty";
    }
    if (event_types.empty()) {
        return "At least one event type must be specified";
    }
    return "";
}

// ===== EventTrigger Implementation =====

EventTrigger::EventTrigger(Changefeed* changefeed,
                           const CDCTriggerConfig& config,
                           TriggerCallback callback)
    : changefeed_(changefeed),
      config_(config),
      callback_(std::move(callback)),
      last_trigger_time_(std::chrono::steady_clock::now()) {
    
    if (!changefeed_) {
        throw std::invalid_argument("EventTrigger: changefeed cannot be null");
    }
    
    if (!config_.isValid()) {
        throw std::invalid_argument("EventTrigger: invalid config - " + 
                                   config_.getValidationError());
    }
    
    if (!callback_) {
        throw std::invalid_argument("EventTrigger: callback cannot be null");
    }
}

EventTrigger::~EventTrigger() {
    stop();
}

void EventTrigger::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_.load()) {
        THEMIS_WARN("EventTrigger already running");
        return;
    }
    
    running_.store(true);
    listener_thread_ = std::thread(&EventTrigger::listenerLoop, this);
    
    THEMIS_INFO("EventTrigger started (key_prefix={}, event_types={}, debounce={}ms)",
                config_.key_prefix,
                config_.event_types.size(),
                config_.debounce_ms);
}

void EventTrigger::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    
    cv_.notify_all();
    
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
    
    THEMIS_INFO("EventTrigger stopped (triggers_fired={})", triggers_fired_.load());
}

void EventTrigger::updateConfig(const CDCTriggerConfig& config) {
    if (!config.isValid()) {
        throw std::invalid_argument("EventTrigger: invalid config - " + 
                                   config.getValidationError());
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    THEMIS_DEBUG("EventTrigger config updated (key_prefix={}, event_types={}, debounce={}ms)",
                 config_.key_prefix,
                 config_.event_types.size(),
                 config_.debounce_ms);
}

EventTrigger::Stats EventTrigger::getStats() const {
    Stats stats;
    stats.events_received = events_received_.load();
    stats.events_matched = events_matched_.load();
    stats.events_debounced = events_debounced_.load();
    stats.triggers_fired = triggers_fired_.load();
    stats.last_trigger_time = last_trigger_time_sys_;
    return stats;
}

void EventTrigger::listenerLoop() {
    THEMIS_DEBUG("EventTrigger listener loop started");
    
    while (running_.load()) {
        try {
            // Get current sequence inside the loop to ensure changefeed is valid
            if (last_sequence_ == 0) {
                last_sequence_ = changefeed_->getLatestSequence();
            }
            // Poll for new events with long-polling
            Changefeed::ListOptions options;
            options.from_sequence = last_sequence_;
            options.limit = 100;
            options.long_poll_ms = 1000; // 1 second long poll
            
            auto events = changefeed_->listEvents(options);
            events_received_ += events.size();
            
            for (const auto& event : events) {
                // Update last sequence
                if (event.sequence > last_sequence_) {
                    last_sequence_ = event.sequence;
                }
                
                // Check if event matches filter
                if (!matchesFilter(event)) {
                    continue;
                }
                
                events_matched_++;
                
                // Check debouncing
                if (shouldDebounce()) {
                    events_debounced_++;
                    THEMIS_DEBUG("Event debounced (key={}, type={})",
                                event.key, static_cast<int>(event.type));
                    continue;
                }
                
                // Update last trigger time
                {
                    std::lock_guard<std::mutex> lock(debounce_mutex_);
                    last_trigger_time_ = std::chrono::steady_clock::now();
                    last_trigger_time_sys_ = std::chrono::system_clock::now();
                }
                
                // Fire callback
                triggers_fired_++;
                THEMIS_DEBUG("EventTrigger fired (key={}, type={}, sequence={})",
                            event.key, static_cast<int>(event.type), event.sequence);
                
                try {
                    callback_(event);
                } catch (const std::exception& e) {
                    THEMIS_ERROR("EventTrigger callback failed: {}", e.what());
                }
            }
            
            // If no events, wait a bit before polling again
            if (events.empty()) {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(100),
                           [this] { return !running_.load(); });
            }
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("EventTrigger listener error: {}", e.what());
            
            // Wait before retrying
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1),
                       [this] { return !running_.load(); });
        }
    }
    
    THEMIS_DEBUG("EventTrigger listener loop stopped");
}

bool EventTrigger::matchesFilter(const Changefeed::ChangeEvent& event) const {
    // Check key prefix
    if (!matchesKeyPrefix(event.key)) {
        return false;
    }
    
    // Check event type
    if (!matchesEventType(event.type)) {
        return false;
    }
    
    // Check optional condition
    if (!matchesCondition(event)) {
        return false;
    }
    
    return true;
}

bool EventTrigger::matchesKeyPrefix(const std::string& key) const {
    const auto& prefix = config_.key_prefix;
    
    // Handle wildcard
    if (prefix == "*") {
        return true;
    }
    
    // Handle prefix with wildcard (e.g., "users:*")
    if (prefix.back() == '*') {
        std::string prefix_without_wildcard = prefix.substr(0, prefix.length() - 1);
        return key.find(prefix_without_wildcard) == 0;
    }
    
    // Exact match
    return key.find(prefix) == 0;
}

bool EventTrigger::matchesEventType(Changefeed::ChangeEventType type) const {
    return config_.event_types.find(type) != config_.event_types.end();
}

bool EventTrigger::matchesCondition(const Changefeed::ChangeEvent& event) const {
    // If no condition specified, always match
    if (!config_.condition || config_.condition->empty()) {
        return true;
    }
    
    // TODO: Implement AQL condition evaluation
    // For now, always return true if condition is specified
    // In full implementation, this would evaluate the AQL expression
    // against the event payload
    THEMIS_DEBUG("Condition evaluation not yet implemented, matching by default");
    return true;
}

bool EventTrigger::shouldDebounce() const {
    if (config_.debounce_ms == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(debounce_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_trigger_time_).count();
    
    return elapsed < config_.debounce_ms;
}

// ===== EventTriggerManager Implementation =====

EventTriggerManager::EventTriggerManager(Changefeed* changefeed)
    : changefeed_(changefeed) {
    if (!changefeed_) {
        throw std::invalid_argument("EventTriggerManager: changefeed cannot be null");
    }
}

EventTriggerManager::~EventTriggerManager() {
    stopAll();
}

bool EventTriggerManager::registerTrigger(const std::string& id,
                                          const CDCTriggerConfig& config,
                                          EventTrigger::TriggerCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (triggers_.find(id) != triggers_.end()) {
        THEMIS_WARN("EventTrigger already registered: {}", id);
        return false;
    }
    
    try {
        auto trigger = std::make_unique<EventTrigger>(changefeed_, config, std::move(callback));
        trigger->start();
        triggers_[id] = std::move(trigger);
        
        THEMIS_INFO("Registered event trigger: {}", id);
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to register event trigger: {}", e.what());
        return false;
    }
}

void EventTriggerManager::unregisterTrigger(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = triggers_.find(id);
    if (it != triggers_.end()) {
        it->second->stop();
        triggers_.erase(it);
        THEMIS_INFO("Unregistered event trigger: {}", id);
    }
}

void EventTriggerManager::updateTrigger(const std::string& id, const CDCTriggerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = triggers_.find(id);
    if (it != triggers_.end()) {
        it->second->updateConfig(config);
        THEMIS_INFO("Updated event trigger: {}", id);
    }
}

std::optional<EventTrigger::Stats> EventTriggerManager::getTriggerStats(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = triggers_.find(id);
    if (it != triggers_.end()) {
        return it->second->getStats();
    }
    
    return std::nullopt;
}

void EventTriggerManager::startAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, trigger] : triggers_) {
        if (!trigger->isRunning()) {
            trigger->start();
        }
    }
    
    THEMIS_INFO("Started all event triggers (count={})", triggers_.size());
}

void EventTriggerManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, trigger] : triggers_) {
        if (trigger->isRunning()) {
            trigger->stop();
        }
    }
    
    THEMIS_INFO("Stopped all event triggers (count={})", triggers_.size());
}

} // namespace themis
