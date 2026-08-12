/**
 * @file conflict_resolution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Advanced Conflict Resolution API
 *
 * Extensible conflict resolution framework for multi-master replication.
 * Provides three concrete resolvers on top of the existing IConflictResolver
 * interface:
 *
 *   ThreeWayMergeResolver  — Git-style three-way merge using the vector-clock
 *                            common ancestor as the base document.
 *   FieldLevelMergeResolver — Merges individual JSON fields according to a
 *                             configurable strategy (UNION, INTERSECT, bias).
 *   AdvancedConflictResolver — Abstract base with richer ResolutionContext.
 *
 * All resolvers are stateless; resolve() receives every context value by
 * value and returns the winning WALEntry in O(1)–O(N·F) time where N is the
 * number of conflicting writes and F is the average field count.
 *
 * Target: v1.7.0
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "replication/replication_manager.h"
#include "replication/multi_master_replication.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// AdvancedConflictResolver — abstract base with ResolutionContext
// ============================================================================

/**
 * AdvancedConflictResolver
 *
 * Extends IConflictResolver with a richer ResolutionContext so that
 * application logic can take collection name, user roles, request timestamp,
 * and arbitrary metadata into account when resolving a conflict.
 *
 * Implementations MUST be stateless: every call to resolve() must be
 * idempotent and thread-safe.
 */
class AdvancedConflictResolver {
public:
    /**
     * Context provided to resolve() on every invocation.
     */
    struct ResolutionContext {
        std::string collection;
        std::string document_id;
        std::map<std::string, std::string>   metadata;
        std::vector<std::string>             user_roles;
        std::string                          client_ip;
        std::chrono::system_clock::time_point request_time = std::chrono::system_clock::now();
    };

    virtual ~AdvancedConflictResolver() = default;

    /**
     * Select the winning write from a set of concurrent conflicting writes.
     *
     * @param document_id        Identifier of the document in conflict.
     * @param conflicting_writes Two or more concurrent MMWriteEntry values.
     * @param context            Additional context for resolution logic.
     * @return                   The winning write entry.
     *
     * Constraints:
     *   - Must be thread-safe and idempotent.
     *   - Must return one of the entries from conflicting_writes (no fabrication).
     *   - conflicting_writes.size() >= 2 is guaranteed by the caller.
     */
    [[nodiscard]] virtual MMWriteEntry resolve(
        const std::string&                document_id,
        const std::vector<MMWriteEntry>&  conflicting_writes,
        const ResolutionContext&          context
    ) = 0;

    /** Human-readable name of this resolver strategy. */
    [[nodiscard]] virtual std::string strategyName() const = 0;
};

// ============================================================================
// ThreeWayMergeResolver
// ============================================================================

/**
 * ThreeWayMergeResolver
 *
 * Resolves conflicts by performing a three-way JSON merge.
 *
 * Algorithm:
 *   1. Identify the common ancestor write by comparing vector clocks:
 *      the write whose clock is dominated by all others is the base.
 *      If no unambiguous ancestor exists the write with the smallest
 *      HLC timestamp is used as the base.
 *   2. For each top-level JSON key in the conflicting writes:
 *      - If only one side changed the key   → take that side's value.
 *      - If both sides changed the key      → Last-Write-Wins on that key.
 *      - If neither side changed the key    → take the base value.
 *   3. Keys present in only one side are included (UNION behaviour).
 *
 * Complexity: O(N · F) where N = writes, F = average top-level field count.
 */
class ThreeWayMergeResolver : public AdvancedConflictResolver {
public:
    ThreeWayMergeResolver() = default;

    MMWriteEntry resolve(
        const std::string&                document_id,
        const std::vector<MMWriteEntry>&  conflicting_writes,
        const ResolutionContext&          context
    ) override;

    std::string strategyName() const override { return "THREE_WAY_MERGE"; }

private:
    /** Select the best common-ancestor candidate from the write set. */
    MMWriteEntry selectBase(const std::vector<MMWriteEntry>& writes) const;

    /**
     * Perform the actual three-way merge of serialised JSON objects.
     * Returns merged JSON string; falls back to `left` on parse error.
     */
    std::string mergeJson(
        const std::string& base,
        const std::string& left,
        const std::string& right
    ) const;
};

// ============================================================================
// FieldLevelMergeResolver
// ============================================================================

/**
 * FieldLevelMergeResolver
 *
 * Merges individual JSON top-level fields according to a configurable strategy.
 *
 * Strategies:
 *   UNION      — Include all fields; on conflict prefer the write with the
 *                latest HLC timestamp for that specific field.
 *   INTERSECT  — Only include fields present in ALL conflicting writes; on
 *                conflict prefer the latest HLC timestamp.
 *   LEFT_BIAS  — For conflicting fields always prefer the first entry in
 *                conflicting_writes (i.e., the one with the lowest HLC).
 *   RIGHT_BIAS — For conflicting fields always prefer the last entry in
 *                conflicting_writes (i.e., the one with the highest HLC).
 */
class FieldLevelMergeResolver : public AdvancedConflictResolver {
public:
    enum class MergeStrategy {
        UNION,       ///< Take all fields; latest-HLC wins on conflict
        INTERSECT,   ///< Take only common fields; latest-HLC wins on conflict
        LEFT_BIAS,   ///< Prefer first (lowest HLC) write on conflict
        RIGHT_BIAS   ///< Prefer last (highest HLC) write on conflict
    };

    explicit FieldLevelMergeResolver(MergeStrategy strategy = MergeStrategy::UNION);

    MMWriteEntry resolve(
        const std::string&                document_id,
        const std::vector<MMWriteEntry>&  conflicting_writes,
        const ResolutionContext&          context
    ) override;

    std::string strategyName() const override;

private:
    MergeStrategy strategy_;

    /**
     * Merge a collection of (field_name → value_string) maps.
     * Returns a JSON object string.
     */
    std::string mergeFields(
        const std::vector<MMWriteEntry>& writes
    ) const;
};

} // namespace replication
} // namespace themisdb
