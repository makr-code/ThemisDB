#pragma once

#include "process/process_graph_rag.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace themis::process {

/**
 * @brief Runtime state of an external orchestration profile.
 */
enum class OrchestrationProfileState {
    STAGED,
    ACTIVE,
    ARCHIVED
};

/**
 * @brief Persisted external orchestration profile metadata and payload.
 */
struct OrchestrationProfileRecord {
    std::string id;
    std::string domain;
    std::string version;
    std::string source_path;
    std::string source_format;
    OrchestrationProfileState state{OrchestrationProfileState::STAGED};
    uint64_t revision{1};
    bool internal_rag_stage{true};
    int64_t created_at_ms{0};
    int64_t updated_at_ms{0};
    nlohmann::json model;

    [[nodiscard]] nlohmann::json toDocument() const;
    [[nodiscard]] static std::optional<OrchestrationProfileRecord> fromDocument(
        const nlohmann::json& doc);
};

/**
 * @brief Process prediction surface for orchestration decisions.
 */
struct ProcessPredictionOutput {
    double sla_breach_probability{0.0};
    double compliance_risk_score{0.0};
    double completion_confidence{0.0};
    std::string recommended_submodel;
    nlohmann::json evidence;

    [[nodiscard]] nlohmann::json toJson() const;
};

/**
 * @brief Registry for externally managed YAML/JSON process orchestration profiles.
 *
 * Profiles are versioned in RocksDB and can transition through staged/active/archived
 * states. `internal_rag_stage` is enforced as true to keep RAG an internal pipeline stage.
 */
class ProcessOrchestrationRegistry {
public:
    explicit ProcessOrchestrationRegistry(::themis::RocksDBWrapper& db,
                                          ProcessGraphRag& rag);

    /**
     * @brief Upsert a profile from a JSON object and optionally activate it.
     * @param[in] profile Document containing at least `id` and `domain` (or `model_id`/`method_id`).
     * @param[in] source_path Optional source path used for traceability.
     * @param[in] source_format Declared source format (`json`, `yaml`, `catalog`).
     * @param[in] activate_if_staged Activates the profile for its domain after persistence.
     * @return Success with profile id or failure reason.
     */
    [[nodiscard]] ProcessModelResult upsertProfile(
        const nlohmann::json& profile,
        std::string_view source_path,
        std::string_view source_format,
        bool activate_if_staged = false);

    /**
     * @brief Load and upsert a profile from a YAML or JSON file.
     * @param[in] profile_path Absolute or relative path to profile file.
     * @param[in] activate_if_staged Activates the profile for its domain after persistence.
     * @return Success with profile id or failure reason.
     */
    [[nodiscard]] ProcessModelResult upsertProfileFromFile(
        std::string_view profile_path,
        bool activate_if_staged = false);

    /**
     * @brief Synchronize profiles referenced by `assets/model_catalog.yaml`.
     * @param[in] catalog_path Path to the catalog YAML file.
     * @param[in] activate_ingested When true, each ingested profile is activated.
     * @return Number of ingested profiles or failure reason.
     */
    [[nodiscard]] std::variant<size_t, ProcessModelResult> syncFromCatalog(
        std::string_view catalog_path,
        bool activate_ingested = false);

    /**
     * @brief Activate a staged profile for a domain (with previous-active demotion).
     */
    [[nodiscard]] ProcessModelResult activateProfile(
        std::string_view domain,
        std::string_view profile_id);

    /**
     * @brief Archive a profile and clear active pointer if it was active.
     */
    [[nodiscard]] ProcessModelResult archiveProfile(std::string_view profile_id);

    /**
     * @brief Roll back a domain to a previously stored profile id.
     */
    [[nodiscard]] ProcessModelResult rollbackDomain(
        std::string_view domain,
        std::string_view profile_id);

    /**
     * @brief Load a profile by id.
     */
    [[nodiscard]] std::optional<OrchestrationProfileRecord> getProfile(
        std::string_view profile_id) const;

    /**
     * @brief Get active profile for a domain.
     */
    [[nodiscard]] std::optional<OrchestrationProfileRecord> getActiveProfile(
        std::string_view domain) const;

    /**
     * @brief List profiles, optionally filtered by domain/state.
     */
    [[nodiscard]] std::vector<OrchestrationProfileRecord> listProfiles(
        std::string_view domain_filter = "",
        std::optional<OrchestrationProfileState> state_filter = std::nullopt,
        size_t limit = 0) const;

    /**
     * @brief Predict process risk/outcome metrics for orchestration decisions.
     *
     * @param[in] instance_id Process instance identifier for retrieval/evidence lookup.
     * @param[in] domain Optional domain to derive recommended active submodel.
     * @return Prediction payload with bounded confidence/risk scores and evidence.
     */
    [[nodiscard]] ProcessPredictionOutput predictOutcome(
        std::string_view instance_id,
        std::string_view domain = "") const;

    [[nodiscard]] static std::string stateToString(OrchestrationProfileState state);
    [[nodiscard]] static std::optional<OrchestrationProfileState> stateFromString(
        std::string_view state);

private:
    [[nodiscard]] static int64_t nowMs();
    [[nodiscard]] static std::string normalizeDomain(std::string_view domain);
    [[nodiscard]] static std::optional<std::string> extractProfileId(const nlohmann::json& profile);
    [[nodiscard]] static std::optional<std::string> extractDomain(const nlohmann::json& profile);
    [[nodiscard]] static bool extractInternalRagStage(const nlohmann::json& profile);

    [[nodiscard]] ProcessModelResult persistProfile(const OrchestrationProfileRecord& record);
    [[nodiscard]] static std::string profileKey(std::string_view id);
    [[nodiscard]] static std::string profileRevisionKey(std::string_view id, uint64_t revision);
    [[nodiscard]] static std::string activeDomainKey(std::string_view domain);

    ::themis::RocksDBWrapper& db_;
    ProcessGraphRag& rag_;
};

} // namespace themis::process
