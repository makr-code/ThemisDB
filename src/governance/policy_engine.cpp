/**
 * @file policy_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=14; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=7, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_engine.h"
#include <stdexcept>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "governance/governance_diagnostics.h"
#include "governance/model_governance.h"
#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

std::string PolicyEngine::normalize(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    // trim spaces
    auto is_space = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!out.empty() && is_space(static_cast<unsigned char>(out.front()))) {
        out.erase(out.begin());
    }
    while (!out.empty() && is_space(static_cast<unsigned char>(out.back()))) {
        out.pop_back();
    }
    return out;
}

bool PolicyEngine::isStrictClass(const std::string &cls) {
    auto c = normalize(cls);
    return (c == "geheim" || c == "streng-geheim");
}

bool PolicyEngine::loadFromYAML(const std::string &yaml_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);

        std::unordered_map<std::string, ClassificationProfile> new_profiles;
        std::unordered_map<std::string, std::string> new_mapping;
        std::string new_mode = "enforce";

        // Load VS classification profiles
        if (config["vs_classification"]) {
            const auto &vs = config["vs_classification"];
            for (const auto &kv : vs) {
                std::string level = kv.first.as<std::string>();
                ClassificationProfile profile;
                profile.level = level;

                const auto &val = kv.second;
                if (val["encryption_required"]) {
                    profile.encryption_required = val["encryption_required"].as<bool>();
                }
                if (val["ann_allowed"]) {
                    profile.ann_allowed = val["ann_allowed"].as<bool>();
                }
                if (val["export_allowed"]) {
                    profile.export_allowed = val["export_allowed"].as<bool>();
                }
                if (val["cache_allowed"]) {
                    profile.cache_allowed = val["cache_allowed"].as<bool>();
                }
                if (val["redaction_level"]) {
                    profile.redaction_level = val["redaction_level"].as<std::string>();
                }
                if (val["retention_days"]) {
                    profile.retention_days = val["retention_days"].as<int>();
                }
                if (val["log_encryption"]) {
                    profile.log_encryption = val["log_encryption"].as<bool>();
                }

                new_profiles[normalize(level)] = profile;
            }
        }

        // Load enforcement resource mappings
        if (config["enforcement"] && config["enforcement"]["resource_mapping"]) {
            const auto &mappings = config["enforcement"]["resource_mapping"];
            for (const auto &kv : mappings) {
                std::string resource  = kv.first.as<std::string>();
                std::string min_class = kv.second.as<std::string>();
                new_mapping[resource] = normalize(min_class);
            }
        }

        // Load default mode
        if (config["enforcement"] && config["enforcement"]["default_mode"]) {
            new_mode = normalize(config["enforcement"]["default_mode"].as<std::string>());
        }

        // Load data masking configuration
        FieldMaskingPolicy new_masking = {};
        if (config["data_masking"]) {
            const auto &dm = config["data_masking"];
            if (dm["enabled"]) {
                new_masking.enabled = dm["enabled"].as<bool>();
            }
            if (dm["rules"] && dm["rules"].IsSequence()) {
                for (const auto &r : dm["rules"]) {
                    FieldMaskingRule rule = {};
                    if (r["field"]) {
                        rule.field_name = r["field"].as<std::string>();
                    }
                    if (r["strategy"]) {
                        const std::string strat = normalize(r["strategy"].as<std::string>());
                        if (strat == "tokenize") {
                            rule.strategy = MaskingStrategy::TOKENIZE;
                        } else if (strat == "truncate") {
                            rule.strategy = MaskingStrategy::TRUNCATE;
                        } else if (strat == "hash") {
                            rule.strategy = MaskingStrategy::HASH;
                        } else {
                            rule.strategy = MaskingStrategy::REDACT;
                        }
                    }
                    if (r["truncate_length"]) {
                        rule.truncate_length = r["truncate_length"].as<int>();
                    }
                    if (r["collection_secret"]) {
                        rule.collection_secret = r["collection_secret"].as<std::string>();
                        // Warn if using placeholder secret (TOKENIZE strategy requires secure secret)
                        if (rule.strategy == MaskingStrategy::TOKENIZE
                            && rule.collection_secret == "change-me-in-production") {
                            THEMIS_WARN("Policy field '{}': collection_secret is still set to placeholder "
                                        "'change-me-in-production'. This is insecure; the placeholder will be "
                                        "used as-is, producing predictable pseudonyms. Configure a strong, "
                                        "unique secret from your KMS or environment variables before "
                                        "production use.",
                                        rule.field_name);
                        }
                    }
                    if (!rule.field_name.empty()) {
                        new_masking.rules.push_back(std::move(rule));
                    }
                }
            }
        }

        // Capture mtime before taking the lock to minimise lock hold time
        std::filesystem::file_time_type mtime{};
        try {
            mtime = std::filesystem::last_write_time(yaml_path);
        } catch (...) {
            // If stat fails use a zero time_point; reloadIfChanged will retry
        }

        // Capture sizes for logging before the move (must be outside the lock)
        const size_t n_profiles = new_profiles.size();
        const size_t n_mappings = new_mapping.size();
        const size_t n_masking  = new_masking.rules.size();

        // Atomically swap policy data under the mutex
        {
            std::lock_guard<std::mutex> lock(mutex_);
            classification_profiles_ = std::move(new_profiles);
            resource_mapping_        = std::move(new_mapping);
            default_mode_            = std::move(new_mode);
            masking_rules_           = std::move(new_masking);
            loaded_yaml_path_        = yaml_path;
            last_loaded_mtime_       = mtime;
        }

        THEMIS_INFO("Loaded governance policies from {}: {} classifications, {} resource mappings, {} masking rules",
                    yaml_path, n_profiles, n_mappings, n_masking);
        return true;

    } catch (const YAML::Exception &e) {
        THEMIS_ERROR("Failed to load governance YAML from {}: {}", yaml_path, e.what());
        return false;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to load governance config: {}", e.what());
        return false;
    }
}

bool PolicyEngine::reloadIfChanged(std::string *err) {
    // Read state under the lock then release before touching the filesystem
    std::string path = {};
    std::filesystem::file_time_type last_mtime;
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        path       = loaded_yaml_path_;
        last_mtime = last_loaded_mtime_;
        audit_log  = audit_logger_;
    }

    if (path.empty()) {
        // No file was ever loaded – nothing to do
        return true;
    }

    std::filesystem::file_time_type current_mtime;
    try {
        current_mtime = std::filesystem::last_write_time(path);
    } catch (const std::exception &e) {
        if (err) {
            *err = std::string("stat failed: ") + e.what();
        }
        observability::MetricsCollector::getInstance().addCounter("governance_policy_reload_total", 1,
                                                                  {{"result", "failure"}});
        return false;
    }

    if (current_mtime <= last_mtime) {
        return true; // File unchanged – fast no-op
    }

    // Compute a version identifier from the file mtime for the audit trail.
    // We use the raw file_clock tick count as an opaque version hash; this is
    // portable across C++17/20 without requiring clock_cast.
    const int64_t old_version = static_cast<int64_t>(last_mtime.time_since_epoch().count());
    const int64_t new_version = static_cast<int64_t>(current_mtime.time_since_epoch().count());

    // File has changed – reload atomically
    THEMIS_INFO("PolicyEngine: governance policy file changed, reloading: {}", path);
    const bool ok = loadFromYAML(path);

    // Emit Prometheus counter per spec (governance_policy_reload_total)
    observability::MetricsCollector::getInstance().addCounter("governance_policy_reload_total", 1,
                                                              {{"result", ok ? "success" : "failure"}});

    // Write audit entry with old and new policy version hashes
    if (audit_log) {
        nlohmann::json audit_event = {{"event_type", "policy_reload"},
                                      {"file", path},
                                      {"old_version", old_version},
                                      {"new_version", new_version},
                                      {"result", ok ? "success" : "failure"},
                                      {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        std::chrono::system_clock::now().time_since_epoch())
                                                        .count()}};
        if (!ok && err && !err->empty()) {
            audit_event["error"] = *err;
        }
        audit_log->logEvent(audit_event);
    }

    return ok;
}

std::string PolicyEngine::getLoadedFilePath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_yaml_path_;
}

std::optional<ClassificationProfile> PolicyEngine::getClassificationProfile(const std::string &level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = classification_profiles_.find(normalize(level));
    if (it == classification_profiles_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PolicyEngine::setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(logger);
}

void PolicyEngine::setOpaEvaluator(IPolicyEvaluator *evaluator) {
    std::lock_guard<std::mutex> lock(mutex_);
    opa_evaluator_ = evaluator;
}

void PolicyEngine::setCcpaOptOutSubjects(std::shared_ptr<std::unordered_set<std::string>> opt_out_registry) {
    std::lock_guard<std::mutex> lock(mutex_);
    ccpa_opt_out_subjects_ = std::move(opt_out_registry);
    THEMIS_INFO("PolicyEngine: CCPA opt-out registry updated ({} subjects)",
                ccpa_opt_out_subjects_ ? ccpa_opt_out_subjects_->size() : 0u);
}

bool PolicyEngine::isCcpaOptedOut(const std::string &subject_id) const {
    if (subject_id.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ccpa_opt_out_subjects_) {
        return false;
    }
    return ccpa_opt_out_subjects_->count(subject_id) > 0;
}

PolicyDecision PolicyEngine::evaluate(const std::unordered_map<std::string, std::string> &headers,
                                      const std::string &route) const {
    auto get = [&]([[maybe_unused]] const char *key) -> std::string {
        auto it = headers.find(key);
        if (it != headers.end()) {
            return it->second;
        }
        return std::string();
    };

    // Snapshot policy data under lock so a concurrent reload doesn't race
    std::unordered_map<std::string, ClassificationProfile> profiles;
    std::unordered_map<std::string, std::string> resource_map;
    std::string mode = {};
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    std::shared_ptr<std::unordered_set<std::string>> ccpa_registry;
    IPolicyEvaluator *evaluator = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        profiles      = classification_profiles_;
        resource_map  = resource_mapping_;
        mode          = default_mode_;
        audit_log     = audit_logger_;
        ccpa_registry = ccpa_opt_out_subjects_;
        evaluator     = opa_evaluator_;
    }

    // ---- OPA evaluation (alternative policy engine) -----------------------
    // If an OPA evaluator is configured, try it first.  Fall back to native
    // evaluation when OPA is unavailable (returns nullopt) and emit counter.
    if (evaluator) {
        auto opa_result = evaluator->evaluate(headers, route);
        if (opa_result.has_value()) {
            PolicyDecision d = *opa_result;
            // Always enforce CCPA opt-out on top of OPA decision
            const std::string subject_id = get("X-User-Id");
            if (!subject_id.empty() && ccpa_registry && ccpa_registry->count(subject_id) > 0) {
                d.ccpa_opted_out = true;
                d.export_allowed = false;
            }
            // Audit log if in enforce mode and logger is configured
            if (audit_log && d.mode == "enforce") {
                nlohmann::json audit_event = {{"event_type", "policy_evaluation"},
                                              {"route", route},
                                              {"classification", d.classification},
                                              {"mode", d.mode},
                                              {"require_content_encryption", d.require_content_encryption},
                                              {"encrypt_logs", d.encrypt_logs},
                                              {"redaction", d.redaction},
                                              {"retention_days", d.retention_days},
                                              {"ccpa_opted_out", d.ccpa_opted_out},
                                              {"evaluator", "opa"},
                                              {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                std::chrono::system_clock::now().time_since_epoch())
                                                                .count()}};
                if (!subject_id.empty()) {
                    audit_event["user_id"] = subject_id;
                }
                audit_log->logEvent(audit_event);
            }
            return d;
        }
        // OPA unavailable – fall through to native evaluation and emit counter.
        observability::MetricsCollector::getInstance().addCounter("governance_opa_fallback_total", 1,
                                                                  {{"source", "policy_engine"}});
    }

    PolicyDecision d;

    // Classification
    auto cls = normalize(get("X-Classification"));
    if (cls.empty()) {
        // Check resource mapping for default
        auto res_it = resource_map.find(route);
        if (res_it != resource_map.end()) {
            cls = res_it->second;
        } else {
            // Phase 3A: Fail-closed default (deny-by-default)
            // Instead of defaulting to "vs-nfd", require explicit classification
            // or deny access
            cls = "streng-geheim";  // Strictest default (deny-by-default)
        }
    }
    d.classification = cls;

    // Mode
    auto req_mode = normalize(get("X-Governance-Mode"));
    if (req_mode != "observe") {
        req_mode = mode;
    }
    d.mode = req_mode;

    // Lookup profile
    auto prof_it = profiles.find(normalize(cls));
    if (prof_it != profiles.end()) {
        const auto &profile          = prof_it->second;
        d.encrypt_logs               = profile.log_encryption;
        d.redaction                  = profile.redaction_level;
        d.ann_allowed                = profile.ann_allowed;
        d.require_content_encryption = profile.encryption_required;
        d.export_allowed             = profile.export_allowed;
        d.cache_allowed              = profile.cache_allowed;
        d.retention_days             = profile.retention_days;
    } else {
        // Phase 3A: Fail-closed fallback (deny-by-default, no implicit allows)
        // Profile not found - apply strictest security posture
        d.encrypt_logs               = true;      // Always encrypt logs
        d.redaction                  = "strict";  // Strictest redaction
        d.ann_allowed                = false;     // Deny approximate NN
        d.require_content_encryption = true;      // Always require encryption
        d.export_allowed             = false;     // Deny export
        d.cache_allowed              = false;     // Deny caching
        d.retention_days             = 7;         // Minimal retention
    }

    // Allow header override for encrypt_logs
    auto enc_logs = normalize(get("X-Encrypt-Logs"));
    if (!enc_logs.empty()) {
        if (enc_logs == "true" || enc_logs == "1" || enc_logs == "yes") {
            d.encrypt_logs = true;
        } else if (enc_logs == "false" || enc_logs == "0" || enc_logs == "no") {
            d.encrypt_logs = false;
        }
    }

    // Allow header override for redaction
    auto redact = normalize(get("X-Redaction-Level"));
    if (!redact.empty()) {
        d.redaction = redact;
    }

    // ---- CCPA/CPRA opt-out enforcement ------------------------------------
    // If the requesting subject has opted out of data sale, override
    // export_allowed=false so the query layer cannot forward this data to
    // third parties.  The opt-out check adds negligible overhead (<< 0.5 ms)
    // because it is a single hash-set lookup on the snapshotted registry.
    const std::string subject_id = get("X-User-Id");
    if (!subject_id.empty() && ccpa_registry && ccpa_registry->count(subject_id) > 0) {
        d.ccpa_opted_out = true;
        d.export_allowed = false; // Data sale / third-party export blocked
    }

    // Audit log if in enforce mode and logger is configured
    if (audit_log && d.mode == "enforce") {
        nlohmann::json audit_event = {{"event_type", "policy_evaluation"},
                                      {"route", route},
                                      {"classification", d.classification},
                                      {"mode", d.mode},
                                      {"require_content_encryption", d.require_content_encryption},
                                      {"encrypt_logs", d.encrypt_logs},
                                      {"redaction", d.redaction},
                                      {"retention_days", d.retention_days},
                                      {"ccpa_opted_out", d.ccpa_opted_out},
                                      {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        std::chrono::system_clock::now().time_since_epoch())
                                                        .count()}};

        // Add user context if available in headers
        if (!subject_id.empty()) {
            audit_event["user_id"] = subject_id;
        }

        audit_log->logEvent(audit_event);
    }

    return d;
}

SimulationResult PolicyEngine::simulateDecision(const SimulationRequest &request) const {
    const auto &headers = request.headers;
    const auto &route   = request.route;

    auto get = [&]([[maybe_unused]] const char *key) -> std::string {
        auto it = headers.find(key);
        if (it != headers.end()) {
            return it->second;
        }
        return std::string();
    };

    // Snapshot policy data under lock so a concurrent reload doesn't race
    std::unordered_map<std::string, ClassificationProfile> profiles;
    std::unordered_map<std::string, std::string> resource_map;
    std::string mode = {};
    IPolicyEvaluator *evaluator = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        profiles     = classification_profiles_;
        resource_map = resource_mapping_;
        mode         = default_mode_;
        evaluator    = opa_evaluator_;
        // audit_logger_ is intentionally NOT captured – dry-run must not log
    }

    SimulationResult result;
    result.dry_run = true;

    // ---- OPA evaluation in dry-run mode ------------------------------------
    // If an OPA evaluator is configured, use it so that the simulation
    // accurately reflects what evaluate() would return.  Audit logging is
    // intentionally suppressed (side-effect-free requirement).
    if (evaluator) {
        auto opa_result = evaluator->evaluate(headers, route);
        if (opa_result.has_value()) {
            result.decision        = *opa_result;
            result.matched_profile = "opa";
            // NOTE: No audit log – dry-run must not write audit entries.
            return result;
        }
        // OPA unavailable – fall through to native simulation.
        // No fallback counter emitted here (side-effect-free requirement).
    }

    PolicyDecision &d = result.decision;

    // Classification
    auto cls = normalize(get("X-Classification"));
    if (cls.empty()) {
        auto res_it = resource_map.find(route);
        if (res_it != resource_map.end()) {
            cls                     = res_it->second;
            result.matched_resource = route;
        } else {
            cls = "vs-nfd"; // ultimate default
        }
    }
    d.classification = cls;

    // Mode
    auto req_mode = normalize(get("X-Governance-Mode"));
    if (req_mode != "observe") {
        req_mode = mode;
    }
    d.mode = req_mode;

    // Lookup profile
    auto prof_it = profiles.find(normalize(cls));
    if (prof_it != profiles.end()) {
        const auto &profile          = prof_it->second;
        d.encrypt_logs               = profile.log_encryption;
        d.redaction                  = profile.redaction_level;
        d.ann_allowed                = profile.ann_allowed;
        d.require_content_encryption = profile.encryption_required;
        d.export_allowed             = profile.export_allowed;
        d.cache_allowed              = profile.cache_allowed;
        d.retention_days             = profile.retention_days;
        result.matched_profile       = profile.level;
    } else {
        // Fallback if profile not found (heuristic)
        bool strict                  = isStrictClass(cls);
        d.encrypt_logs               = strict;
        d.redaction                  = strict ? "strict" : "standard";
        d.ann_allowed                = !strict;
        d.require_content_encryption = strict;
        d.export_allowed             = !strict;
        d.cache_allowed              = !strict;
        d.retention_days             = 365;
    }

    // Allow header override for encrypt_logs
    auto enc_logs = normalize(get("X-Encrypt-Logs"));
    if (!enc_logs.empty()) {
        if (enc_logs == "true" || enc_logs == "1" || enc_logs == "yes") {
            d.encrypt_logs = true;
        } else if (enc_logs == "false" || enc_logs == "0" || enc_logs == "no") {
            d.encrypt_logs = false;
        }
    }

    // Allow header override for redaction
    auto redact = normalize(get("X-Redaction-Level"));
    if (!redact.empty()) {
        d.redaction = redact;
    }

    // NOTE: Dry-run / simulation mode – audit log is intentionally NOT written.
    return result;
}

void PolicyEngine::setModelGovernancePolicy(std::shared_ptr<ModelGovernancePolicy> policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    model_governance_policy_ = std::move(policy);
}

ModelGovernanceDecision PolicyEngine::checkExportPermission(const ModelTrainingExportRequest &request) const {
    // Snapshot the model governance policy under the lock (may be null)
    std::shared_ptr<ModelGovernancePolicy> mgp;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        mgp = model_governance_policy_;
    }

    if (mgp) {
        // Delegate entirely to the configured ModelGovernancePolicy
        return mgp->checkExportPermission(request);
    }

    // ── Fallback: no ModelGovernancePolicy configured ─────────────────────────
    // Apply the built-in classification rule: "geheim" and "streng-geheim"
    // data must never be exported for model training.
    const std::string cls_lower = [&] {
        std::string s = request.classification;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        return s;
    }();

    ModelGovernanceDecision decision = {};
    if (cls_lower == "geheim" || cls_lower == "streng-geheim") {
        decision.is_permitted = false;
        decision.denial_reason
            = "Data classification '" + request.classification + "' is not permitted for model training";
        THEMIS_WARN("PolicyEngine::checkExportPermission: denied for job '{}': {}", request.export_job_id,
                    decision.denial_reason);
    } else {
        decision.is_permitted = true;
        THEMIS_INFO("PolicyEngine::checkExportPermission: permitted for job '{}' (fallback path)",
                    request.export_job_id);
    }
    return decision;
}

FieldMaskingPolicy PolicyEngine::getMaskingPolicy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return masking_rules_;
}

QueryPermissionResult PolicyEngine::checkQueryPermission(const std::unordered_map<std::string, std::string> &headers,
                                                         const std::string &route) const {
    QueryPermissionResult result;

    // Reuse the standard evaluate() path for the PolicyDecision so that CCPA,
    // classification lookup, audit logging, and all existing logic remain in
    // one place.
    result.decision = evaluate(headers, route);

    // Attach the masking policy (snapshot under the lock).
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result.masking_policy = masking_rules_;
    }

    return result;
}

InferencePermissionResult
PolicyEngine::checkInferencePermission(const std::unordered_map<std::string, std::string> &headers) const {
    InferencePermissionResult result;

    // ── Step 1: extract the API key from the Authorization header ──────────
    // Accept "Bearer <key>" format (standard OpenAI SDK convention).
    static const std::string k_route         = "/v1/chat/completions";
    static const std::string k_auth_header   = "Authorization";
    static const std::string k_bearer_prefix = "Bearer ";

    auto find_header_ci = [&headers](const std::string &key) -> std::optional<std::string> {
        auto it = headers.find(key);
        if (it != headers.end()) {
            return it->second;
        }
        for (const auto &kv : headers) {
            if (static_cast<int>(kv.first.size()) != key.size()) {
                continue;
            }
            bool equal_ci = true;
            for (size_t i = 0; i < key.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(kv.first[i]))
                    != std::tolower(static_cast<unsigned char>(key[i]))) {
                    equal_ci = false;
                    break;
                }
            }
            if (equal_ci) {
                return kv.second;
            }
        }
        return std::nullopt;
    };

    auto auth_value_opt = find_header_ci(k_auth_header);
    if (!auth_value_opt.has_value() || auth_value_opt->empty()) {
        result.allowed       = false;
        result.http_status   = 401;
        result.denial_reason = "Missing Authorization header; provide a Bearer API key";
        return result;
    }

    const std::string &auth_value = *auth_value_opt;
    const bool has_bearer_prefix
        = auth_value.size() >= k_bearer_prefix.size()
          && std::equal(k_bearer_prefix.begin(), k_bearer_prefix.end(), auth_value.begin(), [](char a, char b) {
                 return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
             });

    if (!has_bearer_prefix) {
        result.allowed       = false;
        result.http_status   = 401;
        result.denial_reason = "Invalid Authorization header format; expected 'Bearer <api-key>'";
        return result;
    }

    const std::string api_key = auth_value.substr(k_bearer_prefix.size());
    if (api_key.empty()) {
        result.allowed       = false;
        result.http_status   = 401;
        result.denial_reason = "Empty API key in Authorization header";
        return result;
    }

    // ── Step 2: evaluate the governance policy for this request ────────────
    // Propagate the extracted identity via X-Api-Key so evaluate() can apply
    // classification and CCPA rules that depend on the caller identity.
    std::unordered_map<std::string, std::string> eval_headers = headers;
    eval_headers["X-Api-Key"]                                 = api_key;

    try {
        result.decision = evaluate(eval_headers, k_route);
    } catch (const std::exception &ex) {
        result.allowed       = false;
        result.http_status   = 403;
        result.denial_reason = std::string("Policy evaluation error: ") + ex.what();
        return result;
    }

    // ── Step 3: map the policy decision to an allow/deny outcome ──────────
    // LLM inference is blocked when the classification is strict ("geheim" /
    // "streng-geheim") or when ANN/inference is explicitly disabled by policy.
    if (!result.decision.ann_allowed) {
        result.allowed       = false;
        result.http_status   = 403;
        result.denial_reason = "Inference is not permitted for the current data classification";
        return result;
    }

    result.allowed = true;
    return result;
}

SafeAccessResult PolicyEngine::validateAccessSafety(const AccessRequest& request) {
    // Lazily initialize safety validator if not already done
    if (!safety_validator_) {
        safety_validator_ = std::make_unique<SafeAccessValidator>(
            &getGlobalDiagnosticAggregator()
        );
    }
    
    return safety_validator_->validateAccessRequest(request);
}

SafeAccessValidator& PolicyEngine::getSafeAccessValidator() {
    if (!safety_validator_) {
        safety_validator_ = std::make_unique<SafeAccessValidator>(
            &getGlobalDiagnosticAggregator()
        );
    }
    return *safety_validator_;
}

} // namespace governance
} // namespace themis

