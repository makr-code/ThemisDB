/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            self_awareness.cpp                                 ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     583                                            ║
    • Open Issues:     TODOs: 4, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/self_awareness.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/sysinfo.h>
    #include <sys/statvfs.h>
    #include <unistd.h>
#endif

namespace themis::util {

// Load configuration from YAML
SelfAwareness::Config SelfAwareness::Config::loadFromYAML(const std::string& yaml_path) {
    Config config;
    
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        
        if (root["self_awareness"]) {
            auto sa = root["self_awareness"];
            
            config.enabled = sa["enabled"].as<bool>(true);
            config.on_audit_signing = sa["on_audit_signing"].as<bool>(true);
            config.on_schedule = sa["on_schedule"].as<bool>(false);
            
            if (sa["schedule_interval_seconds"]) {
                config.schedule_interval = std::chrono::seconds(
                    sa["schedule_interval_seconds"].as<uint64_t>(3600));
            }
            
            // Thresholds
            if (sa["thresholds"]) {
                auto thresh = sa["thresholds"];
                config.cpu_warning_threshold = thresh["cpu_warning"].as<double>(0.80);
                config.cpu_critical_threshold = thresh["cpu_critical"].as<double>(0.95);
                config.memory_warning_threshold = thresh["memory_warning"].as<double>(0.80);
                config.memory_critical_threshold = thresh["memory_critical"].as<double>(0.90);
                config.disk_warning_threshold = thresh["disk_warning"].as<double>(0.80);
                config.disk_critical_threshold = thresh["disk_critical"].as<double>(0.90);
            }
            
            // Snapshot settings
            if (sa["snapshots"]) {
                auto snaps = sa["snapshots"];
                config.max_snapshots_retained = snaps["max_retained"].as<uint32_t>(100);
                config.persist_snapshots = snaps["persist"].as<bool>(true);
                config.snapshot_directory = snaps["directory"].as<std::string>(
                    "/var/lib/themisdb/self-awareness");
            }
        }
        
    } catch (const std::exception& e) {
        // Use defaults if config fails to load
    }
    
    return config;
}

// Constructor
SelfAwareness::SelfAwareness(const Config& config)
    : config_(config) {
    
    if (config_.persist_snapshots) {
        loadSnapshots();
    }
}

// Destructor
SelfAwareness::~SelfAwareness() {
    // Persist any pending snapshots
}

// Take snapshot
SelfAwareness::Snapshot SelfAwareness::takeSnapshot(const std::string& triggered_by) {
    Snapshot snapshot;
    
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.triggered_by = triggered_by;
    
    // Collect metrics
    snapshot.health = collectHealthMetrics();
    snapshot.capabilities = collectCapabilityState();
    snapshot.performance = collectQueryPerformance();
    
    // Detect anomalies
    snapshot.anomalies = detectAnomalies(snapshot);
    
    // Self-assessment
    snapshot.overall_health_status = assessOverallHealth(snapshot);
    snapshot.confidence_score = 1.0;  // TODO: Calculate based on data quality
    
    // Store snapshot
    snapshots_.push_back(snapshot);
    
    // Persist if configured
    if (config_.persist_snapshots) {
        persistSnapshot(snapshot);
    }
    
    // Prune old snapshots
    pruneSnapshots();
    
    return snapshot;
}

// Trigger on audit signing
SelfAwareness::Snapshot SelfAwareness::onAuditSigning(const nlohmann::json& audit_entry) {
    if (!config_.enabled || !config_.on_audit_signing) {
        return Snapshot{};
    }
    
    // Create snapshot triggered by audit signing
    auto snapshot = takeSnapshot("audit_signing");
    
    // Log the self-awareness event
    // TODO: Add to audit log that self-awareness was triggered
    
    return snapshot;
}

// Collect health metrics
SelfAwareness::HealthMetrics SelfAwareness::collectHealthMetrics() const {
    HealthMetrics metrics;
    
#ifdef _WIN32
    // Windows implementation using WAPI
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    
    if (GlobalMemoryStatusEx(&mem_status)) {
        metrics.memory_total_bytes = mem_status.ullTotalPhys;
        metrics.memory_used_bytes = mem_status.ullTotalPhys - mem_status.ullAvailPhys;
        metrics.memory_available_bytes = mem_status.ullAvailPhys;
        metrics.memory_usage_percent = mem_status.dwMemoryLoad / 100.0;
    }
    
    // Disk usage (approximate path)
    ULARGE_INTEGER free_bytes, total_bytes;
    if (GetDiskFreeSpaceExA("C:\\", &free_bytes, &total_bytes, nullptr)) {
        metrics.disk_total_bytes = total_bytes.QuadPart;
        metrics.disk_available_bytes = free_bytes.QuadPart;
        metrics.disk_used_bytes = total_bytes.QuadPart - free_bytes.QuadPart;
        metrics.disk_usage_percent = 
            static_cast<double>(metrics.disk_used_bytes) / metrics.disk_total_bytes;
    }
    
    // CPU and thread info
    metrics.thread_count = std::thread::hardware_concurrency();
    metrics.cpu_usage_percent = 0.0;  // Placeholder - would need performance counters
    metrics.cpu_load_1min = 0.0;
    metrics.cpu_load_5min = 0.0;
    metrics.cpu_load_15min = 0.0;
    metrics.uptime_seconds = GetTickCount64() / 1000;  // System uptime in seconds
    metrics.open_file_descriptors = 0;  // Placeholder for Windows
    
#else
    // Linux implementation using system calls
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        metrics.memory_total_bytes = si.totalram * si.mem_unit;
        metrics.memory_used_bytes = (si.totalram - si.freeram) * si.mem_unit;
        metrics.memory_available_bytes = si.freeram * si.mem_unit;
        metrics.memory_usage_percent = 
            static_cast<double>(metrics.memory_used_bytes) / metrics.memory_total_bytes;
        
        metrics.cpu_load_1min = si.loads[0] / 65536.0;
        metrics.cpu_load_5min = si.loads[1] / 65536.0;
        metrics.cpu_load_15min = si.loads[2] / 65536.0;
        
        metrics.uptime_seconds = si.uptime;
    }
    
    // Get disk usage
    struct statvfs st;
    if (statvfs("/var/lib/themisdb", &st) == 0) {
        metrics.disk_total_bytes = st.f_blocks * st.f_frsize;
        metrics.disk_available_bytes = st.f_bavail * st.f_frsize;
        metrics.disk_used_bytes = metrics.disk_total_bytes - metrics.disk_available_bytes;
        metrics.disk_usage_percent = 
            static_cast<double>(metrics.disk_used_bytes) / metrics.disk_total_bytes;
    }
    
    // Get thread count
    metrics.thread_count = std::thread::hardware_concurrency();
    
    // CPU usage (simplified - would need more complex calculation)
    metrics.cpu_usage_percent = metrics.cpu_load_1min / std::thread::hardware_concurrency();
    
    // Get open file descriptors
    metrics.open_file_descriptors = 0;  // Placeholder
#endif
    
    return metrics;
}

// Collect capability state
SelfAwareness::CapabilityState SelfAwareness::collectCapabilityState() const {
    CapabilityState state;
    
    // TODO: Query ShardTopology for actual shard information
    // For now, return placeholder data
    
    state.total_shards = 0;
    state.active_shards = 0;
    state.inactive_shards = 0;
    
    state.total_capabilities_configured = 0;
    state.auto_generated_capabilities = 0;
    state.manually_configured_capabilities = 0;
    
    state.total_documents = 0;
    state.total_size_bytes = 0;
    
    state.total_unique_keywords = 0;
    state.total_unique_domains = 0;
    state.total_unique_organizations = 0;
    state.total_unique_regions = 0;
    
    return state;
}

// Collect query performance
SelfAwareness::QueryPerformance SelfAwareness::collectQueryPerformance() const {
    QueryPerformance perf;
    
    // TODO: Query AdaptiveShardRouter for statistics
    // For now, return placeholder data
    
    perf.total_queries = 0;
    perf.adaptive_routed_queries = 0;
    perf.scatter_gather_queries = 0;
    perf.adaptive_routing_ratio = 0.0;
    
    perf.avg_query_time_ms = 0.0;
    perf.p50_query_time_ms = 0.0;
    perf.p95_query_time_ms = 0.0;
    perf.p99_query_time_ms = 0.0;
    
    perf.avg_shards_queried = 0.0;
    perf.network_traffic_saved_percent = 0.0;
    perf.iterations_saved = 0;
    
    return perf;
}

// Detect anomalies
std::vector<std::string> SelfAwareness::detectAnomalies(const Snapshot& snapshot) const {
    std::vector<std::string> anomalies;
    
    // CPU anomalies
    if (snapshot.health.cpu_usage_percent >= config_.cpu_critical_threshold) {
        anomalies.push_back("CRITICAL: CPU usage at " + 
            std::to_string(static_cast<int>(snapshot.health.cpu_usage_percent * 100)) + "%");
    } else if (snapshot.health.cpu_usage_percent >= config_.cpu_warning_threshold) {
        anomalies.push_back("WARNING: CPU usage at " + 
            std::to_string(static_cast<int>(snapshot.health.cpu_usage_percent * 100)) + "%");
    }
    
    // Memory anomalies
    if (snapshot.health.memory_usage_percent >= config_.memory_critical_threshold) {
        anomalies.push_back("CRITICAL: Memory usage at " + 
            std::to_string(static_cast<int>(snapshot.health.memory_usage_percent * 100)) + "%");
    } else if (snapshot.health.memory_usage_percent >= config_.memory_warning_threshold) {
        anomalies.push_back("WARNING: Memory usage at " + 
            std::to_string(static_cast<int>(snapshot.health.memory_usage_percent * 100)) + "%");
    }
    
    // Disk anomalies
    if (snapshot.health.disk_usage_percent >= config_.disk_critical_threshold) {
        anomalies.push_back("CRITICAL: Disk usage at " + 
            std::to_string(static_cast<int>(snapshot.health.disk_usage_percent * 100)) + "%");
    } else if (snapshot.health.disk_usage_percent >= config_.disk_warning_threshold) {
        anomalies.push_back("WARNING: Disk usage at " + 
            std::to_string(static_cast<int>(snapshot.health.disk_usage_percent * 100)) + "%");
    }
    
    // Capability anomalies
    if (snapshot.capabilities.total_shards > 0 && 
        snapshot.capabilities.active_shards == 0) {
        anomalies.push_back("CRITICAL: No active shards");
    }
    
    // Performance anomalies
    if (snapshot.performance.total_queries > 100 &&
        snapshot.performance.avg_query_time_ms > 5000) {
        anomalies.push_back("WARNING: High average query time: " + 
            std::to_string(static_cast<int>(snapshot.performance.avg_query_time_ms)) + "ms");
    }
    
    return anomalies;
}

// Assess overall health
std::string SelfAwareness::assessOverallHealth(const Snapshot& snapshot) const {
    // Critical if any critical anomalies
    for (const auto& anomaly : snapshot.anomalies) {
        if (anomaly.find("CRITICAL") != std::string::npos) {
            return "critical";
        }
    }
    
    // Degraded if any warnings
    if (!snapshot.anomalies.empty()) {
        return "degraded";
    }
    
    // Good if metrics are reasonable
    if (snapshot.health.cpu_usage_percent < 0.60 &&
        snapshot.health.memory_usage_percent < 0.70 &&
        snapshot.health.disk_usage_percent < 0.70) {
        return "excellent";
    }
    
    return "good";
}

// Compare with previous
nlohmann::json SelfAwareness::compareWithPrevious() const {
    nlohmann::json comparison;
    
    if (snapshots_.size() < 2) {
        comparison["status"] = "insufficient_data";
        comparison["message"] = "Need at least 2 snapshots for comparison";
        return comparison;
    }
    
    const auto& current = snapshots_.back();
    const auto& previous = snapshots_[snapshots_.size() - 2];
    
    // Compare health metrics
    comparison["health"]["cpu_usage_change"] = 
        current.health.cpu_usage_percent - previous.health.cpu_usage_percent;
    comparison["health"]["memory_usage_change"] = 
        current.health.memory_usage_percent - previous.health.memory_usage_percent;
    comparison["health"]["disk_usage_change"] = 
        current.health.disk_usage_percent - previous.health.disk_usage_percent;
    
    // Compare capabilities
    comparison["capabilities"]["shard_count_change"] = 
        static_cast<int>(current.capabilities.total_shards) - 
        static_cast<int>(previous.capabilities.total_shards);
    comparison["capabilities"]["document_count_change"] = 
        static_cast<int64_t>(current.capabilities.total_documents) - 
        static_cast<int64_t>(previous.capabilities.total_documents);
    
    // Compare performance
    comparison["performance"]["query_time_change_ms"] = 
        current.performance.avg_query_time_ms - previous.performance.avg_query_time_ms;
    comparison["performance"]["total_queries_change"] = 
        static_cast<int64_t>(current.performance.total_queries) - 
        static_cast<int64_t>(previous.performance.total_queries);
    
    // Overall assessment
    comparison["health_status_change"] = {
        {"previous", previous.overall_health_status},
        {"current", current.overall_health_status}
    };
    
    return comparison;
}

// Convert snapshot to JSON
nlohmann::json SelfAwareness::Snapshot::toJSON() const {
    nlohmann::json j;
    
    // Timestamp
    auto time_t_timestamp = std::chrono::system_clock::to_time_t(timestamp);
    j["timestamp"] = std::ctime(&time_t_timestamp);
    j["triggered_by"] = triggered_by;
    
    // Health metrics
    j["health"] = {
        {"cpu", {
            {"usage_percent", health.cpu_usage_percent},
            {"load_1min", health.cpu_load_1min},
            {"load_5min", health.cpu_load_5min},
            {"load_15min", health.cpu_load_15min}
        }},
        {"memory", {
            {"total_bytes", health.memory_total_bytes},
            {"used_bytes", health.memory_used_bytes},
            {"available_bytes", health.memory_available_bytes},
            {"usage_percent", health.memory_usage_percent}
        }},
        {"disk", {
            {"total_bytes", health.disk_total_bytes},
            {"used_bytes", health.disk_used_bytes},
            {"available_bytes", health.disk_available_bytes},
            {"usage_percent", health.disk_usage_percent}
        }},
        {"process", {
            {"thread_count", health.thread_count},
            {"open_file_descriptors", health.open_file_descriptors},
            {"uptime_seconds", health.uptime_seconds}
        }}
    };
    
    // Capabilities
    j["capabilities"] = {
        {"shards", {
            {"total", capabilities.total_shards},
            {"active", capabilities.active_shards},
            {"inactive", capabilities.inactive_shards}
        }},
        {"configured_capabilities", {
            {"total", capabilities.total_capabilities_configured},
            {"auto_generated", capabilities.auto_generated_capabilities},
            {"manual", capabilities.manually_configured_capabilities}
        }},
        {"data", {
            {"total_documents", capabilities.total_documents},
            {"total_size_bytes", capabilities.total_size_bytes}
        }},
        {"metadata", {
            {"unique_keywords", capabilities.total_unique_keywords},
            {"unique_domains", capabilities.total_unique_domains},
            {"unique_organizations", capabilities.total_unique_organizations},
            {"unique_regions", capabilities.total_unique_regions}
        }}
    };
    
    // Performance
    j["performance"] = {
        {"queries", {
            {"total", performance.total_queries},
            {"adaptive_routed", performance.adaptive_routed_queries},
            {"scatter_gather", performance.scatter_gather_queries},
            {"adaptive_ratio", performance.adaptive_routing_ratio}
        }},
        {"latency", {
            {"avg_ms", performance.avg_query_time_ms},
            {"p50_ms", performance.p50_query_time_ms},
            {"p95_ms", performance.p95_query_time_ms},
            {"p99_ms", performance.p99_query_time_ms}
        }},
        {"efficiency", {
            {"avg_shards_queried", performance.avg_shards_queried},
            {"network_traffic_saved_percent", performance.network_traffic_saved_percent},
            {"iterations_saved", performance.iterations_saved}
        }}
    };
    
    // Assessment
    j["assessment"] = {
        {"overall_health_status", overall_health_status},
        {"confidence_score", confidence_score},
        {"anomalies", anomalies}
    };
    
    return j;
}

// Persist snapshot
void SelfAwareness::persistSnapshot(const Snapshot& snapshot) {
    try {
        std::filesystem::create_directories(config_.snapshot_directory);
        
        // Filename: snapshot_<unix_ms>.json
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            snapshot.timestamp.time_since_epoch()).count();
        std::string filename = config_.snapshot_directory + "/snapshot_" +
                               std::to_string(ms) + ".json";
        
        std::ofstream ofs(filename);
        if (ofs) {
            ofs << snapshot.toJSON().dump(2) << "\n";
        }
    } catch (const std::exception&) {
        // Snapshot persistence is best-effort; do not propagate errors
    }
}

// Load snapshots
void SelfAwareness::loadSnapshots() {
    try {
        if (!std::filesystem::exists(config_.snapshot_directory)) {
            return;
        }

        // Collect snapshot files sorted by name (which encodes timestamp)
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(config_.snapshot_directory)) {
            if (entry.is_regular_file() &&
                entry.path().filename().string().rfind("snapshot_", 0) == 0) {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());

        // Load the most recent max_snapshots_retained files
        if (files.size() > config_.max_snapshots_retained) {
            files.erase(files.begin(),
                        files.begin() + static_cast<std::ptrdiff_t>(
                            files.size() - config_.max_snapshots_retained));
        }

        for (const auto& path : files) {
            try {
                std::ifstream ifs(path);
                if (!ifs) continue;
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
                auto j = nlohmann::json::parse(content);

                Snapshot s;
                // Restore timestamp from JSON (stored as ctime string)
                if (j.contains("timestamp")) {
                    // best-effort: timestamp_epoch_ms is stored in filename
                    auto fname = path.stem().string(); // "snapshot_<ms>"
                    auto sep = fname.rfind('_');
                    if (sep != std::string::npos) {
                        try {
                            auto epoch_ms = std::stoll(fname.substr(sep + 1));
                            s.timestamp = std::chrono::system_clock::time_point(
                                std::chrono::milliseconds(epoch_ms));
                        } catch (...) {}
                    }
                }
                s.triggered_by = j.value("triggered_by", "loaded");
                snapshots_.push_back(std::move(s));
            } catch (const std::exception&) {
                // Skip malformed files
            }
        }
    } catch (const std::exception&) {
        // Snapshot loading is best-effort
    }
}

// Prune snapshots
void SelfAwareness::pruneSnapshots() {
    while (snapshots_.size() > config_.max_snapshots_retained) {
        snapshots_.erase(snapshots_.begin());
    }
}

// Get statistics
nlohmann::json SelfAwareness::getStatistics() const {
    return {
        {"total_snapshots", snapshots_.size()},
        {"oldest_snapshot", snapshots_.empty() ? "none" : "timestamp"},
        {"latest_snapshot", snapshots_.empty() ? "none" : "timestamp"},
        {"enabled", config_.enabled},
        {"on_audit_signing", config_.on_audit_signing}
    };
}

} // namespace themis::util
