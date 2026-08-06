/**
 * @file event_trigger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: event_trigger.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 565
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=5, M=8, L=0
 * PR History (last 5): #1301 Scheduler Module: Productio... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "scheduler/event_trigger.h"
#include "utils/logger.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace themis {

// ===== Simple condition evaluator =====
//
// Supported syntax (case-sensitive operators):
//   key == "value"
//   key != "value"
//   key STARTS_WITH "prefix"
//   key ENDS_WITH "suffix"
//   key CONTAINS "substring"
//   value == "v"  (matches event.value when present)
//   value CONTAINS "s"
//   ... (same operators applied to "value" field)
//
// Unquoted RHS tokens are also accepted.
// On any parse / evaluation error the condition is treated as a match (fail-open).

namespace {

// Trim leading/trailing whitespace
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

// Strip surrounding double-quotes if present
static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Evaluate a single simple condition against lhs (the resolved field value)
static bool evalOp(const std::string& lhs, const std::string& op, const std::string& rhs) {
    if (op == "==") {
        return lhs == rhs;
    } else if (op == "!=") {
        return lhs != rhs;
    } else if (op == "STARTS_WITH") {
        return lhs.size() >= rhs.size() && lhs.compare(0, rhs.size(), rhs) == 0;
    } else if (op == "ENDS_WITH") {
        return lhs.size() >= rhs.size() &&
               lhs.compare(lhs.size() - rhs.size(), rhs.size(), rhs) == 0;
    } else if (op == "CONTAINS") {
        return lhs.find(rhs) != std::string::npos;
    }
    // Unknown operator – fail-open
    THEMIS_WARN("EventTrigger: unknown condition operator '{}', matching by default", op);
    return true;
}

} // anonymous namespace


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

    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    // Invalidate cached parsed condition so it is rebuilt on next event
    {
        std::lock_guard<std::mutex> clock(condition_cache_mutex_);
        condition_parsed_ = false;
        parsed_clauses_.clear();
    }

    THEMIS_DEBUG("EventTrigger config updated (key_prefix={}, event_types={}, debounce={}ms)",
                 config.key_prefix,
                 config.event_types.size(),
                 config.debounce_ms);
}

EventTrigger::Stats EventTrigger::getStats() const {
    Stats stats;
    stats.events_received = events_received_.load();
    stats.events_matched = events_matched_.load();
    stats.events_debounced = events_debounced_.load();
    stats.triggers_fired = triggers_fired_.load();
    stats.callback_failures = callback_failures_.load();
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        stats.circuit_open = cb_open_;
    }
    stats.last_trigger_time = last_trigger_time_sys_;
    return stats;
}

void EventTrigger::setCircuitBreakerConfig(const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    cb_config_ = config;
}

bool EventTrigger::circuitAllows() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    if (!cb_open_) {
        return true;
    }
    // Check if cooldown has elapsed → allow a half-open probe
    auto elapsed = std::chrono::steady_clock::now() - cb_open_since_;
    if (elapsed >= cb_config_.cooldown) {
        THEMIS_INFO("EventTrigger circuit breaker: cooldown elapsed, allowing probe");
        return true;  // Let one call through (half-open)
    }
    return false;
}

void EventTrigger::circuitRecordSuccess() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    if (cb_open_) {
        THEMIS_INFO("EventTrigger circuit breaker: closing (callback recovered)");
    }
    cb_consecutive_failures_ = 0;
    cb_open_ = false;
}

void EventTrigger::circuitRecordFailure() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    ++cb_consecutive_failures_;
    callback_failures_++;
    if (!cb_open_ && cb_consecutive_failures_ >= cb_config_.failure_threshold) {
        cb_open_ = true;
        cb_open_since_ = std::chrono::steady_clock::now();
        THEMIS_WARN("EventTrigger circuit breaker: opened after {} consecutive callback failures "
                    "(cooldown={}s)",
                    cb_consecutive_failures_, cb_config_.cooldown.count());
    }
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
                
                // Fire callback – guarded by circuit breaker
                if (!circuitAllows()) {
                    THEMIS_DEBUG("EventTrigger circuit breaker open: dropping callback for "
                                 "key={}", event.key);
                    continue;
                }

                // GAP 2 FIX: Deduplication - prevent duplicate trigger firings
                // Check if this event is the same as the last fired event (within debounce window)
                if (shouldDebounce() && last_fired_event_key_ == event.key && 
                    last_fired_event_value_ == (event.value ? *event.value : "")) {
                    THEMIS_DEBUG("EventTrigger: duplicate trigger prevented (key={}, debounce_active)",
                                event.key);
                    continue;
                }
                
                // Update last fired tracking
                last_fired_event_key_ = event.key;
                last_fired_event_value_ = event.value ? *event.value : "";

                triggers_fired_++;
                THEMIS_DEBUG("EventTrigger fired (key={}, type={}, sequence={})",
                            event.key, static_cast<int>(event.type), event.sequence);
                
                try {
                    callback_(event);
                    circuitRecordSuccess();
                } catch (const std::exception& e) {
                    THEMIS_ERROR("EventTrigger callback failed: {}", e.what());
                    circuitRecordFailure();
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

    // Ensure clauses are parsed (lazy, cached)
    {
        std::lock_guard<std::mutex> clock(condition_cache_mutex_);
        if (!condition_parsed_) {
            rebuildConditionCache_();
            condition_parsed_ = true;
        }
    }
    
    // GAP 3 FIX: Detect circular dependencies early
    if (!validateNoCycularDependencies()) {
        THEMIS_ERROR("EventTrigger: condition validation failed due to circular dependency");
        return false;  // Reject event on structural validation failure
    }

    // GAP 1 FIX: Atomic evaluation - validate all clauses structurally first
    // Returns false (no match) if any predicate fails structural checks
    for (const auto& clause : parsed_clauses_) {
        // Structural validation: field must be known
        if (clause.field != "key" && clause.field != "value") {
            THEMIS_ERROR("EventTrigger: invalid field '{}' in condition - structural validation failed",
                        clause.field);
            return false;  // kTriggerInvalid behavior: reject on structural error
        }
    }

    // Evaluate each cached clause; all must pass (AND semantics)
    for (const auto& clause : parsed_clauses_) {
        // Resolve LHS (now guaranteed to be valid)
        std::string lhs;
        if (clause.field == "key") {
            lhs = event.key;
        } else { // "value"
            lhs = event.value ? *event.value : "";
        }

        if (!evalOp(lhs, clause.op, clause.rhs)) {
            return false;
        }
    }

    return true;
}

void EventTrigger::rebuildConditionCache_() const {
    // Must be called under condition_cache_mutex_
    parsed_clauses_.clear();

    if (!config_.condition || config_.condition->empty()) {
        return;
    }

    const std::string& condition = *config_.condition;
    static const std::string AND_SEP = " AND ";

    // Split on AND
    std::vector<std::string> raw_clauses;
    size_t pos = 0;
    while (pos < condition.size()) {
        size_t found = condition.find(AND_SEP, pos);
        if (found == std::string::npos) {
            raw_clauses.push_back(themis::utils::trim(condition.substr(pos)));
            break;
        }
        raw_clauses.push_back(themis::utils::trim(condition.substr(pos, found - pos)));
        pos = found + AND_SEP.size();
    }

    for (const auto& raw : raw_clauses) {
        if (raw.empty()) continue;

        // Tokenise the clause
        std::vector<std::string> tokens;
        size_t i = 0;
        const size_t n = raw.size();
        while (i < n) {
            while (i < n && std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
            if (i >= n) break;
            if (raw[i] == '"') {
                size_t j = i + 1;
                while (j < n && raw[j] != '"') ++j;
                tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
                i = j + 1;
            } else {
                size_t j = i;
                while (j < n && !std::isspace(static_cast<unsigned char>(raw[j]))) ++j;
                tokens.push_back(raw.substr(i, j - i));
                i = j;
            }
        }

        if (tokens.size() < 3) {
            THEMIS_WARN("EventTrigger: malformed condition clause '{}', skipping", raw);
            continue;
        }

        ParsedClause pc;
        pc.field = tokens[0];
        pc.op    = tokens[1];
        std::string rhs;
        for (size_t k = 2; k < tokens.size(); ++k) {
            if (k > 2) rhs += " ";
            rhs += tokens[k];
        }
        pc.rhs = stripQuotes(rhs);

        parsed_clauses_.push_back(std::move(pc));
    }
}


// GAP 3 FIX: Circular dependency prevention
bool EventTrigger::validateNoCycularDependencies() const {
    // Check if condition references the key or value field recursively
    // (self-referential conditions would cause infinite loops)
    if (!config_.condition || config_.condition->empty()) {
        return true;  // No condition = no circular dependency
    }
    
    const std::string& condition = *config_.condition;
    
    // Detect self-referential patterns:
    // - If key_prefix is "task:*" and condition contains "key == task:", potential cycle
    // - If condition field name appears in the RHS value, potential cycle
    
    // Simple heuristic: reject conditions where field name appears in RHS
    for (const auto& clause : parsed_clauses_) {
        // Check if field name appears in RHS (simple string match)
        if (clause.rhs.find(clause.field) != std::string::npos) {
            THEMIS_WARN("EventTrigger: possible circular dependency detected "
                       "(field='{}' appears in RHS='{}'), rejecting condition",
                       clause.field, clause.rhs);
            return false;
        }
    }
    
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

