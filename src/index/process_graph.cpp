/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            process_graph.cpp                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:26:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     2103                                           ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 354c97d283  2026-03-16  feat: Add new erasure coding backend and related components ║
    • b308eb2146  2026-03-15  fix: persist visited_nodes/visit_timestamps in COMPLETED ... ║
    • c4ae3846c4  2026-03-15  feat(network): implement ProcessGraphVisitLog and getVisi... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Process Graph Manager Implementation
// Supports BPMN, EPK, and advanced process modeling patterns

#include "index/process_graph.h"
#include "index/edge_types.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "utils/logger.h"

#include <queue>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>
#include <unordered_set>

namespace themis {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
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
    std::string op;
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
            if (val.is_boolean()) return val.get<bool>();
            if (val.is_number()) return val.get<double>() != 0;
            if (val.is_string()) return !val.get<std::string>().empty();
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
    std::string syncTypeStr;
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
    ValidationResult result;
    
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
                if (typeStr == "START_EVENT") info.node_type = BPMNNodeType::START_EVENT;
                else if (typeStr == "END_EVENT") info.node_type = BPMNNodeType::END_EVENT;
                else if (typeStr == "TASK") info.node_type = BPMNNodeType::TASK;
                else if (typeStr == "EXCLUSIVE_GATEWAY") info.node_type = BPMNNodeType::EXCLUSIVE_GATEWAY;
                else if (typeStr == "PARALLEL_GATEWAY") info.node_type = BPMNNodeType::PARALLEL_GATEWAY;
                else info.node_type = BPMNNodeType::TASK; // Default
            } else {
                if (typeStr == "EVENT") info.node_type = EPKNodeType::EVENT;
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
    
    // 1. Check for start node
    bool hasStart = false;
    bool hasEnd = false;
    for (const auto& [id, node] : nodes) {
        if (isStartNode(node)) hasStart = true;
        if (isEndNode(node)) hasEnd = true;
    }
    if (!hasStart) {
        result.errors.push_back("Process has no start event");
    }
    if (!hasEnd) {
        result.warnings.push_back("Process has no end event");
    }

    // 2. Check for orphan nodes (no incoming or outgoing edges)
    std::unordered_set<std::string> hasIncoming;
    std::unordered_set<std::string> hasOutgoing;
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
                if (edge.to_node == id) incoming++;
                if (edge.from_node == id) outgoing++;
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
    std::string startNodeId;
    std::string nodePrefix = "process:node:" + std::string(process_id) + ":";
    db_.scanPrefix(nodePrefix, [&startNodeId](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon != std::string::npos) {
            std::string nodeId = keyStr.substr(lastColon + 1);
            std::vector<uint8_t> blob(val.begin(), val.end());
            BaseEntity entity = BaseEntity::deserialize(nodeId, blob);
            
            auto nodeType = entity.getFieldAsString("node_type").value_or("");
            if (nodeType == "START_EVENT" || nodeType == "EVENT") {
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
    ProcessInstance instance;
    
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
    if (stateStr == "RUNNING") instance.state = ProcessInstance::State::RUNNING;
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
            if (stateStr == "READY") token.state = ProcessToken::State::READY;
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
    if (!st.ok) return std::nullopt;

    std::optional<std::chrono::system_clock::time_point> result;
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
    if (!st.ok) return st;

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
                if (cond) edge.condition_expression = *cond;
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
        
        return Status::OK();
    }

    // Sort by priority
    std::sort(outgoing.begin(), outgoing.end(), 
        [](const ProcessEdgeInfo& a, const ProcessEdgeInfo& b) {
            return a.priority > b.priority;
        });

    // Evaluate conditions and select the first matching edge
    std::string targetNode;
    std::string defaultNode;
    
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
    if (!st.ok) return st;

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
    if (!st.ok) return st;
    
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
        if (!nodeBlob) continue;
        
        BaseEntity nodeEntity = BaseEntity::deserialize(token.current_node, *nodeBlob);
        auto nodeTypeStr = nodeEntity.getFieldAsString("node_type").value_or("");
        auto subtypeStr = nodeEntity.getFieldAsString("subtype").value_or("");
        
        // Check if this is a message or signal catching event
        bool isMatchingEvent = false;
        if (nodeTypeStr == "INTERMEDIATE_EVENT" || nodeTypeStr == "BOUNDARY_EVENT") {
            // Check if the event name matches
            auto eventNameField = nodeEntity.getFieldAsString("event_name");
            if (eventNameField && *eventNameField == std::string(event_name)) {
                isMatchingEvent = true;
            }
        }
        
        if (!isMatchingEvent) continue;
        
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
        return Status::Error("No tokens waiting for event '" + std::string(event_name) + "'");
    }
    
    return Status::OK();
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
        if (secondColon == std::string::npos) return true;
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) return true;
        
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
            
            if (stateStr == "READY") token.state = ProcessToken::State::READY;
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
        if (secondColon == std::string::npos) return true;
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) return true;
        
        std::string instanceId = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
        std::string tokenId = keyStr.substr(thirdColon + 1);
        
        // Get instance to check process_id
        std::string instanceKey = makeInstanceKey_(instanceId);
        auto instanceBlob = db_.get(instanceKey);
        if (!instanceBlob) return true;
        
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
        
        if (!visitedNode) return true;
        
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
        if (stateStr == "READY") token.state = ProcessToken::State::READY;
        else if (stateStr == "ACTIVE") token.state = ProcessToken::State::ACTIVE;
        else if (stateStr == "COMPLETED") token.state = ProcessToken::State::COMPLETED;
        else if (stateStr == "WAITING") token.state = ProcessToken::State::WAITING;
        else if (stateStr == "FAILED") token.state = ProcessToken::State::FAILED;
        
        // Load timestamps
        auto startedAt = tokenEntity.getFieldAsInt("started_at");
        if (startedAt) token.started_at_ms = *startedAt;
        
        auto completedAt = tokenEntity.getFieldAsInt("completed_at");
        if (completedAt) token.completed_at_ms = *completedAt;
        
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
        if (secondColon == std::string::npos) return true;
        
        size_t thirdColon = keyStr.find(':', secondColon + 1);
        if (thirdColon == std::string::npos) return true;
        
        std::string instanceId = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
        std::string tokenId = keyStr.substr(thirdColon + 1);
        
        // Check if this instance belongs to the target process
        std::string instanceKey = makeInstanceKey_(instanceId);
        auto instanceBlob = db_.get(instanceKey);
        if (!instanceBlob) return true;
        
        BaseEntity instanceEntity = BaseEntity::deserialize(instanceId, *instanceBlob);
        auto instanceProcessId = instanceEntity.getFieldAsString("process_id").value_or("");
        
        if (instanceProcessId != std::string(process_id)) {
            return true;
        }
        
        // Parse token
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, blob);
        
        auto currentNode = tokenEntity.getFieldAsString("current_node").value_or("");
        if (currentNode.empty()) return true;
        
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
    std::unordered_map<std::string, double> nodeDurations;
    
    // Load node durations from metrics
    for (const auto& m : metrics) {
        nodeDurations[m.node_id] = m.avg_duration_ms;
    }
    
    // Load edges to build adjacency list
    std::string edgePrefix = "process:edge:" + std::string(process_id) + ":";
    db_.scanPrefix(edgePrefix, [&adjacency](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) return true;
        
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
    std::string startNode;
    std::string nodePrefix = "process:node:" + std::string(process_id) + ":";
    db_.scanPrefix(nodePrefix, [&startNode](std::string_view key, std::string_view val) {
        std::string keyStr(key);
        size_t lastColon = keyStr.rfind(':');
        if (lastColon == std::string::npos) return true;
        
        std::string nodeId = keyStr.substr(lastColon + 1);
        std::vector<uint8_t> blob(val.begin(), val.end());
        BaseEntity entity = BaseEntity::deserialize(nodeId, blob);
        
        auto nodeType = entity.getFieldAsString("node_type").value_or("");
        if (nodeType == "START_EVENT" || nodeType == "EVENT") {
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
    std::vector<std::string> longestPath;
    double maxDuration = 0.0;
    
    // Stack entry: (node, cumDuration, path, visited)
    struct StackEntry {
        std::string node;
        double cumDuration;
        std::vector<std::string> path;
        std::unordered_set<std::string> visited;
    };
    
    std::vector<StackEntry> stack;
    stack.push_back({startNode, 0.0, {}, {}});
    
    while (!stack.empty()) {
        auto entry = std::move(stack.back());
        stack.pop_back();
        
        // Skip if already visited in this path
        if (entry.visited.count(entry.node)) {
            continue;
        }
        
        // Mark as visited
        entry.visited.insert(entry.node);
        entry.path.push_back(entry.node);
        
        // Add node duration
        double nodeDur = nodeDurations.count(entry.node) ? nodeDurations[entry.node] : 0.0;
        entry.cumDuration += nodeDur;
        
        // Check if this is longest path so far
        if (entry.cumDuration > maxDuration) {
            maxDuration = entry.cumDuration;
            longestPath = entry.path;
        }
        
        // Explore neighbors
        if (adjacency.count(entry.node)) {
            for (const auto& neighbor : adjacency[entry.node]) {
                if (entry.visited.find(neighbor) == entry.visited.end()) {
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
    Hyperedge hyperedge;
    
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
        if (syncTypeStr == "AND_JOIN") hyperedge.sync_type = Hyperedge::SyncType::AND_JOIN;
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
// Multi-Model Query Stubs
// ============================================================================

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::queryTasksByFormData(
    std::string_view process_id,
    const nlohmann::json& filter_conditions
) const {
    (void)process_id;
    (void)filter_conditions;
    return {Status::Error("Form data queries not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::JoinResult>>
ProcessGraphManager::joinWithCollection(
    std::string_view process_id,
    std::string_view collection_name,
    std::string_view local_field,
    std::string_view foreign_field
) const {
    (void)process_id;
    (void)collection_name;
    (void)local_field;
    (void)foreign_field;
    return {Status::Error("Collection joins not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::AggregateResult>>
ProcessGraphManager::aggregateByField(
    std::string_view process_id,
    std::string_view group_field,
    std::string_view agg_field,
    std::string_view agg_function
) const {
    (void)process_id;
    (void)group_field;
    (void)agg_field;
    (void)agg_function;
    return {Status::Error("Field aggregation not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::SimilarProcess>>
ProcessGraphManager::findSimilarProcesses(
    const std::vector<float>& query_embedding,
    size_t k,
    float min_similarity
) const {
    (void)query_embedding;
    (void)k;
    (void)min_similarity;
    return {Status::Error("Similar process search not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findSimilarTasks(
    std::string_view instance_id,
    std::string_view task_node,
    size_t k
) const {
    (void)instance_id;
    (void)task_node;
    (void)k;
    return {Status::Error("Similar task search not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::SimilarProcess>>
ProcessGraphManager::semanticSearchProcesses(
    std::string_view natural_language_query,
    size_t k
) const {
    (void)natural_language_query;
    (void)k;
    return {Status::Error("Semantic search not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::AnomalyResult>>
ProcessGraphManager::detectAnomalies(
    std::string_view process_id,
    float threshold
) const {
    (void)process_id;
    (void)threshold;
    return {Status::Error("Anomaly detection not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findTasksInArea(
    std::string_view process_id,
    double center_lon,
    double center_lat,
    double radius_km
) const {
    (void)process_id;
    (void)center_lon;
    (void)center_lat;
    (void)radius_km;
    return {Status::Error("Geo area search not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::findTasksInGeofence(
    std::string_view process_id,
    std::string_view geofence_wkt
) const {
    (void)process_id;
    (void)geofence_wkt;
    return {Status::Error("Geofence search not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::RouteStop>>
ProcessGraphManager::optimizeTaskRoute(
    const std::vector<std::string>& task_ids,
    double start_lon,
    double start_lat
) const {
    (void)task_ids;
    (void)start_lon;
    (void)start_lat;
    return {Status::Error("Route optimization not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, bool>
ProcessGraphManager::validateLocationConstraint(
    std::string_view instance_id,
    std::string_view task_node,
    double execution_lon,
    double execution_lat
) const {
    (void)instance_id;
    (void)task_node;
    (void)execution_lon;
    (void)execution_lat;
    return {Status::Error("Location validation not yet implemented"), false};
}

std::pair<ProcessGraphManager::Status, nlohmann::json>
ProcessGraphManager::getRegionalParameters(
    std::string_view process_id,
    double lon,
    double lat
) const {
    (void)process_id;
    (void)lon;
    (void)lat;
    return {Status::Error("Regional parameters not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::MultiModelResult>>
ProcessGraphManager::executeMultiModelQuery(
    std::string_view process_id,
    const MultiModelQuery& query
) const {
    (void)process_id;
    (void)query;
    return {Status::Error("Multi-model query not yet implemented"), {}};
}

// Private helper stubs
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
    ProcessInstance& instance,
    ProcessToken& token,
    std::string_view target_node
) {
    (void)instance;
    token.current_node = std::string(target_node);
    token.visited_nodes.push_back(std::string(target_node));
    token.visit_timestamps[std::string(target_node)] = std::chrono::system_clock::now();
    return Status::OK();
}

std::vector<std::string> ProcessGraphManager::evaluateGateway_(
    const ProcessNodeInfo& gateway,
    const ProcessToken& token,
    const std::vector<ProcessEdgeInfo>& outgoing_edges
) const {
    (void)gateway;
    (void)token;
    
    std::vector<std::string> targets;
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
    auto& registry = EdgeTypeRegistry::instance();

    // BPMN Sequence Flow
    registry.registerType({
        .type_name = "SEQUENCE_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "BPMN sequence flow connecting activities",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // BPMN Message Flow
    registry.registerType({
        .type_name = "MESSAGE_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "BPMN message flow between pools",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // EPK Control Flow
    registry.registerType({
        .type_name = "CONTROL_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "EPK control flow between events and functions",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // EPK Information Flow
    registry.registerType({
        .type_name = "INFORMATION_FLOW",
        .category = EdgeCategory::REFERENCE,
        .description = "EPK information flow to/from functions",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "INFORMATION_FLOW"
    });

    // Data Association
    registry.registerType({
        .type_name = "DATA_ASSOCIATION",
        .category = EdgeCategory::REFERENCE,
        .description = "Data input/output association",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // Conditional Flow
    registry.registerType({
        .type_name = "CONDITIONAL_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "Flow with guard condition",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = true,  // Weight can be used for priority
        .inverse_type = std::nullopt
    });

    // Default Flow
    registry.registerType({
        .type_name = "DEFAULT_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "Default path from exclusive gateway",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // Exception Flow
    registry.registerType({
        .type_name = "EXCEPTION_FLOW",
        .category = EdgeCategory::WORKFLOW,
        .description = "Error/compensation flow",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // Organization Assignment
    registry.registerType({
        .type_name = "ASSIGNED_TO",
        .category = EdgeCategory::ACCESS,
        .description = "Task assignment to user/role",
        .is_bidirectional = false,
        .requires_temporal = true,  // Assignments can be time-bound
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // Process Reference
    registry.registerType({
        .type_name = "CALLS_PROCESS",
        .category = EdgeCategory::REFERENCE,
        .description = "Call activity references subprocess",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    THEMIS_INFO("Registered BPMN/EPK process edge types");
}

} // namespace themis
