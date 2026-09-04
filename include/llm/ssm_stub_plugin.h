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
    std::string getName() const { return "synthetic-ssm-stub"; }

    std::string getVersion() const { return "0.1.0-alpha"; }

    std::string getDescription() const {
        return "Synthetic SSM stub for Phase 1 dataflow validation (THEMIS_SSM_STUB_MODE)";
    }

    bool initialize();
    bool deinitialize();
    bool isAvailable() const { return initialized_; }

    // === Minimal ILLMPlugin stubs to make this class concrete for tests ===
    bool loadModel(const std::string& model_path, const json& config = {}) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override { return std::nullopt; }
    bool isModelLoaded() const override { return false; }

    bool loadLoRA(const std::string& lora_id, const std::string& lora_path, float scale = 1.0f) override { return false; }
    bool unloadLoRA(const std::string& lora_id) override { return false; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }

    InferenceResponse generate(const InferenceRequest& request) override { return {}; }
    InferenceResponse generateRAG(const RAGContext& rag_context, const InferenceRequest& request) override { return {}; }
    std::vector<float> embed(const std::string& text) override { return {}; }

    LLMCapabilities getCapabilities() const override { return LLMCapabilities(); }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }

    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override { return {}; }
    bool importLoRA(const std::string& lora_id, const std::vector<uint8_t>& data) override { return false; }

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
    std::string fingerprint_ = {};
};

}  // namespace themis::llm

// Inline implementations to avoid separate TU and satisfy unity builds
namespace themis::llm {

inline SyntheticSSMStub::SyntheticSSMStub()
    : hidden_state_(HIDDEN_DIM, 0.0f), rng_(STUB_SEED), fingerprint_("synthetic-ssm-v0") {}

inline bool SyntheticSSMStub::initialize() {
    initialized_ = true;
    token_count_ = 0;
    std::fill(hidden_state_.begin(), hidden_state_.end(), 0.0f);
    return true;
}

inline bool SyntheticSSMStub::updateState(const std::vector<int32_t>& tokens) {
    if (!initialized_) {
      return false;
    }
    for (auto t : tokens) {
        if (t < 0 || t > 1000000) return false; // simple validation
        size_t idx = static_cast<size_t>(token_count_ % HIDDEN_DIM);
        hidden_state_[idx] += static_cast<float>(t % 256) / 255.0f;
        token_count_++;
    }
    return true;
}

inline SSMStateSnapshot SyntheticSSMStub::getStateSnapshot(core::HLCTimestamp snapshot_ts) {
    SSMStateSnapshot snap;
    snap.snapshot_ts = snapshot_ts;
    snap.sequence_counter = token_count_;
    snap.state_fingerprint = fingerprint_;
    snap.state_data.resize(HIDDEN_DIM);
    for (int i = 0; i < HIDDEN_DIM; ++i) {
      snap.state_data[i] = static_cast<uint8_t>(hidden_state_[i] * 255.0f);
    }
    return snap;
}

inline bool SyntheticSSMStub::restoreState(const SSMStateSnapshot& snapshot) {
    if (snapshot.state_fingerprint != fingerprint_) {
      return false;
    }
    if (snapshot.state_data.size() < static_cast<size_t>(HIDDEN_DIM)) {
      return false;
    }
    for (int i = 0; i < HIDDEN_DIM; ++i) {
      hidden_state_[i] = static_cast<float>(snapshot.state_data[i]) / 255.0f;
    }
    token_count_ = snapshot.sequence_counter;
    return true;
}

inline void SyntheticSSMStub::resetState() {
    token_count_ = 0;
    std::fill(hidden_state_.begin(), hidden_state_.end(), 0.0f);
}

inline double SyntheticSSMStub::getStateRetentionScore() const {
    // simple decay: exp(-tokens/10000)
    const double decay = std::exp(-static_cast<double>(token_count_) / 10000.0);
    return std::min(1.0, std::max(0.0, decay));
}

inline std::string SyntheticSSMStub::getStateFingerprint() const { return fingerprint_; }

} // namespace themis::llm

