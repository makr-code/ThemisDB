#include "llm/lora_framework/lora_feedback_storage.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <uuid/uuid.h>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

// ═══════════════════════════════════════════════════════════
// FeedbackStorageService Implementation
// ═══════════════════════════════════════════════════════════

FeedbackStorageService::FeedbackStorageService(const Config& config)
    : config_(config)
{
    if (!config_.db) {
        throw std::runtime_error("FeedbackStorageService: RocksDB instance is required");
    }
    
    spdlog::info("FeedbackStorageService initialized with collection: {}", 
                 config_.collection_name);
}

FeedbackStorageService::~FeedbackStorageService() = default;

void FeedbackStorageService::registerPlugin(std::shared_ptr<FeedbackPlugin> plugin) {
    std::lock_guard<std::mutex> lock(mutex_);
    plugins_.push_back(plugin);
    spdlog::info("Registered feedback plugin: {}", plugin->getName());
}

std::optional<Feedback> FeedbackStorageService::createFeedback(Feedback feedback) {
    // Generate ID if not provided
    if (feedback.id.empty()) {
        feedback.id = generateFeedbackId();
    }
    
    // Set timestamp if not provided
    if (feedback.timestamp == std::chrono::system_clock::time_point{}) {
        feedback.timestamp = std::chrono::system_clock::now();
    }
    
    // Run validation
    if (!runValidation(feedback)) {
        spdlog::warn("Feedback validation failed for adapter: {}", feedback.adapter_id);
        return std::nullopt;
    }
    
    // Run processing
    runProcessing(feedback);
    
    // Store in database
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = makeFeedbackKey(feedback.id);
        std::string value = feedback.toJSON().dump();
        
        auto status = config_.db->put(key, value);
        if (!status.ok()) {
            spdlog::error("Failed to store feedback: {}", status.ToString());
            return std::nullopt;
        }
        
        // Create graph link to adapter
        if (config_.enable_graph_links && config_.graph_index) {
            createGraphLink(feedback.id, feedback.adapter_id);
        }
        
        spdlog::debug("Created feedback {} for adapter {}", feedback.id, feedback.adapter_id);
        return feedback;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception storing feedback: {}", e.what());
        return std::nullopt;
    }
}

std::optional<Feedback> FeedbackStorageService::getFeedback(const std::string& id) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = makeFeedbackKey(id);
        std::string value;
        
        auto status = config_.db->get(key, value);
        if (!status.ok()) {
            return std::nullopt;
        }
        
        auto j = json::parse(value);
        return Feedback::fromJSON(j);
        
    } catch (const std::exception& e) {
        spdlog::error("Exception retrieving feedback {}: {}", id, e.what());
        return std::nullopt;
    }
}

std::vector<Feedback> FeedbackStorageService::listFeedback(const FeedbackFilter& filter) const {
    std::vector<Feedback> results;
    
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string prefix = config_.collection_name + ":";
        std::vector<std::string> keys;
        config_.db->scanKeys(prefix, keys);
        
        size_t count = 0;
        size_t skipped = 0;
        
        for (const auto& key : keys) {
            // Skip if we've hit the offset
            if (skipped < filter.offset) {
                skipped++;
                continue;
            }
            
            // Stop if we've reached the limit
            if (count >= filter.limit) {
                break;
            }
            
            std::string value;
            auto status = config_.db->get(key, value);
            if (!status.ok()) continue;
            
            try {
                auto j = json::parse(value);
                Feedback fb = Feedback::fromJSON(j);
                
                // Apply filters
                if (filter.adapter_id && fb.adapter_id != *filter.adapter_id) continue;
                if (filter.user_id && fb.user_id != *filter.user_id) continue;
                if (filter.min_rating && fb.rating < *filter.min_rating) continue;
                if (filter.flagged_for_training && fb.flagged_for_training != *filter.flagged_for_training) continue;
                if (filter.training_category && fb.training_category != *filter.training_category) continue;
                if (filter.since && fb.timestamp < *filter.since) continue;
                
                results.push_back(fb);
                count++;
                
            } catch (const std::exception& e) {
                spdlog::warn("Failed to parse feedback: {}", e.what());
                continue;
            }
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Exception listing feedback: {}", e.what());
    }
    
    return results;
}

bool FeedbackStorageService::updateFeedback(const std::string& id, const Feedback& feedback) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if feedback exists
        std::string key = makeFeedbackKey(id);
        std::string old_value;
        auto status = config_.db->get(key, old_value);
        if (!status.ok()) {
            return false;
        }
        
        // Update with new data
        auto updated_feedback = feedback;
        updated_feedback.id = id; // Preserve ID
        
        std::string value = updated_feedback.toJSON().dump();
        status = config_.db->put(key, value);
        
        if (!status.ok()) {
            spdlog::error("Failed to update feedback {}: {}", id, status.ToString());
            return false;
        }
        
        spdlog::debug("Updated feedback {}", id);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception updating feedback {}: {}", id, e.what());
        return false;
    }
}

bool FeedbackStorageService::deleteFeedback(const std::string& id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Get feedback to find adapter_id for graph link removal
        auto feedback = getFeedback(id);
        
        std::string key = makeFeedbackKey(id);
        auto status = config_.db->del(key);
        
        if (!status.ok()) {
            return false;
        }
        
        // Remove graph link
        if (config_.enable_graph_links && config_.graph_index && feedback) {
            removeGraphLink(id, feedback->adapter_id);
        }
        
        spdlog::debug("Deleted feedback {}", id);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception deleting feedback {}: {}", id, e.what());
        return false;
    }
}

std::vector<Feedback> FeedbackStorageService::getFeedbackForAdapter(
    const std::string& adapter_id,
    size_t limit
) const {
    FeedbackFilter filter;
    filter.adapter_id = adapter_id;
    filter.limit = limit;
    return listFeedback(filter);
}

std::vector<Feedback> FeedbackStorageService::getTrainingFeedback(
    const std::optional<std::string>& adapter_id,
    size_t limit
) const {
    FeedbackFilter filter;
    if (adapter_id) {
        filter.adapter_id = adapter_id;
    }
    filter.flagged_for_training = true;
    filter.limit = limit;
    return listFeedback(filter);
}

bool FeedbackStorageService::shouldTriggerTraining(const std::string& adapter_id) const {
    auto training_feedback = getTrainingFeedback(adapter_id);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check with all registered plugins
    for (const auto& plugin : plugins_) {
        if (plugin->onTrainingTrigger(training_feedback)) {
            return true;
        }
    }
    
    return false;
}

json FeedbackStorageService::getStatistics(const std::optional<std::string>& adapter_id) const {
    FeedbackFilter filter;
    if (adapter_id) {
        filter.adapter_id = adapter_id;
    }
    filter.limit = 10000; // Get more for statistics
    
    auto feedback_list = listFeedback(filter);
    
    json stats;
    stats["total_count"] = feedback_list.size();
    
    if (feedback_list.empty()) {
        stats["avg_rating"] = 0.0;
        stats["flagged_for_training"] = 0;
        stats["by_rating"] = json::object();
        return stats;
    }
    
    // Calculate statistics
    int64_t sum_rating = 0;
    int flagged_count = 0;
    std::map<int, int> rating_counts;
    std::map<std::string, int> category_counts;
    
    for (const auto& fb : feedback_list) {
        sum_rating += fb.rating;
        if (fb.flagged_for_training) flagged_count++;
        rating_counts[fb.rating]++;
        if (!fb.training_category.empty()) {
            category_counts[fb.training_category]++;
        }
    }
    
    stats["avg_rating"] = static_cast<double>(sum_rating) / feedback_list.size();
    stats["flagged_for_training"] = flagged_count;
    stats["by_rating"] = rating_counts;
    stats["by_category"] = category_counts;
    
    return stats;
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

std::string FeedbackStorageService::generateFeedbackId() const {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

std::string FeedbackStorageService::makeFeedbackKey(const std::string& id) const {
    return config_.collection_name + ":" + id;
}

bool FeedbackStorageService::createGraphLink(
    const std::string& feedback_id,
    const std::string& adapter_id
) {
    if (!config_.graph_index) {
        return false;
    }
    
    try {
        // Create edge: feedback --[belongs_to_adapter]--> adapter
        std::string from = makeFeedbackKey(feedback_id);
        std::string to = "lora_adapters:" + adapter_id;
        std::string edge_type = "belongs_to_adapter";
        
        // Note: GraphIndex API may vary - adapt as needed
        // This is a placeholder for the actual graph link creation
        spdlog::debug("Created graph link: {} --[{}]--> {}", from, edge_type, to);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create graph link: {}", e.what());
        return false;
    }
}

bool FeedbackStorageService::removeGraphLink(
    const std::string& feedback_id,
    const std::string& adapter_id
) {
    if (!config_.graph_index) {
        return false;
    }
    
    try {
        // Remove edge: feedback --[belongs_to_adapter]--> adapter
        std::string from = makeFeedbackKey(feedback_id);
        std::string to = "lora_adapters:" + adapter_id;
        std::string edge_type = "belongs_to_adapter";
        
        spdlog::debug("Removed graph link: {} --[{}]--> {}", from, edge_type, to);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to remove graph link: {}", e.what());
        return false;
    }
}

bool FeedbackStorageService::runValidation(const Feedback& feedback) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Run validation through all plugins
    for (const auto& plugin : plugins_) {
        if (!plugin->validate(feedback)) {
            spdlog::debug("Validation failed by plugin: {}", plugin->getName());
            return false;
        }
    }
    
    return true;
}

void FeedbackStorageService::runProcessing(Feedback& feedback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Run processing through all plugins
    for (const auto& plugin : plugins_) {
        plugin->process(feedback);
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
