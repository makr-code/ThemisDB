/**
 * @file ssm_state_store.h
 * @brief SSM state persistence interface for ThemisDB LLM stack.
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL (Phase 1 PoC)
 * @note Status: In-memory implementation only
 * @note This file is auto-generated and will be updated per Phase 1 gates.
 */

#pragma once

#include "llm/i_ssm_plugin.h"
#include "storage/hlc.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace themis::llm {

struct ISSMStateStore {
    /**
     * @brief ISSMState Store.
     * @return Return value.
     */
    virtual ~ISSMStateStore() = default;

    /**
     * @brief Checkpoint.
     * @param[in] session_id Identifier of the session.
     * @param[in] snapshot Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool checkpoint(const std::string& session_id,
                            const SSMStateSnapshot& snapshot) = 0;

    virtual std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts = std::nullopt) = 0;

    /**
     * @brief Invalidate.
     * @param[in] session_id Identifier of the session.
     * @return True when the operation succeeds.
     */
    virtual bool invalidate(const std::string& session_id) = 0;

    /**
     * @brief Compact.
     * @param[in] retention_window_ms Input parameter.
     * @return Return value.
     */
    virtual uint64_t compact(uint64_t retention_window_ms) = 0;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    virtual std::string getStats() const = 0;
};

class InMemorySSMStateStore : public ISSMStateStore {
public:
    explicit InMemorySSMStateStore(size_t max_snapshots_per_session = 10);

    bool checkpoint(const std::string& session_id,
                    const SSMStateSnapshot& snapshot) override;

    std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts =
            std::nullopt) override;

    bool invalidate(const std::string& session_id) override;

    uint64_t compact(uint64_t retention_window_ms) override;

    std::string getStats() const override;

private:
    std::unordered_map<std::string, std::vector<SSMStateSnapshot>>
        state_by_session_;

    size_t max_snapshots_per_session_;

    mutable std::mutex mu_;
};

}  // namespace themis::llm

// Inline simple implementation for InMemorySSMStateStore (phase-1)
namespace themis::llm {

/**
 * @brief In Memory SSMState Store.
 * @param[in] max_snapshots_per_session Input parameter.
 * @return Return value.
 */
inline InMemorySSMStateStore::InMemorySSMStateStore(size_t max_snapshots_per_session)
    : max_snapshots_per_session_(max_snapshots_per_session) {}

/**
 * @brief Checkpoint.
 * @param[in] session_id Identifier of the session.
 * @param[in] snapshot Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lk(), push_back(), size(), erase(), begin().
 */
inline bool InMemorySSMStateStore::checkpoint(const std::string& session_id,
                                              const SSMStateSnapshot& snapshot) {
    std::lock_guard<std::mutex> lk(mu_);
    auto &vec = state_by_session_[session_id];
    // duplicate timestamp rejected
    for (const auto &s : vec) {
      if (s.snapshot_ts == snapshot.snapshot_ts) return false;
    }
    vec.push_back(snapshot);
    if (vec.size() > max_snapshots_per_session_) {
      vec.erase(vec.begin());
    }
    return true;
}

/**
 * @brief Resume.
 * @param[in] session_id Identifier of the session.
 * @param[in] snapshot_ts Input parameter.
 * @return Return value.
 * @details Calls: lk(), find(), end(), empty(), has_value(), value(), back().
 */
inline std::optional<SSMStateSnapshot> InMemorySSMStateStore::resume(
    const std::string& session_id,
    const std::optional<HLCTimestamp>& snapshot_ts) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = state_by_session_.find(session_id);
    if (it == state_by_session_.end() || it->second.empty()) {
      return std::nullopt;
    }
    if (snapshot_ts.has_value()) {
        for (const auto &s : it->second) {
          if (s.snapshot_ts == snapshot_ts.value()) return s;
        }
        return std::nullopt;
    }
    return it->second.back();
}

/**
 * @brief Invalidate.
 * @param[in] session_id Identifier of the session.
 * @return True when the operation succeeds.
 * @details Calls: lk(), erase().
 */
inline bool InMemorySSMStateStore::invalidate(const std::string& session_id) {
    std::lock_guard<std::mutex> lk(mu_);
    return state_by_session_.erase(session_id) > 0;
}

/**
 * @brief Compact.
 * @param[in] uint64_t Input parameter.
 * @return Return value.
 * @details Implements compact without additional internal calls.
 */
inline uint64_t InMemorySSMStateStore::compact(uint64_t /*retention_window_ms*/) {
    // Phase1: no-op
    return 0;
}

inline std::string InMemorySSMStateStore::getStats() const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    size_t sessions = state_by_session_.size();
    size_t total = 0;
    for (const auto &p : state_by_session_) {
      total += p.second.size();
    }
    return "{\"sessions\": " + std::to_string(sessions) + ", \"snapshots\": " + std::to_string(total) + "}";
}

} // namespace themis::llm

