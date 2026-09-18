/**
 * @file i_ssm_plugin.h
 * @brief SSM / Mamba backend plugin interface for ThemisDB LLM stack.
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL (Phase 1 PoC)
 * @note Status: Pending human architect approval (P1-D01 design gate)
 * @note This file is auto-generated and will be updated per design review feedback.
 * @note **Plugin Interface**: Abstract interface for SSM/Mamba backend implementations.
 *       No .cpp implementation needed. Implementations provided by plugin system.
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "core/timestamp.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::llm {

struct SSMStateSnapshot {
    core::HLCTimestamp snapshot_ts;

    std::vector<uint8_t> state_data;

    std::string state_fingerprint;

    uint64_t sequence_counter = 0;

    // Backwards-compatibility fields expected by older tests
    std::string session_id;
    std::string hidden_state; // string form of state_data for legacy tests
    std::string cell_state;
    std::string metadata; // JSON string metadata
};

struct ISSMPlugin : public ILLMPlugin {
    /**
     * @brief ISSMPlugin.
     * @return Return value.
     */
    virtual ~ISSMPlugin() = default;

    /**
     * @brief Update State.
     * @param[in] tokens Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool updateState(const std::vector<int32_t>& tokens) = 0;

    /**
     * @brief Get State Snapshot.
     * @param[in] snapshot_ts Input parameter.
     * @return Return value.
     */
    virtual SSMStateSnapshot getStateSnapshot(core::HLCTimestamp snapshot_ts) = 0;

    /**
     * @brief Restore State.
     * @param[in] snapshot Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool restoreState(const SSMStateSnapshot& snapshot) = 0;

    /**
     * @brief Reset State.
     */
    virtual void resetState() = 0;

    /**
     * @brief Get State Retention Score.
     * @return Return value.
     */
    virtual double getStateRetentionScore() const = 0;

    /**
     * @brief Get State Fingerprint.
     * @return Return value.
     */
    virtual std::string getStateFingerprint() const = 0;
};

}  // namespace themis::llm

