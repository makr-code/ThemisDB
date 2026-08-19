/**
 * @file lora_feedback_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_feedback.h"
#include "feedback_plugin.h"
#include "storage/rocksdb_wrapper.h"
#include "index/graph_index.h"
#include <memory>
#include <vector>
#include <optional>
#include <mutex>
#include <functional>

namespace themis {
namespace llm {
namespace lora {

// Reuse the global GraphIndexManager type to avoid duplicate class names
using GraphIndexManager = ::themis::GraphIndexManager;

/**
 * @brief Feedback storage service with graph integration
 * 
 * Features:
 * - CRUD operations for feedback
 * - Graph links to LoRA adapters via "belongs_to_adapter" edges
 * - Plugin-based validation and processing
 * - Uses existing help_feedback collection
 */

class FeedbackStorageService {
public:
    using CreateGraphLinkFn = std::function<bool(const std::string& feedback_pk,
                                                 const std::string& adapter_pk,
                                                 const std::string& edge_type)>;
    using RemoveGraphLinkFn = std::function<bool(const std::string& feedback_pk,
                                                 const std::string& adapter_pk,
                                                 const std::string& edge_type)>;

    /**
     * @brief Configuration for feedback storage
     */
    struct Config {
        std::shared_ptr<RocksDBWrapper> db;              // RocksDB instance
        std::shared_ptr<GraphIndexManager> graph_index;  // Graph index for relationships
        std::string collection_name = "help_feedback";   // Collection name
        bool enable_graph_links = true;                  // Enable graph relationships
        CreateGraphLinkFn create_graph_link_fn;          // Optional graph-link bridge
        RemoveGraphLinkFn remove_graph_link_fn;          // Optional graph-unlink bridge
    };
    
    explicit FeedbackStorageService(const Config& config);
    ~FeedbackStorageService() noexcept;
    
    // Disable copy
    FeedbackStorageService(const FeedbackStorageService&) = delete;
    FeedbackStorageService& operator=(const FeedbackStorageService&) = delete;
    
    /**
     * @brief Register a feedback plugin
     * @param plugin Plugin to register
     */
    void registerPlugin(std::shared_ptr<FeedbackPlugin> plugin);
    
    /**
     * @brief Create new feedback entry
     * @param feedback Feedback to store (id will be generated if empty)
     * @return Stored feedback with generated ID, or nullopt if validation failed
     */
    std::optional<Feedback> createFeedback(Feedback feedback);
    
    /**
     * @brief Get feedback by ID
     * @param id Feedback ID
     * @return Feedback if found, nullopt otherwise
     */
    std::optional<Feedback> getFeedback(const std::string& id) const;
    
    /**
     * @brief List feedback with optional filters
     * @param filter Filter options
     * @return Vector of matching feedback entries
     */
    std::vector<Feedback> listFeedback(const FeedbackFilter& filter = FeedbackFilter{}) const;
    
    /**
     * @brief Update existing feedback
     * @param id Feedback ID
     * @param feedback Updated feedback data
     * @return true if updated successfully, false if not found
     */
    bool updateFeedback(const std::string& id, const Feedback& feedback);
    
    /**
     * @brief Delete feedback by ID
     * @param id Feedback ID
     * @return true if deleted, false if not found
     */
    bool deleteFeedback(const std::string& id);
    
    /**
     * @brief Get feedback for a specific adapter
     * @param adapter_id LoRA adapter ID
     * @param limit Maximum number of results
     * @return Vector of feedback entries
     */
    std::vector<Feedback> getFeedbackForAdapter(
        const std::string& adapter_id,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Get training-flagged feedback
     * @param adapter_id Optional adapter filter
     * @param limit Maximum number of results
     * @return Vector of feedback entries flagged for training
     */
    std::vector<Feedback> getTrainingFeedback(
        const std::optional<std::string>& adapter_id = std::nullopt,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Check if training should be triggered
     * @param adapter_id LoRA adapter ID
     * @return true if training should be triggered
     */
    bool shouldTriggerTraining(const std::string& adapter_id) const;
    
    /**
     * @brief Get statistics about feedback
     * @param adapter_id Optional adapter filter
     * @return JSON with statistics
     */
    json getStatistics(const std::optional<std::string>& adapter_id = std::nullopt) const;
    
    /**
     * @brief Get weighted training feedback
     * 
     * Returns feedback for training with weights applied.
     * Cached responses have lower weights to prevent overtraining.
     * 
     * @param adapter_id Optional adapter filter
     * @param limit Maximum number of results
     * @return Vector of feedback entries with training weights
     */
    std::vector<Feedback> getWeightedTrainingFeedback(
        const std::optional<std::string>& adapter_id = std::nullopt,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Calculate effective training batch size
     * 
     * Sums up training weights instead of counting entries.
     * Example: 100 direct responses (weight=1.0) = 100 effective samples
     *          100 cached responses (weight=0.4) = 40 effective samples
     * 
     * @param adapter_id LoRA adapter ID
     * @return Effective batch size considering weights
     */
        // ---------------------------------------------------------------------------
    // Callback bridges for graph edge persistence (stub #304)
    // ---------------------------------------------------------------------------

    /**
     * @brief Inject a real graph-edge creation backend.
     *
     * When set, createGraphLink() delegates to this function instead of the
     * log-only placeholder.  Calling with nullptr reverts to placeholder behavior.
     * Thread-safe: uses an internal mutex.
     */
    void setCreateGraphLinkFn(CreateGraphLinkFn fn);

    /**
     * @brief Inject a real graph-edge deletion backend.
     *
     * When set, removeGraphLink() delegates to this function instead of the
     * log-only placeholder.  Calling with nullptr reverts to placeholder behavior.
     * Thread-safe: uses an internal mutex.
     */
    void setRemoveGraphLinkFn(RemoveGraphLinkFn fn);

float calculateEffectiveBatchSize(const std::string& adapter_id) const;

private:
    Config config_;
    std::vector<std::shared_ptr<FeedbackPlugin>> plugins_;
    mutable std::mutex mutex_;
    
    // Helper methods
    std::string generateFeedbackId() const;
    std::string makeFeedbackKey(const std::string& id) const;
    bool createGraphLink(const std::string& feedback_id, const std::string& adapter_id);
    bool removeGraphLink(const std::string& feedback_id, const std::string& adapter_id);
    
    // Validation and processing
    bool runValidation(const Feedback& feedback) const;
    void runProcessing(Feedback& feedback);

    // Bridge callbacks for graph edge persistence (stub #304)
    CreateGraphLinkFn create_graph_link_fn_;
    RemoveGraphLinkFn remove_graph_link_fn_;
};

} // namespace lora
} // namespace llm
} // namespace themis
