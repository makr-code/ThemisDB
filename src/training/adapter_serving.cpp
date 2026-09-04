/**
 * @file adapter_serving.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/adapter_serving.h"
#include "utils/checksum_utils.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

// This translation unit anchors the out-of-line members for ILLMRouter.
// All logic lives in the header / in IncrementalLoRATrainer::Impl.

namespace themis {
namespace training {

// Provide an out-of-line body so that the vtable is emitted exactly once.
// The destructor body is intentionally empty; derived classes clean up their
// own state.
ILLMRouter::~ILLMRouter() {}

// ============================================================================
// Phase 2: Deployment Validation Implementation
// ============================================================================

std::string validateDeploymentReadiness(
    const std::string& adapter_version,
    const std::string& checkpoint_path,
    const std::string& expected_sha256) {
    
    // Validate version string: non-empty, contains only alphanumeric, dots, underscores
    if (adapter_version.empty()) {
        return "Adapter version cannot be empty";
    }
    
    if (adapter_version.length() > 256) {
        return "Adapter version string too long (max 256 chars)";
    }
    
    for (char c : adapter_version) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != '.' && c != '_' && c != '-') {
            return "Adapter version contains invalid character: " + std::string(1, c);
        }
    }
    
    // Validate checkpoint path: must exist and be readable
    if (checkpoint_path.empty()) {
        return "Checkpoint path cannot be empty";
    }
    
    std::ifstream f(checkpoint_path, std::ios::binary);
    if (!f.is_open()) {
        return "Checkpoint file not found or not readable: " + checkpoint_path;
    }
    
    // Check minimum file size (at least 1KB)
    f.seekg(0, std::ios::end);
    std::streamoff file_size_raw = f.tellg();
    f.close();
    
    if (file_size_raw < 0) {
        return "Failed to determine checkpoint file size";
    }
    size_t file_size = static_cast<size_t>(file_size_raw);
    
    if (file_size < 1024) {
        return "Checkpoint file too small (" + std::to_string(file_size) + 
               " bytes, minimum 1024 required)";
    }
    
    // If expected SHA-256 is provided, verify it
    if (!expected_sha256.empty()) {
        std::string actual_sha256 = utils::calculateSHA256(checkpoint_path);
        if (actual_sha256.empty()) {
            return "Failed to compute SHA-256 of checkpoint file";
        }
        
        if (actual_sha256 != expected_sha256) {
            return "Checkpoint SHA-256 mismatch: expected " + expected_sha256 +
                   ", got " + actual_sha256;
        }
    }
    
    return ""; // Valid
}

std::string computeDeploymentFingerprint(
    const std::string& adapter_version,
    const std::string& checkpoint_sha256) {
    
    // Combine version and checkpoint hash for deterministic fingerprint.
    // Uses a FNV-1a-style 64-bit hash for a fast, non-cryptographic fingerprint.
    // Callers requiring cryptographic guarantees should use utils::calculateSHA256
    // directly on the checkpoint file.
    std::string combined = adapter_version + "::" + checkpoint_sha256;
    uint64_t hash = 0;
    const uint64_t kPrime = 1099511628211ull;
    const uint64_t kOffset = 1469598103934665603ull;
    
    hash = kOffset;
    for (unsigned char c : combined) {
        hash ^= static_cast<uint64_t>(c);
        hash *= kPrime;
    }
    
    // Convert to hex string (first 16 chars of 64-bit fingerprint)
    std::ostringstream oss = {};
    oss << std::hex << hash;
    return oss.str();
}

} // namespace training
} // namespace themis
