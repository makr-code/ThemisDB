/**
 * @file process_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/edge_types.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <chrono>
#include <array>

namespace themis {

/**
 * @brief Integrierte Multi-Model Prozess-Architektur für ThemisDB
 * 
 * ## Design-Philosophie: Vollständige Integration
 * 
 * Prozesse werden NICHT als separate Entitäten behandelt, sondern als
 * **normale Collections** mit speziellen reservierten Feldern. Dies ermöglicht:
 * 
 * 1. **Einheitliche AQL-Syntax** - Keine neuen Sprachelemente erforderlich
 * 2. **Multi-Model-Kombination** - Graph + Relational + Vektor + Geo in einer Abfrage
 * 3. **Transparente Optimierung** - Query-Optimizer wählt beste Indizes
 * 
 * ## Reservierte Felder für Multi-Model
 * 
 * | Feld | Typ | Model | Beschreibung |
 * |------|-----|-------|--------------|
 * | `_from` | string | Graph | Quellknoten-ID |
 * | `_to` | string | Graph | Zielknoten-ID |
 * | `_type` | string | Graph/Process | Kanten-/Knotentyp |
 * | `_embedding` | float[] | Vector | Embedding-Array |
 * | `_geometry` | string | Geo | WKT oder GeoJSON |
 * | `_valid_from` | int64 | Temporal | Gültig ab (ms) |
 * | `_valid_to` | int64 | Temporal | Gültig bis (ms) |
 * | `_state` | string | Process | Ausführungszustand |
 * | `_tokens` | array | Process | Aktive Token |
 * | `_variables` | object | Process | Prozess-Variablen |
 * 
 * ## System-Collections für Prozesse
 * 
 * - `_process_definitions` - Prozess-Modelle (BPMN/EPK)
 * - `_process_nodes` - Knoten im Prozess-Modell
 * - `_process_edges` - Kanten/Flüsse im Prozess-Modell
 * - `_process_instances` - Laufende Prozess-Instanzen
 * - `_process_tokens` - Token (Ausführungsposition)
 * - `_process_history` - Audit-Log der Ausführung
 * 
 * ## Beispiel: Integrierte Multi-Model-Abfrage
 * 
 * ```aql
 * FOR task IN _process_tokens
 *   FOR node IN _process_nodes
 *     FILTER task.current_node == node.id
 *     FILTER node._type == "USER_TASK"
 *   LET customer = DOCUMENT("customers", task.variables.customerId)
 *   FILTER GEO_DISTANCE(node._geometry, customer._geometry) < 50000
 *   LET similar = SIMILARITY(task._embedding, customer._embedding, 5)
 *   RETURN { task, node, customer, similar }
 * ```
 */

// ============================================================================
// Reservierte Felder für Multi-Model Integration
// ============================================================================

/**
 * @brief Reservierte Feldnamen für Multi-Model-Dokumente
 * 
 * Diese Felder werden automatisch erkannt und entsprechend indiziert.
 */
namespace ReservedFields {
    // Graph-Felder
    constexpr const char* FROM = "_from";           ///< Quellknoten-ID
    constexpr const char* TO = "_to";               ///< Zielknoten-ID
    constexpr const char* TYPE = "_type";           ///< Kanten-/Knotentyp
    
    // Vektor-Felder
    constexpr const char* EMBEDDING = "_embedding"; ///< Embedding-Array
    
    // Geo-Felder
    constexpr const char* GEOMETRY = "_geometry";   ///< WKT oder GeoJSON
    
    // Temporal-Felder
    constexpr const char* VALID_FROM = "_valid_from"; ///< Gültig ab (ms)
    constexpr const char* VALID_TO = "_valid_to";     ///< Gültig bis (ms)
    
    // Prozess-Felder
    constexpr const char* STATE = "_state";         ///< Ausführungszustand
    constexpr const char* TOKENS = "_tokens";       ///< Aktive Token
    constexpr const char* VARIABLES = "_variables"; ///< Prozess-Variablen
    constexpr const char* PARENT = "_parent";       ///< Eltern-Instanz
    
    // Audit-Felder
    constexpr const char* CREATED_AT = "_created_at";
    constexpr const char* UPDATED_AT = "_updated_at";
    constexpr const char* CREATED_BY = "_created_by";
    constexpr const char* VERSION = "_version";
}

/**
 * @brief System-Collections für Prozesse
 */
namespace SystemCollections {
    constexpr const char* PROCESS_DEFINITIONS = "_process_definitions";
    constexpr const char* PROCESS_NODES = "_process_nodes";
    constexpr const char* PROCESS_EDGES = "_process_edges";
    constexpr const char* PROCESS_INSTANCES = "_process_instances";
    constexpr const char* PROCESS_TOKENS = "_process_tokens";
    constexpr const char* PROCESS_HISTORY = "_process_history";
}

/**
 * @brief Erkennt automatisch die Multi-Model-Aspekte eines Dokuments
 */
struct DocumentModelInfo {
    bool is_graph_node = false;     ///< Hat _type aber kein _from/_to
    bool is_graph_edge = false;     ///< Hat _from und _to
    bool has_vector = false;        ///< Hat _embedding
    bool has_geo = false;           ///< Hat _geometry
    bool has_temporal = false;      ///< Hat _valid_from oder _valid_to
    bool is_process = false;        ///< Hat _state
    
    static DocumentModelInfo detect(const BaseEntity& doc) {
        DocumentModelInfo info;
        
        // Graph-Erkennung
        bool hasFrom = doc.hasField(ReservedFields::FROM);
        bool hasTo = doc.hasField(ReservedFields::TO);
        info.is_graph_edge = hasFrom && hasTo;
        info.is_graph_node = doc.hasField(ReservedFields::TYPE) && !info.is_graph_edge;
        
        // Multi-Model-Erkennung
        info.has_vector = doc.hasField(ReservedFields::EMBEDDING);
        info.has_geo = doc.hasField(ReservedFields::GEOMETRY);
        info.has_temporal = doc.hasField(ReservedFields::VALID_FROM) || 
                           doc.hasField(ReservedFields::VALID_TO);
        info.is_process = doc.hasField(ReservedFields::STATE);
        
        return info;
    }
};

/**
 * @brief Vereinfachte Multi-Model Aspekte für Prozesselemente
 * 
 * Da wir reservierte Felder verwenden, ist dies hauptsächlich
 * für Metadaten und Konfiguration gedacht.
 */
struct MultiModelAspects {
    bool has_graph = true;          ///< Immer true für Prozesselemente
    bool has_relational = true;     ///< Meist true für Geschäftsdaten
    bool has_vector = false;        ///< Für semantische Suche
    bool has_geo = false;           ///< Für ortsbezogene Prozesse
};

// ============================================================================
// Process Node Types (BPMN + EPK)
// ============================================================================

/**
 * @brief BPMN Node Types
 */
enum class BPMNNodeType {
    // Events
    START_EVENT,
    END_EVENT,
    INTERMEDIATE_EVENT,
    BOUNDARY_EVENT,
    
    // Event Subtypes (via _subtype field)
    // MESSAGE, TIMER, ERROR, CANCEL, COMPENSATION, SIGNAL, LINK, TERMINATE
    
    // Activities
    TASK,                   // Atomic activity
    SUBPROCESS,             // Compound activity
    CALL_ACTIVITY,          // Reusable subprocess reference
    
    // Task Subtypes (via _subtype field)
    // USER_TASK, SERVICE_TASK, SCRIPT_TASK, SEND_TASK, RECEIVE_TASK, MANUAL_TASK, BUSINESS_RULE_TASK
    
    // Gateways
    EXCLUSIVE_GATEWAY,      // XOR - one path
    PARALLEL_GATEWAY,       // AND - all paths
    INCLUSIVE_GATEWAY,      // OR - one or more paths
    EVENT_BASED_GATEWAY,    // Wait for event
    COMPLEX_GATEWAY,        // Custom condition
    
    // Swimlanes
    POOL,
    LANE,
    
    // Artifacts
    DATA_OBJECT,
    DATA_STORE,
    GROUP,
    ANNOTATION
};

/**
 * @brief EPK (Event-driven Process Chain) Node Types
 */
enum class EPKNodeType {
    // Core Elements
    EVENT,              // Ereignis - describes a state/condition
    FUNCTION,           // Funktion - describes an activity
    
    // Connectors
    AND_CONNECTOR,      // All paths (parallel)
    OR_CONNECTOR,       // One or more paths
    XOR_CONNECTOR,      // Exactly one path
    
    // Extended Elements
    ORGANIZATIONAL_UNIT,    // Organisationseinheit
    INFORMATION_OBJECT,     // Informationsobjekt
    APPLICATION_SYSTEM,     // Anwendungssystem
    PROCESS_PATH            // Prozesswegweiser (reference to subprocess)
};

/**
 * @brief Process Edge Types (Flow connections)
 */
enum class ProcessEdgeType {
    // BPMN Flows
    SEQUENCE_FLOW,          // Normal control flow
    MESSAGE_FLOW,           // Between pools
    ASSOCIATION,            // Artifact connection
    DATA_ASSOCIATION,       // Data input/output
    
    // EPK Flows
    CONTROL_FLOW,           // Kontrollfluss
    INFORMATION_FLOW,       // Informationsfluss
    ORGANIZATION_FLOW,      // Zuordnung zu Org-Einheit
    
    // Execution Flows
    DEFAULT_FLOW,           // Default path from gateway
    CONDITIONAL_FLOW,       // Guarded by condition
    EXCEPTION_FLOW          // Error/compensation path
};

// ============================================================================
// Process Node Metadata
// ============================================================================

/**
 * @brief Metadata for process nodes
 */
struct ProcessNodeInfo {
    std::string node_id;
    std::string name;
    std::string description;
    
    // Type information
    std::variant<BPMNNodeType, EPKNodeType> node_type;
    std::string subtype;        // e.g., "USER_TASK", "MESSAGE" for events
    
    // ===== Multi-Model Aspects =====
    MultiModelAspects model_aspects;
    
    // Execution properties
    bool is_async = false;      // Asynchronous execution
    bool is_multi_instance = false;  // Loop/parallel execution
    std::optional<int> loop_cardinality;
    
    // Timing
    std::optional<std::chrono::milliseconds> timeout;
    std::optional<std::chrono::milliseconds> retry_delay;
    int max_retries = 0;
    
    // Script/Service configuration
    std::optional<std::string> script;          // For script tasks
    std::optional<std::string> service_ref;     // For service tasks
    std::optional<std::string> implementation;  // WebService, Java, etc.
    
    // Data
    std::vector<std::string> input_variables;
    std::vector<std::string> output_variables;
    
    // ===== Relational Data (when has_relational=true) =====
    nlohmann::json form_schema;                 ///< Form fields for user tasks
    std::vector<std::string> required_fields;   ///< Mandatory fields
    
    // ===== Vector Data (when has_vector=true) =====
    std::optional<std::vector<float>> description_embedding;  ///< Embedding of description
    std::optional<std::string> semantic_category;             ///< AI-classified category
    
    // ===== Geo Data (when has_geo=true) =====
    std::optional<std::string> location_constraint;   ///< WKT geofence where task can be executed
    std::optional<double> max_distance_km;            ///< Max distance from location
    std::optional<std::string> region;                ///< Geographic region code

    // ===== Layout / Diagram Interchange =====
    /// Optional graphical layout hints populated from BPMNDI (BPMNShape) on import.
    /// Schema: { "x": float, "y": float, "width": float, "height": float }
    nlohmann::json metadata;  ///< Extended key-value metadata (e.g. layout)

    // ===== BPMN-S DSGVO Security Annotations =====
    struct DsgvoAnnotation {
        std::string data_category;       ///< "personal", "sensitive", "anonymised"
        std::string legal_basis;         ///< e.g. "Art. 6(1)(e) DSGVO"
        std::optional<int> retention_days;
        bool requires_consent{false};
    };
    std::optional<DsgvoAnnotation> dsgvo_annotation; ///< BPMN-S DSGVO annotation (null if not annotated)
};

/**
 * @brief Metadata for process edges (flows)
 */
struct ProcessEdgeInfo {
    std::string edge_id;
    ProcessEdgeType edge_type;
    
    // Source and target
    std::string from_node;
    std::string to_node;
    
    // ===== Multi-Model Aspects =====
    MultiModelAspects model_aspects;
    
    // Conditional flow
    std::optional<std::string> condition_expression;  // e.g., "${amount > 1000}"
    bool is_default = false;    // Default flow from gateway
    
    // Priority for evaluation order
    int priority = 0;
    
    // Message flow properties
    std::optional<std::string> message_name;
    std::optional<std::string> message_payload_schema;
    
    // Timing constraints
    std::optional<std::chrono::milliseconds> min_delay;
    std::optional<std::chrono::milliseconds> max_delay;
    
    // ===== Relational Data =====
    nlohmann::json transition_data;             ///< Data to pass between nodes
    
    // ===== Vector Data =====
    std::optional<float> similarity_threshold;  ///< Min similarity for semantic conditions
    
    // ===== Geo Data =====
    std::optional<std::string> route_geometry;  ///< Path geometry for field processes
    std::optional<double> estimated_distance_km;
};

// ============================================================================
// Hyperedge Support (for complex synchronization)
// ============================================================================

/**
 * @brief Hyperedge: Connects multiple nodes simultaneously
 * 
 * Used for:
 * - AND-joins (wait for all incoming paths)
 * - AND-splits (activate all outgoing paths)
 * - Complex synchronization patterns
 * 
 * Unlike regular edges (1:1), hyperedges are (N:M):
 * - Multiple source nodes
 * - Multiple target nodes
 * - Single synchronization point
 */
struct Hyperedge {
    std::string hyperedge_id;
    std::string name;
    
    // Multiple sources and targets
    std::vector<std::string> source_nodes;
    std::vector<std::string> target_nodes;
    
    // Synchronization type
    enum class SyncType {
        AND_JOIN,       // Wait for all sources
        AND_SPLIT,      // Activate all targets
        OR_JOIN,        // Wait for any source
        N_OF_M_JOIN,    // Wait for N of M sources
        DISCRIMINATOR   // Wait for first, ignore rest
    };
    SyncType sync_type = SyncType::AND_JOIN;
    
    // For N_OF_M_JOIN
    std::optional<int> required_count;
    
    // Execution state
    std::unordered_set<std::string> activated_sources;
    bool is_complete = false;
};

// ============================================================================
// Process Execution State (Runtime)
// ============================================================================

/**
 * @brief ProcessGraphVisitLog: Maps a node ID to its most recent visit timestamp.
 *
 * Used to record when a process token visited each node during traversal,
 * enabling temporal ordering of multi-hop graph paths.
 */
using ProcessGraphVisitLog = std::unordered_map<std::string, std::chrono::system_clock::time_point>;

/**
 * @brief Token: Represents execution state at a node
 * 
 * Based on Petri Net semantics used in BPMN execution.
 */
struct ProcessToken {
    std::string token_id;
    std::string process_instance_id;
    std::string current_node;
    
    // State
    enum class State {
        WAITING,        // At gateway, waiting for join
        READY,          // Ready to execute
        ACTIVE,         // Currently executing
        COMPLETED,      // Execution finished
        FAILED,         // Execution failed
        CANCELLED       // Manually cancelled
    };
    State state = State::READY;
    
    // Timing
    int64_t created_at_ms;
    std::optional<int64_t> started_at_ms;
    std::optional<int64_t> completed_at_ms;
    
    // Data context (variables in scope)
    nlohmann::json variables;
    
    // History (for audit/replay)
    std::vector<std::string> visited_nodes;
    std::vector<std::string> traversed_edges;

    // Per-node visit timestamps: records when each node was last visited
    ProcessGraphVisitLog visit_timestamps;
};

/**
 * @brief Process Instance: A running process
 */
struct ProcessInstance {
    std::string instance_id;
    std::string process_definition_id;  // The process model
    std::string name;
    
    // State
    enum class State {
        CREATED,
        RUNNING,
        SUSPENDED,
        COMPLETED,
        TERMINATED,
        FAILED
    };
    State state = State::CREATED;
    
    // Active tokens
    std::vector<ProcessToken> tokens;
    
    // Variables
    nlohmann::json variables;
    
    // Timing
    int64_t started_at_ms = 0;
    std::optional<int64_t> completed_at_ms;
    
    // Parent instance (for subprocesses)
    std::optional<std::string> parent_instance_id;
    std::optional<std::string> parent_node_id;  // Call activity that spawned this
};

// ============================================================================
// Process Graph Manager
// ============================================================================

class RocksDBWrapper;

/**
 * @brief ProcessGraphManager: Manages process models and execution
 * 
 * Features:
 * - BPMN/EPK node and edge management
 * - Hyperedge support for complex joins
 * - Process instance execution
 * - Token-based state tracking
 * - Temporal queries on process history
 */
class ProcessGraphManager {
public:
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    explicit ProcessGraphManager(RocksDBWrapper& db);

    /**
     * @brief Wire a text-embedding function for auto-generating process embeddings.
     *
     * When set, registerProcess() automatically computes and persists an embedding
     * for the process name (and BPMN description if available) so that
     * findSimilarProcesses() / semanticSearchProcesses() work without manual
     * pre-computation.
     *
     * @param embedder  `(std::string_view text) → std::vector<float>`.
     *                  Pass an empty function to disable.
     */
    void setEmbedder(std::function<std::vector<float>(std::string_view)> embedder);

    // ===== Process Model Management =====
    
    /**
     * @brief Register a process definition
     */
    Status registerProcess(std::string_view process_id, std::string_view name, 
                           std::string_view bpmn_xml = "");
    
    /**
     * @brief Add a node to a process model
     */
    Status addProcessNode(std::string_view process_id, const ProcessNodeInfo& node);
    
    /**
     * @brief Add an edge (flow) to a process model
     */
    Status addProcessEdge(std::string_view process_id, const ProcessEdgeInfo& edge);
    
    /**
     * @brief Add a hyperedge (for AND-joins, etc.)
     */
    Status addHyperedge(std::string_view process_id, const Hyperedge& hyperedge);
    
    /**
     * @brief Validate process model (check for deadlocks, unreachable nodes, etc.)
     */
    struct ValidationResult {
        bool is_valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    std::pair<Status, ValidationResult> validateProcess(std::string_view process_id) const;

    // ===== Process Execution =====
    
    /**
     * @brief Start a new process instance
     */
    std::pair<Status, std::string> startProcess(
        std::string_view process_id,
        const nlohmann::json& initial_variables = {}
    );
    
    /**
     * @brief Get current state of a process instance
     */
    std::pair<Status, ProcessInstance> getProcessInstance(std::string_view instance_id) const;
    
    /**
     * @brief Advance a token to the next node(s)
     */
    Status advanceToken(std::string_view instance_id, std::string_view token_id);
    
    /**
     * @brief Signal an event (for message/signal catching)
     */
    Status signalEvent(std::string_view instance_id, std::string_view event_name,
                       const nlohmann::json& payload = {});
    
    /**
     * @brief Complete a user task
     */
    Status completeTask(std::string_view instance_id, std::string_view task_node,
                        const nlohmann::json& output_variables = {});
    
    /**
     * @brief Suspend/resume a process instance
     */
    Status suspendProcess(std::string_view instance_id);
    Status resumeProcess(std::string_view instance_id);
    
    /**
     * @brief Terminate a process instance
     */
    Status terminateProcess(std::string_view instance_id, std::string_view reason = "");

    // ===== Process Queries =====
    
    /**
     * @brief Resolve a token-only task_id to its (instance_id, current_node) pair.
     *
     * Scans all stored tokens and returns the instance_id and current_node for
     * the first READY or ACTIVE token whose token_id matches @p token_id.
     * Used by WireProtocolServer to accept task_ids without the
     * "instance_id:node_id" colon format.  Stub #138 resolution.
     *
     * @param token_id  The token identifier (without instance prefix).
     * @return A pair {instance_id, current_node} if a matching active token is
     *         found; std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<std::pair<std::string, std::string>>
    findTokenByTokenId(std::string_view token_id) const;

    /**
     * @brief Find all active tasks for a user/role
     */
    std::pair<Status, std::vector<ProcessToken>> findActiveTasks(
        std::string_view assignee_or_role
    ) const;
    
    /**
     * @brief Get the visit timestamp for a node within a process instance.
     *
     * Returns the most recent time_point at which any token in the given
     * instance visited @p node_id, or std::nullopt if the node has not
     * been visited or the instance does not exist.
     *
     * When multiple tokens have visited the same node (e.g. in parallel
     * gateway scenarios), the most recent timestamp across all tokens is
     * returned.
     */
    std::optional<std::chrono::system_clock::time_point> getVisitTimestamp(
        std::string_view instance_id,
        std::string_view node_id
    ) const;

    /**
     * @brief Get process history (all tokens that passed through a node)
     */
    std::pair<Status, std::vector<ProcessToken>> getNodeHistory(
        std::string_view process_id,
        std::string_view node_id,
        std::optional<int64_t> since_ms = std::nullopt
    ) const;
    
    /**
     * @brief Find bottlenecks (nodes with longest average duration)
     */
    struct NodeMetrics {
        std::string node_id;
        size_t execution_count;
        double avg_duration_ms;
        double max_duration_ms;
        size_t failure_count;
    };
    std::pair<Status, std::vector<NodeMetrics>> getProcessMetrics(
        std::string_view process_id
    ) const;
    
    /**
     * @brief Critical path analysis
     */
    std::pair<Status, std::vector<std::string>> findCriticalPath(
        std::string_view process_id
    ) const;

    // ===== Hyperedge Queries =====
    
    /**
     * @brief Get hyperedge status (how many sources have been activated)
     */
    std::pair<Status, Hyperedge> getHyperedgeStatus(
        std::string_view hyperedge_id
    ) const;
    
    /**
     * @brief Check if hyperedge join condition is satisfied
     */
    std::pair<Status, bool> isHyperedgeReady(std::string_view hyperedge_id) const;

    // =========================================================================
    // Multi-Model Queries
    // =========================================================================

    /**
     * @brief AQL query executor injection function type.
     *
     * When set via setAqlQueryExecutor(), the multi-model query methods
     * (queryTasksByFormData, joinWithCollection, aggregateByField) delegate to
     * this function instead of running O(n) in-process RocksDB scans.
     *
     * @param aql        AQL query string with bind variable placeholders.
     * @param bind_vars  Bind variable values keyed by name (without leading "@").
     * @return           Result rows as JSON objects (one per document).
     */
    using AqlQueryExecutorFn =
        std::function<std::vector<nlohmann::json>(std::string_view aql,
                                                  const nlohmann::json& bind_vars)>;

    /**
     * @brief Inject an AQL query executor for index-backed multi-model queries.
     *
     * When a non-null executor is provided the three multi-model query methods
     * will build an AQL statement, invoke the executor, and map the results back
     * to their typed return values.  The in-process O(n) scan fallbacks are
     * retained and used when no executor is set.
     *
     * Thread safety: call before any concurrent query; the function object is
     * read under a shared lock from query methods.
     */
    void setAqlQueryExecutor(AqlQueryExecutorFn fn);

    // ----- RELATIONAL Queries -----
    
    /**
     * @brief Query tasks by form field values (SQL-like)
     * 
     * Example: Find all tasks where order.amount > 1000
     */
    std::pair<Status, std::vector<ProcessToken>> queryTasksByFormData(
        std::string_view process_id,
        const nlohmann::json& filter_conditions
    ) const;
    
    /**
     * @brief Join process data with external entity collection
     * 
     * Example: Get tasks with customer details joined from customers collection
     */
    struct JoinResult {
        ProcessToken token;
        nlohmann::json joined_data;
    };
    std::pair<Status, std::vector<JoinResult>> joinWithCollection(
        std::string_view process_id,
        std::string_view collection_name,
        std::string_view local_field,
        std::string_view foreign_field
    ) const;
    
    /**
     * @brief Aggregate process metrics by grouping field
     * 
     * Example: COUNT tasks GROUP BY assignee
     */
    struct AggregateResult {
        nlohmann::json group_key;
        size_t count;
        double sum;
        double avg;
        double min;
        double max;
    };
    std::pair<Status, std::vector<AggregateResult>> aggregateByField(
        std::string_view process_id,
        std::string_view group_field,
        std::string_view agg_field,
        std::string_view agg_function  // COUNT, SUM, AVG, MIN, MAX
    ) const;
    
    // ----- VECTOR Queries -----
    
    /**
     * @brief Find similar processes by description embedding
     * 
     * Uses vector similarity to find processes with similar purpose.
     */
    struct SimilarProcess {
        std::string process_id;
        std::string name;
        float similarity;
    };
    std::pair<Status, std::vector<SimilarProcess>> findSimilarProcesses(
        const std::vector<float>& query_embedding,
        size_t k = 10,
        float min_similarity = 0.7f
    ) const;
    
    /**
     * @brief Find similar tasks for intelligent routing
     * 
     * Find tasks that are semantically similar to route to same handlers.
     */
    std::pair<Status, std::vector<ProcessToken>> findSimilarTasks(
        std::string_view instance_id,
        std::string_view task_node,
        size_t k = 5
    ) const;
    
    /**
     * @brief Semantic search in process descriptions
     * 
     * Natural language search across all process definitions.
     */
    std::pair<Status, std::vector<SimilarProcess>> semanticSearchProcesses(
        std::string_view natural_language_query,
        size_t k = 10
    ) const;
    
    /**
     * @brief Detect anomalies in process execution using embeddings
     * 
     * Compare current execution pattern to historical patterns.
     */
    struct AnomalyResult {
        std::string instance_id;
        std::string anomaly_type;  // "path_deviation", "duration_outlier", "data_anomaly"
        float anomaly_score;
        std::string description;
    };
    std::pair<Status, std::vector<AnomalyResult>> detectAnomalies(
        std::string_view process_id,
        float threshold = 0.8f
    ) const;
    
    // ----- GEO Queries -----
    
    /**
     * @brief Find tasks within geographic area
     * 
     * Example: Find all field tasks within 10km of a location.
     */
    std::pair<Status, std::vector<ProcessToken>> findTasksInArea(
        std::string_view process_id,
        double center_lon,
        double center_lat,
        double radius_km
    ) const;
    
    /**
     * @brief Find tasks within a geofence polygon
     */
    std::pair<Status, std::vector<ProcessToken>> findTasksInGeofence(
        std::string_view process_id,
        std::string_view geofence_wkt  // WKT polygon
    ) const;
    
    /**
     * @brief Get optimal route for field process tasks
     * 
     * Returns ordered list of tasks optimized for travel distance.
     */
    struct RouteStop {
        ProcessToken token;
        double distance_from_prev_km;
        double estimated_travel_time_min;
    };
    std::pair<Status, std::vector<RouteStop>> optimizeTaskRoute(
        const std::vector<std::string>& task_ids,
        double start_lon,
        double start_lat
    ) const;
    
    /**
     * @brief Check if task location satisfies geo-constraints
     */
    std::pair<Status, bool> validateLocationConstraint(
        std::string_view instance_id,
        std::string_view task_node,
        double execution_lon,
        double execution_lat
    ) const;
    
    /**
     * @brief Get region-specific process parameters
     * 
     * Returns process variable overrides based on geographic region.
     */
    std::pair<Status, nlohmann::json> getRegionalParameters(
        std::string_view process_id,
        double lon,
        double lat
    ) const;
    
    // ----- Cross-Model Queries -----
    
    /**
     * @brief Combined multi-model query
     * 
     * Execute a query that spans all model aspects:
     * - Graph: Traversal constraints
     * - Relational: Filter conditions
     * - Vector: Similarity constraints
     * - Geo: Spatial constraints
     */
    struct MultiModelQuery {
        // Graph constraints
        std::optional<std::string> start_node;
        std::optional<int> max_depth;
        std::vector<std::string> edge_types;
        
        // Relational constraints
        nlohmann::json filter_conditions;
        std::vector<std::string> select_fields;
        
        // Vector constraints
        std::optional<std::vector<float>> similarity_vector;
        std::optional<float> min_similarity;
        
        // Geo constraints
        std::optional<std::string> within_geofence;
        std::optional<double> max_distance_km;
        std::optional<std::pair<double, double>> from_location;
    };
    
    struct MultiModelResult {
        ProcessToken token;
        nlohmann::json selected_fields;
        std::optional<float> similarity_score;
        std::optional<double> distance_km;
    };
    
    std::pair<Status, std::vector<MultiModelResult>> executeMultiModelQuery(
        std::string_view process_id,
        const MultiModelQuery& query
    ) const;

private:
    RocksDBWrapper& db_;
    std::function<std::vector<float>(std::string_view)> embedder_;
    AqlQueryExecutorFn aql_query_executor_;
    
    // Internal helpers
    std::string makeProcessKey_(std::string_view process_id) const;
    std::string makeNodeKey_(std::string_view process_id, std::string_view node_id) const;
    std::string makeEdgeKey_(std::string_view process_id, std::string_view edge_id) const;
    std::string makeHyperedgeKey_(std::string_view process_id, std::string_view hyperedge_id) const;
    std::string makeInstanceKey_(std::string_view instance_id) const;
    std::string makeTokenKey_(std::string_view instance_id, std::string_view token_id) const;
    
    // Token management
    std::string generateTokenId_() const;
    Status createToken_(ProcessInstance& instance, std::string_view node_id);
    Status moveToken_(ProcessInstance& instance, ProcessToken& token, std::string_view target_node);
    
    // Gateway logic
    std::vector<std::string> evaluateGateway_(
        const ProcessNodeInfo& gateway,
        const ProcessToken& token,
        const std::vector<ProcessEdgeInfo>& outgoing_edges
    ) const;
    
    // Hyperedge logic
    bool checkHyperedgeCondition_(const Hyperedge& hyperedge) const;
    Status activateHyperedgeSource_(Hyperedge& hyperedge, std::string_view source_node);
};

// ============================================================================
// Built-in Process Edge Types for EdgeTypeRegistry
// ============================================================================

/**
 * @brief Register BPMN/EPK edge types with the EdgeTypeRegistry
 * 
 * Call this at application startup to enable process-aware edge validation.
 */
void registerProcessEdgeTypes();

} // namespace themis

