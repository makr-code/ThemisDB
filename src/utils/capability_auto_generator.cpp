/**
 * @file capability_auto_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.47
 * @date 2026-06-02 11:49:05
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 79/100
 * @note Lines: 669
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=1
 * @note PR History (last 5): #4275 feat(utils): CapabilityAuto... (2026-03-15) | #3632 fix(build): register 40+ mi... (2026-03-12)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/capability_auto_generator.h"
#include <stdexcept>
#include "utils/self_awareness.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <filesystem>

namespace themis::util {

// Load configuration from YAML
CapabilityAutoGenerator::Config CapabilityAutoGenerator::Config::loadFromYAML(const std::string& yaml_path) {
    Config config;
    
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        
        config.enabled = root["enabled"].as<bool>(false);
        
        // Load schedules
        if (root["schedules"]) {
            for (const auto& schedule_node : root["schedules"]) {
                std::string type = schedule_node.first.as<std::string>();
                auto sched = schedule_node.second;
                
                UpdateSchedule schedule;
                schedule.shard_type = type;
                schedule.enabled = sched["enabled"].as<bool>(true);
                schedule.interval = std::chrono::seconds(sched["interval_seconds"].as<uint64_t>());
                schedule.min_document_change = sched["min_document_change"].as<uint64_t>(1000);
                schedule.min_keyword_change = sched["min_keyword_change"].as<double>(0.05);
                schedule.require_review = sched["require_review"].as<bool>(false);
                schedule.auto_approve_threshold = sched["auto_approve_threshold"].as<uint32_t>(10);
                
                config.schedules[type] = schedule;
            }
        }
        
        // RocksDB analysis settings
        if (root["rocksdb_analysis"]) {
            auto analysis = root["rocksdb_analysis"];
            config.sampling_rate = analysis["sampling_rate"].as<uint32_t>(100);
            config.max_keywords = analysis["max_keywords"].as<uint32_t>(100);
        }
        
        // Audit settings
        if (root["audit"]) {
            auto audit = root["audit"];
            config.audit_logging = audit["enabled"].as<bool>(true);
            config.audit_log_path = audit["log_path"].as<std::string>("/var/log/themisdb/capability-generation.log");
        }
        
        // Security settings
        if (root["security"]) {
            auto security = root["security"];
            config.require_signature = security["require_signature"].as<bool>(true);
            config.signing_key_path = security["signing_key_path"].as<std::string>("");
        }
        
        // Output settings
        if (root["output"]) {
            auto output = root["output"];
            config.output_directory = output["directory"].as<std::string>("config/capabilities");
            config.create_backups = output["create_backups"].as<bool>(true);
            config.git_commit = output["git_commit"].as<bool>(false);
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load capability auto-gen config: " + std::string(e.what()));
    }
    
    return config;
}

// Constructor
CapabilityAutoGenerator::CapabilityAutoGenerator(
    const Config& config,
    std::shared_ptr<sharding::ShardTopology> topology,
    std::shared_ptr<SelfAwareness> self_awareness,
    std::shared_ptr<RocksDBWrapper> state_db
) : config_(config), topology_(topology), self_awareness_(self_awareness),
    state_db_(state_db) {
    if (state_db_) {
        loadPersistedState();
    }
}

// Destructor
CapabilityAutoGenerator::~CapabilityAutoGenerator() {
    stop();
}

// Start background thread
void CapabilityAutoGenerator::start() {
    if (running_) {
        return;  // Already running
    }
    
    if (!config_.enabled) {
        return;  // Disabled in config
    }
    
    running_ = true;
    stop_requested_ = false;
    
    worker_thread_ = std::make_unique<std::thread>(&CapabilityAutoGenerator::workerThread, this);
}

// Stop background thread
void CapabilityAutoGenerator::stop() {
    if (!running_) {
        return;
    }
    
    stop_requested_ = true;
    
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    
    running_ = false;
}

// Worker thread main loop
void CapabilityAutoGenerator::workerThread() {
    while (!stop_requested_) {
        try {
            // Get all shards
            auto shards = topology_->getAllShards();
            
            for (const auto& shard : shards) {
                if (stop_requested_) break;
                
                // Process this shard
                processShard(shard);
                
                // Small delay between shards to avoid overload
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Sleep until next cycle (use minimum schedule interval)
            std::chrono::seconds min_interval(3600);  // Default: 1 hour
            for (const auto& [type, schedule] : config_.schedules) {
                if (schedule.enabled && schedule.interval < min_interval) {
                    min_interval = schedule.interval;
                }
            }
            
            std::this_thread::sleep_for(min_interval);
            
        } catch (const std::exception& e) {
            // Log error and continue
            auditLog("system", {
                {"error", e.what()},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
            });
            
            // Sleep before retry
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }
}

// Process single shard
void CapabilityAutoGenerator::processShard(const sharding::ShardInfo& shard) {
    // Check if it's time to update this shard
    auto schedule = getScheduleForShard(shard);
    
    if (!schedule.enabled) {
        return;  // Updates disabled for this shard type
    }
    
    // Check last update time and compare with schedule interval
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = last_run_timestamps_.find(shard.shard_id);
        if (it != last_run_timestamps_.end()) {
            int64_t now_s = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int64_t elapsed = now_s - it->second;
            if (elapsed < static_cast<int64_t>(schedule.interval.count())) {
                return;  // Within the schedule interval — skip regeneration
            }
        }
    }
    
    try {
        // Analyze shard data
        std::string data_path = "/var/lib/themisdb/data/" + shard.shard_id;
        auto result = analyzeShardData(shard.shard_id, data_path);
        
        // Check if update is needed
        if (!shouldUpdate(shard, result)) {
            return;  // No significant changes
        }
        
        // Generate capability
        const sharding::DomainCapability* prev = &shard.domain_capability;
        auto new_capability = generateFromAnalysis(result, prev);
        
        // Create audit trail
        auto audit_info = generateAuditTrail(shard.shard_id, prev, result);
        
        // Save capability
        if (saveCapability(shard.shard_id, new_capability, audit_info)) {
            successful_generations_++;
            // Persist state: timestamp and document count
            int64_t now_s = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            persistState(shard.shard_id, now_s, result.document_count);
        } else {
            failed_generations_++;
        }
        
        total_generations_++;
        
    } catch (const std::exception& e) {
        failed_generations_++;
        auditLog(shard.shard_id, {
            {"error", e.what()},
            {"status", "failed"}
        });
    }
}

// Analyze RocksDB data
CapabilityAutoGenerator::AnalysisResult CapabilityAutoGenerator::analyzeShardData(
    const std::string& shard_id,
    const std::string& data_path
) {
    AnalysisResult result;
    result.shard_id = shard_id;
    
    // Open RocksDB read-only
    rocksdb::Options options;
    options.create_if_missing = false;
     
    rocksdb::DB* db_instance = nullptr;
    rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, data_path, &db_instance);

    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }

    // Defer database cleanup via RAII scope.
    auto db_guard = [db_instance]() noexcept { delete db_instance; };
    (void)db_guard;  // unused-variable warning suppression for defer semantics

    // Iterate through database
    std::unique_ptr<rocksdb::Iterator> it(db_instance->NewIterator(rocksdb::ReadOptions()));
    
    uint64_t doc_count = 0;
    uint64_t total_size = 0;
    std::unordered_map<std::string, uint32_t> keyword_freq;
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        doc_count++;
        
        auto value = it->value();
        total_size += value.size();
        
        // Sample every Nth document
        if (doc_count % config_.sampling_rate != 0) {
            continue;
        }
        
        // Parse document (assume JSON)
        try {
            auto doc = nlohmann::json::parse(value.ToString());
            
            // Extract metadata
            if (doc.contains("type")) {
                std::string type = doc["type"];
                result.data_types.push_back(type);
            }
            
            if (doc.contains("organization")) {
                result.organizations.push_back(doc["organization"]);
            }
            
            if (doc.contains("region")) {
                result.regions.push_back(doc["region"]);
            }
            
            // Extract keywords from text fields
            std::vector<std::string> text_fields = {"title", "description", "content"};
            for (const auto& field : text_fields) {
                if (doc.contains(field)) {
                    std::string text = doc[field];
                    // Simple tokenization
                    std::istringstream iss(text);
                    std::string word;
                    while (iss >> word) {
                        // Convert to lowercase
                        std::transform(word.begin(), word.end(), word.begin(), 
                                     [](unsigned char c) { return std::tolower(c); });
                        if (word.length() >= 3) {
                            keyword_freq[word]++;
                        }
                    }
                }
            }
            
        } catch (const nlohmann::json::exception &) {
            // Skip documents that can't be parsed
        } catch (const std::exception &) {
            // Skip documents that can't be parsed
        } catch (const std::string &) {
            // Skip documents that can't be parsed
        } catch (const char *) {
            // Skip documents that can't be parsed
        }
    }
    
    // Extract top keywords
    std::vector<std::pair<std::string, uint32_t>> sorted_keywords(keyword_freq.begin(), keyword_freq.end());
    std::sort(sorted_keywords.begin(), sorted_keywords.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(sorted_keywords.size(), (size_t)config_.max_keywords); ++i) {
        result.keywords.push_back(sorted_keywords[i].first);
    }
    
    // Deduplicate metadata
    auto dedupe = [](std::vector<std::string>& vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };
    
    dedupe(result.data_types);
    dedupe(result.organizations);
    dedupe(result.regions);
     
    result.document_count = doc_count;
    result.total_size_bytes = total_size;
    result.last_update_time = std::chrono::system_clock::now();
     
    delete db_instance;
    return result;
}

// Generate capability from analysis
sharding::DomainCapability CapabilityAutoGenerator::generateFromAnalysis(
    const AnalysisResult& result,
    const sharding::DomainCapability* previous_capability
) {
    sharding::DomainCapability capability;
    
    // Copy basic info
    capability.domains = result.domains;
    capability.organizations = result.organizations;
    capability.regions = result.regions;
    capability.data_types = result.data_types;
    capability.keywords = result.keywords;
    
    // Preserve manual edits if previous capability exists
    if (previous_capability) {
        // Keep manually added domains/organizations that weren't auto-detected
        for (const auto& domain : previous_capability->domains) {
            if (std::find(capability.domains.begin(), capability.domains.end(), domain) == capability.domains.end()) {
                capability.domains.push_back(domain);
            }
        }
    }
    
    return capability;
}

// Check if update is needed
bool CapabilityAutoGenerator::shouldUpdate(const sharding::ShardInfo& shard, const AnalysisResult& current) {
    // Always update if no previous capability
    if (shard.domain_capability.isEmpty()) {
        return true;
    }
    
    // Check document count change against persisted previous count
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = last_document_counts_.find(shard.shard_id);
        if (it != last_document_counts_.end()) {
            uint64_t prev_count = it->second;
            uint64_t curr_count = current.document_count;
            uint64_t delta = (curr_count > prev_count) ? (curr_count - prev_count) : 0;
            auto schedule = getScheduleForShard(shard);
            if (delta >= schedule.min_document_change) {
                return true;  // Enough new documents to warrant regeneration
            }
        }
    }
    
    // Check keyword change
    auto prev_keywords = shard.domain_capability.keywords;
    auto curr_keywords = current.keywords;
    
    std::set<std::string> prev_set(prev_keywords.begin(), prev_keywords.end());
    std::set<std::string> curr_set(curr_keywords.begin(), curr_keywords.end());
    
    // Calculate Jaccard similarity
    std::set<std::string> intersection;
    std::set_intersection(prev_set.begin(), prev_set.end(),
                         curr_set.begin(), curr_set.end(),
                         std::inserter(intersection, intersection.begin()));
    
    std::set<std::string> union_set;
    std::set_union(prev_set.begin(), prev_set.end(),
                  curr_set.begin(), curr_set.end(),
                  std::inserter(union_set, union_set.begin()));
    
    double similarity = union_set.empty() ? 1.0 : 
        static_cast<double>(intersection.size()) / union_set.size();
    double change = 1.0 - similarity;
    
    auto schedule = getScheduleForShard(shard);
    return change >= schedule.min_keyword_change;
}

// Get schedule for shard
CapabilityAutoGenerator::UpdateSchedule CapabilityAutoGenerator::getScheduleForShard(const sharding::ShardInfo& shard) const {
    std::string shard_type = determineShardType(shard);
    
    auto it = config_.schedules.find(shard_type);
    if (it != config_.schedules.end()) {
        return it->second;
    }
    
    // Return default
    return config_.schedules.at("normal");
}

// Determine shard type
std::string CapabilityAutoGenerator::determineShardType(const sharding::ShardInfo& shard) const {
    // Simple heuristic based on shard ID or metadata
    // In production, would check metadata fields
    
    if (shard.shard_id.find("critical") != std::string::npos) {
        return "critical";
    }
    if (shard.shard_id.find("realtime") != std::string::npos) {
        return "real-time";
    }
    
    return "normal";
}

// Save capability
bool CapabilityAutoGenerator::saveCapability(
    const std::string& shard_id,
    const sharding::DomainCapability& capability,
    const nlohmann::json& audit_info
) {
    // Serialize DomainCapability to YAML using yaml-cpp
    // and atomically write to output_directory/<shard_id>.yaml
    try {
        namespace fs = std::filesystem;

        // Ensure output directory exists
        fs::path out_dir(config_.output_directory);
        fs::create_directories(out_dir);

        fs::path out_path  = out_dir / (shard_id + ".yaml");
        fs::path tmp_path  = out_dir / (shard_id + ".yaml.tmp");

        // Build YAML document
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;

        emitter << YAML::Key << "shard_id" << YAML::Value << shard_id;

        auto emitSeq = [&](const std::string& key, const std::vector<std::string>& vec) {
            emitter << YAML::Key << key;
            if (vec.empty()) {
                emitter << YAML::Value << YAML::BeginSeq << YAML::EndSeq;
            } else {
                emitter << YAML::Value << YAML::BeginSeq;
                for (const auto& s : vec) emitter << s;
                emitter << YAML::EndSeq;
            }
        };

        emitSeq("domains",       capability.domains);
        emitSeq("organizations", capability.organizations);
        emitSeq("regions",       capability.regions);
        emitSeq("data_types",    capability.data_types);
        emitSeq("keywords",      capability.keywords);

        if (!capability.metadata.empty()) {
            emitter << YAML::Key << "metadata" << YAML::Value << YAML::BeginMap;
            for (const auto& [k, v] : capability.metadata) {
                emitter << YAML::Key << k << YAML::Value << v;
            }
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndMap;

        // Atomic write: write to .tmp then rename
        {
            std::ofstream ofs(tmp_path, std::ios::trunc);
            if (!ofs) {
                auditLog(shard_id, {{"error", "Failed to open tmp file for writing"},
                                    {"path", tmp_path.string()}});
                return false;
            }
            ofs << emitter.c_str();
        }
        fs::rename(tmp_path, out_path);

    } catch (const std::exception& e) {
        auditLog(shard_id, {{"error", e.what()}, {"status", "save_failed"}});
        return false;
    }

    auditLog(shard_id, audit_info);
    return true;
}

// Audit logging
void CapabilityAutoGenerator::auditLog(const std::string& shard_id, const nlohmann::json& entry) {
    if (!config_.audit_logging) {
        return;
    }
    
    try {
        std::ofstream log(config_.audit_log_path, std::ios::app);
        nlohmann::json log_entry = entry;
        log_entry["shard_id"] = shard_id;
        log_entry["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        log << log_entry.dump() << "\n";
        
        // TRIGGER SELF-AWARENESS: When audit log is signed/written
        if (self_awareness_ && config_.require_signature) {
            // Trigger self-awareness snapshot on audit signing
            self_awareness_->onAuditSigning(log_entry);
        }
    } catch (const std::exception &) {
        // Ignore logging errors
    } catch (const std::string &) {
        // Ignore logging errors
    } catch (const char *) {
        // Ignore logging errors
    }
}

// Generate audit trail
nlohmann::json CapabilityAutoGenerator::generateAuditTrail(
    const std::string& shard_id,
    const sharding::DomainCapability* previous,
    const AnalysisResult& current
) {
    nlohmann::json audit;
    
    audit["shard_id"] = shard_id;
    audit["generation_method"] = "auto-generated";
    audit["generated_at"] = std::chrono::system_clock::now().time_since_epoch().count();
    audit["generated_by"] = "system";
    
    // Change summary
    if (previous) {
        size_t added = 0, removed = 0;
        std::set<std::string> prev_set(previous->keywords.begin(), previous->keywords.end());
        std::set<std::string> curr_set(current.keywords.begin(), current.keywords.end());
        
        for (const auto& kw : curr_set) {
            if (prev_set.find(kw) == prev_set.end()) added++;
        }
        for (const auto& kw : prev_set) {
            if (curr_set.find(kw) == curr_set.end()) removed++;
        }
        
        audit["change_summary"] = std::to_string(added) + " keywords added, " + 
                                 std::to_string(removed) + " removed";
    } else {
        audit["change_summary"] = "Initial generation";
    }
    
    return audit;
}

// Get statistics
nlohmann::json CapabilityAutoGenerator::getStatistics() const {
    return {
        {"total_generations", total_generations_.load()},
        {"successful_generations", successful_generations_.load()},
        {"failed_generations", failed_generations_.load()},
        {"auto_approved", auto_approved_.load()},
        {"manual_review_required", manual_review_required_.load()},
        {"running", running_.load()}
    };
}

// Load persisted schedule/count state from state_db_ into in-memory maps
void CapabilityAutoGenerator::loadPersistedState() {
    if (!state_db_) return;

    // Iterate all keys with prefix "utils_capgen_state:" to load per-shard state
    static constexpr std::string_view STATE_KEY_PREFIX = "utils_capgen_state:";

    std::string start_key(STATE_KEY_PREFIX);
    std::string end_key = start_key;
    end_key.back()++;  // e.g. "utils_capgen_state:" -> "utils_capgen_state;"

    std::vector<std::pair<std::string, std::string>> corrupt_entries;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_db_->iterateRange(start_key, end_key,
            [&](std::string_view key, std::string_view value) -> bool {
                if (key.size() <= STATE_KEY_PREFIX.size()) return true;
                std::string shard_id(key.substr(STATE_KEY_PREFIX.size()));

                try {
                    auto j = nlohmann::json::parse(value);
                    if (j.contains("last_run_timestamp")) {
                        last_run_timestamps_[shard_id] = j["last_run_timestamp"].get<int64_t>();
                    }
                    if (j.contains("last_document_count")) {
                        last_document_counts_[shard_id] = j["last_document_count"].get<uint64_t>();
                    }
                } catch (const std::exception& e) {
                    // Collect corrupted entries to log after releasing the lock
                    corrupt_entries.emplace_back(shard_id, e.what());
                }
                return true;  // continue iteration
            });
    }

    // Log corrupted entries outside the lock to avoid I/O under mutex
    for (const auto& [shard_id, err] : corrupt_entries) {
        auditLog(shard_id, {{"warning", "Skipping corrupted state entry"},
                            {"error", err}});
    }
}

// Persist the last-run timestamp and document count for a shard to state_db_
void CapabilityAutoGenerator::persistState(
    const std::string& shard_id,
    int64_t timestamp,
    uint64_t doc_count
) {
    // Update in-memory maps
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_run_timestamps_[shard_id]   = timestamp;
        last_document_counts_[shard_id]  = doc_count;
    }

    if (!state_db_) return;

    std::string key = "utils_capgen_state:" + shard_id;
    nlohmann::json j = {
        {"last_run_timestamp",   timestamp},
        {"last_document_count",  doc_count}
    };
    state_db_->put(key, j.dump());
}

} // namespace themis::util
