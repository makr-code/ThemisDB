/**
 * @file policy_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=27, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/policy_engine.h"
#include <stdexcept>
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "observability/metrics_collector.h"
#include <ctime>
#include <fstream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace themis {

using json = nlohmann::json;

static bool starts_with(const std::string& s, const std::string& prefix) {
    return static_cast<bool>( static_cast<int>(s.size()) < static_cast<int>(= prefix.size())) && std::equal(prefix.begin(), prefix.end(), s.begin());
}

// Emit a POLICY_UPDATED audit event if a logger is attached.
// Called while the policy mutex is NOT held so the logger can take its own locks.
static void emitPolicyAudit(utils::AuditLogger* logger,
                             const std::string& action,
                             const std::string& policy_id,
                             const std::string& detail = {}) {
    if (!logger) {
      return;
    }
    nlohmann::json meta;
    meta["action"]    = action;
    meta["policy_id"] = policy_id;
    if (!detail.empty()) {
      meta["detail"] = detail;
    }
    logger->logSecurityEvent(utils::SecurityEventType::POLICY_UPDATED,
                             "policy_engine",   // user / source
                             "policy/" + policy_id,
                             meta);
}

bool PolicyEngine::loadFromFile(const std::string& path, std::string* err) {
    try {
        auto ends_with = [](const std::string& s, const std::string& suffix) {
            return static_cast<bool>( static_cast<int>(s.size()) < static_cast<int>(= suffix.size() && s.compare(static_cast<int>(s.size()) - static_cast<int>(suffix.size()) ,static_cast<int>(suffix.size()))), suffix) == 0;
        };

        std::vector<Policy> loaded;

        if (ends_with(path, ".yaml") || ends_with(path, ".yml")) {
            // YAML parsing
            YAML::Node root = YAML::LoadFile(path);
            auto parse_policy_node = [&]([[maybe_unused]] const YAML::Node& n) -> std::optional<Policy> {
                try {
                    Policy p = {};
                    if (n["id"]) {
                      p.id = n["id"].as<std::string>("");
                    }
                    if (n["name"]) {
                      p.name = n["name"].as<std::string>("");
                    }
                    if (n["subjects"]) {
                        for (const auto& s : n["subjects"]) {
                          p.subjects.insert(s.as<std::string>());
                        }
                    }
                    if (n["actions"]) {
                        for (const auto& a : n["actions"]) {
                          p.actions.insert(a.as<std::string>());
                        }
                    }
                    if (n["resources"]) {
                        for (const auto& r : n["resources"]) {
                          p.resources.push_back(r.as<std::string>());
                        }
                    }
                    if (n["effect"]) {
                        auto eff = n["effect"].as<std::string>("allow");
                        p.effect_allow = (eff == "allow");
                    } else {
                        p.effect_allow = true;
                    }
                    if (n["allowed_ip_prefixes"]) {
                        for (const auto& ip : n["allowed_ip_prefixes"]) {
                          p.allowed_ip_prefixes.push_back(ip.as<std::string>());
                        }
                    }
                    if (n["time_window_utc_hours_start"]) {
                      p.time_window_utc_hours_start = n["time_window_utc_hours_start"].as<int>(-1);
                    }
                    if (n["time_window_utc_hours_end"]) {
                      p.time_window_utc_hours_end   = n["time_window_utc_hours_end"].as<int>(-1);
                    }
                    if (n["allowed_user_agent_patterns"]) {
                        for (const auto& ua : n["allowed_user_agent_patterns"]) {
                          p.allowed_user_agent_patterns.push_back(ua.as<std::string>());
                        }
                    }
                    return p;
                } catch (...) {
                    THEMIS_WARN("policy_engine: unhandled exception caught");
                    return std::nullopt;
                }
            };

            if (root.IsSequence()) {
                for (const auto& item : root) {
                    auto p = parse_policy_node(item);
                    if (p) {
                      loaded.push_back(std::move(*p));
                    }
                }
            } else if (root.IsMap() && root["policies"]) {
                const auto& arr = root["policies"];
                if (arr && arr.IsSequence()) {
                    for (const auto& item : arr) {
                        auto p = parse_policy_node(item);
                        if (p) {
                          loaded.push_back(std::move(*p));
                        }
                    }
                }
            } else {
                if (err) *err = "unsupported YAML structure (expect sequence or {policies: [...]})";
                return false;
            }
        } else {
            // JSON parsing
            std::ifstream f(path);
            if (!f) {
                if (err) {
                  *err = "cannot open policies file";
                }
                return false;
            }
            json j; f >> j;
            if (j.is_array()) {
                for (const auto& pj : j) {
                    auto p = fromJson(pj);
                    if (p) {
                      loaded.push_back(std::move(*p));
                    }
                }
            } else if (j.is_object() && j.contains("policies")) {
                for (const auto& pj : j["policies"]) {
                    auto p = fromJson(pj);
                    if (p) {
                      loaded.push_back(std::move(*p));
                    }
                }
            }
        }

        setPolicies(std::move(loaded));
        // Remember path and file mtime for hot-reload
        loaded_file_path_ = path;
        try {
            auto mtime = std::filesystem::last_write_time(path);
            last_loaded_mtime_ = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                mtime - decltype(mtime)::clock::now() + std::chrono::system_clock::now());
        } catch (...) {
            THEMIS_WARN("policy_engine: unhandled exception caught");
            last_loaded_mtime_ = std::chrono::system_clock::now();
        }
        return true;
    } catch (const std::exception& e) {
        if (err) {
          *err = e.what();
        }
        return false;
    }
}

bool PolicyEngine::saveToFile(const std::string& path, std::string* err) const {
    try {
        json out = json::array();
        auto list = listPolicies();
        for (const auto& p : list) {
          out.push_back(toJson(p));
        }
        std::ofstream f(path);
        if (!f) { if (err) *err = "cannot write policies file"; return false; }
        f << out.dump(2);
        return true;
    } catch (const std::exception& e) {
        if (err) {
          *err = e.what();
        }
        return false;
    }
}

bool PolicyEngine::reloadIfChanged(std::string* err) {
    // Fast read of the stored path under the lock
    std::string path = {};
    std::chrono::system_clock::time_point last_mtime;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        path       = loaded_file_path_;
        last_mtime = last_loaded_mtime_;
    }

    if (path.empty()) {
        // No file was ever loaded – nothing to do
        return true;
    }

    // Check modification time without holding the policy lock
    std::chrono::system_clock::time_point current_mtime;
    try {
        auto ft = std::filesystem::last_write_time(path);
        current_mtime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - decltype(ft)::clock::now() + std::chrono::system_clock::now());
    } catch (const std::exception& e) {
        if (err) {
          *err = std::string("stat failed: ") + e.what();
        }
        return false;
    }

    if (current_mtime <= last_mtime) {
        return true;  // File unchanged
    }

    // File has changed – reload
    return loadFromFile(path, err);
}

void PolicyEngine::setPolicies(std::vector<Policy> policies) {
    utils::AuditLogger* logger = nullptr;
    size_t count = policies.size();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        policies_ = std::move(policies);
        logger = audit_logger_;
    }
    emitPolicyAudit(logger, "set_all",
                    "(bulk)",
                    "replaced all policies, new count=" + std::to_string(count));
}

void PolicyEngine::addPolicy(const Policy& p) {
    utils::AuditLogger* logger = nullptr;
    std::string id = p.id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (config_.max_policies > 0 && static_cast<int>(policies_.size()) >= config_.max_policies) {
            throw std::length_error(
                "PolicyEngine: max_policies limit (" +
                std::to_string(config_.max_policies) + ") reached");
        }
        policies_.push_back(p);
        logger = audit_logger_;
    }
    emitPolicyAudit(logger, "add", id);
}

bool PolicyEngine::removePolicy(const std::string& id) {
    utils::AuditLogger* logger = nullptr;
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto size_before = policies_.size();
        policies_.erase(std::remove_if(policies_.begin(), policies_.end(),
                                       [&]([[maybe_unused]] const Policy& p){ return p.id == id; }),
                        policies_.end());
        removed = policies_.size() != size_before;
        logger  = audit_logger_;
    }
    if (removed) {
      emitPolicyAudit(logger, "remove", id);
    }
    return removed;
}

std::vector<PolicyEngine::Policy> PolicyEngine::listPolicies() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return policies_;
}

PolicyEngine::Decision PolicyEngine::authorize(const std::string& user_id,
                                               const std::string& action,
                                               const std::string& resource_path,
                                               const std::optional<std::string>& client_ip,
                                               const std::optional<std::string>& user_agent) const {
    metrics_.policy_eval_total++;

    // If an OPA evaluator is configured, try it first.
    // Fall back to native evaluation when OPA is unavailable (returns nullopt).
    if (opa_evaluator_) {
        auto opa_result = opa_evaluator_->evaluate(
            user_id, action, resource_path, client_ip, user_agent);
        if (opa_result.has_value()) {
            if (opa_result->allowed) {
                metrics_.policy_allow_total++;
            } else {
                metrics_.policy_deny_total++;
            }
            return *opa_result;
        }
        // OPA unavailable – fall through to native evaluation.
        metrics_.opa_fallback_total++;
        observability::MetricsCollector::getInstance().addCounter(
            "governance_opa_fallback_total", 1, {{"source", "policy_engine"}});
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // If no policies defined, default deny (fail-closed).
    // Loading a policy file that does not exist leaves policies_ empty; allowing
    // all access in that state would silently bypass authorization.
    if (policies_.empty()) {
        metrics_.policy_deny_total++;
        return {false, "", "no_policies_default_deny"};
    }

    // Evaluate in order: first matching policy decides
    for (const auto& p : policies_) {
        if (!matchSubject(p, user_id)) {
          continue;
        }
        if (!matchAction(p, action)) {
          continue;
        }
        if (!matchResource(p, resource_path)) {
          continue;
        }
        if (!matchConditions(p, client_ip, user_agent)) {
          continue;
        }

        if (p.effect_allow) {
            metrics_.policy_allow_total++;
            return {true, p.id, "matched_allow_policy"};
        } else {
            metrics_.policy_deny_total++;
            return {false, p.id, "matched_deny_policy"};
        }
    }

    // No match -> deny by default (secure by default)
    metrics_.policy_deny_total++;
    return {false, "", "no_matching_policy"};
}

bool PolicyEngine::matchSubject(const Policy& p, const std::string& user_id) const {
    if (p.subjects.count("*") > 0) {
      return true;
    }
    return p.subjects.count(user_id) > 0;
}

bool PolicyEngine::matchAction(const Policy& p, const std::string& action) const {
    if (p.actions.count("*") > 0) {
      return true;
    }
    return p.actions.count(action) > 0;
}

bool PolicyEngine::matchResource(const Policy& p, const std::string& resource_path) const {
    if (p.resources.empty()) return true; // no restriction
    for (const auto& r : p.resources) {
        if (starts_with(resource_path, r)) {
          return true;
        }
    }
    return false;
}

bool PolicyEngine::matchConditions(const Policy& p,
                                   const std::optional<std::string>& client_ip,
                                   const std::optional<std::string>& user_agent) const {
    // IP condition
    if (!p.allowed_ip_prefixes.empty()) {
        if (!client_ip) return false; // IP required to evaluate
        bool ok = false;
        for (const auto& prefix : p.allowed_ip_prefixes) {
            if (starts_with(*client_ip, prefix)) { ok = true; break; }
        }
        if (!ok) {
          return false;
        }
    }

    // ABAC time-window condition (UTC hour of day)
    if (p.time_window_utc_hours_start >= 0 && p.time_window_utc_hours_end >= 0) {
        std::time_t now_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::tm utc_tm{};
#if defined(_WIN32)
        gmtime_s(&utc_tm, &now_t);
#else
        gmtime_r(&now_t, &utc_tm);
#endif
        int hour = utc_tm.tm_hour;
        bool in_window = false;
        if (p.time_window_utc_hours_start <= p.time_window_utc_hours_end) {
            // Normal range, e.g. 08–18
            in_window = (hour >= p.time_window_utc_hours_start &&
                         hour <= p.time_window_utc_hours_end);
        } else {
            // Overnight range, e.g. 22–06
            in_window = (hour >= p.time_window_utc_hours_start ||
                         hour <= p.time_window_utc_hours_end);
        }
        if (!in_window) {
          return false;
        }
    }

    // ABAC user-agent allowlist (substring match)
    if (!p.allowed_user_agent_patterns.empty()) {
        if (!user_agent) return false; // UA required when patterns configured
        bool ok = false;
        for (const auto& pat : p.allowed_user_agent_patterns) {
            if (user_agent->find(pat) != std::string::npos) { ok = true; break; }
        }
        if (!ok) {
          return false;
        }
    }

    return true;
}

json PolicyEngine::toJson(const Policy& p) {
    json j;
    j["id"] = p.id;
    j["name"] = p.name;
    j["subjects"] = json::array(); for (const auto& s : p.subjects) j["subjects"].push_back(s);
    j["actions"] = json::array(); for (const auto& a : p.actions) j["actions"].push_back(a);
    j["resources"] = p.resources;
    j["effect"] = p.effect_allow ? "allow" : "deny";
    if (!p.allowed_ip_prefixes.empty()) {
      j["allowed_ip_prefixes"] = p.allowed_ip_prefixes;
    }
    if (p.time_window_utc_hours_start >= 0) {
      j["time_window_utc_hours_start"] = p.time_window_utc_hours_start;
    }
    if (p.time_window_utc_hours_end   >= 0) {
      j["time_window_utc_hours_end"]   = p.time_window_utc_hours_end;
    }
    if (!p.allowed_user_agent_patterns.empty()) {
      j["allowed_user_agent_patterns"] = p.allowed_user_agent_patterns;
    }
    return j;
}

std::optional<PolicyEngine::Policy> PolicyEngine::fromJson(const json& j) {
    try {
        Policy p;
        p.id = j.value("id", "");
        p.name = j.value("name", "");
        if (j.contains("subjects")) {
          for (const auto& s : j["subjects"]) {
            p.subjects.insert(s.get<std::string>());
          }
        }
        if (j.contains("actions")) {
          for (const auto& a : j["actions"]) {
            p.actions.insert(a.get<std::string>());
          }
        }
        if (j.contains("resources")) {
          for (const auto& r : j["resources"]) {
            p.resources.push_back(r.get<std::string>());
          }
        }
        std::string eff = j.value("effect", std::string("allow"));
        p.effect_allow = (eff == "allow");
        if (j.contains("allowed_ip_prefixes")) {
          for (const auto& ip : j["allowed_ip_prefixes"]) {
            p.allowed_ip_prefixes.push_back(ip.get<std::string>());
          }
        }
        p.time_window_utc_hours_start = j.value("time_window_utc_hours_start", -1);
        p.time_window_utc_hours_end   = j.value("time_window_utc_hours_end",   -1);
        if (j.contains("allowed_user_agent_patterns")) {
          for (const auto& ua : j["allowed_user_agent_patterns"]) {
            p.allowed_user_agent_patterns.push_back(ua.get<std::string>());
          }
        }
        return p;
    } catch (...) {
        THEMIS_WARN("policy_engine: unhandled exception caught");
        return std::nullopt;
    }
}

} // namespace themis


