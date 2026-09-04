/**
 * @file lora_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/lora_config.h"
#include "storage/base_entity.h"
#include <vector>
#include <string>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Graph representation of LoRA adapter relationships
 * 
 * ThemisDB's multi-model approach:
 * - LoRA adapters are documents (BaseEntity)
 * - Relationships form a graph
 * - Metadata has vector embeddings
 * 
 * Graph Structure:
 * 
 *   [Base Model] --DERIVED_FROM--> [LoRA Adapter v1]
 *                                        |
 *                               TRAINED_ON |
 *                                        v
 *                                   [Training Dataset]
 *                                        |
 *                               RETRAINED |
 *                                        v
 *                               [LoRA Adapter v2] --SIMILAR_TO--> [Other Adapter]
 *                                        |
 *                               USED_IN  |
 *                                        v
 *                                   [Inference Session]
 */

/**
 * @brief Edge types for LoRA adapter graph
 */
enum class LoRAEdgeType {
    DERIVED_FROM,       // Adapter derived from base model or parent adapter
    TRAINED_ON,         // Adapter trained on dataset
    RETRAINED,          // Incremental training (v1 -> v2)
    FORKED_FROM,        // Adapter forked from another
    MERGED_INTO,        // Adapter merged into another
    USED_IN,            // Adapter used in inference session
    SIMILAR_TO,         // Semantic similarity between adapters
    DEPENDS_ON,         // Adapter depends on another (composition)
    VERSIONED_AS,       // Version relationship (v1 -> v2)
    DEPLOYED_TO,        // Adapter deployed to shard/node
    TRAINED_BY,         // Adapter trained by user/system
    FEEDBACK_FOR        // Feedback for specific adapter
};

/**
 * @brief Edge in LoRA adapter graph
 */
struct LoRAGraphEdge {
    std::string from_id;        // Source node (adapter, model, dataset, etc.)
    std::string to_id;          // Target node
    LoRAEdgeType edge_type;
    float weight = 1.0f;        // Edge weight (e.g., similarity score)
    json metadata;              // Additional metadata
    std::chrono::system_clock::time_point created_at;
    
    std::string getEdgeKey() const {
        return from_id + ":" + edgeTypeToString(edge_type) + ":" + to_id;
    }
    
    json toJSON() const {
        auto ts = std::chrono::system_clock::to_time_t(created_at);
        return json{
            {"from", from_id},
            {"to", to_id},
            {"type", edgeTypeToString(edge_type)},
            {"weight", weight},
            {"metadata", metadata},
            {"created_at", ts}
        };
    }
    
    static std::string edgeTypeToString(LoRAEdgeType type) {
        switch (type) {
            case LoRAEdgeType::DERIVED_FROM: return "DERIVED_FROM";
            case LoRAEdgeType::TRAINED_ON: return "TRAINED_ON";
            case LoRAEdgeType::RETRAINED: return "RETRAINED";
            case LoRAEdgeType::FORKED_FROM: return "FORKED_FROM";
            case LoRAEdgeType::MERGED_INTO: return "MERGED_INTO";
            case LoRAEdgeType::USED_IN: return "USED_IN";
            case LoRAEdgeType::SIMILAR_TO: return "SIMILAR_TO";
            case LoRAEdgeType::DEPENDS_ON: return "DEPENDS_ON";
            case LoRAEdgeType::VERSIONED_AS: return "VERSIONED_AS";
            case LoRAEdgeType::DEPLOYED_TO: return "DEPLOYED_TO";
            case LoRAEdgeType::TRAINED_BY: return "TRAINED_BY";
            case LoRAEdgeType::FEEDBACK_FOR: return "FEEDBACK_FOR";
            default: return "UNKNOWN";
        }
    }
};

/**
 * @brief Vector embedding for LoRA adapter metadata
 * 
 * Enables semantic search and similarity queries:
 * - Find similar adapters
 * - Recommend adapters for tasks
 * - Cluster adapters by functionality
 * - Detect duplicate/redundant adapters
 */
struct LoRAVectorEmbedding {
    std::string adapter_id;
    std::vector<float> embedding;      // Vector representation (e.g., 768-dim)
    std::string embedding_model;       // Model used to create embedding
    std::string source_text;           // Text used to generate embedding
    
    // Embedding sources
    enum class Source {
        DESCRIPTION,        // From adapter description
        HYPERPARAMETERS,    // From hyperparameters config
        TRAINING_DATA,      // From training data samples
        PERFORMANCE,        // From performance metrics
        COMBINED            // Combined from multiple sources
    };
    Source source = Source::DESCRIPTION;
    
    json toJSON() const {
        return json{
            {"adapter_id", adapter_id},
            {"embedding", embedding},
            {"embedding_model", embedding_model},
            {"source_text", source_text},
            {"source", static_cast<int>(source)},
            {"dimensions", embedding.size()}
        };
    }

    /**
     * @brief Deserialise a LoRAVectorEmbedding from its JSON representation.
     *
     * All fields are optional; missing fields retain their zero-value defaults.
     *
     * @param j  JSON object produced by toJSON().
     * @return   Populated LoRAVectorEmbedding.
     */
    static LoRAVectorEmbedding fromJSON(const json& j) {
        LoRAVectorEmbedding emb = {};
        if (j.contains("adapter_id")) {
          emb.adapter_id      = j["adapter_id"].get<std::string>();
        }
        if (j.contains("embedding_model")) {
          emb.embedding_model = j["embedding_model"].get<std::string>();
        }
        if (j.contains("source_text")) {
          emb.source_text     = j["source_text"].get<std::string>();
        }
        if (j.contains("embedding") && j["embedding"].is_array()) {
            emb.embedding = j["embedding"].get<std::vector<float>>();
        }
        if (j.contains("source") && j["source"].is_number_integer()) {
            int s = j["source"].get<int>();
            if (s >= 0 && s <= static_cast<int>(Source::COMBINED)) {
                emb.source = static_cast<Source>(s);
            }
        }
        return emb;
    }
};

/**
 * @brief Graph path for LoRA adapter
 * 
 * Represents the lineage and relationships of an adapter:
 * 
 * Example path:
 * llama-2-7b → themis_help_lora_v1 → themis_help_lora_v2 → production_deployment
 *                       ↓
 *                  training_dataset_20240101
 *                       ↓
 *                  feedback_collection
 */
struct LoRAGraphPath {
    std::string adapter_id = {};
    std::vector<LoRAGraphEdge> edges;  // Ordered path of edges
    
    /**
     * @brief Get all nodes in path
     */
    std::vector<std::string> getNodes() const {
        std::vector<std::string> nodes = {};

        if (!edges.empty()) {
            nodes.push_back(edges[0].from_id);
            for (const auto& edge : edges) {
                nodes.push_back(edge.to_id);
            }
        }
        return nodes;
    }
    
    /**
     * @brief Get path length (number of hops)
     */
    size_t length() const {
        return edges.size();
    }
    
    /**
     * @brief Get path as string
     */
    std::string toString() const {
        if (edges.empty()) {
          return adapter_id;
        }
        
        std::string result = edges[0].from_id;
        for (const auto& edge : edges) {
            result += " -[" + LoRAGraphEdge::edgeTypeToString(edge.edge_type) + "]-> ";
            result += edge.to_id;
        }
        return result;
    }
    
    json toJSON() const {
        json j;
        j["adapter_id"] = adapter_id;
        j["length"] = length();
        j["nodes"] = getNodes();
        j["edges"] = json::array();
        for (const auto& edge : edges) {
            j["edges"].push_back(edge.toJSON());
        }
        j["path_string"] = toString();
        return j;
    }
};

/**
 * @brief Enhanced adapter metadata with graph and vector support
 */
struct AdapterMetadataEnhanced : public AdapterMetadata {
    ~AdapterMetadataEnhanced() override = default;
    // Graph information
    std::vector<LoRAGraphEdge> edges;           // All edges connected to this adapter
    LoRAGraphPath lineage_path;                 // Path from base model to this adapter
    
    // Vector embeddings
    std::optional<LoRAVectorEmbedding> description_embedding;  // From description
    std::optional<LoRAVectorEmbedding> task_embedding;         // From task/use-case
    std::optional<LoRAVectorEmbedding> performance_embedding;  // From metrics
    
    // Related adapters (via graph or vector similarity)
    std::vector<std::string> similar_adapters;       // Similar by vector
    std::vector<std::string> derived_adapters;       // Derived from this one
    std::vector<std::string> dependency_adapters;    // Dependencies
    
    json toJSON() const {
        json j = AdapterMetadata::toJSON();
        
        // Add graph info
        j["edges"] = json::array();
        for (const auto& edge : edges) {
            j["edges"].push_back(edge.toJSON());
        }
        j["lineage"] = lineage_path.toJSON();
        
        // Add vector info
        if (description_embedding) {
            j["description_embedding"] = description_embedding->toJSON();
        }
        if (task_embedding) {
            j["task_embedding"] = task_embedding->toJSON();
        }
        if (performance_embedding) {
            j["performance_embedding"] = performance_embedding->toJSON();
        }
        
        // Add relationships
        j["similar_adapters"] = similar_adapters;
        j["derived_adapters"] = derived_adapters;
        j["dependency_adapters"] = dependency_adapters;
        
        return j;
    }
    
    static AdapterMetadataEnhanced fromJSON(const json& j) {
        AdapterMetadataEnhanced metadata;
        
        // Base metadata
        if (j.contains("adapter_id")) {
          metadata.adapter_id = j["adapter_id"];
        }
        if (j.contains("version")) {
          metadata.version = j["version"];
        }
        if (j.contains("base_model")) {
          metadata.base_model = j["base_model"];
        }
        if (j.contains("description")) {
          metadata.description = j["description"];
        }
        
        // Graph info
        if (j.contains("edges")) {
            for (const auto& edge_json : j["edges"]) {
                LoRAGraphEdge edge;
                edge.from_id = edge_json["from"];
                edge.to_id = edge_json["to"];
                // Parse edge type...
                metadata.edges.push_back(edge);
            }
        }
        
        // Vector info — parse per-role embeddings from their JSON representations
        if (j.contains("description_embedding") && j["description_embedding"].is_object()) {
            metadata.description_embedding = LoRAVectorEmbedding::fromJSON(j["description_embedding"]);
        }
        if (j.contains("task_embedding") && j["task_embedding"].is_object()) {
            metadata.task_embedding = LoRAVectorEmbedding::fromJSON(j["task_embedding"]);
        }
        if (j.contains("performance_embedding") && j["performance_embedding"].is_object()) {
            metadata.performance_embedding = LoRAVectorEmbedding::fromJSON(j["performance_embedding"]);
        }
        
        // Relationships
        if (j.contains("similar_adapters")) {
            metadata.similar_adapters = j["similar_adapters"].get<std::vector<std::string>>();
        }
        
        return metadata;
    }
};

/**
 * @brief Enhanced adapter info with graph path and vectors
 */
struct AdapterInfoEnhanced : public AdapterInfo {
    ~AdapterInfoEnhanced() override = default;
    // Graph path
    LoRAGraphPath graph_path;
    
    // Vector embeddings
    std::vector<LoRAVectorEmbedding> embeddings;
    
    // Graph metrics
    int incoming_edges = 0;      // How many adapters derived from this
    int outgoing_edges = 0;      // How many dependencies
    int path_length = 0;         // Distance from base model
    float centrality = 0.0f;     // Graph centrality score
    
    json toJSON() const {
        json j = AdapterInfo::toJSON();
        
        j["graph_path"] = graph_path.toJSON();
        j["embeddings"] = json::array();
        for (const auto& emb : embeddings) {
            j["embeddings"].push_back(emb.toJSON());
        }
        
        j["graph_metrics"] = {
            {"incoming_edges", incoming_edges},
            {"outgoing_edges", outgoing_edges},
            {"path_length", path_length},
            {"centrality", centrality}
        };
        
        return j;
    }
};

} // namespace lora
} // namespace llm
} // namespace themis

