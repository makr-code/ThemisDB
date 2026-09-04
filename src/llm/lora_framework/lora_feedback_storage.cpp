/**
 * @file lora_feedback_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_feedback_storage.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

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
    // Ensure the database is opened for CRUD operations in tests (monolithic, internal storage)
    if (!config_.db->isOpen()) {
        bool ok = config_.db->open();
        if (!ok) {
            throw std::runtime_error("FeedbackStorageService: failed to open RocksDB database");
        }
    }
    
    spdlog::info("FeedbackStorageService initialized with collection: {}", 
                 config_.collection_name);
}

FeedbackStorageService::~FeedbackStorageService() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — pImpl destructor may throw.
    // Reset inside try/catch to enforce noexcept guarantee.
    try {
        // members (plugins_, mutex_) destroyed after this block
    } catch (const std::exception& e) {
        spdlog::error("FeedbackStorageService::~FeedbackStorageService: exception (suppressed): {}", e.what());
    } catch (...) {
        spdlog::error("FeedbackStorageService::~FeedbackStorageService: unknown exception (suppressed)");
    }
}

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
        // Store in DB under lock to protect DB handle usage
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::string key = makeFeedbackKey(feedback.id);
            std::string value = feedback.toJSON().dump();

            bool success = config_.db->put(key, value);
            if (!success) {
                spdlog::error("Failed to store feedback {}", feedback.id);
                return std::nullopt;
            }
        }

        // Create graph link to adapter without holding the service mutex to avoid nested locks
        if (config_.enable_graph_links &&
            (config_.graph_index || config_.create_graph_link_fn)) {
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
        std::string value = {};
        
        bool success = config_.db->get(key, value);
        if (!success) {
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
        // Prefix scan over collection namespace: "<collection_name>:<id>"
        const std::string prefix = config_.collection_name + ":";
        size_t skipped = 0;
        
        config_.db->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
            try {
                // Parse JSON value into Feedback
                json j = json::parse(value);
                Feedback fb = Feedback::fromJSON(j);
                
                // Apply filters
                if (filter.adapter_id && fb.adapter_id != *filter.adapter_id) return true; // continue
                if (filter.user_id && fb.user_id != *filter.user_id) return true; // continue
                if (filter.min_rating && fb.rating < *filter.min_rating) return true; // continue
                if (filter.flagged_for_training && fb.flagged_for_training != *filter.flagged_for_training) return true; // continue
                if (filter.training_category && fb.training_category != *filter.training_category) return true; // continue
                if (filter.since && fb.timestamp < *filter.since) return true; // continue
                
                // Pagination: offset then limit
                if (skipped < filter.offset) {
                    ++skipped;
                    return true; // continue
                }
                
                results.push_back(std::move(fb));
                if (results.size() >= filter.limit) {
                    return false; // stop scanning
                }
            } catch (const std::exception& e) {
                spdlog::warn("Failed to parse feedback entry for key {}: {}", std::string(key), e.what());
                // Continue scanning despite parse errors
            }
            return true; // continue
        });
        
        return results;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception listing feedback: {}", e.what());
        return results;
    }
}

bool FeedbackStorageService::updateFeedback(const std::string& id, const Feedback& feedback) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if feedback exists
        std::string key = makeFeedbackKey(id);
        std::string old_value = {};
        bool exists = config_.db->get(key, old_value);
        if (!exists) {
            return false;
        }
        
        // Update with new data
        auto updated_feedback = feedback;
        updated_feedback.id = id; // Preserve ID
        
        std::string value = updated_feedback.toJSON().dump();
        bool success = config_.db->put(key, value);
        
        if (!success) {
            spdlog::error("Failed to update feedback {}", id);
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
        // Retrieve feedback (acquires lock internally)
        auto feedback = getFeedback(id);

        // Delete under lock to protect DB handle
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::string key = makeFeedbackKey(id);
            bool success = config_.db->del(key);

            if (!success) {
                return false;
            }
        }

        // Remove graph link without holding mutex to avoid nested locks
        if (config_.enable_graph_links &&
            (config_.graph_index || config_.remove_graph_link_fn) &&
            feedback) {
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
    FeedbackFilter filter = {};
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
    FeedbackFilter filter = {};
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
        if (fb.flagged_for_training) {
          flagged_count++;
        }
        rating_counts[fb.rating]++;
        if (!fb.training_category.empty()) {
            category_counts[fb.training_category]++;
        }
    }
    
    stats["avg_rating"] = static_cast<double>(sum_rating) / feedback_list.size();
    stats["flagged_for_training"] = flagged_count;
    stats["by_rating"] = rating_counts;
    stats["by_category"] = category_counts;
    
    // Cache statistics
    int cached_count = 0;
    int direct_count = 0;
    float total_weight = 0.0f;
    
    for (const auto& fb : feedback_list) {
        if (fb.is_cached_response) {
            cached_count++;
        } else {
            direct_count++;
        }
        if (fb.flagged_for_training) {
            total_weight += fb.training_weight;
        }
    }
    
    stats["cached_responses"] = cached_count;
    stats["direct_responses"] = direct_count;
    stats["effective_training_size"] = total_weight;
    
    return stats;
}

std::vector<Feedback> FeedbackStorageService::getWeightedTrainingFeedback(
    const std::optional<std::string>& adapter_id,
    size_t limit
) const {
    // Get training feedback
    auto training_feedback = getTrainingFeedback(adapter_id, limit * 2);
    
    // Sort by training weight (descending) to prioritize high-weight feedback
    std::sort(training_feedback.begin(), training_feedback.end(),
        [](const Feedback& a, const Feedback& b) {
            return a.training_weight > b.training_weight;
        }
    );
    
    // Take top 'limit' entries
    if (training_feedback.size() > limit) {
        training_feedback.resize(limit);
    }
    
    return training_feedback;
}

float FeedbackStorageService::calculateEffectiveBatchSize(const std::string& adapter_id) const {
    auto training_feedback = getTrainingFeedback(adapter_id, 10000);
    
    float effective_size = 0.0f;
    for (const auto& fb : training_feedback) {
        effective_size += fb.training_weight;
    }
    
    return effective_size;
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

std::string FeedbackStorageService::generateFeedbackId() const {
    // Windows-compatible UUID generation (RFC 4122 v4)
    std::random_device rd = {};
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);
    
    // Set version (4) and variant bits
    part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0')
        << std::setw(8) << ((part1 >> 32) & 0xFFFFFFFF) << '-'
        << std::setw(4) << ((part1 >> 16) & 0xFFFF) << '-'
        << std::setw(4) << (part1 & 0xFFFF) << '-'
        << std::setw(4) << ((part2 >> 48) & 0xFFFF) << '-'
        << std::setw(12) << (part2 & 0xFFFFFFFFFFFFULL);
    return oss.str();
}

std::string FeedbackStorageService::makeFeedbackKey(const std::string& id) const {
    return config_.collection_name + ":" + id;
}

bool FeedbackStorageService::createGraphLink(
    const std::string& feedback_id,
    const std::string& adapter_id
) {
    if (!config_.graph_index && !create_graph_link_fn_) {
        spdlog::warn([[maybe_unused]] "No graph index or callback available for creating graph link");
        return false;
    }
    
    try {
        std::string from = makeFeedbackKey(feedback_id);
        std::string to = "lora_adapters:" + adapter_id;
        const std::string edge_type = "belongs_to_adapter";
        
        // Try callback first if available (Stub #304 remediation)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (create_graph_link_fn_) {
                if (create_graph_link_fn_(from, to, edge_type)) {
                    spdlog::debug("Created graph link via callback: {} -> {}", from, to);
                    return true;
                } else {
                    spdlog::error("Callback failed to create graph link {} -> {}", from, to);
                    return false;
                }
            }
        }
        
        // Fall back to direct graph index if no callback (backward compatibility)
        if (config_.graph_index) {
            const std::string edge_id = "feedback_link:" + feedback_id + ":" + adapter_id;
            
            BaseEntity edge;
            edge.setPrimaryKey(edge_id);
            edge.setField("id", Value(edge_id));
            edge.setField("_from", Value(from));
            edge.setField("_to", Value(to));
            edge.setField("_type", Value(edge_type));

            auto status = config_.graph_index->addEdge(edge);
            if (!status.ok) {
                spdlog::error("Failed to create graph link {} -> {}: {}", from, to, status.message);
                return false;
            }
            spdlog::debug("Created graph link via graph index: {} -> {}", from, to);
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception creating graph link: {}", e.what());
        return false;
    }
}

bool FeedbackStorageService::removeGraphLink(
    const std::string& feedback_id,
    const std::string& adapter_id
) {
    if (!config_.graph_index && !remove_graph_link_fn_) {
        spdlog::warn([[maybe_unused]] "No graph index or callback available for removing graph link");
        return false;
    }
    
    try {
        std::string from = makeFeedbackKey(feedback_id);
        std::string to = "lora_adapters:" + adapter_id;
        const std::string edge_type = "belongs_to_adapter";
        
        // Try callback first if available (Stub #304 remediation)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (remove_graph_link_fn_) {
                if (remove_graph_link_fn_(from, to, edge_type)) {
                    spdlog::debug("Removed graph link via callback: {} -> {}", from, to);
                    return true;
                } else {
                    spdlog::error("Callback failed to remove graph link {} -> {}", from, to);
                    return false;
                }
            }
        }
        
        // Fall back to direct graph index if no callback (backward compatibility)
        if (config_.graph_index) {
            const std::string edge_id = "feedback_link:" + feedback_id + ":" + adapter_id;
            auto status = config_.graph_index->deleteEdge(edge_id);
            if (!status.ok) {
                spdlog::error("Failed to remove graph link {}: {}", edge_id, status.message);
                return false;
            }
            spdlog::debug("Removed graph link via graph index: {}", edge_id);
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception removing graph link: {}", e.what());
        return false;
    }
}

void FeedbackStorageService::setCreateGraphLinkFn(CreateGraphLinkFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    create_graph_link_fn_ = std::move(fn);
}

void FeedbackStorageService::setRemoveGraphLinkFn(RemoveGraphLinkFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    remove_graph_link_fn_ = std::move(fn);
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
