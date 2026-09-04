/**
 * @file document_diff_merge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    document_diff_merge.h
 * Module:  include/document/
 * Purpose: IDocumentDiffMerge — structured field-level diff and three-way
 *          merge for document pairs.  Includes InMemoryDocumentDiffMerge as
 *          a reference implementation backed by IDocumentStore.
 *
 * Version: 1.3.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "document/document_store.h"
#include "utils/expected.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// FieldChange
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single field-level change between two document versions.
 */
struct FieldChange {
    std::string    field_name;  ///< JSON key that changed
    nlohmann::json old_value;   ///< Value in the base document (null if added)
    nlohmann::json new_value;   ///< Value in the target document (null if removed)
};

// ─────────────────────────────────────────────────────────────────────────────
// DocumentDiff
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Structured change list between a base and a target document.
 *
 * Computed by IDocumentDiffMerge::diff().  Operating on document IDs ensures
 * that no plaintext content of encrypted entities is exposed.
 */
struct DocumentDiff {
    std::vector<std::string>  added_fields;    ///< Keys present in target but not base
    std::vector<std::string>  removed_fields;  ///< Keys present in base but not target
    std::vector<FieldChange>  modified_fields; ///< Keys present in both, value changed

    bool isEmpty() const noexcept {
        return added_fields.empty() &&
               removed_fields.empty() &&
               modified_fields.empty();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// MergeConflict
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A field modified differently in both branches of a three-way merge.
 */
struct MergeConflict {
    std::string    field_name;   ///< Conflicting JSON key
    nlohmann::json base_value;   ///< Value in the common base
    nlohmann::json ours_value;   ///< Value in the "ours" branch
    nlohmann::json theirs_value; ///< Value in the "theirs" branch
};

// ─────────────────────────────────────────────────────────────────────────────
// MergeStrategy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Strategy applied to resolve the three-way merge.
 */
enum class MergeStrategy {
    OURS_WINS,   ///< On conflict, "ours" value is used
    THEIRS_WINS, ///< On conflict, "theirs" value is used
    FAIL,        ///< Return ERR_DOC_MERGE_CONFLICT if any conflict exists
};

// ─────────────────────────────────────────────────────────────────────────────
// MergeResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Outcome of IDocumentDiffMerge::merge().
 */
struct MergeResult {
    nlohmann::json              merged_body; ///< Resulting merged document body
    std::vector<MergeConflict>  conflicts;   ///< Empty ⟹ clean merge
    MergeStrategy               strategy_applied;
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentDiffMerge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Structural diff and three-way merge interface for document pairs.
 *
 * Operates on document IDs, not raw payloads; implementations resolve IDs
 * against their backing store.  The interface never exposes the plaintext
 * content of encrypted entities.
 *
 * ### Error codes
 *   - ERR_DOC_DIFF_NOT_FOUND    — one or both document IDs not found
 *   - ERR_DOC_MERGE_CONFLICT    — merge() with MergeStrategy::FAIL and conflicts present
 *   - ERR_DOC_INVALID_ARGUMENT  — empty document ID supplied
 */
class IDocumentDiffMerge {
public:
    virtual ~IDocumentDiffMerge() = default;

    /**
     * @brief Compute a field-level diff between two documents.
     *
     * @param collection Collection owning both documents.
     * @param base_id    Document treated as the baseline.
     * @param target_id  Document treated as the changed version.
     *
     * @return ERR_DOC_DIFF_NOT_FOUND  if either document does not exist.
     * @return ERR_DOC_INVALID_ARGUMENT if either id is empty.
     */
    virtual Result<DocumentDiff> diff(const CollectionId& collection,
                                      const DocumentId&   base_id,
                                      const DocumentId&   target_id) const = 0;

    /**
     * @brief Perform a three-way merge.
     *
     * Changes applied to @p base to reach @p ours and @p theirs are merged.
     * Non-conflicting changes are combined; conflicting changes (both branches
     * modified the same field with different values) are listed in
     * MergeResult::conflicts.
     *
     * @param strategy  How to handle conflicts (default: FAIL).
     *
     * @return ERR_DOC_DIFF_NOT_FOUND    if any of the three IDs do not exist.
     * @return ERR_DOC_MERGE_CONFLICT    if strategy == FAIL and conflicts exist.
     * @return ERR_DOC_INVALID_ARGUMENT  if any id is empty.
     */
    virtual Result<MergeResult> merge(
        const CollectionId& collection,
        const DocumentId&   base_id,
        const DocumentId&   ours_id,
        const DocumentId&   theirs_id,
        MergeStrategy       strategy = MergeStrategy::FAIL) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocumentDiffMerge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Reference implementation of IDocumentDiffMerge backed by an
 *        IDocumentStore.
 *
 * Thread-safe.  The store reference must outlive this object.
 */
class InMemoryDocumentDiffMerge final : public IDocumentDiffMerge {
public:
    explicit InMemoryDocumentDiffMerge(IDocumentStore& store)
        : store_(store) {}

    Result<DocumentDiff> diff(const CollectionId& collection,
                               const DocumentId&   base_id,
                               const DocumentId&   target_id) const override
    {
        if (base_id.empty() || target_id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                "document ids must not be empty"));
        }

        auto base_result   = store_.get(collection, base_id);
        auto target_result = store_.get(collection, target_id);

        if (!base_result || !target_result) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND,
                base_id + ", " + target_id));
        }
        const auto& base_opt   = *base_result;
        const auto& target_opt = *target_result;
        if (!base_opt.has_value()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND, base_id));
        }
        if (!target_opt.has_value()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND, target_id));
        }

        return computeDiff(base_opt.value().body,
                           target_opt.value().body);
    }

    Result<MergeResult> merge(
        const CollectionId& collection,
        const DocumentId&   base_id,
        const DocumentId&   ours_id,
        const DocumentId&   theirs_id,
        MergeStrategy       strategy = MergeStrategy::FAIL) const override
    {
        if (base_id.empty() || ours_id.empty() || theirs_id.empty()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                "document ids must not be empty"));
        }

        auto base_res   = store_.get(collection, base_id);
        auto ours_res   = store_.get(collection, ours_id);
        auto theirs_res = store_.get(collection, theirs_id);

        if (!base_res || !ours_res || !theirs_res) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND,
                base_id + ", " + ours_id + ", " + theirs_id));
        }
        const auto& base_opt   = *base_res;
        const auto& ours_opt   = *ours_res;
        const auto& theirs_opt = *theirs_res;
        if (!base_opt.has_value() || !ours_opt.has_value() ||
            !theirs_opt.has_value()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND,
                base_id + ", " + ours_id + ", " + theirs_id));
        }

        const nlohmann::json& base   = base_opt.value().body;
        const nlohmann::json& ours   = ours_opt.value().body;
        const nlohmann::json& theirs = theirs_opt.value().body;

        return computeMerge(base, ours, theirs, strategy);
    }

private:
    // ── diff helpers ──────────────────────────────────────────────────────

    static DocumentDiff computeDiff(const nlohmann::json& base,
                                     const nlohmann::json& target)
    {
        DocumentDiff d;
        if (!base.is_object() || !target.is_object()) {
            return d; // non-object payloads: no field-level diff
        }

        // Fields in base
        for (const auto& [key, val] : base.items()) {
            if (!target.contains(key)) {
                d.removed_fields.push_back(key);
            } else if (target[key] != val) {
                d.modified_fields.push_back({key, val, target[key]});
            }
        }
        // Fields only in target
        for (const auto& [key, val] : target.items()) {
            if (!base.contains(key)) {
                d.added_fields.push_back(key);
            }
        }
        return d;
    }

    // ── merge helpers ─────────────────────────────────────────────────────

    static Result<MergeResult> computeMerge(const nlohmann::json& base,
                                             const nlohmann::json& ours,
                                             const nlohmann::json& theirs,
                                             MergeStrategy         strategy)
    {
        MergeResult result;
        result.strategy_applied = strategy;

        if (!base.is_object() || !ours.is_object() || !theirs.is_object()) {
            // Non-object payloads: just return ours.
            result.merged_body = ours;
            return result;
        }

        nlohmann::json merged = base; // start from common ancestor

        // Collect all keys
        std::vector<std::string> all_keys;
        for (const auto& [k, _] : base.items()) {
          all_keys.push_back(k);
        }
        for (const auto& [k, _] : ours.items()) {
          all_keys.push_back(k);
        }
        for (const auto& [k, _] : theirs.items()) {
          all_keys.push_back(k);
        }
        // Deduplicate
        std::sort(all_keys.begin(), all_keys.end());
        all_keys.erase(std::unique(all_keys.begin(), all_keys.end()),
                       all_keys.end());

        for (const auto& key : all_keys) {
            const bool in_base   = base.contains(key);
            const bool in_ours   = ours.contains(key);
            const bool in_theirs = theirs.contains(key);

            const nlohmann::json base_val   = in_base   ? base[key]   : nlohmann::json{};
            const nlohmann::json ours_val   = in_ours   ? ours[key]   : nlohmann::json{};
            const nlohmann::json theirs_val = in_theirs ? theirs[key] : nlohmann::json{};

            const bool ours_changed   = (in_base != in_ours) ||
                                        (in_base && in_ours && ours_val   != base_val);
            const bool theirs_changed = (in_base != in_theirs) ||
                                        (in_base && in_theirs && theirs_val != base_val);

            if (!ours_changed && !theirs_changed) {
                // No change in either branch: keep base value.
                continue;
            }
            if (ours_changed && !theirs_changed) {
                // Only ours changed: apply ours.
                if (!in_ours) {
                    merged.erase(key);
                } else {
                    merged[key] = ours_val;
                }
            } else if (!ours_changed && theirs_changed) {
                // Only theirs changed: apply theirs.
                if (!in_theirs) {
                    merged.erase(key);
                } else {
                    merged[key] = theirs_val;
                }
            } else {
                // Both changed: conflict.
                if (!in_ours && !in_theirs) {
                    // Both branches deleted the field: clean deletion.
                    merged.erase(key);
                } else if (ours_val == theirs_val) {
                    // Same value on both sides (both added/modified to same value): clean.
                    if (!in_ours) {
                        merged.erase(key);
                    } else {
                        merged[key] = ours_val;
                    }
                } else {
                    result.conflicts.push_back({key, base_val, ours_val, theirs_val});
                    if (strategy == MergeStrategy::OURS_WINS) {
                        merged[key] = ours_val;
                    } else if (strategy == MergeStrategy::THEIRS_WINS) {
                        merged[key] = theirs_val;
                    }
                    // MergeStrategy::FAIL: leave key as base for now; return
                    // error after accumulating all conflicts.
                }
            }
        }

        if (!result.conflicts.empty() && strategy == MergeStrategy::FAIL) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_MERGE_CONFLICT,
                std::to_string(result.conflicts.size()) + " conflict(s)"));
        }

        result.merged_body = merged;
        return result;
    }

    IDocumentStore& store_;
};

} // namespace document
} // namespace themis
