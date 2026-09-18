/**
 * @file integrity_verification.h
 * @brief Integrity verification, Merkle structures, and receipt semantics for distributed tensor artifacts.
 *
 * This header defines the integrity and verification model for distributed tensor artifacts,
 * including Merkle-compatible fragment verification and receipt-chain-aligned metadata.
 *
 * **Purpose:**
 * Distributed tensor artifacts must be verifiable across shards and across rebuild workflows.
 * This module provides cryptographic verification hooks for detecting corruption, verifying
 * fragment membership, proving package lineage, and supporting audit trails.
 *
 * **Scope:**
 * - Content-hash model for artifact verification
 * - Merkle tree structures for fragment-level verification
 * - Receipt-chain semantics compatible with package lineage tracking
 * - Provenance verification hooks and audit trail support
 * - Verification state machine and failure handling
 *
 * **Design Principles:**
 * 1. All integrity metadata is manifest-embedded (no separate verification storage)
 * 2. Merkle proofs are lazily computed on demand (not pre-stored)
 * 3. Receipt chains link to package lineage, not isolated artifact chains
 * 4. Verification state is ephemeral; artifacts are trusted until proven otherwise
 * 5. Corruption detection is automatic on load; tampering requires active verification
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace distributed_tensor {

using json = nlohmann::json;

// ============================================================================
// Content Hash Model
// ============================================================================

struct ContentHash {
    std::string value;  ///< SHA-256 hash as lowercase hex string (64 chars)

    [[nodiscard]] bool isValid() const;

    bool operator==(const ContentHash& other) const {
        return value == other.value;
    }

    bool operator!=(const ContentHash& other) const {
        return value != other.value;
    }
};

// ============================================================================
// Merkle Fragment Verification Model
// ============================================================================

struct MerkleProofComponent {
    std::string sibling_hash;  ///< Sibling node hash in the proof path
    bool        is_left = false; ///< true if sibling is to the left; false if right

    [[nodiscard]] json toJSON() const;

    [[nodiscard]] static std::optional<MerkleProofComponent> fromJSON(
        const json& j);
};

struct MerkleProof {
    std::string          artifact_id;      ///< Artifact this proof belongs to
    uint64_t             fragment_index = 0; ///< Index of fragment in shard placement
    std::string          fragment_hash;    ///< SHA-256 of fragment content
    std::vector<MerkleProofComponent> proof_path;  ///< Path from leaf to root
    std::string          artifact_root_hash;       ///< Expected root hash for verification
    std::string          root_hash;                ///< Compatibility alias for artifact_root_hash

    [[nodiscard]] bool verify(const std::string& expected_root) const;

    [[nodiscard]] size_t verificationCost() const { return static_cast<int>(proof_path.size()); }

    // Compatibility public member `root_hash` exists for tests and older APIs.

    [[nodiscard]] size_t getProofDepth() const { return verificationCost(); }

    [[nodiscard]] json toJSON() const;

    [[nodiscard]] static std::optional<MerkleProof> fromJSON(const json& j);
};

// ============================================================================
// Receipt-Chain Verification Model
// ============================================================================

struct VerificationReceipt {
    std::string receipt_id;              ///< Unique receipt identifier (UUID)
    std::string artifact_id;             ///< Artifact this receipt certifies
    std::string content_hash;            ///< SHA-256 of artifact content at receipt time
    std::string timestamp;               ///< ISO 8601 UTC timestamp of receipt creation
    std::string parent_receipt_hash;     ///< SHA-256 of previous receipt in chain (empty for genesis)
    std::string receipt_hash;            ///< SHA-256 of this receipt's canonical form

    // Provenance linkage
    std::string package_lineage_hash;    ///< Hash linking to package rebuild source
    std::string shard_placement_id;      ///< Placement strategy used at receipt time
    json        metadata;                ///< Additional certification metadata

    [[nodiscard]] std::string computeContentHash() const;

    [[nodiscard]] bool verifyIntegrity() const;

    [[nodiscard]] json toJSON() const;

    [[nodiscard]] static std::optional<VerificationReceipt> fromJSON(
        const json& j);
};

class ReceiptChain {
public:
    ReceiptChain() = default;

    /**
     * @brief Append Receipt.
     * @param[in] receipt Input parameter.
     * @return Return value.
     */
    VerificationReceipt appendReceipt(VerificationReceipt receipt);

    [[nodiscard]] std::vector<VerificationReceipt> getAllReceipts() const;

    [[nodiscard]] std::optional<VerificationReceipt> getHeadReceipt() const;

    [[nodiscard]] std::optional<VerificationReceipt> getGenesisReceipt() const;

    [[nodiscard]] bool verifyChainIntegrity() const;

    [[nodiscard]] size_t size() const;

    [[nodiscard]] bool empty() const { return size() == 0; }

    [[nodiscard]] json toJSON() const;

    [[nodiscard]] json toManifestMetadataJSON() const;

    [[nodiscard]] static std::optional<ReceiptChain> fromJSON(const json& j);

private:
    std::vector<VerificationReceipt> receipts_;  ///< Receipts from oldest to newest
};

// ============================================================================
// Verification State Machine
// ============================================================================

enum class VerificationState {
    UNVERIFIED = 0,         ///< Initial state; integrity not yet checked
    VERIFIED = 1,           ///< Content hash verified; full artifact integrity confirmed
    VERIFIED_FRAGMENTS = 2, ///< Fragment-level Merkle proofs verified
    CORRUPT = 3,            ///< Integrity check failed; artifact corrupted or tampered
    STALE = 4               ///< Content OK but provenance outdated (rebuild recommended)
};

[[nodiscard]] std::string verificationStateToString(VerificationState state);

[[nodiscard]] std::optional<VerificationState> stringToVerificationState(
    const std::string& s);

// ============================================================================
// Verification Result
// ============================================================================

struct VerificationResult {
    bool                     success = false;     ///< true if verification passed
    VerificationState        state = VerificationState::UNVERIFIED;
    std::string              artifact_id;         ///< Artifact that was verified
    std::string              expected_hash;       ///< Expected content hash
    std::string              actual_hash;         ///< Actual content hash (if computed)
    std::vector<std::string> error_messages;      ///< Diagnostic messages on failure
    size_t                   fragments_verified = 0; ///< Number of fragments verified
    json                     metadata;            ///< Additional verification metadata

    [[nodiscard]] json toJSON() const;
};

// ============================================================================
// Provenance Verification Hooks
// ============================================================================

class ProvenanceVerificationHook {
public:
    /**
     * @brief Provenance Verification Hook.
     * @return Return value.
     */
    virtual ~ProvenanceVerificationHook() = default;

    /**
     * @brief Verify Provenance.
     * @param[in] artifact_id Identifier of the artifact.
     * @param[in] content_hash Input parameter.
     * @param[in] package_lineage_hash Input parameter.
     * @return Return value.
     */
    virtual VerificationResult verifyProvenance(
        const std::string& artifact_id,
        const std::string& content_hash,
        const std::string& package_lineage_hash) = 0;

    /**
     * @brief Get Current Package Lineage.
     * @return Return value.
     */
    virtual std::string getCurrentPackageLineage() = 0;
};

class VerificationAuditTrail {
public:
    /**
     * @brief Verification Audit Trail.
     * @return Return value.
     */
    virtual ~VerificationAuditTrail() = default;

    /**
     * @brief Record Verification Event.
     * @param[in] artifact_id Identifier of the artifact.
     * @param[in] result Input parameter.
     * @param[in] timestamp Input parameter.
     */
    virtual void recordVerificationEvent(
        const std::string& artifact_id,
        const VerificationResult& result,
        const std::string& timestamp) = 0;
};

// ============================================================================
// PHASE 3: Error Handling & Edge Cases
// ============================================================================

[[nodiscard]] VerificationResult verifyArtifactIntegrity(
    const std::string& artifact_id,
    std::string_view payload,
    const std::string& expected_content_hash,
    const std::optional<MerkleProof>& merkle_proof = std::nullopt,
    const std::optional<ReceiptChain>& receipt_chain = std::nullopt,
    ProvenanceVerificationHook* provenance_hook = nullptr);

[[nodiscard]] VerificationResult detectReceiptChainTampering(
    const ReceiptChain& chain);

[[nodiscard]] VerificationResult handlePartialReceiptChain(
    const ReceiptChain& partial_chain,
    const std::string& artifact_id,
    const std::string& current_hash);

[[nodiscard]] VerificationResult handleStaleReceipt(
    const VerificationReceipt& head_receipt,
    const std::string& current_lineage_hash,
    const std::string& content_hash);

[[nodiscard]] VerificationResult verifyFragmentIntegrity(
    const std::string& artifact_id,
    std::string_view fragment_data,
    size_t fragment_index,
    const MerkleProof& merkle_proof,
    const std::string& expected_root);

class IntegrityRecoveryHook {
public:
    /**
     * @brief Integrity Recovery Hook.
     * @return Return value.
     */
    virtual ~IntegrityRecoveryHook() = default;

    /**
     * @brief Request Artifact Recovery.
     * @param[in] artifact_id Identifier of the artifact.
     * @param[in] reason Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool requestArtifactRecovery(
        const std::string& artifact_id,
        const std::string& reason) = 0;

    /**
     * @brief Request Chain Rebuild.
     * @param[in] artifact_id Identifier of the artifact.
     * @return True when the operation succeeds.
     */
    virtual bool requestChainRebuild(const std::string& artifact_id) = 0;

    /**
     * @brief Get Recovery Status.
     * @param[in] artifact_id Identifier of the artifact.
     * @return Return value.
     */
    virtual std::string getRecoveryStatus(const std::string& artifact_id) = 0;
};

/**
 * @brief Set Integrity Recovery Hook.
 * @param[in,out] hook Input/output parameter.
 */
void setIntegrityRecoveryHook(IntegrityRecoveryHook* hook);

/**
 * @brief Get Integrity Recovery Hook.
 * @return Pointer to the result.
 */
IntegrityRecoveryHook* getIntegrityRecoveryHook();

// ============================================================================
// Utility Functions
// ============================================================================

[[nodiscard]] std::string computeSHA256(const std::string& data);

[[nodiscard]] std::string computeSHA256(std::string_view data);

[[nodiscard]] std::string computeSHA256(const char* data);

[[nodiscard]] std::string computeJSONHash(const json& j);

[[nodiscard]] bool isValidSHA256Hex(std::string_view hex_str);

}  // namespace distributed_tensor
}  // namespace themis
