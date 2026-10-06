#include "process/process_orchestration_registry.h"

#include "utils/logger.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <variant>

#if defined(HAVE_YAML_CPP) || __has_include(<yaml-cpp/yaml.h>)
#include <yaml-cpp/yaml.h>
#define THEMIS_PROCESS_HAS_YAML_CPP 1
#endif

namespace themis::process {

namespace {

std::string trim(std::string value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(),
                value.end());
    return value;
}

nlohmann::json yamlNodeToJson(
#if defined(THEMIS_PROCESS_HAS_YAML_CPP)
    const YAML::Node& node
#else
    const int& /*unused*/
#endif
) {
#if defined(THEMIS_PROCESS_HAS_YAML_CPP)
    if (!node) {
        return nullptr;
    }
    if (node.IsScalar()) {
        return node.as<std::string>("");
    }
    if (node.IsSequence()) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : node) {
            arr.push_back(yamlNodeToJson(item));
        }
        return arr;
    }
    if (node.IsMap()) {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto& item : node) {
            obj[item.first.as<std::string>("")] = yamlNodeToJson(item.second);
        }
        return obj;
    }
#endif
    return nlohmann::json{};
}

std::optional<nlohmann::json> loadStructuredFile(std::string_view path, std::string& out_format) {
    namespace fs = std::filesystem;

    const fs::path file_path(path);
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        return std::nullopt;
    }

    const auto ext = file_path.extension().string();
    const auto ext_lower = [&ext]() {
        std::string v = ext;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return v;
    }();

    if (ext_lower == ".json") {
        std::ifstream in(file_path);
        if (!in.good()) {
            return std::nullopt;
        }
        nlohmann::json doc;
        in >> doc;
        out_format = "json";
        return doc;
    }

#if defined(THEMIS_PROCESS_HAS_YAML_CPP)
    if (ext_lower == ".yaml" || ext_lower == ".yml") {
        const auto yaml_doc = YAML::LoadFile(file_path.string());
        out_format = "yaml";
        return yamlNodeToJson(yaml_doc);
    }
#endif

    return std::nullopt;
}

} // namespace

nlohmann::json OrchestrationProfileRecord::toDocument() const {
    return {
        {"id", id},
        {"domain", domain},
        {"version", version},
        {"source_path", source_path},
        {"source_format", source_format},
        {"state", ProcessOrchestrationRegistry::stateToString(state)},
        {"revision", revision},
        {"internal_rag_stage", internal_rag_stage},
        {"created_at_ms", created_at_ms},
        {"updated_at_ms", updated_at_ms},
        {"model", model}
    };
}

std::optional<OrchestrationProfileRecord> OrchestrationProfileRecord::fromDocument(
    const nlohmann::json& doc) {
    if (!doc.is_object()) {
        return std::nullopt;
    }

    auto record = OrchestrationProfileRecord{};
    record.id = doc.value("id", "");
    record.domain = doc.value("domain", "");
    if (record.id.empty() || record.domain.empty()) {
        return std::nullopt;
    }

    record.version = doc.value("version", "");
    record.source_path = doc.value("source_path", "");
    record.source_format = doc.value("source_format", "unknown");
    record.revision = doc.value("revision", static_cast<uint64_t>(1));
    record.internal_rag_stage = doc.value("internal_rag_stage", true);
    record.created_at_ms = doc.value("created_at_ms", static_cast<int64_t>(0));
    record.updated_at_ms = doc.value("updated_at_ms", static_cast<int64_t>(0));
    record.model = doc.value("model", nlohmann::json::object());

    const auto state = ProcessOrchestrationRegistry::stateFromString(
        doc.value("state", std::string{"STAGED"}));
    record.state = state.value_or(OrchestrationProfileState::STAGED);

    return record;
}

nlohmann::json ProcessPredictionOutput::toJson() const {
    return {
        {"sla_breach_probability", sla_breach_probability},
        {"compliance_risk_score", compliance_risk_score},
        {"completion_confidence", completion_confidence},
        {"recommended_submodel", recommended_submodel},
        {"evidence", evidence}
    };
}

ProcessOrchestrationRegistry::ProcessOrchestrationRegistry(
    ::themis::RocksDBWrapper& db,
    ProcessGraphRag& rag)
    : db_(db)
    , rag_(rag) {
}

ProcessModelResult ProcessOrchestrationRegistry::upsertProfile(
    const nlohmann::json& profile,
    std::string_view source_path,
    std::string_view source_format,
    bool activate_if_staged) {
    if (!profile.is_object()) {
        return ProcessModelResult::failure("Orchestration profile must be a JSON object");
    }

    const auto id_opt = extractProfileId(profile);
    const auto domain_opt = extractDomain(profile);
    if (!id_opt || !domain_opt) {
        return ProcessModelResult::failure("Orchestration profile requires id/method_id and domain");
    }

    if (!extractInternalRagStage(profile)) {
        return ProcessModelResult::failure("Orchestration profile must enforce internal_rag_stage=true");
    }

    auto record = OrchestrationProfileRecord{};
    if (const auto existing = getProfile(*id_opt); existing.has_value()) {
        record = *existing;
        record.revision += 1;
        record.created_at_ms = existing->created_at_ms;
    }

    record.id = *id_opt;
    record.domain = normalizeDomain(*domain_opt);
    record.version = profile.value("version", "v1");
    record.source_path = std::string(source_path);
    record.source_format = std::string(source_format.empty() ? "json" : source_format);
    record.internal_rag_stage = true;
    record.model = profile;
    record.updated_at_ms = nowMs();
    if (record.created_at_ms == 0) {
        record.created_at_ms = record.updated_at_ms;
    }

    if (record.model.contains("bpmn") && record.model["bpmn"].is_object()) {
        record.model["bpmn"]["internal_rag_stage"] = true;
    } else {
        record.model["internal_rag_stage"] = true;
    }

    if (record.state == OrchestrationProfileState::ARCHIVED) {
        record.state = OrchestrationProfileState::STAGED;
    }

    auto persist = persistProfile(record);
    if (!persist.ok) {
        return persist;
    }

    if (activate_if_staged) {
        return activateProfile(record.domain, record.id);
    }

    return ProcessModelResult::success(record.id);
}

ProcessModelResult ProcessOrchestrationRegistry::upsertProfileFromFile(
    std::string_view profile_path,
    bool activate_if_staged) {
    std::string format;
    auto doc = loadStructuredFile(profile_path, format);
    if (!doc.has_value()) {
        return ProcessModelResult::failure(
            "Failed to load orchestration profile file: " + std::string(profile_path));
    }
    return upsertProfile(*doc, profile_path, format, activate_if_staged);
}

std::variant<size_t, ProcessModelResult> ProcessOrchestrationRegistry::syncFromCatalog(
    std::string_view catalog_path,
    bool activate_ingested) {
#if !defined(THEMIS_PROCESS_HAS_YAML_CPP)
    return ProcessModelResult::failure(
        "Catalog sync requires yaml-cpp support (HAVE_YAML_CPP)");
#else
    namespace fs = std::filesystem;
    const fs::path catalog(catalog_path);
    if (!fs::exists(catalog)) {
        return ProcessModelResult::failure("Catalog file not found: " + std::string(catalog_path));
    }

    const YAML::Node root = YAML::LoadFile(catalog.string());
    const YAML::Node models = root["models"];
    if (!models || !models.IsSequence()) {
        return ProcessModelResult::failure("Catalog missing models sequence");
    }

    size_t ingested = 0;
    for (const auto& model_entry : models) {
        const auto type = model_entry["type"].as<std::string>("");
        if (type != "process") {
            continue;
        }

        const auto rel_path = model_entry["path"].as<std::string>("");
        if (rel_path.empty()) {
            continue;
        }

        fs::path resolved = rel_path;
        if (resolved.is_relative()) {
            resolved = catalog.parent_path() / resolved;
        }

        auto res = upsertProfileFromFile(resolved.string(), activate_ingested);
        if (!res.ok) {
            return res;
        }
        ++ingested;
    }

    return ingested;
#endif
}

ProcessModelResult ProcessOrchestrationRegistry::activateProfile(
    std::string_view domain,
    std::string_view profile_id) {
    auto record_opt = getProfile(profile_id);
    if (!record_opt) {
        return ProcessModelResult::failure("Profile not found: " + std::string(profile_id));
    }

    auto record = *record_opt;
    const auto normalized_domain = normalizeDomain(domain);
    if (record.domain != normalized_domain) {
        return ProcessModelResult::failure("Profile domain mismatch during activation");
    }
    if (record.state == OrchestrationProfileState::ARCHIVED) {
        return ProcessModelResult::failure("Archived profile cannot be activated");
    }

    if (auto active = getActiveProfile(normalized_domain); active.has_value()) {
        if (active->id != record.id) {
            auto previous = *active;
            previous.state = OrchestrationProfileState::STAGED;
            previous.updated_at_ms = nowMs();
            auto demote = persistProfile(previous);
            if (!demote.ok) {
                return demote;
            }
        }
    }

    record.state = OrchestrationProfileState::ACTIVE;
    record.updated_at_ms = nowMs();
    auto persist = persistProfile(record);
    if (!persist.ok) {
        return persist;
    }

    if (!db_.put(activeDomainKey(normalized_domain), std::string(profile_id))) {
        return ProcessModelResult::failure("Failed to update active profile pointer");
    }

    return ProcessModelResult::success(std::string(profile_id));
}

ProcessModelResult ProcessOrchestrationRegistry::archiveProfile(std::string_view profile_id) {
    auto record_opt = getProfile(profile_id);
    if (!record_opt) {
        return ProcessModelResult::failure("Profile not found: " + std::string(profile_id));
    }

    auto record = *record_opt;
    const auto domain_key = activeDomainKey(record.domain);

    std::string active_id;
    if (db_.get(domain_key, active_id) && trim(active_id) == record.id) {
        db_.del(domain_key);
    }

    record.state = OrchestrationProfileState::ARCHIVED;
    record.updated_at_ms = nowMs();
    return persistProfile(record);
}

ProcessModelResult ProcessOrchestrationRegistry::rollbackDomain(
    std::string_view domain,
    std::string_view profile_id) {
    return activateProfile(domain, profile_id);
}

std::optional<OrchestrationProfileRecord> ProcessOrchestrationRegistry::getProfile(
    std::string_view profile_id) const {
    std::string payload;
    if (!db_.get(profileKey(profile_id), payload)) {
        return std::nullopt;
    }

    try {
        auto doc = nlohmann::json::parse(payload);
        return OrchestrationProfileRecord::fromDocument(doc);
    } catch (...) {
        THEMIS_WARN("ProcessOrchestrationRegistry: malformed profile payload for id={}.",
                    profile_id);
        return std::nullopt;
    }
}

std::optional<OrchestrationProfileRecord> ProcessOrchestrationRegistry::getActiveProfile(
    std::string_view domain) const {
    std::string id;
    if (!db_.get(activeDomainKey(normalizeDomain(domain)), id)) {
        return std::nullopt;
    }
    return getProfile(trim(id));
}

std::vector<OrchestrationProfileRecord> ProcessOrchestrationRegistry::listProfiles(
    std::string_view domain_filter,
    std::optional<OrchestrationProfileState> state_filter,
    size_t limit) const {
    std::vector<OrchestrationProfileRecord> out;
    const auto prefix = std::string{"proc:orch:"};
    const auto normalized_domain = normalizeDomain(domain_filter);

    db_.scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
        const std::string key_s(key);
        if (key_s.find(":rev:") != std::string::npos ||
            key_s.rfind("proc:orch:active:", 0) == 0) {
            return true;
        }

        try {
            auto doc = nlohmann::json::parse(value.begin(), value.end());
            auto rec_opt = OrchestrationProfileRecord::fromDocument(doc);
            if (!rec_opt) {
                return true;
            }

            const auto& rec = *rec_opt;
            if (!normalized_domain.empty() && rec.domain != normalized_domain) {
                return true;
            }
            if (state_filter.has_value() && rec.state != *state_filter) {
                return true;
            }

            out.push_back(rec);
            if (limit > 0 && out.size() >= limit) {
                return false;
            }
        } catch (...) {
            return true;
        }
        return true;
    });

    std::sort(out.begin(), out.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.updated_at_ms > rhs.updated_at_ms;
    });
    return out;
}

ProcessPredictionOutput ProcessOrchestrationRegistry::predictOutcome(
    std::string_view instance_id,
    std::string_view domain) const {
    auto prediction = ProcessPredictionOutput{};

    const auto summary = rag_.summarizeVerwaltungsvorgang(instance_id);
    const auto compliance = rag_.checkCompliance(instance_id);
    const auto similar_cases = rag_.findSimilarCases(instance_id, 6, 0.55f);

    const auto progress_pct = summary.value("progress_pct", 0.0);
    const auto missing_docs = summary.value("missing_documents", nlohmann::json::array()).size();
    const auto sla_status = summary.value("sla_status", std::string{"unknown"});

    prediction.compliance_risk_score = std::clamp(1.0 - static_cast<double>(compliance.compliance_score),
                                                  0.0,
                                                  1.0);

    double sla_base = 0.15;
    if (sla_status == "at_risk") {
        sla_base = 0.55;
    } else if (sla_status == "overdue") {
        sla_base = 0.85;
    }
    sla_base += std::min<double>(0.25, static_cast<double>(missing_docs) * 0.05);
    prediction.sla_breach_probability = std::clamp(sla_base, 0.0, 1.0);

    const auto similarity_boost = similar_cases.empty()
        ? 0.0
        : static_cast<double>(similar_cases.front().similarity) * 0.35;
    const auto progress_boost = std::clamp(progress_pct / 100.0, 0.0, 1.0) * 0.55;
    const auto violation_penalty = std::min<double>(0.3, compliance.violations.size() * 0.08);
    prediction.completion_confidence = std::clamp(
        0.2 + similarity_boost + progress_boost - violation_penalty,
        0.0,
        1.0);

    std::string resolved_domain = normalizeDomain(domain);
    if (resolved_domain.empty()) {
        resolved_domain = normalizeDomain(summary.value("domain", std::string{}));
    }
    if (!resolved_domain.empty()) {
        if (auto active = getActiveProfile(resolved_domain); active.has_value()) {
            prediction.recommended_submodel = active->id;
        }
    }

    prediction.evidence = {
        {"instance_id", instance_id},
        {"progress_pct", progress_pct},
        {"missing_documents_count", missing_docs},
        {"sla_status", sla_status},
        {"compliance_score", compliance.compliance_score},
        {"violation_count", compliance.violations.size()},
        {"similar_case_count", similar_cases.size()},
        {"knowledge_source_class", nlohmann::json::array({"graph", "rag", "wiki"})}
    };

    return prediction;
}

std::string ProcessOrchestrationRegistry::stateToString(OrchestrationProfileState state) {
    switch (state) {
    case OrchestrationProfileState::STAGED:
        return "STAGED";
    case OrchestrationProfileState::ACTIVE:
        return "ACTIVE";
    case OrchestrationProfileState::ARCHIVED:
        return "ARCHIVED";
    }
    return "STAGED";
}

std::optional<OrchestrationProfileState> ProcessOrchestrationRegistry::stateFromString(
    std::string_view state) {
    if (state == "STAGED") {
        return OrchestrationProfileState::STAGED;
    }
    if (state == "ACTIVE") {
        return OrchestrationProfileState::ACTIVE;
    }
    if (state == "ARCHIVED") {
        return OrchestrationProfileState::ARCHIVED;
    }
    return std::nullopt;
}

int64_t ProcessOrchestrationRegistry::nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string ProcessOrchestrationRegistry::normalizeDomain(std::string_view domain) {
    std::string out(domain);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return trim(out);
}

std::optional<std::string> ProcessOrchestrationRegistry::extractProfileId(
    const nlohmann::json& profile) {
    const std::array<std::string, 3> keys{"id", "method_id", "model_id"};
    for (const auto& key : keys) {
        if (profile.contains(key) && profile[key].is_string()) {
            const auto id = trim(profile[key].get<std::string>());
            if (!id.empty()) {
                return id;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> ProcessOrchestrationRegistry::extractDomain(
    const nlohmann::json& profile) {
    if (profile.contains("domain") && profile["domain"].is_string()) {
        const auto domain = trim(profile["domain"].get<std::string>());
        if (!domain.empty()) {
            return domain;
        }
    }

    if (profile.contains("bindings") && profile["bindings"].is_object() &&
        profile["bindings"].contains("domain") && profile["bindings"]["domain"].is_string()) {
        const auto domain = trim(profile["bindings"]["domain"].get<std::string>());
        if (!domain.empty()) {
            return domain;
        }
    }

    return std::nullopt;
}

bool ProcessOrchestrationRegistry::extractInternalRagStage(const nlohmann::json& profile) {
    if (profile.contains("internal_rag_stage")) {
        return profile.value("internal_rag_stage", false);
    }
    if (profile.contains("bpmn") && profile["bpmn"].is_object() &&
        profile["bpmn"].contains("internal_rag_stage")) {
        return profile["bpmn"].value("internal_rag_stage", false);
    }
    return true;
}

ProcessModelResult ProcessOrchestrationRegistry::persistProfile(
    const OrchestrationProfileRecord& record) {
    const auto payload = record.toDocument().dump();
    if (!db_.put(profileKey(record.id), payload)) {
        return ProcessModelResult::failure("Failed to persist orchestration profile");
    }

    if (!db_.put(profileRevisionKey(record.id, record.revision), payload)) {
        return ProcessModelResult::failure("Failed to persist orchestration revision");
    }

    return ProcessModelResult::success(record.id);
}

std::string ProcessOrchestrationRegistry::profileKey(std::string_view id) {
    return "proc:orch:" + std::string(id);
}

std::string ProcessOrchestrationRegistry::profileRevisionKey(
    std::string_view id,
    uint64_t revision) {
    return "proc:orch:" + std::string(id) + ":rev:" + std::to_string(revision);
}

std::string ProcessOrchestrationRegistry::activeDomainKey(std::string_view domain) {
    return "proc:orch:active:" + std::string(domain);
}

} // namespace themis::process
