/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analytics/knowledge_base.h                         ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   92.0/100                                        ║
    • Open Issues:     STUBs: 1 (#259 — YAML loader line-parser)       ║
╚═════════════════════════════════════════════════════════════════════╝
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
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// Fact
// ──────────────────────────────────────────────────────────────────────────────

/**
 * A (subject, predicate, object) fact triple stored in the working memory.
 */
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

/**
 * A pattern element for Horn clause conditions / consequents.
 * Strings starting with '?' are variable names; others are literals.
 */
struct TriplePattern {
    std::string subject;    ///< e.g. "?incident" or "Alice"
    std::string predicate;  ///< e.g. "type" or "?p"
    std::string object;     ///< e.g. "SecurityIncident" or "?count"
};

// ──────────────────────────────────────────────────────────────────────────────
// HornClause
// ──────────────────────────────────────────────────────────────────────────────

/**
 * A Horn clause rule: if all conditions match, derive each consequent.
 */
struct HornClause {
    std::string                  id;
    int                          priority = 0;     ///< Higher fires first
    std::string                  description;
    std::vector<TriplePattern>   conditions;       ///< All must match (conjunction)
    std::vector<TriplePattern>   consequents;      ///< Derived on match
    double                       ml_confidence_threshold = 0.0; ///< 0.0 = no ML required
};

// ──────────────────────────────────────────────────────────────────────────────
// KnowledgeBase
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Persistent (in-memory) working memory + Horn clause rule store.
 *
 * Facts are indexed by predicate for O(1) average lookup.
 * When kMaxFacts is reached, the oldest inserted fact is evicted (FIFO).
 */
class KnowledgeBase {
public:
    static constexpr std::size_t kMaxFacts = 10'000;

    KnowledgeBase() = default;
    ~KnowledgeBase() = default;

    KnowledgeBase(const KnowledgeBase&)            = delete;
    KnowledgeBase& operator=(const KnowledgeBase&) = delete;

    // ── Working Memory ────────────────────────────────────────────────────────

    /**
     * Assert a new fact.  Returns the assigned fact id.
     * Evicts the oldest fact when size reaches kMaxFacts.
     */
    [[nodiscard]] std::string assertFact(const std::string& subject,
                                          const std::string& predicate,
                                          const std::string& object);

    /**
     * Retract the fact with the given id.
     * @return true iff the fact was found and removed.
     */
    bool retractFact(const std::string& fact_id);

    /**
     * Return all facts with the given predicate.
     * If predicate is empty, returns all facts in the WM.
     */
    [[nodiscard]] std::vector<Fact> getFacts(const std::string& predicate = "") const;

    /**
     * Return the fact with the given id, or nullopt if not found.
     */
    [[nodiscard]] std::optional<Fact> getFactById(const std::string& id) const;

    /**
     * Return the number of facts currently in working memory.
     */
    [[nodiscard]] std::size_t factCount() const noexcept { return insertion_order_.size(); }

    /**
     * Remove all facts (rules are preserved).
     */
    void clearFacts();

    // ── Rule Store ────────────────────────────────────────────────────────────

    /**
     * Add a Horn clause rule programmatically.
     * Rules with the same id overwrite the previous entry.
     */
    void addRule(HornClause rule);

    /**
     * Remove the rule with the given id.
     * @return true iff the rule was found and removed.
     */
    bool removeRule(const std::string& rule_id);

    /**
     * Return all rules sorted by priority descending.
     */
    [[nodiscard]] std::vector<HornClause> getRules() const;

    /**
     * Return the number of registered rules.
     */
    [[nodiscard]] std::size_t ruleCount() const noexcept { return rules_.size(); }

    /**
     * Remove all rules (facts are preserved).
     */
    void clearRules();

    /**
     * Load Horn clause rules from a YAML file.
     *
     * STUB/SIMULATION NOTE:
     * Purpose: Simple inline YAML parser for the Horn clause rule format.
     * Activation: Always (no yaml-cpp dependency required).
     * Production Delta: Only handles the specific rule format documented in
     *   FUTURE_ENHANCEMENTS.md §Expert System Engine. Complex YAML features
     *   (anchors, aliases, multi-line strings, nested maps beyond one level)
     *   are not supported.
     * Removal Plan: Q2 2027 — wire yaml-cpp (already optional dep) for full
     *   YAML spec compliance.
     *
     * @return Number of rules loaded, or -1 on file-open error.
     */
    [[nodiscard]] int loadRulesFromYaml(const std::string& path);

private:
    [[nodiscard]] std::string generateId();

    // Predicate-indexed storage for O(1) average getFacts(predicate) lookup.
    std::unordered_multimap<std::string, Fact> facts_by_predicate_;
    // fact_id → predicate (for retract)
    std::unordered_map<std::string, std::string> fact_id_to_predicate_;
    // fact_id → Fact (for getFactById)
    std::unordered_map<std::string, Fact> fact_by_id_;
    // insertion-ordered ids for FIFO eviction
    std::deque<std::string> insertion_order_;

    std::vector<HornClause> rules_;

    // Monotonically increasing counter for id generation (not thread-safe by design).
    std::size_t id_counter_ = 0;
};

} // namespace analytics
} // namespace themisdb
