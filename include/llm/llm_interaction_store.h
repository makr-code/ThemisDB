/**
 * @file llm_interaction_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief LLM Interaction Store - persists and retrieves LLM conversation/interaction data
 * 
 * Features:
 * - Chain-of-Thought (CoT) storage: structured reasoning steps
 * - Prompt template versioning
 * - Token count and latency tracking
 * - Metadata (model version, feedback, etc.)
 * 
 * Storage: RocksDB with JSON serialization
 * Key format: "llm_interaction:{interaction_id}"
 */
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
        nlohmann::json toJson() const;
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

    /**
     * @brief Construct LLMInteractionStore
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     */
    explicit LLMInteractionStore(rocksdb::TransactionDB* db, 
                                   rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~LLMInteractionStore() = default;

    /**
     * @brief Store a new interaction
     * @param interaction Interaction to store (id will be generated if empty)
     * @return Stored interaction with generated ID
     */
    Interaction createInteraction(Interaction interaction);

    /**
     * @brief Retrieve interaction by ID
     * @param id Interaction ID
     * @return Interaction if found, nullopt otherwise
     */
    std::optional<Interaction> getInteraction(const std::string& id) const;

    /**
     * @brief List interactions with default options.
     * @return Vector of interactions.
     */
    std::vector<Interaction> listInteractions() const;
    /**
     * @brief List interactions with optional filters.
     * @param options List options (pagination, filters).
     * @return Vector of interactions.
     */
    std::vector<Interaction> listInteractions(const ListOptions& options) const;

    /**
     * @brief Get store statistics
     * @return Stats struct
     */
    Stats getStats() const;

    /**
     * @brief Delete interaction by ID
     * @param id Interaction ID
     * @return true if deleted, false if not found
     */
    bool deleteInteraction(const std::string& id);

    /**
     * @brief Clear all interactions
     */
    void clear();

    /**
     * @brief Update interaction metadata (e.g., for feedback or other extensions)
     * @param id Interaction ID
     * @param metadata_updates JSON object with metadata updates
     * @return true if updated, false if interaction not found
     */
    bool updateMetadata(const std::string& id, const nlohmann::json& metadata_updates);

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_; // nullptr = default CF

    static constexpr const char* KEY_PREFIX = "llm_interaction:";
    
    std::string makeKey(const std::string& id) const;
    std::string generateId() const;
};

} // namespace themis
