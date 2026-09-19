/**
 * @file knowledge_base.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * KnowledgeBase — ThemisDB Expert System working memory + rule store.
 *
 * Stores facts as (subject, predicate, object) triples and Horn clause
 * rules.  Supports FIFO eviction when the working memory exceeds kMaxFacts.
 *
 * Thread-safety: NOT thread-safe internally — callers (ExpertSystemEngine)
 * are responsible for external synchronisation.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// Fact
// ──────────────────────────────────────────────────────────────────────────────

struct Fact {
    std::string id;             ///< Unique id assigned on assertion (e.g. "f_0001")
    std::string subject;
    std::string predicate;
    std::string object;
    int64_t     asserted_at_ms = 0;  ///< Unix epoch ms (set by assertFact)
};

// ──────────────────────────────────────────────────────────────────────────────
// TriplePattern
// ──────────────────────────────────────────────────────────────────────────────

struct TriplePattern {
    std::string subject;    ///< e.g. "?incident" or "Alice"
    std::string predicate;  ///< e.g. "type" or "?p"
    std::string object;     ///< e.g. "SecurityIncident" or "?count"
};

// ──────────────────────────────────────────────────────────────────────────────
// HornClause
// ──────────────────────────────────────────────────────────────────────────────

struct HornClause {
    std::string                  id = {};
    int                          priority = 0;     ///< Higher fires first
    std::string                  description;
    std::vector<TriplePattern>   conditions;       ///< All must match (conjunction)
    std::vector<TriplePattern>   consequents;      ///< Derived on match
    double                       ml_confidence_threshold = 0.0; ///< 0.0 = no ML required
};

// ──────────────────────────────────────────────────────────────────────────────
// KnowledgeBase
// ──────────────────────────────────────────────────────────────────────────────

class KnowledgeBase {
public:
    static constexpr std::size_t kMaxFacts = 10'000;

    KnowledgeBase() = default;
    ~KnowledgeBase() = default;

    KnowledgeBase(const KnowledgeBase&)            = delete;
    KnowledgeBase& operator=(const KnowledgeBase&) = delete;

    // ── Working Memory ────────────────────────────────────────────────────────

    [[nodiscard]] std::string assertFact(const std::string& subject,
                                          const std::string& predicate,
                                          const std::string& object);

    /**
     * @brief Retract Fact.
     * @param[in] fact_id Identifier of the fact.
     * @return True when the operation succeeds.
     */
    bool retractFact(const std::string& fact_id);

    [[nodiscard]] std::vector<Fact> getFacts(const std::string& predicate = "") const;

    /**
     * @brief Get Fact By Triple.
     * @param[in] subject Subject component of the triple.
     * @param[in] predicate Predicate component of the triple.
     * @param[in] object Object component of the triple.
     * @return Matching fact when the exact triple exists, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<Fact> getFact(const std::string& subject, const std::string& predicate,
                                              const std::string& object) const;

    /**
     * @brief Check Whether A Triple Exists.
     * @param[in] subject Subject component of the triple.
     * @param[in] predicate Predicate component of the triple.
     * @param[in] object Object component of the triple.
     * @return True when the exact triple exists in working memory.
     */
    [[nodiscard]] bool hasFact(const std::string& subject, const std::string& predicate,
                               const std::string& object) const;

    [[nodiscard]] std::optional<Fact> getFactById(const std::string& id) const;

    [[nodiscard]] std::size_t factCount() const noexcept { return insertion_order_.size(); }

    /**
     * @brief Clear Facts.
     */
    void clearFacts();


    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(HornClause rule);

    /**
     * @brief Remove Rule.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool removeRule(const std::string& rule_id);

    [[nodiscard]] std::vector<HornClause> getRules() const;

    [[nodiscard]] std::size_t ruleCount() const noexcept { return rules_.size(); }

    /**
     * @brief Clear Rules.
     */
    void clearRules();

    [[nodiscard]] int loadRulesFromYaml(const std::string& path);

    // ─── YAML parser bridge injection API (STUB #272) ─────────────────────────

    using YamlParserFn = std::function<int(const std::string& path, KnowledgeBase&)>;

    /**
     * @brief Set Yaml Parser Fn.
     * @param[in] fn Input parameter.
     */
    static void setYamlParserFn(YamlParserFn fn);
    /**
     * @brief Clear Yaml Parser Fn.
     */
    static void clearYamlParserFn();

private:
    [[nodiscard]] std::string generateId();

    // Predicate-indexed storage for O(1) average getFacts(predicate) lookup.
    std::unordered_multimap<std::string, Fact> facts_by_predicate_;
    // fact_id → predicate (for retract)
    std::unordered_map<std::string, std::string> fact_id_to_predicate_;
    // fact_id → Fact (for getFactById)
    std::unordered_map<std::string, Fact> fact_by_id_;
    // triple key → fact id (for O(1) exact fact existence checks)
    std::unordered_map<std::string, std::string> fact_key_to_id_;
    // insertion-ordered ids for FIFO eviction
    std::deque<std::string> insertion_order_;

    std::vector<HornClause> rules_;

    // Monotonically increasing counter for id generation (not thread-safe by design).
    std::size_t id_counter_ = 0;

    [[nodiscard]] static std::string makeFactKey(const std::string& subject, const std::string& predicate,
                                                 const std::string& object);
};

} // namespace analytics
} // namespace themisdb
