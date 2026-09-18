/**
 * @file ontology_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class OntologyManager {
public:
    // ── Tuning constants ────────────────────────────────────────────────────
    static constexpr int kMaxIsADepth = 20;
    static constexpr std::size_t kIsACacheCapacity = 1000;

    // ── Ruleset modes ───────────────────────────────────────────────────────
    enum class Ruleset {
        Strict,
        Warn
    };

    // ── Internal data structures ────────────────────────────────────────────
    struct ConceptNode {
        std::string id;
        std::vector<std::string> parents;
        std::unordered_set<std::string> allowed_edge_types_as_source;
        std::unordered_set<std::string> allowed_edge_types_as_target;
    };

    struct Axiom {
        std::string source_class;
        std::string edge_type;
        std::string target_class;
    };

    // ── Life-cycle ──────────────────────────────────────────────────────────
    OntologyManager() = default;

    OntologyManager(const OntologyManager&) = delete;
    OntologyManager& operator=(const OntologyManager&) = delete;

    OntologyManager(OntologyManager&&) noexcept = default;
    OntologyManager& operator=(OntologyManager&&) noexcept = default;

    ~OntologyManager() = default;

    /**
     * @brief ── Schema loading ──────────────────────────────────────────────────────
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */

    bool loadFromJson(std::string_view path);

    /**
     * @brief Load From Yaml.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromYaml(std::string_view path);

    /**
     * @brief Load From Json String.
     * @param[in] json_text Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromJsonString(std::string_view json_text);

    void addConcept(std::string id, std::vector<std::string> parents = {});

    /**
     * @brief Add Axiom.
     * @param[in] source_class Input parameter.
     * @param[in] edge_type Input parameter.
     * @param[in] target_class Input parameter.
     */
    void addAxiom(std::string source_class, std::string edge_type, std::string target_class);

    /**
     * @brief Build.
     */
    void build();

    /**
     * @brief ── Query API ───────────────────────────────────────────────────────────
     * @param[in] conceptName Input parameter.
     * @param[in] superConcept Input parameter.
     * @return True when the operation succeeds.
     */

    bool isA(std::string_view conceptName, std::string_view superConcept) const;

    [[nodiscard]] std::unordered_set<std::string> allowedEdgeTypes(
        std::string_view sourceClass,
        std::string_view targetClass) const;

     [[nodiscard]] bool isEdgeTypeAllowed(std::string_view sourceClass,
                                          std::string_view targetClass,
                                          std::string_view edgeType) const;

    // ── Serialisation ───────────────────────────────────────────────────────

    [[nodiscard]] std::string toJson() const;

    [[nodiscard]] std::string toYaml() const;

    // ── Introspection ───────────────────────────────────────────────────────

    [[nodiscard]] bool isBuilt() const noexcept { return built_; }

    [[nodiscard]] std::size_t conceptCount() const noexcept { return concepts_.size(); }

    [[nodiscard]] std::size_t axiomCount() const noexcept { return axioms_.size(); }

    [[nodiscard]] bool hasConcept(std::string_view id) const;

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

    /**
     * @brief ── Internal helpers ─────────────────────────────────────────────────────
     * @param[in] conceptName Input parameter.
     * @param[in] superConcept Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAUncached(std::string_view conceptName, std::string_view superConcept) const;
    /**
     * @brief Evict Is ACache Entry.
     */
    void evictIsACacheEntry() const;

    /**
     * @brief Parse Json.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseJson(const std::string& text);
    /**
     * @brief Parse Yaml.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseYaml(const std::string& text);
};

} // namespace graph
} // namespace themis
