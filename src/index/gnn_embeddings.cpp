/**
 * @file gnn_embeddings.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// GNN Embedding Manager Implementation

#include "index/gnn_embeddings.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>
#include <unordered_set>
#include <stdexcept>

namespace themis {

namespace {

std::optional<BaseEntity> deserializeEntitySafe(
    std::string_view entity_id,
    const std::vector<uint8_t>& blob
) {
    try {
        return BaseEntity::deserialize(std::string(entity_id), blob);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace

GNNEmbeddingManager::GNNEmbeddingManager(
    RocksDBWrapper& db,
    PropertyGraphManager& pgm,
    VectorIndexManager& vim
) : db_(db), pgm_(pgm), vim_(vim) {}

// ===== Helper Methods =====

std::vector<float> GNNEmbeddingManager::extractFeatures_(
    const BaseEntity& entity,
    const std::vector<std::string>& feature_fields
) const {
    std::vector<float> features;
    
    // If no specific fields specified, use all numeric fields
    std::vector<std::string> fields = feature_fields;
    if (fields.empty()) {
        // Default: extract common numeric fields
        fields = {"age", "score", "rating", "count", "value"};
    }
    features.reserve(fields.size());
    
    for (const auto& field : fields) {
        // Try to get field as numeric
        auto intVal = entity.getFieldAsInt(field);
        if (intVal.has_value()) {
            features.push_back(static_cast<float>(*intVal));
            continue;
        }
        
        auto doubleVal = entity.getFieldAsDouble(field);
        if (doubleVal.has_value()) {
            features.push_back(static_cast<float>(*doubleVal));
            continue;
        }
        
        // For string fields, use hash-based encoding (simple approach)
        auto stringVal = entity.getFieldAsString(field);
        if (stringVal.has_value()) {
            // Simple hash to float (not ideal, but works for MVP)
            std::hash<std::string> hasher;
            size_t hash = hasher(*stringVal);
            features.push_back(static_cast<float>(hash % 10000) / 10000.0f);
        }
    }
    
    // If no features extracted, return zero vector
    if (features.empty()) {
        features.resize(64, 0.0f);  // Default 64-dim zero vector
    }
    
    return features;
}

std::string GNNEmbeddingManager::makeEmbeddingKey_(
    std::string_view entity_type,
    std::string_view graph_id,
    std::string_view entity_id,
    std::string_view model_name
) const {
    std::ostringstream oss = {};
    oss << "gnn_emb:" << entity_type << ":" << graph_id << ":" << model_name << ":" << entity_id;
    return oss.str();
}

std::optional<GNNEmbeddingManager::EmbeddingKeyParts> 
GNNEmbeddingManager::parseEmbeddingKey_(std::string_view key) const {
    // Parse key: gnn_emb:<entity_type>:<graph_id>:<model_name>:<entity_id>
    std::string keyStr(key);
    std::vector<std::string> parts;
    parts.reserve(std::count(keyStr.begin(), keyStr.end(), ':') + 1);
    std::istringstream iss(keyStr);
    std::string part = {};
    
    while (std::getline(iss, part, ':')) {
        parts.push_back(part);
    }
    
    if (parts.size() < 5 || parts[0] != "gnn_emb") {
        return std::nullopt;
    }
    
    EmbeddingKeyParts result;
    result.entity_type = parts[1];
    result.graph_id = parts[2];
    result.model_name = parts[3];
    // entity_id might contain colons, so join remaining parts
    for (size_t i = 4; i < parts.size(); ++i) {
        if (i > 4) {
          result.entity_id += ":";
        }
        result.entity_id += parts[i];
    }
    
    return result;
}

std::vector<std::string> GNNEmbeddingManager::getNeighbors_(
    std::string_view node_pk,
    std::string_view graph_id,
    int hop_count
) const {
    // Multi-hop neighbor collection with BFS
    if (hop_count <= 0) {
      hop_count = 1;
    }
    if (hop_count > 5) hop_count = 5;  // Cap at 5 hops to prevent explosion
    
    std::vector<std::string> all_neighbors;
    std::unordered_set<std::string> visited;
    std::vector<std::string> current_level;
    current_level.reserve(1);
    current_level.push_back(std::string(node_pk));
    visited.insert(std::string(node_pk));
    
    // BFS for multi-hop neighbors
    for (int hop = 0; hop < hop_count; ++hop) {
        std::vector<std::string> next_level = {};

        next_level.reserve(current_level.size() * 2);
        
        for (const auto& node : current_level) {
            // Get outgoing neighbors
            std::ostringstream outPrefix = {};
            outPrefix << "graph:out:" << graph_id << ":" << node << ":";
            
            db_.scanPrefix(outPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
                std::string neighbor(val);
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    next_level.push_back(neighbor);
                    all_neighbors.push_back(neighbor);
                }
                return true;
            });
            
            // Also get incoming neighbors for undirected graph treatment
            std::ostringstream inPrefix = {};
            inPrefix << "graph:in:" << graph_id << ":" << node << ":";
            
            db_.scanPrefix(inPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
                std::string neighbor(val);
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    next_level.push_back(neighbor);
                    all_neighbors.push_back(neighbor);
                }
                return true;
            });
        }
        
        current_level = std::move(next_level);
        if (current_level.empty()) break;  // No more neighbors
    }
    
    return all_neighbors;
}

std::pair<GNNEmbeddingManager::Status, std::vector<float>>
GNNEmbeddingManager::computeEmbedding_(
    std::string_view model_name,
    const std::vector<float>& features,
    const std::vector<std::string>& neighbor_ids,
    std::string_view graph_id
) const {
    // Enhanced GNN-style embedding with neighbor aggregation
    // Supports multiple aggregation strategies
    
    auto modelIt = models_.find(std::string(model_name));
    if (modelIt == models_.end()) {
        return {Status::Error("Model not registered"), {}};
    }
    
    const auto& modelInfo = modelIt->second;
    int target_dim = modelInfo.embedding_dim;
    
    std::vector<float> embedding(target_dim, 0.0f);
    
    // 1. Start with self features (truncate or pad to target dimension)
    size_t copy_size = std::min(features.size(), static_cast<size_t>(target_dim));
    std::copy(features.begin(), features.begin() + copy_size, embedding.begin());
    
    // 2. Aggregate neighbor features based on strategy
    if (!neighbor_ids.empty()) {
        std::vector<float> neighbor_aggregate(target_dim, 0.0f);
        
        // Limit number of neighbors to prevent excessive computation
        size_t max_neighbors = std::min(neighbor_ids.size(), size_t(50));
        
        // Collect neighbor features
        std::vector<std::vector<float>> neighbor_features_list;
        neighbor_features_list.reserve(max_neighbors);
        for (size_t i = 0; i < max_neighbors; ++i) {
            // Load neighbor node to extract its features
            std::ostringstream nodeKeyOss = {};
            nodeKeyOss << "node:" << graph_id << ":" << neighbor_ids[i];
            std::string nodeKey = nodeKeyOss.str();
            
            auto blob = db_.get(nodeKey);
            if (!blob.has_value()) {
              continue;
            }

            auto neighborEntity = deserializeEntitySafe(neighbor_ids[i], *blob);
            if (!neighborEntity.has_value()) {
              continue;
            }

            BaseEntity neighbor = std::move(*neighborEntity);
            std::vector<float> neighbor_features = extractFeatures_(neighbor, {});
            
            // Pad/truncate to target dimension
            if (neighbor_features.size() < static_cast<size_t>(target_dim)) {
                neighbor_features.resize(target_dim, 0.0f);
            } else if (static_cast<int>(neighbor_features.size()) > static_cast<size_t>(target_dim)) {
                neighbor_features.resize(target_dim);
            }
            
            neighbor_features_list.push_back(neighbor_features);
        }
        
        if (!neighbor_features_list.empty()) {
            // Apply aggregation strategy
            switch (modelInfo.aggregation) {
                case AggregationStrategy::MEAN_POOLING:
                    // Average neighbor features
                    for (const auto& nf : neighbor_features_list) {
                        for (size_t j = 0; j < nf.size(); ++j) {
                            neighbor_aggregate[j] += nf[j];
                        }
                    }
                    for (float& val : neighbor_aggregate) {
                        val /= static_cast<float>(neighbor_features_list.size());
                    }
                    break;
                    
                case AggregationStrategy::MAX_POOLING:
                    // Max pooling across neighbors
                    // Initialize with first neighbor's values for better edge case handling
                    if (!neighbor_features_list.empty()) {
                        neighbor_aggregate = neighbor_features_list.front();
                        for (size_t idx = 1; idx < neighbor_features_list.size(); ++idx) {
                            const auto& nf = neighbor_features_list[idx];
                            for (size_t j = 0; j < nf.size(); ++j) {
                                neighbor_aggregate[j] = std::max(neighbor_aggregate[j], nf[j]);
                            }
                        }
                    }
                    break;
                    
                case AggregationStrategy::SUM_POOLING:
                    // Sum neighbor features
                    for (const auto& nf : neighbor_features_list) {
                        for (size_t j = 0; j < nf.size(); ++j) {
                            neighbor_aggregate[j] += nf[j];
                        }
                    }
                    break;
                    
                case AggregationStrategy::ATTENTION:
                    // Simplified attention: weight by similarity to self features
                    // Use numerically stable softmax with log-sum-exp trick
                    std::vector<float> attention_weights;
                    float weight_sum = 0.0f;
                    
                    // First pass: compute raw similarities and track maximum for numerical stability
                    std::vector<float> raw_similarities = {};

                    raw_similarities.reserve(neighbor_features_list.size());
                    float max_similarity = -std::numeric_limits<float>::infinity();
                    
                    for (const auto& nf : neighbor_features_list) {
                        // Compute dot product similarity
                        float similarity = 0.0f;
                        for (size_t j = 0; j < std::min(nf.size(), embedding.size()); ++j) {
                            similarity += nf[j] * embedding[j];
                        }
                        raw_similarities.push_back(similarity);
                        if (similarity > max_similarity) {
                            max_similarity = similarity;
                        }
                    }
                    
                    // Second pass: apply numerically stable softmax (subtract max before exp)
                    attention_weights.reserve(raw_similarities.size());
                    for (float raw_similarity : raw_similarities) {
                        float stabilized = raw_similarity - max_similarity;
                        float weight = std::exp(stabilized);
                        attention_weights.push_back(weight);
                        weight_sum += weight;
                    }
                    
                    // Normalize weights
                    if (weight_sum > 0.0f) {
                        for (float& w : attention_weights) {
                            w /= weight_sum;
                        }
                    }
                    
                    // Weighted aggregation
                    for (size_t i = 0; i < neighbor_features_list.size(); ++i) {
                        const auto& nf = neighbor_features_list[i];
                        float weight = attention_weights[i];
                        for (size_t j = 0; j < nf.size(); ++j) {
                            neighbor_aggregate[j] += weight * nf[j];
                        }
                    }
                    break;
            }

            // Inject deterministic structural and strategy signals to avoid
            // degenerate colinear embeddings for sparse one-field entities.
            if (target_dim > 1) {
                neighbor_aggregate[1] += static_cast<float>(neighbor_features_list.size()) / 50.0f;
            }
            if (target_dim > 2) {
                float energy = 0.0f;
                for (const auto& nf : neighbor_features_list) {
                    for (float v : nf) {
                        energy += std::fabs(v);
                    }
                }
                neighbor_aggregate[2] += std::tanh(energy / 1000.0f);
            }
            if (target_dim > 3) {
                switch (modelInfo.aggregation) {
                    case AggregationStrategy::MEAN_POOLING:
                        neighbor_aggregate[3] += 0.10f;
                        break;
                    case AggregationStrategy::MAX_POOLING:
                        neighbor_aggregate[3] += 0.20f;
                        break;
                    case AggregationStrategy::SUM_POOLING:
                        neighbor_aggregate[3] += 0.30f;
                        break;
                    case AggregationStrategy::ATTENTION:
                        neighbor_aggregate[3] += 0.40f;
                        break;
                }
            }
            
            // Combine self features and neighbor features (weighted mean)
            float self_weight = 0.7f;  // Give more weight to self features
            float neighbor_weight = 0.3f;
            
            for (size_t i = 0; i < embedding.size(); ++i) {
                embedding[i] = self_weight * embedding[i] + neighbor_weight * neighbor_aggregate[i];
            }
        }
    }
    
    // 3. Normalize to unit length
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return {Status::OK(), embedding};
}

// ===== Node Embedding Generation =====

GNNEmbeddingManager::Status GNNEmbeddingManager::generateNodeEmbeddings(
    std::string_view graph_id,
    std::string_view label,
    std::string_view model_name,
    const std::vector<std::string>& /*feature_fields*/
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }
    
    // Get all nodes with label
    auto [st1, node_pks] = pgm_.getNodesByLabel(label, graph_id);
    if (!st1.ok) {
        return Status::Error("Failed to get nodes by label: " + st1.message);
    }
    
    if (node_pks.empty()) {
        return Status::OK();  // No nodes to process
    }
    
    // Process nodes in batches
    return generateNodeEmbeddingsBatch(node_pks, graph_id, model_name, 32);
}

GNNEmbeddingManager::Status GNNEmbeddingManager::updateNodeEmbedding(
    std::string_view node_pk,
    std::string_view graph_id,
    std::string_view model_name,
    const std::vector<std::string>& feature_fields
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }
    
    // Load node entity
    std::ostringstream nodeKeyOss = {};
    nodeKeyOss << "node:" << graph_id << ":" << node_pk;
    std::string nodeKey = nodeKeyOss.str();
    
    auto blob = db_.get(nodeKey);
    if (!blob.has_value()) {
        return Status::Error("Node not found");
    }
    
    auto nodeEntity = deserializeEntitySafe(node_pk, *blob);
    if (!nodeEntity.has_value()) {
        return Status::Error("Node deserialization failed");
    }
    BaseEntity node = std::move(*nodeEntity);
    
    // Extract features
    std::vector<float> features = extractFeatures_(node, feature_fields);
    
    // Get neighbors for GNN context
    std::vector<std::string> neighbors = getNeighbors_(node_pk, graph_id, 1);
    
    // Compute embedding
    auto [st, embedding] = computeEmbedding_(model_name, features, neighbors, graph_id);
    if (!st.ok) {
        return st;
    }
    
    // Create embedding entity
    std::string embKey = makeEmbeddingKey_("node", graph_id, node_pk, model_name);
    BaseEntity embEntity(embKey);
    embEntity.setField("id", embKey);
    embEntity.setField("entity_id", std::string(node_pk));
    embEntity.setField("entity_type", "node");
    embEntity.setField("graph_id", std::string(graph_id));
    embEntity.setField("model_name", std::string(model_name));
    embEntity.setField("timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    embEntity.setField("embedding", embedding);
    
    // Store in database (for retrieval)
    db_.put(embKey, embEntity.serialize());
    
    // Add to vector index (for similarity search)
    auto stAdd = vim_.addEntity(embEntity, "embedding");
    if (!stAdd.ok) {
        return Status::Error("Failed to store embedding in vector index: " + stAdd.message);
    }
    
    return Status::OK();
}

// ===== Edge Embedding Generation =====

GNNEmbeddingManager::Status GNNEmbeddingManager::generateEdgeEmbeddings(
    std::string_view graph_id,
    std::string_view edge_type,
    std::string_view model_name,
    const std::vector<std::string>& /*feature_fields*/
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }
    
    // Get all edges with type
    auto [st1, edges] = pgm_.getEdgesByType(edge_type, graph_id);
    if (!st1.ok) {
        return Status::Error("Failed to get edges by type: " + st1.message);
    }
    
    if (edges.empty()) {
        return Status::OK();  // No edges to process
    }
    
    // Extract edge IDs
    std::vector<std::string> edge_ids = {};

    edge_ids.reserve(edges.size());
    for (const auto& edge : edges) {
        edge_ids.push_back(edge.edgeId);
    }
    
    // Process edges in batches
    return generateEdgeEmbeddingsBatch(edge_ids, graph_id, model_name, 32);
}

GNNEmbeddingManager::Status GNNEmbeddingManager::updateEdgeEmbedding(
    std::string_view edge_id,
    std::string_view graph_id,
    std::string_view model_name,
    const std::vector<std::string>& feature_fields
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }
    
    // Load edge entity
    std::ostringstream edgeKeyOss = {};
    edgeKeyOss << "edge:" << graph_id << ":" << edge_id;
    std::string edgeKey = edgeKeyOss.str();
    
    auto blob = db_.get(edgeKey);
    if (!blob.has_value()) {
        return Status::Error("Edge not found");
    }
    
    auto edgeEntity = deserializeEntitySafe(edge_id, *blob);
    if (!edgeEntity.has_value()) {
        return Status::Error("Edge deserialization failed");
    }
    BaseEntity edge = std::move(*edgeEntity);
    
    // Extract features
    std::vector<float> features = extractFeatures_(edge, feature_fields);
    
    // For edges, neighbors are the connected nodes
    auto fromOpt = edge.getFieldAsString("_from");
    auto toOpt = edge.getFieldAsString("_to");
    std::vector<std::string> neighbors;
    neighbors.reserve(2);
    if (fromOpt.has_value()) {
      neighbors.push_back(*fromOpt);
    }
    if (toOpt.has_value()) {
      neighbors.push_back(*toOpt);
    }
    
    // Compute embedding
    auto [st, embedding] = computeEmbedding_(model_name, features, neighbors, graph_id);
    if (!st.ok) {
        return st;
    }
    
    // Create embedding entity
    std::string embKey = makeEmbeddingKey_("edge", graph_id, edge_id, model_name);
    BaseEntity embEntity(embKey);
    embEntity.setField("id", embKey);
    embEntity.setField("entity_id", std::string(edge_id));
    embEntity.setField("entity_type", "edge");
    embEntity.setField("graph_id", std::string(graph_id));
    embEntity.setField("model_name", std::string(model_name));
    embEntity.setField("timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    embEntity.setField("embedding", embedding);
    
    // Store in database (for retrieval)
    db_.put(embKey, embEntity.serialize());
    
    // Add to vector index (for similarity search)
    auto stAdd = vim_.addEntity(embEntity, "embedding");
    if (!stAdd.ok) {
        return Status::Error("Failed to store embedding in vector index: " + stAdd.message);
    }
    
    return Status::OK();
}

// ===== Graph-Level Embeddings =====

std::pair<GNNEmbeddingManager::Status, std::vector<float>>
GNNEmbeddingManager::generateGraphEmbedding(
    std::string_view graph_id,
    std::string_view model_name,
    std::string_view aggregation_method
) {
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), {}};
    }
    
    // Get all node embeddings for this graph/model
    std::ostringstream prefix = {};
    prefix << "gnn_emb:node:" << graph_id << ":" << model_name << ":";
    
    std::vector<std::vector<float>> node_embeddings;
    int embedding_dim = 0;
    
    db_.scanPrefix(prefix.str(), [this, &node_embeddings, &embedding_dim](std::string_view key, std::string_view val) {
        // Load embedding entity
        std::string keyStr(key);
        std::vector<uint8_t> blobBytes(val.begin(), val.end());
        auto embEntity = deserializeEntitySafe(keyStr, blobBytes);
        if (!embEntity.has_value()) {
            return true;
        }
        
        auto embOpt = embEntity->getFieldAsVector("embedding");
        if (embOpt.has_value()) {
            node_embeddings.push_back(*embOpt);
            if (embedding_dim == 0) {
                embedding_dim = static_cast<int>(embOpt->size());
            }
        }
        return true;
    });
    
    if (node_embeddings.empty()) {
        return {Status::Error("No node embeddings found for graph"), {}};
    }
    
    // Aggregate embeddings
    std::vector<float> graph_embedding(embedding_dim, 0.0f);
    
    if (aggregation_method == "mean") {
        // Mean pooling
        for (const auto& emb : node_embeddings) {
            for (size_t i = 0; i < emb.size() && i < graph_embedding.size(); ++i) {
                graph_embedding[i] += emb[i];
            }
        }
        float count = static_cast<float>(node_embeddings.size());
        for (float& val : graph_embedding) {
            val /= count;
        }
    } else if (aggregation_method == "sum") {
        // Sum pooling
        for (const auto& emb : node_embeddings) {
            for (size_t i = 0; i < emb.size() && i < graph_embedding.size(); ++i) {
                graph_embedding[i] += emb[i];
            }
        }
    } else if (aggregation_method == "max") {
        // Max pooling
        std::fill(graph_embedding.begin(), graph_embedding.end(), -std::numeric_limits<float>::infinity());
        for (const auto& emb : node_embeddings) {
            for (size_t i = 0; i < emb.size() && i < graph_embedding.size(); ++i) {
                graph_embedding[i] = std::max(graph_embedding[i], emb[i]);
            }
        }
    }
    
    return {Status::OK(), graph_embedding};
}

// ===== Embedding Retrieval =====

std::pair<GNNEmbeddingManager::Status, GNNEmbeddingManager::EmbeddingInfo>
GNNEmbeddingManager::getNodeEmbedding(
    std::string_view node_pk,
    std::string_view graph_id,
    std::string_view model_name
) const {
    std::string embKey = makeEmbeddingKey_("node", graph_id, node_pk, model_name);
    auto blob = db_.get(embKey);
    
    if (!blob.has_value()) {
        return {Status::Error("Embedding not found"), {}};
    }
    
    auto embEntity = deserializeEntitySafe(embKey, *blob);
    if (!embEntity.has_value()) {
        return {Status::Error("Embedding deserialization failed"), {}};
    }
    
    EmbeddingInfo info;
    info.entity_id = std::string(node_pk);
    info.entity_type = "node";
    info.graph_id = std::string(graph_id);
    info.model_name = std::string(model_name);
    
    auto timestampOpt = embEntity->getFieldAsInt("timestamp");
    if (timestampOpt.has_value()) {
        info.timestamp = *timestampOpt;
    }
    
    auto embOpt = embEntity->getFieldAsVector("embedding");
    if (embOpt.has_value()) {
        info.embedding = *embOpt;
    }
    
    return {Status::OK(), info};
}

std::pair<GNNEmbeddingManager::Status, GNNEmbeddingManager::EmbeddingInfo>
GNNEmbeddingManager::getEdgeEmbedding(
    std::string_view edge_id,
    std::string_view graph_id,
    std::string_view model_name
) const {
    std::string embKey = makeEmbeddingKey_("edge", graph_id, edge_id, model_name);
    auto blob = db_.get(embKey);
    
    if (!blob.has_value()) {
        return {Status::Error("Embedding not found"), {}};
    }
    
    auto embEntity = deserializeEntitySafe(embKey, *blob);
    if (!embEntity.has_value()) {
        return {Status::Error("Embedding deserialization failed"), {}};
    }
    
    EmbeddingInfo info;
    info.entity_id = std::string(edge_id);
    info.entity_type = "edge";
    info.graph_id = std::string(graph_id);
    info.model_name = std::string(model_name);
    
    auto timestampOpt = embEntity->getFieldAsInt("timestamp");
    if (timestampOpt.has_value()) {
        info.timestamp = *timestampOpt;
    }
    
    auto embOpt = embEntity->getFieldAsVector("embedding");
    if (embOpt.has_value()) {
        info.embedding = *embOpt;
    }
    
    return {Status::OK(), info};
}

// ===== Similarity Search =====

std::pair<GNNEmbeddingManager::Status, std::vector<GNNEmbeddingManager::SimilarityResult>>
GNNEmbeddingManager::findSimilarNodes(
    std::string_view node_pk,
    std::string_view graph_id,
    int k,
    std::string_view model_name
) const {
    // Get query embedding
    auto [st, embInfo] = getNodeEmbedding(node_pk, graph_id, model_name);
    if (!st.ok) {
        return {st, {}};
    }
    
    // Search in vector index
    auto [st2, results] = vim_.searchKnn(embInfo.embedding, k + 1);  // +1 to exclude self
    if (!st2.ok) {
        return {Status::Error("Vector search failed: " + st2.message), {}};
    }
    
    // Convert results
    std::vector<SimilarityResult> similar = {};

    similar.reserve(std::min(results.size(), static_cast<size_t>(std::max(0, k))));
    for (const auto& res : results) {
        // Parse embedding key to get entity info
        auto parts = parseEmbeddingKey_(res.pk);
        if (!parts.has_value()) {
          continue;
        }
        
        // Skip self
        if (parts->entity_id == node_pk) {
          continue;
        }
        
        // Filter by graph and model
        if (parts->graph_id != graph_id || parts->model_name != model_name) {
          continue;
        }
        
        SimilarityResult simRes;
        simRes.entity_id = parts->entity_id;
        simRes.similarity = 1.0f - res.distance;  // Convert distance to similarity
        simRes.entity_type = parts->entity_type;
        simRes.graph_id = parts->graph_id;
        
        similar.push_back(simRes);
        
        if (static_cast<int>(similar.size()) > = static_cast<size_t>(k)) {
          break;
        }
    }
    
    return {Status::OK(), similar};
}

std::pair<GNNEmbeddingManager::Status, std::vector<GNNEmbeddingManager::SimilarityResult>>
GNNEmbeddingManager::findSimilarEdges(
    std::string_view edge_id,
    std::string_view graph_id,
    int k,
    std::string_view model_name
) const {
    // Get query embedding
    auto [st, embInfo] = getEdgeEmbedding(edge_id, graph_id, model_name);
    if (!st.ok) {
        return {st, {}};
    }
    
    // Search in vector index
    auto [st2, results] = vim_.searchKnn(embInfo.embedding, k + 1);
    if (!st2.ok) {
        return {Status::Error("Vector search failed: " + st2.message), {}};
    }
    
    // Convert results (similar to findSimilarNodes)
    std::vector<SimilarityResult> similar = {};

    similar.reserve(std::min(results.size(), static_cast<size_t>(std::max(0, k))));
    for (const auto& res : results) {
        auto parts = parseEmbeddingKey_(res.pk);
        if (!parts.has_value()) {
          continue;
        }
        if (parts->entity_id == edge_id) {
          continue;
        }
        if (parts->graph_id != graph_id || parts->model_name != model_name) {
          continue;
        }
        
        SimilarityResult simRes;
        simRes.entity_id = parts->entity_id;
        simRes.similarity = 1.0f - res.distance;
        simRes.entity_type = parts->entity_type;
        simRes.graph_id = parts->graph_id;
        
        similar.push_back(simRes);
        if (static_cast<int>(similar.size()) > = static_cast<size_t>(k)) {
          break;
        }
    }
    
    return {Status::OK(), similar};
}

// ===== Model Management =====

GNNEmbeddingManager::Status GNNEmbeddingManager::registerModel(
    std::string_view model_name,
    std::string_view model_type,
    int embedding_dim,
    std::string_view config
) {
    ModelInfo info;
    info.name = std::string(model_name);
    info.type = std::string(model_type);
    info.embedding_dim = embedding_dim;
    info.config = std::string(config);
    info.registered_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    models_[info.name] = info;
    return Status::OK();
}

std::pair<GNNEmbeddingManager::Status, std::vector<std::string>>
GNNEmbeddingManager::listModels() const {
    std::vector<std::string> names = {};

    names.reserve(models_.size());
    for (const auto& [name, _] : models_) {
        names.push_back(name);
    }
    return {Status::OK(), names};
}

std::pair<GNNEmbeddingManager::Status, GNNEmbeddingManager::ModelInfo>
GNNEmbeddingManager::getModelInfo(std::string_view model_name) const {
    auto it = models_.find(std::string(model_name));
    if (it == models_.end()) {
        return {Status::Error("Model not found"), {}};
    }
    return {Status::OK(), it->second};
}

GNNEmbeddingManager::Status GNNEmbeddingManager::setAggregationStrategy(
    std::string_view model_name,
    AggregationStrategy strategy
) {
    auto it = models_.find(std::string(model_name));
    if (it == models_.end()) {
        return Status::Error("Model not found");
    }
    
    it->second.aggregation = strategy;
    return Status::OK();
}

// ===== Batch Operations =====

GNNEmbeddingManager::Status GNNEmbeddingManager::generateNodeEmbeddingsBatch(
    const std::vector<std::string>& node_pks,
    std::string_view graph_id,
    std::string_view model_name,
    size_t batch_size
) {
    if (batch_size == 0) {
        return Status::Error("batch_size must be > 0");
    }
    for (size_t i = 0; i < node_pks.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, node_pks.size());
        
        for (size_t j = i; j < end; ++j) {
            auto st = updateNodeEmbedding(node_pks[j], graph_id, model_name);
            // Silently continue on error (for batch processing)
        }
    }
    
    return Status::OK();
}

GNNEmbeddingManager::Status GNNEmbeddingManager::generateEdgeEmbeddingsBatch(
    const std::vector<std::string>& edge_ids,
    std::string_view graph_id,
    std::string_view model_name,
    size_t batch_size
) {
    if (batch_size == 0) {
        return Status::Error("batch_size must be > 0");
    }
    for (size_t i = 0; i < edge_ids.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, edge_ids.size());
        
        for (size_t j = i; j < end; ++j) {
            auto st = updateEdgeEmbedding(edge_ids[j], graph_id, model_name);
            // Silently continue on error (for batch processing)
        }
    }
    
    return Status::OK();
}

// ===== Statistics =====

std::pair<GNNEmbeddingManager::Status, GNNEmbeddingManager::EmbeddingStats>
GNNEmbeddingManager::getStats() const {
    EmbeddingStats stats;
    stats.total_node_embeddings = 0;
    stats.total_edge_embeddings = 0;
    
    // Scan all embeddings
    db_.scanPrefix("gnn_emb:", [&stats](std::string_view key, std::string_view /*val*/) {
        std::string keyStr(key);
        
        // Parse key to extract entity_type, model_name, graph_id
        std::vector<std::string> parts;
        parts.reserve(std::count(keyStr.begin(), keyStr.end(), ':') + 1);
        std::istringstream iss(keyStr);
        std::string part = {};
        while (std::getline(iss, part, ':')) {
            parts.push_back(part);
        }
        
        if (static_cast<int>(parts.size()) > = 4) {
            std::string entity_type = parts[1];
            std::string graph_id = parts[2];
            std::string model_name = parts[3];
            
            if (entity_type == "node") {
                stats.total_node_embeddings++;
            } else if (entity_type == "edge") {
                stats.total_edge_embeddings++;
            }
            
            stats.embeddings_per_model[model_name]++;
            stats.embeddings_per_graph[graph_id]++;
        }
        
        return true;
    });
    
    return {Status::OK(), stats};
}

} // namespace themis

