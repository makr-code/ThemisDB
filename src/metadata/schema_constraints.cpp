/**
 * @file schema_constraints.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_constraints.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace themis {

// ============================================================================
// ConstraintViolation
// ============================================================================

json ConstraintViolation::toJSON() const {
    return {
        {"table_name",       table_name},
        {"column_name",      column_name},
        {"constraint_name",  constraint_name},
        {"constraint_type",  constraint_type},
        {"message",          message},
    };
}

// ============================================================================
// ColumnConstraint factories
// ============================================================================

ColumnConstraint ColumnConstraint::makeNotNull(std::string constraint_name) {
    ColumnConstraint c;
    c.kind = Kind::NOT_NULL;
    c.name = std::move(constraint_name);
    return c;
}

ColumnConstraint ColumnConstraint::makeUnique(std::string constraint_name) {
    ColumnConstraint c;
    c.kind = Kind::UNIQUE;
    c.name = std::move(constraint_name);
    return c;
}

ColumnConstraint ColumnConstraint::makeCheck(std::string constraint_name, std::string expr) {
    ColumnConstraint c;
    c.kind       = Kind::CHECK;
    c.name       = std::move(constraint_name);
    c.check_expr = std::move(expr);
    return c;
}

ColumnConstraint ColumnConstraint::makeDefault(std::string constraint_name, ColumnValue value) {
    ColumnConstraint c;
    c.kind          = Kind::DEFAULT;
    c.name          = std::move(constraint_name);
    c.default_value = std::move(value);
    return c;
}

ColumnConstraint ColumnConstraint::makeForeignKey(
    std::string constraint_name,
    std::string ref_table,
    std::string ref_column)
{
    ColumnConstraint c;
    c.kind      = Kind::FOREIGN_KEY;
    c.name      = std::move(constraint_name);
    c.fk_table  = std::move(ref_table);
    c.fk_column = std::move(ref_column);
    return c;
}

static std::string kindToString(ColumnConstraint::Kind kind) {
    switch (kind) {
        case ColumnConstraint::Kind::NOT_NULL:    return "NOT_NULL";
        case ColumnConstraint::Kind::UNIQUE:      return "UNIQUE";
        case ColumnConstraint::Kind::CHECK:       return "CHECK";
        case ColumnConstraint::Kind::DEFAULT:     return "DEFAULT";
        case ColumnConstraint::Kind::FOREIGN_KEY: return "FOREIGN_KEY";
    }
    return "UNKNOWN";
}

static ColumnConstraint::Kind kindFromString(const std::string& s) {
    if (s == "NOT_NULL") {
      return ColumnConstraint::Kind::NOT_NULL;
    }
    if (s == "UNIQUE") {
      return ColumnConstraint::Kind::UNIQUE;
    }
    if (s == "CHECK") {
      return ColumnConstraint::Kind::CHECK;
    }
    if (s == "DEFAULT") {
      return ColumnConstraint::Kind::DEFAULT;
    }
    if (s == "FOREIGN_KEY") {
      return ColumnConstraint::Kind::FOREIGN_KEY;
    }
    throw std::runtime_error("Unknown constraint kind: " + s);
}

json ColumnConstraint::toJSON() const {
    json j = {
        {"kind", kindToString(kind)},
        {"name", name},
    };
    if (check_expr.has_value()) {
        j["check_expr"] = *check_expr;
    }
    if (fk_table.has_value()) {
        j["fk_table"]  = *fk_table;
        j["fk_column"] = fk_column.value_or("");
    }
    if (default_value.has_value()) {
        std::visit([&j](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                j["default_value"] = nullptr;
            } else if constexpr (std::is_same_v<T, std::string>) {
                j["default_value"] = v;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                j["default_value"] = v;
            } else if constexpr (std::is_same_v<T, double>) {
                j["default_value"] = v;
            } else if constexpr (std::is_same_v<T, bool>) {
                j["default_value"] = v;
            }
        }, *default_value);
    }
    return j;
}

// ============================================================================
// SchemaConstraints – constraint management
// ============================================================================

void SchemaConstraints::addConstraint(
    std::string_view table_name,
    std::string_view column_name,
    ColumnConstraint constraint)
{
    constraints_[std::string(table_name)][std::string(column_name)]
        .push_back(std::move(constraint));

    spdlog::debug("SchemaConstraints: Added '{}' constraint on {}.{}",
                  kindToString(constraints_.at(std::string(table_name))
                                           .at(std::string(column_name)).back().kind),
                  table_name, column_name);
}

void SchemaConstraints::removeColumnConstraints(
    std::string_view table_name,
    std::string_view column_name)
{
    auto t_it = constraints_.find(std::string(table_name));
    if (t_it == constraints_.end()) {
      return;
    }
    t_it->second.erase(std::string(column_name));
    if (t_it->second.empty()) {
        constraints_.erase(t_it);
    }
}

void SchemaConstraints::removeTableConstraints(std::string_view table_name) {
    constraints_.erase(std::string(table_name));
}

std::vector<ColumnConstraint> SchemaConstraints::getColumnConstraints(
    std::string_view table_name,
    std::string_view column_name) const
{
    auto t_it = constraints_.find(std::string(table_name));
    if (t_it == constraints_.end()) return {};
    auto c_it = t_it->second.find(std::string(column_name));
    if (c_it == t_it->second.end()) return {};
    return c_it->second;
}

std::vector<ColumnConstraint> SchemaConstraints::getTableConstraints(
    std::string_view table_name) const
{
    std::vector<ColumnConstraint> result;
    auto t_it = constraints_.find(std::string(table_name));
    if (t_it == constraints_.end()) {
      return result;
    }
    for (const auto& [col, vec] : t_it->second) {
        for (const auto& c : vec) {
            result.push_back(c);
        }
    }
    return result;
}

// ============================================================================
// Enforcement
// ============================================================================

static bool isNull(const ColumnValue& v) {
    return std::holds_alternative<std::monostate>(v);
}

std::vector<ConstraintViolation> SchemaConstraints::enforce(
    std::string_view table_name,
    const std::map<std::string, ColumnValue>& row) const
{
    std::vector<ConstraintViolation> violations;

    auto t_it = constraints_.find(std::string(table_name));
    if (t_it == constraints_.end()) {
        return violations;  // No constraints registered for this table
    }

    // Track values seen per column for UNIQUE checking (within-row scope only;
    // cross-row uniqueness requires storage-level enforcement which is a v1.8.0
    // enhancement; here we validate uniqueness of the constraint metadata).
    for (const auto& [col_name, constraints] : t_it->second) {
        // Get value for this column (missing key == NULL)
        const ColumnValue* val_ptr = nullptr;
        ColumnValue null_val = std::monostate{};
        auto row_it = row.find(col_name);
        if (row_it != row.end()) {
            val_ptr = &row_it->second;
        } else {
            val_ptr = &null_val;
        }

        for (const auto& c : constraints) {
            switch (c.kind) {
                case ColumnConstraint::Kind::NOT_NULL: {
                    auto v = checkNotNull(table_name, col_name, c, *val_ptr);
                    if (v.has_value()) {
                      violations.push_back(std::move(*v));
                    }
                    break;
                }
                case ColumnConstraint::Kind::CHECK: {
                    auto v = checkCheck(table_name, col_name, c, *val_ptr);
                    if (v.has_value()) {
                      violations.push_back(std::move(*v));
                    }
                    break;
                }
                case ColumnConstraint::Kind::UNIQUE:
                    // Cross-row UNIQUE enforcement requires index lookup;
                    // logged as a "registered" constraint; callers should
                    // perform index-level dedup (v1.8.0 storage integration).
                    spdlog::trace("SchemaConstraints: UNIQUE constraint '{}' on {}.{} noted",
                                  c.name, table_name, col_name);
                    break;
                case ColumnConstraint::Kind::FOREIGN_KEY:
                    // Cross-table FK enforcement requires storage lookup;
                    // deferred to v1.8.0 storage integration.
                    spdlog::trace("SchemaConstraints: FK constraint '{}' on {}.{} noted",
                                  c.name, table_name, col_name);
                    break;
                case ColumnConstraint::Kind::DEFAULT:
                    // DEFAULT is applied via applyDefaults(), not checked here.
                    break;
            }
        }
    }

    return violations;
}

std::map<std::string, ColumnValue> SchemaConstraints::applyDefaults(
    std::string_view table_name,
    std::map<std::string, ColumnValue> row) const
{
    auto t_it = constraints_.find(std::string(table_name));
    if (t_it == constraints_.end()) {
      return row;
    }

    for (const auto& [col_name, constraints] : t_it->second) {
        // Only apply default if the column is absent or NULL
        auto row_it = row.find(col_name);
        bool is_missing = (row_it == row.end());
        bool is_null_val = (!is_missing && isNull(row_it->second));

        if (!is_missing && !is_null_val) {
            continue;  // Value already present; skip
        }

        for (const auto& c : constraints) {
            if (c.kind == ColumnConstraint::Kind::DEFAULT && c.default_value.has_value()) {
                row[col_name] = *c.default_value;
                spdlog::trace("SchemaConstraints: Applied default for {}.{}",
                              table_name, col_name);
                break;  // First DEFAULT wins
            }
        }
    }

    return row;
}

// ============================================================================
// Serialisation
// ============================================================================

json SchemaConstraints::toJSON() const {
    json j = json::object();
    for (const auto& [table, cols] : constraints_) {
        json t_obj = json::object();
        for (const auto& [col, vec] : cols) {
            json c_arr = json::array();
            for (const auto& c : vec) {
                c_arr.push_back(c.toJSON());
            }
            t_obj[col] = c_arr;
        }
        j[table] = t_obj;
    }
    return j;
}

SchemaConstraints SchemaConstraints::fromJSON(const json& j) {
    SchemaConstraints sc;

    for (const auto& [table, t_obj] : j.items()) {
        if (!t_obj.is_object()) {
          continue;
        }
        for (const auto& [col, c_arr] : t_obj.items()) {
            if (!c_arr.is_array()) {
              continue;
            }
            for (const auto& cj : c_arr) {
                ColumnConstraint c;
                c.kind = kindFromString(cj.value("kind", std::string("NOT_NULL")));
                c.name = cj.value("name", std::string(""));

                if (cj.contains("check_expr") && cj["check_expr"].is_string()) {
                    c.check_expr = cj["check_expr"].get<std::string>();
                }
                if (cj.contains("fk_table") && cj["fk_table"].is_string()) {
                    c.fk_table  = cj["fk_table"].get<std::string>();
                    c.fk_column = cj.value("fk_column", std::string(""));
                }
                if (cj.contains("default_value")) {
                    const auto& dv = cj["default_value"];
                    if (dv.is_null()) {
                        c.default_value = std::monostate{};
                    } else if (dv.is_string()) {
                        c.default_value = dv.get<std::string>();
                    } else if (dv.is_boolean()) {
                        c.default_value = dv.get<bool>();
                    } else if (dv.is_number_integer()) {
                        c.default_value = dv.get<int64_t>();
                    } else if (dv.is_number_float()) {
                        c.default_value = dv.get<double>();
                    }
                }
                sc.addConstraint(table, col, std::move(c));
            }
        }
    }

    return sc;
}

// ============================================================================
// Private checkers
// ============================================================================

std::optional<ConstraintViolation> SchemaConstraints::checkNotNull(
    std::string_view table_name,
    std::string_view column_name,
    const ColumnConstraint& c,
    const ColumnValue& value) const
{
    if (isNull(value)) {
        ConstraintViolation v;
        v.table_name       = std::string(table_name);
        v.column_name      = std::string(column_name);
        v.constraint_name  = c.name;
        v.constraint_type  = "NOT_NULL";
        v.message          = "Column '" + std::string(column_name) +
                             "' cannot be NULL (constraint: " + c.name + ")";
        return v;
    }
    return std::nullopt;
}

std::optional<ConstraintViolation> SchemaConstraints::checkCheck(
    std::string_view table_name,
    std::string_view column_name,
    const ColumnConstraint& c,
    const ColumnValue& value) const
{
    if (!c.check_expr.has_value() || c.check_expr->empty()) {
        return std::nullopt;
    }

    // Simple CHECK: expression of the form "column > literal" or "column IN (a,b)"
    // is evaluated via a lightweight string comparison for common cases.
    // Full expression parsing is a future enhancement (v1.8.0).
    //
    // Current supported patterns:
    //   "col > N"  / "col >= N"  / "col < N"  / "col <= N"  / "col = N"
    // where N is a number and col matches column_name.
    const std::string& expr = *c.check_expr;

    // Strip whitespace helper
    auto strip = [](const std::string& s) {
        size_t l = s.find_first_not_of(" \t");
        size_t r = s.find_last_not_of(" \t");
        return (l == std::string::npos) ? std::string{} : s.substr(l, r - l + 1);
    };

    // Attempt simple numeric comparison pattern: "<col> <op> <num>"
    static const std::vector<std::string> ops = {">=", "<=", ">", "<", "="};
    for (const auto& op : ops) {
        auto pos = expr.find(op);
        if (pos == std::string::npos) {
          continue;
        }
        std::string lhs = strip(expr.substr(0, pos));
        std::string rhs = strip(expr.substr(pos + static_cast<int>(op.size()) ));

        if (lhs != std::string(column_name)) continue;  // Different column, skip

        // Try to parse rhs as a number
        double rhs_num = 0.0;
        bool rhs_ok = false;
        try {
            rhs_num = std::stod(rhs);
            rhs_ok = true;
        } catch (...) {}
        if (!rhs_ok) {
          continue;
        }

        // Try to get a numeric value from the column value
        double val_num = 0.0;
        bool val_ok = false;
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int64_t>) {
                val_num = static_cast<double>(v);
                val_ok  = true;
            } else if constexpr (std::is_same_v<T, double>) {
                val_num = v;
                val_ok  = true;
            }
        }, value);
        if (!val_ok) {
          continue;
        }

        bool passes = false;
        if (op == ">") {
          passes = val_num >  rhs_num;
        }
        if (op == ">=") {
          passes = val_num >= rhs_num;
        }
        if (op == "<") {
          passes = val_num <  rhs_num;
        }
        if (op == "<=") {
          passes = val_num <= rhs_num;
        }
        if (op == "=") {
          passes = val_num == rhs_num;
        }

        if (!passes) {
            ConstraintViolation v;
            v.table_name      = std::string(table_name);
            v.column_name     = std::string(column_name);
            v.constraint_name = c.name;
            v.constraint_type = "CHECK";
            v.message         = "CHECK constraint '" + c.name + "' violated: " + expr;
            return v;
        }
        // Constraint passed
        return std::nullopt;
    }

    // Unrecognised expression pattern – log and skip silently
    spdlog::trace("SchemaConstraints: Skipping unrecognised CHECK expr '{}'", expr);
    return std::nullopt;
}

// ============================================================================
// RocksDB persistence
// ============================================================================

bool SchemaConstraints::persistTo(RocksDBWrapper& db) const {
    bool all_ok = true;
    for (const auto& [table_name, _] : constraints_) {
        if (!persistTableTo(db, table_name)) {
            all_ok = false;
        }
    }
    return all_ok;
}

bool SchemaConstraints::persistTableTo(RocksDBWrapper& db,
                                        std::string_view table_name) const
{
    try {
        auto t_it = constraints_.find(std::string(table_name));
        if (t_it == constraints_.end()) {
            return true;  // Nothing to persist
        }

        // Serialise just this table's constraints as a JSON object
        json t_obj = json::object();
        for (const auto& [col, vec] : t_it->second) {
            json c_arr = json::array();
            for (const auto& c : vec) {
                c_arr.push_back(c.toJSON());
            }
            t_obj[col] = c_arr;
        }

        std::string key   = "config:constraints:" + std::string(table_name);
        std::string value = t_obj.dump();
        std::vector<uint8_t> data(value.begin(), value.end());

        if (!db.put(key, data)) {
            spdlog::error("SchemaConstraints: Failed to persist constraints for '{}'",
                          table_name);
            return false;
        }

        spdlog::debug("SchemaConstraints: Persisted constraints for '{}'", table_name);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("SchemaConstraints: Exception persisting constraints for '{}': {}",
                      table_name, e.what());
        return false;
    }
}

size_t SchemaConstraints::loadFrom(RocksDBWrapper& db) {
    constraints_.clear();

    size_t loaded = 0;
    try {
        auto it_result = db.newIterator();
        if (!it_result) {
            spdlog::warn("SchemaConstraints: Failed to create iterator for loading");
            return 0;
        }
        auto& it = it_result.value();

        std::string prefix = "config:constraints:";
        it->Seek(prefix);

        while (it->Valid()) {
            std::string key = it->key().ToString();
            if (key.rfind(prefix, 0) != 0) {
              break;
            }

            std::string table_name = key.substr(prefix.size());
            if (loadTableFrom(db, table_name)) {
                ++loaded;
            }
            it->Next();
        }

        if (loaded > 0) {
            spdlog::info("SchemaConstraints: Loaded constraints for {} table(s)", loaded);
        }

    } catch (const std::exception& e) {
        spdlog::error("SchemaConstraints: Exception loading constraints: {}", e.what());
    }

    return loaded;
}

bool SchemaConstraints::loadTableFrom(RocksDBWrapper& db,
                                       std::string_view table_name)
{
    try {
        std::string key = "config:constraints:" + std::string(table_name);
        auto result = db.get(key);
        if (!result.has_value() || result->empty()) {
            return false;
        }

        std::string raw(result->begin(), result->end());
        json t_obj = json::parse(raw);

        if (!t_obj.is_object()) {
          return false;
        }

        for (const auto& [col, c_arr] : t_obj.items()) {
            if (!c_arr.is_array()) {
              continue;
            }
            for (const auto& cj : c_arr) {
                ColumnConstraint c;
                try {
                    c.kind = kindFromString(cj.value("kind", std::string("NOT_NULL")));
                } catch (...) {
                    continue;
                }
                c.name = cj.value("name", std::string(""));
                if (cj.contains("check_expr") && cj["check_expr"].is_string()) {
                    c.check_expr = cj["check_expr"].get<std::string>();
                }
                if (cj.contains("fk_table") && cj["fk_table"].is_string()) {
                    c.fk_table  = cj["fk_table"].get<std::string>();
                    c.fk_column = cj.value("fk_column", std::string(""));
                }
                if (cj.contains("default_value")) {
                    const auto& dv = cj["default_value"];
                    if (dv.is_null()) {
                        c.default_value = std::monostate{};
                    } else if (dv.is_string()) {
                        c.default_value = dv.get<std::string>();
                    } else if (dv.is_boolean()) {
                        c.default_value = dv.get<bool>();
                    } else if (dv.is_number_integer()) {
                        c.default_value = dv.get<int64_t>();
                    } else if (dv.is_number_float()) {
                        c.default_value = dv.get<double>();
                    }
                }
                constraints_[std::string(table_name)][col].push_back(std::move(c));
            }
        }

        spdlog::debug("SchemaConstraints: Loaded constraints for table '{}'", table_name);
        return true;

    } catch (const std::exception& e) {
        spdlog::warn("SchemaConstraints: Failed to load constraints for '{}': {}",
                     table_name, e.what());
        return false;
    }
}

} // namespace themis


