/**
 * @file ssm_stub_plugin.h
 * @brief Synthetic SSM stub plugin for Phase 1 dataflow validation.
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Gap Summary: Stub/Simulation marker
 * @note Status: Phase 1 PoC
 */

#pragma once

#include "llm/i_ssm_plugin.h"
#include "llm/ssm_state_store.h"

#include <random>

namespace themis::llm {

/**
 * @brief Synthetic SSM stub for dataflow validation (P1-D03).
 *
 * **STUB/SIMULATION NOTE:**
 * Purpose: Validate SSM plugin dataflow without a real Mamba model
 * Activation: Only when THEMIS_SSM_STUB_MODE=1 build flag set
 * Production Delta: Uses fixed random state (Seed=42); no real token processing
 * Removal Plan: Replace with real Mamba ISSMPlugin in Phase 2
 *
 * Implements the ISSMPlugin interface with deterministic synthetic state:
 * - updateState(): appends token count to fixed 128-dim hidden state buffer
 * - getStateSnapshot(): serializes current buffer with HLC timestamp
 * - restoreState(): deserializes and validates fingerprint
 * - getStateRetentionScore(): returns synthetic decay metric
 */
class SyntheticSSMStub : public ISSMPlugin {
public:
    SyntheticSSMStub();

    /// Plugin metadata
    std::string getName() const override { return "synthetic-ssm-stub"; }

    std::string getVersion() const override { return "0.1.0-alpha"; }

    std::string getDescription() const override {
        return "Synthetic SSM stub for Phase 1 dataflow validation (THEMIS_SSM_STUB_MODE)";
    }

    bool initialize() override;
    bool deinitialize() override;
    bool isAvailable() const override { return initialized_; }

    // SSM-specific operations
    bool updateState(const std::vector<int32_t>& tokens) override;
    SSMStateSnapshot getStateSnapshot(core::HLCTimestamp snapshot_ts) override;
    bool restoreState(const SSMStateSnapshot& snapshot) override;
    void resetState() override;
    double getStateRetentionScore() const override;
    std::string getStateFingerprint() const override;

private:
    static constexpr int HIDDEN_DIM = 128;
    static constexpr uint32_t STUB_SEED = 42;

    /// Hidden state buffer (fixed size for Phase 1)
    std::vector<float> hidden_state_;

    /// Token sequence counter
    uint64_t token_count_ = 0;

    /// Initialization flag
    bool initialized_ = false;

    /// RNG for decay metric
    std::mt19937 rng_;

    /// Model architecture fingerprint
    std::string fingerprint_;
};

}  // namespace themis::llm

