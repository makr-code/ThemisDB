/**
 * @file adapter_provenance.h
 * @brief Canonicalisation, signing and verification of LoRA/AdaLoRA adapter
 *        metadata (`themis.provenance.*`) for safetensors packages and GGUF
 *        adapter files.
 *
 * ## Canonical form (single definition; Python never computes it)
 * The signed payload is the set of all `themis.*` metadata keys except
 * `themis.provenance.*`, as a string→string map that MUST contain
 * `themis.adapter.sha256_tensors` (binding the tensor bytes of the
 * safetensors package) and is serialised as JSON with lexicographically
 * sorted keys, no insignificant whitespace and raw UTF-8 (nlohmann::json
 * `dump()` of an object; invalid UTF-8 is rejected).  The signature is
 * computed by a SigningService over exactly these canonical bytes.
 *
 * Written keys:
 * - `themis.provenance.canonical_json`   – the canonical payload itself
 * - `themis.provenance.canonical_sha256` – SHA-256 (hex) of the payload
 * - `themis.provenance.signature`        – signature (hex)
 * - `themis.provenance.key_id`           – signing key id
 * - `themis.provenance.alg`              – algorithm reported by the signer
 *
 * The payload is embedded because the GGUF converter stores some keys typed
 * (u32/f32, see scripts/lora THEMIS_KV_TYPES) and f32 values do not
 * round-trip to the signed decimal strings.  GGUF verification therefore
 * checks the signature over the embedded payload and then checks that the
 * GGUF `themis.*` keys are exactly the payload keys with type-consistent
 * values (strings exact, integers exact, f32 equal after rounding to float).
 *
 * Signing happens in C++ on the safetensors package before GGUF conversion;
 * the converter passes all `themis.*` header keys through unchanged.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace themis {
class SigningService;
}

namespace themis {
namespace llm {
namespace lora {

/// @brief Provenance metadata key names.
namespace provenance_keys {
inline constexpr const char* kPrefix          = "themis.provenance.";
inline constexpr const char* kCanonicalJson   = "themis.provenance.canonical_json";
inline constexpr const char* kCanonicalSha256 = "themis.provenance.canonical_sha256";
inline constexpr const char* kSignature       = "themis.provenance.signature";
inline constexpr const char* kKeyId           = "themis.provenance.key_id";
inline constexpr const char* kAlg             = "themis.provenance.alg";
inline constexpr const char* kTensorSha256    = "themis.adapter.sha256_tensors";
}  // namespace provenance_keys

/**
 * @brief Policy for adapter signature enforcement.
 *
 * A present-but-invalid signature is ALWAYS rejected; the flags only decide
 * whether an unsigned adapter is acceptable.
 */
struct AdapterSignaturePolicy {
    bool require_signed_imports = true;   ///< Reject unsigned safetensors imports.
    bool require_signed_hot_load = true;  ///< Reject unsigned GGUF adapters at hot-load.
    bool require_signed_training = true;  ///< Fail training jobs if no signer is configured.
    std::string key_id;                   ///< Key used to sign newly trained adapters.
    std::set<std::string> trusted_key_ids;///< If non-empty, only these key ids verify.
};

/// @brief Canonicalisation helpers (the single C++ definition).
class AdapterProvenanceCanonical {
public:
    /**
     * @brief Extract the signed field set: all `themis.*` keys except
     *        `themis.provenance.*`.
     * @return std::nullopt with @p error if `themis.adapter.sha256_tensors`
     *         is missing.
     */
    [[nodiscard]] static std::optional<std::map<std::string, std::string>> signedFields(
        const std::map<std::string, std::string>& metadata, std::string* error = nullptr);

    /**
     * @brief Canonical JSON of signedFields(@p metadata).
     * @return std::nullopt with @p error on missing tensor hash or invalid UTF-8.
     */
    [[nodiscard]] static std::optional<std::string> canonicalJson(
        const std::map<std::string, std::string>& metadata, std::string* error = nullptr);
};

/// @brief Outcome of a provenance verification.
struct ProvenanceVerification {
    enum class Status {
        Verified,  ///< Signature valid and all checks passed.
        Unsigned,  ///< No `themis.provenance.signature` present.
        Invalid    ///< Present but tampered/malformed/unverifiable.
    };
    Status status = Status::Invalid;
    std::string error;                               ///< Reason for Unsigned/Invalid.
    std::string key_id;                              ///< Signing key id (Verified).
    std::map<std::string, std::string> signed_fields;///< Verified payload (Verified).

    /// @brief True iff acceptable: Verified, or Unsigned when not @p require_signed.
    [[nodiscard]] bool acceptable(bool require_signed) const {
        return status == Status::Verified || (status == Status::Unsigned && !require_signed);
    }
};

/**
 * @brief Signs adapter metadata through a SigningService.
 * @note Thread-safe if the SigningService is thread-safe.
 */
class AdapterProvenanceSigner {
public:
    /**
     * @param signer Signing backend (e.g. createKeyProviderSigningService()).
     * @param key_id Key id passed to SigningService::sign().
     */
    AdapterProvenanceSigner(std::shared_ptr<SigningService> signer, std::string key_id);

    /**
     * @brief Replace any `themis.provenance.*` keys in @p metadata by a fresh
     *        signature over the canonical payload.
     * @return false with @p error if canonicalisation or signing fails
     *         (@p metadata is unchanged in that case).
     */
    bool sign(std::map<std::string, std::string>& metadata, std::string* error = nullptr) const;

private:
    std::shared_ptr<SigningService> signer_;
    std::string key_id_;
};

/**
 * @brief Verifies adapter provenance on safetensors metadata, safetensors
 *        files and GGUF KV metadata.
 * @note Thread-safe if the SigningService is thread-safe.
 */
class AdapterProvenanceVerifier {
public:
    /**
     * @param verifier        Signature backend used for SigningService::verify().
     * @param trusted_key_ids If non-empty, signatures by other key ids are Invalid.
     */
    explicit AdapterProvenanceVerifier(std::shared_ptr<SigningService> verifier,
                                       std::set<std::string> trusted_key_ids = {});

    /**
     * @brief Verify a safetensors-style string metadata map.
     *
     * Recomputes the canonical payload from @p metadata, compares it with the
     * embedded payload and canonical_sha256, and verifies the signature.
     * Tensor bytes are not read; use verifySafetensorsFile() for that.
     */
    [[nodiscard]] ProvenanceVerification verifyMetadata(
        const std::map<std::string, std::string>& metadata) const;

    /**
     * @brief Verify a safetensors file: header metadata (verifyMetadata())
     *        plus recomputation of the tensor-data SHA-256 against
     *        `themis.adapter.sha256_tensors`.
     */
    [[nodiscard]] ProvenanceVerification verifySafetensorsFile(
        const std::filesystem::path& path) const;

    /**
     * @brief Verify GGUF KV metadata (as returned by
     *        lora_gguf::readAdapterGgufMetadata().kv).
     *
     * Verifies the signature over the embedded canonical payload and checks
     * that the GGUF `themis.*` (non-provenance) keys are exactly the payload
     * keys with type-consistent values.
     */
    [[nodiscard]] ProvenanceVerification verifyGgufKv(const nlohmann::json& kv) const;

private:
    [[nodiscard]] ProvenanceVerification verifyPayload(
        const std::string& canonical,
        const std::map<std::string, std::string>& provenance) const;

    std::shared_ptr<SigningService> verifier_;
    std::set<std::string> trusted_key_ids_;
};

/**
 * @brief Atomically replace the `__metadata__` of a safetensors file.
 *
 * Tensor entries and the data region are copied byte-for-byte (so
 * `themis.adapter.sha256_tensors` stays valid); the header is re-serialised
 * and space-padded to an 8-byte boundary.  Writes `<path>.tmp` and renames.
 *
 * @return false with @p error on I/O or format errors (original untouched).
 */
bool rewriteSafetensorsMetadata(const std::filesystem::path& path,
                                const std::map<std::string, std::string>& metadata,
                                std::string* error = nullptr);

}  // namespace lora
}  // namespace llm
}  // namespace themis
