/**
 * @file schema_diff.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "schema_manager.h"

namespace themis {
namespace metadata {

using json = nlohmann::json;

// ── ColumnDiffType ─────────────────────────────────────────────────────────────

/**
 * @brief Classifies a column-level schema difference.
 */
enum class ColumnDiffType {
    ADDED,                ///< Column was added in the new schema
    REMOVED,              ///< Column was removed in the new schema
    TYPE_CHANGED,         ///< Column data type changed
    NULLABILITY_CHANGED,  ///< Column nullability changed
    INDEX_CHANGED,        ///< Column index presence or type changed
};

// ── ColumnDiff ─────────────────────────────────────────────────────────────────

/**
 * @brief Describes a single column-level change between two schema versions.
 */
struct ColumnDiff {
    ColumnDiffType           diff_type;   ///< Nature of the change
    std::string              column_name; ///< Affected column name
    std::optional<std::string> old_value; ///< Previous value (empty for ADDED)
    std::optional<std::string> new_value; ///< New value (empty for REMOVED)

    /**
     * @brief Serialise this diff to a JSON object.
     *
     * Schema:
     * @code
     * {
     *   "diff_type":   "ADDED" | "REMOVED" | "TYPE_CHANGED" | ...,
     *   "column_name": "<name>",
     *   "old_value":   "<string>" | null,
     *   "new_value":   "<string>" | null
     * }
     * @endcode
     */
    json toJSON() const {
        static const char* kTypeNames[] = {
            "ADDED", "REMOVED", "TYPE_CHANGED",
            "NULLABILITY_CHANGED", "INDEX_CHANGED"
        };
        json j;
        j["diff_type"]   = kTypeNames[static_cast<int>(diff_type)];
        j["column_name"] = column_name;
        j["old_value"]   = old_value.has_value()
                               ? json(old_value.value())
                               : json(nullptr);
        j["new_value"]   = new_value.has_value()
                               ? json(new_value.value())
                               : json(nullptr);
        return j;
    }
};

// ── IndexDiffType ──────────────────────────────────────────────────────────────

/**
 * @brief Classifies an index-level schema difference.
 */
enum class IndexDiffType {
    ADDED,   ///< Index was added in the new schema
    REMOVED, ///< Index was removed in the new schema
    CHANGED, ///< Index with the same name has different attributes
};

// ── IndexDiff ──────────────────────────────────────────────────────────────────

/**
 * @brief Describes a single index-level change between two schema versions.
 */
struct IndexDiff {
    IndexDiffType diff_type;  ///< Nature of the change
    std::string   index_name; ///< Affected index name

    /**
     * @brief Serialise this diff to a JSON object.
     *
     * Schema:
     * @code
     * {
     *   "diff_type":  "ADDED" | "REMOVED" | "CHANGED",
     *   "index_name": "<name>"
     * }
     * @endcode
     */
    json toJSON() const {
        static const char* kTypeNames[] = { "ADDED", "REMOVED", "CHANGED" };
        json j;
        j["diff_type"]  = kTypeNames[static_cast<int>(diff_type)];
        j["index_name"] = index_name;
        return j;
    }
};

// ── SchemaDiff ─────────────────────────────────────────────────────────────────

/**
 * @brief Aggregated diff result for a single table.
 *
 * Both @c column_diffs and @c index_diffs are sorted by their respective
 * name fields to guarantee deterministic output regardless of the order in
 * which properties/indexes appear in the source schemas.
 */
struct SchemaDiff {
    std::string            table_name;   ///< Name of the diffed table
    std::vector<ColumnDiff> column_diffs; ///< All column-level changes
    std::vector<IndexDiff>  index_diffs;  ///< All index-level changes

    /** @return true if no column or index changes were found. */
    bool isEmpty() const {
        return column_diffs.empty() && index_diffs.empty();
    }

    /** @return Number of columns added in the new schema. */
    size_t addedColumnCount() const {
        return static_cast<size_t>(
            std::count_if(column_diffs.begin(), column_diffs.end(),
                [](const ColumnDiff& d) {
                    return d.diff_type == ColumnDiffType::ADDED;
                }));
    }

    /** @return Number of columns removed in the new schema. */
    size_t removedColumnCount() const {
        return static_cast<size_t>(
            std::count_if(column_diffs.begin(), column_diffs.end(),
                [](const ColumnDiff& d) {
                    return d.diff_type == ColumnDiffType::REMOVED;
                }));
    }

    /**
     * @return Number of columns that exist in both schemas but whose
     *         attributes (type, nullability, index) have changed.
     */
    size_t modifiedColumnCount() const {
        return static_cast<size_t>(
            std::count_if(column_diffs.begin(), column_diffs.end(),
                [](const ColumnDiff& d) {
                    return d.diff_type != ColumnDiffType::ADDED &&
                           d.diff_type != ColumnDiffType::REMOVED;
                }));
    }

    /**
     * @brief Serialise the full diff to JSON.
     *
     * Schema:
     * @code
     * {
     *   "table_name":    "<name>",
     *   "column_diffs":  [ ... ],
     *   "index_diffs":   [ ... ],
     *   "summary": {
     *     "added_columns":    <n>,
     *     "removed_columns":  <n>,
     *     "modified_columns": <n>,
     *     "index_changes":    <n>
     *   }
     * }
     * @endcode
     */
    json toJSON() const {
        json j;
        j["table_name"] = table_name;
        j["column_diffs"] = json::array();
        for (const auto& cd : column_diffs) {
            j["column_diffs"].push_back(cd.toJSON());
        }
        j["index_diffs"] = json::array();
        for (const auto& id : index_diffs) {
            j["index_diffs"].push_back(id.toJSON());
        }
        j["summary"] = {
            {"added_columns",    addedColumnCount()},
            {"removed_columns",  removedColumnCount()},
            {"modified_columns", modifiedColumnCount()},
            {"index_changes",    index_diffs.size()}
        };
        return j;
    }
};

// ── SchemaDiffEngine ───────────────────────────────────────────────────────────

/**
 * @brief Computes structural diffs between two TableSchema instances.
 *
 * The engine is stateless; create a single instance and call diff() as many
 * times as needed.  All comparisons are purely value-based — no database
 * connection is required.
 *
 * Algorithm:
 *  1. Build name-keyed maps of PropertyInfo and IndexInfo for both schemas.
 *  2. Walk the union of property names to classify each as ADDED / REMOVED /
 *     TYPE_CHANGED / NULLABILITY_CHANGED / INDEX_CHANGED.  A property may
 *     produce multiple ColumnDiff entries when more than one attribute changed.
 *  3. Walk the union of index names to classify each as ADDED / REMOVED /
 *     CHANGED (type, uniqueness, or column list differs).
 *  4. Sort both result lists alphabetically by name for deterministic output.
 *
 * Example:
 * @code
 *   SchemaDiffEngine engine;
 *   auto diff = engine.diff(old_schema, new_schema);
 *   if (!diff.isEmpty()) {
 *       std::cout << diff.toJSON().dump(2) << "\n";
 *   }
 * @endcode
 */
class SchemaDiffEngine {
public:
    /**
     * @brief Compute the structural diff between @p from and @p to.
     *
     * @param from  The baseline (old) schema.
     * @param to    The target (new) schema.
     * @return      A SchemaDiff describing every detected change.
     *              Returns an empty diff when the schemas are identical.
     */
    SchemaDiff diff(const SchemaManager::TableSchema& from,
                    const SchemaManager::TableSchema& to) const {
        SchemaDiff result;
        result.table_name = to.name.empty() ? from.name : to.name;

        // ── Column diffs ──────────────────────────────────────────────────────
        std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
        std::map<std::string, const SchemaManager::PropertyInfo*> to_props;

        for (const auto& p : from.properties) {
          from_props[p.name] = &p;
        }
        for (const auto& p : to.properties) {
          to_props[p.name]   = &p;
        }

        // Collect the union of all property names.
        std::set<std::string> all_prop_names = {};

        for (const auto& kv : from_props) {
          all_prop_names.insert(kv.first);
        }
        for (const auto& kv : to_props) {
          all_prop_names.insert(kv.first);
        }

        for (const auto& name : all_prop_names) {
            auto f_it = from_props.find(name);
            auto t_it = to_props.find(name);

            if (f_it == from_props.end()) {
                // Column exists only in the new schema → ADDED
                result.column_diffs.push_back({
                    ColumnDiffType::ADDED, name, std::nullopt,
                    std::optional<std::string>(t_it->second->type)
                });
            } else if (t_it == to_props.end()) {
                // Column exists only in the old schema → REMOVED
                result.column_diffs.push_back({
                    ColumnDiffType::REMOVED, name,
                    std::optional<std::string>(f_it->second->type),
                    std::nullopt
                });
            } else {
                const auto& fp = *f_it->second;
                const auto& tp = *t_it->second;

                if (fp.type != tp.type) {
                    result.column_diffs.push_back({
                        ColumnDiffType::TYPE_CHANGED, name,
                        std::optional<std::string>(fp.type),
                        std::optional<std::string>(tp.type)
                    });
                }
                if (fp.nullable != tp.nullable) {
                    result.column_diffs.push_back({
                        ColumnDiffType::NULLABILITY_CHANGED, name,
                        std::optional<std::string>(fp.nullable ? "true" : "false"),
                        std::optional<std::string>(tp.nullable ? "true" : "false")
                    });
                }
                if (fp.indexed != tp.indexed || fp.index_type != tp.index_type) {
                    result.column_diffs.push_back({
                        ColumnDiffType::INDEX_CHANGED, name,
                        std::optional<std::string>(
                            fp.indexed ? fp.index_type : "none"),
                        std::optional<std::string>(
                            tp.indexed ? tp.index_type : "none")
                    });
                }
            }
        }

        // ── Index diffs ───────────────────────────────────────────────────────
        std::map<std::string, const SchemaManager::IndexInfo*> from_idxs;
        std::map<std::string, const SchemaManager::IndexInfo*> to_idxs;

        for (const auto& idx : from.indexes) {
          from_idxs[idx.name] = &idx;
        }
        for (const auto& idx : to.indexes) {
          to_idxs[idx.name]   = &idx;
        }

        std::set<std::string> all_idx_names = {};

        for (const auto& kv : from_idxs) {
          all_idx_names.insert(kv.first);
        }
        for (const auto& kv : to_idxs) {
          all_idx_names.insert(kv.first);
        }

        for (const auto& name : all_idx_names) {
            auto f_it = from_idxs.find(name);
            auto t_it = to_idxs.find(name);

            if (f_it == from_idxs.end()) {
                result.index_diffs.push_back({IndexDiffType::ADDED, name});
            } else if (t_it == to_idxs.end()) {
                result.index_diffs.push_back({IndexDiffType::REMOVED, name});
            } else {
                const auto& fi = *f_it->second;
                const auto& ti = *t_it->second;
                if (fi.type != ti.type ||
                    fi.unique != ti.unique ||
                    fi.columns != ti.columns) {
                    result.index_diffs.push_back({IndexDiffType::CHANGED, name});
                }
            }
        }

        // ── Sort for deterministic output ─────────────────────────────────────
        std::sort(result.column_diffs.begin(), result.column_diffs.end(),
            [](const ColumnDiff& a, const ColumnDiff& b) {
                return a.column_name < b.column_name;
            });
        std::sort(result.index_diffs.begin(), result.index_diffs.end(),
            [](const IndexDiff& a, const IndexDiff& b) {
                return a.index_name < b.index_name;
            });

        return result;
    }
};

} // namespace metadata
} // namespace themis

