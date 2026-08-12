/**
 * @file feedback_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/feedback_store.h"
#include "llm/lora_framework/lora_graph.h"
#include "utils/logger.h"
#include "utils/pointer_utils.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <regex>
#include <mutex>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <mutex>

namespace themis {
namespace llm {

namespace {
std::mutex g_spam_keywords_provider_mutex;
FeedbackStore::SpamKeywordsProviderFn g_spam_keywords_provider;
} // namespace

// ===== Helper function to convert enum to string =====

static std::string feedbackTypeToString(FeedbackType type) {
    switch (type) {
        case FeedbackType::POSITIVE: return "positive";
        case FeedbackType::NEGATIVE: return "negative";
        default: return "unknown";
    }
}

static FeedbackType feedbackTypeFromString(const std::string& str) {
    if (str == "positive") return FeedbackType::POSITIVE;
    if (str == "negative") return FeedbackType::NEGATIVE;
    throw std::invalid_argument("Invalid feedback type: " + str + " (must be 'positive' or 'negative')");
}

static std::string validationStatusToString(ValidationStatus status) {
    switch (status) {
        case ValidationStatus::PENDING: return "pending";
        case ValidationStatus::APPROVED: return "approved";
        case ValidationStatus::REJECTED: return "rejected";
        case ValidationStatus::FLAGGED: return "flagged";
        default: return "pending";
    }
}

static ValidationStatus validationStatusFromString(const std::string& str) {
    if (str == "pending") return ValidationStatus::PENDING;
    if (str == "approved") return ValidationStatus::APPROVED;
    if (str == "rejected") return ValidationStatus::REJECTED;
    if (str == "flagged") return ValidationStatus::FLAGGED;
    throw std::invalid_argument("Invalid validation status: " + str);
}

// ===== FeedbackEntry JSON Serialization =====

nlohmann::json FeedbackStore::FeedbackEntry::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["interaction_id"] = interaction_id;
    j["user_id"] = user_id;
    j["type"] = feedbackTypeToString(type);
    j["question"] = question;
    j["answer"] = answer;
    j["correction"] = correction;
    j["comment"] = comment;
    j["timestamp_ms"] = timestamp_ms;
    j["validation_status"] = validationStatusToString(validation_status);
    j["model_version"] = model_version;
    j["adapter_id"] = adapter_id;
    j["adapter_version"] = adapter_version;
    j["used_for_training"] = used_for_training;
    j["training_batch_id"] = training_batch_id;
    j["metadata"] = metadata;
    return j;
}

FeedbackStore::FeedbackEntry FeedbackStore::FeedbackEntry::fromJson(const nlohmann::json& j) {
    FeedbackEntry entry;
    entry.id = j.value("id", "");
    entry.interaction_id = j.value("interaction_id", "");
    entry.user_id = j.value("user_id", "");
    
    // Use default value with error handling for type
    std::string type_str = j.value("type", "positive");
    try {
        entry.type = feedbackTypeFromString(type_str);
    } catch (...) {
        THEMIS_WARN("feedback_store::feedbackTypeFromString: unhandled exception caught");
        entry.type = FeedbackType::POSITIVE; // Fallback for corrupted data
    }
    
    entry.question = j.value("question", "");
    entry.answer = j.value("answer", "");
    entry.correction = j.value("correction", "");
    entry.comment = j.value("comment", "");
    entry.timestamp_ms = j.value("timestamp_ms", int64_t(0));
    
    // Use default value with error handling for validation status
    std::string status_str = j.value("validation_status", "pending");
    try {
        entry.validation_status = validationStatusFromString(status_str);
    } catch (...) {
        THEMIS_WARN("feedback_store::validationStatusFromString: unhandled exception caught");
        entry.validation_status = ValidationStatus::PENDING; // Fallback for corrupted data
    }
    
    entry.model_version = j.value("model_version", "");
    entry.adapter_id = j.value("adapter_id", "");
    entry.adapter_version = j.value("adapter_version", "");
    entry.used_for_training = j.value("used_for_training", false);
    entry.training_batch_id = j.value("training_batch_id", 0);
    
    if (j.contains("metadata")) {
        entry.metadata = j["metadata"];
    }
    
    return entry;
}

// ===== FeedbackStore Implementation =====

FeedbackStore::FeedbackStore(rocksdb::TransactionDB* db, 
                             rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf), validation_plugin_(nullptr) {
    if (!db_) {
        throw std::invalid_argument("FeedbackStore: db cannot be null");
    }
}

void FeedbackStore::setValidationPlugin(std::shared_ptr<IFeedbackPlugin> plugin) {
    validation_plugin_ = plugin;
    if (plugin) {
        THEMIS_INFO("FeedbackStore: validation plugin set to '{}'", plugin->getName());
    } else {
        THEMIS_INFO("FeedbackStore: validation plugin disabled");
    }
}

std::shared_ptr<IFeedbackPlugin> FeedbackStore::getValidationPlugin() const {
    return validation_plugin_;
}

void FeedbackStore::setSpamKeywordsProvider(SpamKeywordsProviderFn provider) {
    std::lock_guard<std::mutex> lock(g_spam_keywords_provider_mutex);
    g_spam_keywords_provider = std::move(provider);
}

void FeedbackStore::clearSpamKeywordsProvider() {
    std::lock_guard<std::mutex> lock(g_spam_keywords_provider_mutex);
    g_spam_keywords_provider = {};
}


std::string FeedbackStore::makeKey(const std::string& id) const {
    return std::string(KEY_PREFIX) + id;
}

std::string FeedbackStore::makeGraphEdgeKey(const std::string& feedback_id,
                                             const std::string& adapter_id) const {
    return std::string(GRAPH_EDGE_PREFIX) + feedback_id + ":" + adapter_id;
}


std::string FeedbackStore::generateId() const {
    // Simple UUID-like ID generation (timestamp + random)
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "-"
        << std::setw(16) << dis(gen);
    
    return oss.str();
}

FeedbackStore::FeedbackEntry FeedbackStore::createFeedback(FeedbackEntry feedback) {
    // Generate ID if empty
    if (feedback.id.empty()) {
        feedback.id = generateId();
    }
    
    // Set timestamp if not set
    if (feedback.timestamp_ms == 0) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        feedback.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }
    
    // Apply validation (plugin or basic)
    if (feedback.validation_status == ValidationStatus::PENDING) {
        feedback.validation_status = applyPluginValidation(feedback);
    }
    
    // Serialize to JSON
    std::string value = feedback.toJson().dump();
    std::string key = makeKey(feedback.id);
    
    // Store in RocksDB
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to store feedback {}: {}", feedback.id, s.ToString());
        throw std::runtime_error("Failed to store feedback: " + s.ToString());
    }
    
    THEMIS_DEBUG("Stored feedback {} (type: {}, status: {})", 
                 feedback.id, 
                 feedbackTypeToString(feedback.type),
                 validationStatusToString(feedback.validation_status));
    
    return feedback;
}

std::optional<FeedbackStore::FeedbackEntry> FeedbackStore::getFeedback(const std::string& id) const {
    std::string key = makeKey(id);
    std::string value;
    
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
        THEMIS_ERROR("Failed to read feedback {}: {}", id, s.ToString());
        return std::nullopt;
    }
    
    try {
        nlohmann::json j = nlohmann::json::parse(value);
        return FeedbackEntry::fromJson(j);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse feedback {}: {}", id, e.what());
        return std::nullopt;
    }
}

std::vector<FeedbackStore::FeedbackEntry> FeedbackStore::listFeedback() const {
    return listFeedback(ListOptions{});
}

std::vector<FeedbackStore::FeedbackEntry> FeedbackStore::listFeedback(const ListOptions& options) const {
    std::vector<FeedbackEntry> results;
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    if (!it) {
        THEMIS_ERROR("Failed to create RocksDB iterator for feedback listing");
        return results; // Return empty vector
    }
    
    // Start from beginning or after cursor
    std::string start_key = KEY_PREFIX;
    if (options.start_after_id) {
        start_key = makeKey(*options.start_after_id);
        it->Seek(start_key);
        if (it->Valid() && it->key().ToString() == start_key) {
            it->Next(); // Skip the cursor item
        }
    } else {
        it->Seek(start_key);
    }
    
    // Iterate and apply filters
    size_t count = 0;
    while (it->Valid() && count < options.limit) {
        std::string key = it->key().ToString();
        
        // Check if still in our prefix
        if (key.substr(0, strlen(KEY_PREFIX)) != KEY_PREFIX) {
            break;
        }
        
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            FeedbackEntry entry = FeedbackEntry::fromJson(j);
            
            // Apply filters
            bool matches = true;
            
            if (options.filter_type && entry.type != *options.filter_type) {
                matches = false;
            }
            
            if (options.filter_status && entry.validation_status != *options.filter_status) {
                matches = false;
            }
            
            if (options.filter_model && entry.model_version != *options.filter_model) {
                matches = false;
            }
            
            if (options.filter_adapter && entry.adapter_id != *options.filter_adapter) {
                matches = false;
            }
            
            if (options.since_timestamp_ms && entry.timestamp_ms < *options.since_timestamp_ms) {
                matches = false;
            }
            
            if (options.unused_for_training && *options.unused_for_training && entry.used_for_training) {
                matches = false;
            }
            
            if (matches) {
                results.push_back(entry);
                count++;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse feedback entry: {}", e.what());
        }
        
        it->Next();
    }
    
    return results;
}

FeedbackStore::Stats FeedbackStore::getStats() const {
    Stats stats{};
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    if (!it) {
        THEMIS_ERROR("Failed to create RocksDB iterator for feedback stats");
        return stats; // Return empty stats
    }
    
    std::string start_key = KEY_PREFIX;
    it->Seek(start_key);
    
    while (it->Valid()) {
        std::string key = it->key().ToString();
        
        // Check if still in our prefix
        if (key.substr(0, strlen(KEY_PREFIX)) != KEY_PREFIX) {
            break;
        }
        
        try {
            nlohmann::json j = nlohmann::json::parse(it->value().ToString());
            FeedbackEntry entry = FeedbackEntry::fromJson(j);
            
            stats.total_feedback++;
            
            if (entry.type == FeedbackType::POSITIVE) {
                stats.positive_count++;
            } else {
                stats.negative_count++;
            }
            
            switch (entry.validation_status) {
                case ValidationStatus::PENDING:
                    stats.pending_validation++;
                    break;
                case ValidationStatus::APPROVED:
                    stats.approved_count++;
                    break;
                case ValidationStatus::REJECTED:
                    stats.rejected_count++;
                    break;
                case ValidationStatus::FLAGGED:
                    // Flagged items are counted but not in other categories
                    break;
            }
            
            if (entry.used_for_training) {
                stats.used_for_training++;
            } else {
                stats.unused_for_training++;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse feedback entry for stats: {}", e.what());
        }
        
        it->Next();
    }
    
    // Calculate positive ratio
    if (stats.total_feedback > 0) {
        stats.positive_ratio = static_cast<double>(stats.positive_count) / 
                              static_cast<double>(stats.total_feedback);
    } else {
        stats.positive_ratio = 0.0;
    }
    
    return stats;
}

bool FeedbackStore::deleteFeedback(const std::string& id) {
    std::string key = makeKey(id);
    
    // Check if exists first
    auto existing = getFeedback(id);
    if (!existing) {
        return false;
    }
    
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Delete(write_opts, cf_, key);
    } else {
        s = db_->Delete(write_opts, key);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to delete feedback {}: {}", id, s.ToString());
        return false;
    }
    
    THEMIS_DEBUG("Deleted feedback {}", id);
    return true;
}

bool FeedbackStore::updateValidationStatus(const std::string& id, ValidationStatus status) {
    auto feedback = getFeedback(id);
    if (!feedback) {
        return false;
    }
    
    feedback->validation_status = status;
    
    try {
        createFeedback(*feedback); // Overwrite existing
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to update validation status for {}: {}", id, e.what());
        return false;
    }
}

bool FeedbackStore::markUsedForTraining(const std::string& id, int batch_id) {
    auto feedback = getFeedback(id);
    if (!feedback) {
        return false;
    }
    
    feedback->used_for_training = true;
    feedback->training_batch_id = batch_id;
    
    try {
        createFeedback(*feedback); // Overwrite existing
        THEMIS_DEBUG("Marked feedback {} as used in training batch {}", id, batch_id);
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to mark feedback {} for training: {}", id, e.what());
        return false;
    }
}

void FeedbackStore::clear() {
    rocksdb::ReadOptions read_opts;
    rocksdb::WriteOptions write_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    if (!it) {
        THEMIS_ERROR("Failed to create RocksDB iterator for feedback clearing");
        return;
    }
    
    std::string start_key = KEY_PREFIX;
    it->Seek(start_key);
    
    size_t deleted = 0;
    while (it->Valid()) {
        std::string key = it->key().ToString();
        
        // Check if still in our prefix
        if (key.substr(0, strlen(KEY_PREFIX)) != KEY_PREFIX) {
            break;
        }
        
        rocksdb::Status s;
        if (cf_) {
            s = db_->Delete(write_opts, cf_, key);
        } else {
            s = db_->Delete(write_opts, key);
        }
        
        if (s.ok()) {
            deleted++;
        }
        
        it->Next();
    }
    
    THEMIS_INFO("Cleared {} feedback entries", deleted);
}

// ===== Validation Logic =====

std::vector<std::string> FeedbackStore::getSpamKeywords() {
    // Default spam keywords used when no runtime provider is configured.
    static const std::vector<std::string> default_spam_keywords = {
        "buy now", "click here", "viagra", "casino", "lottery", 
        "free money", "million dollars", "nigerian prince",
        "weight loss", "work from home", "make money fast"
    };

    SpamKeywordsProviderFn provider;
    {
        std::lock_guard<std::mutex> lock(g_spam_keywords_provider_mutex);
        provider = g_spam_keywords_provider;
    }

    if (provider) {
        try {
            auto runtime_keywords = provider();
            if (!runtime_keywords.empty()) {
                runtime_keywords.erase(
                    std::remove_if(runtime_keywords.begin(),
                                   runtime_keywords.end(),
                                   [](const auto& keyword) { return keyword.empty(); }),
                    runtime_keywords.end());
            }

            if (!runtime_keywords.empty()) {
                return runtime_keywords;
            }

            THEMIS_WARN("Spam keywords provider returned empty list; using built-in defaults");
        } catch (const std::exception& e) {
            THEMIS_WARN("Spam keywords provider failed: {}; using built-in defaults", e.what());
        }
    }

    return default_spam_keywords;
}

bool FeedbackStore::isLikelySpam(const std::string& text) {
    if (text.empty()) {
        return false; // Empty is not spam, just low quality
    }
    
    // Too short (likely not meaningful)
    if (text.length() < 3) {
        return true;
    }
    
    // Too long (likely copy-paste spam)
    if (text.length() > 10000) {
        return true;
    }
    
    // Excessive repetition
    std::regex repeat_pattern(R"((.)\1{10,})"); // Same character 10+ times
    if (std::regex_search(text, repeat_pattern)) {
        return true;
    }
    
    // Common spam patterns
    const auto spam_keywords = getSpamKeywords();
    
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    for (const auto& keyword : spam_keywords) {
        if (lower_text.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

ValidationStatus FeedbackStore::validateFeedback(const FeedbackEntry& feedback) {
    // Check for spam in question, answer, correction, and comment
    if (isLikelySpam(feedback.question) || 
        isLikelySpam(feedback.answer) ||
        isLikelySpam(feedback.correction) ||
        isLikelySpam(feedback.comment)) {
        return ValidationStatus::REJECTED;
    }
    
    // Negative feedback should have a correction or comment
    if (feedback.type == FeedbackType::NEGATIVE) {
        if (feedback.correction.empty() && feedback.comment.empty()) {
            return ValidationStatus::FLAGGED; // Flag for manual review
        }
    }
    
    // Check minimum quality thresholds
    if (feedback.question.empty() || feedback.answer.empty()) {
        return ValidationStatus::REJECTED;
    }
    
    // Basic quality check: question and answer should be reasonably sized
    if (feedback.question.length() < 5 || feedback.answer.length() < 5) {
        return ValidationStatus::FLAGGED;
    }
    
    // All checks passed
    return ValidationStatus::APPROVED;
}

// ===== Plugin Integration =====

ValidationStatus FeedbackStore::applyPluginValidation(FeedbackEntry& feedback) {
    if (!validation_plugin_) {
        // No plugin, use basic validation
        return validateFeedback(feedback);
    }
    
    // Convert to plugin format
    FeedbackData data;
    data.question = feedback.question;
    data.answer = feedback.answer;
    data.correction = feedback.correction;
    data.comment = feedback.comment;
    data.user_id = feedback.user_id;
    data.adapter_id = feedback.adapter_id;
    data.model_version = feedback.model_version;
    data.is_positive = (feedback.type == FeedbackType::POSITIVE);
    data.metadata = feedback.metadata;
    
    // Validate through plugin
    try {
        auto result = validation_plugin_->validate(data);
        
        switch (result.result) {
            case FeedbackValidationResult::ACCEPT:
                return ValidationStatus::APPROVED;
            case FeedbackValidationResult::REJECT:
                return ValidationStatus::REJECTED;
            case FeedbackValidationResult::FLAG:
                return ValidationStatus::FLAGGED;
            case FeedbackValidationResult::MODIFY:
                // Apply plugin-provided transformations before storing the feedback.
                if (result.modified_comment.has_value()) {
                    feedback.comment = *result.modified_comment;
                }
                if (result.modified_metadata.has_value()) {
                    feedback.metadata = *result.modified_metadata;
                }
                return ValidationStatus::APPROVED;
            default:
                return ValidationStatus::PENDING;
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Plugin validation failed: {}", e.what());
        // Fallback to basic validation
        return validateFeedback(feedback);
    }
}

// ===== Graph Link Methods =====

bool FeedbackStore::createAdapterLink(
    const std::string& feedback_id,
    const std::string& adapter_id,
    const nlohmann::json& metadata) {
    
    // Check if feedback exists
    if (!getFeedback(feedback_id)) {
        THEMIS_ERROR("Cannot create adapter link: feedback {} not found", feedback_id);
        return false;
    }
    
    // Create graph edge
    lora::LoRAGraphEdge edge;
    edge.from_id = feedback_id;
    edge.to_id = adapter_id;
    edge.edge_type = lora::LoRAEdgeType::FEEDBACK_FOR;
    edge.weight = 1.0f;
    edge.metadata = metadata;
    edge.created_at = std::chrono::system_clock::now();
    
    // Serialize edge
    std::string key = makeGraphEdgeKey(feedback_id, adapter_id);
    std::string value = edge.toJSON().dump();
    
    // Store in RocksDB
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to create adapter link: {}", s.ToString());
        return false;
    }
    
    THEMIS_DEBUG("Created FEEDBACK_FOR link: {} -> {}", feedback_id, adapter_id);
    return true;
}

std::vector<FeedbackStore::FeedbackEntry> FeedbackStore::getFeedbackForAdapter(
    const std::string& adapter_id,
    const ListOptions& options) const {
    
    std::vector<FeedbackEntry> results;
    
    // First, find all feedback IDs linked to this adapter
    std::vector<std::string> feedback_ids;
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    if (!it) {
        THEMIS_ERROR("Failed to create RocksDB iterator for adapter feedback lookup");
        return results; // Return empty vector
    }
    
    // Scan graph edges
    std::string edge_prefix = GRAPH_EDGE_PREFIX;
    it->Seek(edge_prefix);
    
    while (it->Valid()) {
        std::string key = it->key().ToString();
        
        if (key.substr(0, edge_prefix.length()) != edge_prefix) {
            break;
        }
        
        try {
            nlohmann::json edge_json = nlohmann::json::parse(it->value().ToString());
            std::string to_id = edge_json.value("to", "");
            
            if (to_id == adapter_id) {
                std::string from_id = edge_json.value("from", "");
                feedback_ids.push_back(from_id);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse graph edge: {}", e.what());
        }
        
        it->Next();
    }
    
    // Now fetch feedback entries for these IDs
    for (const auto& feedback_id : feedback_ids) {
        auto feedback = getFeedback(feedback_id);
        if (feedback) {
            // Apply filters
            bool matches = true;
            
            if (options.filter_type && feedback->type != *options.filter_type) {
                matches = false;
            }
            
            if (options.filter_status && feedback->validation_status != *options.filter_status) {
                matches = false;
            }
            
            if (options.since_timestamp_ms && feedback->timestamp_ms < *options.since_timestamp_ms) {
                matches = false;
            }
            
            if (options.unused_for_training && *options.unused_for_training && feedback->used_for_training) {
                matches = false;
            }
            
            if (matches) {
                results.push_back(*feedback);
                if (results.size() >= options.limit) {
                    break;
                }
            }
        }
    }
    
    return results;
}

std::vector<FeedbackStore::FeedbackEntry> FeedbackStore::getFeedbackForAdapter(
    const std::string& adapter_id) const {
    return getFeedbackForAdapter(adapter_id, ListOptions{});
}

std::vector<std::string> FeedbackStore::getLinkedAdapters(const std::string& feedback_id) const {
    std::vector<std::string> adapter_ids;
    
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;
    
    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }
    
    if (!it) {
        THEMIS_ERROR("Failed to create RocksDB iterator for linked adapters lookup");
        return adapter_ids; // Return empty vector
    }
    
    // Search for edges starting with this feedback ID
    std::string edge_prefix = std::string(GRAPH_EDGE_PREFIX) + feedback_id + ":";
    it->Seek(edge_prefix);
    
    while (it->Valid()) {
        std::string key = it->key().ToString();
        
        if (key.substr(0, edge_prefix.length()) != edge_prefix) {
            break;
        }
        
        try {
            nlohmann::json edge_json = nlohmann::json::parse(it->value().ToString());
            std::string to_id = edge_json.value("to", "");
            if (!to_id.empty()) {
                adapter_ids.push_back(to_id);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse graph edge: {}", e.what());
        }
        
        it->Next();
    }
    
    return adapter_ids;
}

bool FeedbackStore::isLinkedToAdapter(
    const std::string& feedback_id,
    const std::string& adapter_id) const {
    
    std::string key = makeGraphEdgeKey(feedback_id, adapter_id);
    std::string value;
    
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;
    
    if (cf_) {
        s = db_->Get(read_opts, cf_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }
    
    return s.ok();
}

} // namespace llm
} // namespace themis

