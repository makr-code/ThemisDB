/**
 * @file aql_train_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <nlohmann/json.hpp>
#include "query/aql_parser.h"
#include "llm/adapter_registry.h"
#include "llm/gguf_st_adapter.h"

namespace themis::llm {

// ============================================================================
// TRAIN Statement AST Nodes
// ============================================================================

/// Extended training configuration for TRAIN statement
/// Extends the base TrainingConfig with TRAIN-specific options
struct TrainStatementConfig : public TrainingConfig {
    ~TrainStatementConfig() override = default;
    // Base model configuration (not in base TrainingConfig)
    std::string base_model_name;               // e.g., "mistral-7b", "llama-3-8b"
    
    // Output format configuration
    GGUFSTConfig::SizeMode size_mode = GGUFSTConfig::SizeMode::COMPACT;
    GGUFSTConfig::QuantizationType quantization_type = GGUFSTConfig::QuantizationType::Q4_K_M;
    bool compress_manifest = true;
    bool embed_safetensors = false;
    
    // Security and versioning
    bool sign_adapter = true;
    std::string adapter_version = "1.0.0";
    std::map<std::string, std::string> custom_metadata;  // Additional metadata
    
    // Quality and validation
    double validation_split = 0.1;
    bool shuffle = true;
    int random_seed = 42;
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TrainStatementConfig fromJSON(const nlohmann::json& j);
};

/// Graph context enrichment configuration
struct GraphContextConfig {
    std::vector<std::string> relationships;     // e.g., ["CITES", "REFERENCES"]
    int max_depth = 2;                          // Maximum traversal depth
    std::string direction = "BOTH";             // "OUTBOUND", "INBOUND", "BOTH"
    int max_nodes = 100;                        // Maximum nodes to include
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static GraphContextConfig fromJSON(const nlohmann::json& j);
};

/// Vector similarity enrichment configuration
struct VectorSimilarityConfig {
    std::string field;                          // Field containing embedding
    double threshold = 0.8;                     // Similarity threshold
    int top_k = 10;                             // Top K similar documents
    std::string metric = "cosine";              // "cosine", "euclidean", "dot"
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static VectorSimilarityConfig fromJSON(const nlohmann::json& j);
};

/// Relational join enrichment configuration
struct RelationalJoinConfig {
    std::string collection;                     // Collection to join with
    std::string local_field;                    // Field in current collection
    std::string foreign_field;                  // Field in foreign collection
    std::string join_type = "LEFT";             // "LEFT", "INNER", "RIGHT"
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static RelationalJoinConfig fromJSON(const nlohmann::json& j);
};

/// Multi-model enrichment configuration
struct MultiModelEnrichment {
    std::optional<GraphContextConfig> graph_context;
    std::optional<VectorSimilarityConfig> vector_similarity;
    std::vector<RelationalJoinConfig> relational_joins;
    
    bool hasEnrichment() const {
        return graph_context.has_value() || 
               vector_similarity.has_value() || 
               !relational_joins.empty();
    }
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static MultiModelEnrichment fromJSON(const nlohmann::json& j);
};

/// Distributed training configuration
struct AQLDistributedTrainingConfig {
    bool enabled = false;
    std::string sync_strategy = "ALL_REDUCE";   // "ALL_REDUCE", "PARAMETER_SERVER"
    std::string coordinator_shard;              // Shard to coordinate training
    std::vector<std::string> participant_shards; // Shards to participate
    int sync_frequency = 1;                     // Sync every N batches
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AQLDistributedTrainingConfig fromJSON(const nlohmann::json& j);
};

/// TRAIN ADAPTER statement AST node
struct TrainAdapterStmt {
    // Adapter identification
    std::string adapter_id;                     // Unique adapter identifier
    
    // Data source (AQL query)
    std::string source_collection;              // FROM collection
    std::shared_ptr<query::Query> data_query;   // Full AQL query for data selection
    
    // Enrichment configuration
    MultiModelEnrichment enrichment;
    
    // Training configuration
    TrainStatementConfig config;
    
    // Distributed training (optional)
    AQLDistributedTrainingConfig distributed;
    
    // Output
    std::string output_path;                    // Where to save adapter
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TrainAdapterStmt fromJSON(const nlohmann::json& j);
};

/// DEPLOY ADAPTER statement AST node
struct DeployAdapterStmt {
    std::string adapter_id;                     // Adapter to deploy
    std::vector<std::string> target_shards;     // Shards to deploy to
    std::string strategy = "CO_LOCATED";        // "CO_LOCATED", "REPLICATED", "LOAD_BALANCED"
    bool validate_compatibility = true;
    bool verify_signature = true;
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static DeployAdapterStmt fromJSON(const nlohmann::json& j);
};

/// VERIFY ADAPTER statement AST node
struct VerifyAdapterStmt {
    std::string adapter_id;                     // Adapter to verify
    bool check_signature = true;
    bool check_manifest = true;
    bool check_safetensors_match = false;
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static VerifyAdapterStmt fromJSON(const nlohmann::json& j);
};

/// LIST ADAPTERS statement AST node
struct ListAdaptersStmt {
    std::optional<std::string> base_model;      // Filter by base model
    std::optional<std::string> domain;          // Filter by domain
    std::optional<AdapterMetadata::Status> status;  // Filter by status
    std::string order_by = "created_at";        // Sort field
    bool descending = true;
    int limit = 100;
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ListAdaptersStmt fromJSON(const nlohmann::json& j);
};

// ============================================================================
// AQL TRAIN Parser Extension
// ============================================================================

/// Parser for TRAIN, DEPLOY, VERIFY, LIST ADAPTERS statements
class AQLTrainParser {
public:
    AQLTrainParser() = default;
    
    /**
     * @brief Parse TRAIN ADAPTER statement Syntax: TRAIN ADAPTER <id> FROM <collection> [WHERE .
     * @param[in] aql Input parameter.
     * @return Return value.
     * @details ..] [USING GRAPH_CONTEXT(...)] [USING VECTOR_SIMILARITY(...)] [USING RELATIONAL_JOIN(...)] [DISTRIBUTED] WITH <config>
     */
    std::shared_ptr<TrainAdapterStmt> parseTrainAdapter(const std::string& aql);
    
    /**
     * @brief Parse DEPLOY ADAPTER statement Syntax: DEPLOY ADAPTER <id> TO SHARD '<shard>' [, '<shard2>'] [WITH strategy = '.
     * @param[in] aql Input parameter.
     * @return Return value.
     * @details ..', validate_compatibility = TRUE]
     */
    std::shared_ptr<DeployAdapterStmt> parseDeployAdapter(const std::string& aql);
    
    /**
     * @brief Parse VERIFY ADAPTER statement Syntax: VERIFY ADAPTER <id> [CHECK signature, manifest, safetensors_match]
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    std::shared_ptr<VerifyAdapterStmt> parseVerifyAdapter(const std::string& aql);
    
    /**
     * @brief Parse LIST ADAPTERS statement Syntax: LIST ADAPTERS [WHERE base_model = '.
     * @param[in] aql Input parameter.
     * @return Return value.
     * @details ..'] [ORDER BY created_at DESC] [LIMIT 100]
     */
    std::shared_ptr<ListAdaptersStmt> parseListAdapters(const std::string& aql);
    
    /// Detect statement type from AQL string
    enum class StatementType {
        TRAIN_ADAPTER,
        DEPLOY_ADAPTER,
        VERIFY_ADAPTER,
        LIST_ADAPTERS,
        UNKNOWN
    };
    
    /**
     * @brief TBD: Describe detectStatementType.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    StatementType detectStatementType(const std::string& aql) const;
    
private:
    /**
     * @brief Helper methods for parsing
     * @param[in] with_clause Input parameter.
     * @return Return value.
     */
    TrainStatementConfig parseTrainingConfig(const std::string& with_clause);
    /**
     * @brief TBD: Describe parseEnrichment.
     * @param[in] using_clauses Input parameter.
     * @return Return value.
     */
    MultiModelEnrichment parseEnrichment(const std::string& using_clauses);
    /**
     * @brief TBD: Describe parseGraphContext.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    GraphContextConfig parseGraphContext(const std::string& args);
    /**
     * @brief TBD: Describe parseVectorSimilarity.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    VectorSimilarityConfig parseVectorSimilarity(const std::string& args);
    /**
     * @brief TBD: Describe parseRelationalJoin.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    RelationalJoinConfig parseRelationalJoin(const std::string& args);
    /**
     * @brief TBD: Describe parseDistributed.
     * @param[in] aql Input parameter.
     * @return Return value.
     */
    AQLDistributedTrainingConfig parseDistributed(const std::string& aql);
    
    /**
     * @brief Tokenization helpers
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::vector<std::string> tokenize(const std::string& input);
    /**
     * @brief TBD: Describe extractClause.
     * @param[in] input Input parameter.
     * @param[in] keyword Input parameter.
     * @return Return value.
     */
    std::string extractClause(const std::string& input, const std::string& keyword);
    std::map<std::string, std::string> parseKeyValuePairs(const std::string& input);
    
    /**
     * @brief Validation
     * @param[in] name Input parameter.
     */
    void validateAdapterName(const std::string& name);
    /**
     * @brief TBD: Describe validateBaseModel.
     * @param[in] model Input parameter.
     */
    void validateBaseModel(const std::string& model);
    /**
     * @brief TBD: Describe validateConfig.
     * @param[in] config Input parameter.
     */
    void validateConfig(const TrainStatementConfig& config);
};

// ============================================================================
// Training Query Builder (Fluent API)
// ============================================================================

/// Builder for constructing TRAIN ADAPTER statements programmatically
class TrainingQueryBuilder {
public:
    TrainingQueryBuilder() = default;
    
    /**
     * @brief Adapter configuration
     * @param[in] id Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& adapter(const std::string& id);
    /**
     * @brief TBD: Describe from.
     * @param[in] collection Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& from(const std::string& collection);
    /**
     * @brief TBD: Describe where.
     * @param[in] condition Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& where(const std::string& condition);
    
    /**
     * @brief Enrichment configuration
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& withGraphContext(const GraphContextConfig& config);
    /**
     * @brief TBD: Describe withVectorSimilarity.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& withVectorSimilarity(const VectorSimilarityConfig& config);
    /**
     * @brief TBD: Describe withRelationalJoin.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& withRelationalJoin(const RelationalJoinConfig& config);
    
    /**
     * @brief Training configuration
     * @param[in] model Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& baseModel(const std::string& model);
    /**
     * @brief TBD: Describe loraRank.
     * @param[in] rank Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& loraRank(int rank);
    /**
     * @brief TBD: Describe epochs.
     * @param[in] n Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& epochs(int n);
    /**
     * @brief TBD: Describe batchSize.
     * @param[in] size Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& batchSize(int size);
    /**
     * @brief TBD: Describe learningRate.
     * @param[in] lr Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& learningRate(double lr);
    /**
     * @brief TBD: Describe quantization.
     * @param[in] quant Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& quantization(GGUFSTConfig::QuantizationType quant);
    /**
     * @brief TBD: Describe sizeMode.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& sizeMode(GGUFSTConfig::SizeMode mode);
    /**
     * @brief TBD: Describe signAdapter.
     * @param[in] sign Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& signAdapter(bool sign);
    
    /**
     * @brief Distributed training
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& distributed(const AQLDistributedTrainingConfig& config);
    
    /**
     * @brief Output
     * @param[in] path Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& outputPath(const std::string& path);
    
    /**
     * @brief Build the final statement
     * @return Return value.
     */
    std::shared_ptr<TrainAdapterStmt> build();
    
    /**
     * @brief Generate AQL string
     * @return Return value.
     */
    std::string toAQL() const;
    
private:
    TrainAdapterStmt stmt_;
    std::string where_clause_;
};

} // namespace themis::llm

