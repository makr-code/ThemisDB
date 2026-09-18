/**
 * @file lora_feedback_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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


class FeedbackStorageService {
public:
    using CreateGraphLinkFn = std::function<bool(const std::string& feedback_pk,
                                                 const std::string& adapter_pk,
                                                 const std::string& edge_type)>;
    using RemoveGraphLinkFn = std::function<bool(const std::string& feedback_pk,
                                                 const std::string& adapter_pk,
                                                 const std::string& edge_type)>;

    struct Config {
        std::shared_ptr<RocksDBWrapper> db;              // RocksDB instance
        std::shared_ptr<GraphIndexManager> graph_index;  // Graph index for relationships
        std::string collection_name = "help_feedback";   // Collection name
        bool enable_graph_links = true;                  // Enable graph relationships
        CreateGraphLinkFn create_graph_link_fn;          // Optional graph-link bridge
        RemoveGraphLinkFn remove_graph_link_fn;          // Optional graph-unlink bridge
    };
    
    /**
     * @brief Feedback Storage Service.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit FeedbackStorageService(const Config& config);
    ~FeedbackStorageService() noexcept;
    
    // Disable copy
    FeedbackStorageService(const FeedbackStorageService&) = delete;
    FeedbackStorageService& operator=(const FeedbackStorageService&) = delete;
    
    /**
     * @brief Register Plugin.
     * @param[in] plugin Input parameter.
     */
    void registerPlugin(std::shared_ptr<FeedbackPlugin> plugin);
    
    /**
     * @brief Create Feedback.
     * @param[in] feedback Input parameter.
     * @return Return value.
     */
    std::optional<Feedback> createFeedback(Feedback feedback);
    
    /**
     * @brief Get Feedback.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<Feedback> getFeedback(const std::string& id) const;
    
    std::vector<Feedback> listFeedback(const FeedbackFilter& filter = FeedbackFilter{}) const;
    
    /**
     * @brief Update Feedback.
     * @param[in] id Input parameter.
     * @param[in] feedback Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateFeedback(const std::string& id, const Feedback& feedback);
    
    /**
     * @brief Delete Feedback.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteFeedback(const std::string& id);
    
    std::vector<Feedback> getFeedbackForAdapter(
        const std::string& adapter_id,
        size_t limit = 100
    ) const;
    
    std::vector<Feedback> getTrainingFeedback(
        const std::optional<std::string>& adapter_id = std::nullopt,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Should Trigger Training.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool shouldTriggerTraining(const std::string& adapter_id) const;
    
    json getStatistics(const std::optional<std::string>& adapter_id = std::nullopt) const;
    
    std::vector<Feedback> getWeightedTrainingFeedback(
        const std::optional<std::string>& adapter_id = std::nullopt,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Calculate Effective Batch Size.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    float calculateEffectiveBatchSize(const std::string& adapter_id) const;

    // ---------------------------------------------------------------------------
    // Callback bridges for graph edge persistence (stub #304)
    // ---------------------------------------------------------------------------

    /**
     * @brief Set Create Graph Link Fn.
     * @param[in] fn Input parameter.
     */
    void setCreateGraphLinkFn(CreateGraphLinkFn fn);

    /**
     * @brief Set Remove Graph Link Fn.
     * @param[in] fn Input parameter.
     */
    void setRemoveGraphLinkFn(RemoveGraphLinkFn fn);

private:
    Config config_;
    std::vector<std::shared_ptr<FeedbackPlugin>> plugins_;
    mutable std::mutex mutex_;
    
    // Helper methods
    /**
     * @brief Generate Feedback Id.
     * @return Return value.
     */
    std::string generateFeedbackId() const;
    /**
     * @brief Make Feedback Key.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::string makeFeedbackKey(const std::string& id) const;
    /**
     * @brief Create Graph Link.
     * @param[in] feedback_id Identifier of the feedback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool createGraphLink(const std::string& feedback_id, const std::string& adapter_id);
    /**
     * @brief Remove Graph Link.
     * @param[in] feedback_id Identifier of the feedback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool removeGraphLink(const std::string& feedback_id, const std::string& adapter_id);
    
    // Validation and processing
    /**
     * @brief Run Validation.
     * @param[in] feedback Input parameter.
     * @return True when the operation succeeds.
     */
    bool runValidation(const Feedback& feedback) const;
    /**
     * @brief Run Processing.
     * @param[in,out] feedback Input/output parameter.
     */
    void runProcessing(Feedback& feedback);

    // Bridge callbacks for graph edge persistence (stub #304)
    CreateGraphLinkFn create_graph_link_fn_;
    RemoveGraphLinkFn remove_graph_link_fn_;
};

} // namespace lora
} // namespace llm
} // namespace themis
