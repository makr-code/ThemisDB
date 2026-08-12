/**
 * @file lora_provenance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.40
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

// ============================================================================
// Local Adapter Provenance
// ============================================================================

/**
 * @brief Full cryptographic provenance record for a locally trained LoRA adapter.
 *
 * Captures every artefact that participated in the training run so that any
 * future audit can reconstruct and verify the exact training environment:
 *   - SHA-256 hashes of the dataset, base model, and hyperparameters
 *   - Trainer identity with CA/eIDAS certificate chain
 *   - RFC 3161 timestamp token for notarial proof-of-existence
 *   - Hardware details for reproducibility
 */
struct LoRAProvenanceRecord {
    // Cryptographic artefact hashes
    std::string dataset_hash;          ///< SHA-256 of the training dataset content
    std::string base_model_hash;       ///< SHA-256 of the base LLM weights
    std::string hyperparameter_hash;   ///< SHA-256 of the serialised hyperparameter block
    std::string adapter_weights_hash;  ///< SHA-256 of the trained adapter weights

    // Identity and trust chain
    std::string trainer_id;            ///< Identifier of the entity that ran training
    std::string ca_chain;              ///< PEM-encoded CA / eIDAS certificate chain
    std::string signature;             ///< Ed25519 / ECDSA signature over the record fields

    // Timestamps
    std::string created_at;            ///< ISO 8601 UTC timestamp (wall-clock)
    std::string rfc3161_timestamp;     ///< Base64-encoded RFC 3161 timestamp token

    // Training context
    double training_duration_secs = 0.0;  ///< Wall-clock training time in seconds
    json   hardware_info;                 ///< GPU model, VRAM, CPU, RAM, etc.
    json   custom_metadata;              ///< Application-specific extensions

    json toJSON() const;
    static LoRAProvenanceRecord fromJSON(const json& j);
};

// ============================================================================
// External Adapter Provenance
// ============================================================================

/**
 * @brief Provenance record for a LoRA adapter imported from an external source.
 *
 * Captures origin information and allows ThemisDB to verify the supplier's
 * signature and certificate chain before the adapter is trusted for inference.
 * Adapters whose signature or provenance cannot be verified are rejected.
 */
struct ExternalAdapterProvenance {
    // Origin
    std::string source_url;            ///< URL of the upstream repository / registry
    std::string commit_hash;           ///< Git commit SHA or equivalent VCS identifier
    std::string description;           ///< Human-readable description of the adapter

    // Integrity and trust
    std::string adapter_hash;          ///< SHA-256 of the imported adapter weights
    std::string provenance_signature;  ///< Supplier's digital signature over the record
    std::string certificate_chain;     ///< PEM-encoded supplier certificate chain

    // Import result
    std::string import_timestamp;      ///< ISO 8601 UTC timestamp of the import
    bool        signature_valid = false; ///< True when signature verification passed
    bool        cert_chain_valid = false; ///< True when certificate chain is trusted
    std::vector<std::string> validation_errors; ///< Non-empty when validation failed

    json toJSON() const;
    static ExternalAdapterProvenance fromJSON(const json& j);
};

// ============================================================================
// Adapter Snapshot (MVCC-style time-travel)
// ============================================================================

/**
 * @brief Point-in-time snapshot of a LoRA adapter with full provenance.
 *
 * Enables MVCC-style time-travel: any prior state of an adapter can be
 * restored from a snapshot together with the provenance that was valid at
 * that point.  Snapshots form a singly-linked chain via parent_snapshot_id.
 */
struct AdapterSnapshot {
    std::string snapshot_id;          ///< Unique snapshot identifier (UUID or similar)
    std::string adapter_id;           ///< Adapter this snapshot belongs to
    std::string version;              ///< Adapter version string at snapshot time

    std::string weights_hash;         ///< SHA-256 of adapter weights at snapshot time
    std::string timestamp;            ///< ISO 8601 UTC timestamp of snapshot creation
    std::string parent_snapshot_id;   ///< Previous snapshot in the chain (empty = root)

    LoRAProvenanceRecord provenance;  ///< Full provenance at snapshot time

    json toJSON() const;
    static AdapterSnapshot fromJSON(const json& j);
};

// ============================================================================
// Inference Audit Entry (Merkle-chained immutable log)
// ============================================================================

/**
 * @brief Single entry in the immutable, Merkle-chained inference audit log.
 *
 * Each entry commits to the inputs, outputs, and model/adapter state used
 * during an inference.  Consecutive entries are linked via previous_hash,
 * forming a tamper-evident chain analogous to a blockchain ledger.
 */
struct InferenceAuditEntry {
    std::string entry_id;             ///< Unique entry identifier
    std::string previous_hash;        ///< SHA-256 of the preceding entry (empty for genesis)
    std::string entry_hash;           ///< SHA-256 of this entry's canonical JSON form

    // Inference context
    std::string timestamp;            ///< ISO 8601 UTC timestamp
    std::string request_id;           ///< Correlation ID of the inference request
    std::string query_hash;           ///< SHA-256 of the query/prompt
    std::string response_hash;        ///< SHA-256 of the generated response
    std::string model_hash;           ///< SHA-256 of the base model weights used
    std::string adapter_hash;         ///< SHA-256 of the adapter weights used

    // Commitments and metadata
    json commitments;                 ///< Additional cryptographic commitments (e.g. ZK)
    json metadata;                    ///< Request-level metadata (user ID, session, …)

    json toJSON() const;
    static InferenceAuditEntry fromJSON(const json& j);

    /**
     * @brief Compute the canonical SHA-256 hash of this entry's content fields.
     *
     * The hash covers all content fields (timestamp through metadata) but
     * deliberately excludes entry_hash itself to avoid circular dependency.
     * It does include previous_hash so that the chain is verified end-to-end.
     */
    std::string computeContentHash() const;
};

// ============================================================================
// LoRA Provenance Manager
// ============================================================================

/**
 * @brief Manages provenance records, snapshots, and the audit-log Merkle chain.
 *
 * Central entry point for the auditability features described in the issue:
 *   1. Build and persist provenance records for locally trained adapters
 *   2. Validate and store provenance for externally imported adapters
 *   3. Create/restore MVCC snapshots for time-travel
 *   4. Append to and verify the Merkle-chained inference audit log
 */
class LoRAProvenanceManager {
public:
    LoRAProvenanceManager();
    ~LoRAProvenanceManager();

    // Disable copy; use shared_ptr for shared ownership
    LoRAProvenanceManager(const LoRAProvenanceManager&)            = delete;
    LoRAProvenanceManager& operator=(const LoRAProvenanceManager&) = delete;

    // -----------------------------------------------------------------------
    // Local adapter provenance
    // -----------------------------------------------------------------------

    /**
     * @brief Store a provenance record for a locally trained adapter.
     * @param adapter_id  Adapter identifier
     * @param record      Populated provenance record
     * @return true on success
     */
    bool storeProvenance(const std::string& adapter_id,
                         const LoRAProvenanceRecord& record);

    /**
     * @brief Retrieve the provenance record for an adapter.
     * @param adapter_id  Adapter identifier
     * @return Provenance record if found
     */
    std::optional<LoRAProvenanceRecord> getProvenance(
        const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // External adapter import
    // -----------------------------------------------------------------------

    /**
     * @brief Import an external adapter after validating its provenance.
     *
     * Validates the supplier's signature and certificate chain.  The adapter
     * is stored only if all checks pass (or if @p allow_unsigned is true).
     *
     * @param adapter_id      Adapter identifier to register under
     * @param provenance      External provenance record supplied by the importer
     * @param trusted_ca_pem  PEM bundle of CA certificates trusted by this node
     * @param allow_unsigned  If true, skip signature/cert validation (NOT recommended)
     * @return Validated provenance record (with validation fields populated)
     */
    ExternalAdapterProvenance importExternalAdapter(
        const std::string& adapter_id,
        ExternalAdapterProvenance provenance,
        const std::string& trusted_ca_pem = "",
        bool allow_unsigned = false);

    /**
     * @brief Retrieve the external provenance record for an imported adapter.
     * @param adapter_id  Adapter identifier
     * @return External provenance record if found
     */
    std::optional<ExternalAdapterProvenance> getExternalProvenance(
        const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // Snapshots / MVCC time-travel
    // -----------------------------------------------------------------------

    /**
     * @brief Create a snapshot of the current adapter state.
     * @param adapter_id    Adapter identifier
     * @param version       Current version string
     * @param weights_hash  SHA-256 of current adapter weights
     * @param provenance    Current provenance record
     * @return Created snapshot (with snapshot_id populated)
     */
    AdapterSnapshot createSnapshot(const std::string& adapter_id,
                                   const std::string& version,
                                   const std::string& weights_hash,
                                   const LoRAProvenanceRecord& provenance);

    /**
     * @brief List all snapshots for an adapter, ordered oldest-first.
     * @param adapter_id  Adapter identifier
     * @return Vector of snapshots
     */
    std::vector<AdapterSnapshot> listSnapshots(
        const std::string& adapter_id) const;

    /**
     * @brief Retrieve a specific snapshot by ID.
     * @param snapshot_id  Snapshot identifier
     * @return Snapshot if found
     */
    std::optional<AdapterSnapshot> getSnapshot(
        const std::string& snapshot_id) const;

    // -----------------------------------------------------------------------
    // Merkle-chained inference audit log
    // -----------------------------------------------------------------------

    /**
     * @brief Append an inference audit entry to the Merkle chain.
     *
     * Automatically links the entry to the previous entry and computes
     * entry_hash.
     *
     * @param adapter_id  Adapter used for inference
     * @param entry       Audit entry (entry_id, previous_hash, entry_hash
     *                    may be empty — they will be populated by this method)
     * @return Populated entry with entry_hash and previous_hash set
     */
    InferenceAuditEntry appendAuditEntry(const std::string& adapter_id,
                                          InferenceAuditEntry entry);

    /**
     * @brief Retrieve audit log entries for an adapter.
     * @param adapter_id  Adapter identifier
     * @return Ordered vector of entries (oldest first)
     */
    std::vector<InferenceAuditEntry> getAuditLog(
        const std::string& adapter_id) const;

    /**
     * @brief Verify the integrity of the Merkle audit chain for an adapter.
     *
     * Recomputes each entry_hash and checks the previous_hash linkage.
     *
     * @param adapter_id  Adapter identifier
     * @return true when the entire chain is intact
     */
    bool verifyAuditChain(const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the SHA-256 hash of arbitrary bytes and return as hex.
     * @param data  Input data
     * @return 64-character lowercase hex string
     */
    static std::string sha256Hex(const std::string& data);

    /**
     * @brief Compute the SHA-256 hash of a file's contents.
     * @param path  File path
     * @return 64-character lowercase hex string, or empty on error
     */
    static std::string sha256File(const std::string& path);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
