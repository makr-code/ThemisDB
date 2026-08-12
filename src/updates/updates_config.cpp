/**
 * @file updates_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/updates_config.h"
#include "updates/canary_rollout.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// UpdatesConfig::CanaryConfig bridge
// ---------------------------------------------------------------------------

::themis::updates::CanaryConfig
UpdatesConfig::CanaryConfig::toCanaryConfig(const std::string& version) const {
    ::themis::updates::CanaryConfig runtime;
    runtime.version = version;
    runtime.node_id = node_id;
    runtime.error_rate_threshold = error_rate_threshold;
    runtime.min_sample_count = min_sample_count;
    runtime.stages.reserve(stages.size());
    for (const auto& s : stages) {
        CanaryStage stage;
        stage.percentage = s.percentage;
        stage.observation_duration = std::chrono::seconds{s.observation_seconds};
        runtime.stages.push_back(stage);
    }
    return runtime;
}

UpdatesConfig UpdatesConfig::loadFromYaml(const std::string& yaml_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        UpdatesConfig result;
        
        // Load update checker settings
        if (config["updates"] && config["updates"]["checker"]) {
            auto checker = config["updates"]["checker"];
            result.checker.enabled = checker["enabled"].as<bool>(false);
            result.checker.check_interval = std::chrono::seconds(
                checker["check_interval_seconds"].as<int>(3600)
            );
            result.checker.github_owner = checker["github_owner"].as<std::string>("makr-code");
            result.checker.github_repo = checker["github_repo"].as<std::string>("ThemisDB");
            result.checker.github_api_url = checker["github_api_url"].as<std::string>("https://api.github.com");
            
            if (checker["github_api_token"]) {
                result.checker.github_api_token = checker["github_api_token"].as<std::string>();
            }
            if (checker["proxy_url"]) {
                result.checker.proxy_url = checker["proxy_url"].as<std::string>();
            }
        }
        
        // Load auto-update settings
        if (config["updates"] && config["updates"]["auto_update"]) {
            auto auto_update = config["updates"]["auto_update"];
            result.auto_update.enabled = auto_update["enabled"].as<bool>(false);
            result.auto_update.critical_only = auto_update["critical_only"].as<bool>(true);
            result.auto_update.require_approval = auto_update["require_approval"].as<bool>(true);
            result.auto_update.approval_timeout = std::chrono::seconds(
                auto_update["approval_timeout_seconds"].as<int>(300)
            );
            result.auto_update.scheduled = auto_update["scheduled"].as<bool>(false);
            result.auto_update.schedule_time = auto_update["schedule_time"].as<std::string>("02:00");
            
            if (auto_update["schedule_days"]) {
                result.auto_update.schedule_days.clear();
                for (const auto& day : auto_update["schedule_days"]) {
                    result.auto_update.schedule_days.push_back(day.as<std::string>());
                }
            }
        }
        
        // Load hot-reload settings
        if (config["updates"] && config["updates"]["hot_reload"]) {
            auto hot_reload = config["updates"]["hot_reload"];
            result.hot_reload.enabled = hot_reload["enabled"].as<bool>(false);
            result.hot_reload.download_directory = hot_reload["download_directory"].as<std::string>("/tmp/themis_updates");
            result.hot_reload.backup_directory = hot_reload["backup_directory"].as<std::string>("/var/lib/themisdb/rollback");
            result.hot_reload.install_directory = hot_reload["install_directory"].as<std::string>(".");
            result.hot_reload.verify_signatures = hot_reload["verify_signatures"].as<bool>(true);
            result.hot_reload.create_backup = hot_reload["create_backup"].as<bool>(true);
            result.hot_reload.keep_rollback_points = hot_reload["keep_rollback_points"].as<int>(3);
            result.hot_reload.download_timeout_seconds = hot_reload["download_timeout_seconds"].as<int>(300);
            result.hot_reload.max_retries = hot_reload["max_retries"].as<int>(3);
            result.hot_reload.retry_delay_seconds = hot_reload["retry_delay_seconds"].as<int>(5);
        }
        
        // Load notification settings
        if (config["updates"] && config["updates"]["notifications"]) {
            auto notifications = config["updates"]["notifications"];
            result.notifications.enabled = notifications["enabled"].as<bool>(false);
            
            if (notifications["on_events"]) {
                result.notifications.on_events.clear();
                for (const auto& event : notifications["on_events"]) {
                    result.notifications.on_events.push_back(event.as<std::string>());
                }
            }
            
            if (notifications["webhook_url"]) {
                result.notifications.webhook_url = notifications["webhook_url"].as<std::string>();
            }
            if (notifications["email_to"]) {
                result.notifications.email_to = notifications["email_to"].as<std::string>();
            }
        }

        // Load canary rollout settings
        if (config["updates"] && config["updates"]["canary"]) {
            auto canary_yaml = config["updates"]["canary"];
            result.canary.enabled = canary_yaml["enabled"].as<bool>(false);
            result.canary.node_id = canary_yaml["node_id"].as<std::string>("");
            result.canary.error_rate_threshold = canary_yaml["error_rate_threshold"].as<double>(0.05);
            result.canary.min_sample_count = canary_yaml["min_sample_count"].as<size_t>(20);
            if (canary_yaml["stages"]) {
                result.canary.stages.clear();
                for (const auto& s : canary_yaml["stages"]) {
                    UpdatesConfig::CanaryConfig::Stage stage;
                    stage.percentage = s["percentage"].as<double>(1.0);
                    stage.observation_seconds = s["observation_seconds"].as<int>(0);
                    result.canary.stages.push_back(stage);
                }
            }
        }

        // Load anonymous hardware telemetry settings
        if (config["updates"] && config["updates"]["telemetry"]) {
            auto tel = config["updates"]["telemetry"];
            result.telemetry.enabled =
                tel["enabled"].as<bool>(false);
            result.telemetry.endpoint_url =
                tel["endpoint_url"].as<std::string>(
                    "https://api.themisdb.org/telemetry.php");
            result.telemetry.send_interval_seconds =
                std::max(86400, tel["send_interval_seconds"].as<int>(86400));
            result.telemetry.include_cpu_model =
                tel["include_cpu_model"].as<bool>(true);
            result.telemetry.include_cpu_cores =
                tel["include_cpu_cores"].as<bool>(true);
            result.telemetry.include_ram_mb =
                tel["include_ram_mb"].as<bool>(true);
            result.telemetry.include_os =
                tel["include_os"].as<bool>(true);
            result.telemetry.include_arch =
                tel["include_arch"].as<bool>(true);
            result.telemetry.http_timeout_seconds =
                tel["http_timeout_seconds"].as<int>(10);
            result.telemetry.max_retries =
                tel["max_retries"].as<int>(2);
        }
        
        LOG_INFO("Loaded updates configuration from {}", yaml_path);
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load updates configuration from {}: {}", yaml_path, e.what());
        return UpdatesConfig();  // Return default config
    }
}

UpdatesConfig UpdatesConfig::fromJson(const json& j) {
    UpdatesConfig result;
    
    try {
        // Load update checker settings
        if (j.contains("checker")) {
            auto checker = j["checker"];
            result.checker.enabled = checker.value("enabled", false);
            result.checker.check_interval = std::chrono::seconds(
                checker.value("check_interval_seconds", 3600)
            );
            result.checker.github_owner = checker.value("github_owner", "makr-code");
            result.checker.github_repo = checker.value("github_repo", "ThemisDB");
            result.checker.github_api_url = checker.value("github_api_url", "https://api.github.com");
            result.checker.github_api_token = checker.value("github_api_token", "");
            result.checker.proxy_url = checker.value("proxy_url", "");
        }
        
        // Load auto-update settings
        if (j.contains("auto_update")) {
            auto auto_update = j["auto_update"];
            result.auto_update.enabled = auto_update.value("enabled", false);
            result.auto_update.critical_only = auto_update.value("critical_only", true);
            result.auto_update.require_approval = auto_update.value("require_approval", true);
            result.auto_update.approval_timeout = std::chrono::seconds(
                auto_update.value("approval_timeout_seconds", 300)
            );
            result.auto_update.scheduled = auto_update.value("scheduled", false);
            result.auto_update.schedule_time = auto_update.value("schedule_time", "02:00");
            
            if (auto_update.contains("schedule_days")) {
                result.auto_update.schedule_days = auto_update["schedule_days"].get<std::vector<std::string>>();
            }
        }
        
        // Load hot-reload settings
        if (j.contains("hot_reload")) {
            auto hot_reload = j["hot_reload"];
            result.hot_reload.enabled = hot_reload.value("enabled", false);
            result.hot_reload.download_directory = hot_reload.value("download_directory", "/tmp/themis_updates");
            result.hot_reload.backup_directory = hot_reload.value("backup_directory", "/var/lib/themisdb/rollback");
            result.hot_reload.install_directory = hot_reload.value("install_directory", ".");
            result.hot_reload.verify_signatures = hot_reload.value("verify_signatures", true);
            result.hot_reload.create_backup = hot_reload.value("create_backup", true);
            result.hot_reload.keep_rollback_points = hot_reload.value("keep_rollback_points", 3);
            result.hot_reload.download_timeout_seconds = hot_reload.value("download_timeout_seconds", 300);
            result.hot_reload.max_retries = hot_reload.value("max_retries", 3);
            result.hot_reload.retry_delay_seconds = hot_reload.value("retry_delay_seconds", 5);
        }
        
        // Load notification settings
        if (j.contains("notifications")) {
            auto notifications = j["notifications"];
            result.notifications.enabled = notifications.value("enabled", false);
            
            if (notifications.contains("on_events")) {
                result.notifications.on_events = notifications["on_events"].get<std::vector<std::string>>();
            }
            
            result.notifications.webhook_url = notifications.value("webhook_url", "");
            result.notifications.email_to = notifications.value("email_to", "");
        }

        // Load canary rollout settings
        if (j.contains("canary")) {
            auto canary_json = j["canary"];
            result.canary.enabled = canary_json.value("enabled", false);
            result.canary.node_id = canary_json.value("node_id", "");
            result.canary.error_rate_threshold = canary_json.value("error_rate_threshold", 0.05);
            result.canary.min_sample_count = canary_json.value("min_sample_count", static_cast<size_t>(20));
            if (canary_json.contains("stages")) {
                result.canary.stages.clear();
                for (const auto& s : canary_json["stages"]) {
                    UpdatesConfig::CanaryConfig::Stage stage;
                    stage.percentage = s.value("percentage", 1.0);
                    stage.observation_seconds = s.value("observation_seconds", 0);
                    result.canary.stages.push_back(stage);
                }
            }
        }

        // Load anonymous hardware telemetry settings
        if (j.contains("telemetry")) {
            auto tel = j["telemetry"];
            result.telemetry.enabled =
                tel.value("enabled", false);
            result.telemetry.endpoint_url =
                tel.value("endpoint_url",
                          std::string("https://api.themisdb.org/telemetry.php"));
            result.telemetry.send_interval_seconds =
                std::max(86400, tel.value("send_interval_seconds", 86400));
            result.telemetry.include_cpu_model =
                tel.value("include_cpu_model", true);
            result.telemetry.include_cpu_cores =
                tel.value("include_cpu_cores", true);
            result.telemetry.include_ram_mb =
                tel.value("include_ram_mb", true);
            result.telemetry.include_os =
                tel.value("include_os", true);
            result.telemetry.include_arch =
                tel.value("include_arch", true);
            result.telemetry.http_timeout_seconds =
                tel.value("http_timeout_seconds", 10);
            result.telemetry.max_retries =
                tel.value("max_retries", 2);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse updates configuration from JSON: {}", e.what());
    }
    
    return result;
}

json UpdatesConfig::toJson() const {
    json j;
    
    // Checker config
    j["checker"]["enabled"] = checker.enabled;
    j["checker"]["check_interval_seconds"] = checker.check_interval.count();
    j["checker"]["github_owner"] = checker.github_owner;
    j["checker"]["github_repo"] = checker.github_repo;
    j["checker"]["github_api_url"] = checker.github_api_url;
    
    // Mask token
    if (!checker.github_api_token.empty()) {
        j["checker"]["github_api_token"] = "***";
    }
    if (!checker.proxy_url.empty()) {
        j["checker"]["proxy_url"] = checker.proxy_url;
    }
    
    // Auto-update config
    j["auto_update"]["enabled"] = auto_update.enabled;
    j["auto_update"]["critical_only"] = auto_update.critical_only;
    j["auto_update"]["require_approval"] = auto_update.require_approval;
    j["auto_update"]["approval_timeout_seconds"] = auto_update.approval_timeout.count();
    j["auto_update"]["scheduled"] = auto_update.scheduled;
    j["auto_update"]["schedule_time"] = auto_update.schedule_time;
    j["auto_update"]["schedule_days"] = auto_update.schedule_days;
    
    // Hot-reload config
    j["hot_reload"]["enabled"] = hot_reload.enabled;
    j["hot_reload"]["download_directory"] = hot_reload.download_directory;
    j["hot_reload"]["backup_directory"] = hot_reload.backup_directory;
    j["hot_reload"]["install_directory"] = hot_reload.install_directory;
    j["hot_reload"]["verify_signatures"] = hot_reload.verify_signatures;
    j["hot_reload"]["create_backup"] = hot_reload.create_backup;
    j["hot_reload"]["keep_rollback_points"] = hot_reload.keep_rollback_points;
    j["hot_reload"]["download_timeout_seconds"] = hot_reload.download_timeout_seconds;
    j["hot_reload"]["max_retries"] = hot_reload.max_retries;
    j["hot_reload"]["retry_delay_seconds"] = hot_reload.retry_delay_seconds;
    
    // Notification config
    j["notifications"]["enabled"] = notifications.enabled;
    j["notifications"]["on_events"] = notifications.on_events;
    if (!notifications.webhook_url.empty()) {
        j["notifications"]["webhook_url"] = notifications.webhook_url;
    }
    if (!notifications.email_to.empty()) {
        j["notifications"]["email_to"] = notifications.email_to;
    }

    // Canary rollout config
    j["canary"]["enabled"] = canary.enabled;
    j["canary"]["node_id"] = canary.node_id;
    j["canary"]["error_rate_threshold"] = canary.error_rate_threshold;
    j["canary"]["min_sample_count"] = canary.min_sample_count;
    j["canary"]["stages"] = json::array();
    for (const auto& s : canary.stages) {
        json stage_json;
        stage_json["percentage"] = s.percentage;
        stage_json["observation_seconds"] = s.observation_seconds;
        j["canary"]["stages"].push_back(stage_json);
    }

    // Anonymous hardware telemetry config
    j["telemetry"]["enabled"]                = telemetry.enabled;
    j["telemetry"]["endpoint_url"]           = telemetry.endpoint_url;
    j["telemetry"]["send_interval_seconds"]  = telemetry.send_interval_seconds;
    j["telemetry"]["include_cpu_model"]      = telemetry.include_cpu_model;
    j["telemetry"]["include_cpu_cores"]      = telemetry.include_cpu_cores;
    j["telemetry"]["include_ram_mb"]         = telemetry.include_ram_mb;
    j["telemetry"]["include_os"]             = telemetry.include_os;
    j["telemetry"]["include_arch"]           = telemetry.include_arch;
    j["telemetry"]["http_timeout_seconds"]   = telemetry.http_timeout_seconds;
    j["telemetry"]["max_retries"]            = telemetry.max_retries;

    return j;
}

void UpdatesConfig::saveToYaml(const std::string& yaml_path) const {
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "updates";
        out << YAML::Value << YAML::BeginMap;
        
        // Checker config
        out << YAML::Key << "checker";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << checker.enabled;
        out << YAML::Key << "check_interval_seconds" << YAML::Value << checker.check_interval.count();
        out << YAML::Key << "github_owner" << YAML::Value << checker.github_owner;
        out << YAML::Key << "github_repo" << YAML::Value << checker.github_repo;
        out << YAML::Key << "github_api_url" << YAML::Value << checker.github_api_url;
        if (!checker.github_api_token.empty()) {
            out << YAML::Key << "github_api_token" << YAML::Value << "***";  // Masked
        }
        if (!checker.proxy_url.empty()) {
            out << YAML::Key << "proxy_url" << YAML::Value << checker.proxy_url;
        }
        out << YAML::EndMap;
        
        // Auto-update config
        out << YAML::Key << "auto_update";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << auto_update.enabled;
        out << YAML::Key << "critical_only" << YAML::Value << auto_update.critical_only;
        out << YAML::Key << "require_approval" << YAML::Value << auto_update.require_approval;
        out << YAML::Key << "approval_timeout_seconds" << YAML::Value << auto_update.approval_timeout.count();
        out << YAML::Key << "scheduled" << YAML::Value << auto_update.scheduled;
        out << YAML::Key << "schedule_time" << YAML::Value << auto_update.schedule_time;
        out << YAML::Key << "schedule_days" << YAML::Value << YAML::Flow << auto_update.schedule_days;
        out << YAML::EndMap;
        
        // Hot-reload config
        out << YAML::Key << "hot_reload";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << hot_reload.enabled;
        out << YAML::Key << "download_directory" << YAML::Value << hot_reload.download_directory;
        out << YAML::Key << "backup_directory" << YAML::Value << hot_reload.backup_directory;
        out << YAML::Key << "install_directory" << YAML::Value << hot_reload.install_directory;
        out << YAML::Key << "verify_signatures" << YAML::Value << hot_reload.verify_signatures;
        out << YAML::Key << "create_backup" << YAML::Value << hot_reload.create_backup;
        out << YAML::Key << "keep_rollback_points" << YAML::Value << hot_reload.keep_rollback_points;
        out << YAML::Key << "download_timeout_seconds" << YAML::Value << hot_reload.download_timeout_seconds;
        out << YAML::Key << "max_retries" << YAML::Value << hot_reload.max_retries;
        out << YAML::Key << "retry_delay_seconds" << YAML::Value << hot_reload.retry_delay_seconds;
        out << YAML::EndMap;
        
        // Notification config
        out << YAML::Key << "notifications";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << notifications.enabled;
        out << YAML::Key << "on_events" << YAML::Value << YAML::Flow << notifications.on_events;
        if (!notifications.webhook_url.empty()) {
            out << YAML::Key << "webhook_url" << YAML::Value << notifications.webhook_url;
        }
        if (!notifications.email_to.empty()) {
            out << YAML::Key << "email_to" << YAML::Value << notifications.email_to;
        }
        out << YAML::EndMap;

        // Canary rollout config
        out << YAML::Key << "canary";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << canary.enabled;
        out << YAML::Key << "node_id" << YAML::Value << canary.node_id;
        out << YAML::Key << "error_rate_threshold" << YAML::Value << canary.error_rate_threshold;
        out << YAML::Key << "min_sample_count" << YAML::Value << canary.min_sample_count;
        out << YAML::Key << "stages" << YAML::Value << YAML::BeginSeq;
        for (const auto& s : canary.stages) {
            out << YAML::BeginMap;
            out << YAML::Key << "percentage" << YAML::Value << s.percentage;
            out << YAML::Key << "observation_seconds" << YAML::Value << s.observation_seconds;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        // Anonymous hardware telemetry config
        out << YAML::Key << "telemetry";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled"               << YAML::Value << telemetry.enabled;
        out << YAML::Key << "endpoint_url"          << YAML::Value << telemetry.endpoint_url;
        out << YAML::Key << "send_interval_seconds" << YAML::Value << telemetry.send_interval_seconds;
        out << YAML::Key << "include_cpu_model"     << YAML::Value << telemetry.include_cpu_model;
        out << YAML::Key << "include_cpu_cores"     << YAML::Value << telemetry.include_cpu_cores;
        out << YAML::Key << "include_ram_mb"        << YAML::Value << telemetry.include_ram_mb;
        out << YAML::Key << "include_os"            << YAML::Value << telemetry.include_os;
        out << YAML::Key << "include_arch"          << YAML::Value << telemetry.include_arch;
        out << YAML::Key << "http_timeout_seconds"  << YAML::Value << telemetry.http_timeout_seconds;
        out << YAML::Key << "max_retries"           << YAML::Value << telemetry.max_retries;
        out << YAML::EndMap;

        out << YAML::EndMap;  // updates
        out << YAML::EndMap;  // root
        
        std::ofstream fout(yaml_path);
        fout << out.c_str();
        
        LOG_INFO("Saved updates configuration to {}", yaml_path);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save updates configuration to {}: {}", yaml_path, e.what());
    }
}

} // namespace updates
} // namespace themis

