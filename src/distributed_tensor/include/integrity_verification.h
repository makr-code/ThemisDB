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

/**
 * @brief Cryptographic hash of artifact content for integrity detection.
 *
 * Uses SHA-256 for deterministic, collision-resistant identification of
 * artifact content. Hex-encoded 64-character string (lowercase).
 *
 * Content hash covers:
 * - Raw tensor payload bytes (if available)
 * - Serialized metadata (deterministic JSON order)
 * - Reconstruction instructions (if present)
 *
 * Content hash does NOT cover:
 * - Provenance references (which may be updated)
 * - Manifest metadata (versioning, timestamps)
 * - Fragment placement (determined at runtime)
 */
struct ContentHash {
    std::string value;  ///< SHA-256 hash as lowercase hex string (64 chars)

    /**
     * @brief Check if this hash represents valid content.
     * @return true if value is a valid 64-character hex string
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief Check if two hashes are identical.
     */
    bool operator==(const ContentHash& other) const {
        return value == other.value;
    }

    /**
     * @brief Check if two hashes differ.
     */
    bool operator!=(const ContentHash& other) const {
        return value != other.value;
    }
};

// ============================================================================
// Merkle Fragment Verification Model
// ============================================================================

/**
 * @brief Path component in a Merkle tree proving membership of a fragment.
 *
 * Used to reconstruct the root hash from a leaf fragment hash.
 * Direction indicates whether to append left or right during hash computation.
 *
 * **Merkle Semantics:**
 * - Leaf: SHA-256(fragment_content)
 * - Parent: SHA-256(left_child || right_child)
 * - Root: Full artifact integrity hash
 */
struct MerkleProofComponent {
    std::string sibling_hash;  ///< Sibling node hash in the proof path
    bool        is_left = false; ///< true if sibling is to the left; false if right

    /**
     * @brief Convert this proof component into manifest-safe JSON.
     * @return JSON object with sibling hash and path direction
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Parse a proof component from manifest JSON.
     *
     * @param j JSON object containing `sibling_hash` and `is_left`
     * @return Parsed proof component, or `std::nullopt` when the payload is invalid
     */
    [[nodiscard]] static std::optional<MerkleProofComponent> fromJSON(
        const json& j);
};

/**
 * @brief Merkle proof proving membership of a tensor fragment in an artifact.
 *
 * A Merkle proof is a minimal set of hashes sufficient to verify that a
 * specific fragment belongs to a larger artifact without downloading the
 * entire artifact.
 *
 * **Fragment Verification Workflow:**
 * 1. Compute leaf_hash from fragment content
 * 2. For each component in proof_path (leaf to root):
 *    - Combine current hash with sibling according to is_left direction
 *    - Hash the concatenation: current_hash := SHA-256(concat)
 * 3. Verify that final current_hash == artifact_root_hash
 *
 * **Performance:**
 * - Proof size: O(log N) where N is number of fragments
 * - Verification cost: O(log N) hash computations
 * - No download of unverified fragments
 */
struct MerkleProof {
    std::string          artifact_id;      ///< Artifact this proof belongs to
    uint64_t             fragment_index = 0; ///< Index of fragment in shard placement
    std::string          fragment_hash;    ///< SHA-256 of fragment content
    std::vector<MerkleProofComponent> proof_path;  ///< Path from leaf to root
    std::string          artifact_root_hash;       ///< Expected root hash for verification
    std::string          root_hash;                ///< Compatibility alias for artifact_root_hash

    /**
     * @brief Verify this proof against an artifact root hash.
     *
     * @param expected_root The artifact's published root hash
     * @return true if proof is valid and fragment is verified to belong to artifact
     */
    [[nodiscard]] bool verify(const std::string& expected_root) const;

    /**
     * @brief Get the number of hash operations required to verify this proof.
     * @return Depth of the proof path (O(log N))
     */
    [[nodiscard]] size_t verificationCost() const { return proof_path.size(); }

    // Compatibility public member `root_hash` exists for tests and older APIs.

    /**
     * Compatibility method matching older API name `getProofDepth()`.
     */
    [[nodiscard]] size_t getProofDepth() const { return verificationCost(); }

    /**
     * @brief Convert this proof into manifest-safe JSON.
     * @return JSON object containing fragment identity, proof path, and expected root
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Parse a Merkle proof from manifest JSON.
     *
     * @param j JSON object produced by toJSON()
     * @return Parsed proof, or `std::nullopt` when required fields are missing or malformed
     */
    [[nodiscard]] static std::optional<MerkleProof> fromJSON(const json& j);
};

// ============================================================================
// Receipt-Chain Verification Model
// ============================================================================

/**
 * @brief Verification receipt proving artifact integrity at a point in time.
 *
 * A receipt is an immutable record linking an artifact to:
 * - Its content hash (integrity proof)
 * - Its package lineage (provenance link)
 * - A timestamp (temporal anchor)
 * - Prior receipts (chain formation)
 *
 * **Receipt-Chain Semantics:**
 * - Genesis receipt: parent_receipt_hash is empty
 * - Chained receipts: form a tamper-evident linked list
 * - Each receipt commits to its content_hash and previous receipt
 * - Verification involves checking the entire chain back to genesis
 *
 * **Use Cases:**
 * 1. Prove artifact state at a historical point (e.g., "was this version available on date X?")
 * 2. Detect when an artifact was replaced or modified
 * 3. Link artifact to package rebuild events
 * 4. Support audit trails and compliance queries
 */
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

    /**
     * @brief Compute the canonical SHA-256 hash of this receipt's content.
     *
     * Hash covers all fields except receipt_hash itself (to avoid circular dependency).
     * Includes parent_receipt_hash to ensure chain integrity.
     *
     * @return 64-character lowercase hex SHA-256 hash
     */
    [[nodiscard]] std::string computeContentHash() const;

    /**
     * @brief Verify that this receipt's content_hash matches the canonical value.
     * @return true if receipt_hash matches computeContentHash()
     */
    [[nodiscard]] bool verifyIntegrity() const;

    /**
     * @brief Convert this receipt into manifest-safe JSON.
     *
     * @return JSON object containing all receipt fields, including `receipt_hash`
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Parse a receipt from manifest JSON.
     *
     * @param j JSON object containing receipt fields
     * @return Parsed receipt, or `std::nullopt` when required fields are invalid
     */
    [[nodiscard]] static std::optional<VerificationReceipt> fromJSON(
        const json& j);
};

/**
 * @brief Receipt chain linking artifact history via cryptographic commitments.
 *
 * A receipt chain is a singly-linked list of VerificationReceipts, each
 * committing to the previous receipt's hash. The chain enables:
 * - Detecting when an artifact was modified or replaced
 * - Verifying artifact state at historical points
 * - Linking artifacts to package lineage events
 * - Supporting compliance and audit queries
 *
 * **Chain Properties:**
 * - Head: Most recent receipt (youngest)
 * - Tail: Genesis receipt (oldest)
 * - Each link: Tamper-evident (receipt_hash includes parent_receipt_hash)
 * - Verification: Check entire chain back to genesis receipt
 */
class ReceiptChain {
public:
    /**
     * @brief Create an empty (genesis) receipt chain.
     */
    ReceiptChain() = default;

    /**
     * @brief Append a new receipt to the chain.
     *
     * Automatically computes receipt_hash and parent_receipt_hash linking.
     *
     * @param receipt  Populated receipt (receipt_hash may be empty — it will be computed)
     * @return Updated receipt with receipt_hash and parent_receipt_hash set
     */
    VerificationReceipt appendReceipt(VerificationReceipt receipt);

    /**
     * @brief Get all receipts in the chain (oldest first).
     * @return Vector of receipts from genesis to head
     */
    [[nodiscard]] std::vector<VerificationReceipt> getAllReceipts() const;

    /**
     * @brief Get the most recent receipt in the chain.
     * @return Head receipt, or empty if chain is empty
     */
    [[nodiscard]] std::optional<VerificationReceipt> getHeadReceipt() const;

    /**
     * @brief Get the genesis (oldest) receipt in the chain.
     * @return Genesis receipt, or empty if chain is empty
     */
    [[nodiscard]] std::optional<VerificationReceipt> getGenesisReceipt() const;

    /**
     * @brief Verify the integrity of the entire receipt chain.
     *
     * Recomputes each receipt_hash and verifies parent_receipt_hash linkage.
     * Ensures chain is tamper-evident from genesis to head.
     *
     * @return true when entire chain is intact and unmodified
     */
    [[nodiscard]] bool verifyChainIntegrity() const;

    /**
     * @brief Get the number of receipts in the chain.
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Check if chain is empty (no receipts appended).
     */
    [[nodiscard]] bool empty() const { return size() == 0; }

    /**
     * @brief Serialize the entire chain for manifest or audit storage.
     *
     * Produces an object with a `receipts` array and summary metadata so callers
     * can persist the head receipt while still retaining chain statistics.
     *
     * @return JSON object describing the full chain
     */
    [[nodiscard]] json toJSON() const;

    /**
     * @brief Serialize only manifest-facing chain statistics.
     *
     * This compact payload is intended for `receipt_chain_metadata`-style fields
     * where callers need chain size and endpoints without embedding all receipts.
     *
     * @return JSON object containing chain size plus head/genesis identifiers
     */
    [[nodiscard]] json toManifestMetadataJSON() const;

    /**
     * @brief Parse a receipt chain from serialized JSON.
     *
     * Accepts either the object produced by toJSON() or a bare receipt array.
     *
     * @param j JSON receipt-chain payload
     * @return Parsed chain, or `std::nullopt` when payload integrity fails
     */
    [[nodiscard]] static std::optional<ReceiptChain> fromJSON(const json& j);

private:
    std::vector<VerificationReceipt> receipts_;  ///< Receipts from oldest to newest
};

// ============================================================================
// Verification State Machine
// ============================================================================

/**
 * @brief Verification state of an artifact at a point in verification workflow.
 *
 * **State Transitions:**
 * - UNVERIFIED: Artifact loaded from storage, integrity unknown
 * - VERIFIED: Content-hash matches; fragment-level proof may be pending
 * - VERIFIED_FRAGMENTS: All required fragments verified via Merkle proofs
 * - CORRUPT: Integrity check failed; artifact should not be used
 * - STALE: Content-hash OK but provenance is outdated
 */
enum class VerificationState {
    UNVERIFIED = 0,         ///< Initial state; integrity not yet checked
    VERIFIED = 1,           ///< Content hash verified; full artifact integrity confirmed
    VERIFIED_FRAGMENTS = 2, ///< Fragment-level Merkle proofs verified
    CORRUPT = 3,            ///< Integrity check failed; artifact corrupted or tampered
    STALE = 4               ///< Content OK but provenance outdated (rebuild recommended)
};

/**
 * @brief Conversion from VerificationState enum to human-readable string.
 */
[[nodiscard]] std::string verificationStateToString(VerificationState state);

/**
 * @brief Conversion from human-readable string to VerificationState enum.
 */
[[nodiscard]] std::optional<VerificationState> stringToVerificationState(
    const std::string& s);

// ============================================================================
// Verification Result
// ============================================================================

/**
 * @brief Result of an integrity verification operation.
 *
 * Returned by verification functions to indicate success/failure and
 * provide diagnostic details for debugging or remediation.
 */
struct VerificationResult {
    bool                     success = false;     ///< true if verification passed
    VerificationState        state = VerificationState::UNVERIFIED;
    std::string              artifact_id;         ///< Artifact that was verified
    std::string              expected_hash;       ///< Expected content hash
    std::string              actual_hash;         ///< Actual content hash (if computed)
    std::vector<std::string> error_messages;      ///< Diagnostic messages on failure
    size_t                   fragments_verified = 0; ///< Number of fragments verified
    json                     metadata;            ///< Additional verification metadata

    /**
     * @brief Convert to JSON for serialization/logging.
     */
    [[nodiscard]] json toJSON() const;
};

// ============================================================================
// Provenance Verification Hooks
// ============================================================================

/**
 * @brief Hook point for provenance-aware integrity verification.
 *
 * Allows artifact verification to integrate with package lineage and
 * graph provenance validation without circular dependencies.
 *
 * **Implementation Pattern:**
 * - Integrity verification computes artifact_id and content_hash
 * - Calls provenance hook to check package lineage compatibility
 * - Provenance hook returns compatibility status and rebuild recommendations
 * - Verification result reflects both integrity and provenance state
 */
class ProvenanceVerificationHook {
public:
    virtual ~ProvenanceVerificationHook() = default;

    /**
     * @brief Verify that artifact provenance is compatible with current package lineage.
     *
     * @param artifact_id              Artifact being verified
     * @param content_hash             Artifact content hash
     * @param package_lineage_hash     Expected package lineage hash
     * @return Compatibility status and rebuild recommendations
     */
    virtual VerificationResult verifyProvenance(
        const std::string& artifact_id,
        const std::string& content_hash,
        const std::string& package_lineage_hash) = 0;

    /**
     * @brief Get the current package lineage hash.
     *
     * Used by integrity verification to determine expected lineage.
     *
     * @return Current lineage hash, or empty if unavailable
     */
    virtual std::string getCurrentPackageLineage() = 0;
};

/**
 * @brief Audit trail callback for recording verification events.
 *
 * Allows integrity verification to record results for later audit and compliance.
 */
class VerificationAuditTrail {
public:
    virtual ~VerificationAuditTrail() = default;

    /**
     * @brief Record a verification event.
     *
     * @param artifact_id  Artifact that was verified
     * @param result       Result of verification
     * @param timestamp    When verification occurred (ISO 8601 UTC)
     */
    virtual void recordVerificationEvent(
        const std::string& artifact_id,
        const VerificationResult& result,
        const std::string& timestamp) = 0;
};

// ============================================================================
// PHASE 3: Error Handling & Edge Cases
// ============================================================================

/**
 * @brief Full artifact verification with comprehensive corruption detection.
 *
 * Performs end-to-end integrity verification including:
 * - Content hash validation against expected value
 * - Merkle fragment verification for partial loads
 * - Receipt chain tamper-evidence checks
 * - Provenance lineage validation
 *
 * @param artifact_id         Identifier of artifact being verified
 * @param payload             Raw artifact payload bytes
 * @param expected_content_hash  Expected SHA-256 hash of payload
 * @param merkle_proof        Optional proof for fragment verification
 * @param receipt_chain       Optional chain to verify lineage
 * @param provenance_hook     Optional hook for graph validation
 * @return VerificationResult with success status and diagnostics
 */
[[nodiscard]] VerificationResult verifyArtifactIntegrity(
    const std::string& artifact_id,
    std::string_view payload,
    const std::string& expected_content_hash,
    const std::optional<MerkleProof>& merkle_proof = std::nullopt,
    const std::optional<ReceiptChain>& receipt_chain = std::nullopt,
    ProvenanceVerificationHook* provenance_hook = nullptr);

/**
 * @brief Detect tampering in receipt chains.
 *
 * Comprehensive tamper detection that verifies:
 * - Each receipt's self-hash integrity
 * - Parent-child linkage (receipt_hash includes parent_receipt_hash)
 * - Genesis receipt properties (empty parent)
 * - Chain continuity (no missing receipts)
 * - Lineage commitments are consistent
 *
 * @param chain              Receipt chain to analyze
 * @return VerificationResult with tamper status; CORRUPT if any issues detected
 */
[[nodiscard]] VerificationResult detectReceiptChainTampering(
    const ReceiptChain& chain);

/**
 * @brief Handle partial receipt chains (incomplete history).
 *
 * When a receipt chain is truncated or incomplete, determine:
 * - Whether the present receipt is still trustworthy
 * - How many historical receipts are missing
 * - Whether fragment-level verification is sufficient fallback
 * - Recovery recommendations (rebuild full chain or accept partial trust)
 *
 * @param partial_chain      Incomplete receipt chain
 * @param artifact_id        Artifact this chain certifies
 * @param current_hash       Current content hash of artifact
 * @return VerificationResult with STALE state if history incomplete,
 *         or VERIFIED if current receipt is sufficient
 */
[[nodiscard]] VerificationResult handlePartialReceiptChain(
    const ReceiptChain& partial_chain,
    const std::string& artifact_id,
    const std::string& current_hash);

/**
 * @brief Handle stale receipts (outdated provenance).
 *
 * When artifact content is valid but package lineage has changed since
 * receipt was issued, determine:
 * - Whether stale provenance requires rebuild
 * - Whether cached result is still acceptable for specific query types
 * - Rebuild priority if needed
 *
 * @param head_receipt       Most recent receipt in chain
 * @param current_lineage_hash  Current package lineage hash
 * @param content_hash       Current artifact content hash (verified)
 * @return VerificationResult with state=STALE if lineage mismatch,
 *         state=VERIFIED if lineage match, or CORRUPT on conflict
 */
[[nodiscard]] VerificationResult handleStaleReceipt(
    const VerificationReceipt& head_receipt,
    const std::string& current_lineage_hash,
    const std::string& content_hash);

/**
 * @brief Verify fragment-level integrity via Merkle proofs.
 *
 * When only a subset of artifact fragments are loaded, use Merkle proofs
 * to verify:
 * - Fragment belongs to claimed artifact
 * - Fragment hash is consistent with full artifact root
 * - No tampering occurred between shard and consumer
 *
 * @param artifact_id        Full artifact identifier
 * @param fragment_data      Loaded fragment bytes
 * @param fragment_index     Position in artifact (0..num_fragments-1)
 * @param merkle_proof       Proof linking fragment to root
 * @param expected_root      Expected Merkle root hash
 * @return VerificationResult with state=VERIFIED_FRAGMENTS on success,
 *         state=CORRUPT if proof invalid
 */
[[nodiscard]] VerificationResult verifyFragmentIntegrity(
    const std::string& artifact_id,
    std::string_view fragment_data,
    size_t fragment_index,
    const MerkleProof& merkle_proof,
    const std::string& expected_root);

/**
 * @brief Recovery coordination callback interface (EPIC 3.5).
 *
 * When integrity verification detects corruption or staleness,
 * allows coordination with recovery subsystem to initiate rebuild.
 */
class IntegrityRecoveryHook {
public:
    virtual ~IntegrityRecoveryHook() = default;

    /**
     * @brief Request recovery/rebuild for corrupted artifact.
     *
     * @param artifact_id   Corrupted artifact identifier
     * @param reason        Why recovery is needed (e.g., "content_hash_mismatch")
     * @return true if recovery initiated, false if recovery cannot be attempted
     */
    virtual bool requestArtifactRecovery(
        const std::string& artifact_id,
        const std::string& reason) = 0;

    /**
     * @brief Request receipt chain rebuild from scratch.
     *
     * @param artifact_id   Artifact needing chain rebuild
     * @return true if rebuild initiated, false if impossible
     */
    virtual bool requestChainRebuild(const std::string& artifact_id) = 0;

    /**
     * @brief Query current recovery status for an artifact.
     *
     * @param artifact_id   Artifact to check
     * @return "not_started", "in_progress", "completed", "failed", or "unknown"
     */
    virtual std::string getRecoveryStatus(const std::string& artifact_id) = 0;
};

/**
 * @brief Set global recovery hook for integrity verification.
 *
 * The hook is called whenever integrity verification detects corruption
 * or determines that rebuild is necessary. Allows EPIC 3.5 to coordinate
 * recovery without tight coupling.
 *
 * @param hook Callback interface, or nullptr to disable recovery coordination
 */
void setIntegrityRecoveryHook(IntegrityRecoveryHook* hook);

/**
 * @brief Get the current recovery hook.
 *
 * @return Currently installed hook, or nullptr if none
 */
IntegrityRecoveryHook* getIntegrityRecoveryHook();

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Compute SHA-256 hash of arbitrary bytes.
 *
 * @param data Input data
 * @return 64-character lowercase hex SHA-256 hash
 */
[[nodiscard]] std::string computeSHA256(const std::string& data);

/**
 * @brief Compute SHA-256 hash of arbitrary bytes (string_view overload).
 *
 * @param data Input data view
 * @return 64-character lowercase hex SHA-256 hash
 */
[[nodiscard]] std::string computeSHA256(std::string_view data);

/**
 * @brief Compute SHA-256 hash of arbitrary C-string (compat overload).
 *
 * Provides an exact-match overload for calls with string literals to avoid
 * ambiguity between `std::string` and `std::string_view` overloads.
 */
[[nodiscard]] std::string computeSHA256(const char* data);

/**
 * @brief Compute deterministic SHA-256 hash of a JSON object.
 *
 * Canonicalizes JSON (sorted keys, normalized whitespace) before hashing
 * to ensure reproducible hashes across serialization boundaries.
 *
 * @param j JSON object
 * @return 64-character lowercase hex SHA-256 hash
 */
[[nodiscard]] std::string computeJSONHash(const json& j);

/**
 * @brief Verify that a hex string is a valid SHA-256 hash.
 *
 * @param hex_str String to validate
 * @return true if hex_str is exactly 64 lowercase hex characters
 */
[[nodiscard]] bool isValidSHA256Hex(std::string_view hex_str);

}  // namespace distributed_tensor
}  // namespace themis
