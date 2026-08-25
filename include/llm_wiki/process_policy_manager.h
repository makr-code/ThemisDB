// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file process_policy_manager.h
 * @brief YAML control-plane loader and validator for LLM Wiki process orchestration.
 *
 * This manager loads the LLM Wiki process policy from YAML, validates mandatory
 * security/governance invariants, and materializes the runtime control knobs for
 * ingest/extract/synthesize/validate/re-anchor stages.
 */

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::llm_wiki {

/**
 * @brief Status for process policy loading and validation.
 */
struct ProcessPolicyStatus {
    enum class Code {
        Ok,
        InvalidFile,
        ParseError,
        ValidationError,
    };

    Code code = Code::Ok;
    std::string message;

    [[nodiscard]] bool ok() const noexcept { return code == Code::Ok; }

    [[nodiscard]] static ProcessPolicyStatus Ok() {
        return {Code::Ok, {}};
    }

    [[nodiscard]] static ProcessPolicyStatus InvalidFile(std::string msg) {
        return {Code::InvalidFile, std::move(msg)};
    }

    [[nodiscard]] static ProcessPolicyStatus ParseError(std::string msg) {
        return {Code::ParseError, std::move(msg)};
    }

    [[nodiscard]] static ProcessPolicyStatus ValidationError(std::string msg) {
        return {Code::ValidationError, std::move(msg)};
    }
};

/**
 * @brief Runtime schedule class for pipeline stages.
 */
enum class ProcessSchedule {
    Interactive,
    NearRealtime,
    Batch,
};

/**
 * @brief Tunable stage policy extracted from YAML.
 */
struct StagePolicy {
    bool enabled = true;
    ProcessSchedule schedule = ProcessSchedule::NearRealtime;
};

/**
 * @brief Bounded scalar knob for ML-driven tuning.
 */
struct KnobBounds {
    double min = 0.0;
    double max = 0.0;
};

/**
 * @brief Materialized process policy used at runtime.
 */
struct LLMWikiProcessPolicy {
    int version = 1;
    std::string policy_id;
    std::string mode;

    std::string planner_owner;
    bool second_planner_allowed = false;
    int interactive_timeout_ms = 1500;

    StagePolicy ingest;
    StagePolicy extract;
    StagePolicy synthesize;
    StagePolicy validate;
    StagePolicy re_anchor;

    // Stage-level tuning values materialized from YAML.
    // 0 / negative means "not configured".
    int synthesize_max_evidence_items = 0;
    double synthesize_min_provenance_confidence = -1.0;

    bool policy_snapshot_required = true;
    bool require_reason_codes = true;

    std::vector<std::string> adjustable_knobs;
    std::unordered_map<std::string, KnobBounds> hard_bounds;
    std::vector<std::string> never_adjust;
};

/**
 * @brief Loads and validates LLM Wiki YAML process policies.
 */
class ProcessPolicyManager {
public:
    /**
     * @brief Load policy from YAML file and validate invariants.
     *
     * @param yaml_path Path to YAML policy file.
     * @param out_policy Materialized policy on success.
     * @return Status code and error message if failed.
     */
    [[nodiscard]] static ProcessPolicyStatus loadFromYaml(
        const std::filesystem::path& yaml_path,
        LLMWikiProcessPolicy& out_policy) noexcept;

    /**
     * @brief Validate a materialized policy.
     *
     * @param policy Materialized policy.
     * @return Status code and validation message.
     */
    [[nodiscard]] static ProcessPolicyStatus validate(
        const LLMWikiProcessPolicy& policy) noexcept;
};

} // namespace themis::llm_wiki
