/**
 * @file query_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <functional>
#include <string_view>
#include <vector>
#include <optional>
#include <utility>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include "utils/expected.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "utils/expected.h"  // For Result<T> pattern

namespace themis {

class RocksDBWrapper;
class SecondaryIndexManager;
class BaseEntity;
class VectorIndexManager;
class GraphIndexManager;
class StatisticsCollector;
class AQLTranslator;
namespace utils {
class AuditLogger;
}
namespace index {
class SpatialIndexManager;
}

namespace query {

// Smart pointer type aliases for dependency injection
using IStorageEnginePtr = std::shared_ptr<IStorageEngine>;

// Forward declaration for StatisticsCollector (avoid including the header)
using StatisticsCollector = ::themis::StatisticsCollector;
using IIndexManagerPtr = std::shared_ptr<IIndexManager>;
// using IQueryEnginePtr = std::shared_ptr<IQueryEngine>;  // IQueryEngine not defined
using IExpressionEvaluatorPtr = std::shared_ptr<IExpressionEvaluator>;
using IVectorIndexPtr = std::shared_ptr<IVectorIndex>;
using ISecondaryIndexPtr = std::shared_ptr<ISecondaryIndex>;
using IGraphIndexPtr = std::shared_ptr<IGraphIndex>;

// Minimal forward declarations for early usage
struct Expression; struct Query; class CTECache; struct QueryPlanNode;

/**
 * @brief Input model for recursive graph path expansion queries.
 *
 * Describes start/end anchors, traversal depth, optional temporal validity
 * constraints, and optional spatial constraints for graph+geo hybrid queries.
 */
struct RecursivePathQuery {
    std::string start_node;
    std::string end_node;
    std::string edge_type;
    std::string graph_id; // multi-graph scope (optional, default="default")
    size_t max_depth = 5;
    std::optional<std::string> valid_from;
    std::optional<std::string> valid_to;
    // Optional: weitere Filter auf Knoten/Kanten
    
    // Spatial constraints for Graph+Geo hybrid queries
    struct SpatialConstraint {
        std::string vertex_geom_field = "location"; // field containing geometry in vertex
        std::shared_ptr<query::Expression> spatial_filter; // e.g., ST_Within(v.location, @region)
    };
    std::optional<SpatialConstraint> spatial_constraint;
};

// General Traversal Query Structures
/**
 * @brief Traversal direction for graph expansion operations.
 */
enum class TraversalDirection { OUTBOUND, INBOUND, ANY };

/**
 * @brief Result row for graph traversals.
 */
struct TraversalResult {
    std::string vertex_pk;
    int depth = 0;
    std::vector<std::string> path;   // Full path from start to this vertex
    std::vector<std::string> edges;  // Edge IDs traversed
    nlohmann::json vertex_data;      // Full vertex entity data
};

/**
 * @brief Query model for hybrid vector similarity plus spatial filtering.
 */
struct VectorGeoQuery {
    std::string table;
    std::string vector_field = "embedding";
    std::string geom_field = "location";
    std::vector<float> query_vector;
    size_t k = 10; // top-k results
    std::shared_ptr<query::Expression> spatial_filter; // e.g., ST_Within(location, @region)
    // Additional non-spatial predicates (equality / range) allowed now
    std::vector<std::shared_ptr<query::Expression>> extra_filters; // evaluated conjunctively
};

/**
 * @brief Query model for vector similarity search with attribute filters.
 *
 * This variant intentionally excludes geo predicates.
 */
struct FilteredVectorSearchQuery {
    std::string table;
    std::string vector_field = "embedding";
    std::vector<float> query_vector;
    size_t k = 10;
    
    // Attribute filters for pre-filtering via SecondaryIndex
    struct AttributeFilter {
        std::string field = {};
        // Vermeide Konflikt mit möglichem Windows Makro IN
        #ifdef IN
        #undef IN
        #endif
        enum class Op { 
            EQUALS,           // field == value
            NOT_EQUALS,       // field != value (post-filter only)
            CONTAINS,         // string contains (post-filter only)
            GREATER_THAN,     // field > value
            LESS_THAN,        // field < value
            GREATER_EQUAL,    // field >= value
            LESS_EQUAL,       // field <= value
            IN,               // field in [values]
            RANGE             // value_min <= field <= value_max
        } op = Op::EQUALS;
        
        std::string value;              // For EQUALS, GT, LT, GTE, LTE, NOT_EQUALS, CONTAINS
        std::vector<std::string> values; // For IN
        std::string value_min;          // For RANGE
        std::string value_max;          // For RANGE
    };
    
    std::vector<AttributeFilter> filters;
    
    // Strategy hint: "auto", "pre-filter", "post-filter"
    std::string strategy = "auto";
};

/**
 * @brief Query model for epsilon/radius vector neighbor search.
 */
struct RadiusVectorSearchQuery {
    std::string table;
    std::string vector_field = "embedding";
    std::vector<float> query_vector;
    float epsilon = 0.5f;  // Distance threshold
    size_t max_results = 0; // 0 = unlimited
    
    // Reuse AttributeFilter from FilteredVectorSearchQuery
    std::vector<FilteredVectorSearchQuery::AttributeFilter> filters;
    std::string strategy = "auto";
};

/**
 * @brief Full-text query model with optional metadata filters.
 */
struct ContentSearchQuery {
    std::string table;
    std::string fulltext_field = "content"; // Field containing text content
    std::string fulltext_query;             // Search query string
    size_t limit = 100;
    
    // Optional metadata filters (category, mime_type, author, etc.)
    struct MetadataFilter {
        std::string field;
        enum class Op { EQUALS, NOT_EQUALS, CONTAINS, IN } op = Op::EQUALS;
        std::string value = {};
        std::vector<std::string> values; // for IN
    };
    std::vector<MetadataFilter> metadata_filters;
    
    // Score threshold (BM25)
    double min_score = 0.0;
};

/**
 * @brief Full-text query model combined with optional spatial filtering.
 */
struct ContentGeoQuery {
    std::string table;
    std::string text_field;
    std::string fulltext_query;
    std::string geom_field = "location";
    std::shared_ptr<query::Expression> spatial_filter; // e.g., ST_DWithin(location, @center, 5000)
    size_t limit = 100;
    bool boost_by_distance = false; // if true, re-rank by spatial proximity
    std::optional<std::vector<float>> center_point; // for distance boosting: [lon, lat]
};

using RocksDBWrapper = ::themis::RocksDBWrapper;
using SecondaryIndexManager = ::themis::SecondaryIndexManager;
using BaseEntity = ::themis::BaseEntity;
using VectorIndexManager = ::themis::VectorIndexManager;
using SpatialIndexManager = ::themis::index::SpatialIndexManager;

using AQLTranslator = ::themis::AQLTranslator; // avoid including translator in header

// Forward declarations für AQL-Typen
struct ForNode;
struct FilterNode;
struct LetNode;
struct ReturnNode;
struct SortNode;
struct LimitNode;
struct CollectNode;
struct Expression;

struct PredicateEq {
    std::string column;
    std::string value; // bereits als String; Encoding für Indexkeys übernimmt SecondaryIndexManager
};

struct PredicateRange {
    std::string column;
    std::optional<std::string> lower; // gte
    std::optional<std::string> upper; // lte
    bool includeLower = true;
    bool includeUpper = true;
};

struct PredicateFulltext {
    std::string column;
    std::string query;
    size_t limit = 1000;
};

// Phrase Search Predicate for exact phrase matching
struct PredicatePhrase {
    std::string column;
    std::string phrase;
    size_t limit = 1000;
};

// Fuzzy Search Predicate for approximate matching with Levenshtein distance
struct PredicateFuzzy {
    std::string column = {};
    std::string query = {};
    int maxDistance = 2;  // Maximum edit distance
    size_t limit = 1000;
};

// Spatial Predicate for Geo queries (G3 - AQL Parser Integration)
struct PredicateSpatial {
    enum class Operation {
        Intersects,    // ST_Intersects(geometry_column, query_geometry)
        Within,        // ST_Within(geometry_column, query_geometry)
        Contains,      // ST_Contains(geometry_column, query_geometry)
        DWithin        // ST_DWithin(geometry_column, center_point, distance)
    };
    
    std::string column;                    // geometry column name (e.g., "location", "geometry")
    Operation operation;                   // spatial operation type
    std::shared_ptr<query::Expression> query_geom; // query geometry expression (parsed at runtime)
    
    // For DWithin queries
    std::optional<double> distance;        // distance in meters
    
    // Computed MBR for index filtering (set by translator)
    std::optional<std::pair<double, double>> bbox_min; // (minx, miny)
    std::optional<std::pair<double, double>> bbox_max; // (maxx, maxy)
};

struct OrderBy {
    std::string column;
    bool desc = false;
    size_t limit = 1000;
    // Optional Cursor-Anker für effiziente Paginierung über Range-Indizes
    // Wenn gesetzt, startet der Scan strikt NACH dem Tupel (cursor_value, cursor_pk)
    // bei asc (desc=false) bzw. strikt VOR dem Tupel bei desc=true.
    std::optional<std::string> cursor_value; // Wert der Sortierspalte des letzten Elements
    std::optional<std::string> cursor_pk;    // PK des letzten Elements (Tiebreaker)
};

struct ConjunctiveQuery {
    std::string table;
    std::vector<PredicateEq> predicates; // alle mit AND verknüpft
    std::vector<PredicateRange> rangePredicates; // zusätzliche AND-Range-Prädikate
    std::optional<OrderBy> orderBy; // optionales ORDER BY über Range-Index
    std::optional<PredicateFulltext> fulltextPredicate; // optional: FULLTEXT(column, query, limit)
    std::optional<PredicatePhrase> phrasePredicate; // optional: PHRASE(column, phrase, limit)
    std::optional<PredicateFuzzy> fuzzyPredicate; // optional: FUZZY(column, query, maxDistance, limit)
    std::optional<PredicateSpatial> spatialPredicate; // optional: ST_*(geometry_column, ...) (G3)

    // Direct primary-key lookup fast path.
    // When set, executeAndKeys / executeAndEntities skip all secondary-index
    // scans and perform a single direct storage read for the given primary key.
    // Other predicates are ignored when pk_eq is set — use this only when the
    // primary key uniquely identifies the desired entity.
    // This is ACID-compliant: the direct RocksDB read uses the same isolation
    // level as any other storage access in the engine.
    std::optional<std::string> pk_eq;
};

// Disjunctive Query: OR-verknüpfte AND-Blöcke (Disjunctive Normal Form)
// Beispiel: (city==Berlin AND age>18) OR (city==Munich AND age>21)
struct DisjunctiveQuery {
    std::string table;
    std::vector<ConjunctiveQuery> disjuncts; // OR-verknüpfte Conjunctions
    std::optional<OrderBy> orderBy;
};

using GraphIndexManager = ::themis::GraphIndexManager;

/** @brief Query engine component. */
class QueryEngine {
public:
    // DEPRECATED: Legacy Status struct - use Result<T> instead
    // Kept temporarily for backward compatibility during migration
    struct [[deprecated("Use Result<T> pattern instead")]] Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    // ========================================================================
    // LEGACY CONSTRUCTORS (Backward Compatibility)
    // ========================================================================
    QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx);
    QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx, GraphIndexManager& graphIdx);
    QueryEngine(RocksDBWrapper& db, SecondaryIndexManager& secIdx, GraphIndexManager& graphIdx,
                VectorIndexManager* vectorIdx, SpatialIndexManager* spatialIdx);
    
    // ========================================================================
    // NEW CONSTRUCTORS (Dependency Injection via Interfaces)
    // ========================================================================
    
    /**
     * @brief Constructor with Dependency Injection via interfaces
     * 
     * @param storage        Storage engine for data retrieval (can be nullptr for late binding)
     * @param index_manager  Index manager for optimization (required)
     * 
     * This constructor enables breaking circular dependencies by allowing
     * late binding of storage via setStorage().
     */
    QueryEngine(
        IStorageEnginePtr storage,
        IIndexManagerPtr index_manager
    );
    
    /**
     * @brief Static factory method creating QueryEngine with default implementations
     * 
     * Creates a QueryEngine with built-in RocksDBWrapper and SecondaryIndexManager.
     * This is a convenience method for backward compatibility.
     * 
     * @note NOT YET IMPLEMENTED - Will be available in Phase 4 when concrete
     *       implementations are adapted to interfaces. For now, use legacy constructors
     *       or QueryEngineBuilder with explicit dependencies.
     * 
     * @throws std::runtime_error Currently not implemented
     * @return Shared pointer to QueryEngine with default dependencies
     */
    static std::shared_ptr<QueryEngine> createDefault();
    
    /**
     * @brief Set storage engine (for late binding)
     * 
     * Called after QueryEngine is constructed to break circular initialization
     * dependencies. This enables the pattern:
     * 
     * 1. Create QueryEngine with nullptr storage
     * 2. Create StorageEngine that needs QueryEngine's expression evaluator
     * 3. Inject storage back into QueryEngine via setStorage()
     * 
     * @param storage Storage engine instance
     */
    void setStorage(IStorageEnginePtr storage);

    /**
     * @brief Inject a StatisticsCollector for cardinality-based query optimisation.
     *
     * When set, executeAndKeys() and executeAndKeysWithFallback() use the
     * per-column cardinality/selectivity data stored in the collector to order
     * equality predicates from most-to-least selective before execution.
     *
     * The pointer is non-owning; the caller manages the lifetime.
     * Pass nullptr to disable statistics-based optimisation.
     */
    void setStatisticsCollector(StatisticsCollector* sc) noexcept {
        std::lock_guard<std::mutex> lk(config_mutex_);
        stats_collector_ = sc;
    }

    /**
     * @brief Inject a collection-access checker (QE-2 fix).
     *
     * When set, every public `execute*` method checks whether the caller is
     * permitted to access the requested collection before executing any I/O.
     *
     * Signature: `bool checker(const std::string& collection,
     *                           const std::string& caller_id)`.
     * Return `true` to allow, `false` to deny.  A `nullptr` checker disables
     * the gate (permissive mode — only safe in single-tenant / trusted callers).
     *
     * The caller is responsible for injecting a real ACL implementation before
     * the engine is exposed to untrusted query paths.
     *
     * @param checker Callable that returns true iff access is allowed.
     * @param caller_id Opaque identity string forwarded to every checker call.
     */
    void setCollectionAccessChecker(
        std::function<bool(const std::string& collection,
                           const std::string& caller_id)> checker,
        std::string caller_id = "") noexcept
    {
        std::lock_guard<std::mutex> lk(config_mutex_);
        collection_access_checker_ = std::move(checker);
        collection_access_caller_id_ = std::move(caller_id);
    }

    /**
     * @brief Inject an optional non-owning audit sink for query phase telemetry.
     *
     * When set, key execution phases (parallel scans, entity loading, federation
     * dispatch) emit structured JSON audit events to the provided logger.
     * The pointer is non-owning; the caller manages the lifetime.
     * Pass nullptr to disable audit logging.
     *
     * Thread-safe: protected by the internal config_mutex_.
     *
     * @param al Pointer to AuditLogger instance, or nullptr to disable.
     */
    void setAuditLogger(::themis::utils::AuditLogger* al) noexcept {
        std::lock_guard<std::mutex> lk(config_mutex_);
        audit_logger_ = al;
    }

    /**
     * @brief Configure per-query execution timeout for parallel TBB task groups.
     *
     * When elapsed time after tg.wait() exceeds @p query_timeout a structured
     * timeout audit event is emitted and a warning is logged.  The timeout is
     * advisory — it does not interrupt already-running TBB tasks.
     *
     * Default: 30 000 ms for queries, 1 000 ms for lock acquisition.
     *
     * @param query_timeout  Maximum allowed wall-clock time for a full query.
     * @param lock_timeout   Maximum allowed time for a single lock attempt.
     */
    void setQueryTimeout(
        std::chrono::milliseconds query_timeout = std::chrono::seconds(30),
        std::chrono::milliseconds lock_timeout  = std::chrono::seconds(1)) noexcept
    {
        std::lock_guard<std::mutex> lk(config_mutex_);
        query_timeout_ms_ = query_timeout;
        lock_timeout_ms_  = lock_timeout;
    }

    /**
     * @brief Provide expression evaluator for Storage and Index to use
     * 
     * Returns an evaluator that wraps this QueryEngine's expression evaluation
     * logic. This breaks the circular dependency by providing a lightweight
     * interface that Storage and Index can use without depending on QueryEngine.
     * 
     * @return Expression evaluator instance
     */
    IExpressionEvaluatorPtr get_expression_evaluator();
    
    // Forward declaration for EvaluationContext
    struct EvaluationContext;
    
    // Expression evaluation (public for testing)
    bool evaluateCondition(
        const std::shared_ptr<query::Expression>& expr,
        const EvaluationContext& ctx
    ) const;
    
    // Rekursive Pfadabfrage (Multi-Hop Traversal)
    Result<std::vector<std::vector<std::string>>> executeRecursivePathQuery(const RecursivePathQuery& q) const;

    // General graph traversal (non-shortest path)
    // Performs BFS with depth filtering and direction support
    // Edge type filtering: pass edgeTypeFilter to restrict which edges are followed.
    /**
     * @brief Execute a general graph traversal query
     * @param startVertex Starting vertex primary key
     * @param minDepth Minimum traversal depth
     * @param maxDepth Maximum traversal depth (limits recursion)
     * @param direction Traversal direction (OUTBOUND, INBOUND, or ANY)
     * @param graphId Graph identifier (default: "default")
     * @param edgeTypeFilter Optional edge type filter; only edges whose graphId
     *        matches this value are followed. Empty string = no filtering.
     * @return Vector of traversal results containing visited vertices and paths
     * 
     * Performs breadth-first graph traversal starting from the given vertex.
     * Results include the full path and depth information for each reachable vertex.
     * When edgeTypeFilter is non-empty, only edges with a matching graphId are
     * traversed (same convention as RecursivePathQuery::edge_type).
     */
    Result<std::vector<TraversalResult>> executeGeneralTraversal(
        const std::string& startVertex,
        int minDepth,
        int maxDepth,
        TraversalDirection direction,
        const std::string& graphId = "default",
        const std::string& edgeTypeFilter = ""
    ) const;

    /**
     * @brief Execute conjunctive (AND) query and return full entities
     * @param q Conjunctive query with equality predicates
     * @return Vector of matching BaseEntity objects
     * 
     * FIND-016: Added Doxygen documentation for public API
     * Executes all equality predicates in parallel using secondary indexes,
     * then intersects the result sets to find matching primary keys.
     * Finally loads and returns the full entity data for each match.
     */
    Result<std::vector<BaseEntity>> executeAndEntities(const ConjunctiveQuery& q) const;
    
    /**
     * @brief Execute conjunctive (AND) query and return only primary keys
     * @param q Conjunctive query with equality predicates
     * @return Vector of matching primary keys
     * 
     * FIND-016: Added Doxygen documentation for public API
     * More efficient than executeAndEntities when only primary keys are needed.
     * Supports fulltext search, fuzzy search, spatial queries, and traditional predicates.
     */
    Result<std::vector<std::string>> executeAndKeys(const ConjunctiveQuery& q) const;

    /**
     * @brief Execute conjunctive (AND) query and return only match count
     * @param q Conjunctive query with equality/range/fulltext/spatial predicates
     * @return Number of matching primary keys
     *
     * Uses the same predicate planning and index-based execution as executeAndKeys,
     * but avoids entity materialization entirely. This is the preferred path for
     * COUNT-like workloads and join cardinality checks.
     *
     * Failure/edge cases:
     * - Returns an error when query validation/execution fails (e.g. invalid table,
     *   unsupported predicate combination, missing required index in strict path).
     * - Returns 0 for valid queries with no matches.
     */
    Result<size_t> executeAndCount(const ConjunctiveQuery& q) const;

    /**
     * @brief Variant of executeAndKeys with BM25 scoring support
     * @param q Conjunctive query with optional fulltext predicates
     * @return KeysWithScores containing primary keys and optional BM25 relevance scores
     * 
     * FIND-016: Added Doxygen documentation for public API
     * For fulltext queries, includes BM25 relevance scores in the result.
     * Useful for ranking search results by relevance.
     */
    // Variant with BM25 score support for FULLTEXT queries
    struct KeysWithScores {
        std::vector<std::string> keys;
        std::shared_ptr<std::unordered_map<std::string, double>> bm25_scores; // pk -> score
    };
    Result<KeysWithScores> executeAndKeysWithScores(const ConjunctiveQuery& q) const;

    /**
     * @brief Execute disjunctive (OR) query and return primary keys
     * @param q Disjunctive query (union of multiple conjunctive queries)
     * @return Vector of matching primary keys (deduplicated union)
     * 
     * FIND-016: Added Doxygen documentation for public API
     * Executes each disjunct (AND block) separately and unions the results.
     */
    // OR-Queries: Union von mehreren AND-Blöcken
    Result<std::vector<std::string>> executeOrKeys(const DisjunctiveQuery& q) const;
    
    /**
     * @brief Execute disjunctive (OR) query and return full entities
     * @param q Disjunctive query (union of multiple conjunctive queries)
     * @return Vector of matching BaseEntity objects (deduplicated)
     * 
     * FIND-016: Added Doxygen documentation for public API
     */
    Result<std::vector<BaseEntity>> executeOrEntities(const DisjunctiveQuery& q) const;
    /**
     * @brief Execute OR query with fallback to full table scan
     * @param q Disjunctive query
     * @param optimize Enable query optimization (default: true)
     * @return Vector of matching primary keys
     * 
     * FIND-016: Added Doxygen documentation for public API
     * Falls back to full table scan if no suitable indexes are available.
     */
    // Varianten mit Fallback (nutzen Full-Scan, wenn kein Index vorhanden ist)
    Result<std::vector<std::string>> executeOrKeysWithFallback(
        const DisjunctiveQuery& q,
        bool optimize = true
    ) const;
    
    /**
     * @brief Execute OR query with fallback, returning full entities
     * @param q Disjunctive query
     * @param optimize Enable query optimization (default: true)
     * @return Vector of matching BaseEntity objects
     * 
     * FIND-016: Added Doxygen documentation for public API
     */
    Result<std::vector<BaseEntity>> executeOrEntitiesWithFallback(
        const DisjunctiveQuery& q,
        bool optimize = true
    ) const;

    /**
     * @brief Execute predicates sequentially in specified order
     * @param table Table name
     * @param orderedPredicates Equality predicates in execution order (from optimizer)
     * @return Vector of matching primary keys
     * 
     * FIND-016: Added Doxygen documentation for public API
     * Used by the query optimizer to execute predicates in optimal order
     * (e.g., most selective predicates first).
     */
    // Sequenzielles Ausführen in vorgegebener Reihenfolge (z. B. vom Optimizer)
    Result<std::vector<std::string>> executeAndKeysSequential(
        const std::string& table,
        const std::vector<PredicateEq>& orderedPredicates
    ) const;
    
    /**
     * @brief Execute predicates sequentially, returning full entities
     * @param table Table name
     * @param orderedPredicates Equality predicates in execution order
     * @return Vector of matching BaseEntity objects
     * 
     * FIND-016: Added Doxygen documentation for public API
     */
    Result<std::vector<BaseEntity>> executeAndEntitiesSequential(
        const std::string& table,
        const std::vector<PredicateEq>& orderedPredicates
    ) const;

    /**
     * @brief Execute AND query with fallback to full table scan
     * @param q Conjunctive query
     * @param optimize Enable query optimization (default: true)
     * @return Vector of matching primary keys
     * 
     * FIND-016: Added Doxygen documentation for public API
     * More flexible than executeAndKeys - falls back to full scan if indexes unavailable.
     */
    // Varianten mit Fallback (nutzen Full-Scan, wenn kein Index vorhanden ist)
    Result<std::vector<std::string>> executeAndKeysWithFallback(
        const ConjunctiveQuery& q,
        bool optimize = true
    ) const;
    
    /**
     * @brief Execute AND query with fallback, returning full entities
     * @param q Conjunctive query
     * @param optimize Enable query optimization (default: true)
     * @return Vector of matching BaseEntity objects
     * 
     * FIND-016: Added Doxygen documentation for public API
     */
    Result<std::vector<BaseEntity>> executeAndEntitiesWithFallback(
        const ConjunctiveQuery& q,
        bool optimize = true
    ) const;
    
    // Join/LET/COLLECT Support (MVP) - Declared in cpp to avoid header dependency
    struct EvaluationContext;
    
    Result<std::vector<nlohmann::json>> executeJoin(
        const std::vector<query::ForNode>& for_nodes,
        const std::vector<std::shared_ptr<query::FilterNode>>& filters,
        const std::vector<query::LetNode>& let_nodes,
        const std::shared_ptr<query::ReturnNode>& return_node,
        const std::shared_ptr<query::SortNode>& sort,
        const std::shared_ptr<query::LimitNode>& limit,
        const EvaluationContext* parent_context = nullptr  // Phase 4.1: For CTE results
    ) const;
    
    Result<std::vector<nlohmann::json>> executeGroupBy(
        const query::ForNode& for_node,
        const std::shared_ptr<query::CollectNode>& collect,
        const std::vector<std::shared_ptr<query::FilterNode>>& filters,
        const std::shared_ptr<query::ReturnNode>& return_node
    ) const;
    
    // Phase 4.1: CTE execution helper
    // Executes CTEs (decoupled from AQLTranslator types to avoid circular deps)
    struct CTESpec {
        std::string name;
        std::shared_ptr<query::Query> subquery;
        bool should_materialize = false;
    };
    
    /**
     * @brief Execute Common Table Expressions (CTEs) and store results in context
     * 
     * GAP-002: Migrated from Status to Result<void> for unified error handling
     * 
     * @param ctes Vector of CTE specifications to execute
     * @param context Evaluation context where CTE results will be stored
     * @return Result<void> indicating success or error with context
     */
    Result<void> executeCTEs(
        const std::vector<CTESpec>& ctes,
        EvaluationContext& context
    ) const;

    // ============================================================================
    // Hybrid Multi-Model Queries
    // ============================================================================
    
    // Vector + Geo: Spatial-filtered ANN search
    // Returns top-k vectors that satisfy spatial constraint
    struct VectorGeoResult {
        std::string pk;
        float vector_distance = 0.0f;
        nlohmann::json entity;
    };
    Result<std::vector<VectorGeoResult>> executeVectorGeoQuery(
        const VectorGeoQuery& q
    ) const;
    
    // Content + Geo: Fulltext + Spatial hybrid search
    // Returns documents matching fulltext query within spatial constraint
    struct ContentGeoResult {
        std::string pk;
        double bm25_score = 0.0;
        std::optional<double> geo_distance; // if boost_by_distance enabled
        nlohmann::json entity;
    };
    Result<std::vector<ContentGeoResult>> executeContentGeoQuery(
        const ContentGeoQuery& q
    ) const;
    
    // Filtered Vector Search: Pure vector search with attribute pre/post-filtering
    // No spatial constraints (use VectorGeoQuery for geo+vector)
    struct FilteredVectorSearchResult {
        std::string pk;
        float vector_distance = 0.0f;
        nlohmann::json entity;
    };
    Result<std::vector<FilteredVectorSearchResult>> executeFilteredVectorSearch(
        const FilteredVectorSearchQuery& q
    ) const;
    
    // Radius Vector Search: Epsilon-based neighbors with attribute filtering
    // Returns all vectors within distance threshold (epsilon)
    struct RadiusVectorSearchResult {
        std::string pk;
        float vector_distance = 0.0f;
        nlohmann::json entity;
    };
    Result<std::vector<RadiusVectorSearchResult>> executeRadiusVectorSearch(
        const RadiusVectorSearchQuery& q
    ) const;
    
    // Content Search: Fulltext + Metadata filtering
    // Returns documents matching fulltext query and metadata filters
    struct ContentSearchResult {
        std::string pk;
        double bm25_score = 0.0;
        nlohmann::json entity;
    };
    Result<std::vector<ContentSearchResult>> executeContentSearch(
        const ContentSearchQuery& q
    ) const;

    // ------------------------------------------------------------------
    // Query Plan Visualisation
    // ------------------------------------------------------------------

    /// Build an execution plan tree for the given conjunctive query.
    /// Uses the internal SecondaryIndexManager to estimate predicate selectivity
    /// and order predicates optimally, then constructs a QueryPlanNode tree via
    /// QueryPlanVisualizer::buildPlan().
    ///
    /// The returned QueryPlanNode can be rendered as text, JSON, or DOT using
    /// QueryPlanVisualizer::toText() / toJSON() / toDOT().
    ///
    /// If the engine has no attached SecondaryIndexManager, all estimates default
    /// to zero and the optimizer still produces a structurally valid plan tree.
    ///
    /// @param q  The logical AND query whose plan is requested.
    /// @returns  Root node of the execution plan tree.
    query::QueryPlanNode buildExplainPlan(const ConjunctiveQuery& q) const;

    /**
     * @brief List all collection/table names known to the storage layer.
     *
     * Scans all keys in the underlying RocksDB and returns the unique
     * collection or table names extracted from the key schema prefix
     * (e.g. keys of the form `doc:<name>:<pk>` or `rel:<name>:<pk>`).
     *
     * Returns an empty vector when the engine was constructed with the DI
     * constructor and no legacy RocksDB instance is available.
     *
     * @return Sorted list of unique collection/table names.
     */
    std::vector<std::string> listCollections() const;

private:
    RocksDBWrapper* db_ = nullptr;  // Changed from reference to pointer to support nullptr in DI constructor
    SecondaryIndexManager* secIdx_ = nullptr;  // Changed from reference to pointer
    GraphIndexManager* graphIdx_ = nullptr;
    VectorIndexManager* vectorIdx_ = nullptr;  // Optional for Vector+Geo optimization
    SpatialIndexManager* spatialIdx_ = nullptr;  // Optional for Spatial pre-filtering
    StatisticsCollector* stats_collector_ = nullptr;  ///< Optional; for cardinality-based optimisation
    ::themis::utils::AuditLogger* audit_logger_ = nullptr;  ///< Optional non-owning audit sink for query phase telemetry
    std::function<bool(const std::string&, const std::string&)> collection_access_checker_;
    std::string collection_access_caller_id_;  ///< Caller identity forwarded to access checker

    struct AuditConfigSnapshot {
        ::themis::utils::AuditLogger* audit_logger;
        std::chrono::milliseconds query_timeout_ms;
    };

    // ── Concurrency protection (Q4) ─────────────────────────────────────────
    /// Guards runtime-configurable fields (audit_logger_, stats_collector_,
    /// collection_access_checker_, collection_access_caller_id_,
    /// query_timeout_ms_, lock_timeout_ms_) against concurrent set/get races.
    mutable std::mutex config_mutex_;

    [[nodiscard]] AuditConfigSnapshot snapshotAuditConfig() const noexcept {
        std::lock_guard<std::mutex> lk(config_mutex_);
        return AuditConfigSnapshot{audit_logger_, query_timeout_ms_};
    }

    /// Per-query advisory timeout: how long a tg.wait() phase may take before
    /// a structured timeout audit event is emitted.  Default: 30 s.
    std::chrono::milliseconds query_timeout_ms_{30000};

    /// Per-lock advisory timeout: threshold for lock-acquisition warnings.
    /// Default: 1 s.
    std::chrono::milliseconds lock_timeout_ms_{1000};
    
    // New interface-based dependencies (used with DI constructors)
    // When these are set, they take precedence over legacy pointers
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
    
    // Internal expression evaluator that wraps QueryEngine's evaluation logic
    // This is returned by get_expression_evaluator() to break circular dependencies
    /** @brief This is returned by get_expression_evaluator() to break circular dependencies. */
    class QueryExpressionEvaluator : public IExpressionEvaluator {
    public:
    ~QueryExpressionEvaluator() override = default;
        explicit QueryExpressionEvaluator(QueryEngine* engine) 
            : engine_(engine) {}
        
        // Delegates to AQL parser + QueryEngine::evaluateCondition()
        bool evaluate(const std::string& expression, const void* context) const override;
        std::string get_expression_type() const override;

        // Helpers for richer evaluation paths (non-override)
        bool evaluateBoolean(std::string_view expression, const void* context) const;
        bool canEvaluate(std::string_view expression) const;
        
    private:
        QueryEngine* engine_;
    };
    
    // Expression evaluation helpers (implemented in cpp)
    Result<nlohmann::json> evaluateExpression(
        const std::shared_ptr<query::Expression>& expr,
        const EvaluationContext& ctx
    ) const;

    static std::vector<std::string> intersectSortedLists_(std::vector<std::vector<std::string>> lists);
    static std::vector<std::string> unionSortedLists_(std::vector<std::vector<std::string>> lists);

    // Full-Scan Fallback: Durchsucht alle Reihen einer Tabelle und filtert per Prädikaten
    std::vector<std::string> fullScanAndFilter_(const ConjunctiveQuery& q) const;

    // Range-Unterstützung
    Result<std::vector<std::string>> executeAndKeysRangeAware_(const ConjunctiveQuery& q) const;
    Result<std::vector<BaseEntity>> executeAndEntitiesRangeAware_(const ConjunctiveQuery& q) const;
};

// EvaluationContext Definition (moved from class body to avoid json dependency in header)
struct QueryEngine::EvaluationContext {
    std::unordered_map<std::string, nlohmann::json> bindings;
    // Optional: BM25/FULLTEXT score context, keyed by primary key ("_key")
    std::shared_ptr<std::unordered_map<std::string, double>> bm25_scores;
    
    // Phase 3: CTE materialization storage (legacy - use cte_cache for large CTEs)
    std::unordered_map<std::string, std::vector<nlohmann::json>> cte_results;
    
    // Phase 4.3: Managed CTE cache with spill-to-disk
    std::shared_ptr<query::CTECache> cte_cache;
    
    // Phase 3.4: Parent context for correlated subqueries
    const EvaluationContext* parent = nullptr;
    
    void bind(const std::string& var, nlohmann::json value) {
        bindings[var] = std::move(value);
    }
    
    std::optional<nlohmann::json> get(const std::string& var) const {
        // First check local bindings
        auto it = bindings.find(var);
        if (it != bindings.end()) {
          return std::make_optional(it->second);
        }
        
        // Phase 3.4: Check parent context for correlated variables
        if (parent) {
            return parent->get(var);
        }
        
        return std::nullopt;
    }
    
    void setBm25Scores(std::shared_ptr<std::unordered_map<std::string, double>> scores) {
        bm25_scores = std::move(scores);
    }
    double getBm25ScoreForPk(const std::string& pk) const {
        if (!bm25_scores) {
          return 0.0;
        }
        auto it = bm25_scores->find(pk);
        if (it == bm25_scores->end()) {
          return 0.0;
        }
        return it->second;
    }
    
    // Phase 4.3: CTE access with cache fallback (out-of-line to avoid incomplete CTECache)
    void storeCTE(const std::string& name, std::vector<nlohmann::json> results);
    std::optional<std::vector<nlohmann::json>> getCTE(const std::string& name) const;
    
    // Phase 3.4: Create child context with parent chain
    EvaluationContext createChild() const {
        EvaluationContext child;
        child.parent = this;
        child.bm25_scores = bm25_scores; // Share BM25 scores
        child.cte_results = cte_results;  // Share CTE results
        child.cte_cache = cte_cache;      // Share CTE cache
        return child;
    }
};

} // namespace query
} // namespace themis


