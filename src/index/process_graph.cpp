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

    BaseEntity process(std::string(process_id));
    process.setField("id", std::string(process_id));
    process.setField("name", std::string(name));
    process.setField("created_at", currentTimeMs());
    if (!bpmn_xml.empty()) {
        process.setField("bpmn_xml", std::string(bpmn_xml));
    }

    std::string key = makeProcessKey_(process_id);
    if (!db_.put(key, process.serialize())) {
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
            BaseEntity entity = BaseEntity::deserialize(nodeId, val);
            
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
            BaseEntity entity = BaseEntity::deserialize(edgeId, val);
            
            ProcessEdgeInfo info;
            info.edge_id = edgeId;
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
            BaseEntity entity = BaseEntity::deserialize(nodeId, val);
            
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
            BaseEntity tokenEntity = BaseEntity::deserialize(tokenId, val);
            
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
            
            instance.tokens.push_back(token);
        }
        return true;
    });

    return {Status::OK(), instance};
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
            BaseEntity entity = BaseEntity::deserialize(edgeId, val);
            
            auto from = entity.getFieldAsString("_from").value_or("");
            if (from == token->current_node) {
                ProcessEdgeInfo edge;
                edge.edge_id = edgeId;
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
        
        // Update token in DB
        BaseEntity tokenEntity(std::string(token_id));
        tokenEntity.setField("id", std::string(token_id));
        tokenEntity.setField("instance_id", std::string(instance_id));
        tokenEntity.setField("current_node", token->current_node);
        tokenEntity.setField("state", "COMPLETED");
        tokenEntity.setField("completed_at", *token->completed_at_ms);
        
        std::string tokenKey = makeTokenKey_(instance_id, token_id);
        db_.put(tokenKey, tokenEntity.serialize());
        
        return Status::OK();
    }

    // Sort by priority
    std::sort(outgoing.begin(), outgoing.end(), 
        [](const ProcessEdgeInfo& a, const ProcessEdgeInfo& b) {
            return a.priority > b.priority;
        });

    // For now, take the first edge (or default)
    // TODO: Implement condition evaluation
    std::string targetNode;
    for (const auto& edge : outgoing) {
        if (edge.is_default || !edge.condition_expression.has_value()) {
            targetNode = edge.to_node;
            token->traversed_edges.push_back(edge.edge_id);
            break;
        }
    }

    if (targetNode.empty() && !outgoing.empty()) {
        targetNode = outgoing[0].to_node;
        token->traversed_edges.push_back(outgoing[0].edge_id);
    }

    // Move token
    token->current_node = targetNode;
    token->visited_nodes.push_back(targetNode);

    // Update token in DB
    BaseEntity tokenEntity(std::string(token_id));
    tokenEntity.setField("id", std::string(token_id));
    tokenEntity.setField("instance_id", std::string(instance_id));
    tokenEntity.setField("current_node", token->current_node);
    tokenEntity.setField("state", "READY");
    
    std::string tokenKey = makeTokenKey_(instance_id, token_id);
    db_.put(tokenKey, tokenEntity.serialize());

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
    // TODO: Implement event handling for message/signal catching events
    (void)instance_id;
    (void)event_name;
    (void)payload;
    return Status::Error("Event signaling not yet implemented");
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>> 
ProcessGraphManager::findActiveTasks(std::string_view assignee_or_role) const {
    // TODO: Implement task assignment queries
    (void)assignee_or_role;
    return {Status::Error("Task assignment queries not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessToken>>
ProcessGraphManager::getNodeHistory(
    std::string_view process_id,
    std::string_view node_id,
    std::optional<int64_t> since_ms
) const {
    // TODO: Implement node history queries using temporal indices
    (void)process_id;
    (void)node_id;
    (void)since_ms;
    return {Status::Error("Node history queries not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<ProcessGraphManager::NodeMetrics>>
ProcessGraphManager::getProcessMetrics(std::string_view process_id) const {
    // TODO: Implement process metrics aggregation
    (void)process_id;
    return {Status::Error("Process metrics not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, std::vector<std::string>>
ProcessGraphManager::findCriticalPath(std::string_view process_id) const {
    // TODO: Implement critical path analysis
    (void)process_id;
    return {Status::Error("Critical path analysis not yet implemented"), {}};
}

std::pair<ProcessGraphManager::Status, Hyperedge>
ProcessGraphManager::getHyperedgeStatus(std::string_view hyperedge_id) const {
    Hyperedge hyperedge;
    // TODO: Implement hyperedge status retrieval
    (void)hyperedge_id;
    return {Status::Error("Hyperedge status not yet implemented"), hyperedge};
}

std::pair<ProcessGraphManager::Status, bool>
ProcessGraphManager::isHyperedgeReady(std::string_view hyperedge_id) const {
    // TODO: Implement hyperedge readiness check
    (void)hyperedge_id;
    return {Status::Error("Hyperedge readiness check not yet implemented"), false};
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
