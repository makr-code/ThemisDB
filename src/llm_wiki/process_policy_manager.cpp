// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "llm_wiki/process_policy_manager.h"

#include "config/config_path_resolver.h"
#include "config/config_schema_validator.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <optional>

namespace themis::llm_wiki {
namespace {

constexpr const char* kPolicySchemaRelativePath =
    "src/llm_wiki/schema/llm_wiki_process_policy.schema.json";

std::optional<std::string> resolveExistingPath(const std::string& path) {
    if (path.empty()) {
        return std::nullopt;
    }

    if (auto mapped = themis::config::ConfigPathResolver::tryResolve(path)) {
        if (std::filesystem::exists(*mapped)) {
            return *mapped;
        }
    }

    if (std::filesystem::exists(path)) {
        return path;
    }
    return std::nullopt;
}

std::optional<std::string> findPolicySchemaPath(
    const std::filesystem::path& policy_path) {
    if (const char* env_schema = std::getenv("THEMIS_LLM_WIKI_PROCESS_POLICY_SCHEMA");
        env_schema != nullptr && *env_schema != '\0') {
        if (auto from_env = resolveExistingPath(env_schema)) {
            return from_env;
        }
    }

    const auto policy_dir = policy_path.parent_path();
    const auto near_policy =
        (policy_dir / ".." / "schema" / "llm_wiki_process_policy.schema.json")
            .lexically_normal()
            .string();
    if (auto resolved_near = resolveExistingPath(near_policy)) {
        return resolved_near;
    }

    if (auto canonical_relative = resolveExistingPath(kPolicySchemaRelativePath)) {
        return canonical_relative;
    }

    const auto from_source_tree =
        (std::filesystem::path(__FILE__).parent_path() /
         "schema/llm_wiki_process_policy.schema.json")
            .lexically_normal()
            .string();
    if (auto resolved_source = resolveExistingPath(from_source_tree)) {
        return resolved_source;
    }

    return std::nullopt;
}

template <typename T>
T safeJsonAs(const nlohmann::json& node, const char* key, const T& fallback) {
    try {
        if (node.is_object()) {
            const auto it = node.find(key);
            if (it != node.end() && !it->is_null()) {
                return it->get<T>();
            }
        }
    } catch (const std::exception&) {
    }
    return fallback;
}

ProcessSchedule parseSchedule(const nlohmann::json& node,
                              ProcessSchedule fallback) {
    std::string value = "near_realtime";
    try {
        if (node.is_string()) {
            value = node.get<std::string>();
        }
    } catch (const std::exception&) {
    }
    if (value == "interactive") {
        return ProcessSchedule::Interactive;
    }
    if (value == "batch") {
        return ProcessSchedule::Batch;
    }
    if (value == "near_realtime") {
        return ProcessSchedule::NearRealtime;
    }
    return fallback;
}

StagePolicy parseStage(const nlohmann::json& stage_node,
                       ProcessSchedule default_schedule) {
    StagePolicy policy;
    if (!stage_node.is_object()) {
        policy.enabled = false;
        policy.schedule = default_schedule;
        return policy;
    }
    policy.enabled = safeJsonAs<bool>(stage_node, "enabled", true);
    const auto sched_it = stage_node.find("schedule");
    policy.schedule = (sched_it != stage_node.end())
                          ? parseSchedule(*sched_it, default_schedule)
                          : default_schedule;
    return policy;
}

void parseHardBounds(const nlohmann::json& bounds_node,
                     std::unordered_map<std::string, KnobBounds>& out_bounds) {
    if (!bounds_node.is_object()) {
        return;
    }

    for (const auto& [knob, bounds] : bounds_node.items()) {
        if (knob.empty() || !bounds.is_object()) {
            continue;
        }

        const auto min_value = safeJsonAs<double>(bounds, "min", 0.0);
        const auto max_value = safeJsonAs<double>(bounds, "max", 0.0);
        out_bounds.emplace(knob, KnobBounds{min_value, max_value});
    }
}

std::vector<std::string> parseStringList(const nlohmann::json& node) {
    std::vector<std::string> values = {};

    if (!node.is_array()) {
        return values;
    }
    values.reserve(node.size());
    for (const auto& item : node) {
        try {
            if (item.is_string()) {
                values.push_back(item.get<std::string>());
            }
        } catch (const std::exception&) {
        }
    }
    return values;
}

int parsePositiveInt(const nlohmann::json& node,
                     const char* key,
                     int fallback = 0) {
    const auto value = safeJsonAs<int>(node, key, fallback);
    return value > 0 ? value : fallback;
}

} // namespace

ProcessPolicyStatus ProcessPolicyManager::loadFromYaml(
    const std::filesystem::path& yaml_path,
    LLMWikiProcessPolicy& out_policy) noexcept {
    try {
        if (yaml_path.empty() || !std::filesystem::exists(yaml_path)) {
            return ProcessPolicyStatus::InvalidFile(
                "YAML policy file does not exist: " + yaml_path.string());
        }

        const auto resolved =
            themis::config::ConfigPathResolver::tryResolve(yaml_path.string())
                .value_or(yaml_path.string());

        const auto schema_path = findPolicySchemaPath(yaml_path);
        if (!schema_path.has_value()) {
            return ProcessPolicyStatus::InvalidFile(
                "Unable to locate process policy schema file");
        }

        const auto schema_validation =
            themis::config::ConfigSchemaValidator::validateWithSchemaFile(
                resolved, *schema_path);
        if (!schema_validation.valid) {
            return ProcessPolicyStatus::ValidationError(
                "Schema validation failed: " + schema_validation.formatErrors());
        }

        const auto root = themis::config::ConfigSchemaValidator::loadAsJson(resolved);

        if (!root.is_object()) {
            return ProcessPolicyStatus::ParseError(
                "YAML root must be a map object");
        }

        LLMWikiProcessPolicy policy;
        policy.version = safeJsonAs<int>(root, "version", 1);
        policy.policy_id = safeJsonAs<std::string>(root, "policy_id", "");
        policy.mode = safeJsonAs<std::string>(root, "mode", "shadow");

        const auto orchestration_it = root.find("orchestration");
        if (orchestration_it != root.end() && orchestration_it->is_object()) {
            policy.planner_owner = safeJsonAs<std::string>(
                *orchestration_it, "planner_owner", "prompt_engineering");
            policy.second_planner_allowed = safeJsonAs<bool>(
                *orchestration_it, "second_planner_allowed", false);
            policy.interactive_timeout_ms = safeJsonAs<int>(
                *orchestration_it, "interactive_timeout_ms", 1500);
        }

        const auto stages_it = root.find("stages");
        const nlohmann::json stages = (stages_it != root.end() && stages_it->is_object())
                                          ? *stages_it
                                          : nlohmann::json::object();
        policy.ingest = parseStage(
            stages.value("ingest", nlohmann::json::object()),
            ProcessSchedule::NearRealtime);
        policy.extract = parseStage(
            stages.value("extract", nlohmann::json::object()),
            ProcessSchedule::NearRealtime);
        policy.synthesize = parseStage(
            stages.value("synthesize", nlohmann::json::object()),
            ProcessSchedule::Interactive);
        policy.validate = parseStage(
            stages.value("validate", nlohmann::json::object()),
            ProcessSchedule::Interactive);
        policy.re_anchor = parseStage(
            stages.value("re_anchor", nlohmann::json::object()),
            ProcessSchedule::Batch);

        const auto synthesize = stages.value("synthesize", nlohmann::json::object());
        if (synthesize.is_object()) {
            policy.synthesize_max_evidence_items =
                parsePositiveInt(synthesize, "max_evidence_items", 0);
            policy.synthesize_min_provenance_confidence =
                safeJsonAs<double>(synthesize, "min_provenance_confidence", -1.0);
        }

        const auto governance_it = root.find("governance");
        if (governance_it != root.end() && governance_it->is_object()) {
            policy.policy_snapshot_required = safeJsonAs<bool>(
                *governance_it, "policy_snapshot_required", true);
            policy.require_reason_codes = safeJsonAs<bool>(
                *governance_it, "require_reason_codes", true);
        }

        const auto ml_it = root.find("ml_control");
        if (ml_it != root.end() && ml_it->is_object()) {
            policy.adjustable_knobs = parseStringList(
                ml_it->value("adjustable_knobs", nlohmann::json::array()));

            const auto safety = ml_it->value("safety", nlohmann::json::object());
            if (safety.is_object()) {
                policy.never_adjust = parseStringList(
                    safety.value("never_adjust", nlohmann::json::array()));
            }

            parseHardBounds(
                ml_it->value("hard_bounds", nlohmann::json::object()),
                policy.hard_bounds);
        }

        const auto status = validate(policy);
        if (!status.ok()) {
            return status;
        }

        out_policy = std::move(policy);
        return ProcessPolicyStatus::Ok();
    } catch (const themis::config::SchemaValidationException& e) {
        return ProcessPolicyStatus::ParseError(
            std::string("Policy parse error: ") + e.what());
    } catch (const std::exception& e) {
        return ProcessPolicyStatus::ParseError(
            std::string("Unexpected policy load error: ") + e.what());
    }
}

ProcessPolicyStatus ProcessPolicyManager::validate(
    const LLMWikiProcessPolicy& policy) noexcept {
    if (policy.version < 1) {
        return ProcessPolicyStatus::ValidationError(
            "Policy version must be >= 1");
    }

    if (policy.policy_id.empty()) {
        return ProcessPolicyStatus::ValidationError(
            "policy_id must be non-empty");
    }

    if (policy.planner_owner != "prompt_engineering") {
        return ProcessPolicyStatus::ValidationError(
            "planner_owner must be 'prompt_engineering'");
    }

    if (policy.second_planner_allowed) {
        return ProcessPolicyStatus::ValidationError(
            "second_planner_allowed must stay false");
    }

    if (policy.interactive_timeout_ms < 100) {
        return ProcessPolicyStatus::ValidationError(
            "interactive_timeout_ms must be >= 100");
    }

    if (!policy.validate.enabled) {
        return ProcessPolicyStatus::ValidationError(
            "validate stage must stay enabled");
    }

    if (!policy.policy_snapshot_required) {
        return ProcessPolicyStatus::ValidationError(
            "policy_snapshot_required must stay true");
    }

    if (!policy.require_reason_codes) {
        return ProcessPolicyStatus::ValidationError(
            "require_reason_codes must stay true");
    }

    for (const auto& knob : policy.adjustable_knobs) {
        const auto it = policy.hard_bounds.find(knob);
        if (it == policy.hard_bounds.end()) {
            return ProcessPolicyStatus::ValidationError(
                "Missing hard bound for adjustable knob: " + knob);
        }
        if (it->second.min > it->second.max) {
            return ProcessPolicyStatus::ValidationError(
                "Invalid hard bound for knob: " + knob);
        }
    }

    constexpr std::array<const char*, 4> kNeverAdjustRequired = {
        "governance.policy_snapshot_required",
        "orchestration.second_planner_allowed",
        "stages.validate.fail_closed",
        "stages.ingest.requires",
    };

    for (const auto* required : kNeverAdjustRequired) {
        const auto found = std::find(policy.never_adjust.begin(),
                                     policy.never_adjust.end(), required);
        if (found == policy.never_adjust.end()) {
            return ProcessPolicyStatus::ValidationError(
                std::string("Missing required never_adjust invariant: ") +
                required);
        }
    }

    return ProcessPolicyStatus::Ok();
}

} // namespace themis::llm_wiki
