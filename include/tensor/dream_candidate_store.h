#pragma once

#include <map>
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
 * @brief Isolated, process-local storage for synthetic Dream-Mode candidates.
 *
 * This store has no production truth-store dependency or write API. Exported
 * envelopes retain all provenance labels and are validated again when imported.
 */
class DreamCandidateStore {
public:
    /**
     * @brief Validates and stores a synthetic candidate without overwriting IDs.
     * @param candidate_id Non-empty candidate identifier.
     * @param payload Non-null JSON candidate payload.
     * @param provenance Must contain both true labels and a non-blank run ID.
     * @param error Optional failure reason; cleared on success.
     * @return True when the candidate was stored; false for malformed input or
     *         a duplicate ID.
     * @note The checksum detects accidental corruption; it is not an authenticity
     *       proof. Storage is in-memory and scoped to this store instance.
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
     * @return True when the envelope is valid and its candidate ID was not present.
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
    std::map<std::string, DreamCandidateArtifact> artifacts_;
    mutable std::mutex mutex_;
};

} // namespace tensor
} // namespace themis
