/**
 * @file integrity_verification.h
 * @brief Integrity verification and Merkle / receipt-chain model.
 *
 * Provides content-addressed verification of tensor artifact stripes using
 * a Merkle tree structure and an append-only receipt chain for auditability.
 *
 * Planned in: docs/EPIC3_INTEGRITY_MODEL.md
 * Sub-issue:   #5432
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// Result of verifying a single stripe.
struct StripeVerification {
    std::string  artifact_id;
    std::uint32_t stripe_index = 0;
    std::string  expected_checksum;
    std::string  actual_checksum;
    bool         passed = false;
};

/// A node in the Merkle tree over an artifact's stripes.
struct MerkleNode {
    std::uint32_t depth  = 0;
    std::uint32_t index  = 0;  ///< Position at this depth
    std::string   hash;        ///< SHA-256 of left_hash || right_hash
    bool          is_leaf = false;
};

/// The Merkle root for one artifact.
struct MerkleRoot {
    std::string  artifact_id;
    std::string  root_hash;
    std::uint32_t num_leaves = 0;
    std::uint64_t epoch      = 0;
};

/// An immutable receipt appended to the chain after each commit.
struct IntegrityReceipt {
    std::string  receipt_id;      ///< Monotonically increasing ID
    std::string  artifact_id;
    std::string  merkle_root_hash;
    std::string  previous_receipt_id; ///< Chaining pointer
    std::string  signature;           ///< Signing node identity
};

/**
 * @brief Integrity verifier and receipt-chain manager.
 */
class IIntegrityVerifier {
public:
    virtual ~IIntegrityVerifier() = default;

    /// Compute the SHA-256 checksum of a byte buffer.
    virtual std::string checksum(const std::vector<std::uint8_t>& data) const = 0;

    /// Verify a single stripe against its expected checksum.
    virtual StripeVerification verifyStripe(
        const std::string& artifact_id,
        std::uint32_t stripe_index,
        const std::vector<std::uint8_t>& data,
        const std::string& expected_checksum) const = 0;

    /// Build a Merkle root from a set of stripe checksums.
    virtual MerkleRoot buildMerkleRoot(
        const std::string& artifact_id,
        const std::vector<std::string>& stripe_checksums,
        std::uint64_t epoch) const = 0;

    /// Append a receipt to the chain; returns the new receipt ID.
    virtual std::string appendReceipt(const std::string& artifact_id,
                                       const std::string& merkle_root_hash) = 0;

    /// Look up the latest receipt for an artifact.
    virtual std::optional<IntegrityReceipt> latestReceipt(
        const std::string& artifact_id) const = 0;

    /// Verify the full receipt chain for an artifact (detect tampering).
    virtual bool verifyChain(const std::string& artifact_id) const = 0;
};

/// Factory: create an in-memory integrity verifier.
std::unique_ptr<IIntegrityVerifier> makeIntegrityVerifier();

} // namespace themis::distributed_tensor
