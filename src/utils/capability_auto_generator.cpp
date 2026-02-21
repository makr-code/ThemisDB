/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            capability_auto_generator.cpp                      ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     516                                            ║
    • Open Issues:     TODOs: 3, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/capability_auto_generator.h"
#include "utils/self_awareness.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <cmath>

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
    std::shared_ptr<SelfAwareness> self_awareness
) : config_(config), topology_(topology), self_awareness_(self_awareness) {
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
    
    // TODO: Check last update time and compare with schedule interval
    // For now, just check if update is needed
    
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
    
    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, data_path, &db);
    
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
    
    std::unique_ptr<rocksdb::DB> db_ptr(db);
    
    // Iterate through database
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions()));
    
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
            
        } catch (...) {
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
    
    // Check document count change
    // TODO: Store previous document count somewhere
    
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
    // TODO: Implement YAML serialization and file writing
    // For now, just log
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
    } catch (...) {
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

} // namespace themis::util
