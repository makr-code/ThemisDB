/**
 * @file property_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Property Graph Manager Implementation

#include "index/property_graph.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <stdexcept>

namespace themis {

namespace {

[[nodiscard]] std::vector<std::string> parseLabelsField(const std::string& raw_labels) {
    std::vector<std::string> labels;
    if (raw_labels.empty()) {
        return labels;
    }

    // Preferred format: JSON string array, e.g. ["Person","Employee"].
    try {
        auto parsed = nlohmann::json::parse(raw_labels);
        if (parsed.is_array()) {
            labels.reserve(parsed.size());
            for (const auto& entry : parsed) {
                if (!entry.is_string()) {
                    continue;
                }
                auto label = entry.get<std::string>();
                if (!label.empty()) {
                    labels.push_back(std::move(label));
                }
            }
            return labels;
        }
    } catch (...) {
        // Backward-compatible fallback below (legacy comma-separated encoding).
    }

    // Legacy format fallback: comma-separated string.
    std::stringstream ss(raw_labels);
    std::string label;
    while (std::getline(ss, label, ',')) {
        label.erase(0, label.find_first_not_of(" \t"));
        label.erase(label.find_last_not_of(" \t") + 1);
        if (!label.empty()) {
            labels.push_back(std::move(label));
        }
    }
    return labels;
}

[[nodiscard]] std::string encodeLabelsField(const std::vector<std::string>& labels) {
    return nlohmann::json(labels).dump();
}

} // namespace

PropertyGraphManager::PropertyGraphManager(RocksDBWrapper& db) : db_(db) {}

// ===== Helper Methods =====

std::vector<std::string> PropertyGraphManager::extractLabels_(const BaseEntity& node) const {
    // Delegate to BaseEntity::getFieldAsStringArray(), which handles both the
    // current JSON-array serialization and the legacy comma-separated format.
    auto arr = node.getFieldAsStringArray("_labels");
    if (!arr.has_value()) {
        return {};
    }

    return *arr;
}

std::optional<std::string> PropertyGraphManager::extractType_(const BaseEntity& edge) const {
    return edge.getFieldAsString("_type");
}

std::string PropertyGraphManager::makeLabelIndexKey_(std::string_view graph_id, std::string_view label, std::string_view pk) const {
    std::ostringstream oss;
    oss << "label:" << graph_id << ":" << label << ":" << pk;
    return oss.str();
}

std::string PropertyGraphManager::makeTypeIndexKey_(std::string_view graph_id, std::string_view type, std::string_view edgeId) const {
    std::ostringstream oss;
    oss << "type:" << graph_id << ":" << type << ":" << edgeId;
    return oss.str();
}

std::string PropertyGraphManager::makeNodeKey_(std::string_view graph_id, std::string_view pk) const {
    std::ostringstream oss;
    oss << "node:" << graph_id << ":" << pk;
    return oss.str();
}

std::string PropertyGraphManager::makeEdgeKey_(std::string_view graph_id, std::string_view edgeId) const {
    std::ostringstream oss;
    oss << "edge:" << graph_id << ":" << edgeId;
    return oss.str();
}

std::string PropertyGraphManager::makeGraphOutdexKey_(std::string_view graph_id, std::string_view fromPk, std::string_view edgeId) const {
    std::ostringstream oss;
    oss << "graph:out:" << graph_id << ":" << fromPk << ":" << edgeId;
    return oss.str();
}

std::string PropertyGraphManager::makeGraphIndegKey_(std::string_view graph_id, std::string_view toPk, std::string_view edgeId) const {
    std::ostringstream oss;
    oss << "graph:in:" << graph_id << ":" << toPk << ":" << edgeId;
    return oss.str();
}

// ===== Node Label Operations =====

PropertyGraphManager::Status PropertyGraphManager::addNode(const BaseEntity& node, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("addNode: Database not open");
    }

    auto pkOpt = node.getFieldAsString("id");
    if (!pkOpt.has_value()) {
        return Status::Error("addNode: Node must have 'id' field");
    }
    const std::string& pk = *pkOpt;

    // Extract labels
    std::vector<std::string> labels = extractLabels_(node);

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("addNode: Could not create write batch");
    }

    // Store node entity
    std::string nodeKey = makeNodeKey_(graph_id, pk);
    batch->put(nodeKey, node.serialize());

    // Create label index entries
    for (const auto& label : labels) {
        std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
        batch->put(labelKey, std::vector<uint8_t>());  // Empty value (index only needs key)
    }

    if (!batch->commit()) {
        return Status::Error("addNode: Failed to commit write batch");
    }

    return Status::OK();
}

PropertyGraphManager::Status PropertyGraphManager::deleteNode(std::string_view pk, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("deleteNode: Database not open");
    }

    // Load node to get labels
    std::string nodeKey = makeNodeKey_(graph_id, pk);
    auto blob = db_.get(nodeKey);
    if (!blob.has_value()) {
        return Status::OK();  // Already deleted (idempotent)
    }

    BaseEntity node = BaseEntity::deserialize(std::string(pk), *blob);
    std::vector<std::string> labels = extractLabels_(node);

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("deleteNode: Could not create write batch");
    }

    // Collect all edges connected to this node (both outgoing and incoming)
    // Use unordered_set to avoid duplicates and improve lookup performance
    std::unordered_set<std::string> edgesToDelete;
    
    // Scan outgoing edges: graph:out:<graph_id>:<pk>:
    {
        std::ostringstream oss;
        oss << "graph:out:" << graph_id << ":" << pk << ":";
        std::string outPrefix = oss.str();
        
        db_.scanPrefix(outPrefix, [&edgesToDelete, &outPrefix](std::string_view key, std::string_view /*val*/) {
            // Extract edgeId from key: graph:out:<graph_id>:<pk>:<edgeId>
            std::string keyStr(key);
            size_t lastColon = keyStr.rfind(':');
            if (lastColon != std::string::npos && lastColon > outPrefix.size() - 1) {
                std::string edgeId = keyStr.substr(lastColon + 1);
                if (!edgeId.empty()) {
                    edgesToDelete.insert(edgeId);
                }
            }
            return true;  // Continue scanning
        });
    }
    
    // Scan incoming edges: graph:in:<graph_id>:<pk>:
    {
        std::ostringstream oss;
        oss << "graph:in:" << graph_id << ":" << pk << ":";
        std::string inPrefix = oss.str();
        
        db_.scanPrefix(inPrefix, [&edgesToDelete, &inPrefix](std::string_view key, std::string_view /*val*/) {
            // Extract edgeId from key: graph:in:<graph_id>:<pk>:<edgeId>
            std::string keyStr(key);
            size_t lastColon = keyStr.rfind(':');
            if (lastColon != std::string::npos && lastColon > inPrefix.size() - 1) {
                std::string edgeId = keyStr.substr(lastColon + 1);
                if (!edgeId.empty()) {
                    edgesToDelete.insert(edgeId);
                }
            }
            return true;  // Continue scanning
        });
    }
    
    // Delete all connected edges
    for (const auto& edgeId : edgesToDelete) {
        // Load edge to get _from, _to, _type
        std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
        auto edgeBlob = db_.get(edgeKey);
        if (!edgeBlob.has_value()) {
            continue;  // Edge already deleted
        }
        
        BaseEntity edge = BaseEntity::deserialize(edgeId, *edgeBlob);
        auto fromOpt = edge.getFieldAsString("_from");
        auto toOpt = edge.getFieldAsString("_to");
        std::optional<std::string> typeOpt = extractType_(edge);
        
        // Delete edge entity
        batch->del(edgeKey);
        
        // Delete graph adjacency indices
        if (fromOpt && toOpt) {
            std::string outdexKey = makeGraphOutdexKey_(graph_id, *fromOpt, edgeId);
            std::string indegKey = makeGraphIndegKey_(graph_id, *toOpt, edgeId);
            batch->del(outdexKey);
            batch->del(indegKey);
        }
        
        // Delete type index entry if type exists
        if (typeOpt.has_value()) {
            std::string typeKey = makeTypeIndexKey_(graph_id, *typeOpt, edgeId);
            batch->del(typeKey);
        }
    }

    // Delete node entity
    batch->del(nodeKey);

    // Delete label index entries
    for (const auto& label : labels) {
        std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
        batch->del(labelKey);
    }

    if (!batch->commit()) {
        return Status::Error("deleteNode: Failed to commit write batch");
    }

    return Status::OK();
}

PropertyGraphManager::Status PropertyGraphManager::addNodeLabel(std::string_view pk, std::string_view label, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("addNodeLabel: Database not open");
    }

    // Load node
    std::string nodeKey = makeNodeKey_(graph_id, pk);
    auto blob = db_.get(nodeKey);
    if (!blob.has_value()) {
        return Status::Error("addNodeLabel: Node not found");
    }

    BaseEntity node = BaseEntity::deserialize(std::string(pk), *blob);
    std::vector<std::string> labels = extractLabels_(node);

    // Check if label already exists
    if (std::find(labels.begin(), labels.end(), label) != labels.end()) {
        return Status::OK();  // Label already exists (idempotent)
    }

    // Add label to node
    labels.push_back(std::string(label));
    node.setField("_labels", encodeLabelsField(labels));

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("addNodeLabel: Could not create write batch");
    }

    // Update node entity
    batch->put(nodeKey, node.serialize());

    // Add label index entry
    std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
    batch->put(labelKey, std::vector<uint8_t>());

    if (!batch->commit()) {
        return Status::Error("addNodeLabel: Failed to commit write batch");
    }

    return Status::OK();
}

PropertyGraphManager::Status PropertyGraphManager::removeNodeLabel(std::string_view pk, std::string_view label, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("removeNodeLabel: Database not open");
    }

    // Load node
    std::string nodeKey = makeNodeKey_(graph_id, pk);
    auto blob = db_.get(nodeKey);
    if (!blob.has_value()) {
        return Status::Error("removeNodeLabel: Node not found");
    }

    BaseEntity node = BaseEntity::deserialize(std::string(pk), *blob);
    std::vector<std::string> labels = extractLabels_(node);

    // Update labels
    const auto it = std::find(labels.begin(), labels.end(), label);
    if (it == labels.end()) {
        return Status::OK();  // Label not present (idempotent)
    }
    labels.erase(it);

    // Update labels string
    node.setField("_labels", encodeLabelsField(labels));

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("removeNodeLabel: Could not create write batch");
    }

    // Update node entity
    batch->put(nodeKey, node.serialize());

    // Delete label index entry
    std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
    batch->del(labelKey);

    if (!batch->commit()) {
        return Status::Error("removeNodeLabel: Failed to commit write batch");
    }

    return Status::OK();
}

std::pair<PropertyGraphManager::Status, bool> PropertyGraphManager::hasNodeLabel(
    std::string_view pk, std::string_view label, std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("hasNodeLabel: Database not open"), false};
    }

    // Check label index
    std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
    auto blob = db_.get(labelKey);
    return {Status::OK(), blob.has_value()};
}

std::pair<PropertyGraphManager::Status, std::vector<std::string>> PropertyGraphManager::getNodesByLabel(
    std::string_view label, std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getNodesByLabel: Database not open"), {}};
    }

    std::vector<std::string> nodes;
    std::ostringstream oss;
    oss << "label:" << graph_id << ":" << label << ":";
    std::string prefix = oss.str();

    db_.scanPrefix(prefix, [&nodes, &prefix](std::string_view key, std::string_view /*val*/) {
        // Extract PK from key: label:<graph_id>:<label>:<pk>
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos && lastColon >= prefix.size() - 1) {
            std::string pk = keyStr.substr(lastColon + 1);
            if (!pk.empty()) {
                nodes.push_back(pk);
            }
        }
        return true;
    });

    return {Status::OK(), nodes};
}

std::pair<PropertyGraphManager::Status, std::vector<std::string>> PropertyGraphManager::getNodeLabels(
    std::string_view pk, std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getNodeLabels: Database not open"), {}};
    }

    // Load node and extract labels
    std::string nodeKey = makeNodeKey_(graph_id, pk);
    auto blob = db_.get(nodeKey);
    if (!blob.has_value()) {
        return {Status::Error("getNodeLabels: Node not found"), {}};
    }

    BaseEntity node = BaseEntity::deserialize(std::string(pk), *blob);
    std::vector<std::string> labels = extractLabels_(node);

    return {Status::OK(), labels};
}

// ===== Relationship Type Operations =====

PropertyGraphManager::Status PropertyGraphManager::addEdge(const BaseEntity& edge, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("addEdge: Database not open");
    }

    auto edgeIdOpt = edge.getFieldAsString("id");
    auto fromOpt = edge.getFieldAsString("_from");
    auto toOpt = edge.getFieldAsString("_to");
    
    if (!edgeIdOpt || !fromOpt || !toOpt) {
        return Status::Error("addEdge: Edge must have 'id', '_from', and '_to' fields");
    }

    const std::string& edgeId = *edgeIdOpt;
    const std::string& from = *fromOpt;
    const std::string& to = *toOpt;

    // Extract type (optional)
    std::optional<std::string> typeOpt = extractType_(edge);

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("addEdge: Could not create write batch");
    }

    // Store edge entity
    std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
    batch->put(edgeKey, edge.serialize());

    // Create graph adjacency indices
    std::string outdexKey = makeGraphOutdexKey_(graph_id, from, edgeId);
    std::string indegKey = makeGraphIndegKey_(graph_id, to, edgeId);
    batch->put(outdexKey, std::vector<uint8_t>(to.begin(), to.end()));
    batch->put(indegKey, std::vector<uint8_t>(from.begin(), from.end()));

    // Create type index entry if type exists
    if (typeOpt.has_value()) {
        std::string typeKey = makeTypeIndexKey_(graph_id, *typeOpt, edgeId);
        batch->put(typeKey, std::vector<uint8_t>());
    }

    if (!batch->commit()) {
        return Status::Error("addEdge: Failed to commit write batch");
    }

    return Status::OK();
}

PropertyGraphManager::Status PropertyGraphManager::deleteEdge(std::string_view edgeId, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("deleteEdge: Database not open");
    }

    // Load edge to get _from, _to, _type
    std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
    auto blob = db_.get(edgeKey);
    if (!blob.has_value()) {
        return Status::OK();  // Already deleted (idempotent)
    }

    BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
    auto fromOpt = edge.getFieldAsString("_from");
    auto toOpt = edge.getFieldAsString("_to");
    std::optional<std::string> typeOpt = extractType_(edge);

    if (!fromOpt || !toOpt) {
        return Status::Error("deleteEdge: Edge has no _from/_to fields");
    }

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("deleteEdge: Could not create write batch");
    }

    // Delete edge entity
    batch->del(edgeKey);

    // Delete graph adjacency indices
    std::string outdexKey = makeGraphOutdexKey_(graph_id, *fromOpt, edgeId);
    std::string indegKey = makeGraphIndegKey_(graph_id, *toOpt, edgeId);
    batch->del(outdexKey);
    batch->del(indegKey);

    // Delete type index entry if type exists
    if (typeOpt.has_value()) {
        std::string typeKey = makeTypeIndexKey_(graph_id, *typeOpt, edgeId);
        batch->del(typeKey);
    }

    if (!batch->commit()) {
        return Status::Error("deleteEdge: Failed to commit write batch");
    }

    return Status::OK();
}

std::pair<PropertyGraphManager::Status, std::vector<PropertyGraphManager::EdgeInfo>> 
PropertyGraphManager::getEdgesByType(std::string_view type, std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getEdgesByType: Database not open"), {}};
    }

    std::vector<EdgeInfo> edges;
    std::ostringstream oss;
    oss << "type:" << graph_id << ":" << type << ":";
    std::string prefix = oss.str();

    db_.scanPrefix(prefix, [this, &edges, &prefix, &graph_id](std::string_view key, std::string_view /*val*/) {
        // Extract edgeId from key: type:<graph_id>:<type>:<edgeId>
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos && lastColon >= prefix.size() - 1) {
            std::string edgeId = keyStr.substr(lastColon + 1);
            if (edgeId.empty()) return true;
            
            // Load edge entity to get _from, _to
            std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
            auto blob = db_.get(edgeKey);
            if (blob.has_value()) {
                BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
                auto fromOpt = edge.getFieldAsString("_from");
                auto toOpt = edge.getFieldAsString("_to");
                auto typeOpt = extractType_(edge);
                
                if (fromOpt && toOpt && typeOpt) {
                    edges.push_back({
                        edgeId,
                        *fromOpt,
                        *toOpt,
                        *typeOpt,
                        std::string(graph_id)
                    });
                }
            }
        }
        return true;
    });

    return {Status::OK(), edges};
}

std::pair<PropertyGraphManager::Status, std::string> PropertyGraphManager::getEdgeType(
    std::string_view edgeId, std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getEdgeType: Database not open"), ""};
    }

    std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
    auto blob = db_.get(edgeKey);
    if (!blob.has_value()) {
        return {Status::Error("getEdgeType: Edge not found"), ""};
    }

    BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
    std::optional<std::string> typeOpt = extractType_(edge);
    
    if (!typeOpt.has_value()) {
        return {Status::Error("getEdgeType: Edge has no _type field"), ""};
    }

    return {Status::OK(), *typeOpt};
}

std::pair<PropertyGraphManager::Status, std::vector<PropertyGraphManager::EdgeInfo>>
PropertyGraphManager::getTypedOutEdges(
    std::string_view fromPk,
    std::string_view type,
    std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getTypedOutEdges: Database not open"), {}};
    }

    std::vector<EdgeInfo> edges;
    std::ostringstream oss;
    oss << "graph:out:" << graph_id << ":" << fromPk << ":";
    std::string prefix = oss.str();

    db_.scanPrefix(prefix, [this, &edges, &type, &graph_id, &fromPk](std::string_view key, std::string_view val) {
        // Extract edgeId from key: graph:out:<graph_id>:<from_pk>:<edgeId>
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) return true;
        
        std::string edgeId = keyStr.substr(lastColon + 1);
        std::string toPk(val);
        
        // Load edge to check type
        std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
        auto blob = db_.get(edgeKey);
        if (!blob.has_value()) return true;
        
        BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
        std::optional<std::string> edgeTypeOpt = extractType_(edge);
        
        // Filter by type
        if (edgeTypeOpt.has_value() && *edgeTypeOpt == type) {
            edges.push_back({
                edgeId,
                std::string(fromPk),
                toPk,
                *edgeTypeOpt,
                std::string(graph_id)
            });
        }
        
        return true;
    });

    return {Status::OK(), edges};
}

// ===== Multi-Graph Federation =====

std::pair<PropertyGraphManager::Status, std::vector<std::string>> PropertyGraphManager::listGraphs() const {
    if (!db_.isOpen()) {
        return {Status::Error("listGraphs: Database not open"), {}};
    }

    std::unordered_set<std::string> graphSet;
    
    // Scan node keys: node:<graph_id>:*
    db_.scanPrefix("node:", [&graphSet](std::string_view key, std::string_view /*val*/) {
        std::string keyStr(key);
        // Extract graph_id from key: node:<graph_id>:<pk>
        size_t firstColon = keyStr.find(':');
        size_t secondColon = keyStr.find(':', firstColon + 1);
        if (firstColon != std::string::npos && secondColon != std::string::npos) {
            std::string graphId = keyStr.substr(firstColon + 1, secondColon - firstColon - 1);
            graphSet.insert(graphId);
        }
        return true;
    });

    std::vector<std::string> graphs(graphSet.begin(), graphSet.end());
    std::sort(graphs.begin(), graphs.end());
    
    return {Status::OK(), graphs};
}

std::pair<PropertyGraphManager::Status, PropertyGraphManager::GraphStats> 
PropertyGraphManager::getGraphStats(std::string_view graph_id) const {
    if (!db_.isOpen()) {
        return {Status::Error("getGraphStats: Database not open"), {}};
    }

    GraphStats stats;
    stats.graph_id = std::string(graph_id);
    stats.node_count = 0;
    stats.edge_count = 0;
    
    std::unordered_set<std::string> labels;
    std::unordered_set<std::string> types;

    // Count nodes
    std::ostringstream nodePrefix;
    nodePrefix << "node:" << graph_id << ":";
    db_.scanPrefix(nodePrefix.str(), [&stats](std::string_view /*key*/, std::string_view /*val*/) {
        stats.node_count++;
        return true;
    });

    // Count edges
    std::ostringstream edgePrefix;
    edgePrefix << "edge:" << graph_id << ":";
    db_.scanPrefix(edgePrefix.str(), [&stats](std::string_view /*key*/, std::string_view /*val*/) {
        stats.edge_count++;
        return true;
    });

    // Count unique labels
    std::ostringstream labelPrefix;
    labelPrefix << "label:" << graph_id << ":";
    db_.scanPrefix(labelPrefix.str(), [&labels, &labelPrefix](std::string_view key, std::string_view /*val*/) {
        std::string keyStr(key);
        // Extract label from key: label:<graph_id>:<label>:<pk>
        size_t prefixLen = labelPrefix.str().size();
        size_t nextColon = keyStr.find(':', prefixLen);
        if (nextColon != std::string::npos) {
            std::string label = keyStr.substr(prefixLen, nextColon - prefixLen);
            labels.insert(label);
        }
        return true;
    });
    stats.label_count = labels.size();

    // Count unique types
    std::ostringstream typePrefix;
    typePrefix << "type:" << graph_id << ":";
    db_.scanPrefix(typePrefix.str(), [&types, &typePrefix](std::string_view key, std::string_view /*val*/) {
        std::string keyStr(key);
        // Extract type from key: type:<graph_id>:<type>:<edgeId>
        size_t prefixLen = typePrefix.str().size();
        size_t nextColon = keyStr.find(':', prefixLen);
        if (nextColon != std::string::npos) {
            std::string type = keyStr.substr(prefixLen, nextColon - prefixLen);
            types.insert(type);
        }
        return true;
    });
    stats.type_count = types.size();

    return {Status::OK(), stats};
}

std::pair<PropertyGraphManager::Status, PropertyGraphManager::FederationResult>
PropertyGraphManager::federatedQuery(const std::vector<FederationPattern>& patterns) const {
    if (!db_.isOpen()) {
        return {Status::Error("federatedQuery: Database not open"), {}};
    }

    FederationResult result;

    for (const auto& pattern : patterns) {
        if (pattern.pattern_type == "node") {
            // Query nodes by label
            auto [st, nodes] = getNodesByLabel(pattern.label_or_type, pattern.graph_id);
            if (!st.ok) {
                return {st, {}};
            }
            
            for (const auto& pk : nodes) {
                result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
            }
        } else if (pattern.pattern_type == "edge") {
            // Query edges by type
            auto [st, edges] = getEdgesByType(pattern.label_or_type, pattern.graph_id);
            if (!st.ok) {
                return {st, {}};
            }
            
            result.edges.insert(result.edges.end(), edges.begin(), edges.end());
        }
    }

    return {Status::OK(), result};
}

// ===== Batch Operations =====

PropertyGraphManager::Status PropertyGraphManager::addNodesBatch(
    const std::vector<BaseEntity>& nodes, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("addNodesBatch: Database not open");
    }

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("addNodesBatch: Could not create write batch");
    }

    for (const auto& node : nodes) {
        auto pkOpt = node.getFieldAsString("id");
        if (!pkOpt.has_value()) {
            batch->rollback();
            return Status::Error("addNodesBatch: Node missing 'id' field");
        }
        const std::string& pk = *pkOpt;

        // Store node
        std::string nodeKey = makeNodeKey_(graph_id, pk);
        batch->put(nodeKey, node.serialize());

        // Create label indices
        std::vector<std::string> labels = extractLabels_(node);
        for (const auto& label : labels) {
            std::string labelKey = makeLabelIndexKey_(graph_id, label, pk);
            batch->put(labelKey, std::vector<uint8_t>());
        }
    }

    if (!batch->commit()) {
        return Status::Error("addNodesBatch: Failed to commit write batch");
    }

    return Status::OK();
}

PropertyGraphManager::Status PropertyGraphManager::addEdgesBatch(
    const std::vector<BaseEntity>& edges, std::string_view graph_id) {
    if (!db_.isOpen()) {
        return Status::Error("addEdgesBatch: Database not open");
    }

    auto batch = db_.createWriteBatch();
    if (!batch) {
        return Status::Error("addEdgesBatch: Could not create write batch");
    }

    for (const auto& edge : edges) {
        auto edgeIdOpt = edge.getFieldAsString("id");
        auto fromOpt = edge.getFieldAsString("_from");
        auto toOpt = edge.getFieldAsString("_to");
        
        if (!edgeIdOpt || !fromOpt || !toOpt) {
            batch->rollback();
            return Status::Error("addEdgesBatch: Edge missing 'id', '_from', or '_to' field");
        }

        const std::string& edgeId = *edgeIdOpt;
        const std::string& from = *fromOpt;
        const std::string& to = *toOpt;

        // Store edge
        std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
        batch->put(edgeKey, edge.serialize());

        // Create adjacency indices
        std::string outdexKey = makeGraphOutdexKey_(graph_id, from, edgeId);
        std::string indegKey = makeGraphIndegKey_(graph_id, to, edgeId);
        batch->put(outdexKey, std::vector<uint8_t>(to.begin(), to.end()));
        batch->put(indegKey, std::vector<uint8_t>(from.begin(), from.end()));

        // Create type index if present
        std::optional<std::string> typeOpt = extractType_(edge);
        if (typeOpt.has_value()) {
            std::string typeKey = makeTypeIndexKey_(graph_id, *typeOpt, edgeId);
            batch->put(typeKey, std::vector<uint8_t>());
        }
    }

    if (!batch->commit()) {
        return Status::Error("addEdgesBatch: Failed to commit write batch");
    }

    return Status::OK();
}

// ===== Graph Traversal Algorithms =====

std::pair<PropertyGraphManager::Status, std::vector<std::string>>
PropertyGraphManager::traverseBFS(
    std::string_view start_node_pk,
    std::string_view graph_id,
    int max_depth
) const {
    if (!db_.isOpen()) {
        return {Status::Error("traverseBFS: Database not open"), {}};
    }

    // Check if start node exists
    std::string nodeKey = makeNodeKey_(graph_id, start_node_pk);
    if (!db_.get(nodeKey).has_value()) {
        return {Status::Error("traverseBFS: Start node not found"), {}};
    }

    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::queue<std::pair<std::string, int>> queue;  // node_pk, depth
    
    queue.push({std::string(start_node_pk), 0});
    visited.insert(std::string(start_node_pk));
    
    while (!queue.empty()) {
        auto [current_node, depth] = queue.front();
        queue.pop();
        
        result.push_back(current_node);
        
        // Stop if max depth reached
        if (max_depth >= 0 && depth >= max_depth) {
            continue;
        }
        
        // Get outgoing edges
        std::ostringstream outPrefix;
        outPrefix << "graph:out:" << graph_id << ":" << current_node << ":";
        
        db_.scanPrefix(outPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
            std::string neighbor(val);
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                queue.push({neighbor, depth + 1});
            }
            return true;
        });
    }
    
    return {Status::OK(), result};
}

std::pair<PropertyGraphManager::Status, std::vector<std::string>>
PropertyGraphManager::traverseDFS(
    std::string_view start_node_pk,
    std::string_view graph_id,
    int max_depth
) const {
    if (!db_.isOpen()) {
        return {Status::Error("traverseDFS: Database not open"), {}};
    }

    // Check if start node exists
    std::string nodeKey = makeNodeKey_(graph_id, start_node_pk);
    if (!db_.get(nodeKey).has_value()) {
        return {Status::Error("traverseDFS: Start node not found"), {}};
    }

    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::stack<std::pair<std::string, int>> stack;  // node_pk, depth
    
    stack.push({std::string(start_node_pk), 0});
    
    while (!stack.empty()) {
        auto [current_node, depth] = stack.top();
        stack.pop();
        
        if (visited.find(current_node) != visited.end()) {
            continue;
        }
        
        visited.insert(current_node);
        result.push_back(current_node);
        
        // Stop if max depth reached
        if (max_depth >= 0 && depth >= max_depth) {
            continue;
        }
        
        // Get outgoing edges (push in reverse order for consistent DFS ordering)
        std::vector<std::string> neighbors;
        std::ostringstream outPrefix;
        outPrefix << "graph:out:" << graph_id << ":" << current_node << ":";
        
        db_.scanPrefix(outPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
            std::string neighbor(val);
            if (visited.find(neighbor) == visited.end()) {
                neighbors.push_back(neighbor);
            }
            return true;
        });
        
        // Push neighbors in reverse order
        for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
            stack.push({*it, depth + 1});
        }
    }
    
    return {Status::OK(), result};
}

std::pair<PropertyGraphManager::Status, std::vector<std::string>>
PropertyGraphManager::findShortestPath(
    std::string_view from_pk,
    std::string_view to_pk,
    std::string_view graph_id
) const {
    if (!db_.isOpen()) {
        return {Status::Error("findShortestPath: Database not open"), {}};
    }

    // Check if both nodes exist
    std::string fromKey = makeNodeKey_(graph_id, from_pk);
    std::string toKey = makeNodeKey_(graph_id, to_pk);
    
    if (!db_.get(fromKey).has_value()) {
        return {Status::Error("findShortestPath: Start node not found"), {}};
    }
    if (!db_.get(toKey).has_value()) {
        return {Status::Error("findShortestPath: Target node not found"), {}};
    }

    // BFS with parent tracking
    std::unordered_set<std::string> visited;
    std::unordered_map<std::string, std::string> parent;
    std::queue<std::string> queue;
    
    std::string from_str(from_pk);
    std::string to_str(to_pk);
    
    queue.push(from_str);
    visited.insert(from_str);
    parent[from_str] = "";  // Root has no parent
    
    bool found = false;
    
    while (!queue.empty() && !found) {
        std::string current = queue.front();
        queue.pop();
        
        if (current == to_str) {
            found = true;
            break;
        }
        
        // Get outgoing edges
        std::ostringstream outPrefix;
        outPrefix << "graph:out:" << graph_id << ":" << current << ":";
        
        db_.scanPrefix(outPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
            std::string neighbor(val);
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                parent[neighbor] = current;
                queue.push(neighbor);
                
                if (neighbor == to_str) {
                    found = true;
                    return false;  // Stop scanning
                }
            }
            return true;
        });
    }
    
    if (!found) {
        return {Status::Error("findShortestPath: No path exists"), {}};
    }
    
    // Reconstruct path
    std::vector<std::string> path;
    std::string current = to_str;
    
    while (!current.empty()) {
        path.push_back(current);
        current = parent[current];
    }
    
    std::reverse(path.begin(), path.end());
    return {Status::OK(), path};
}

std::pair<PropertyGraphManager::Status, BaseEntity>
PropertyGraphManager::getNode(
    std::string_view pk,
    std::string_view graph_id
) const {
    if (!db_.isOpen()) {
        return {Status::Error("getNode: Database not open"), BaseEntity("")};
    }

    std::string nodeKey = makeNodeKey_(graph_id, pk);
    auto blob = db_.get(nodeKey);
    
    if (!blob.has_value()) {
        return {Status::Error("getNode: Node not found"), BaseEntity("")};
    }

    BaseEntity node = BaseEntity::deserialize(std::string(pk), *blob);
    return {Status::OK(), node};
}

std::pair<PropertyGraphManager::Status, BaseEntity>
PropertyGraphManager::getEdge(
    std::string_view edgeId,
    std::string_view graph_id
) const {
    if (!db_.isOpen()) {
        return {Status::Error("getEdge: Database not open"), BaseEntity("")};
    }

    std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
    auto blob = db_.get(edgeKey);
    
    if (!blob.has_value()) {
        return {Status::Error("getEdge: Edge not found"), BaseEntity("")};
    }

    BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
    return {Status::OK(), edge};
}

std::pair<PropertyGraphManager::Status, std::vector<PropertyGraphManager::EdgeInfo>>
PropertyGraphManager::getOutgoingEdges(
    std::string_view fromPk,
    std::string_view graph_id
) const {
    if (!db_.isOpen()) {
        return {Status::Error("getOutgoingEdges: Database not open"), {}};
    }

    std::vector<EdgeInfo> edges;
    std::ostringstream outPrefix;
    outPrefix << "graph:out:" << graph_id << ":" << fromPk << ":";
    
    db_.scanPrefix(outPrefix.str(), [this, &edges, &fromPk, &graph_id](std::string_view key, std::string_view val) {
        // Extract edgeId from key: graph:out:<graph_id>:<from_pk>:<edgeId>
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) return true;
        
        std::string edgeId = keyStr.substr(lastColon + 1);
        std::string toPk(val);
        
        // Load edge to get type
        std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
        auto blob = db_.get(edgeKey);
        if (!blob.has_value()) return true;
        
        BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
        std::optional<std::string> typeOpt = extractType_(edge);
        
        edges.push_back({
            edgeId,
            std::string(fromPk),
            toPk,
            typeOpt.value_or(""),
            std::string(graph_id)
        });
        
        return true;
    });
    
    return {Status::OK(), edges};
}

std::pair<PropertyGraphManager::Status, std::vector<PropertyGraphManager::EdgeInfo>>
PropertyGraphManager::getIncomingEdges(
    std::string_view toPk,
    std::string_view graph_id
) const {
    if (!db_.isOpen()) {
        return {Status::Error("getIncomingEdges: Database not open"), {}};
    }

    std::vector<EdgeInfo> edges;
    std::ostringstream inPrefix;
    inPrefix << "graph:in:" << graph_id << ":" << toPk << ":";
    
    db_.scanPrefix(inPrefix.str(), [this, &edges, &toPk, &graph_id](std::string_view key, std::string_view val) {
        // Extract edgeId from key: graph:in:<graph_id>:<to_pk>:<edgeId>
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) return true;
        
        std::string edgeId = keyStr.substr(lastColon + 1);
        std::string fromPk(val);
        
        // Load edge to get type
        std::string edgeKey = makeEdgeKey_(graph_id, edgeId);
        auto blob = db_.get(edgeKey);
        if (!blob.has_value()) return true;
        
        BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
        std::optional<std::string> typeOpt = extractType_(edge);
        
        edges.push_back({
            edgeId,
            fromPk,
            std::string(toPk),
            typeOpt.value_or(""),
            std::string(graph_id)
        });
        
        return true;
    });
    
    return {Status::OK(), edges};
}

// ===== Graph Analytics =====

std::pair<PropertyGraphManager::Status, std::map<std::string, double>>
PropertyGraphManager::computePageRank(
    std::string_view graph_id,
    double damping_factor,
    int max_iterations,
    double tolerance
) const {
    if (!db_.isOpen()) {
        return {Status::Error("computePageRank: Database not open"), {}};
    }

    // Collect all nodes in the graph
    std::vector<std::string> nodes;
    std::ostringstream nodePrefix;
    nodePrefix << "node:" << graph_id << ":";
    
    db_.scanPrefix(nodePrefix.str(), [&nodes, &nodePrefix](std::string_view key, std::string_view /*val*/) {
        std::string keyStr(key);
        // Extract node PK from key: node:<graph_id>:<pk>
        size_t prefixLen = nodePrefix.str().size();
        if (keyStr.size() > prefixLen) {
            std::string pk = keyStr.substr(prefixLen);
            nodes.push_back(pk);
        }
        return true;
    });
    
    if (nodes.empty()) {
        return {Status::Error("computePageRank: No nodes in graph"), {}};
    }
    
    size_t N = nodes.size();
    
    // Build adjacency information
    // outgoing_count: number of outgoing edges from each node
    // incoming_nodes: for each node, which nodes point to it
    std::unordered_map<std::string, int> outgoing_count;
    std::unordered_map<std::string, std::vector<std::string>> incoming_nodes;
    
    // Initialize
    for (const auto& node : nodes) {
        outgoing_count[node] = 0;
        incoming_nodes[node] = {};
    }
    
    // Count outgoing edges and build incoming edge lists
    for (const auto& node : nodes) {
        std::ostringstream outPrefix;
        outPrefix << "graph:out:" << graph_id << ":" << node << ":";
        
        db_.scanPrefix(outPrefix.str(), [&](std::string_view /*key*/, std::string_view val) {
            std::string to_node(val);
            outgoing_count[node]++;
            incoming_nodes[to_node].push_back(node);
            return true;
        });
    }
    
    // Initialize PageRank scores (uniform distribution)
    std::map<std::string, double> pagerank;
    std::map<std::string, double> pagerank_new;
    
    double initial_score = 1.0 / N;
    for (const auto& node : nodes) {
        pagerank[node] = initial_score;
        pagerank_new[node] = 0.0;
    }
    
    // Iterate until convergence or max iterations
    bool converged = false;
    for (int iter = 0; iter < max_iterations && !converged; ++iter) {
        // Compute new PageRank scores
        for (const auto& node : nodes) {
            {
                double sum = 0.0;
                
                // Sum contributions from incoming nodes
                for (const auto& in_node : incoming_nodes[node]) {
                    int out_count = outgoing_count[in_node];
                    if (out_count > 0) {
                        sum += pagerank[in_node] / out_count;
                    }
                }
                
                // PageRank formula: PR(node) = (1-d)/N + d * sum(PR(in_node) / out_degree(in_node))
                pagerank_new[node] = (1.0 - damping_factor) / N + damping_factor * sum;
            }
        }
        
        // Check convergence
        {
            double diff = 0.0;
            for (const auto& node : nodes) {
                diff += std::abs(pagerank_new[node] - pagerank[node]);
        }
        
        if (diff < tolerance) {
            converged = true;
        }
        }
        
        // Update scores
        pagerank = pagerank_new;
    }
    
    return {Status::OK(), pagerank};
}

} // namespace themis

