/**
 * @file ssm_stub_plugin.cpp
 * @brief Synthetic SSM stub plugin implementation (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Status: Stub/Simulation for Phase 1 dataflow validation
 */

#include "llm/ssm_stub_plugin.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace themis::llm {

SyntheticSSMStub::SyntheticSSMStub() : rng_(STUB_SEED) {
    // Generate fingerprint from seed and hidden dimension
    std::ostringstream oss;
    oss << "stub-v0.1-seed" << STUB_SEED << "-dim" << HIDDEN_DIM;
    fingerprint_ = oss.str();
}

bool SyntheticSSMStub::initialize() {
    if (initialized_) {
        return true;
    }

    // Initialize hidden state buffer with zeros
    hidden_state_.resize(HIDDEN_DIM, 0.0f);
    token_count_ = 0;
    initialized_ = true;

    return true;
}

bool SyntheticSSMStub::deinitialize() {
    hidden_state_.clear();
    token_count_ = 0;
    initialized_ = false;
    return true;
}

bool SyntheticSSMStub::updateState(const std::vector<int32_t>& tokens) {
    if (!initialized_) {
        return false;
    }

    if (tokens.empty()) {
        return true;
    }

    // Validate token IDs
    for (int32_t token : tokens) {
        if (token < 0 || token > 32000) {
            return false;  // Invalid token range
        }
    }

    // Synthetic state update: blend tokens into hidden state (deterministic)
    for (int32_t token : tokens) {
        // Distribute token value across hidden dimensions
        for (int i = 0; i < HIDDEN_DIM; ++i) {
            // Deterministic update with seed
            float contribution = std::sin(token * 0.1f + i * 0.01f) * 0.1f;
            hidden_state_[i] = (hidden_state_[i] * 0.95f) + contribution;
        }
        token_count_++;
    }

    return true;
}

SSMStateSnapshot SyntheticSSMStub::getStateSnapshot(
    core::HLCTimestamp snapshot_ts) {
    SSMStateSnapshot snap;
    snap.snapshot_ts = snapshot_ts;
    snap.state_fingerprint = fingerprint_;
    snap.sequence_counter = token_count_;

    // Serialize hidden state to bytes
    snap.state_data.resize(sizeof(float) * HIDDEN_DIM);
    std::memcpy(snap.state_data.data(), hidden_state_.data(),
                snap.state_data.size());

    return snap;
}

bool SyntheticSSMStub::restoreState(const SSMStateSnapshot& snapshot) {
    if (!initialized_) {
        return false;
    }

    // Validate fingerprint
    if (snapshot.state_fingerprint != fingerprint_) {
        return false;  // Model mismatch
    }

    // Validate size
    if (snapshot.state_data.size() != sizeof(float) * HIDDEN_DIM) {
        return false;  // Corrupted snapshot
    }

    // Restore state
    std::memcpy(hidden_state_.data(), snapshot.state_data.data(),
                snapshot.state_data.size());
    token_count_ = snapshot.sequence_counter;

    return true;
}

void SyntheticSSMStub::resetState() {
    std::fill(hidden_state_.begin(), hidden_state_.end(), 0.0f);
    token_count_ = 0;
}

double SyntheticSSMStub::getStateRetentionScore() const {
    if (!initialized_ || token_count_ == 0) {
        return 0.0;
    }

    // Synthetic decay: freshness decreases with token age
    // Max retention after 8192 tokens, then linear decay
    double tokens_normalized = std::min(token_count_ / 8192.0, 1.0);
    double retention = 1.0 - (0.2 * (1.0 - tokens_normalized));

    // Add small random jitter for realism
    std::uniform_real_distribution<double> dist(-0.05, 0.05);
    retention = std::clamp(retention + dist(rng_), 0.0, 1.0);

    return retention;
}

std::string SyntheticSSMStub::getStateFingerprint() const {
    return fingerprint_;
}

}  // namespace themis::llm

