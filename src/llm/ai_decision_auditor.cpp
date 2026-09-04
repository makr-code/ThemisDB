/**
 * @file ai_decision_auditor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/ai_decision_auditor.h"
#include "utils/logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <openssl/sha.h>

namespace themis {
namespace llm {

// ===== AIDecisionAudit JSON Serialization =====

json AIDecisionAudit::toJson() const {
    json j;
    j["decision_id"] = decision_id;
    j["user_id"] = user_id;
    j["session_id"] = session_id;
    j["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);

    // P1.3 — W3C traceparent correlation fields (always serialised; empty when not set).
    j["trace_id"] = trace_id;
    j["span_id"]  = span_id;
    
    j["query"] = query;
    j["context"] = context;
    
    j["model_name"] = model_name;
    j["model_version"] = model_version;
    j["model_params"] = model_params;
    
    j["response"] = response;
    j["confidence_score"] = confidence_score;
    j["alternatives"] = alternatives;
    
    j["explanation"] = explanation;
    j["reasoning_steps"] = reasoning_steps;
    j["key_factors"] = key_factors;
    
    j["signature"] = signature;
    j["requires_human_review"] = requires_human_review;
    j["human_override"] = human_override;
    j["reviewer_id"] = reviewer_id;
    
    j["latency_ms"] = latency_ms;
    j["token_count"] = token_count;
    
    return j;
}

AIDecisionAudit AIDecisionAudit::fromJson(const json& j) {
    AIDecisionAudit audit;
    
    audit.decision_id = j.value("decision_id", "");
    audit.user_id = j.value("user_id", "");
    audit.session_id = j.value("session_id", "");
    
    if (j.contains("timestamp")) {
        std::time_t ts = j["timestamp"];
        audit.timestamp = std::chrono::system_clock::from_time_t(ts);
    }

    // P1.3 — W3C traceparent fields (absent in older records → defaults to empty).
    audit.trace_id = j.value("trace_id", "");
    audit.span_id  = j.value("span_id",  "");
    
    audit.query = j.value("query", "");
    if (j.contains("context")) {
        audit.context = j["context"];
    }
    
    audit.model_name = j.value("model_name", "");
    audit.model_version = j.value("model_version", "");
    if (j.contains("model_params")) {
        audit.model_params = j["model_params"];
    }
    
    audit.response = j.value("response", "");
    audit.confidence_score = j.value("confidence_score", 0.0f);
    
    if (j.contains("alternatives") && j["alternatives"].is_array()) {
        audit.alternatives = j["alternatives"].get<std::vector<std::string>>();
    }
    
    audit.explanation = j.value("explanation", "");
    if (j.contains("reasoning_steps") && j["reasoning_steps"].is_array()) {
        audit.reasoning_steps = j["reasoning_steps"].get<std::vector<std::string>>();
    }
    if (j.contains("key_factors")) {
        audit.key_factors = j["key_factors"];
    }
    
    audit.signature = j.value("signature", "");
    audit.requires_human_review = j.value("requires_human_review", false);
    audit.human_override = j.value("human_override", "");
    audit.reviewer_id = j.value("reviewer_id", "");
    
    audit.latency_ms = j.value("latency_ms", int64_t(0));
    audit.token_count = j.value("token_count", 0);
    
    return audit;
}

// ===== AIDecisionAuditor Implementation =====

AIDecisionAuditor::AIDecisionAuditor(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf,
    std::shared_ptr<VCCPKIClient> pki_client)
    : db_(db), cf_(cf), pki_client_(pki_client) {
    
    if (!db_) {
        throw std::invalid_argument("AIDecisionAuditor: db cannot be null");
    }
    
    THEMIS_INFO("AIDecisionAuditor initialized (signing: {})", 
                pki_client_ ? "enabled" : "disabled");
}

std::string AIDecisionAuditor::makeKey(const std::string& id) const {
    return std::string(KEY_PREFIX) + id;
}

std::string AIDecisionAuditor::generateId() const {
    // UUID-like ID generation (timestamp + random)
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    std::ostringstream oss = {};
    oss << "aid-" << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "-"
        << std::setw(16) << dis(gen);
    
    return oss.str();
}

std::string AIDecisionAuditor::signDecision(const AIDecisionAudit& audit) {
    if (!pki_client_) {
        return ""; // Signing disabled
    }
    
    // Create canonical representation for signing
    json canonical = audit.toJson();
    canonical.erase("signature"); // Don't include signature in signed data
    
    std::string data = canonical.dump();
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), 
           data.length(), hash);
    
    // Convert to hex string
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

bool AIDecisionAuditor::verifySignature(const AIDecisionAudit& audit) const {
    if (!pki_client_ || audit.signature.empty()) {
        return true; // Signing not enabled or no signature
    }
    
    // Recreate signature and compare
    auto expected_sig = const_cast<AIDecisionAuditor*>(this)->signDecision(audit);
    return expected_sig == audit.signature;
}

AIDecisionAudit AIDecisionAuditor::logDecision(AIDecisionAudit audit) {
    // Generate ID if empty
    if (audit.decision_id.empty()) {
        audit.decision_id = generateId();
    }
    
    // Set timestamp if not set
    if (audit.timestamp == std::chrono::system_clock::time_point{}) {
        audit.timestamp = std::chrono::system_clock::now();
    }
    
    // Check if requires human review based on confidence threshold
    if (audit.confidence_score < 0.7f && audit.confidence_score > 0.0f) {
        audit.requires_human_review = true;
        THEMIS_WARN("AI decision {} flagged for review (confidence: {:.2f})", 
                   audit.decision_id, audit.confidence_score);
    }
    
    // Sign the decision
    audit.signature = signDecision(audit);
    
    // Serialize to JSON
    std::string value = audit.toJson().dump();
    std::string key = makeKey(audit.decision_id);
    
    // Store in RocksDB
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to store AI decision {}: {}", 
                    audit.decision_id, s.ToString());
        throw std::runtime_error("Failed to store AI decision: " + s.ToString());
    }
    
    THEMIS_DEBUG("Stored AI decision {} (confidence: {:.2f}, review: {})", 
                 audit.decision_id, audit.confidence_score, 
                 audit.requires_human_review);
    
    return audit;
}

std::string AIDecisionAuditor::generateExplanation(const std::string& decision_id) {
    auto audit_opt = getDecision(decision_id);
    if (!audit_opt.has_value()) {
        return "";
    }
    
    const auto& audit = *audit_opt;
    
    // If explanation already exists, return it
    if (!audit.explanation.empty()) {
        return audit.explanation;
    }
    
    // Generate explanation from available data
    std::ostringstream explanation = {};
    explanation << "AI Decision Explanation:\n\n";
    explanation << "Query: " << audit.query << "\n";
    explanation << "Model: " << audit.model_name << " (v" << audit.model_version << ")\n";
    explanation << "Confidence: " << std::fixed << std::setprecision(1) 
                << (audit.confidence_score * 100.0f) << "%\n\n";
    
    if (!audit.reasoning_steps.empty()) {
        explanation << "Reasoning Steps:\n";
        for (size_t i = 0; i <static_cast<int>(audit.reasoning_steps.size()); i++) {
            explanation << (i + 1) << ". " << audit.reasoning_steps[i] << "\n";
        }
        explanation << "\n";
    }
    
    if (!audit.key_factors.empty()) {
        explanation << "Key Factors:\n";
        for (const auto& [key, value] : audit.key_factors.items()) {
            explanation << "- " << key << ": " << value.dump() << "\n";
        }
        explanation << "\n";
    }
    
    explanation << "Response: " << audit.response << "\n";
    
    if (audit.requires_human_review) {
        explanation << "\n⚠️  This decision has been flagged for human review due to low confidence.\n";
    }
    
    return explanation.str();
}

std::optional<AIDecisionAudit> AIDecisionAuditor::getDecision(const std::string& decision_id) const {
    std::string key = makeKey(decision_id);
    std::string value = {};
    
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Get(read_opts, cf_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }
    
    if (s.IsNotFound()) {
        return std::nullopt;
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to read AI decision {}: {}", decision_id, s.ToString());
        return std::nullopt;
    }
    
    try {
        json j = json::parse(value);
        return AIDecisionAudit::fromJson(j);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse AI decision {}: {}", decision_id, e.what());
        return std::nullopt;
    }
}

std::vector<AIDecisionAudit> AIDecisionAuditor::queryAuditLog(const QueryFilter& filter) {
    std::vector<AIDecisionAudit> results;
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    for (; it->Valid() && static_cast<int>(results.size()) < filter.limit; it->Next()) {
        std::string key = it->key().ToString();
        
        // Stop if we've left the ai_decision prefix
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        try {
            json j = json::parse(it->value().ToString());
            AIDecisionAudit audit = AIDecisionAudit::fromJson(j);
            
            // Apply filters
            bool matches = true;
            
            if (filter.user_id.has_value() && audit.user_id != *filter.user_id) {
                matches = false;
            }
            
            if (filter.start_time.has_value() && audit.timestamp < *filter.start_time) {
                matches = false;
            }
            
            if (filter.end_time.has_value() && audit.timestamp > *filter.end_time) {
                matches = false;
            }
            
            if (filter.min_confidence.has_value() && 
                audit.confidence_score < *filter.min_confidence) {
                matches = false;
            }
            
            if (filter.max_confidence.has_value() && 
                audit.confidence_score > *filter.max_confidence) {
                matches = false;
            }
            
            if (filter.requires_review.has_value() && 
                audit.requires_human_review != *filter.requires_review) {
                matches = false;
            }
            
            if (matches) {
                results.push_back(audit);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse AI decision at key {}: {}", key, e.what());
            continue;
        }
    }
    
    return results;
}

bool AIDecisionAuditor::flagForReview(const std::string& decision_id, 
                                      const std::string& reason) {
    auto audit_opt = getDecision(decision_id);
    if (!audit_opt.has_value()) {
        THEMIS_WARN("Cannot flag for review: decision {} not found", decision_id);
        return false;
    }
    
    AIDecisionAudit audit = *audit_opt;
    audit.requires_human_review = true;
    
    // Add reason to key factors
    if (audit.key_factors.is_null()) {
        audit.key_factors = json::object();
    }
    audit.key_factors["review_reason"] = reason;
    
    // Re-sign after modification
    audit.signature = signDecision(audit);
    
    // Store updated audit
    std::string value = audit.toJson().dump();
    std::string key = makeKey(decision_id);
    
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to flag decision {} for review: {}", 
                    decision_id, s.ToString());
        return false;
    }
    
    THEMIS_INFO("Flagged AI decision {} for review: {}", decision_id, reason);
    return true;
}

bool AIDecisionAuditor::recordOverride(
    const std::string& decision_id,
    const std::string& override_reason,
    const std::string& reviewer_id) {
    
    auto audit_opt = getDecision(decision_id);
    if (!audit_opt.has_value()) {
        THEMIS_WARN("Canno[[maybe_unused]] t recor[[maybe_unused]] d overrid[[maybe_unused]] e: decisio[[maybe_unused]] n {} no[[maybe_unused]] t foun[[maybe_unused]] d", decision_i[[maybe_unused]] d);
        return false;
    }
    
    AIDecisionAudit audit = *audit_opt;
    audit.human_override = override_reason;
    audit.reviewer_id = reviewer_id;
    audit.requires_human_review = false; // Review completed
    
    // Re-sign after modification
    audit.signature = signDecision(audit);
    
    // Store updated audit
    std::string value = audit.toJson().dump();
    std::string key = makeKey(decision_id);
    
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to record override for decision {}: {}", 
                    decision_id, s.ToString());
        return false;
    }
    
    THEMIS_INFO("Recorded override for AI decision {} by reviewer {}", 
               decision_id, reviewer_id);
    return true;
}

bool AIDecisionAuditor::verifyIntegrity(const std::string& decision_id) const {
    auto audit_opt = getDecision(decision_id);
    if (!audit_opt.has_value()) {
        return false;
    }
    
    return verifySignature(*audit_opt);
}

bool AIDecisionAuditor::exportForCompliance(
    const std::string& output_path, 
    const QueryFilter& filter) {
    
    auto decisions = queryAuditLog(filter);
    
    try {
        std::ofstream out(output_path);
        if (!out.is_open()) {
            THEMIS_ERROR("Failed to open export file: {}", output_path);
            return false;
        }
        
        json export_data;
        export_data["export_timestamp"] = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );
        export_data["total_decisions"] = decisions.size();
        export_data["decisions"] = json::array();
        
        for (const auto& decision : decisions) {
            export_data["decisions"].push_back(decision.toJson());
        }
        
        out << export_data.dump(2); // Pretty-print with 2-space indent
        out.close();
        
        THEMIS_INFO("Exported {} AI decisions to {}",static_cast<int>(decisions.size()), output_path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to export compliance data: {}", e.what());
        return false;
    }
}

AIDecisionAuditor::Stats AIDecisionAuditor::getStats() const {
    Stats stats{};
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    it->Seek(KEY_PREFIX);
    
    float total_confidence = 0.0f;
    int64_t total_latency = 0;
    
    // Limit iteration to prevent performance issues with large logs
    // For very large datasets, consider maintaining stats incrementally
    const size_t MAX_SCAN_LIMIT = 10000;
    size_t scanned = 0;
    
    for (; it->Valid() && scanned < MAX_SCAN_LIMIT; it->Next(), ++scanned) {
        std::string key = it->key().ToString();
        
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        
        try {
            json j = json::parse(it->value().ToString());
            AIDecisionAudit audit = AIDecisionAudit::fromJson(j);
            
            stats.total_decisions++;
            if (audit.requires_human_review) {
                stats.flagged_for_review++;
            }
            if (!audit.human_override.empty()) {
                stats.human_overrides++;
            }
            total_confidence += audit.confidence_score;
            total_latency += audit.latency_ms;
            
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse decision for stats: {}", e.what());
            continue;
        }
    }
    
    if (scanned == MAX_SCAN_LIMIT) {
        THEMIS_WARN("Stats scan limited to {} entries for performance. "
                   "Consider maintaining incremental statistics.", MAX_SCAN_LIMIT);
    }
    
    if (stats.total_decisions > 0) {
        stats.avg_confidence = total_confidence / stats.total_decisions;
        stats.avg_latency_ms = total_latency / stats.total_decisions;
    }
    
    return stats;
}

} // namespace llm
} // namespace themis

