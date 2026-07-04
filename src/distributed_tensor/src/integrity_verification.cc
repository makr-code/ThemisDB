/**
 * @file integrity_verification.cc
 * @brief Implementation of integrity verification, Merkle structures, and receipt semantics.
 */

#include "integrity_verification.h"

#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// ContentHash Implementation
// ============================================================================

bool ContentHash::isValid() const {
    // SHA-256 produces 32 bytes = 64 hex characters (lowercase)
    if (value.size() != 64) return false;
    for (char c : value) {
        if (!std::isxdigit(c)) return false;
        if (std::isalpha(c) && c != std::tolower(c)) return false;  // reject uppercase
    }
    return true;
}

// ============================================================================
// Merkle Proof Implementation
// ============================================================================

bool MerkleProof::verify(const std::string& expected_root) const {
    if (!isValidSHA256Hex(artifact_id) || artifact_id.empty()) {
        spdlog::warn("MerkleProof::verify: invalid artifact_id");
        return false;
    }
    if (!isValidSHA256Hex(fragment_hash) || fragment_hash.empty()) {
        spdlog::warn("MerkleProof::verify: invalid fragment_hash");
        return false;
    }
    if (!isValidSHA256Hex(artifact_root_hash) || artifact_root_hash.empty()) {
        spdlog::warn("MerkleProof::verify: invalid artifact_root_hash");
        return false;
    }

    // Start with fragment hash and work up the tree
    std::string current_hash = fragment_hash;

    for (const auto& component : proof_path) {
        if (!isValidSHA256Hex(component.sibling_hash)) {
            spdlog::warn("MerkleProof::verify: invalid sibling_hash in proof path");
            return false;
        }

        // Concatenate hashes in the correct order (left sibling first, then right)
        std::string concat;
        if (component.is_left) {
            concat = component.sibling_hash + current_hash;
        } else {
            concat = current_hash + component.sibling_hash;
        }

        // Compute hash of concatenation
        current_hash = computeSHA256(concat);
    }

    // Verify final hash matches expected root
    bool verified = (current_hash == artifact_root_hash) &&
                    (artifact_root_hash == expected_root);

    if (!verified) {
        spdlog::debug("MerkleProof::verify: computed root {} does not match expected {}",
                      current_hash, expected_root);
    }

    return verified;
}

// ============================================================================
// VerificationReceipt Implementation
// ============================================================================

std::string VerificationReceipt::computeContentHash() const {
    json j;
    j["receipt_id"] = receipt_id;
    j["artifact_id"] = artifact_id;
    j["content_hash"] = content_hash;
    j["timestamp"] = timestamp;
    j["parent_receipt_hash"] = parent_receipt_hash;
    j["package_lineage_hash"] = package_lineage_hash;
    j["shard_placement_id"] = shard_placement_id;
    j["metadata"] = metadata;

    return computeJSONHash(j);
}

bool VerificationReceipt::verifyIntegrity() const {
    return receipt_hash == computeContentHash();
}

// ============================================================================
// ReceiptChain Implementation
// ============================================================================

VerificationReceipt ReceiptChain::appendReceipt(VerificationReceipt receipt) {
    // Determine parent receipt hash
    std::string parent_hash;
    if (!receipts_.empty()) {
        parent_hash = receipts_.back().receipt_hash;
    }

    // Update receipt with parent linkage
    receipt.parent_receipt_hash = parent_hash;

    // Compute receipt hash
    receipt.receipt_hash = receipt.computeContentHash();

    // Append to chain
    receipts_.push_back(receipt);

    spdlog::debug("ReceiptChain::appendReceipt: appended receipt {} for artifact {}",
                  receipt.receipt_id, receipt.artifact_id);

    return receipt;
}

std::vector<VerificationReceipt> ReceiptChain::getAllReceipts() const {
    return receipts_;
}

std::optional<VerificationReceipt> ReceiptChain::getHeadReceipt() const {
    if (receipts_.empty()) return std::nullopt;
    return receipts_.back();
}

std::optional<VerificationReceipt> ReceiptChain::getGenesisReceipt() const {
    if (receipts_.empty()) return std::nullopt;
    return receipts_.front();
}

bool ReceiptChain::verifyChainIntegrity() const {
    if (receipts_.empty()) {
        // Empty chain is trivially valid
        return true;
    }

    // Verify the first (genesis) receipt has empty parent
    if (!receipts_.front().parent_receipt_hash.empty()) {
        spdlog::error("ReceiptChain::verifyChainIntegrity: genesis receipt has non-empty parent");
        return false;
    }

    // Verify entire chain linkage
    std::string expected_previous;
    for (size_t i = 0; i < receipts_.size(); ++i) {
        const auto& receipt = receipts_[i];

        // Verify parent linkage
        if (receipt.parent_receipt_hash != expected_previous) {
            spdlog::error("ReceiptChain::verifyChainIntegrity: "
                         "parent_receipt_hash mismatch at entry {} (expected {}, got {})",
                         i, expected_previous, receipt.parent_receipt_hash);
            return false;
        }

        // Verify receipt integrity
        if (!receipt.verifyIntegrity()) {
            spdlog::error("ReceiptChain::verifyChainIntegrity: "
                         "receipt_hash mismatch at entry {}", i);
            return false;
        }

        // Update expected parent for next iteration
        expected_previous = receipt.receipt_hash;
    }

    return true;
}

size_t ReceiptChain::size() const {
    return receipts_.size();
}

// ============================================================================
// VerificationState Utilities
// ============================================================================

std::string verificationStateToString(VerificationState state) {
    switch (state) {
        case VerificationState::UNVERIFIED:
            return "UNVERIFIED";
        case VerificationState::VERIFIED:
            return "VERIFIED";
        case VerificationState::VERIFIED_FRAGMENTS:
            return "VERIFIED_FRAGMENTS";
        case VerificationState::CORRUPT:
            return "CORRUPT";
        case VerificationState::STALE:
            return "STALE";
        default:
            return "UNKNOWN";
    }
}

std::optional<VerificationState> stringToVerificationState(
    const std::string& s) {
    if (s == "UNVERIFIED") return VerificationState::UNVERIFIED;
    if (s == "VERIFIED") return VerificationState::VERIFIED;
    if (s == "VERIFIED_FRAGMENTS") return VerificationState::VERIFIED_FRAGMENTS;
    if (s == "CORRUPT") return VerificationState::CORRUPT;
    if (s == "STALE") return VerificationState::STALE;
    return std::nullopt;
}

// ============================================================================
// VerificationResult Implementation
// ============================================================================

json VerificationResult::toJSON() const {
    json j;
    j["success"] = success;
    j["state"] = verificationStateToString(state);
    j["artifact_id"] = artifact_id;
    j["expected_hash"] = expected_hash;
    j["actual_hash"] = actual_hash;
    j["error_messages"] = error_messages;
    j["fragments_verified"] = fragments_verified;
    j["metadata"] = metadata;
    return j;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string computeSHA256(const std::string& data) {
    return computeSHA256(std::string_view(data));
}

std::string computeSHA256(std::string_view data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash, &sha256);

    // Convert to hex string (lowercase)
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(hash[i]);
    }

    return oss.str();
}

std::string computeJSONHash(const json& j) {
    // Deterministic JSON serialization (sorted keys, no whitespace)
    std::string canonical = j.dump(
        -1,  // compact output
        ' ', // indent (not used with -1)
        false,  // ensure ASCII output
        json::error_handler_t::strict
    );

    return computeSHA256(canonical);
}

bool isValidSHA256Hex(std::string_view hex_str) {
    // SHA-256 produces 32 bytes = 64 hex characters
    if (hex_str.size() != 64) return false;

    for (char c : hex_str) {
        // Must be lowercase hex digit
        if (!std::isxdigit(c)) return false;
        if (std::isalpha(c) && c != std::tolower(c)) return false;
    }

    return true;
}

}  // namespace distributed_tensor
}  // namespace themis
