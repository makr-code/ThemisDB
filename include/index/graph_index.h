/**
 * @file graph_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include "index/temporal_graph.h"
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <memory>

namespace themis {

#include "themis/export.h"

class BaseEntity;
class IExpressionEvaluator;

namespace utils {
    class AuditLogger;
}

/// GraphIndexManager
/// - Verwaltet Adjazenz-Indizes für gerichtete Kanten
/// - Key-Schema:
///   - Out: graph:out:<from_pk>:<edge_id>  -> value: <to_pk>
///   - In:  graph:in:<to_pk>:<edge_id>    -> value: <from_pk>
/// - Atomare Operationen via WriteBatch
/// - In-Memory Topologie für O(1) Nachbarschaftsabfragen
/// - Saubere Fehler über Status-Rückgabe, kein Exception-API nach außen
/// - Optional: Audit Logging für Graph-Operationen (Phase 1 Knowledge Graph Protection)
///
/// Note: Windows DLL exports are handled centrally by the module target via
/// CMake's WINDOWS_EXPORT_ALL_SYMBOLS. Keeping the explicit class-level export
/// here creates inconsistent dllimport/dllexport bindings across translation
/// units and triggers C4273 in mixed build/import scenarios.
class GraphIndexManager {
public:
    struct AdjacencyInfo {
        std::string edgeId;
        std::string targetPk;
        std::string graphId; // multi-graph identifier
    };

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    explicit GraphIndexManager(RocksDBWrapper& db);
    
    // Set optional audit logger for tracking graph operations (Phase 1)
    void setAuditLogger(std::shared_ptr<utils::AuditLogger> logger, std::string user_context = "system");
    
    // Set user context for audit logging
    void setUserContext(std::string user_id);
    
    // Phase 4: Set optional expression evaluator for advanced filtering
    void setExpressionEvaluator(std::shared_ptr<IExpressionEvaluator> evaluator);
    
    // Get expression evaluator
    std::shared_ptr<IExpressionEvaluator> getExpressionEvaluator() const;


    // Topologie aus RocksDB laden (optional beim Start)
    Status rebuildTopology();

    /// Edge-Operationen (Edge-Entity benötigt Felder: id, _from, _to)
    /// 
    /// @brief Fügt eine Kante zum Graphen hinzu (atomare Operation über WriteBatch).
    /// 
    /// @param edge BaseEntity mit erforderlichen Feldern:
    ///   - id: Eindeutige Kanten-ID (non-empty)
    ///   - _from: Quell-Knoten-ID (non-empty, fail-closed QW-45 Guard)
    ///   - _to: Ziel-Knoten-ID (non-empty, fail-closed QW-45 Guard)
    /// 
    /// @return Status::OK() bei erfolgreicher Einfügung, Status::Error() bei Validierungsfehlern
    ///         (fehlende Felder, leere Node-IDs).
    /// 
    /// @note **QW-45 Fail-Closed Guard:** Empty or missing _from/_to node IDs are rejected
    ///       before persistence to prevent graph topology corruption. The guards ensure that
    ///       every edge has valid, non-empty source and target node references. Any validation
    ///       failure returns Status::Error() immediately (fail-closed behavior).
    Status addEdge(const BaseEntity& edge);
    Status deleteEdge(std::string_view edgeId);

    // Varianten für Transaktionen: nutzen bestehende WriteBatch
    /// WriteBatch variant for atomic multi-edge operations.
    /// Applies same QW-45 fail-closed guards as the main addEdge method.
    Status addEdge(const BaseEntity& edge, RocksDBWrapper::WriteBatchWrapper& batch);
    Status deleteEdge(std::string_view edgeId, RocksDBWrapper::WriteBatchWrapper& batch);

    // MVCC Transaction Varianten
    /// Transaction variant for MVCC isolation.
    /// Applies same QW-45 fail-closed guards as the main addEdge method.
    Status addEdge(const BaseEntity& edge, RocksDBWrapper::TransactionWrapper& txn);
    Status deleteEdge(std::string_view edgeId, RocksDBWrapper::TransactionWrapper& txn);

    /// Create a write batch for atomic multi-edge mutations (e.g. scheduled refresh).
    std::unique_ptr<RocksDBWrapper::WriteBatchWrapper> createWriteBatch();

    // Nachbarschaftsabfragen (nutzt In-Memory falls verfügbar, sonst RocksDB)
    std::pair<Status, std::vector<std::string>> outNeighbors(std::string_view fromPk) const;
    std::pair<Status, std::vector<std::string>> inNeighbors(std::string_view toPk) const;

    // Nachbarschaft mit Kanten-IDs (für RETURN e/p)
    std::pair<Status, std::vector<AdjacencyInfo>> outAdjacency(std::string_view fromPk) const;
    std::pair<Status, std::vector<AdjacencyInfo>> inAdjacency(std::string_view toPk) const;

    // Shortest-Path-Algorithmen (gewichtete Graphen)
    // Weight wird aus Edge-Entity-Feld "_weight" gelesen (default: 1.0)
    struct PathResult {
        std::vector<std::string> path;  // Knoten vom Start zum Ziel
        double totalCost = 0.0;
    };
    
    // Path Constraints for advanced traversal control
    struct PathConstraints {
        bool unique_vertices = false;      // No vertex visited twice (cycle detection)
        bool unique_edges = false;         // No edge traversed twice
        std::unordered_set<std::string> forbidden_vertices;  // Blacklist vertices
        std::unordered_set<std::string> forbidden_edges;     // Blacklist edges
        std::unordered_set<std::string> required_vertices;   // Must-visit vertices
        int max_edge_count = -1;           // Limit edges per path (-1 = unlimited)
        int min_edge_count = 0;            // Minimum edges per path
    };

    // Sprint B: Temporal Graph Extensions
    // Traversal with temporal filtering (edges must be valid at specified timestamp)
    std::pair<Status, std::vector<std::string>> bfsAtTime(
        std::string_view startPk,
        int64_t timestamp_ms,
        int maxDepth = 3
    ) const;
    
    std::pair<Status, PathResult> dijkstraAtTime(
        std::string_view startPk,
        std::string_view targetPk,
        int64_t timestamp_ms
    ) const;

    // Sprint B Extended: Time-Range Queries
    // Find all edges valid during a time window (any overlap)
    struct EdgeInfo {
        std::string edgeId;
        std::string fromPk;
        std::string toPk;
        std::optional<int64_t> valid_from;
        std::optional<int64_t> valid_to;
    };
    
    std::pair<Status, std::vector<EdgeInfo>> getEdgesInTimeRange(
        int64_t range_start_ms,
        int64_t range_end_ms,
        bool require_full_containment = false
    ) const;
    
    // Find edges from specific node valid in time range
    std::pair<Status, std::vector<EdgeInfo>> getOutEdgesInTimeRange(
        std::string_view fromPk,
        int64_t range_start_ms,
        int64_t range_end_ms,
        bool require_full_containment = false
    ) const;

    // ===== Temporal Aggregations =====
    enum class Aggregation { COUNT, SUM, AVG, MIN, MAX };

    struct TemporalAggregationResult {
        size_t count = 0;
        double value = 0.0; // SUM for SUM, AVG for AVG, MIN/MAX as appropriate
    };

    /// Aggregate a numeric edge property across edges matching the time range.
    /// - property: the numeric field name on the Edge entity (e.g. "_weight" or "cost")
    /// - agg: aggregation type (COUNT, SUM, AVG, MIN, MAX)
    /// - range_start_ms / range_end_ms: time window
    /// - require_full_containment: if true only include fully contained edges
    /// - optional edge_type: if provided, only consider edges with matching _type
    std::pair<Status, TemporalAggregationResult> aggregateEdgePropertyInTimeRange(
        std::string_view property,
        Aggregation agg,
        int64_t range_start_ms,
        int64_t range_end_ms,
        bool require_full_containment = false,
        std::optional<std::string_view> edge_type = std::nullopt
    ) const;

    // Temporal aggregations over time ranges
    std::pair<Status, TemporalStats> getTemporalStats(
        int64_t range_start_ms,
        int64_t range_end_ms,
        bool require_full_containment = false
    ) const;

    // Traversierungen
    std::pair<Status, std::vector<std::string>> bfs(
        std::string_view startPk,
        int maxDepth = 3
    ) const;

    // BFS with edge type filtering and graph scope (server-side)
    std::pair<Status, std::vector<std::string>> bfs(
        std::string_view startPk,
        int maxDepth,
        std::string_view edge_type,
        std::string_view graph_id
    ) const;

    // Dijkstra: Kürzester Pfad von start zu target
    std::pair<Status, PathResult> dijkstra(
        std::string_view startPk,
        std::string_view targetPk
    ) const;

    // Dijkstra with edge type filtering and graph scope (server-side)
    std::pair<Status, PathResult> dijkstra(
        std::string_view startPk,
        std::string_view targetPk,
        std::string_view edge_type,
        std::string_view graph_id
    ) const;
    
    // BFS with path constraints
    std::pair<Status, std::vector<std::string>> bfsWithConstraints(
        std::string_view startPk,
        int maxDepth,
        const PathConstraints& constraints,
        std::string_view edge_type = "",
        std::string_view graph_id = ""
    ) const;
    
    // Dijkstra with path constraints
    std::pair<Status, PathResult> dijkstraWithConstraints(
        std::string_view startPk,
        std::string_view targetPk,
        const PathConstraints& constraints,
        std::string_view edge_type = "",
        std::string_view graph_id = ""
    ) const;

    // A*: Kürzester Pfad mit Heuristik (optional)
    // heuristic: Funktion die Schätzkosten von einem Knoten zum Ziel liefert
    using HeuristicFunc = std::function<double(const std::string& node)>;
    std::pair<Status, PathResult> aStar(
        std::string_view startPk,
        std::string_view targetPk,
        HeuristicFunc heuristic = nullptr
    ) const;

    // Statistiken
    size_t getTopologyNodeCount() const;
    size_t getTopologyEdgeCount() const;

    /// Return all vertex IDs present in the in-memory topology (sources and
    /// targets of all edges added via addEdge). When the topology has not been
    /// loaded, returns an empty vector. Thread-safe.
    std::vector<std::string> getAllVertices() const;

    // Edge attribute retrieval (for weighted graph algorithms)
    // Returns edge weight from specified attribute (default: "_weight")
    // Falls back to 1.0 if attribute doesn't exist or edge not found
    double getEdgeWeight(std::string_view graphId, std::string_view edgeId, 
                        std::string_view weightAttribute = "_weight") const;

    // General string field accessor for edge entities.
    // Tries key formats: "edge:<graphId>:<edgeId>" and "edge:<edgeId>".
    // Returns nullopt if the edge or field does not exist.
    std::optional<std::string> getEdgeField(std::string_view edgeId,
                                            std::string_view fieldName) const;

    // General string field accessor for vertex (node) entities.
    // Uses key format: "node:<vertexId>" (KeySchema::makeGraphNodeKey).
    // Returns nullopt if the vertex or field does not exist.
    std::optional<std::string> getNodeField(std::string_view vertexId,
                                            std::string_view fieldName) const;

    // Optional: provide FieldEncryption for encrypting sensitive edge fields
    void setFieldEncryption(std::shared_ptr<class FieldEncryption> fe) { field_encryption_ = fe; }

    // Returns a deduplicated list of all vertex IDs present in the in-memory
    // topology (union of source vertices in outEdges_ and target vertices in
    // inEdges_).  Returns an empty vector when the topology has not been
    // populated yet.
    std::pair<Status, std::vector<std::string>> allVertices() const;

private:
    RocksDBWrapper& db_;

    // ========================================================================
    // Thread Safety: Lock Hierarchy (Phase 3 A-5 Circular Lock Ordering)
    // ========================================================================
    // 
    // LOCK HIERARCHY (prevents deadlocks via consistent acquisition order):
    //   Tier 1 (Global):    topology_mutex_      ← Acquire FIRST
    //   Tier 2 (Partition): [reserved for future partition locks]
    //   Tier 3 (Element):   [reserved for future element-level locks]
    // 
    // INVARIANT: All code paths must acquire locks in order Tier 1 → Tier 2 → Tier 3.
    //            Violating this order creates deadlock risk. ThreadSanitizer detects violations.
    //            See: https://github.com/google/sanitizers/wiki/ThreadSanitizerDeadlockDetector
    //
    // In-Memory Adjazenzlisten (thread-safe via topology_mutex_)
    mutable std::shared_mutex topology_mutex_;  // Tier 1: Global topology lock
    std::unordered_map<std::string, std::vector<AdjacencyInfo>> outEdges_; // fromPk -> [(edgeId, toPk)]
    std::unordered_map<std::string, std::vector<AdjacencyInfo>> inEdges_;  // toPk -> [(edgeId, fromPk)]
    std::atomic<bool> topologyLoaded_{false};

    // Hilfsfunktionen
    void addEdgeToTopology_(const std::string& edgeId, const std::string& from, const std::string& to, const std::string& graphId = "");
    void removeEdgeFromTopology_(const std::string& edgeId, const std::string& from, const std::string& to, const std::string& graphId = "");
    void addEdgeToTopologyUnlocked_(const std::string& edgeId, const std::string& from, const std::string& to, const std::string& graphId = "");
    void removeEdgeFromTopologyUnlocked_(const std::string& edgeId, const std::string& from, const std::string& to, const std::string& graphId = "");
    
    // Edge-Weight-Parsing (liest _weight aus Edge-Entity, default 1.0)
    double getEdgeWeight_(std::string_view graphId, std::string_view edgeId) const;

    // Edge-Type-Parsing (liest _type aus Edge-Entity, empty wenn nicht gesetzt)
    std::string getEdgeType_(std::string_view graphId, std::string_view edgeId) const;

    // Parse keys: graph:out:<graph_id>:<fromPk>:<edgeId>
    static bool parseOutKey_(std::string_view key, std::string& graphId, std::string& fromPk, std::string& edgeId);
    // Parse keys: graph:in:<graph_id>:<toPk>:<edgeId>
    static bool parseInKey_(std::string_view key, std::string& graphId, std::string& toPk, std::string& edgeId);
    
    static std::vector<uint8_t> toBytes(std::string_view sv);

    // Optional FieldEncryption instance (not owned)
    std::shared_ptr<class FieldEncryption> field_encryption_;
    
    // Phase 1: Optional AuditLogger for knowledge graph protection
    std::shared_ptr<utils::AuditLogger> audit_logger_;
    std::string user_context_ = "system";  // Default user context
    
    // Phase 4: Optional ExpressionEvaluator for advanced filtering
    std::shared_ptr<IExpressionEvaluator> expression_evaluator_;
    
    // Helper: Log audit event if logger is set
    void logAuditEvent_(const std::string& event_type, const std::string& resource, 
                       const std::string& operation, size_t count = 0, int depth = 0) const;
};

} // namespace themis
