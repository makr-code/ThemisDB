/**
 * @file llm_interaction_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <nlohmann/json.hpp>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

class LLMInteractionStore {
public:
    struct Interaction {
        std::string id;                        // UUID or generated ID
        std::string prompt_template_id;        // Reference to prompt template version
        std::string prompt;                    // Actual prompt sent
        std::vector<std::string> reasoning_chain; // CoT steps
        std::string response;                  // Final LLM response
        std::string model_version;             // e.g., "gpt-4o-mini"
        int64_t timestamp_ms = 0;              // Creation timestamp
        int latency_ms = 0;                    // Response latency
        int token_count = 0;                   // Total tokens used
        nlohmann::json metadata;               // Additional fields (feedback, user_id, etc.)

        // Serialization
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static Interaction fromJson(const nlohmann::json& j);
    };

    struct ListOptions {
        size_t limit = 100;                    // Max interactions to return
        std::optional<std::string> start_after_id; // Pagination cursor
        std::optional<std::string> filter_model;   // Filter by model version
        std::optional<int64_t> since_timestamp_ms; // Filter by time
    };

    struct Stats {
        size_t total_interactions = 0;
        int64_t total_tokens = 0;
        double avg_latency_ms = 0.0;
        size_t total_size_bytes = 0;
    };

    explicit LLMInteractionStore(rocksdb::TransactionDB* db, 
                                   rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~LLMInteractionStore() = default;

    /**
     * @brief Create Interaction.
     * @param[in] interaction Input parameter.
     * @return Return value.
     */
    Interaction createInteraction(Interaction interaction);

    /**
     * @brief Get Interaction.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<Interaction> getInteraction(const std::string& id) const;

    /**
     * @brief List Interactions.
     * @return Return value.
     */
    std::vector<Interaction> listInteractions() const;
    /**
     * @brief List Interactions.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<Interaction> listInteractions(const ListOptions& options) const;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Delete Interaction.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteInteraction(const std::string& id);

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Update Metadata.
     * @param[in] id Input parameter.
     * @param[in] metadata_updates Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateMetadata(const std::string& id, const nlohmann::json& metadata_updates);

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_; // nullptr = default CF

    static constexpr const char* KEY_PREFIX = "llm_interaction:";
    
    /**
     * @brief Make Key.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::string makeKey(const std::string& id) const;
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    std::string generateId() const;
};

} // namespace themis
