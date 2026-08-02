/**
 * @file ontology_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: ontology_manager.h | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 275
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5169 Research review: Complete f... (2026-05-19)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <optional>
#include <utility>
#include <functional>

namespace themis {
namespace graph {

/**
 * @brief OWL-lite ontology concept hierarchy for semantic graph constraints.
 *
 * @par Design
 * - Immutable after `build()` — all queries are read-only and lock-free (single
 *   writer during load; `build()` seals the object for concurrent readers).
 * - `isA()` performs BFS over the ancestor chain (depth-limited to
 *   kMaxIsADepth hops) with a bounded LRU result cache of kIsACacheCapacity
 *   entries to amortize repeated lookups.
 * - Unknown concept IDs are treated as unconstrained (warning is logged, no
 *   exception is thrown). This ensures graceful degradation when new edge types
 *   are introduced before the ontology schema is updated.
 *
 * @par JSON Schema (OWL-lite subset)
 * @code{.json}
 * {
 *   "concepts": [
 *     { "id": "Entity" },
 *     { "id": "LegalEntity", "parents": ["Entity"] },
 *     { "id": "Person",      "parents": ["LegalEntity"] }
 *   ],
 *   "axioms": [
 *     { "source_class": "Person", "edge_type": "knows", "target_class": "Person" }
 *   ]
 * }
 * @endcode
 *
 * @par YAML Schema (equivalent)
 * @code{.yaml}
 * concepts:
 *   - id: Entity
 *   - id: LegalEntity
 *     parents: [Entity]
 * axioms:
 *   - source_class: Person
 *     edge_type: knows
 *     target_class: Person
 * @endcode
 */
class OntologyManager {
public:
    // ── Tuning constants ────────────────────────────────────────────────────
    /// Maximum ancestor-chain depth explored by isA().
    static constexpr int kMaxIsADepth = 20;
    /// Capacity of the per-instance isA result cache (entry count).
    static constexpr std::size_t kIsACacheCapacity = 1000;

    // ── Ruleset modes ───────────────────────────────────────────────────────
    /// Controls how constraint violations are reported.
    enum class Ruleset {
        /// Constraint violations cause path rejection (default).
        Strict,
        /// Constraint violations emit a warning but do not reject the path.
        Warn
    };

    // ── Internal data structures ────────────────────────────────────────────
    /**
     * @brief Single concept node in the ontology DAG.
     *
     * Stores parent class IDs, the set of edge types that this class is
     * permitted to emit as a source, and the set it may receive as a target.
     */
    struct ConceptNode {
        std::string id;
        std::vector<std::string> parents;
        /// Edge types for which (this_class → any_class) axioms exist.
        std::unordered_set<std::string> allowed_edge_types_as_source;
        /// Edge types for which (any_class → this_class) axioms exist.
        std::unordered_set<std::string> allowed_edge_types_as_target;
    };

    /**
     * @brief Single OWL-lite domain/range axiom.
     *
     * Declares that @p edge_type may connect a node of class @p source_class to
     * a node of class @p target_class (inclusive of subclasses when isA() is used).
     */
    struct Axiom {
        std::string source_class;
        std::string edge_type;
        std::string target_class;
    };

    // ── Life-cycle ──────────────────────────────────────────────────────────
    OntologyManager() = default;

    /// Non-copyable — too heavy for accidental copies; share via shared_ptr.
    OntologyManager(const OntologyManager&) = delete;
    OntologyManager& operator=(const OntologyManager&) = delete;

    OntologyManager(OntologyManager&&) = default;
    OntologyManager& operator=(OntologyManager&&) = default;

    /// Explicit destructor for Rule of Five compliance and semantic clarity.
    /// Cleans up all member resources (maps, lists, mutexes); relies on standard
    /// library destructors for cleanup (RAII principle).
    ~OntologyManager() = default;

    // ── Schema loading ──────────────────────────────────────────────────────

    /**
     * @brief Load an OWL-lite JSON schema from a file.
     *
     * Expected JSON structure: { "concepts": [...], "axioms": [...] }.
     * See class docstring for the exact schema.
     *
     * @param path Filesystem path to the JSON file.
     * @return true on success, false on parse or I/O error.
     */
    bool loadFromJson(std::string_view path);

    /**
     * @brief Load an OWL-lite schema from a YAML file.
     *
     * Parses a manually-implemented subset (sequences, mappings, scalars).
     * Requires no external yaml-cpp dependency.
     *
     * @param path Filesystem path to the YAML file.
     * @return true on success, false on parse or I/O error.
     */
    bool loadFromYaml(std::string_view path);

    /**
     * @brief Load an OWL-lite JSON schema from an in-memory string.
     *
     * Identical semantics to `loadFromJson(path)` but operates on a string
     * directly; useful for unit tests and embedded schemas.
     *
     * @param json_text Raw JSON text.
     * @return true on success, false on parse error.
     */
    bool loadFromJsonString(std::string_view json_text);

    /**
     * @brief Add a concept programmatically (before build()).
     */
    void addConcept(std::string id, std::vector<std::string> parents = {});

    /**
     * @brief Add an axiom programmatically (before build()).
     */
    void addAxiom(std::string source_class, std::string edge_type, std::string target_class);

    /**
     * @brief Finalise the concept DAG and seal the object for read-only access.
     *
     * Must be called once after all `loadFrom*` / `addConcept` / `addAxiom`
     * invocations and before any `isA` / `allowedEdgeTypes` queries.
     * Subsequent `loadFrom*` calls after `build()` are a no-op.
     *
     * `build()` propagates axiom-derived edge-type sets along the inheritance
     * hierarchy so that subclasses automatically inherit parent permissions.
     */
    void build();

    // ── Query API ───────────────────────────────────────────────────────────

    /**
     * @brief Check whether @p conceptName is the same as or a subclass of @p superConcept.
     *
     * Performs BFS over the ancestor chain up to kMaxIsADepth hops.
     * Results are cached in an LRU table (capacity: kIsACacheCapacity) to
     * amortize repeated lookups during hot traversal loops.
     *
     * Unknown concepts return `false` without throwing (graceful degradation).
     *
    * @param conceptName  Concept identifier to test.
     * @param superConcept Candidate ancestor concept.
    * @return true if @p conceptName is-a @p superConcept.
     */
    bool isA(std::string_view conceptName, std::string_view superConcept) const;

    /**
     * @brief Return the set of edge types permitted between a source and target class.
     *
     * Returns all edge types `e` for which an axiom (src, e, tgt) exists such that
     * @p sourceClass isA src AND @p targetClass isA tgt.
     *
     * Unknown classes return an empty set (graceful degradation).
     *
     * @param sourceClass Class of the source node.
     * @param targetClass Class of the target node.
     * @return Unordered set of allowed edge type strings; empty if unconstrained.
     */
    [[nodiscard]] std::unordered_set<std::string> allowedEdgeTypes(
        std::string_view sourceClass,
        std::string_view targetClass) const;

    /**
     * @brief Check if an edge type is allowed between two entity types.
     *
     * Returns `true` if:
     * - Either @p sourceClass or @p targetClass is unknown (graceful degradation), OR
     * - @p edgeType is explicitly allowed for the class pair by ontology axioms, OR
     * - Axioms exist for the class pair and @p edgeType is unknown globally
     *   (schema-evolution fallback).
     *
     * Returns `false` if:
     * - Both classes are known AND there are no axioms for this pair (strict mode).
     * - @p edgeType is known in the ontology but not allowed for this class pair.
     *
     * @see allowedEdgeTypes() to retrieve the exact set of axiom-defined edge types.
     */
    [[nodiscard]] bool isEdgeTypeAllowed(std::string_view sourceClass,
                                         std::string_view targetClass,
                                         std::string_view edgeType) const;

    // ── Serialisation ───────────────────────────────────────────────────────

    /**
     * @brief Serialise the ontology to a JSON string (round-trip safe).
     * @return JSON text equivalent to the format accepted by `loadFromJson`.
     */
    [[nodiscard]] std::string toJson() const;

    /**
     * @brief Serialise the ontology to a YAML string (round-trip safe).
     * @return YAML text equivalent to the format accepted by `loadFromYaml`.
     */
    [[nodiscard]] std::string toYaml() const;

    // ── Introspection ───────────────────────────────────────────────────────

    /// Returns true if `build()` has been called.
    [[nodiscard]] bool isBuilt() const noexcept { return built_; }

    /// Number of concepts currently registered.
    [[nodiscard]] std::size_t conceptCount() const noexcept { return concepts_.size(); }

    /// Number of axioms currently registered.
    [[nodiscard]] std::size_t axiomCount() const noexcept { return axioms_.size(); }

    /// True if the concept with the given @p id exists in the ontology.
    [[nodiscard]] bool hasConcept(std::string_view id) const;

    /// Retrieve a concept node by id; returns nullptr if not found.
    [[nodiscard]] const ConceptNode* getConcept(std::string_view id) const;

private:
    // ── Storage ─────────────────────────────────────────────────────────────
    std::unordered_map<std::string, ConceptNode> concepts_;
    std::vector<Axiom> axioms_;
    bool built_ = false;

    // ── isA LRU cache ────────────────────────────────────────────────────────
    // Key: "concept\0superConcept" (null-byte separator avoids ambiguity)
    mutable std::unordered_map<std::string, bool> isa_cache_;
    mutable std::list<std::string> isa_cache_lru_;  // front = oldest; O(1) eviction
    mutable std::shared_mutex isa_cache_mutex_;

    // ── Internal helpers ─────────────────────────────────────────────────────
    bool isAUncached(std::string_view conceptName, std::string_view superConcept) const;
    void evictIsACacheEntry() const;

    /// Parse JSON text into concepts_ + axioms_ (shared by file and string loaders).
    bool parseJson(const std::string& text);
    /// Parse YAML text into concepts_ + axioms_.
    bool parseYaml(const std::string& text);
};

} // namespace graph
} // namespace themis
