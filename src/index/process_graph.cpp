/**
 * @file process_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=0, M=44, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Process Graph Manager Implementation
// Supports BPMN, EPK, and advanced process modeling patterns

#include "index/process_graph.h"
#include <stdexcept>
#include "index/edge_types.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "utils/logger.h"

#include <queue>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <unordered_set>

namespace themis {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

static constexpr double kPi = 3.14159265358979323846;

inline nlohmann::json parseJsonObjectOrEmpty(
    const std::optional<std::string>& raw,
    std::string_view context,
    std::string_view fieldName
) {
    nlohmann::json parsed = nlohmann::json::object();
    if (!raw || raw->empty()) {
        return parsed;
    }

    try {
        parsed = nlohmann::json::parse(*raw);
    } catch (const std::exception& e) {
        THEMIS_DEBUG(
            "ProcessGraphManager::{} failed to parse JSON field '{}': {}",
            context, fieldName, e.what());
        return nlohmann::json::object();
    }

    if (!parsed.is_object()) {
        THEMIS_DEBUG(
            "ProcessGraphManager::{} ignored non-object JSON field '{}'",
            context, fieldName);
        return nlohmann::json::object();
    }

    return parsed;
}

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << dis(gen);
    oss << std::setw(16) << dis(gen);
    return oss.str();
}

int64_t currentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string bpmnNodeTypeToString(BPMNNodeType type) {
    switch (type) {
        case BPMNNodeType::START_EVENT: return "START_EVENT";
        case BPMNNodeType::END_EVENT: return "END_EVENT";
        case BPMNNodeType::INTERMEDIATE_EVENT: return "INTERMEDIATE_EVENT";
        case BPMNNodeType::BOUNDARY_EVENT: return "BOUNDARY_EVENT";
        case BPMNNodeType::TASK: return "TASK";
        case BPMNNodeType::SUBPROCESS: return "SUBPROCESS";
        case BPMNNodeType::CALL_ACTIVITY: return "CALL_ACTIVITY";
        case BPMNNodeType::EXCLUSIVE_GATEWAY: return "EXCLUSIVE_GATEWAY";
        case BPMNNodeType::PARALLEL_GATEWAY: return "PARALLEL_GATEWAY";
        case BPMNNodeType::INCLUSIVE_GATEWAY: return "INCLUSIVE_GATEWAY";
        case BPMNNodeType::EVENT_BASED_GATEWAY: return "EVENT_BASED_GATEWAY";
        case BPMNNodeType::COMPLEX_GATEWAY: return "COMPLEX_GATEWAY";
        case BPMNNodeType::POOL: return "POOL";
        case BPMNNodeType::LANE: return "LANE";
        case BPMNNodeType::DATA_OBJECT: return "DATA_OBJECT";
        case BPMNNodeType::DATA_STORE: return "DATA_STORE";
        case BPMNNodeType::GROUP: return "GROUP";
        case BPMNNodeType::ANNOTATION: return "ANNOTATION";
    }
    return "UNKNOWN";
}

std::string epkNodeTypeToString(EPKNodeType type) {
    switch (type) {
        case EPKNodeType::EVENT: return "EVENT";
        case EPKNodeType::FUNCTION: return "FUNCTION";
        case EPKNodeType::AND_CONNECTOR: return "AND_CONNECTOR";
        case EPKNodeType::OR_CONNECTOR: return "OR_CONNECTOR";
        case EPKNodeType::XOR_CONNECTOR: return "XOR_CONNECTOR";
        case EPKNodeType::ORGANIZATIONAL_UNIT: return "ORGANIZATIONAL_UNIT";
        case EPKNodeType::INFORMATION_OBJECT: return "INFORMATION_OBJECT";
        case EPKNodeType::APPLICATION_SYSTEM: return "APPLICATION_SYSTEM";
        case EPKNodeType::PROCESS_PATH: return "PROCESS_PATH";
    }
    return "UNKNOWN";
}

std::string processEdgeTypeToString(ProcessEdgeType type) {
    switch (type) {
        case ProcessEdgeType::SEQUENCE_FLOW: return "SEQUENCE_FLOW";
        case ProcessEdgeType::MESSAGE_FLOW: return "MESSAGE_FLOW";
        case ProcessEdgeType::ASSOCIATION: return "ASSOCIATION";
        case ProcessEdgeType::DATA_ASSOCIATION: return "DATA_ASSOCIATION";
        case ProcessEdgeType::CONTROL_FLOW: return "CONTROL_FLOW";
        case ProcessEdgeType::INFORMATION_FLOW: return "INFORMATION_FLOW";
        case ProcessEdgeType::ORGANIZATION_FLOW: return "ORGANIZATION_FLOW";
        case ProcessEdgeType::DEFAULT_FLOW: return "DEFAULT_FLOW";
        case ProcessEdgeType::CONDITIONAL_FLOW: return "CONDITIONAL_FLOW";
        case ProcessEdgeType::EXCEPTION_FLOW: return "EXCEPTION_FLOW";
    }
    return "UNKNOWN";
}

bool isGatewayNode(const ProcessNodeInfo& node) {
    if (std::holds_alternative<BPMNNodeType>(node.node_type)) {
        auto type = std::get<BPMNNodeType>(node.node_type);
        return type == BPMNNodeType::EXCLUSIVE_GATEWAY ||
               type == BPMNNodeType::PARALLEL_GATEWAY ||
               type == BPMNNodeType::INCLUSIVE_GATEWAY ||
               type == BPMNNodeType::EVENT_BASED_GATEWAY ||
               type == BPMNNodeType::COMPLEX_GATEWAY;
    }
    if (std::holds_alternative<EPKNodeType>(node.node_type)) {
        auto type = std::get<EPKNodeType>(node.node_type);
        return type == EPKNodeType::AND_CONNECTOR ||
               type == EPKNodeType::OR_CONNECTOR ||
               type == EPKNodeType::XOR_CONNECTOR;
    }
    return false;
}

/**
 * @brief Serialize a ProcessGraphVisitLog to a JSON string (node_id -> ns since epoch).
 */
std::string serializeVisitTimestamps(const ProcessGraphVisitLog& log) {
    nlohmann::json obj = nlohmann::json::object();
    for (const auto& [node, tp] : log) {
        int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            tp.time_since_epoch()).count();
        obj[node] = ns;
    }
    return obj.dump();
}

/**
 * @brief Deserialize a ProcessGraphVisitLog from a JSON string.
 */
ProcessGraphVisitLog deserializeVisitTimestamps(const std::string& s) {
    ProcessGraphVisitLog log;
    try {
        auto obj = nlohmann::json::parse(s);
        if (obj.is_object()) {
            for (const auto& [node, val] : obj.items()) {
                if (val.is_number_integer()) {
                    int64_t ns = val.get<int64_t>();
                    const auto dur = std::chrono::duration_cast<std::chrono::system_clock::duration>(
                        std::chrono::nanoseconds{ns});
                    log[node] = std::chrono::system_clock::time_point{dur};
                }
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        THEMIS_WARN("ProcessGraph: failed to deserialize visit_timestamps: {}", e.what());
    }
    return log;
}

/**
 * @brief Serialize visited_nodes vector to a JSON array string.
 */
std::string serializeVisitedNodes(const std::vector<std::string>& nodes) {
    nlohmann::json arr = nodes;
    return arr.dump();
}

/**
 * @brief Deserialize visited_nodes vector from a JSON array string.
 */
std::vector<std::string> deserializeVisitedNodes(const std::string& s) {
    std::vector<std::string> nodes;
    try {
        auto arr = nlohmann::json::parse(s);
        if (arr.is_array()) {
            nodes.reserve(arr.size());
            for (const auto& item : arr) {
                if (item.is_string()) {
                    nodes.push_back(item.get<std::string>());
                }
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        THEMIS_WARN("ProcessGraph: failed to deserialize visited_nodes: {}", e.what());
    }
    return nodes;
}

bool isStartNode(const ProcessNodeInfo& node) {
    if (std::holds_alternative<BPMNNodeType>(node.node_type)) {
        return std::get<BPMNNodeType>(node.node_type) == BPMNNodeType::START_EVENT;
    }
    // EPK: First event is typically the start
    return false;
}

bool isEndNode(const ProcessNodeInfo& node) {
    if (std::holds_alternative<BPMNNodeType>(node.node_type)) {
        return std::get<BPMNNodeType>(node.node_type) == BPMNNodeType::END_EVENT;
    }
    // EPK: Last event is typically the end
    return false;
}

/**
 * @brief Evaluate a simple condition expression against process variables
 * 
 * Supports expressions like:
 * - "amount > 1000"
 * - "status == 'approved'"
 * - "count <= 5"
 * - "approved && amount > 500"
 * 
 * @param condition The condition expression to evaluate
 * @param variables The process variables (JSON object)
 * @return true if condition is satisfied, false otherwise
 */
bool evaluateCondition(const std::string& condition, const nlohmann::json& variables) {
    if (condition.empty()) {
        return true; // Empty condition always true
    }
    
    std::string expr = condition;
    
    // Remove leading/trailing whitespace
    expr.erase(0, expr.find_first_not_of(" \t\n\r"));
    expr.erase(expr.find_last_not_of(" \t\n\r") + 1);
    
    // Handle logical operators (&&, ||)
    size_t and_pos = expr.find("&&");
    if (and_pos != std::string::npos) {
        std::string left = expr.substr(0, and_pos);
        std::string right = expr.substr(and_pos + 2);
        return evaluateCondition(left, variables) && evaluateCondition(right, variables);
    }
    
    size_t or_pos = expr.find("||");
    if (or_pos != std::string::npos) {
        std::string left = expr.substr(0, or_pos);
        std::string right = expr.substr(or_pos + 2);
        return evaluateCondition(left, variables) || evaluateCondition(right, variables);
    }
    
    // Parse comparison operators
    std::string op = {};
    size_t op_pos = std::string::npos;
    
    // Check for == first (before single =)
    if (expr.find("==") != std::string::npos) {
        op = "==";
        op_pos = expr.find("==");
    } else if (expr.find("!=") != std::string::npos) {
        op = "!=";
        op_pos = expr.find("!=");
    } else if (expr.find("<=") != std::string::npos) {
        op = "<=";
        op_pos = expr.find("<=");
    } else if (expr.find(">=") != std::string::npos) {
        op = ">=";
        op_pos = expr.find(">=");
    } else if (expr.find("<") != std::string::npos) {
        op = "<";
        op_pos = expr.find("<");
    } else if (expr.find(">") != std::string::npos) {
        op = ">";
        op_pos = expr.find(">");
    } else {
        // No operator found, treat as boolean variable
        if (variables.contains(expr)) {
            auto val = variables[expr];
            if (val.is_boolean()) {
              return val.get<bool>();
            }
            if (val.is_number()) {
              return val.get<double>() != 0;
            }
            if (val.is_string()) {
              return !val.get<std::string>().empty();
            }
        }
        return false;
    }
    
    // Extract left and right operands
    std::string left = expr.substr(0, op_pos);
    std::string right = expr.substr(op_pos + op.size());
    
    // Trim whitespace
    left.erase(0, left.find_first_not_of(" \t"));
    left.erase(left.find_last_not_of(" \t") + 1);
    right.erase(0, right.find_first_not_of(" \t"));
    right.erase(right.find_last_not_of(" \t") + 1);
    
    // Get left value from variables
    if (!variables.contains(left)) {
        return false; // Variable not found
    }
    
    auto leftVal = variables[left];
    
    // Parse right value (could be literal or variable)
    nlohmann::json rightVal;
    if (right.front() == '\'' || right.front() == '"') {
        // String literal
        rightVal = right.substr(1, right.size() - 2);
    } else if (variables.contains(right)) {
        // Variable reference
        rightVal = variables[right];
    } else {
        // Numeric literal
        try {
            if (right.find('.') != std::string::npos) {
                rightVal = std::stod(right);
            } else {
                rightVal = std::stoll(right);
            }
        } catch (const std::invalid_argument&) {
            // Invalid numeric format
            return false;
        } catch (const std::out_of_range&) {
            // Number too large
            return false;
        } catch (...) {
            // Other unexpected errors
            return false;
        }
    }
    
    // Perform comparison
    if (op == "==") {
        return leftVal == rightVal;
    } else if (op == "!=") {
        return leftVal != rightVal;
    } else if (op == "<") {
        if (leftVal.is_number() && rightVal.is_number()) {
            return leftVal.get<double>() < rightVal.get<double>();
        }
        return false;
    } else if (op == ">") {
        if (leftVal.is_number() && rightVal.is_number()) {
            return leftVal.get<double>() > rightVal.get<double>();
        }
        return false;
    } else if (op == "<=") {
        if (leftVal.is_number() && rightVal.is_number()) {
            return leftVal.get<double>() <= rightVal.get<double>();
        }
        return false;
    } else if (op == ">=") {
        if (leftVal.is_number() && rightVal.is_number()) {
            return leftVal.get<double>() >= rightVal.get<double>();
        }
        return false;
    }
    
    return false;
}

} // anonymous namespace

// ============================================================================
// ProcessGraphManager Implementation
// ============================================================================

ProcessGraphManager::ProcessGraphManager(RocksDBWrapper& db) : db_(db) {}

void ProcessGraphManager::setEmbedder(
    std::function<std::vector<float>(std::string_view)> embedder)
{
    embedder_ = std::move(embedder);
}

std::string ProcessGraphManager::makeProcessKey_(std::string_view process_id) const {
    return std::string("process:def:") + std::string(process_id);
}

std::string ProcessGraphManager::makeNodeKey_(std::string_view process_id, std::string_view node_id) const {
    return std::string("process:node:") + std::string(process_id) + ":" + std::string(node_id);
}

std::string ProcessGraphManager::makeEdgeKey_(std::string_view process_id, std::string_view edge_id) const {
    return std::string("process:edge:") + std::string(process_id) + ":" + std::string(edge_id);
}

std::string ProcessGraphManager::makeHyperedgeKey_(std::string_view process_id, std::string_view hyperedge_id) const {
    return std::string("process:hyperedge:") + std::string(process_id) + ":" + std::string(hyperedge_id);
}

std::string ProcessGraphManager::makeInstanceKey_(std::string_view instance_id) const {
    return std::string("process:instance:") + std::string(instance_id);
}

std::string ProcessGraphManager::makeTokenKey_(std::string_view instance_id, std::string_view token_id) const {
    return std::string("process:token:") + std::string(instance_id) + ":" + std::string(token_id);
}

std::string ProcessGraphManager::generateTokenId_() const {
    return "token-" + generateUUID().substr(0, 16);
}

// ===== Process Model Management =====

ProcessGraphManager::Status ProcessGraphManager::registerProcess(
    std::string_view process_id, 
    std::string_view name,
    std::string_view bpmn_xml
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }
    if (process_id.empty()) {
        return Status::Error("Process ID cannot be empty");
    }

    BaseEntity::FieldMap fields;
    fields["id"] = std::string(process_id);
    fields["name"] = std::string(name);
    fields["created_at"] = static_cast<int64_t>(currentTimeMs());
    if (!bpmn_xml.empty()) {
        fields["bpmn_xml"] = std::string(bpmn_xml);
    }

    // Auto-generate description embedding when an embedder is wired.
    if (embedder_) {
        const std::string embed_text =
            std::string(name) + (bpmn_xml.empty() ? "" : " " + std::string(bpmn_xml).substr(
                0, std::min<size_t>(bpmn_xml.size(), 2048u)));
        try {
            std::vector<float> emb = embedder_(embed_text);
            if (!emb.empty()) {
                // Serialise as JSON array string.
                nlohmann::json ja(emb);
                fields["embedding"] = ja.dump();
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("ProcessGraph: embedding generation failed for '{}': {}",
                        process_id, e.what());
        }
    }

    BaseEntity processEntity = BaseEntity::fromFields(std::string(process_id), fields);

    std::string key = makeProcessKey_(process_id);
    if (!db_.put(key, processEntity.serialize())) {
        return Status::Error("Failed to store process definition");
    }

    THEMIS_INFO("Registered process: {} ({})", process_id, name);
    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::addProcessNode(
    std::string_view process_id,
    const ProcessNodeInfo& node
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }

    // Build entity from ProcessNodeInfo
    BaseEntity entity(node.node_id);
    entity.setField("id", node.node_id);
    entity.setField("name", node.name);
    entity.setField("description", node.description);
    entity.setField("subtype", node.subtype);
    
    // Store node type
    if (std::holds_alternative<BPMNNodeType>(node.node_type)) {
        entity.setField("node_category", "BPMN");
        entity.setField("node_type", bpmnNodeTypeToString(std::get<BPMNNodeType>(node.node_type)));
    } else {
        entity.setField("node_category", "EPK");
        entity.setField("node_type", epkNodeTypeToString(std::get<EPKNodeType>(node.node_type)));
    }

    // Execution properties
    entity.setField("is_async", node.is_async);
    entity.setField("is_multi_instance", node.is_multi_instance);
    if (node.loop_cardinality) {
        entity.setField("loop_cardinality", static_cast<int64_t>(*node.loop_cardinality));
    }
    if (node.timeout) {
        entity.setField("timeout_ms", static_cast<int64_t>(node.timeout->count()));
    }
    entity.setField("max_retries", static_cast<int64_t>(node.max_retries));

    // Script/service config
    if (node.script) {
        entity.setField("script", *node.script);
    }
    if (node.service_ref) {
        entity.setField("service_ref", *node.service_ref);
    }
    if (node.implementation) {
        entity.setField("implementation", *node.implementation);
    }

    // Store
    std::string key = makeNodeKey_(process_id, node.node_id);
    if (!db_.put(key, entity.serialize())) {
        return Status::Error("Failed to store process node");
    }

    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::addProcessEdge(
    std::string_view process_id,
    const ProcessEdgeInfo& edge
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }

    BaseEntity entity(edge.edge_id);
    entity.setField("id", edge.edge_id);
    entity.setField("_from", edge.from_node);
    entity.setField("_to", edge.to_node);
    entity.setField("_type", processEdgeTypeToString(edge.edge_type));
    entity.setField("priority", static_cast<int64_t>(edge.priority));
    entity.setField("is_default", edge.is_default);

    if (edge.condition_expression) {
        entity.setField("condition", *edge.condition_expression);
    }
    if (edge.message_name) {
        entity.setField("message_name", *edge.message_name);
    }
    if (edge.min_delay) {
        entity.setField("min_delay_ms", static_cast<int64_t>(edge.min_delay->count()));
    }
    if (edge.max_delay) {
        entity.setField("max_delay_ms", static_cast<int64_t>(edge.max_delay->count()));
    }

    std::string key = makeEdgeKey_(process_id, edge.edge_id);
    if (!db_.put(key, entity.serialize())) {
        return Status::Error("Failed to store process edge");
    }

    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::addHyperedge(
    std::string_view process_id,
    const Hyperedge& hyperedge
) {
    if (!db_.isOpen()) {
        return Status::Error("Database not open");
    }

    BaseEntity entity(hyperedge.hyperedge_id);
    entity.setField("id", hyperedge.hyperedge_id);
    entity.setField("name", hyperedge.name);
    
    // Store sources and targets as JSON arrays
    nlohmann::json sources = hyperedge.source_nodes;
    nlohmann::json targets = hyperedge.target_nodes;
    entity.setField("sources", sources.dump());
    entity.setField("targets", targets.dump());

    // Sync type
    std::string syncTypeStr = {};
    switch (hyperedge.sync_type) {
        case Hyperedge::SyncType::AND_JOIN: syncTypeStr = "AND_JOIN"; break;
        case Hyperedge::SyncType::AND_SPLIT: syncTypeStr = "AND_SPLIT"; break;
        case Hyperedge::SyncType::OR_JOIN: syncTypeStr = "OR_JOIN"; break;
        case Hyperedge::SyncType::N_OF_M_JOIN: syncTypeStr = "N_OF_M_JOIN"; break;
        case Hyperedge::SyncType::DISCRIMINATOR: syncTypeStr = "DISCRIMINATOR"; break;
    }
    entity.setField("sync_type", syncTypeStr);

    if (hyperedge.required_count) {
        entity.setField("required_count", static_cast<int64_t>(*hyperedge.required_count));
    }

    std::string key = makeHyperedgeKey_(process_id, hyperedge.hyperedge_id);
    if (!db_.put(key, entity.serialize())) {
        return Status::Error("Failed to store hyperedge");
    }

    return Status::OK();
}

std::pair<ProcessGraphManager::Status, ProcessGraphManager::ValidationResult> 
ProcessGraphManager::validateProcess(std::string_view process_id) const {
    ValidationResult result = {};
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), result};
    }

    // Collect all nodes
    std::unordered_map<std::string, ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;
    
    std::string nodePrefix = "process:node:" + std::string(process_id) + ":";
    db_.scanPrefix(nodePrefix, [&nodes](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string nodeId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity entity = BaseEntity::deserialize(nodeId, blob);
            
            ProcessNodeInfo info;
            info.node_id = nodeId;
            info.name = entity.getFieldAsString("name").value_or("");
            info.description = entity.getFieldAsString("description").value_or("");
            
            // Reconstruct node type
            auto category = entity.getFieldAsString("node_category").value_or("BPMN");
            auto typeStr = entity.getFieldAsString("node_type").value_or("");
            
            if (category == "BPMN") {
                if (typeStr == "START_EVENT") {
                  info.node_type = BPMNNodeType::START_EVENT;
                }
                else if (typeStr == "END_EVENT") info.node_type = BPMNNodeType::END_EVENT;
                else if (typeStr == "TASK") info.node_type = BPMNNodeType::TASK;
                else if (typeStr == "EXCLUSIVE_GATEWAY") info.node_type = BPMNNodeType::EXCLUSIVE_GATEWAY;
                else if (typeStr == "PARALLEL_GATEWAY") info.node_type = BPMNNodeType::PARALLEL_GATEWAY;
                else info.node_type = BPMNNodeType::TASK; // Default
            } else {
                if (typeStr == "EVENT") {
                  info.node_type = EPKNodeType::EVENT;
                }
                else if (typeStr == "FUNCTION") info.node_type = EPKNodeType::FUNCTION;
                else if (typeStr == "AND_CONNECTOR") info.node_type = EPKNodeType::AND_CONNECTOR;
                else if (typeStr == "OR_CONNECTOR") info.node_type = EPKNodeType::OR_CONNECTOR;
                else if (typeStr == "XOR_CONNECTOR") info.node_type = EPKNodeType::XOR_CONNECTOR;
                else info.node_type = EPKNodeType::FUNCTION; // Default
            }
            
            nodes[nodeId] = info;
        }
        return true;
    });

    std::string edgePrefix = "process:edge:" + std::string(process_id) + ":";
    db_.scanPrefix(edgePrefix, [&edges](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string edgeId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity entity = BaseEntity::deserialize(edgeId, blob);
            
            ProcessEdgeInfo info;
            info.edge_id = edgeId;
            info.edge_type = ProcessEdgeType::SEQUENCE_FLOW;  // Default edge type
            info.from_node = entity.getFieldAsString("_from").value_or("");
            info.to_node = entity.getFieldAsString("_to").value_or("");
            info.is_default = entity.getFieldAsBool("is_default").value_or(false);
            
            edges.push_back(info);
        }
        return true;
    });

    // Validation checks
    
    // Pre-allocate result vectors based on worst-case sizes to avoid repeated reallocations
    result.errors.reserve(nodes.size() + edges.size());
    result.warnings.reserve(nodes.size());

    // 1. Check for start node
    bool hasStart = false;
    bool hasEnd = false;
    for (const auto& [id, node] : nodes) {
        if (isStartNode(node)) {
          hasStart = true;
        }
        if (isEndNode(node)) {
          hasEnd = true;
        }
    }
    if (!hasStart) {
        result.errors.push_back([[maybe_unused]] "Process has no start event");
    }
    if (!hasEnd) {
        result.warnings.push_back([[maybe_unused]] "Process has no end event");
    }

    // 2. Check for orphan nodes (no incoming or outgoing edges)
    std::unordered_set<std::string> hasIncoming;
    std::unordered_set<std::string> hasOutgoing = {};

    for (const auto& edge : edges) {
        hasOutgoing.insert(edge.from_node);
        hasIncoming.insert(edge.to_node);
    }
    for (const auto& [id, node] : nodes) {
        if (!isStartNode(node) && hasIncoming.find(id) == hasIncoming.end()) {
            result.errors.push_back("Node '" + id + "' has no incoming edges");
        }
        if (!isEndNode(node) && hasOutgoing.find(id) == hasOutgoing.end()) {
            result.errors.push_back("Node '" + id + "' has no outgoing edges");
        }
    }

    // 3. Check for edge targets that don't exist
    for (const auto& edge : edges) {
        if (nodes.find(edge.from_node) == nodes.end()) {
            result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_node + "'");
        }
        if (nodes.find(edge.to_node) == nodes.end()) {
            result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent target '" + edge.to_node + "'");
        }
    }

    // 4. Check gateway rules
    for (const auto& [id, node] : nodes) {
        if (isGatewayNode(node)) {
            // Count incoming and outgoing edges
            int incoming = 0, outgoing = 0;
            for (const auto& edge : edges) {
                if (edge.to_node == id) {
                  incoming++;
                }
                if (edge.from_node == id) {
                  outgoing++;
                }
            }

            if (std::holds_alternative<BPMNNodeType>(node.node_type)) {
                auto type = std::get<BPMNNodeType>(node.node_type);
                
                // XOR gateway should have at least 2 outgoing for split
                if (type == BPMNNodeType::EXCLUSIVE_GATEWAY && outgoing > 1) {
                    // Check for default flow
                    bool hasDefault = false;
                    for (const auto& edge : edges) {
                        if (edge.from_node == id && edge.is_default) {
                            hasDefault = true;
                            break;
                        }
                    }
                    if (!hasDefault) {
                        result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
                    }
                }
            }
        }
    }

    result.is_valid = result.errors.empty();
    return {Status::OK(), result};
}

// ===== Process Execution =====

std::pair<ProcessGraphManager::Status, std::string> ProcessGraphManager::startProcess(
    std::string_view process_id,
    const nlohmann::json& initial_variables
) {
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), ""};
    }

    // Find start event
    std::string startNodeId = {};
    std::string nodePrefix = "process:node:" + std::string(process_id) + ":";
    db_.scanPrefix(nodePrefix, [&startNodeId](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string nodeId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity entity = BaseEntity::deserialize(nodeId, blob);
            
            auto nodeType = entity.getFieldAsString("node_type").value_or("");
            if ([[maybe_unused]] nodeType == "START_EVENT" || nodeType == "EVENT") {
                startNodeId = nodeId;
                return false; // Stop scanning
            }
        }
        return true;
    });

    if (startNodeId.empty()) {
        return {Status::Error("Process has no start event"), ""};
    }

    // Create process instance
    std::string instanceId = "inst-" + generateUUID().substr(0, 16);
    
    ProcessInstance instance;
    instance.instance_id = instanceId;
    instance.process_definition_id = std::string(process_id);
    instance.state = ProcessInstance::State::RUNNING;
    instance.variables = initial_variables;
    instance.started_at_ms = currentTimeMs();

    // Create initial token at start event
    ProcessToken token;
    token.token_id = generateTokenId_();
    token.process_instance_id = instanceId;
    token.current_node = startNodeId;
    token.state = ProcessToken::State::READY;
    token.created_at_ms = currentTimeMs();
    token.variables = initial_variables;
    token.visited_nodes.push_back(startNodeId);
    token.visit_timestamps[startNodeId] = std::chrono::system_clock::now();

    instance.tokens.push_back(token);

    // Store instance
    BaseEntity instanceEntity(instanceId);
    instanceEntity.setField("id", instanceId);
    instanceEntity.setField("process_id", std::string(process_id));
    instanceEntity.setField("state", "RUNNING");
    instanceEntity.setField("started_at", instance.started_at_ms);
    instanceEntity.setField("variables", initial_variables.dump());

    std::string instanceKey = makeInstanceKey_(instanceId);
    if (!db_.put(instanceKey, instanceEntity.serialize())) {
        return {Status::Error("Failed to store process instance"), ""};
    }

    // Store token
    BaseEntity tokenEntity(token.token_id);
    tokenEntity.setField("id", token.token_id);
    tokenEntity.setField("instance_id", instanceId);
    tokenEntity.setField("current_node", startNodeId);
    tokenEntity.setField("state", "READY");
    tokenEntity.setField("created_at", token.created_at_ms);
    tokenEntity.setField("visited_nodes", serializeVisitedNodes(token.visited_nodes));
    tokenEntity.setField("visit_timestamps", serializeVisitTimestamps(token.visit_timestamps));

    std::string tokenKey = makeTokenKey_(instanceId, token.token_id);
    if (!db_.put(tokenKey, tokenEntity.serialize())) {
        return {Status::Error("Failed to store token"), ""};
    }

    THEMIS_INFO("Started process instance: {} for process {}", instanceId, process_id);
    return {Status::OK(), instanceId};
}

std::pair<ProcessGraphManager::Status, ProcessInstance> 
ProcessGraphManager::getProcessInstance(std::string_view instance_id) const {
    ProcessInstance instance = {};
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), instance};
    }

    std::string instanceKey = makeInstanceKey_(instance_id);
    auto blob = db_.get(instanceKey);
    if (!blob) {
        return {Status::Error("Process instance not found"), instance};
    }

    BaseEntity entity = BaseEntity::deserialize(std::string(instance_id), *blob);
    instance.instance_id = std::string(instance_id);
    instance.process_definition_id = entity.getFieldAsString("process_id").value_or("");
    instance.started_at_ms = entity.getFieldAsInt("started_at").value_or(0);
    
    auto stateStr = entity.getFieldAsString("state").value_or("CREATED");
    if (stateStr == "RUNNING") {
      instance.state = ProcessInstance::State::RUNNING;
    }
    else if (stateStr == "COMPLETED") instance.state = ProcessInstance::State::COMPLETED;
    else if (stateStr == "SUSPENDED") instance.state = ProcessInstance::State::SUSPENDED;
    else if (stateStr == "TERMINATED") instance.state = ProcessInstance::State::TERMINATED;
    else if (stateStr == "FAILED") instance.state = ProcessInstance::State::FAILED;

    auto varsStr = entity.getFieldAsString("variables");
    if (varsStr) {
        try {
            instance.variables = nlohmann::json::parse(*varsStr);
        } catch (...) {
            instance.variables = nlohmann::json::object();
        }
    }

    // Load tokens
    std::string tokenPrefix = "process:token:" + std::string(instance_id) + ":";
    db_.scanPrefix(tokenPrefix, [&instance](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string tokenId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);
            
            ProcessToken token;
            token.token_id = tokenId;
            token.process_instance_id = tokenEntity.getFieldAsString("instance_id").value_or("");
            token.current_node = tokenEntity.getFieldAsString("current_node").value_or("");
            token.created_at_ms = tokenEntity.getFieldAsInt("created_at").value_or(0);
            
            auto stateStr = tokenEntity.getFieldAsString("state").value_or("READY");
            if (stateStr == "READY") {
              token.state = ProcessToken::State::READY;
            }
            else if (stateStr == "ACTIVE") token.state = ProcessToken::State::ACTIVE;
            else if (stateStr == "COMPLETED") token.state = ProcessToken::State::COMPLETED;
            else if (stateStr == "WAITING") token.state = ProcessToken::State::WAITING;
            else if (stateStr == "FAILED") token.state = ProcessToken::State::FAILED;

            auto visitedNodesStr = tokenEntity.getFieldAsString("visited_nodes");
            if (visitedNodesStr) {
                token.visited_nodes = deserializeVisitedNodes(*visitedNodesStr);
            }

            auto visitTimestampsStr = tokenEntity.getFieldAsString("visit_timestamps");
            if (visitTimestampsStr) {
                token.visit_timestamps = deserializeVisitTimestamps(*visitTimestampsStr);
            }
            
            instance.tokens.push_back(token);
        }
        return true;
    });

    return {Status::OK(), instance};
}

std::optional<std::chrono::system_clock::time_point>
ProcessGraphManager::getVisitTimestamp(
    std::string_view instance_id,
    std::string_view node_id
) const {
    auto [st, instance] = getProcessInstance(instance_id);
    if (!st.ok) {
      return std::nullopt;
    }

    std::optional<std::chrono::system_clock::time_point> result = {};

    for (const auto& token : instance.tokens) {
        auto it = token.visit_timestamps.find(std::string(node_id));
        if (it != token.visit_timestamps.end()) {
            // Return the most recent timestamp across all tokens
            if (!result.has_value() || it->second > result.value()) {
                result = it->second;
            }
        }
    }
    return result;
}

ProcessGraphManager::Status ProcessGraphManager::advanceToken(
    std::string_view instance_id,
    std::string_view token_id
) {
    // Load instance and token
    auto [st, instance] = getProcessInstance(instance_id);
    if (!st.ok) {
      return st;
    }

    // Find the token
    ProcessToken* token = nullptr;
    for (auto& t : instance.tokens) {
        if (t.token_id == std::string(token_id)) {
            token = &t;
            break;
        }
    }
    if (!token) {
        return Status::Error("Token not found");
    }

    // Find outgoing edges from current node
    std::string edgePrefix = "process:edge:" + instance.process_definition_id + ":";
    std::vector<ProcessEdgeInfo> outgoing;
    
    db_.scanPrefix(edgePrefix, [&outgoing, &token](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string edgeId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity entity = BaseEntity::deserialize(edgeId, blob);
            
            auto from = entity.getFieldAsString("_from").value_or("");
            if (from == token->current_node) {
                ProcessEdgeInfo edge;
                edge.edge_id = edgeId;
                edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;  // Default edge type
                edge.from_node = from;
                edge.to_node = entity.getFieldAsString("_to").value_or("");
                edge.is_default = entity.getFieldAsBool("is_default").value_or(false);
                edge.priority = static_cast<int>(entity.getFieldAsInt("priority").value_or(0));
                auto cond = entity.getFieldAsString("condition");
                if (cond) {
                  edge.condition_expression = *cond;
                }
                outgoing.push_back(edge);
            }
        }
        return true;
    });

    if (outgoing.empty()) {
        // End of process path
        token->state = ProcessToken::State::COMPLETED;
        token->completed_at_ms = currentTimeMs();
        
        // Update token in DB — include visited_nodes and visit_timestamps so
        // history is preserved even after the token reaches a terminal node.
        BaseEntity::FieldMap fields;
        fields["id"] = std::string(token_id);
        fields["instance_id"] = std::string(instance_id);
        fields["current_node"] = token->current_node;
        fields["state"] = std::string("COMPLETED");
        fields["completed_at"] = static_cast<int64_t>(*token->completed_at_ms);
        fields["visited_nodes"] = serializeVisitedNodes(token->visited_nodes);
        fields["visit_timestamps"] = serializeVisitTimestamps(token->visit_timestamps);
        BaseEntity tokenEntity = BaseEntity::fromFields(std::string(token_id), fields);
        
        std::string tokenKey = makeTokenKey_(instance_id, token_id);
        db_.put(tokenKey, tokenEntity.serialize());

        // Check whether ALL tokens in the instance have now completed and, if so,
        // mark the instance COMPLETED and persist a summary embedding.
        const bool all_done = std::all_of(
            instance.tokens.begin(), instance.tokens.end(),
            [](const ProcessToken& t) {
                return t.state == ProcessToken::State::COMPLETED ||
                       t.state == ProcessToken::State::FAILED;
            });

        if (all_done) {
            // Persist instance state as COMPLETED.
            std::string instanceKey = makeInstanceKey_(instance_id);
            auto instBlob = db_.get(instanceKey);
            if (instBlob) {
                BaseEntity instEntity = BaseEntity::deserialize(
                    std::string(instance_id), *instBlob);
                instEntity.setField("state", "COMPLETED");
                instEntity.setField("completed_at",
                    static_cast<int64_t>(*token->completed_at_ms));
                db_.put(instanceKey, instEntity.serialize());
            }

            // Auto-generate instance embedding when an embedder is wired.
            if (embedder_) {
                // Build a compact summary text from visited nodes and variables.
                nlohmann::json summary;
                summary["instance_id"] = std::string(instance_id);
                summary["process_id"]  = instance.process_definition_id;
                std::vector<std::string> visited_all = {};

                for (const auto& t : instance.tokens) {
                    for (const auto& n : t.visited_nodes) {
                        visited_all.push_back(n);
                    }
                }
                summary["visited_nodes"] = visited_all;
                summary["variables"]     = instance.variables;

                try {
                    std::vector<float> emb = embedder_(summary.dump());
                    if (!emb.empty()) {
                        nlohmann::json ja(emb);
                        // Store under a dedicated key so it can be scanned
                        // by findSimilarTasks / ProcessGraphRag::findSimilarCases().
                        std::string embKey = "proc:inst_emb:" + std::string(instance_id);
                        db_.put(embKey, ja.dump());
                    }
                } catch (const std::exception& e) {
                    THEMIS_WARN("ProcessGraph: instance embedding failed for '{}': {}",
                                instance_id, e.what());
                }
            }
        }

        return Status::OK();
    }

    // Sort by priority
    std::sort(outgoing.begin(), outgoing.end(), 
        [](const ProcessEdgeInfo& a, const ProcessEdgeInfo& b) {
            return a.priority > b.priority;
        });

    // Evaluate conditions and select the first matching edge
    std::string targetNode = {};
    std::string defaultNode = {};
    
    for (const auto& edge : outgoing) {
        // Remember default edge
        if (edge.is_default) {
            defaultNode = edge.to_node;
        }
        
        // Evaluate condition if present
        if (edge.condition_expression.has_value()) {
            if (evaluateCondition(*edge.condition_expression, token->variables)) {
                targetNode = edge.to_node;
                token->traversed_edges.push_back(edge.edge_id);
                break;
            }
        } else {
            // No condition means always take this edge
            targetNode = edge.to_node;
            token->traversed_edges.push_back(edge.edge_id);
            break;
        }
    }

    // If no condition matched, use default edge
    if (targetNode.empty() && !defaultNode.empty()) {
        targetNode = defaultNode;
        for (const auto& edge : outgoing) {
            if (edge.is_default) {
                token->traversed_edges.push_back(edge.edge_id);
                break;
            }
        }
    }
    
    // If still no target, take first edge as fallback
    if (targetNode.empty() && !outgoing.empty()) {
        targetNode = outgoing[0].to_node;
        token->traversed_edges.push_back(outgoing[0].edge_id);
    }

    // Move token
    token->current_node = targetNode;
    token->visited_nodes.push_back(targetNode);
    token->visit_timestamps[targetNode] = std::chrono::system_clock::now();

    // Update token in DB
    BaseEntity::FieldMap fields2;
    fields2["id"] = std::string(token_id);
    fields2["instance_id"] = std::string(instance_id);
    fields2["current_node"] = token->current_node;
    fields2["state"] = std::string("READY");
    fields2["visited_nodes"] = serializeVisitedNodes(token->visited_nodes);
    fields2["visit_timestamps"] = serializeVisitTimestamps(token->visit_timestamps);
    BaseEntity tokenEntity2 = BaseEntity::fromFields(std::string(token_id), fields2);
    
    std::string tokenKey = makeTokenKey_(instance_id, token_id);
    db_.put(tokenKey, tokenEntity2.serialize());

    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::completeTask(
    std::string_view instance_id,
    std::string_view task_node,
    const nlohmann::json& output_variables
) {
    auto [st, instance] = getProcessInstance(instance_id);
    if (!st.ok) {
      return st;
    }

    // Find token at this task
    for (auto& token : instance.tokens) {
        if (token.current_node == std::string(task_node) && 
            token.state == ProcessToken::State::READY) {
            
            // Merge output variables
            for (auto& [key, val] : output_variables.items()) {
                token.variables[key] = val;
            }
            
            // Advance token
            return advanceToken(instance_id, token.token_id);
        }
    }

    return Status::Error("No active token at task node");
}

ProcessGraphManager::Status ProcessGraphManager::suspendProcess(std::string_view instance_id) {
    std::string instanceKey = makeInstanceKey_(instance_id);
    auto blob = db_.get(instanceKey);
    if (!blob) {
        return Status::Error("Process instance not found");
    }

    BaseEntity entity = BaseEntity::deserialize(std::string(instance_id), *blob);
    entity.setField("state", "SUSPENDED");
    
    if (!db_.put(instanceKey, entity.serialize())) {
        return Status::Error("Failed to update process instance");
    }

    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::resumeProcess(std::string_view instance_id) {
    std::string instanceKey = makeInstanceKey_(instance_id);
    auto blob = db_.get(instanceKey);
    if (!blob) {
        return Status::Error("Process instance not found");
    }

    BaseEntity entity = BaseEntity::deserialize(std::string(instance_id), *blob);
    entity.setField("state", "RUNNING");
    
    if (!db_.put(instanceKey, entity.serialize())) {
        return Status::Error("Failed to update process instance");
    }

    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::terminateProcess(
    std::string_view instance_id,
    std::string_view reason
) {
    std::string instanceKey = makeInstanceKey_(instance_id);
    auto blob = db_.get(instanceKey);
    if (!blob) {
        return Status::Error("Process instance not found");
    }

    BaseEntity entity = BaseEntity::deserialize(std::string(instance_id), *blob);
    entity.setField("state", "TERMINATED");
    entity.setField("completed_at", currentTimeMs());
    if (!reason.empty()) {
        entity.setField("termination_reason", std::string(reason));
    }
    
    if (!db_.put(instanceKey, entity.serialize())) {
        return Status::Error("Failed to update process instance");
    }

    THEMIS_INFO("Terminated process instance: {} reason: {}", instance_id, reason);
    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::signalEvent(
    std::string_view instance_id,
    std::string_view event_name,
    const nlohmann::json& payload
) {
    // Get process instance
    auto [st, instance] = getProcessInstance(instance_id);
    if (!st.ok) {
      return st;
    }
    
    // Find tokens waiting for this event
    // Note: We modify token fields but not the vector structure, so iteration is safe
    bool foundWaiting = false;
    
    for (auto& token : instance.tokens) {
        if (token.state != ProcessToken::State::WAITING) {
            continue;
        }
        
        // Get the current node to check if it's an event node
        std::string nodeKey = makeNodeKey_(instance.process_definition_id, token.current_node);
        auto nodeBlob = db_.get(nodeKey);
        if (!nodeBlob) {
          continue;
        }
        
        BaseEntity nodeEntity = BaseEntity::deserialize(token.current_node, *nodeBlob);
        auto nodeTypeStr = nodeEntity.getFieldAsString("node_type").value_or("");
        auto subtypeStr = nodeEntity.getFieldAsString("subtype").value_or("");
        
        // Check if this is a message or signal catching event
        bool isMatchingEvent = false;
        if ([[maybe_unused]] nodeTypeStr == "INTERMEDIATE_EVENT" || nodeTypeStr == "BOUNDARY_EVENT") {
            // Check if the event name matches
            auto eventNameField = nodeEntity.getFieldAsString([[maybe_unused]] "event_name");
            if ([[maybe_unused]] eventNameField && *eventNameField == std::string(event_name)) {
                isMatchingEvent = true;
            }
        }
        
        if (!isMatchingEvent) {
          continue;
        }
        
        // Merge event payload into token variables
        for (auto& [key, val] : payload.items()) {
            token.variables[key] = val;
        }
        
        // Set token to READY state
        token.state = ProcessToken::State::READY;
        
        // Update token in database — preserve visited_nodes and visit_timestamps
        BaseEntity tokenEntity(token.token_id);
        tokenEntity.setField("id", token.token_id);
        tokenEntity.setField("instance_id", std::string(instance_id));
        tokenEntity.setField("current_node", token.current_node);
        tokenEntity.setField("state", "READY");
        tokenEntity.setField("variables", token.variables.dump());
        tokenEntity.setField("visited_nodes", serializeVisitedNodes(token.visited_nodes));
        tokenEntity.setField("visit_timestamps", serializeVisitTimestamps(token.visit_timestamps));
        
        std::string tokenKey = makeTokenKey_(instance_id, token.token_id);
        if (!db_.put(tokenKey, tokenEntity.serialize())) {
            return Status::Error("Failed to update token");
        }
        
        foundWaiting = true;
        THEMIS_INFO("Event '{}' triggered token {} in instance {}", event_name, token.token_id, instance_id);
    }
    
    if (!foundWaiting) {
        return Status::Error([[maybe_unused]] "No tokens waiting for event '" + std::string(event_name) + "'");
    }
    
    return Status::OK();
}

std::optional<std::pair<std::string, std::string>>
ProcessGraphManager::findTokenByTokenId(std::string_view token_id) const {
    if (!db_.isOpen() || token_id.empty()) {
      return std::nullopt;
    }

    std::optional<std::pair<std::string, std::string>> found;
    const std::string token_id_str(token_id);

    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) -> bool {
        if (found) return false; // already found, stop scan

        // Key format: "process:token:<instance_id>:<token_id>"
        const std::string keyStr(key);
        // After "process:token:" (14 chars) find the colon separating instance from token
        const size_t prefix_len = 14; // strlen("process:token:")
        const size_t second_colon = keyStr.find(':', prefix_len);
        if (second_colon == std::string::npos) {
          return true;
        }

        const std::string stored_token_id = keyStr.substr(second_colon + 1);
        if (stored_token_id != token_id_str) {
          return true;
        }

        // Found a key with matching token_id — check it's READY or ACTIVE
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity tokenEntity = BaseEntity::deserialize(stored_token_id, blob);
        const auto stateStr = tokenEntity.getFieldAsString("state").value_or("");
        if (stateStr != "READY" && stateStr != "ACTIVE") {
          return true;
        }

        const std::string instance_id = keyStr.substr(prefix_len, second_colon - prefix_len);
        const std::string current_node =
            tokenEntity.getFieldAsString("current_node").value_or("");
        found = std::make_pair(instance_id, current_node);
        return false; // stop scan
    });

    return found;
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>> 
ProcessGraphManager::findActiveTasks(std::string_view assignee_or_role) const {
    std::vector<ProcessToken> result;
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), result};
    }
    
    // Scan all tokens across all instances
    std::string tokenPrefix = "process:token:";
    db_.scanPrefix(tokenPrefix, [&result, &assignee_or_role, this](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        
        // Parse instance_id and token_id from key: process:token:{instance_id}:{token_id}
        size_t secondColon = keyStr.find(':', 14); // After "process:token:"
        if (secondColon == std::string::npos) {
          return true;
        }
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) {
          return true;
        }
        
        std::string instanceId = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
        std::string tokenId = keyStr.substr(thirdColon + 1);
        
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);
        
        // Check if token is READY or ACTIVE (active tasks)
        auto stateStr = tokenEntity.getFieldAsString("state").value_or("");
        if (stateStr != "READY" && stateStr != "ACTIVE") {
            return true; // Skip non-active tokens
        }
        
        // Check assignment - can be stored in token or in node metadata
        bool isAssigned = false;
        
        // First check if token has assignment metadata
        auto assigneeField = tokenEntity.getFieldAsString("assignee");
        auto roleField = tokenEntity.getFieldAsString("role");
        
        if (assigneeField && *assigneeField == std::string(assignee_or_role)) {
            isAssigned = true;
        } else if (roleField && *roleField == std::string(assignee_or_role)) {
            isAssigned = true;
        }
        
        // If assignment found, construct token and add to result
        if (isAssigned) {
            ProcessToken token;
            token.token_id = tokenId;
            token.process_instance_id = instanceId;
            token.current_node = tokenEntity.getFieldAsString("current_node").value_or("");
            token.created_at_ms = tokenEntity.getFieldAsInt("created_at").value_or(0);
            
            if (stateStr == "READY") {
              token.state = ProcessToken::State::READY;
            }
            else if (stateStr == "ACTIVE") token.state = ProcessToken::State::ACTIVE;
            
            // Load variables if available
            auto varsStr = tokenEntity.getFieldAsString("variables");
            if (varsStr) {
                try {
                    token.variables = nlohmann::json::parse(*varsStr);
                } catch (...) {
                    token.variables = nlohmann::json::object();
                }
            }
            
            result.push_back(token);
        }
        
        return true; // Continue scanning
    });
    
    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::getNodeHistory(
    std::string_view process_id,
    std::string_view node_id,
    std::optional<int64_t> since_ms
) const {
    std::vector<ProcessToken> result;
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), result};
    }
    
    // Scan all tokens to find those that visited this node
    std::string tokenPrefix = "process:token:";
    db_.scanPrefix(tokenPrefix, [&result, &process_id, &node_id, &since_ms, this](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        
        // Parse instance_id and token_id from key
        size_t secondColon = keyStr.find(':', 14);
        if (secondColon == std::string::npos) {
          return true;
        }
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) {
          return true;
        }
        
        std::string instanceId = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
        std::string tokenId = keyStr.substr(thirdColon + 1);
        
        // Get instance to check process_id
        std::string instanceKey = makeInstanceKey_(instanceId);
        auto instanceBlob = db_.get(instanceKey);
        if (!instanceBlob) {
          return true;
        }
        
        BaseEntity instanceEntity = BaseEntity::deserialize(instanceId, *instanceBlob);
        auto instanceProcessId = instanceEntity.getFieldAsString("process_id").value_or("");
        
        // Skip if not matching process
        if (instanceProcessId != std::string(process_id)) {
            return true;
        }
        
        // Parse token
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);
        
        // Check if token visited this node
        auto currentNode = tokenEntity.getFieldAsString("current_node").value_or("");
        bool visitedNode = (currentNode == std::string(node_id));
        
        // Also check visited_nodes history if available
        auto visitedStr = tokenEntity.getFieldAsString("visited_nodes");
        if (!visitedNode && visitedStr) {
            try {
                auto visitedJson = nlohmann::json::parse(*visitedStr);
                if (visitedJson.is_array()) {
                    for (const auto& visited : visitedJson) {
                        if (visited.is_string() && visited.get<std::string>() == std::string(node_id)) {
                            visitedNode = true;
                            break;
                        }
                    }
                }
            } catch (...) {}
        }
        
        if (!visitedNode) {
          return true;
        }
        
        // Check time filter
        auto createdAt = tokenEntity.getFieldAsInt("created_at").value_or(0);
        if (since_ms && createdAt < *since_ms) {
            return true;
        }
        
        // Construct token
        ProcessToken token;
        token.token_id = tokenId;
        token.process_instance_id = instanceId;
        token.current_node = currentNode;
        token.created_at_ms = createdAt;
        
        auto stateStr = tokenEntity.getFieldAsString("state").value_or("READY");
        if (stateStr == "READY") {
          token.state = ProcessToken::State::READY;
        }
        else if (stateStr == "ACTIVE") token.state = ProcessToken::State::ACTIVE;
        else if (stateStr == "COMPLETED") token.state = ProcessToken::State::COMPLETED;
        else if (stateStr == "WAITING") token.state = ProcessToken::State::WAITING;
        else if (stateStr == "FAILED") token.state = ProcessToken::State::FAILED;
        
        // Load timestamps
        auto startedAt = tokenEntity.getFieldAsInt("started_at");
        if (startedAt) {
          token.started_at_ms = *startedAt;
        }
        
        auto completedAt = tokenEntity.getFieldAsInt("completed_at");
        if (completedAt) {
          token.completed_at_ms = *completedAt;
        }
        
        // Load variables
        auto varsStr = tokenEntity.getFieldAsString("variables");
        if (varsStr) {
            try {
                token.variables = nlohmann::json::parse(*varsStr);
            } catch (...) {
                token.variables = nlohmann::json::object();
            }
        }
        
        result.push_back(token);
        return true;
    });
    
    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::NodeMetrics>>
ProcessGraphManager::getProcessMetrics(std::string_view process_id) const {
    std::vector<NodeMetrics> result;
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), result};
    }
    
    // Map to aggregate metrics per node
    std::unordered_map<std::string, NodeMetrics> metricsMap;
    
    // Scan all tokens to gather metrics
    std::string tokenPrefix = "process:token:";
    db_.scanPrefix(tokenPrefix, [&metricsMap, &process_id, this](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        
        // Parse instance_id and token_id
        size_t secondColon = keyStr.find(':', 14);
        if (secondColon == std::string::npos) {
          return true;
        }
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) {
          return true;
        }
        
        std::string instanceId = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
        std::string tokenId = keyStr.substr(thirdColon + 1);
        
        // Check if this instance belongs to the target process
        std::string instanceKey = makeInstanceKey_(instanceId);
        auto instanceBlob = db_.get(instanceKey);
        if (!instanceBlob) {
          return true;
        }
        
        BaseEntity instanceEntity = BaseEntity::deserialize(instanceId, *instanceBlob);
        auto instanceProcessId = instanceEntity.getFieldAsString("process_id").value_or("");
        
        if (instanceProcessId != std::string(process_id)) {
            return true;
        }
        
        // Parse token
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);
        
        auto currentNode = tokenEntity.getFieldAsString("current_node").value_or("");
        if (currentNode.empty()) {
          return true;
        }
        
        auto stateStr = tokenEntity.getFieldAsString("state").value_or("");
        auto createdAt = tokenEntity.getFieldAsInt("created_at").value_or(0);
        auto startedAt = tokenEntity.getFieldAsInt("started_at");
        auto completedAt = tokenEntity.getFieldAsInt("completed_at");
        
        // Initialize metrics for this node if not exists
        if (metricsMap.find(currentNode) == metricsMap.end()) {
            metricsMap[currentNode] = NodeMetrics{
                .node_id = currentNode,
                .execution_count = 0,
                .avg_duration_ms = 0.0,
                .max_duration_ms = 0.0,
                .failure_count = 0
            };
        }
        
        auto& metrics = metricsMap[currentNode];
        
        // Count execution
        if (stateStr == "COMPLETED" || stateStr == "ACTIVE" || stateStr == "FAILED") {
            metrics.execution_count++;
        }
        
        // Count failures
        if (stateStr == "FAILED") {
            metrics.failure_count++;
        }
        
        // Calculate duration if completed
        if (completedAt && startedAt) {
            double duration = static_cast<double>(*completedAt - *startedAt);
            
            // Update max duration
            if (duration > metrics.max_duration_ms) {
                metrics.max_duration_ms = duration;
            }
            
            // Update average duration (incremental average)
            // Ensure we don't underflow when subtracting failure_count
            if (metrics.execution_count >= metrics.failure_count) {
                size_t completedCount = metrics.execution_count - metrics.failure_count;
                if (completedCount > 0) {
                    metrics.avg_duration_ms = 
                        (metrics.avg_duration_ms * (completedCount - 1) + duration) / completedCount;
                }
            }
        } else if (completedAt) {
            // Use created_at if started_at not available
            double duration = static_cast<double>(*completedAt - createdAt);
            
            if (duration > metrics.max_duration_ms) {
                metrics.max_duration_ms = duration;
            }
            
            // Ensure we don't underflow
            if (metrics.execution_count >= metrics.failure_count) {
                size_t completedCount = metrics.execution_count - metrics.failure_count;
                if (completedCount > 0) {
                    metrics.avg_duration_ms = 
                        (metrics.avg_duration_ms * (completedCount - 1) + duration) / completedCount;
                }
            }
        }
        
        return true;
    });
    
    // Convert map to vector
    for (const auto& [nodeId, metrics] : metricsMap) {
        result.push_back(metrics);
    }
    
    // Sort by execution count (descending)
    std::sort(result.begin(), result.end(), 
        [](const NodeMetrics& a, const NodeMetrics& b) {
            return a.execution_count > b.execution_count;
        });
    
    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<std::string>>
ProcessGraphManager::findCriticalPath(std::string_view process_id) const {
    std::vector<std::string> result;
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), result};
    }
    
    // First get metrics to identify nodes with longest durations
    auto [metricsSt, metrics] = getProcessMetrics(process_id);
    if (!metricsSt.ok) {
        return {Status::Error("Failed to get process metrics"), result};
    }
    
    if (metrics.empty()) {
        return {Status::Error("No metrics available for this process"), result};
    }
    
    // Build a graph of the process flow
    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    std::unordered_map<std::string, double> nodeDurations = {};

    nodeDurations.reserve(metrics.size());
    adjacency.reserve(metrics.size());
    
    // Load node durations from metrics
    for (const auto& m : metrics) {
        nodeDurations[m.node_id] = m.avg_duration_ms;
    }
    
    // Load edges to build adjacency list
    std::string edgePrefix = "process:edge:" + std::string(process_id) + ":";
    db_.scanPrefix(edgePrefix, [&adjacency](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) {
          return true;
        }
        
        std::string edgeId = keyStr.substr(lastColon + 1);
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity entity = BaseEntity::deserialize(edgeId, blob);
        
        auto from = entity.getFieldAsString("_from").value_or("");
        auto to = entity.getFieldAsString("_to").value_or("");
        
        if (!from.empty() && !to.empty()) {
            adjacency[from].push_back(to);
        }
        
        return true;
    });
    
    // Find start node
    std::string startNode = {};
    std::string nodePrefix = "process:node:" + std::string(process_id) + ":";
    db_.scanPrefix(nodePrefix, [&startNode](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) {
          return true;
        }
        
        std::string nodeId = keyStr.substr(lastColon + 1);
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity entity = BaseEntity::deserialize(nodeId, blob);
        
        auto nodeType = entity.getFieldAsString("node_type").value_or("");
        if ([[maybe_unused]] nodeType == "START_EVENT" || nodeType == "EVENT") {
            startNode = nodeId;
            return false; // Stop scanning
        }
        
        return true;
    });
    
    if (startNode.empty()) {
        return {Status::Error("No start node found"), result};
    }
    
    // Use iterative DFS to find path with maximum cumulative duration
    // This avoids stack overflow for deep process graphs
    std::vector<std::string> longestPath = {};

    longestPath.reserve(nodeDurations.size());
    double maxDuration = 0.0;
    
    // Stack entry: (node, cumDuration, path, visited)
    struct StackEntry {
        std::string node = {};
        double cumDuration;
        std::vector<std::string> path;
        std::unordered_set<std::string> visited;
        ~StackEntry() = default;
    };
    
    std::vector<StackEntry> stack = {};

    stack.reserve(nodeDurations.size() + 1);
    stack.push_back({startNode, 0.0, std::vector<std::string>(), std::unordered_set<std::string>()});
    
    while (!stack.empty()) {
        auto entry = std::move(stack.back());
        stack.pop_back();
        
        // Skip if already visited in this path
        if (entry.visited.count(entry.node)) {
            continue;
        }
        
        // Mark as visited
        entry.visited.insert(entry.node);
        entry.path.reserve(entry.path.size() + 1);
        entry.path.push_back(entry.node);
        
        // Add node duration
        double nodeDur = 0.0;
        if (const auto durIt = nodeDurations.find(entry.node); durIt != nodeDurations.end()) {
            nodeDur = durIt->second;
        }
        entry.cumDuration += nodeDur;
        
        // Check if this is longest path so far
        if (entry.cumDuration > maxDuration) {
            maxDuration = entry.cumDuration;
            longestPath = entry.path;
        }
        
        // Explore neighbors
        if (const auto neighborsIt = adjacency.find(entry.node); neighborsIt != adjacency.end()) {
            for (const auto& neighbor : neighborsIt->second) {
                if (!entry.visited.count(neighbor)) {
                    // Push new entry for neighbor
                    stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
                }
            }
        }
    }
    
    result = longestPath;
    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, Hyperedge>
ProcessGraphManager::getHyperedgeStatus(std::string_view hyperedge_id) const {
    Hyperedge hyperedge = {};
    
    if (!db_.isOpen()) {
        return {Status::Error("Database not open"), hyperedge};
    }
    
    // Need to find the hyperedge - scan all process hyperedges
    std::string hyperedgePrefix = "process:hyperedge:";
    bool found = false;
    
    db_.scanPrefix(hyperedgePrefix, [this, &hyperedge, &hyperedge_id, &found](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        
        // Check if this is the hyperedge we're looking for
        if (keyStr.find(std::string(hyperedge_id)) == std::string::npos) {
            return true; // Continue scanning
        }
        
        // Parse the hyperedge
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity entity = BaseEntity::deserialize(std::string(hyperedge_id), blob);
        
        hyperedge.hyperedge_id = std::string(hyperedge_id);
        hyperedge.name = entity.getFieldAsString("name").value_or("");
        
        // Parse sources and targets
        auto sourcesStr = entity.getFieldAsString("sources");
        if (sourcesStr) {
            try {
                auto sourcesJson = nlohmann::json::parse(*sourcesStr);
                if (sourcesJson.is_array()) {
                    for (const auto& src : sourcesJson) {
                        if (src.is_string()) {
                            hyperedge.source_nodes.push_back(src.get<std::string>());
                        }
                    }
                }
            } catch (...) {}
        }
        
        auto targetsStr = entity.getFieldAsString("targets");
        if (targetsStr) {
            try {
                auto targetsJson = nlohmann::json::parse(*targetsStr);
                if (targetsJson.is_array()) {
                    for (const auto& tgt : targetsJson) {
                        if (tgt.is_string()) {
                            hyperedge.target_nodes.push_back(tgt.get<std::string>());
                        }
                    }
                }
            } catch (...) {}
        }
        
        // Parse sync type
        auto syncTypeStr = entity.getFieldAsString("sync_type").value_or("AND_JOIN");
        if (syncTypeStr == "AND_JOIN") {
          hyperedge.sync_type = Hyperedge::SyncType::AND_JOIN;
        }
        else if (syncTypeStr == "AND_SPLIT") hyperedge.sync_type = Hyperedge::SyncType::AND_SPLIT;
        else if (syncTypeStr == "OR_JOIN") hyperedge.sync_type = Hyperedge::SyncType::OR_JOIN;
        else if (syncTypeStr == "N_OF_M_JOIN") hyperedge.sync_type = Hyperedge::SyncType::N_OF_M_JOIN;
        else if (syncTypeStr == "DISCRIMINATOR") hyperedge.sync_type = Hyperedge::SyncType::DISCRIMINATOR;
        
        // Parse required count
        auto reqCount = entity.getFieldAsInt("required_count");
        if (reqCount) {
            hyperedge.required_count = static_cast<int>(*reqCount);
        }
        
        // Parse activated sources (runtime state)
        auto activatedStr = entity.getFieldAsString("activated_sources");
        if (activatedStr) {
            try {
                auto activatedJson = nlohmann::json::parse(*activatedStr);
                if (activatedJson.is_array()) {
                    for (const auto& act : activatedJson) {
                        if (act.is_string()) {
                            hyperedge.activated_sources.insert(act.get<std::string>());
                        }
                    }
                }
            } catch (...) {}
        }
        
        // Check completion status
        hyperedge.is_complete = checkHyperedgeCondition_(hyperedge);
        
        found = true;
        return false; // Stop scanning
    });
    
    if (!found) {
        return {Status::Error("Hyperedge not found"), hyperedge};
    }
    
    return {Status::OK(), hyperedge};
}

std::pair<ProcessGraphManager::Status, bool>
ProcessGraphManager::isHyperedgeReady(std::string_view hyperedge_id) const {
    // Get the hyperedge status
    auto [st, hyperedge] = getHyperedgeStatus(hyperedge_id);
    if (!st.ok) {
        return {st, false};
    }
    
    // Check if the hyperedge condition is satisfied
    bool ready = checkHyperedgeCondition_(hyperedge);
    
    return {Status::OK(), ready};
}

// ============================================================================
// PERMANENT FALLBACK NOTE (ProcessGraph Multi-Model Query — in-process RocksDB scan):
// Purpose: Implement AQL-style multi-model queries (form-data filter,
//   foreign-key join, aggregation) directly over the RocksDB in-process store
//   using scanPrefix().  This avoids requiring a live ArangoDB/ThemisDB query
//   engine for process graph analytics.  The functions provide real query
//   semantics for the in-process database.
// Activation: Always active — these functions run against the in-process
//   RocksDB store regardless of whether a remote query engine is configured.
// Production Delta: Filter and join operations use O(n) full scans instead of
//   index-backed AQL traversals.  Performance degrades with large process
//   instances (> 10 K tokens per process): queryTasksByFormData is O(n) in
//   tokens, queryForeignKeyJoin is O(n×m) in tokens×foreign docs.  In
//   production with a live query engine these would be replaced by AQL queries
//   with server-side index acceleration.
// Note: Wire a ThemisDB AQL query engine reference via setAqlQueryExecutor()
//   to delegate to engine-backed queries; the in-process scan path then
//   becomes the offline fallback.
// Roadmap ref: src/index/FUTURE_ENHANCEMENTS.md §"Process Graph Multi-Model Query Engine"
// ============================================================================
// Multi-Model Query Implementation
// ============================================================================

void ProcessGraphManager::setAqlQueryExecutor(AqlQueryExecutorFn fn) {
    aql_query_executor_ = std::move(fn);
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::queryTasksByFormData(
    std::string_view process_id,
    const nlohmann::json& filter_conditions
) const {
    std::vector<ProcessToken> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};
    if (filter_conditions.is_null() || filter_conditions.empty())
        return {Status::Error("filter_conditions must be a non-empty JSON object"), result};

    // ── AQL-backed path (index-accelerated) ──────────────────────────────
    if (aql_query_executor_) {
        const std::string aql =
            "FOR t IN process_tokens "
            "FILTER t.process_id == @pid "
            "FILTER MATCHES(t.variables, @filter) OR MATCHES(t.form_data, @filter) "
            "RETURN t";
        nlohmann::json bind_vars;
        bind_vars["pid"]    = std::string(process_id);
        bind_vars["filter"] = filter_conditions;
        const auto rows = aql_query_executor_(aql, bind_vars);
        for (const auto& row : rows) {
            ProcessToken token;
            token.token_id             = row.value("token_id", std::string{});
            token.process_instance_id  = row.value("process_instance_id", std::string{});
            token.current_node         = row.value("current_node", std::string{});
            token.created_at_ms        = row.value("created_at_ms", int64_t{0});
            if (row.contains("variables") && row["variables"].is_object())
                token.variables = row["variables"];
            const auto stStr = row.value("state", std::string{"READY"});
            if      (stStr == "ACTIVE") {
              token.state = ProcessToken::State::ACTIVE;
            }
            else if (stStr == "WAITING")   token.state = ProcessToken::State::WAITING;
            else if (stStr == "COMPLETED") token.state = ProcessToken::State::COMPLETED;
            else if (stStr == "FAILED")    token.state = ProcessToken::State::FAILED;
            result.push_back(std::move(token));
        }
        return {Status::OK(), result};
    }

    const std::string pid(process_id);
    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        // Parse instance_id from key: "process:token:{instance_id}:{token_id}"
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 16); // skip "process:token:"
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string instanceId = keyStr.substr(14, p1 - 14);

        // Verify the instance belongs to the target process.
        const auto instBlob = db_.get(makeInstanceKey_(instanceId));
        if (!instBlob) {
          return true;
        }
        const BaseEntity instEntity = BaseEntity::deserialize(instanceId, *instBlob);
        if (instEntity.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        // Deserialize token.
        const std::string tokenId = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);

        // Load variables JSON.
        nlohmann::json vars = nlohmann::json::object();
        const auto varsStr = tokenEntity.getFieldAsString("variables");
        vars = parseJsonObjectOrEmpty(varsStr, "queryTasksByFormData", "variables");
        // Also check form_data field.
        const auto formStr = tokenEntity.getFieldAsString("form_data");
        const auto fd = parseJsonObjectOrEmpty(formStr, "queryTasksByFormData", "form_data");
        for (auto& [k, v] : fd.items()) {
          vars[k] = v;
        }

        // Check all filter conditions (AND semantics).
        bool matches = true;
        if (filter_conditions.is_object()) {
            for (auto& [field, expected] : filter_conditions.items()) {
                if (!vars.contains(field) || vars[field] != expected) {
                    matches = false;
                    break;
                }
            }
        }
        if (!matches) {
          return true;
        }

        ProcessToken token;
        token.token_id               = tokenId;
        token.process_instance_id    = instanceId;
        token.current_node           = tokenEntity.getFieldAsString("current_node").value_or("");
        token.created_at_ms          = tokenEntity.getFieldAsInt("created_at").value_or(0);
        token.variables              = vars;
        const auto stStr             = tokenEntity.getFieldAsString("state").value_or("READY");
        if (stStr == "ACTIVE") {
          token.state = ProcessToken::State::ACTIVE;
        }
        else if (stStr == "WAITING") token.state = ProcessToken::State::WAITING;
        else if (stStr == "COMPLETED") token.state = ProcessToken::State::COMPLETED;
        else if (stStr == "FAILED")  token.state = ProcessToken::State::FAILED;
        result.push_back(std::move(token));
        return true;
    });

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::JoinResult>>
ProcessGraphManager::joinWithCollection(
    std::string_view process_id,
    std::string_view collection_name,
    std::string_view local_field,
    std::string_view foreign_field
) const {
    std::vector<JoinResult> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    // ── AQL-backed path (index-accelerated) ──────────────────────────────
    if (aql_query_executor_) {
        const std::string aql =
            "FOR t IN process_tokens "
            "FILTER t.process_id == @pid "
            "LET fk = t.variables[@lf] "
            "LET ext = FIRST(FOR doc IN @@coll FILTER doc[@ff] == fk RETURN doc) "
            "RETURN { token: t, joined: ext }";
        nlohmann::json bind_vars;
        bind_vars["pid"]   = std::string(process_id);
        bind_vars["@coll"] = std::string(collection_name);
        bind_vars["lf"]    = std::string(local_field);
        bind_vars["ff"]    = std::string(foreign_field);
        const auto rows = aql_query_executor_(aql, bind_vars);
        for (const auto& row : rows) {
            JoinResult jr = {};
            if (row.contains("token") && row["token"].is_object()) {
                const auto& t = row["token"];
                jr.token.token_id            = t.value("token_id", std::string{});
                jr.token.process_instance_id = t.value("process_instance_id", std::string{});
                jr.token.current_node        = t.value("current_node", std::string{});
                jr.token.created_at_ms       = t.value("created_at_ms", int64_t{0});
                if (t.contains("variables") && t["variables"].is_object())
                    jr.token.variables = t["variables"];
            }
            jr.joined_data = row.value("joined", nlohmann::json::object());
            result.push_back(std::move(jr));
        }
        return {Status::OK(), result};
    }

    const std::string pid(process_id);
    const std::string lf(local_field);
    const std::string ff(foreign_field);
    const std::string collPrefix = "entity:" + std::string(collection_name) + ":";

    // Build lookup map from collection: foreign_field_value → document json
    std::unordered_map<std::string, nlohmann::json> foreignIndex;
    db_.scanPrefix(collPrefix, [&](std::string_view, std::string_view val) {
        const std::string docId(val.begin(), val.begin() + std::min(val.size(), size_t{0}));
        const std::vector<uint8_t> blob(val.begin(), val.end());
        // Try to parse the value as a BaseEntity serialized doc.
        try {
            const BaseEntity ent = BaseEntity::deserialize("_", blob);
            const auto ffVal = ent.getFieldAsString(ff);
            if (ffVal) {
                nlohmann::json doc = nlohmann::json::object();
                // Expose all string and int fields in the doc JSON.
                // (BaseEntity doesn't have an iterator; use known serialization.)
                // We store the raw entity as a JSON with the foreign key.
                doc[ff] = *ffVal;
                foreignIndex[*ffVal] = std::move(doc);
            }
        } catch (const std::exception& e) {
            THEMIS_DEBUG("ProcessGraphManager::joinWithCollection skipped invalid foreign document: {}", e.what());
        }
        return true;
    });

    // Scan tokens for the given process, join on local_field.
    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 14);
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string instanceId = keyStr.substr(14, p1 - 14);

        const auto instBlob = db_.get(makeInstanceKey_(instanceId));
        if (!instBlob) {
          return true;
        }
        const BaseEntity instEntity = BaseEntity::deserialize(instanceId, *instBlob);
        if (instEntity.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        const std::string tokenId = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);

        nlohmann::json vars = parseJsonObjectOrEmpty(
            tokenEntity.getFieldAsString("variables"), "joinWithCollection", "variables");

        // Look up the local_field value.
        if (!vars.contains(lf)) {
          return true;
        }
        const std::string localVal = vars[lf].is_string() ? vars[lf].get<std::string>()
                                                          : vars[lf].dump();

        const auto it = foreignIndex.find(localVal);
        if (it == foreignIndex.end()) {
          return true;
        }

        ProcessToken token;
        token.token_id            = tokenId;
        token.process_instance_id = instanceId;
        token.current_node        = tokenEntity.getFieldAsString("current_node").value_or("");
        token.created_at_ms       = tokenEntity.getFieldAsInt("created_at").value_or(0);
        token.variables           = vars;

        JoinResult jr;
        jr.token       = std::move(token);
        jr.joined_data = it->second;
        result.push_back(std::move(jr));
        return true;
    });

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::AggregateResult>>
ProcessGraphManager::aggregateByField(
    std::string_view process_id,
    std::string_view group_field,
    std::string_view agg_field,
    std::string_view agg_function
) const {
    std::vector<AggregateResult> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    // ── AQL-backed path (index-accelerated) ──────────────────────────────
    if (aql_query_executor_) {
        const std::string aggFn(agg_function);
        const std::string aql =
            "FOR t IN process_tokens "
            "FILTER t.process_id == @pid "
            "COLLECT gk = t.variables[@gf] "
            "AGGREGATE cnt = COUNT(1), "
            "           sm  = SUM(t.variables[@af]), "
            "           mn  = MIN(t.variables[@af]), "
            "           mx  = MAX(t.variables[@af]) "
            "RETURN { group_key: gk, count: cnt, sum: sm, min: mn, max: mx }";
        nlohmann::json bind_vars;
        bind_vars["pid"] = std::string(process_id);
        bind_vars["gf"]  = std::string(group_field);
        bind_vars["af"]  = std::string(agg_field);
        const auto rows = aql_query_executor_(aql, bind_vars);
        for (const auto& row : rows) {
            AggregateResult ar;
            ar.group_key = row.value("group_key", nlohmann::json{});
            ar.count     = row.value("count", size_t{0});
            ar.sum       = row.value("sum",   0.0);
            ar.min       = row.value("min",   0.0);
            ar.max       = row.value("max",   0.0);
            ar.avg       = ar.count > 0 ? ar.sum / static_cast<double>(ar.count) : 0.0;
            result.push_back(std::move(ar));
        }
        return {Status::OK(), result};
    }

    const std::string pid(process_id);
    const std::string gf(group_field);
    const std::string af(agg_field);
    [[maybe_unused]] const std::string fn(agg_function);

    struct GroupAcc {
        size_t count = 0;
        double sum   = 0.0;
        double min   = std::numeric_limits<double>::max();
        double max   = std::numeric_limits<double>::lowest();
    };
    std::unordered_map<std::string, GroupAcc> groups;

    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 14);
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string instanceId = keyStr.substr(14, p1 - 14);

        const auto instBlob = db_.get(makeInstanceKey_(instanceId));
        if (!instBlob) {
          return true;
        }
        const BaseEntity instEntity = BaseEntity::deserialize(instanceId, *instBlob);
        if (instEntity.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        const std::string tokenId = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);

        nlohmann::json vars = parseJsonObjectOrEmpty(
            tokenEntity.getFieldAsString("variables"), "aggregateTokensByField", "variables");

        if (!vars.contains(gf)) {
          return true;
        }

        const std::string groupKey = vars[gf].is_string() ? vars[gf].get<std::string>()
                                                          : vars[gf].dump();
        auto& acc = groups[groupKey];
        acc.count++;

        if (!af.empty() && vars.contains(af) && vars[af].is_number()) {
            const double v = vars[af].get<double>();
            acc.sum += v;
            acc.min  = std::min(acc.min, v);
            acc.max  = std::max(acc.max, v);
        }
        return true;
    });

    result.reserve(groups.size());
    for (auto& [key, acc] : groups) {
        AggregateResult ar;
        ar.group_key = key;
        ar.count     = acc.count;
        ar.sum       = acc.sum;
        ar.avg       = acc.count > 0 ? acc.sum / static_cast<double>(acc.count) : 0.0;
        ar.min       = acc.count > 0 && !af.empty() ? acc.min : 0.0;
        ar.max       = acc.count > 0 && !af.empty() ? acc.max : 0.0;
        // agg_function selects which field to surface; all are computed
        result.push_back(std::move(ar));
    }

    return {Status::OK(), result};
}

// ---------------------------------------------------------------------------
// Vector helpers
// ---------------------------------------------------------------------------

namespace {

float computeCosineSimilarity(const std::vector<float>& a,
                              const std::vector<float>& b) noexcept {
    if (a.size() != b.size() || a.empty()) {
      return 0.0f;
    }
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    const float denom = std::sqrt(na) * std::sqrt(nb);
    return (denom > 1e-9f) ? dot / denom : 0.0f;
}

/// Deserialize a JSON float array stored as "[0.1, 0.2, ...]" to vector<float>.
std::vector<float> parseEmbeddingJson(const std::string& s) {
    std::vector<float> emb;
    try {
        const auto arr = nlohmann::json::parse(s);
        if (arr.is_array()) {
            emb.reserve(arr.size());
            for (const auto& v : arr) {
                if (v.is_number()) {
                  emb.push_back(v.get<float>());
                }
            }
        }
    } catch (const std::exception& e) {
        THEMIS_DEBUG("ProcessGraphManager::parseEmbeddingJson failed: {}", e.what());
    }
    return emb;
}

} // anonymous namespace

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::SimilarProcess>>
ProcessGraphManager::findSimilarProcesses(
    const std::vector<float>& query_embedding,
    size_t k,
    float min_similarity
) const {
    std::vector<SimilarProcess> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};
    if (query_embedding.empty()) return {Status::Error("query_embedding must not be empty"), result};

    // Scan all process definitions for stored embeddings.
    db_.scanPrefix("process:def:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const std::string procId = keyStr.substr(12); // len("process:def:")
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity ent = BaseEntity::deserialize(procId, blob);

        const auto embStr = ent.getFieldAsString("embedding");
        if (!embStr) {
          return true;
        }

        const auto emb = parseEmbeddingJson(*embStr);
        if (emb.empty()) {
          return true;
        }

        const float sim = computeCosineSimilarity(query_embedding, emb);
        if (sim < min_similarity) {
          return true;
        }

        SimilarProcess sp;
        sp.process_id = procId;
        sp.name       = ent.getFieldAsString("name").value_or(procId);
        sp.similarity = sim;
        result.push_back(std::move(sp));
        return true;
    });

    // Sort descending by similarity and truncate to k.
    std::sort(result.begin(), result.end(),
              [](const SimilarProcess& a, const SimilarProcess& b) {
                  return a.similarity > b.similarity;
              });
    if (result.size() > k) {
      result.resize(k);
    }

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findSimilarTasks(
    std::string_view instance_id,
    std::string_view task_node,
    size_t k
) const {
    std::vector<ProcessToken> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    // Load the reference token at task_node to get its embedding.
    const std::string instId(instance_id);
    const std::string nodeId(task_node);
    std::vector<float> queryEmb;

    db_.scanPrefix("process:token:" + instId + ":", [&](std::string_view key, std::string_view val) {
        if (!queryEmb.empty()) return false; // found already
        const std::string keyStr(key);
        const size_t p = keyStr.rfind(':');
        if (p == std::string::npos) {
          return true;
        }
        const std::string tid = keyStr.substr(p + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity te = BaseEntity::deserialize(tid, blob);
        if (te.getFieldAsString("current_node").value_or("") != nodeId) {
          return true;
        }
        const auto embStr = te.getFieldAsString("embedding");
        if (embStr) {
          queryEmb = parseEmbeddingJson(*embStr);
        }
        return true;
    });

    if (queryEmb.empty()) {
        return {Status::Error("No embedding found for task_node in instance"), result};
    }

    // Get the process_definition_id for this instance.
    const auto instBlob = db_.get(makeInstanceKey_(instance_id));
    if (!instBlob) return {Status::Error("Instance not found"), result};
    const BaseEntity instEntity = BaseEntity::deserialize(instId, *instBlob);
    const std::string pid = instEntity.getFieldAsString("process_id").value_or("");

    // Scan all tokens for instances of the same process.
    std::vector<std::pair<float, ProcessToken>> candidates;
    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 14);
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string iid = keyStr.substr(14, p1 - 14);
        if (iid == instId) return true; // skip the source instance itself

        const auto ib = db_.get(makeInstanceKey_(iid));
        if (!ib) {
          return true;
        }
        const BaseEntity ie = BaseEntity::deserialize(iid, *ib);
        if (ie.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        const std::string tid = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity te = BaseEntity::deserialize(tid, blob);

        const auto embStr = te.getFieldAsString("embedding");
        if (!embStr) {
          return true;
        }
        const auto emb = parseEmbeddingJson(*embStr);
        if (emb.empty()) {
          return true;
        }

        [[maybe_unused]] const float sim = computeCosineSimilarity(queryEmb, emb);

        ProcessToken token;
        token.token_id            = tid;
        token.process_instance_id = iid;
        token.current_node        = te.getFieldAsString("current_node").value_or("");
        token.created_at_ms       = te.getFieldAsInt("created_at").value_or(0);
        candidates.emplace_back(sim, std::move(token));
        return true;
    });

    // Sort descending by similarity, take top k.
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    if (candidates.size() > k) {
      candidates.resize(k);
    }
    result.reserve(candidates.size());
    for (auto& [sim, tok] : candidates) {
        result.push_back(std::move(tok));
    }

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::SimilarProcess>>
ProcessGraphManager::semanticSearchProcesses(
    std::string_view natural_language_query,
    size_t k
) const {
    std::vector<SimilarProcess> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    const std::string query(natural_language_query);
    // Normalise query to lowercase for substring matching.
    std::string queryLower = query;
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    // Tokenise query into words for relevance scoring.
    std::vector<std::string> queryTokens;
    {
        std::istringstream ss(queryLower);
        std::string word = {};
        while (ss >> word) {
          queryTokens.push_back(word);
        }
    }
    if (queryTokens.empty()) return {Status::Error("Empty query"), result};

    db_.scanPrefix("process:def:", [&](std::string_view key, std::string_view val) {
        const std::string procId = std::string(key).substr(12);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity ent = BaseEntity::deserialize(procId, blob);

        const std::string name = ent.getFieldAsString("name").value_or("");
        const std::string desc = ent.getFieldAsString("description").value_or("");
        std::string haystack   = name + " " + desc;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        // Score = fraction of query tokens found in haystack.
        float hits = 0.0f;
        for (const auto& tok : queryTokens) {
            if (haystack.find(tok) != std::string::npos) {
              hits += 1.0f;
            }
        }
        const float sim = hits / static_cast<float>(queryTokens.size());
        if (sim < 0.01f) {
          return true;
        }

        SimilarProcess sp;
        sp.process_id = procId;
        sp.name       = name;
        sp.similarity = sim;
        result.push_back(std::move(sp));
        return true;
    });

    std::sort(result.begin(), result.end(),
              [](const SimilarProcess& a, const SimilarProcess& b) {
                  return a.similarity > b.similarity;
              });
    if (result.size() > k) {
      result.resize(k);
    }

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::AnomalyResult>>
ProcessGraphManager::detectAnomalies(
    std::string_view process_id,
    float threshold
) const {
    std::vector<AnomalyResult> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    const std::string pid(process_id);

    // Collect all completed tokens for this process to build baseline stats.
    // Stats: per-node average duration and standard deviation.
    struct NodeStats {
        std::vector<double> durations_ms;
    };
    std::unordered_map<std::string, NodeStats> nodeStats;

    // Also track the most common path (sequence of visited_nodes) for deviation detection.
    std::unordered_map<std::string, size_t> pathFreq; // serialized path → count

    struct TokenInfo {
        std::string instanceId;
        std::string tokenId;
        std::string currentNode;
        std::string state = {};
        double      durationMs{0.0};
        std::string visitedPath = {};
    };
    std::vector<TokenInfo> allTokens;

    db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 14);
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string iid = keyStr.substr(14, p1 - 14);

        const auto ib = db_.get(makeInstanceKey_(iid));
        if (!ib) {
          return true;
        }
        const BaseEntity ie = BaseEntity::deserialize(iid, *ib);
        if (ie.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        const std::string tid = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity te = BaseEntity::deserialize(tid, blob);

        TokenInfo info;
        info.instanceId  = iid;
        info.tokenId     = tid;
        info.currentNode = te.getFieldAsString("current_node").value_or("");
        info.state       = te.getFieldAsString("state").value_or("");

        const int64_t created   = te.getFieldAsInt("created_at").value_or(0);
        const auto completedOpt = te.getFieldAsInt("completed_at");
        if (completedOpt && created > 0) {
            info.durationMs = static_cast<double>(*completedOpt - created);
        }

        const auto vnStr = te.getFieldAsString("visited_nodes");
        if (vnStr) {
          info.visitedPath = *vnStr;
        }

        allTokens.push_back(std::move(info));

        // Baseline: accumulate duration for completed tokens.
        if (info.state == "COMPLETED" && info.durationMs > 0.0) {
            nodeStats[info.currentNode].durations_ms.push_back(info.durationMs);
        }
        if (!info.visitedPath.empty()) {
            pathFreq[info.visitedPath]++;
        }
        return true;
    });

    // Compute mean and stddev per node.
    struct NodeBaseline {
        double mean{0.0};
        double stddev{0.0};
    };
    std::unordered_map<std::string, NodeBaseline> baselines = {};

    for (auto& [node, stats] : nodeStats) {
        if (stats.durations_ms.empty()) {
          continue;
        }
        double sum = 0.0;
        for (double d : stats.durations_ms) {
          sum += d;
        }
        const double mean = sum / static_cast<double>(stats.durations_ms.size());
        double varSum = 0.0;
        for (double d : stats.durations_ms) {
          varSum += (d - mean) * (d - mean);
        }
        const double stddev = std::sqrt(varSum / static_cast<double>(stats.durations_ms.size()));
        baselines[node] = {mean, stddev};
    }

    // Find the most common path.
    std::string dominantPath = {};
    size_t maxPathCount = 0;
    for (const auto& [path, cnt] : pathFreq) {
        if (cnt > maxPathCount) { maxPathCount = cnt; dominantPath = path; }
    }

    // Detect anomalies: only for active/running tokens.
    for (const auto& ti : allTokens) {
        if (ti.state != "ACTIVE" && ti.state != "READY") {
          continue;
        }

        // 1. Duration anomaly: current duration vs baseline.
        if (ti.durationMs > 0.0 && baselines.count(ti.currentNode)) {
            const auto& bl = baselines[ti.currentNode];
            if (bl.stddev > 0.0) {
                const double zScore = (ti.durationMs - bl.mean) / bl.stddev;
                if (zScore > static_cast<double>(threshold) * 3.0) {
                    AnomalyResult ar;
                    ar.instance_id   = ti.instanceId;
                    ar.anomaly_type  = "duration_outlier";
                    ar.anomaly_score = std::min(1.0f, static_cast<float>(zScore / 10.0));
                    ar.description   = "Token at '" + ti.currentNode +
                                       "' is running " + std::to_string(static_cast<int>(zScore)) +
                                       " stddevs above mean (" +
                                       std::to_string(static_cast<int>(bl.mean)) + " ms)";
                    result.push_back(std::move(ar));
                }
            }
        }

        // 2. Path deviation anomaly.
        if (!ti.visitedPath.empty() && !dominantPath.empty() &&
            ti.visitedPath != dominantPath) {
            // Compute a simple deviation score: fraction of nodes not in dominant path.
            const auto tokenNodes  = deserializeVisitedNodes(ti.visitedPath);
            const auto normalNodes = deserializeVisitedNodes(dominantPath);
            const std::unordered_set<std::string> normalSet(normalNodes.begin(), normalNodes.end());
            size_t deviatedCount = 0;
            for (const auto& n : tokenNodes) {
                if (!normalSet.count(n)) {
                  deviatedCount++;
                }
            }
            if (!tokenNodes.empty()) {
                const float score = static_cast<float>(deviatedCount) /
                                    static_cast<float>(tokenNodes.size());
                if (score >= threshold) {
                    AnomalyResult ar;
                    ar.instance_id   = ti.instanceId;
                    ar.anomaly_type  = "path_deviation";
                    ar.anomaly_score = score;
                    ar.description   = std::to_string(deviatedCount) + " of " +
                                       std::to_string(tokenNodes.size()) +
                                       " visited nodes deviate from the dominant process path";
                    result.push_back(std::move(ar));
                }
            }
        }
    }

    // Sort by severity descending.
    std::sort(result.begin(), result.end(),
              [](const AnomalyResult& a, const AnomalyResult& b) {
                  return a.anomaly_score > b.anomaly_score;
              });

    return {Status::OK(), result};
}

// ---------------------------------------------------------------------------
// Geo helpers (self-contained; avoids pulling in geo module headers)
// ---------------------------------------------------------------------------

namespace {

static constexpr double kEarthRadiusKm = 6371.0;

double processGraphHaversineKm(double lon1, double lat1,
                                double lon2, double lat2) noexcept {
    const double dLat = (lat2 - lat1) * kPi / 180.0;
    const double dLon = (lon2 - lon1) * kPi / 180.0;
    const double lat1r = lat1 * kPi / 180.0;
    const double lat2r = lat2 * kPi / 180.0;
    const double sinDLat = std::sin(dLat * 0.5);
    const double sinDLon = std::sin(dLon * 0.5);
    const double a = sinDLat * sinDLat +
                     std::cos(lat1r) * std::cos(lat2r) * sinDLon * sinDLon;
    const double c = 2.0 * std::asin(std::sqrt(a < 1.0 ? a : 1.0));
    return kEarthRadiusKm * c;
}

/// Ray-casting point-in-polygon for a simple polygon given as (lon,lat) pairs.
bool pointInPolygon(double lon, double lat,
                    const std::vector<std::pair<double,double>>& ring) noexcept {
    bool inside = false;
    const size_t n = ring.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const double xi = ring[i].first,  yi = ring[i].second;
        const double xj = ring[j].first,  yj = ring[j].second;
        if (((yi > lat) != (yj > lat)) &&
            (lon < (xj - xi) * (lat - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }
    return inside;
}

/// Parse a WKT "POLYGON((lon lat, ...))" into the outer ring.
std::vector<std::pair<double,double>> parseWktPolygon(const std::string& wkt) {
    std::vector<std::pair<double,double>> ring;
    const size_t start = wkt.find('(');
    const size_t end   = wkt.rfind(')');
    if (start == std::string::npos || end == std::string::npos) {
      return ring;
    }
    // Find the inner parenthesis for the outer ring.
    const size_t inner = wkt.find('(', start + 1);
    const size_t close = wkt.find(')', inner + 1);
    if (inner == std::string::npos || close == std::string::npos) {
      return ring;
    }
    std::istringstream ss(wkt.substr(inner + 1, close - inner - 1));
    std::string pair = {};
    while (std::getline(ss, pair, ',')) {
        std::istringstream ps(pair);
        double x, y;
        if (ps >> x >> y) {
          ring.emplace_back(x, y);
        }
    }
    return ring;
}

/// Extract lon/lat from a token's variables JSON.
/// Checks "_lon"/"_lat", "lon"/"lat", "_geometry" (WKT POINT), "geometry".
bool extractTokenGeo(const nlohmann::json& vars, double& lon, double& lat) {
    // "_lon" / "_lat" or "lon" / "lat" direct fields.
    for (const auto& lnk : {"_lon", "lon"}) {
        for (const auto& lak : {"_lat", "lat"}) {
            if (vars.contains(lnk) && vars.contains(lak) &&
                vars[lnk].is_number() && vars[lak].is_number()) {
                lon = vars[lnk].get<double>();
                lat = vars[lak].get<double>();
                return true;
            }
        }
    }
    // "_geometry" WKT POINT
    for (const auto& gk : {"_geometry", "geometry", "_geo"}) {
        if (!vars.contains(gk) || !vars[gk].is_string()) {
          continue;
        }
        const std::string geom = vars[gk].get<std::string>();
        // "POINT(lon lat)" or "POINT (lon lat)"
        const size_t p = geom.find('(');
        const size_t q = geom.find(')');
        if (p == std::string::npos || q == std::string::npos) {
          continue;
        }
        std::istringstream ss(geom.substr(p + 1, q - p - 1));
        if (ss >> lon >> lat) {
          return true;
        }
    }
    return false;
}

} // anonymous namespace

/// Shared helper: load all tokens for a given process (scanning by process_id match).
/// `callback(instanceId, tokenId, tokenEntity)` returns true to continue.
static void scanProcessTokens(
    RocksDBWrapper& db,
    const std::string& pid,
    const std::function<bool(const std::string&, const std::string&, const BaseEntity&)>& cb,
    const std::function<std::string(std::string_view)>& makeInstanceKey)
{
    db.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
        const std::string keyStr(key);
        const size_t p1 = keyStr.find(':', 14);
        if (p1 == std::string::npos) {
          return true;
        }
        const std::string iid = keyStr.substr(14, p1 - 14);

        const auto ib = db.get(makeInstanceKey(iid));
        if (!ib) {
          return true;
        }
        const BaseEntity ie = BaseEntity::deserialize(iid, *ib);
        if (ie.getFieldAsString("process_id").value_or("") != pid) {
          return true;
        }

        const std::string tid = keyStr.substr(p1 + 1);
        const std::vector<uint8_t> blob(val.begin(), val.end());
        const BaseEntity te = BaseEntity::deserialize(tid, blob);
        return cb(iid, tid, te);
    });
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findTasksInArea(
    std::string_view process_id,
    double center_lon,
    double center_lat,
    double radius_km
) const {
    std::vector<ProcessToken> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};
    if (radius_km <= 0.0) return {Status::Error("radius_km must be > 0"), result};

    const std::string pid(process_id);
    scanProcessTokens(db_, pid,
        [&](const std::string& iid, const std::string& tid, const BaseEntity& te) {
            nlohmann::json vars = parseJsonObjectOrEmpty(
                te.getFieldAsString("variables"), "findTasksInArea", "variables");

            double lon, lat;
            if (!extractTokenGeo(vars, lon, lat)) {
              return true;
            }

            const double dist = processGraphHaversineKm(center_lon, center_lat, lon, lat);
            if (dist > radius_km) {
              return true;
            }

            ProcessToken token;
            token.token_id            = tid;
            token.process_instance_id = iid;
            token.current_node        = te.getFieldAsString("current_node").value_or("");
            token.created_at_ms       = te.getFieldAsInt("created_at").value_or(0);
            token.variables           = vars;
            result.push_back(std::move(token));
            return true;
        },
        [this](std::string_view iid) { return makeInstanceKey_(iid); });

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findTasksInGeofence(
    std::string_view process_id,
    std::string_view geofence_wkt
) const {
    std::vector<ProcessToken> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    const std::string wkt(geofence_wkt);
    const auto ring = parseWktPolygon(wkt);
    if (ring.size() < 3) return {Status::Error("Invalid or empty WKT polygon"), result};

    const std::string pid(process_id);
    scanProcessTokens(db_, pid,
        [&](const std::string& iid, const std::string& tid, const BaseEntity& te) {
            nlohmann::json vars = parseJsonObjectOrEmpty(
                te.getFieldAsString("variables"), "findTasksInGeofence", "variables");

            double lon, lat;
            if (!extractTokenGeo(vars, lon, lat)) {
              return true;
            }
            if (!pointInPolygon(lon, lat, ring)) {
              return true;
            }

            ProcessToken token;
            token.token_id            = tid;
            token.process_instance_id = iid;
            token.current_node        = te.getFieldAsString("current_node").value_or("");
            token.created_at_ms       = te.getFieldAsInt("created_at").value_or(0);
            token.variables           = vars;
            result.push_back(std::move(token));
            return true;
        },
        [this](std::string_view iid) { return makeInstanceKey_(iid); });

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::RouteStop>>
ProcessGraphManager::optimizeTaskRoute(
    const std::vector<std::string>& task_ids,
    double start_lon,
    double start_lat
) const {
    std::vector<RouteStop> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};
    if (task_ids.empty()) return {Status::OK(), result};

    // Load each token and extract its geo coordinates.
    struct Stop {
        ProcessToken token;
        double lon{0.0};
        double lat{0.0};
        bool has_geo{false};
    };
    std::vector<Stop> stops = {};

    stops.reserve(task_ids.size());

    for (const auto& tid : task_ids) {
        // Tokens can be in different instances; scan all to find by token_id.
        Stop stop;
        bool found = false;
        db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) {
            if (found) {
              return false;
            }
            const std::string keyStr(key);
            const size_t p = keyStr.rfind(':');
            if (p == std::string::npos || keyStr.substr(p + 1) != tid) {
              return true;
            }

            const std::string iid = keyStr.substr(14, p - 14 - 1);
            const std::vector<uint8_t> blob(val.begin(), val.end());
            const BaseEntity te = BaseEntity::deserialize(tid, blob);

            nlohmann::json vars = parseJsonObjectOrEmpty(
                te.getFieldAsString("variables"), "optimizeTaskRoute", "variables");

            stop.token.token_id            = tid;
            stop.token.process_instance_id = iid;
            stop.token.current_node        = te.getFieldAsString("current_node").value_or("");
            stop.token.created_at_ms       = te.getFieldAsInt("created_at").value_or(0);
            stop.token.variables           = vars;
            stop.has_geo = extractTokenGeo(vars, stop.lon, stop.lat);
            found = true;
            return false;
        });
        if (found) {
          stops.push_back(std::move(stop));
        }
    }

    if (stops.empty()) return {Status::OK(), result};

    // Nearest-neighbor greedy TSP from (start_lon, start_lat).
    std::vector<bool> visited(stops.size(), false);
    double curLon = start_lon, curLat = start_lat;
    double prevLon = start_lon, prevLat = start_lat;

    result.reserve(stops.size());
    for (size_t step = 0; step < stops.size(); ++step) {
        double bestDist = std::numeric_limits<double>::max();
        size_t bestIdx  = 0;
        for (size_t i = 0; i < stops.size(); ++i) {
            if (visited[i]) {
              continue;
            }
            const double d = stops[i].has_geo
                ? processGraphHaversineKm(curLon, curLat, stops[i].lon, stops[i].lat)
                : 0.0;
            if (d < bestDist) { bestDist = d; bestIdx = i; }
        }
        visited[bestIdx] = true;
        const auto& s = stops[bestIdx];
        const double travelDist = s.has_geo
            ? processGraphHaversineKm(prevLon, prevLat, s.lon, s.lat) : 0.0;

        RouteStop rs;
        rs.token                     = s.token;
        rs.distance_from_prev_km     = travelDist;
        rs.estimated_travel_time_min = travelDist / 50.0 * 60.0; // assume 50 km/h
        result.push_back(std::move(rs));

        if (s.has_geo) { prevLon = curLon = s.lon; prevLat = curLat = s.lat; }
    }

    return {Status::OK(), result};
}

std::pair<ProcessGraphManager::Status, bool>
ProcessGraphManager::validateLocationConstraint(
    std::string_view instance_id,
    std::string_view task_node,
    double execution_lon,
    double execution_lat
) const {
    if (!db_.isOpen()) return {Status::Error("Database not open"), false};

    // Get the process instance to find the process_definition_id.
    auto [st, instance] = getProcessInstance(instance_id);
    if (!st.ok) return {st, false};

    // Load the node definition to get location_constraint and max_distance_km.
    const std::string nodeKey = makeNodeKey_(instance.process_definition_id, task_node);
    const auto nodeBlob = db_.get(nodeKey);
    if (!nodeBlob) {
        // Node not found — no constraint; allow execution.
        return {Status::OK(), true};
    }
    const BaseEntity nodeEntity = BaseEntity::deserialize(std::string(task_node), *nodeBlob);

    const auto locationConstraint = nodeEntity.getFieldAsString("location_constraint");
    const auto maxDistKm          = nodeEntity.getFieldAsDouble("max_distance_km");

    if (!locationConstraint && !maxDistKm) {
        return {Status::OK(), true}; // no geo constraint defined
    }

    // 1. WKT polygon constraint.
    if (locationConstraint && !locationConstraint->empty()) {
        const auto ring = parseWktPolygon(*locationConstraint);
        if (ring.size() >= 3) {
            const bool inside = pointInPolygon(execution_lon, execution_lat, ring);
            if (!inside) {
                return {Status::Error("Execution location is outside the required geofence"), false};
            }
        }
    }

    // 2. Max-distance constraint from the node's reference coordinates.
    if (maxDistKm) {
        const auto refLon = nodeEntity.getFieldAsDouble("ref_lon");
        const auto refLat = nodeEntity.getFieldAsDouble("ref_lat");
        if (refLon && refLat) {
            const double dist = processGraphHaversineKm(*refLon, *refLat,
                                                        execution_lon, execution_lat);
            if (dist > *maxDistKm) {
                return {Status::Error("Execution location exceeds max_distance_km constraint"), false};
            }
        }
    }

    return {Status::OK(), true};
}

std::pair<ProcessGraphManager::Status, nlohmann::json>
ProcessGraphManager::getRegionalParameters(
    std::string_view process_id,
    double lon,
    double lat
) const {
    if (!db_.isOpen()) return {Status::Error("Database not open"), nlohmann::json::object()};

    // Load the process definition.
    const auto procBlob = db_.get(makeProcessKey_(process_id));
    if (!procBlob) return {Status::Error("Process definition not found"), nlohmann::json::object()};
    const BaseEntity procEntity = BaseEntity::deserialize(std::string(process_id), *procBlob);

    // Try to load a "regional_parameters" JSON field from the process definition.
    // Format: JSON object mapping region-code or WKT-polygon to parameter overrides.
    // e.g. {"POLYGON((...))": {"tax_rate": 0.19, "currency": "EUR"}}
    const auto regParamsStr = procEntity.getFieldAsString("regional_parameters");
    if (!regParamsStr || regParamsStr->empty()) {
        return {Status::OK(), nlohmann::json::object()};
    }

    nlohmann::json regParams;
    try {
        regParams = nlohmann::json::parse(*regParamsStr);
    } catch (const std::exception& e) {
        return {Status::Error(std::string("Failed to parse regional_parameters JSON: ") + e.what()), nlohmann::json::object()};
    }

    // Iterate entries: key is WKT polygon, value is parameter map.
    nlohmann::json merged = nlohmann::json::object();
    if (regParams.is_object()) {
        for (auto& [key, params] : regParams.items()) {
            if (key.substr(0, 7) == "POLYGON") {
                const auto ring = parseWktPolygon(key);
                if (ring.size() >= 3 && pointInPolygon(lon, lat, ring)) {
                    if (params.is_object()) {
                        for (auto& [pk, pv] : params.items()) {
                          merged[pk] = pv;
                        }
                    }
                }
            }
        }
    }

    return {Status::OK(), merged};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::MultiModelResult>>
ProcessGraphManager::executeMultiModelQuery(
    std::string_view process_id,
    const MultiModelQuery& query
) const {
    std::vector<MultiModelResult> result;

    if (!db_.isOpen()) return {Status::Error("Database not open"), result};

    const std::string pid(process_id);
    std::unordered_set<std::string> edgeTypeFilter = {};

    if (!query.edge_types.empty()) {
        edgeTypeFilter.reserve(query.edge_types.size());
        edgeTypeFilter.insert(query.edge_types.begin(), query.edge_types.end());
    }

    // Build geofence ring once (if a geo constraint is present).
    std::vector<std::pair<double,double>> geofenceRing;
    if (query.within_geofence && !query.within_geofence->empty()) {
        geofenceRing = parseWktPolygon(*query.within_geofence);
    }

    // If a graph traversal constraint is set, collect the reachable node set.
    std::unordered_set<std::string> allowedNodes = {};

    if (query.start_node) {
        const int maxDepth = query.max_depth.value_or(10);
        // BFS from start_node over allowed edge types.
        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string,int>> bfsQ;
        bfsQ.push({*query.start_node, 0});
        while (!bfsQ.empty()) {
            auto [node, depth] = bfsQ.front(); bfsQ.pop();
            if (visited.count(node)) {
              continue;
            }
            visited.insert(node);
            allowedNodes.insert(node);
            if (depth >= maxDepth) {
              continue;
            }

            const std::string edgePrefix = "process:edge:" + pid + ":";
            db_.scanPrefix(edgePrefix, [&](std::string_view ekey, std::string_view eval) {
                const std::string ekStr(ekey);
                const size_t ep = ekStr.rfind(':');
                if (ep == std::string::npos) {
                  return true;
                }
                const std::string eid = ekStr.substr(ep + 1);
                const std::vector<uint8_t> eblob(eval.begin(), eval.end());
                const BaseEntity ee = BaseEntity::deserialize(eid, eblob);
                if (ee.getFieldAsString("_from").value_or("") != node) {
                  return true;
                }

                // Check edge type filter.
                if (!edgeTypeFilter.empty()) {
                    const std::string et = ee.getFieldAsString("_type").value_or("");
                    if (!edgeTypeFilter.count(et)) {
                      return true;
                    }
                }

                const std::string toNode = ee.getFieldAsString("_to").value_or("");
                if (!toNode.empty() && !visited.count(toNode)) {
                    bfsQ.push({toNode, depth + 1});
                }
                return true;
            });
        }
    }

    // Scan all tokens for the process.
    scanProcessTokens(db_, pid,
        [&](const std::string& iid, const std::string& tid, const BaseEntity& te) {
            const std::string curNode = te.getFieldAsString("current_node").value_or("");

            // 1. Graph filter: node must be in allowed set (if graph constraint active).
            if (!allowedNodes.empty() && !allowedNodes.count(curNode)) {
              return true;
            }

            // 2. Relational filter.
            nlohmann::json vars = parseJsonObjectOrEmpty(
                te.getFieldAsString("variables"), "executeMultiModelQuery", "variables");

            if (!query.filter_conditions.is_null() && query.filter_conditions.is_object()) {
                for (auto& [field, expected] : query.filter_conditions.items()) {
                    if (!vars.contains(field) || vars[field] != expected) {
                      return true;
                    }
                }
            }

            // 3. Geo filter.
            double tokenLon = 0.0, tokenLat = 0.0;
            bool hasGeo = extractTokenGeo(vars, tokenLon, tokenLat);

            if (!geofenceRing.empty()) {
                if (!hasGeo || !pointInPolygon(tokenLon, tokenLat, geofenceRing)) {
                  return true;
                }
            }
            std::optional<double> distKm = {};

            if (query.from_location && hasGeo) {
                const double d = processGraphHaversineKm(
                    query.from_location->first, query.from_location->second,
                    tokenLon, tokenLat);
                if (query.max_distance_km && d > *query.max_distance_km) {
                  return true;
                }
                distKm = d;
            }

            // 4. Vector filter.
            std::optional<float> simScore = {};

            if (query.similarity_vector && !query.similarity_vector->empty()) {
                const auto embStr = te.getFieldAsString("embedding");
                if (!embStr) {
                  return true;
                }
                const auto emb = parseEmbeddingJson(*embStr);
                if (emb.empty()) {
                  return true;
                }
                const float sim = computeCosineSimilarity(*query.similarity_vector, emb);
                if (query.min_similarity && sim < *query.min_similarity) {
                  return true;
                }
                simScore = sim;
            }

            // Build token.
            ProcessToken token;
            token.token_id            = tid;
            token.process_instance_id = iid;
            token.current_node        = curNode;
            token.created_at_ms       = te.getFieldAsInt("created_at").value_or(0);
            token.variables           = vars;

            // Extract selected fields.
            nlohmann::json selectedFields = nlohmann::json::object();
            for (const auto& field : query.select_fields) {
                if (vars.contains(field)) {
                  selectedFields[field] = vars[field];
                }
            }

            MultiModelResult mmr;
            mmr.token           = std::move(token);
            mmr.selected_fields = std::move(selectedFields);
            mmr.similarity_score = simScore;
            mmr.distance_km      = distKm;
            result.push_back(std::move(mmr));
            return true;
        },
        [this](std::string_view iid) { return makeInstanceKey_(iid); });

    // Sort by similarity (if vector constraint), then by distance (if geo constraint).
    if (query.similarity_vector) {
        std::sort(result.begin(), result.end(),
                  [](const MultiModelResult& a, const MultiModelResult& b) {
                      return a.similarity_score.value_or(0) > b.similarity_score.value_or(0);
                  });
    } else if (query.from_location) {
        std::sort(result.begin(), result.end(),
                  [](const MultiModelResult& a, const MultiModelResult& b) {
                      return a.distance_km.value_or(1e9) < b.distance_km.value_or(1e9);
                  });
    }

    return {Status::OK(), result};
}
ProcessGraphManager::Status ProcessGraphManager::createToken_(
    ProcessInstance& instance,
    std::string_view node_id
) {
    ProcessToken token;
    token.token_id = generateTokenId_();
    token.process_instance_id = instance.instance_id;
    token.current_node = std::string(node_id);
    token.state = ProcessToken::State::READY;
    token.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    instance.tokens.push_back(token);
    return Status::OK();
}

ProcessGraphManager::Status ProcessGraphManager::moveToken_(
    [[maybe_unused]] ProcessInstance& instance,
    ProcessToken& token,
    std::string_view target_node
) {
    token.current_node = std::string(target_node);
    token.visited_nodes.push_back(std::string(target_node));
    token.visit_timestamps[std::string(target_node)] = std::chrono::system_clock::now();
    return Status::OK();
}

std::vector<std::string> ProcessGraphManager::evaluateGateway_(
    [[maybe_unused]] const ProcessNodeInfo& gateway,
    [[maybe_unused]] const ProcessToken& token,
    const std::vector<ProcessEdgeInfo>& outgoing_edges
) const {
    
    std::vector<std::string> targets = {};

    targets.reserve(outgoing_edges.size());
    for (const auto& edge : outgoing_edges) {
        targets.push_back(edge.to_node);
    }
    return targets;
}

bool ProcessGraphManager::checkHyperedgeCondition_(const Hyperedge& hyperedge) const {
    switch (hyperedge.sync_type) {
        case Hyperedge::SyncType::AND_JOIN:
            return hyperedge.activated_sources.size() == hyperedge.source_nodes.size();
        case Hyperedge::SyncType::OR_JOIN:
            return !hyperedge.activated_sources.empty();
        case Hyperedge::SyncType::N_OF_M_JOIN:
            return hyperedge.required_count.has_value() && 
                   hyperedge.activated_sources.size() >= static_cast<size_t>(*hyperedge.required_count);
        case Hyperedge::SyncType::DISCRIMINATOR:
            return hyperedge.activated_sources.size() == 1;
        default:
            return false;
    }
}

ProcessGraphManager::Status ProcessGraphManager::activateHyperedgeSource_(
    Hyperedge& hyperedge,
    std::string_view source_node
) {
    hyperedge.activated_sources.insert(std::string(source_node));
    hyperedge.is_complete = checkHyperedgeCondition_(hyperedge);
    return Status::OK();
}

// ============================================================================
// Register Process Edge Types
// ============================================================================

void registerProcessEdgeTypes() {
    THEMIS_INFO("registerProcessEdgeTypes: deferred (edge registry module not linked in this build)");
}

} // namespace themis

