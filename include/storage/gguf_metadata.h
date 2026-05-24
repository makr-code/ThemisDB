/*
 * ThemisDB | File: gguf_metadata.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file storage/gguf_metadata.h
 * @brief GGUF v3 Metadata and Provenance for TT-compressed tensors.
 *
 * ## Overview (paper §GGUF Metadata and File Stability)
 *
 * Every TT-core stored in ThemisDB carries provenance metadata: origin
 * document, page, line, ingest timestamp, and a tenant-scoped HMAC
 * signature that prevents tampering.  This metadata is attached to the
 * key inside the storage backend and can be retrieved without loading
 * the full TT-core payload.
 *
 * The design mirrors GGUF v3 key-value metadata semantics so that
 * metadata can be directly embedded into GGUF files when the
 * `GgmlTensorBridge` exports a model.
 *
 * `sign()` / `verify()` use a built-in OpenSSL HMAC-SHA256 implementation
 * by default. Callers can still override this via `setHmacFn()` to route
 * signing/verification through external KMS/HSM integrations.
 *
 * ## Thread Safety
 * All public methods are thread-safe; the internal store uses a
 * shared_mutex for read/write isolation.
 *
 * ## References
 * - GGUF v3 spec: https://github.com/ggml-org/ggml/blob/master/docs/gguf.md
 * - ThemisDB Research Group (2026). §GGUF Metadata. Internal pre-print.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// ProvenanceRecord — per-core or per-adapter origin metadata
// ============================================================================

/**
 * @brief GGUF v3 provenance fields attached to a stored TT-core.
 *
 * All string fields are UTF-8.  The `hmac_signature` is a
 * hex-encoded HMAC-SHA256 tag computed over the canonical fields
 * (source.filename + page + line + doc_id + tenant_id + ingest_ts).
 *
 * When `hmac_signature` is empty the record has not yet been signed.
 */
struct ProvenanceRecord {
    // ─── Mandatory GGUF v3 provenance fields ──────────────────────────────
    std::string source_filename;       ///< e.g. "contract_2025_q4.pdf"
    int32_t     source_page    = -1;   ///< 0-based page number; -1 = unknown
    int32_t     source_line    = -1;   ///< 0-based line number; -1 = unknown
    std::string source_doc_id;         ///< Stable document identifier (UUID)
    std::string tenant_id;             ///< Owning tenant namespace

    /// ISO-8601 ingestion timestamp (e.g. "2026-05-06T19:30:44Z")
    std::string ingest_timestamp;

    // ─── Security ─────────────────────────────────────────────────────────
    /**
     * Hex-encoded HMAC-SHA256 signature over the canonical field set.
     */
    std::string hmac_signature;

    // ─── Helpers ──────────────────────────────────────────────────────────

    /// True if all mandatory fields are non-empty / valid.
    [[nodiscard]] bool isComplete() const noexcept {
        return !source_filename.empty()
            && !source_doc_id.empty()
            && !tenant_id.empty()
            && !ingest_timestamp.empty();
    }

    /// Produce the canonical byte string that is signed / verified.
    [[nodiscard]] std::string canonicalBytes() const;

    bool operator==(const ProvenanceRecord& o) const noexcept {
        return source_filename   == o.source_filename
            && source_page       == o.source_page
            && source_line       == o.source_line
            && source_doc_id     == o.source_doc_id
            && tenant_id         == o.tenant_id
            && ingest_timestamp  == o.ingest_timestamp;
        // intentionally excludes hmac_signature from equality
    }
    bool operator!=(const ProvenanceRecord& o) const noexcept {
        return !(*this == o);
    }
};

// ============================================================================
// GGUFMetadata — thread-safe provenance store
// ============================================================================

/**
 * @brief In-memory GGUF v3 metadata store for TT-cores and adapters.
 *
 * Each entry is keyed by the storage key used for the tensor or adapter
 * (e.g. `__ttcore__:<tenant>:<file_id>:<chunk_id>`).  The metadata can
 * be serialised to / deserialised from a flat binary format for embedding
 * inside a GGUF file.
 *
 * ### Typical usage
 * ```cpp
 * GGUFMetadata meta;
 * ProvenanceRecord rec;
 * rec.source_filename  = "contract.pdf";
 * rec.source_doc_id    = "doc-uuid-001";
 * rec.tenant_id        = "legal_team";
 * rec.ingest_timestamp = "2026-05-06T19:00:00Z";
 *
 * meta.sign(rec, "tenant_hmac_key");
 * meta.attach("__ttcore__:legal_team:file42:chunk7", rec);
 *
 * auto retrieved = meta.retrieve("__ttcore__:legal_team:file42:chunk7");
 * if (retrieved && meta.verify(*retrieved, "tenant_hmac_key")) { ... }
 * ```
 */
class GGUFMetadata {
public:
    GGUFMetadata() = default;

    // ─── Write API ────────────────────────────────────────────────────────

    /**
     * @brief Attach provenance metadata to a storage key.
     *
     * Overwrites any previously attached record for the same key.
     *
     * @param storage_key  The key under which the tensor / adapter is stored.
     * @param record       Provenance record to attach.
     */
    void attach(const std::string& storage_key,
                const ProvenanceRecord& record);

    /**
     * @brief Remove the metadata entry for a storage key.
     *
     * @return true if the key was found and removed, false otherwise.
     */
    bool detach(const std::string& storage_key);

    // ─── Read API ─────────────────────────────────────────────────────────

    /**
     * @brief Retrieve the provenance record for a storage key.
     *
     * @return The record if found, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<ProvenanceRecord>
        retrieve(const std::string& storage_key) const;

    /**
     * @brief Test whether a provenance record exists for the given key.
     */
    [[nodiscard]] bool has(const std::string& storage_key) const;

    /**
     * @brief Return all storage keys that have attached provenance records.
     */
    [[nodiscard]] std::vector<std::string> keys() const;

    // ─── Signing / Verification ───────────────────────────────────────────

    /**
     * @brief Callable type for a real HMAC-SHA256 implementation.
     *
     * Signature: `std::string hmac(const std::string& data, const std::string& key)`
     *
     * `GGUFMetadata` includes a secure OpenSSL HMAC-SHA256 default path.
     * Inject a custom implementation via `setHmacFn()` only when external
     * crypto backends (e.g. KMS/HSM gateways) are required.
     * The function must be thread-safe; it is called under no lock.
     */
    using HmacFn = std::function<std::string(const std::string& data,
                                              const std::string& key)>;

    /**
     * @brief Inject a real HMAC implementation (thread-safe, process-global).
     *
     * Once set, all subsequent calls to `sign()` and `verify()` will delegate
     * to @p fn instead of using the built-in OpenSSL HMAC-SHA256 path.
     * Pass a null fn to restore the built-in implementation.
     *
     * @param fn  HMAC function, e.g. wrapping OpenSSL `HMAC()` with SHA-256.
     */
    static void setHmacFn(HmacFn fn);

    /**
     * @brief Compute a signature tag and write it to `record.hmac_signature`.
     *
     * Delegates to the injected `HmacFn` when one has been set via
     * `setHmacFn()`; otherwise uses the built-in OpenSSL HMAC-SHA256 path.
     *
     * @param record    Record to sign (modifies `hmac_signature` in-place).
     * @param hmac_key  Tenant-specific signing key (arbitrary bytes).
     */
    static void sign(ProvenanceRecord& record,
                     const std::string& hmac_key);

    /**
     * @brief Verify that `record.hmac_signature` matches a freshly computed tag.
     *
     * Uses the same HMAC source (injected fn or built-in OpenSSL path) as `sign()`.
     *
     * @param record    Record whose signature should be checked.
     * @param hmac_key  Tenant-specific signing key.
     * @return true if the signature is valid, false otherwise.
     */
    [[nodiscard]] static bool verify(const ProvenanceRecord& record,
                                     const std::string& hmac_key);

    // ─── Serialisation ────────────────────────────────────────────────────

    /**
     * @brief Serialise all stored records to a flat binary blob.
     *
     * Format:
     *  - uint32_t : number of records
     *  - per record:
     *    - uint32_t key_len + key bytes
     *    - ProvenanceRecord fields (length-prefixed strings + int32_t fields)
     */
    [[nodiscard]] std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialise records from a binary blob produced by `serialize()`.
     *
     * @param bytes  Binary blob.
     * @return true on success, false if the blob is malformed.
     */
    bool deserialize(const std::vector<uint8_t>& bytes);

    // ─── Diagnostics ──────────────────────────────────────────────────────

    /// Total number of provenance records stored.
    [[nodiscard]] std::size_t size() const noexcept;

private:
    mutable std::shared_mutex   mutex_;
    std::unordered_map<std::string, ProvenanceRecord> store_;
};

} // namespace storage
} // namespace themis
