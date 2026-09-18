/**
 * @file lora_provenance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.40
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static LoRAProvenanceRecord fromJSON(const json& j);
};

// ============================================================================
// External Adapter Provenance
// ============================================================================

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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ExternalAdapterProvenance fromJSON(const json& j);
};

// ============================================================================
// Adapter Snapshot (MVCC-style time-travel)
// ============================================================================

struct AdapterSnapshot {
    std::string snapshot_id;          ///< Unique snapshot identifier (UUID or similar)
    std::string adapter_id;           ///< Adapter this snapshot belongs to
    std::string version;              ///< Adapter version string at snapshot time

    std::string weights_hash;         ///< SHA-256 of adapter weights at snapshot time
    std::string timestamp;            ///< ISO 8601 UTC timestamp of snapshot creation
    std::string parent_snapshot_id;   ///< Previous snapshot in the chain (empty = root)

    LoRAProvenanceRecord provenance;  ///< Full provenance at snapshot time

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AdapterSnapshot fromJSON(const json& j);
};

// ============================================================================
// Inference Audit Entry (Merkle-chained immutable log)
// ============================================================================

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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static InferenceAuditEntry fromJSON(const json& j);

    /**
     * @brief Compute Content Hash.
     * @return Return value.
     */
    std::string computeContentHash() const;
};

// ============================================================================
// LoRA Provenance Manager
// ============================================================================

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
     * @brief Store Provenance.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool storeProvenance(const std::string& adapter_id,
                         const LoRAProvenanceRecord& record);

    /**
     * @brief Get Provenance.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<LoRAProvenanceRecord> getProvenance(
        const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // External adapter import
    // -----------------------------------------------------------------------

    ExternalAdapterProvenance importExternalAdapter(
        const std::string& adapter_id,
        ExternalAdapterProvenance provenance,
        const std::string& trusted_ca_pem = "",
        bool allow_unsigned = false);

    /**
     * @brief Get External Provenance.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<ExternalAdapterProvenance> getExternalProvenance(
        const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // Snapshots / MVCC time-travel
    // -----------------------------------------------------------------------

    /**
     * @brief Create Snapshot.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] version Input parameter.
     * @param[in] weights_hash Input parameter.
     * @param[in] provenance Input parameter.
     * @return Return value.
     */
    AdapterSnapshot createSnapshot(const std::string& adapter_id,
                                   const std::string& version,
                                   const std::string& weights_hash,
                                   const LoRAProvenanceRecord& provenance);

    /**
     * @brief List Snapshots.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<AdapterSnapshot> listSnapshots(
        const std::string& adapter_id) const;

    /**
     * @brief Get Snapshot.
     * @param[in] snapshot_id Identifier of the snapshot.
     * @return Return value.
     */
    std::optional<AdapterSnapshot> getSnapshot(
        const std::string& snapshot_id) const;

    /**
     * @brief ----------------------------------------------------------------------- Merkle-chained inference audit log -----------------------------------------------------------------------
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */

    InferenceAuditEntry appendAuditEntry(const std::string& adapter_id,
                                          InferenceAuditEntry entry);

    /**
     * @brief Get Audit Log.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<InferenceAuditEntry> getAuditLog(
        const std::string& adapter_id) const;

    /**
     * @brief Verify Audit Chain.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool verifyAuditChain(const std::string& adapter_id) const;

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    /**
     * @brief Sha256 Hex.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string sha256Hex(const std::string& data);

    /**
     * @brief Sha256 File.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string sha256File(const std::string& path);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
