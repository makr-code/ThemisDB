#pragma once

#include "storage/tensor_network_storage_engine.h"

#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace themis {
namespace tensor {

/**
 * @brief Provenance required for every synthetic Dream-Mode candidate.
 */
struct DreamCandidateProvenance {
    bool synthetic = false;
    bool dream_mode = false;
    std::string run_id;
};

/**
 * @brief Immutable candidate content and its validated provenance envelope.
 */
struct DreamCandidateArtifact {
    std::string candidate_id;
    nlohmann::json payload;
    DreamCandidateProvenance provenance;
    std::string checksum;

    /**
     * @brief Serializes the candidate and provenance as a versioned JSON envelope.
     * @return JSON object containing the envelope and its checksum.
     */
    [[nodiscard]] nlohmann::json toJson() const;
};

/**
 * @brief Isolated storage for synthetic Dream-Mode candidate envelopes.
 *
 * The default constructor uses an in-memory backend. Supplying a durable
 * ITensorStorageBackend persists envelopes there under a dedicated key prefix.
 * This store has no production truth-store dependency or promotion API.
 */
class DreamCandidateStore {
public:
    /**
     * @brief Creates a process-local store for callers that do not inject storage.
     * @note Data is volatile and is not shared with other store instances.
     */
    DreamCandidateStore();

    /**
     * @brief Creates a store backed by an isolated tensor key namespace.
     * @param backend Non-null backend; use a durable backend to retain candidates
     *        across process restarts.
     * @throws std::invalid_argument if backend is null.
     * @note Duplicate IDs are rejected atomically when the backend implements
     *       putIfAbsent(); unsupported backends fail closed. Backends shared
     *       across processes must provide storage-native atomic insertion.
     */
    explicit DreamCandidateStore(
        std::shared_ptr<storage::ITensorStorageBackend> backend);

    /**
     * @brief Validates and stores a synthetic candidate without overwriting IDs.
     * @param candidate_id Non-empty candidate identifier.
     * @param payload Non-null JSON candidate payload.
     * @param provenance Must contain both true labels and a non-blank run ID.
     * @param error Optional failure reason; cleared on success.
     * @return True when the candidate was stored; false for malformed input,
     *         a duplicate ID, or a backend failure.
     * @note The checksum detects accidental corruption; it is not an authenticity
     *       proof. Backend failures cause the write to fail without fallback.
     */
    [[nodiscard]] bool store(const std::string& candidate_id,
                             const nlohmann::json& payload,
                             const DreamCandidateProvenance& provenance,
                             std::string* error = nullptr);

    /**
     * @brief Reads a candidate only when its required provenance and checksum pass.
     * @param candidate_id Identifier previously accepted by store() or importArtifact().
     * @return Candidate copy, or std::nullopt when absent or invalid.
     */
    [[nodiscard]] std::optional<DreamCandidateArtifact> get(
        const std::string& candidate_id) const;

    /**
     * @brief Exports a stored candidate as a checksummed JSON envelope.
     * @param candidate_id Identifier previously accepted by this store.
     * @return Serialized envelope, or std::nullopt when absent or invalid.
     */
    [[nodiscard]] std::optional<std::string> exportArtifact(
        const std::string& candidate_id) const;

    /**
     * @brief Validates a serialized envelope before storing it in this isolated store.
     * @param serialized JSON envelope emitted by exportArtifact().
     * @param error Optional failure reason; cleared on success.
     * @return True when the envelope is valid and inserted atomically; false for
     *         malformed input, a duplicate ID, or a backend failure.
     */
    [[nodiscard]] bool importArtifact(const std::string& serialized,
                                      std::string* error = nullptr);

    /**
     * @brief Verifies required labels and the envelope checksum.
     * @param artifact Candidate artifact to validate.
     * @return True only when the complete provenance envelope is valid.
     */
    [[nodiscard]] static bool verifyArtifact(
        const DreamCandidateArtifact& artifact) noexcept;

private:
    [[nodiscard]] static std::string storageKeyFor(const std::string& candidate_id);

    std::shared_ptr<storage::ITensorStorageBackend> backend_;
    mutable std::mutex mutex_;
};

} // namespace tensor
} // namespace themis
