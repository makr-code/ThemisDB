/**
 * @file relationship_mapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

class RelationshipMapper {
public:
    struct RelationshipMapping {
        std::string edge_type;          ///< e.g. "orders_references_users"
        std::string source_table = {};
        std::string source_column;      ///< comma-joined for composite FKs
        std::string target_table = {};
        std::string target_column;      ///< comma-joined for composite FKs
        std::string cardinality;        ///< ONE_TO_ONE | MANY_TO_ONE | MANY_TO_MANY
        std::string on_delete_action;   ///< CASCADE | SET NULL | RESTRICT | NO ACTION | SET DEFAULT
        std::string on_update_action;   ///< CASCADE | SET NULL | RESTRICT | NO ACTION | SET DEFAULT
        bool is_self_referential = false; ///< source_table == target_table

        json toThemisEdge() const {
            return json{
                {"_type",              edge_type},
                {"_from",              source_table + "/" + source_column},
                {"_to",               target_table + "/" + target_column},
                {"cardinality",        cardinality},
                {"source_table",       source_table},
                {"target_table",       target_table},
                {"on_delete",          on_delete_action},
                {"on_update",          on_update_action},
                {"is_self_referential", is_self_referential}
            };
        }

        json toJson() const { return toThemisEdge(); }
    };

    /**
     * @brief Generate Inverse Edges.
     * @param[in] mappings Input parameter.
     * @return Return value.
     * @details Calls: push_back(), std::move().
     */
    static std::vector<RelationshipMapping> generateInverseEdges(
            const std::vector<RelationshipMapping>& mappings) {

        std::vector<RelationshipMapping> inverse = {};

        for (const auto& m : mappings) {
            if (m.cardinality != "MANY_TO_ONE") {
              continue;
            }
            if (m.is_self_referential) {
              continue;
            }

            RelationshipMapping inv;
            inv.source_table      = m.target_table;
            inv.source_column     = m.target_column;
            inv.target_table      = m.source_table;
            inv.target_column     = m.source_column;
            inv.cardinality       = "ONE_TO_MANY";
            inv.on_delete_action  = m.on_delete_action;
            inv.on_update_action  = m.on_update_action;
            inv.is_self_referential = false;
            inv.edge_type = m.target_table + "_has_many_" + m.source_table;
            inverse.push_back(std::move(inv));
        }
        return inverse;
    }

    template <typename TableSchemaMap>
    static std::vector<RelationshipMapping> mapFromForeignKeys(
            const TableSchemaMap& schemas,
            const std::string& mode = "auto") {

        std::vector<RelationshipMapping> result = {};

        if (mode != "auto") {
          return result;
        }

        for (const auto& [tname, tschema] : schemas) {
            for (const auto& fk : tschema.foreign_keys) {
                if (fk.target_table.empty()) {
                  continue;
                }

                RelationshipMapping m;
                m.source_table       = tname;
                m.source_column      = fk.source_column;
                m.target_table       = fk.target_table;
                m.target_column      = fk.target_column;
                m.on_delete_action   = fk.on_delete_action;
                m.on_update_action   = fk.on_update_action;
                m.is_self_referential = (tname == fk.target_table);

                // Edge type: use FK name if set; for self-referential use the
                // FK column as a relationship hint; otherwise <src>_references_<tgt>
                if (!fk.name.empty()) {
                    m.edge_type = fk.name;
                } else if (m.is_self_referential) {
                    // e.g. employees.manager_id → employees.id  → "employee_manages_employee"
                    std::string col_hint = fk.source_column;
                    // Strip common FK suffix patterns: _id, _fk, _ref
                    for (const auto& sfx : {"_id", "_fk", "_ref"}) {
                        if (col_hint.size() > std::string(sfx).size() &&
                            col_hint.compare(col_hint.size() - std::string(sfx).size(),
                                             std::string(sfx).size(), sfx) == 0) {
                            col_hint = col_hint.substr(0, col_hint.size() - std::string(sfx).size());
                            break;
                        }
                    }
                    m.edge_type = tname + "_" + col_hint + "_" + tname;
                } else {
                    m.edge_type = tname + "_references_" + fk.target_table;
                }

                // Cardinality detection
                m.cardinality = detectCardinalityImpl(tschema, fk, schemas);

                result.push_back(std::move(m));
            }
        }
        return result;
    }

    template <typename TableSchemaMap>
    /**
     * @brief Validate Mappings.
     * @param[in] mappings Input parameter.
     * @param[in] schemas Input parameter.
     * @param[in,out] errors Input/output parameter.
     * @return True when the operation succeeds.
     * @details Calls: count(), push_back(), at(), splitColumns(), std::find(), begin(), end().
     */
    static bool validateMappings(
            const std::vector<RelationshipMapping>& mappings,
            const TableSchemaMap& schemas,
            std::vector<std::string>& errors) {

        bool ok = true;
        for (const auto& m : mappings) {
            if (!schemas.count(m.source_table)) {
                errors.push_back("Relationship source table not found: " + m.source_table);
                ok = false;
                continue;
            }
            if (!schemas.count(m.target_table)) {
                errors.push_back("Relationship target table not found: " + m.target_table);
                ok = false;
                continue;
            }
            // Validate target column exists
            const auto& target = schemas.at(m.target_table);
            auto target_cols = splitColumns(m.target_column);
            for (const auto& col : target_cols) {
                if (std::find(target.columns.begin(), target.columns.end(), col)
                        == target.columns.end()) {
                    errors.push_back("Target column '" + col + "' not found in table '"
                                     + m.target_table + "' (edge: " + m.edge_type + ")");
                    ok = false;
                }
            }
        }
        return ok;
    }

    template <typename TableSchemaMap>
    /**
     * @brief Detect Circular References.
     * @param[in] schemas Input parameter.
     * @param[in,out] cycles Input/output parameter.
     * @return True when the operation succeeds.
     * @details Calls: empty(), insert(), count(), dfsCycle().
     */
    static bool detectCircularReferences(
            const TableSchemaMap& schemas,
            std::vector<std::string>& cycles) {

        // Build adjacency list: table → set of tables it references via FK
        std::map<std::string, std::set<std::string>> adj;
        for (const auto& [tname, tschema] : schemas) {
            for (const auto& fk : tschema.foreign_keys) {
                if (!fk.target_table.empty() && fk.target_table != tname) {
                    adj[tname].insert(fk.target_table);
                }
            }
        }

        std::set<std::string> visited;
        std::set<std::string> in_stack;
        std::vector<std::string> path;

        bool found_cycle = false;
        for (const auto& [tname, _] : schemas) {
            if (!visited.count(tname)) {
                dfsCycle(tname, adj, visited, in_stack, path, cycles, found_cycle);
            }
        }
        return found_cycle;
    }

    /**
     * @brief ------------------------------------------------------------------------- Helpers (public for unit-testing) -------------------------------------------------------------------------
     * @param[in] cols Input parameter.
     * @return Return value.
     * @details Calls: trimStr(), empty(), push_back(), clear().
     */

    static std::vector<std::string> splitColumns(const std::string& cols) {
        std::vector<std::string> result;
        std::string cur = {};
        for (char c : cols) {
            if (c == ',') {
                auto s = trimStr(cur);
                if (!s.empty()) {
                  result.push_back(s);
                }
                cur.clear();
            } else {
                cur += c;
            }
        }
        auto s = trimStr(cur);
        if (!s.empty()) {
          result.push_back(s);
        }
        return result;
    }

private:
    /**
     * @brief Trim Str.
     * @param[in] s Input parameter.
     * @return Return value.
     * @details Calls: find_first_not_of(), find_last_not_of(), substr().
     */
    static std::string trimStr(const std::string& s) {
        size_t l = s.find_first_not_of(" \t\r\n");
        size_t r = s.find_last_not_of(" \t\r\n");
        return (l == std::string::npos) ? "" : s.substr(l, r - l + 1);
    }

    template <typename SourceSchema, typename FKConstraint, typename TableSchemaMap>
    /**
     * @brief Detect Cardinality Impl.
     * @param[in] source Input parameter.
     * @param[in] fk Input parameter.
     * @param[in] schemas Input parameter.
     * @return Return value.
     * @details Calls: splitColumns(), empty(), size(), std::find(), begin(), end(), find().
     */
    static std::string detectCardinalityImpl(
            const SourceSchema& source,
            const FKConstraint& fk,
            const TableSchemaMap& schemas) {

        const auto& pks = source.primary_keys;
        auto src_cols = splitColumns(fk.source_column);

        // If source columns ARE exactly the primary key(s) → ONE_TO_ONE
        if (!pks.empty() && pks.size() == src_cols.size()) {
            bool is_pk = true;
            for (const auto& sc : src_cols) {
                if (std::find(pks.begin(), pks.end(), sc) == pks.end()) {
                    is_pk = false;
                    break;
                }
            }
            if (is_pk) {
              return "ONE_TO_ONE";
            }
        }

        // Check if the target column is a primary key in the target table.
        // If NOT → MANY_TO_MANY; if YES → MANY_TO_ONE (default)
        auto it = schemas.find(fk.target_table);
        if (it != schemas.end()) {
            const auto& target = it->second;
            auto tgt_cols = splitColumns(fk.target_column);
            const auto& tpks = target.primary_keys;

            bool tgt_is_pk = (!tpks.empty() && tpks.size() == tgt_cols.size());
            if (tgt_is_pk) {
                for (const auto& tc : tgt_cols) {
                    if (std::find(tpks.begin(), tpks.end(), tc) == tpks.end()) {
                        tgt_is_pk = false;
                        break;
                    }
                }
            }
            if (!tgt_is_pk) {
              return "MANY_TO_MANY";
            }
        }

        return "MANY_TO_ONE";
    }

    static void dfsCycle(
            const std::string& node,
            const std::map<std::string, std::set<std::string>>& adj,
            std::set<std::string>& visited,
            std::set<std::string>& in_stack,
            std::vector<std::string>& path,
            std::vector<std::string>& cycles,
            bool& found_cycle) {

        visited.insert(node);
        in_stack.insert(node);
        path.push_back(node);

        auto it = adj.find(node);
        if (it != adj.end()) {
            for (const auto& neighbour : it->second) {
                if (!visited.count(neighbour)) {
                    dfsCycle(neighbour, adj, visited, in_stack, path,
                             cycles, found_cycle);
                } else if (in_stack.count(neighbour)) {
                    // Found a cycle – reconstruct the chain from the cycle start
                    found_cycle = true;
                    std::string chain = {};
                    bool in_cycle = false;
                    for (const auto& n : path) {
                        if (n == neighbour) {
                          in_cycle = true;
                        }
                        if (in_cycle) {
                            if (!chain.empty()) {
                              chain += " -> ";
                            }
                            chain += n;
                        }
                    }
                    chain += " -> " + neighbour;
                    cycles.push_back(chain);
                }
            }
        }

        path.pop_back();
        in_stack.erase(node);
    }
};

} // namespace importers
} // namespace themis
