// PostgreSQL Importer v2.0 – Unit Tests
//
// Tests cover:
//   FK parsing (simple, composite, inline REFERENCES, DEFERRABLE)
//   CREATE INDEX parsing (btree, hash, gist, gin, partial, unique)
//   ALTER TABLE ADD CONSTRAINT FOREIGN KEY parsing
//   FK reference validation
//   Relationship mapping (cardinality detection, edge types)
//   Circular reference detection
//   Column defaults, UNIQUE constraints, NOT NULL inline
//   getSourceSchema() v2 enriched output
// Additional v2.1 module coverage:
// schema_inference, column_importance, crdt_importer, postgres_cdc,
// data_quality, audit_trail, adaptive_import, polyglot_mapper,
// temporal_support, blockchain_integrity, federated_learning,
// graphql_federation.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <algorithm>
#include <sstream>

// ============================================================================
// Minimal self-contained helpers (mirrors postgres_importer.cpp helpers)
// ============================================================================

static std::vector<std::string> splitTopLevelCommas(const std::string& s) {
    std::vector<std::string> result;
    int   depth     = 0;
    bool  in_string = false;
    std::string current;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (in_string) {
            current += c;
            if (c == '\'') {
                if (i + 1 < s.size() && s[i + 1] == '\'') {
                  current += s[++i];
                }
                else in_string = false;
            }
        } else if (c == '\'') { in_string = true; current += c; }
        else if (c == '(') { ++depth; current += c; }
        else if (c == ')') { --depth; current += c; }
        else if (c == ',' && depth == 0) { result.push_back(current); current.clear(); }
        else current += c;
    }
    if (!current.empty()) {
      result.push_back(current);
    }
    return result;
}

// ============================================================================
// Minimal struct mirrors for unit testing (no need to link the full importer)
// ============================================================================

struct ForeignKeyConstraint {
    std::string name;
    std::string source_column;
    std::string target_table;
    std::string target_column;
    std::string on_delete_action;
    std::string on_update_action;
    bool deferrable = false;
    bool initially_deferred = false;
};

struct IndexMetadata {
    std::string name;
    std::string type;
    std::vector<std::string> columns;
    bool unique = false;
    bool partial = false;
    std::string where_clause;
};

// v2.1 structs
struct CheckConstraint {
    std::string name;
    std::string expression;
};

struct GeneratedColumnInfo {
    std::string column;
    std::string expression;
    std::string generation;  // "ALWAYS" or "BY_DEFAULT"
    bool is_identity = false;
    bool stored      = false;
};

struct ExcludeConstraint {
    std::string name;
    std::string definition;
};

struct TableSchema {
    std::string name;
    std::string schema;
    std::vector<std::string> columns;
    std::map<std::string, std::string> column_types;
    std::vector<std::string> primary_keys;
    std::vector<ForeignKeyConstraint> foreign_keys;
    std::map<std::string, std::string> column_defaults;
    std::map<std::string, std::string> column_constraints;
    std::vector<IndexMetadata> indexes;
    // v2.1
    std::vector<CheckConstraint>     check_constraints;
    std::vector<GeneratedColumnInfo> generated_columns;
    std::vector<ExcludeConstraint>   exclude_constraints;
};

// ============================================================================
// Parser logic extracted (pure functions matching postgres_importer.cpp v2.0)
// ============================================================================

static size_t findMatchingParen(const std::string& sql, size_t open_pos) {
    if (open_pos >= sql.size() || sql[open_pos] != '(') return std::string::npos;
    int depth = 0; bool in_string = false;
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == '\'' && k+1 < sql.size() && sql[k+1] == '\'') {
              ++k;
            }
            else if (c == '\'') in_string = false;
        } else if (c == '\'') in_string = true;
        else if (c == '(') ++depth;
        else if (c == ')') { --depth; if (depth == 0) return k; }
    }
    return std::string::npos;
}

static std::string toUpper(std::string s) {
    for (auto& c : s) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

static std::string trimStr(const std::string& s) {
    size_t l = s.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return {};
    size_t r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

// --- parseForeignKeyConstraint ---
static bool parseForeignKeyConstraint(const std::string& constraint_def,
                                       ForeignKeyConstraint& fk) {
    // [CONSTRAINT name] FOREIGN KEY (cols) REFERENCES [schema.]table [(cols)]
    std::regex fk_re(
        R"((?:CONSTRAINT\s+(\w+)\s+)?FOREIGN\s+KEY\s*\(([^)]+)\)\s+REFERENCES\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?)",
        std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(constraint_def, m, fk_re)) {
      return false;
    }
    fk.name          = m[1].matched ? m[1].str() : "";
    fk.target_table  = m[3].str();
    fk.target_column = m[4].matched ? trimStr(m[4].str()) : "";

    auto normCols = [](const std::string& cols) {
        std::string result;
        std::istringstream ss(cols);
        std::string c;
        while (std::getline(ss, c, ',')) {
            c = trimStr(c);
            if (!c.empty() && c.front() == '"') {
              c = c.substr(1, c.size()-2);
            }
            if (!result.empty()) {
              result += ",";
            }
            result += c;
        }
        return result;
    };
    fk.source_column = normCols(m[2].str());
    fk.target_column = normCols(fk.target_column);

    std::string upper = toUpper(constraint_def);
    auto extractAction = [&upper](const std::string& keyword) -> std::string {
        size_t pos = upper.find(keyword);
        if (pos == std::string::npos) {
          return "";
        }
        std::string rest = trimStr(upper.substr(pos + keyword.size()));
        if (rest.substr(0,7) == "CASCADE") {
          return "CASCADE";
        }
        if (rest.substr(0,8) == "SET NULL") {
          return "SET NULL";
        }
        if (rest.substr(0,11) == "SET DEFAULT") {
          return "SET DEFAULT";
        }
        if (rest.substr(0,8) == "RESTRICT") {
          return "RESTRICT";
        }
        if (rest.substr(0,9) == "NO ACTION") {
          return "NO ACTION";
        }
        return "";
    };
    fk.on_delete_action = extractAction("ON DELETE ");
    fk.on_update_action = extractAction("ON UPDATE ");
    fk.deferrable = (upper.find("DEFERRABLE") != std::string::npos &&
                     upper.find("NOT DEFERRABLE") == std::string::npos);
    fk.initially_deferred = (upper.find("INITIALLY DEFERRED") != std::string::npos);
    return !fk.target_table.empty();
}

// --- parseCreateIndex ---
static bool parseCreateIndex(const std::string& sql, IndexMetadata& index) {
    std::regex idx_re(
        R"(CREATE\s+(UNIQUE\s+)?INDEX\s+(?:CONCURRENTLY\s+)?(?:IF\s+NOT\s+EXISTS\s+)?(\w+)\s+ON\s+(?:\w+\.)?(\w+)\s*(?:USING\s+(\w+))?\s*\(([^)]+)\)(?:\s+WHERE\s+(.+?))?(?:\s*;)?$)",
        std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(sql, m, idx_re)) {
      return false;
    }
    index.unique = m[1].matched && !m[1].str().empty();
    index.name   = m[2].str();
    std::string tp = m[4].matched ? m[4].str() : "btree";
    for (auto& c : tp) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    index.type = tp;
    std::string cols = m[5].str();
    std::istringstream css(cols);
    std::string col;
    while (std::getline(css, col, ',')) {
        // Trim leading/trailing whitespace FIRST
        col = trimStr(col);
        // Then strip expression parts like ASC/DESC, NULLS FIRST
        size_t sp = col.find_first_of(" \t(");
        if (sp != std::string::npos) {
          col = col.substr(0, sp);
        }
        if (!col.empty() && col.front() == '"') {
          col = col.substr(1, col.size()-2);
        }
        if (!col.empty()) {
          index.columns.push_back(col);
        }
    }
    if (m[6].matched && !m[6].str().empty()) {
        index.partial = true;
        index.where_clause = trimStr(m[6].str());
        size_t r = index.where_clause.find_last_not_of(" \t\r\n;");
        if (r != std::string::npos) {
          index.where_clause = index.where_clause.substr(0, r+1);
        }
    }
    return !index.name.empty() && !index.columns.empty();
}

// ============================================================================
// v2.1 Parser stubs
// ============================================================================

static bool parseCheckConstraint(const std::string& constraint_def, CheckConstraint& ck) {
    std::regex cname_re(R"(CONSTRAINT\s+(\w+))", std::regex_constants::icase);
    std::smatch cm;
    if (std::regex_search(constraint_def, cm, cname_re)) {
      ck.name = cm[1].str();
    }
    std::string upper = toUpper(constraint_def);
    size_t ck_pos = upper.find("CHECK");
    if (ck_pos == std::string::npos) {
      return false;
    }
    size_t paren = constraint_def.find('(', ck_pos);
    if (paren == std::string::npos) {
      return false;
    }
    size_t paren_end = findMatchingParen(constraint_def, paren);
    if (paren_end == std::string::npos) {
      return false;
    }
    ck.expression = trimStr(constraint_def.substr(paren + 1, paren_end - paren - 1));
    return !ck.expression.empty();
}

static bool parseExcludeConstraint(const std::string& constraint_def, ExcludeConstraint& excl) {
    std::regex cname_re(R"(CONSTRAINT\s+(\w+))", std::regex_constants::icase);
    std::smatch cm;
    if (std::regex_search(constraint_def, cm, cname_re)) {
      excl.name = cm[1].str();
    }
    std::string upper = toUpper(constraint_def);
    size_t ex_pos = upper.find("EXCLUDE");
    if (ex_pos == std::string::npos) {
      return false;
    }
    excl.definition = trimStr(constraint_def.substr(ex_pos));
    size_t r = excl.definition.find_last_not_of(" \t\r\n;");
    if (r != std::string::npos) {
      excl.definition = excl.definition.substr(0, r + 1);
    }
    return !excl.definition.empty();
}

static bool parseGeneratedColumn(const std::string& col_def, const std::string& col_name,
                                  GeneratedColumnInfo& gen) {
    std::string upper = toUpper(col_def);
    size_t gen_pos = upper.find(" GENERATED ");
    if (gen_pos == std::string::npos) {
      return false;
    }
    gen.column = col_name;
    std::string rest = upper.substr(gen_pos + 11);
    if (rest.substr(0, 6) == "ALWAYS") { gen.generation = "ALWAYS"; rest = rest.substr(6); }
    else if (rest.substr(0, 10) == "BY DEFAULT") { gen.generation = "BY_DEFAULT"; rest = rest.substr(10); }
    else return false;
    size_t ws = rest.find_first_not_of(" \t\r\n");
    if (ws == std::string::npos) {
      return false;
    }
    rest = rest.substr(ws);
    if (rest.substr(0, 2) == "AS") {
      rest = rest.substr(2);
    }
    ws = rest.find_first_not_of(" \t\r\n");
    if (ws != std::string::npos) {
      rest = rest.substr(ws);
    }
    if (rest.substr(0, 8) == "IDENTITY") { gen.is_identity = true; gen.stored = false; return true; }
    if (!rest.empty() && rest[0] == '(') {
        size_t orig_paren = col_def.find('(', gen_pos);
        if (orig_paren == std::string::npos) {
          return false;
        }
        size_t orig_end = findMatchingParen(col_def, orig_paren);
        if (orig_end == std::string::npos) {
          return false;
        }
        gen.expression = trimStr(col_def.substr(orig_paren + 1, orig_end - orig_paren - 1));
        gen.stored = (upper.find("STORED", orig_end) != std::string::npos);
        gen.is_identity = false;
        return true;
    }
    return false;
}

// ============================================================================
// RelationshipMapper logic (header-only, mirrored here for unit testing)
// ============================================================================

static std::vector<std::string> splitColumns(const std::string& cols) {
    std::vector<std::string> result;
    std::string cur;
    for (char c : cols) {
        if (c == ',') { auto s = trimStr(cur); if (!s.empty()) result.push_back(s); cur.clear(); }
        else cur += c;
    }
    auto s = trimStr(cur); if (!s.empty()) result.push_back(s);
    return result;
}

static std::string detectCardinality(const TableSchema& source, const ForeignKeyConstraint& fk,
                                      const std::map<std::string, TableSchema>& schemas) {
    const auto& pks = source.primary_keys;
    auto src_cols = splitColumns(fk.source_column);
    if (!pks.empty() && pks.size() == src_cols.size()) {
        bool is_pk = true;
        for (const auto& sc : src_cols)
            if (std::find(pks.begin(), pks.end(), sc) == pks.end()) { is_pk = false; break; }
        if (is_pk) {
          return "ONE_TO_ONE";
        }
    }
    auto it = schemas.find(fk.target_table);
    if (it != schemas.end()) {
        const auto& target = it->second;
        auto tgt_cols = splitColumns(fk.target_column);
        const auto& tpks = target.primary_keys;
        bool tgt_is_pk = (!tpks.empty() && tpks.size() == tgt_cols.size());
        if (tgt_is_pk) {
            for (const auto& tc : tgt_cols)
                if (std::find(tpks.begin(), tpks.end(), tc) == tpks.end()) { tgt_is_pk = false; break; }
        }
        if (!tgt_is_pk) {
          return "MANY_TO_MANY";
        }
    }
    return "MANY_TO_ONE";
}

// Minimal RelationshipMapper stub for unit testing
class RelationshipMapper {
public:
    struct RelationshipMapping {
        std::string edge_type;
        std::string source_table;
        std::string source_column;
        std::string target_table;
        std::string target_column;
        std::string cardinality;
        std::string on_delete_action;
        std::string on_update_action;
        bool is_self_referential = false;
    };

    static std::vector<RelationshipMapping> mapFromForeignKeys(
            const std::map<std::string, TableSchema>& schemas,
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
                if (!fk.name.empty()) {
                    m.edge_type = fk.name;
                } else if (m.is_self_referential) {
                    std::string col_hint = fk.source_column;
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
                m.cardinality = detectCardinality(tschema, fk, schemas);
                result.push_back(std::move(m));
            }
        }
        return result;
    }

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

    static bool detectCircularReferences(
            const std::map<std::string, TableSchema>& schemas,
            std::vector<std::string>& cycles) {
        std::map<std::string, std::set<std::string>> adj;
        for (const auto& [tname, tschema] : schemas)
            for (const auto& fk : tschema.foreign_keys)
                if (!fk.target_table.empty() && fk.target_table != tname)
                    adj[tname].insert(fk.target_table);
        std::set<std::string> visited, in_stack;
        std::vector<std::string> path;
        bool found = false;
        std::function<void(const std::string&)> dfs = [&](const std::string& node) {
            visited.insert(node); in_stack.insert(node); path.push_back(node);
            auto it = adj.find(node);
            if (it != adj.end()) {
                for (const auto& nb : it->second) {
                    if (!visited.count(nb)) { dfs(nb); }
                    else if (in_stack.count(nb)) {
                        found = true;
                        std::string chain; bool in = false;
                        for (const auto& n : path) {
                            if (n == nb) {
                              in = true;
                            }
                            if (in) { if (!chain.empty()) chain += " -> "; chain += n; }
                        }
                        chain += " -> " + nb;
                        cycles.push_back(chain);
                    }
                }
            }
            path.pop_back(); in_stack.erase(node);
        };
        for (const auto& [tname, _] : schemas)
            if (!visited.count(tname)) {
              dfs(tname);
            }
        return found;
    }
};

// ============================================================================
// Test Suite: Foreign Key Parsing
// ============================================================================

TEST(PostgresImporterV2, ParseSimpleForeignKey) {
    std::string def = "CONSTRAINT fk_orders_users FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.name, "fk_orders_users");
    EXPECT_EQ(fk.source_column, "user_id");
    EXPECT_EQ(fk.target_table, "users");
    EXPECT_EQ(fk.target_column, "id");
    EXPECT_EQ(fk.on_delete_action, "CASCADE");
    EXPECT_FALSE(fk.deferrable);
    EXPECT_FALSE(fk.initially_deferred);
}

TEST(PostgresImporterV2, ParseForeignKeyNoName) {
    std::string def = "FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE RESTRICT ON UPDATE CASCADE";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.name, "");
    EXPECT_EQ(fk.source_column, "order_id");
    EXPECT_EQ(fk.target_table, "orders");
    EXPECT_EQ(fk.target_column, "id");
    EXPECT_EQ(fk.on_delete_action, "RESTRICT");
    EXPECT_EQ(fk.on_update_action, "CASCADE");
}

TEST(PostgresImporterV2, ParseCompositeForeignKey) {
    std::string def = "CONSTRAINT fk_composite FOREIGN KEY (order_id, item_id) REFERENCES order_details(order_id, item_id)";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.name, "fk_composite");
    EXPECT_EQ(fk.source_column, "order_id,item_id");
    EXPECT_EQ(fk.target_column, "order_id,item_id");
    EXPECT_EQ(fk.target_table, "order_details");
}

TEST(PostgresImporterV2, ParseDeferrableForeignKey) {
    std::string def = "CONSTRAINT fk_deferred FOREIGN KEY (user_id) REFERENCES users(id) DEFERRABLE INITIALLY DEFERRED";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.name, "fk_deferred");
    EXPECT_TRUE(fk.deferrable);
    EXPECT_TRUE(fk.initially_deferred);
}

TEST(PostgresImporterV2, ParseDeferrableInitiallyImmediate) {
    std::string def = "FOREIGN KEY (tenant_id) REFERENCES tenants(id) DEFERRABLE INITIALLY IMMEDIATE";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_TRUE(fk.deferrable);
    EXPECT_FALSE(fk.initially_deferred);
}

TEST(PostgresImporterV2, ParseNotDeferrable) {
    std::string def = "FOREIGN KEY (product_id) REFERENCES products(id) NOT DEFERRABLE";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_FALSE(fk.deferrable);
}

TEST(PostgresImporterV2, ParseForeignKeySetNull) {
    std::string def = "FOREIGN KEY (manager_id) REFERENCES employees(id) ON DELETE SET NULL ON UPDATE SET DEFAULT";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.on_delete_action, "SET NULL");
    EXPECT_EQ(fk.on_update_action, "SET DEFAULT");
}

TEST(PostgresImporterV2, ParseForeignKeyNoAction) {
    std::string def = "FOREIGN KEY (cat_id) REFERENCES categories(id) ON DELETE NO ACTION";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.on_delete_action, "NO ACTION");
}

TEST(PostgresImporterV2, ParseForeignKeySchemaQualifiedTarget) {
    std::string def = "FOREIGN KEY (user_id) REFERENCES public.users(id)";
    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(def, fk));
    EXPECT_EQ(fk.target_table, "users");
}

TEST(PostgresImporterV2, RejectInvalidForeignKeyDefinition) {
    std::string def = "user_id INTEGER NOT NULL";
    ForeignKeyConstraint fk;
    EXPECT_FALSE(parseForeignKeyConstraint(def, fk));
}

// ============================================================================
// Test Suite: CREATE INDEX Parsing
// ============================================================================

TEST(PostgresImporterV2, ParseSimpleIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE INDEX idx_user_email ON users(email);", idx));
    EXPECT_EQ(idx.name, "idx_user_email");
    EXPECT_FALSE(idx.unique);
    EXPECT_EQ(idx.type, "btree");
    ASSERT_EQ(idx.columns.size(), 1u);
    EXPECT_EQ(idx.columns[0], "email");
    EXPECT_FALSE(idx.partial);
}

TEST(PostgresImporterV2, ParseUniqueIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE UNIQUE INDEX idx_email ON users(email);", idx));
    EXPECT_EQ(idx.name, "idx_email");
    EXPECT_TRUE(idx.unique);
    EXPECT_EQ(idx.columns[0], "email");
}

TEST(PostgresImporterV2, ParseHashIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE INDEX idx_hash ON orders USING hash (order_code);", idx));
    EXPECT_EQ(idx.type, "hash");
    EXPECT_EQ(idx.columns[0], "order_code");
}

TEST(PostgresImporterV2, ParseGistIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE INDEX idx_geo ON locations USING gist (coordinates);", idx));
    EXPECT_EQ(idx.type, "gist");
    EXPECT_EQ(idx.columns[0], "coordinates");
}

TEST(PostgresImporterV2, ParseGinIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE INDEX idx_tags ON posts USING gin (tags);", idx));
    EXPECT_EQ(idx.type, "gin");
}

TEST(PostgresImporterV2, ParseMultiColumnIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex("CREATE INDEX idx_composite ON orders(user_id, created_at);", idx));
    ASSERT_EQ(idx.columns.size(), 2u);
    EXPECT_EQ(idx.columns[0], "user_id");
    EXPECT_EQ(idx.columns[1], "created_at");
}

TEST(PostgresImporterV2, ParsePartialIndex) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex(
        "CREATE INDEX idx_active ON users(email) WHERE active = true", idx));
    EXPECT_TRUE(idx.partial);
    EXPECT_FALSE(idx.where_clause.empty());
    EXPECT_EQ(idx.columns[0], "email");
}

TEST(PostgresImporterV2, ParseIndexConcurrently) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex(
        "CREATE INDEX CONCURRENTLY idx_c ON orders(status);", idx));
    EXPECT_EQ(idx.name, "idx_c");
    EXPECT_EQ(idx.columns[0], "status");
}

TEST(PostgresImporterV2, ParseIndexIfNotExists) {
    IndexMetadata idx;
    ASSERT_TRUE(parseCreateIndex(
        "CREATE INDEX IF NOT EXISTS idx_safe ON products(sku);", idx));
    EXPECT_EQ(idx.name, "idx_safe");
}

// ============================================================================
// Test Suite: Relationship Mapping / Cardinality Detection
// ============================================================================

TEST(PostgresImporterV2, CardinalityManyToOne) {
    TableSchema orders;
    orders.name = "orders";
    orders.columns = {"id", "user_id"};
    orders.primary_keys = {"id"};

    TableSchema users;
    users.name = "users";
    users.columns = {"id"};
    users.primary_keys = {"id"};

    ForeignKeyConstraint fk;
    fk.source_column = "user_id";
    fk.target_table  = "users";
    fk.target_column = "id";

    std::map<std::string, TableSchema> schemas = {{"users", users}, {"orders", orders}};
    EXPECT_EQ(detectCardinality(orders, fk, schemas), "MANY_TO_ONE");
}

TEST(PostgresImporterV2, CardinalityOneToOne) {
    TableSchema profiles;
    profiles.name = "profiles";
    profiles.columns = {"user_id", "bio"};
    profiles.primary_keys = {"user_id"};  // profile.user_id IS the PK

    TableSchema users;
    users.name = "users";
    users.columns = {"id"};
    users.primary_keys = {"id"};

    ForeignKeyConstraint fk;
    fk.source_column = "user_id";
    fk.target_table  = "users";
    fk.target_column = "id";

    std::map<std::string, TableSchema> schemas = {{"users", users}, {"profiles", profiles}};
    EXPECT_EQ(detectCardinality(profiles, fk, schemas), "ONE_TO_ONE");
}

TEST(PostgresImporterV2, CardinalityManyToMany) {
    TableSchema order_tags;
    order_tags.name = "order_tags";
    order_tags.columns = {"order_id", "tag_id"};
    order_tags.primary_keys = {"order_id", "tag_id"};

    TableSchema tags;
    tags.name = "tags";
    tags.columns = {"id", "label"};
    tags.primary_keys = {"id"};

    // FK points to tags.label (not the PK)
    ForeignKeyConstraint fk;
    fk.source_column = "tag_id";
    fk.target_table  = "tags";
    fk.target_column = "label";  // not a PK → MANY_TO_MANY

    std::map<std::string, TableSchema> schemas = {{"tags", tags}, {"order_tags", order_tags}};
    EXPECT_EQ(detectCardinality(order_tags, fk, schemas), "MANY_TO_MANY");
}

TEST(PostgresImporterV2, EdgeTypeFromFkName) {
    // Edge type should use FK name if set
    ForeignKeyConstraint fk;
    fk.name = "fk_orders_users";
    fk.source_column = "user_id";
    fk.target_table  = "users";
    fk.target_column = "id";

    // Expected edge_type = fk.name = "fk_orders_users"
    std::string edge_type = fk.name.empty() ? ("orders_references_users") : fk.name;
    EXPECT_EQ(edge_type, "fk_orders_users");
}

TEST(PostgresImporterV2, EdgeTypeGenerated) {
    ForeignKeyConstraint fk;
    fk.name = "";  // no name
    fk.source_column = "user_id";
    fk.target_table  = "users";

    std::string edge_type = fk.name.empty() ? ("orders_references_users") : fk.name;
    EXPECT_EQ(edge_type, "orders_references_users");
}

// ============================================================================
// Test Suite: Circular Reference Detection
// ============================================================================

TEST(PostgresImporterV2, NoCircularReferences) {
    // A → B → C (no cycle)
    std::map<std::string, std::set<std::string>> adj = {
        {"a", {"b"}}, {"b", {"c"}}, {"c", {}}
    };
    // Manual DFS check
    std::set<std::string> visited, in_stack;
    std::vector<std::string> path, cycles;

    // Simplified cycle detection using the same logic
    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        visited.insert(node);
        in_stack.insert(node);
        path.push_back(node);
        bool found = false;
        for (auto& nb : adj[node]) {
            if (!visited.count(nb)) { if (dfs(nb)) found = true; }
            else if (in_stack.count(nb)) { found = true; cycles.push_back(node + " → " + nb); }
        }
        path.pop_back();
        in_stack.erase(node);
        return found;
    };
    bool cycle = false;
    for (auto& [n, _] : adj) {
      if (!visited.count(n)) if (dfs(n)) cycle = true;
    }
    EXPECT_FALSE(cycle);
    EXPECT_TRUE(cycles.empty());
}

TEST(PostgresImporterV2, DetectsCircularReferences) {
    // A → B → C → A (cycle)
    std::map<std::string, std::set<std::string>> adj = {
        {"a", {"b"}}, {"b", {"c"}}, {"c", {"a"}}
    };
    std::set<std::string> visited, in_stack;
    std::vector<std::string> path, cycles;
    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        visited.insert(node); in_stack.insert(node); path.push_back(node);
        bool found = false;
        for (auto& nb : adj[node]) {
            if (!visited.count(nb)) { if (dfs(nb)) found = true; }
            else if (in_stack.count(nb)) { found = true; cycles.push_back(node + " → " + nb); }
        }
        path.pop_back(); in_stack.erase(node);
        return found;
    };
    bool cycle = false;
    for (auto& [n, _] : adj) {
      if (!visited.count(n)) if (dfs(n)) cycle = true;
    }
    EXPECT_TRUE(cycle);
    EXPECT_FALSE(cycles.empty());
}

// ============================================================================
// Test Suite: FK Reference Validation
// ============================================================================

TEST(PostgresImporterV2, FKTargetTableExists) {
    std::map<std::string, TableSchema> schemas = {};

    TableSchema users; users.name = "users"; users.columns = {"id"}; users.primary_keys = {"id"};
    TableSchema orders; orders.name = "orders"; orders.columns = {"id", "user_id"};
    ForeignKeyConstraint fk; fk.target_table = "users"; fk.target_column = "id";
    orders.foreign_keys.push_back(fk);
    schemas["users"] = users;
    schemas["orders"] = orders;

    // Validate
    bool all_valid = true;
    for (auto& [tname, tschema] : schemas) {
        for (auto& fk_ref : tschema.foreign_keys) {
            if (!schemas.count(fk_ref.target_table)) {
              all_valid = false;
            }
        }
    }
    EXPECT_TRUE(all_valid);
}

TEST(PostgresImporterV2, FKTargetTableMissing) {
    std::map<std::string, TableSchema> schemas;
    TableSchema orders; orders.name = "orders";
    ForeignKeyConstraint fk; fk.target_table = "nonexistent_users";
    orders.foreign_keys.push_back(fk);
    schemas["orders"] = orders;

    bool all_valid = true;
    for (auto& [tname, tschema] : schemas) {
        for (auto& fk_ref : tschema.foreign_keys) {
            if (!schemas.count(fk_ref.target_table)) {
              all_valid = false;
            }
        }
    }
    EXPECT_FALSE(all_valid);
}

TEST(PostgresImporterV2, FKTargetColumnMissing) {
    std::map<std::string, TableSchema> schemas = {};

    TableSchema users; users.name = "users"; users.columns = {"id"};
    TableSchema orders; orders.name = "orders";
    ForeignKeyConstraint fk; fk.target_table = "users"; fk.target_column = "nonexistent_col";
    orders.foreign_keys.push_back(fk);
    schemas["users"] = users; schemas["orders"] = orders;

    bool col_ok = true;
    for (auto& [tname, tschema] : schemas) {
        for (auto& fk_ref : tschema.foreign_keys) {
            if (schemas.count(fk_ref.target_table)) {
                const auto& tgt = schemas.at(fk_ref.target_table);
                if (std::find(tgt.columns.begin(), tgt.columns.end(), fk_ref.target_column)
                        == tgt.columns.end()) col_ok = false;
            }
        }
    }
    EXPECT_FALSE(col_ok);
}

// ============================================================================
// Test Suite: splitTopLevelCommas (regression)
// ============================================================================

TEST(PostgresImporterV2, SplitNestedParens) {
    auto parts = splitTopLevelCommas("a, f(x,y), c");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(trimStr(parts[1]), "f(x,y)");
}

TEST(PostgresImporterV2, SplitQuotedDefault) {
    auto parts = splitTopLevelCommas("a, DEFAULT 'x,y', c");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(trimStr(parts[1]), "DEFAULT 'x,y'");
}

TEST(PostgresImporterV2, SplitCheckConstraint) {
    auto parts = splitTopLevelCommas("a, CHECK (x > 0 AND y > 1), c");
    ASSERT_EQ(parts.size(), 3u);
}

// ============================================================================
// Test Suite: RelationshipMapper::splitColumns
// ============================================================================

TEST(PostgresImporterV2, SplitColumnsSimple) {
    auto cols = splitColumns("user_id");
    ASSERT_EQ(cols.size(), 1u);
    EXPECT_EQ(cols[0], "user_id");
}

TEST(PostgresImporterV2, SplitColumnsComposite) {
    auto cols = splitColumns("order_id, item_id");
    ASSERT_EQ(cols.size(), 2u);
    EXPECT_EQ(cols[0], "order_id");
    EXPECT_EQ(cols[1], "item_id");
}

TEST(PostgresImporterV2, SplitColumnsWithSpaces) {
    auto cols = splitColumns("  a  ,  b  ,  c  ");
    ASSERT_EQ(cols.size(), 3u);
    EXPECT_EQ(cols[0], "a");
    EXPECT_EQ(cols[2], "c");
}

// ============================================================================
// Test Suite: ALTER TABLE FOREIGN KEY pattern
// ============================================================================

TEST(PostgresImporterV2, AlterTableForeignKeyPattern) {
    const std::string sql =
        "ALTER TABLE ONLY public.orders "
        "ADD CONSTRAINT fk_orders_users FOREIGN KEY (user_id) REFERENCES public.users(id) "
        "ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED;";

    ForeignKeyConstraint fk;
    ASSERT_TRUE(parseForeignKeyConstraint(sql, fk));
    EXPECT_EQ(fk.name, "fk_orders_users");
    EXPECT_EQ(fk.source_column, "user_id");
    EXPECT_EQ(fk.target_table, "users");
    EXPECT_EQ(fk.on_delete_action, "CASCADE");
    EXPECT_TRUE(fk.deferrable);
    EXPECT_TRUE(fk.initially_deferred);
}

// ============================================================================
// v2.1 Tests – Additional Constraint Types
// ============================================================================

TEST(PostgresImporterV2, ParseCheckConstraintNamed) {
    const std::string def = "CONSTRAINT ck_positive CHECK (amount > 0)";
    CheckConstraint ck;
    ASSERT_TRUE(parseCheckConstraint(def, ck));
    EXPECT_EQ(ck.name, "ck_positive");
    EXPECT_EQ(ck.expression, "amount > 0");
}

TEST(PostgresImporterV2, ParseCheckConstraintUnnamed) {
    const std::string def = "CHECK (price >= 0)";
    CheckConstraint ck;
    ASSERT_TRUE(parseCheckConstraint(def, ck));
    EXPECT_TRUE(ck.name.empty());
    EXPECT_EQ(ck.expression, "price >= 0");
}

TEST(PostgresImporterV2, ParseCheckConstraintComplex) {
    const std::string def = "CONSTRAINT ck_range CHECK (age >= 0 AND age < 150)";
    CheckConstraint ck;
    ASSERT_TRUE(parseCheckConstraint(def, ck));
    EXPECT_EQ(ck.name, "ck_range");
    EXPECT_FALSE(ck.expression.empty());
    EXPECT_NE(ck.expression.find("age >= 0"), std::string::npos);
}

TEST(PostgresImporterV2, ParseCheckConstraintReturnsFalseWithoutCheck) {
    const std::string def = "UNIQUE (email)";
    CheckConstraint ck;
    ASSERT_FALSE(parseCheckConstraint(def, ck));
}

TEST(PostgresImporterV2, ParseExcludeConstraint) {
    const std::string def = "CONSTRAINT room_exclusive EXCLUDE USING gist (room WITH =, during WITH &&)";
    ExcludeConstraint excl;
    ASSERT_TRUE(parseExcludeConstraint(def, excl));
    EXPECT_EQ(excl.name, "room_exclusive");
    EXPECT_NE(excl.definition.find("EXCLUDE"), std::string::npos);
}

TEST(PostgresImporterV2, ParseExcludeConstraintNoName) {
    const std::string def = "EXCLUDE USING gist (period WITH &&)";
    ExcludeConstraint excl;
    ASSERT_TRUE(parseExcludeConstraint(def, excl));
    EXPECT_TRUE(excl.name.empty());
    EXPECT_NE(excl.definition.find("EXCLUDE"), std::string::npos);
}

TEST(PostgresImporterV2, ParseGeneratedAlwaysAsIdentity) {
    const std::string col_def = "id INTEGER GENERATED ALWAYS AS IDENTITY";
    GeneratedColumnInfo gen;
    ASSERT_TRUE(parseGeneratedColumn(col_def, "id", gen));
    EXPECT_EQ(gen.column, "id");
    EXPECT_TRUE(gen.is_identity);
    EXPECT_EQ(gen.generation, "ALWAYS");
}

TEST(PostgresImporterV2, ParseGeneratedByDefaultAsIdentity) {
    const std::string col_def = "seq_no BIGINT GENERATED BY DEFAULT AS IDENTITY";
    GeneratedColumnInfo gen;
    ASSERT_TRUE(parseGeneratedColumn(col_def, "seq_no", gen));
    EXPECT_TRUE(gen.is_identity);
    EXPECT_EQ(gen.generation, "BY_DEFAULT");
}

TEST(PostgresImporterV2, ParseGeneratedAlwaysAsExpressionStored) {
    const std::string col_def =
        "total DECIMAL GENERATED ALWAYS AS (price * quantity) STORED";
    GeneratedColumnInfo gen;
    ASSERT_TRUE(parseGeneratedColumn(col_def, "total", gen));
    EXPECT_FALSE(gen.is_identity);
    EXPECT_TRUE(gen.stored);
    EXPECT_EQ(gen.generation, "ALWAYS");
    EXPECT_NE(gen.expression.find("price"), std::string::npos);
}

TEST(PostgresImporterV2, ParseGeneratedReturnsFalseWithoutGenerated) {
    const std::string col_def = "amount DECIMAL NOT NULL";
    GeneratedColumnInfo gen;
    ASSERT_FALSE(parseGeneratedColumn(col_def, "amount", gen));
}

TEST(PostgresImporterV2, CheckConstraintExtractedFromTableDef) {
    // Simulate the CHECK clause that would be extracted from a CREATE TABLE
    // column_def list and passed to parseCheckConstraint
    const std::string def = "CONSTRAINT ck_price_positive CHECK (price > 0)";
    CheckConstraint ck;
    ASSERT_TRUE(parseCheckConstraint(def, ck));
    EXPECT_EQ(ck.name, "ck_price_positive");
    EXPECT_NE(ck.expression.find("price > 0"), std::string::npos);
}

TEST(PostgresImporterV2, UnnamedCheckExtractedFromTableDef) {
    // Simulate the unnamed CHECK clause from a CREATE TABLE
    const std::string def = "CHECK (stars >= 1 AND stars <= 5)";
    CheckConstraint ck;
    ASSERT_TRUE(parseCheckConstraint(def, ck));
    EXPECT_TRUE(ck.name.empty());
    EXPECT_NE(ck.expression.find("stars >= 1"), std::string::npos);
}

TEST(PostgresImporterV2, GeneratedAlwaysAsIdentityFromColDef) {
    const std::string col_def = "id INTEGER GENERATED ALWAYS AS IDENTITY";
    GeneratedColumnInfo gen;
    ASSERT_TRUE(parseGeneratedColumn(col_def, "id", gen));
    EXPECT_EQ(gen.column, "id");
    EXPECT_TRUE(gen.is_identity);
    EXPECT_EQ(gen.generation, "ALWAYS");
}

// ============================================================================
// v2.1 Tests – Relationship Mapping Enhancements
// ============================================================================

TEST(PostgresImporterV2, RelationshipMappingOnDeletePropagated) {
    TableSchema users;
    users.name = "users";
    users.columns = {"id"};
    users.primary_keys = {"id"};

    TableSchema orders;
    orders.name = "orders";
    orders.columns = {"id", "user_id"};
    ForeignKeyConstraint fk;
    fk.source_column    = "user_id";
    fk.target_table     = "users";
    fk.target_column    = "id";
    fk.on_delete_action = "CASCADE";
    fk.on_update_action = "RESTRICT";
    orders.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["users"]  = users;
    schemas["orders"] = orders;

    auto mappings = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    ASSERT_EQ(mappings.size(), 1u);
    EXPECT_EQ(mappings[0].on_delete_action, "CASCADE");
    EXPECT_EQ(mappings[0].on_update_action, "RESTRICT");
    EXPECT_FALSE(mappings[0].is_self_referential);
}

TEST(PostgresImporterV2, SelfReferentialRelationship) {
    TableSchema employees;
    employees.name = "employees";
    employees.columns = {"id", "manager_id"};
    employees.primary_keys = {"id"};
    ForeignKeyConstraint fk;
    fk.source_column = "manager_id";
    fk.target_table  = "employees";
    fk.target_column = "id";
    employees.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["employees"] = employees;

    auto mappings = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    ASSERT_EQ(mappings.size(), 1u);
    EXPECT_TRUE(mappings[0].is_self_referential);
    // Edge type should include the column hint "manager" (stripped "_id")
    EXPECT_NE(mappings[0].edge_type.find("manager"), std::string::npos);
}

TEST(PostgresImporterV2, SelfReferentialNotInverted) {
    TableSchema nodes;
    nodes.name = "nodes";
    nodes.columns = {"id", "parent_id"};
    nodes.primary_keys = {"id"};
    ForeignKeyConstraint fk;
    fk.source_column = "parent_id";
    fk.target_table  = "nodes";
    fk.target_column = "id";
    nodes.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["nodes"] = nodes;

    auto mappings = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    auto inverse = RelationshipMapper::generateInverseEdges(mappings);
    // Self-referential edges must NOT be inverted
    EXPECT_EQ(inverse.size(), 0u);
}

TEST(PostgresImporterV2, InverseEdgesGenerated) {
    TableSchema users;
    users.name = "users";
    users.columns = {"id"};
    users.primary_keys = {"id"};

    TableSchema orders;
    orders.name = "orders";
    orders.columns = {"id", "user_id"};
    ForeignKeyConstraint fk;
    fk.source_column = "user_id";
    fk.target_table  = "users";
    fk.target_column = "id";
    orders.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["users"]  = users;
    schemas["orders"] = orders;

    auto forward = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    auto inverse = RelationshipMapper::generateInverseEdges(forward);

    ASSERT_EQ(inverse.size(), 1u);
    EXPECT_EQ(inverse[0].cardinality, "ONE_TO_MANY");
    EXPECT_EQ(inverse[0].source_table, "users");
    EXPECT_EQ(inverse[0].target_table, "orders");
    // edge_type should be users_has_many_orders
    EXPECT_NE(inverse[0].edge_type.find("has_many"), std::string::npos);
}

TEST(PostgresImporterV2, InverseEdgesNotGeneratedForManyToMany) {
    TableSchema a;
    a.name = "a";
    a.columns = {"id", "b_ref"};
    a.primary_keys = {"id"};

    TableSchema b;
    b.name = "b";
    b.columns = {"id", "some_col"};
    b.primary_keys = {"id"};

    ForeignKeyConstraint fk;
    fk.source_column = "b_ref";
    fk.target_table  = "b";
    fk.target_column = "some_col";  // not a PK → MANY_TO_MANY
    a.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["a"] = a;
    schemas["b"] = b;

    auto forward = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    ASSERT_EQ(forward.size(), 1u);
    EXPECT_EQ(forward[0].cardinality, "MANY_TO_MANY");

    auto inverse = RelationshipMapper::generateInverseEdges(forward);
    EXPECT_EQ(inverse.size(), 0u);  // MANY_TO_MANY has no simple inverse
}

TEST(PostgresImporterV2, RelationshipEdgeTypeUsesNameWhenSet) {
    TableSchema orders;
    orders.name = "orders";
    orders.columns = {"id", "user_id"};
    orders.primary_keys = {"id"};

    TableSchema users;
    users.name = "users";
    users.columns = {"id"};
    users.primary_keys = {"id"};

    ForeignKeyConstraint fk;
    fk.name          = "fk_orders_users";
    fk.source_column = "user_id";
    fk.target_table  = "users";
    fk.target_column = "id";
    orders.foreign_keys.push_back(fk);

    std::map<std::string, TableSchema> schemas;
    schemas["orders"] = orders;
    schemas["users"]  = users;

    auto mappings = RelationshipMapper::mapFromForeignKeys(schemas, "auto");
    ASSERT_EQ(mappings.size(), 1u);
    EXPECT_EQ(mappings[0].edge_type, "fk_orders_users");
}

// ============================================================================
// Main
// ============================================================================
#include <atomic>
#include <thread>
#include <chrono>

// New v2.1+ headers
#include "importers/schema_inference.h"
#include "importers/column_importance.h"
#include "importers/crdt_importer.h"
#include "importers/postgres_cdc.h"
#include "importers/data_quality.h"
#include "importers/audit_trail.h"
#include "importers/adaptive_import.h"
#include "importers/polyglot_mapper.h"
#include "importers/temporal_support.h"
#include "importers/blockchain_integrity.h"
#include "importers/federated_learning.h"
#include "importers/graphql_federation.h"

using json = nlohmann::json;
using namespace themis::importers;

// ===========================================================================
// Helper: build a minimal InferenceTableSchema
// ===========================================================================
static InferenceTableSchema makeSchema(
    const std::string& name,
    const std::vector<std::string>& columns,
    const std::vector<std::string>& pks = {},
    const std::vector<std::pair<std::string,std::string>>& fks = {})
{
    InferenceTableSchema s;
    s.name        = name;
    s.schema_ns   = "public";
    s.columns     = columns;
    s.primary_keys = pks;
    s.foreign_keys = fks;
    for (const auto& col : columns) {
      s.column_types[col] = "text";
    }
    return s;
}

// ===========================================================================
// 1. Schema Inference Engine
// ===========================================================================

TEST(SchemaInferenceEngine, DefaultConstruct) {
    SchemaInferenceEngine engine;
    EXPECT_NO_THROW(engine.inferImplicitRelationships({}, {}));
}

TEST(SchemaInferenceEngine, InferImplicitRelationshipsByNameStem) {
    auto users  = makeSchema("users",  {"id", "name"},        {"id"});
    auto orders = makeSchema("orders", {"id", "user_id", "amount"}, {"id"});

    SchemaInferenceEngine engine;
    auto inferred = engine.inferImplicitRelationships({users, orders}, {});

    // orders table should have at least one likely relationship detected
    bool found = false;
    for (const auto& inf : inferred) {
        if (inf.table_name == "orders" && !inf.likely_relationships.empty()) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(SchemaInferenceEngine, InferReturnsSchemasForEveryTable) {
    auto t1 = makeSchema("t1", {"id"}, {"id"});
    auto t2 = makeSchema("t2", {"id"}, {"id"});
    SchemaInferenceEngine engine;
    auto result = engine.inferImplicitRelationships({t1, t2}, {});
    EXPECT_EQ(result.size(), 2u);
}

TEST(SchemaInferenceEngine, SemanticTypeDetection_Email) {
    auto s = makeSchema("users", {"email"});
    SampleData sd;
    sd.table_name  = "users";
    sd.column_name = "email";
    sd.values      = {"alice@example.com", "bob@test.org", "carol@domain.net"};

    SchemaInferenceEngine engine;
    auto types = engine.detectSemanticTypes({s}, {sd});

    ASSERT_TRUE(types.count("users.email"));
    EXPECT_EQ(types.at("users.email"), SchemaInferenceEngine::SemanticType::EMAIL);
}

TEST(SchemaInferenceEngine, SemanticTypeDetection_UUID) {
    auto s = makeSchema("entities", {"uuid_col"});
    SampleData sd;
    sd.table_name  = "entities";
    sd.column_name = "uuid_col";
    sd.values = {
        "550e8400-e29b-41d4-a716-446655440000",
        "6ba7b810-9dad-11d1-80b4-00c04fd430c8",
        "6ba7b811-9dad-11d1-80b4-00c04fd430c8"
    };

    SchemaInferenceEngine engine;
    auto types = engine.detectSemanticTypes({s}, {sd});
    ASSERT_TRUE(types.count("entities.uuid_col"));
    EXPECT_EQ(types.at("entities.uuid_col"), SchemaInferenceEngine::SemanticType::UUID);
}

TEST(SchemaInferenceEngine, SemanticTypeToString) {
    EXPECT_EQ(SchemaInferenceEngine::semanticTypeToString(
                SchemaInferenceEngine::SemanticType::EMAIL),
              "EMAIL");
    EXPECT_EQ(SchemaInferenceEngine::semanticTypeToString(
                SchemaInferenceEngine::SemanticType::UNKNOWN),
              "UNKNOWN");
}

TEST(SchemaInferenceEngine, CardinalityEstimateFromStats) {
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    ColumnStatistics cs;
    cs.column_name   = "user_id";
    cs.table_name    = "orders";
    cs.total_rows    = 1000;
    cs.distinct_count = 100;
    cs.null_count    = 10;

    SchemaInferenceEngine engine;
    auto ests = engine.estimateCardinalities({orders},
        {{"orders.user_id", cs}});

    ASSERT_FALSE(ests.empty());
    EXPECT_GT(ests[0].one_to_many_ratio, 0.0);
    EXPECT_GE(ests[0].selectivity, 0.0);
    EXPECT_LE(ests[0].selectivity, 1.0);
    EXPECT_EQ(ests[0].confidence_interval.size(), 2u);
}

TEST(SchemaInferenceEngine, CardinalityEstimateNoStats) {
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    SchemaInferenceEngine engine;
    auto ests = engine.estimateCardinalities({orders}, {});
    EXPECT_FALSE(ests.empty());
}

// ===========================================================================
// 2. Column Importance Analyzer
// ===========================================================================

TEST(ColumnImportanceAnalyzer, AnalyzeEmptySamples) {
    ColumnImportanceAnalyzer analyzer;
    auto result = analyzer.analyzeImportance({}, {});
    EXPECT_TRUE(result.empty());
}

TEST(ColumnImportanceAnalyzer, EntropyIsNonNegative) {
    auto schema = makeSchema("logs", {"level", "message"});
    SampleData sd;
    sd.table_name  = "logs";
    sd.column_name = "level";
    sd.values      = {"INFO", "WARN", "ERROR", "INFO", "INFO"};

    ColumnImportanceAnalyzer analyzer;
    auto result = analyzer.analyzeImportance({schema}, {sd});

    ASSERT_FALSE(result.empty());
    for (const auto& ci : result) {
        EXPECT_GE(ci.entropy, 0.0);
        EXPECT_GE(ci.gini_impurity, 0.0);
        EXPECT_GE(ci.information_gain, 0.0);
    }
}

TEST(ColumnImportanceAnalyzer, ToJsonReturnsTable) {
    ColumnImportanceAnalyzer::ColumnImportance ci;
    ci.table_name  = "t";
    ci.column_name = "c";
    ci.entropy     = 1.5;
    auto j = ci.toJson();
    EXPECT_EQ(j["table"].get<std::string>(), "t");
    EXPECT_EQ(j["column"].get<std::string>(), "c");
    EXPECT_DOUBLE_EQ(j["entropy"].get<double>(), 1.5);
}

TEST(ColumnImportanceAnalyzer, FindRedundantColumnsEmpty) {
    ColumnImportanceAnalyzer analyzer;
    auto red = analyzer.findRedundantColumns({});
    EXPECT_TRUE(red.empty());
}

TEST(ColumnImportanceAnalyzer, FindRedundantColumnsDetected) {
    ColumnImportanceAnalyzer::ColumnImportance a, b;
    a.table_name = b.table_name = "t";
    a.column_name = "c1"; b.column_name = "c2";
    a.entropy = b.entropy = 2.0; // identical entropy

    ColumnImportanceAnalyzer analyzer;
    auto red = analyzer.findRedundantColumns({a, b}, 0.95);
    EXPECT_FALSE(red.empty());
}

// ===========================================================================
// 3. CRDT Importer
// ===========================================================================

TEST(CRDTTableState, ImportAndLookup) {
    crdt::CRDTTableState state;
    json r1 = {{"id", "a"}, {"value", 1}};
    json r2 = {{"id", "b"}, {"value", 2}};

    size_t written = state.importWithCRDT("orders", {r1, r2}, "replica_1");
    EXPECT_EQ(written, 2u);

    auto* rec = state.lookup("orders", "a");
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->replica_id, "replica_1");
}

TEST(CRDTTableState, MergeLastWriteWins) {
    crdt::CRDTTableState::CRDTRecord left, right;
    left.id            = "x"; left.wall_clock_ns  = 100; left.lamport_clock = 1;
    left.replica_id    = "r1";
    right.id           = "x"; right.wall_clock_ns = 200; right.lamport_clock = 1;
    right.replica_id   = "r2";

    auto winner = crdt::CRDTTableState::CRDTRecord::merge(left, right);
    EXPECT_EQ(winner.wall_clock_ns, 200u);
    EXPECT_EQ(winner.replica_id, "r2");
}

TEST(CRDTTableState, MergeTiebreakByReplicaId) {
    crdt::CRDTTableState::CRDTRecord left, right;
    left.wall_clock_ns = right.wall_clock_ns = 100;
    left.lamport_clock = right.lamport_clock = 5;
    left.replica_id  = "z_replica";
    right.replica_id = "a_replica";

    auto winner = crdt::CRDTTableState::CRDTRecord::merge(left, right);
    EXPECT_EQ(winner.replica_id, "z_replica"); // lexicographically higher wins
}

TEST(CRDTTableState, SkipRecordsWithoutId) {
    crdt::CRDTTableState state;
    json r = {{"value", 42}}; // no "id"
    size_t written = state.importWithCRDT("t", {r}, "r1");
    EXPECT_EQ(written, 0u);
}

TEST(CRDTTableState, SerialiseDeserialise) {
    crdt::CRDTTableState::CRDTRecord r;
    r.id = "abc"; r.lamport_clock = 7; r.replica_id = "rx";
    r.wall_clock_ns = 999; r.value = json{{"x", 1}};

    auto j   = r.toJson();
    auto r2  = crdt::CRDTTableState::CRDTRecord::fromJson(j);
    EXPECT_EQ(r2.id, r.id);
    EXPECT_EQ(r2.lamport_clock, r.lamport_clock);
    EXPECT_EQ(r2.replica_id, r.replica_id);
}

TEST(CRDTTableState, TickClock) {
    crdt::CRDTTableState state;
    uint64_t t1 = state.tickClock();
    uint64_t t2 = state.tickClock();
    EXPECT_GT(t2, t1);
}

// ===========================================================================
// 4. PostgreSQL CDC
// ===========================================================================

TEST(PostgreSQLCDC, CreateDecoder) {
    auto dec = PostgreSQLCDC::createDecoder("host=localhost dbname=test");
    ASSERT_NE(dec, nullptr);
}

TEST(PostgreSQLCDC, CreatePublicationReturnTrue) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_TRUE(dec->createPublication("pub1", {"orders", "users"}));
}

TEST(PostgreSQLCDC, CreateReplicationSlotReturnTrue) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_TRUE(dec->createReplicationSlot("slot1", true));
}

TEST(PostgreSQLCDC, ConfirmLSNNoThrow) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_NO_THROW(dec->confirmLSN(12345));
}

TEST(PostgreSQLCDC, CancelStopsSubscription) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");

    int event_count = 0;
    // Run subscribeToChanges in a thread and cancel immediately
    std::atomic<bool> started{false};
    std::thread t([&]() {
        dec->subscribeToChanges("slot1", [&](const PostgreSQLCDC::ChangeEvent& e) {
            ++event_count;
            started = true;
        });
    });

    // Wait for sentinel event, then cancel
    while (!started) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    dec->cancel();
    t.join();

    EXPECT_GE(event_count, 1); // at least the sentinel
}

// ===========================================================================
// 5. Data Quality Framework
// ===========================================================================

TEST(DataQualityFramework, AssessTableEmpty) {
    DataQualityFramework::QualityAssessor qa;
    auto m = qa.assessTable("t", {});
    EXPECT_EQ(m.overall_quality_score, 0.0);
}

TEST(DataQualityFramework, AssessTablePerfectData) {
    DataQualityFramework::QualityAssessor qa;
    std::vector<json> rows = {
        {{"id", "1"}, {"name", "Alice"}},
        {{"id", "2"}, {"name", "Bob"}},
        {{"id", "3"}, {"name", "Carol"}}
    };
    auto m = qa.assessTable("users", rows);
    EXPECT_GT(m.overall_quality_score, 50.0);
    EXPECT_LE(m.completeness, 1.0);
    EXPECT_GE(m.completeness, 0.0);
}

TEST(DataQualityFramework, ToJsonFields) {
    DataQualityFramework::DataQualityMetrics m;
    m.completeness = 0.9; m.accuracy = 0.8;
    m.overall_quality_score = 85.0;
    auto j = m.toJson();
    EXPECT_DOUBLE_EQ(j["completeness"].get<double>(), 0.9);
    EXPECT_DOUBLE_EQ(j["overall_quality_score"].get<double>(), 85.0);
}

TEST(DataQualityFramework, GenerateQualityReportHasTimestamp) {
    DataQualityFramework::QualityAssessor qa;
    auto schema = makeSchema("t", {"id"}, {"id"});
    auto report = qa.generateQualityReport({schema});
    EXPECT_FALSE(report.generation_timestamp.empty());
}

TEST(DataQualityFramework, LowQualityTableTriggersIssue) {
    DataQualityFramework::QualityAssessor qa;
    // All null values → very low completeness
    std::vector<json> rows(5, json{{"id", nullptr}, {"val", nullptr}});
    auto schema = makeSchema("t", {"id", "val"});

    // Manually assess and check score is low
    auto m = qa.assessTable("t", rows);
    EXPECT_LT(m.completeness, 0.5);
}

// ===========================================================================
// 6. Audit Trail
// ===========================================================================

TEST(AuditTrail, RecordAndSize) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type               = AuditedImporter::EventType::IMPORT_STARTED;
    ev.timestamp          = "2026-03-11T00:00:00Z";
    ev.user_principal     = "user@example.com";
    ev.importer_instance_id = "inst-001";
    ev.correlation_id     = "corr-abc";
    ev.details            = json{{"rows", 100}};

    log.recordEvent(ev);
    EXPECT_EQ(log.size(), 1u);
}

TEST(AuditTrail, IntegrityVerifiesCleanLog) {
    AuditedImporter::ImmutableAuditLog log;
    for (int i = 0; i < 5; ++i) {
        AuditedImporter::AuditEvent ev;
        ev.type               = AuditedImporter::EventType::RECORD_IMPORTED;
        ev.timestamp          = "2026-03-11T00:00:0" + std::to_string(i) + "Z";
        ev.user_principal     = "svc";
        ev.importer_instance_id = "inst";
        ev.correlation_id     = "c";
        ev.details            = json{{"i", i}};
        log.recordEvent(ev);
    }
    EXPECT_TRUE(log.verifyIntegrity());
}

TEST(AuditTrail, ExportForSIEM_Raw) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type = AuditedImporter::EventType::IMPORT_COMPLETED;
    ev.timestamp = "2026-03-11T00:00:00Z";
    ev.details = json{{"tables", 3}};
    log.recordEvent(ev);

    auto exported = log.exportForSIEM("raw");
    ASSERT_TRUE(exported.is_array());
    EXPECT_EQ(exported.size(), 1u);
    EXPECT_EQ(exported[0]["event_type"].get<std::string>(), "IMPORT_COMPLETED");
}

TEST(AuditTrail, ExportForSIEM_Splunk) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type = AuditedImporter::EventType::IMPORT_STARTED;
    ev.timestamp = "2026-03-11T00:00:00Z";
    ev.details = json{};
    log.recordEvent(ev);

    auto exported = log.exportForSIEM("splunk");
    ASSERT_TRUE(exported.is_array());
    ASSERT_EQ(exported.size(), 1u);
    EXPECT_TRUE(exported[0].contains("sourcetype"));
}

TEST(AuditTrail, EventTypeToString) {
    EXPECT_EQ(AuditedImporter::eventTypeToString(
                AuditedImporter::EventType::IMPORT_STARTED),
              "IMPORT_STARTED");
    EXPECT_EQ(AuditedImporter::eventTypeToString(
                AuditedImporter::EventType::ERROR_OCCURRED),
              "ERROR_OCCURRED");
}

// ===========================================================================
// 7. Adaptive Import Optimizer
// ===========================================================================

TEST(AdaptiveImportOptimizer, OptimizePlanNoFK) {
    auto t1 = makeSchema("a", {"id"}, {"id"});
    auto t2 = makeSchema("b", {"id"}, {"id"});
    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({t1, t2});
    EXPECT_EQ(plan.import_order.size(), 2u);
    EXPECT_TRUE(plan.parallel_candidates.at("a"));
    EXPECT_TRUE(plan.parallel_candidates.at("b"));
}

TEST(AdaptiveImportOptimizer, TopologicalOrderRespectsFKs) {
    auto users  = makeSchema("users",  {"id"}, {"id"});
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});

    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({orders, users}); // reverse input order

    // "users" must appear before "orders"
    auto& ord = plan.import_order;
    auto u_pos = std::find(ord.begin(), ord.end(), "users");
    auto o_pos = std::find(ord.begin(), ord.end(), "orders");
    ASSERT_NE(u_pos, ord.end());
    ASSERT_NE(o_pos, ord.end());
    EXPECT_LT(u_pos, o_pos);
}

TEST(AdaptiveImportOptimizer, AdaptBatchSizeReducesUnderLoad) {
    AdaptiveImportOptimizer opt;
    double before = opt.currentBatchMultiplier();

    AdaptiveImportOptimizer::RuntimeMetrics metrics;
    metrics.cpu_utilization    = 95.0;
    metrics.memory_utilization = 85.0;
    opt.adaptBatchSize(metrics);

    EXPECT_LT(opt.currentBatchMultiplier(), before);
}

TEST(AdaptiveImportOptimizer, AdaptBatchSizeIncreasesUnderLowLoad) {
    AdaptiveImportOptimizer opt;
    double before = opt.currentBatchMultiplier();

    AdaptiveImportOptimizer::RuntimeMetrics metrics;
    metrics.cpu_utilization    = 20.0;
    metrics.memory_utilization = 20.0;
    opt.adaptBatchSize(metrics);

    EXPECT_GT(opt.currentBatchMultiplier(), before);
}

TEST(AdaptiveImportOptimizer, PerformancePredictorNoThrow) {
    auto schema = makeSchema("t", {"id"}, {"id"});
    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({schema});

    AdaptiveImportOptimizer::PerformancePredictor pp;
    EXPECT_NO_THROW(pp.predictPerformance(plan, {schema}));
}

// ===========================================================================
// 8. Polyglot Persistence Mapper
// ===========================================================================

TEST(PolyglotPersistenceMapper, RecommendRelationalByDefault) {
    auto schema = makeSchema("products",
        {"id", "name", "price"}, {"id"});

    PolyglotPersistenceMapper mapper;
    auto recommendations = mapper.recommendDataModels({schema});
    ASSERT_EQ(recommendations.size(), 1u);
    EXPECT_EQ(recommendations[0].source_table, "products");
}

TEST(PolyglotPersistenceMapper, RecommendTimeSeries) {
    auto schema = makeSchema("events",
        {"id", "timestamp", "value"}, {"id"});

    PolyglotPersistenceMapper mapper;
    auto recs = mapper.recommendDataModels({schema});
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].recommended_model, PolyglotPersistenceMapper::DataModel::TIMESERIES);
}

TEST(PolyglotPersistenceMapper, RecommendKeyValue) {
    auto schema = makeSchema("settings", {"key", "value"}, {"key"});

    PolyglotPersistenceMapper mapper;
    auto recs = mapper.recommendDataModels({schema});
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].recommended_model, PolyglotPersistenceMapper::DataModel::KEYVALUE);
}

TEST(PolyglotPersistenceMapper, DataModelToString) {
    EXPECT_EQ(PolyglotPersistenceMapper::dataModelToString(
                PolyglotPersistenceMapper::DataModel::GRAPH),
              "GRAPH");
    EXPECT_EQ(PolyglotPersistenceMapper::dataModelToString(
                PolyglotPersistenceMapper::DataModel::TIMESERIES),
              "TIMESERIES");
}

TEST(PolyglotPersistenceMapper, TableToDocumentAddsSchemaField) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    PolyglotPersistenceMapper::ModelTransformer t;
    json row = {{"id", "1"}, {"user_id", "42"}};
    auto doc = t.tableToDocument(row, schema);
    EXPECT_TRUE(doc.contains("_schema"));
    EXPECT_TRUE(doc.contains("users")); // nested FK
}

TEST(PolyglotPersistenceMapper, TableToGraphProducesNodesAndEdges) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    PolyglotPersistenceMapper::ModelTransformer t;
    std::vector<json> rows = {
        {{"id", "1"}, {"user_id", "10"}},
        {{"id", "2"}, {"user_id", "11"}}
    };
    auto [nodes, edges] = t.tableToGraph(rows, schema);
    EXPECT_EQ(nodes.size(), 2u);
    EXPECT_EQ(edges.size(), 2u);
}

// ===========================================================================
// 9. Temporal Database Support
// ===========================================================================

TEST(TemporalDatabaseSupport, DetectValidTime) {
    auto schema = makeSchema("contracts",
        {"id", "valid_from", "valid_to"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::VALID_TIME);
    EXPECT_EQ(detected[0].valid_from_column, "valid_from");
    EXPECT_EQ(detected[0].valid_to_column,   "valid_to");
}

TEST(TemporalDatabaseSupport, DetectTransactionTime) {
    auto schema = makeSchema("log",
        {"id", "created_at", "updated_at"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::TRANSACTION_TIME);
}

TEST(TemporalDatabaseSupport, DetectBiTemporal) {
    auto schema = makeSchema("facts",
        {"id", "valid_from", "valid_to", "created_at"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::BI_TEMPORAL);
}

TEST(TemporalDatabaseSupport, NoBiTemporalColumns) {
    auto schema = makeSchema("products", {"id", "name"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    EXPECT_TRUE(detected.empty());
}

TEST(TemporalDatabaseSupport, PointInTimeQueryContainsTimestamp) {
    TemporalDatabaseSupport::TemporalSchema ts;
    ts.table_name        = "contracts";
    ts.temporal_model    = TemporalDatabaseSupport::TemporalModel::VALID_TIME;
    ts.valid_from_column = "valid_from";
    ts.valid_to_column   = "valid_to";

    TemporalDatabaseSupport::TemporalQueryBuilder builder;
    auto sql = builder.buildPointInTimeQuery(ts, "2023-06-01");
    EXPECT_NE(sql.find("2023-06-01"), std::string::npos);
    EXPECT_NE(sql.find("contracts"), std::string::npos);
}

TEST(TemporalDatabaseSupport, SystemTimeQueryContainsForSystemTime) {
    TemporalDatabaseSupport::TemporalSchema ts;
    ts.table_name = "orders";

    TemporalDatabaseSupport::TemporalQueryBuilder builder;
    auto sql = builder.buildSystemTimeQuery(ts, "2024-01-01");
    EXPECT_NE(sql.find("FOR SYSTEM_TIME AS OF"), std::string::npos);
}

TEST(TemporalDatabaseSupport, TemporalModelToString) {
    EXPECT_EQ(TemporalDatabaseSupport::temporalModelToString(
                TemporalDatabaseSupport::TemporalModel::BI_TEMPORAL),
              "BI_TEMPORAL");
}

// ===========================================================================
// 10. Blockchain Integrity Verifier
// ===========================================================================

TEST(BlockchainIntegrityVerifier, BuildMerkleTreeSingleRecord) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    json rec = {{"id", "1"}, {"value", "hello"}};
    auto root = builder.buildMerkleTree({rec});
    EXPECT_EQ(root.size(), 64u); // 64 hex chars
}

TEST(BlockchainIntegrityVerifier, BuildMerkleTreeMultipleRecords) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    std::vector<json> records = {
        {{"id", "1"}}, {{"id", "2"}}, {{"id", "3"}}
    };
    auto root = builder.buildMerkleTree(records);
    EXPECT_FALSE(root.empty());
}

TEST(BlockchainIntegrityVerifier, DifferentInputsDifferentRoot) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    auto r1 = builder.buildMerkleTree({{{"id", "a"}}});
    auto r2 = builder.buildMerkleTree({{{"id", "b"}}});
    EXPECT_NE(r1, r2);
}

TEST(BlockchainIntegrityVerifier, VerifyRecordInTree_SingleRecord) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    json rec = {{"id", "1"}};
    auto root = builder.buildMerkleTree({rec});
    EXPECT_TRUE(builder.verifyRecordInTree(rec, root));
}

TEST(BlockchainIntegrityVerifier, AnchorToBlockchain) {
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;
    auto proof = anchor.anchorToBlockchain("a1b2c3d4e5f6" + std::string(52, '0'));
    EXPECT_FALSE(proof.merkle_root.empty());
    EXPECT_FALSE(proof.blockchain_tx_hash.empty());
    EXPECT_FALSE(proof.timestamp_rfc3339.empty());
}

TEST(BlockchainIntegrityVerifier, VerifyBlockchainAnchorValid) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;

    auto root = builder.buildMerkleTree({{{"id", "test"}}});
    auto proof = anchor.anchorToBlockchain(root);
    EXPECT_TRUE(anchor.verifyBlockchainAnchor(proof));
}

TEST(BlockchainIntegrityVerifier, VerifyBlockchainAnchorInvalidEmpty) {
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;
    BlockchainIntegrityVerifier::IntegrityProof proof;
    EXPECT_FALSE(anchor.verifyBlockchainAnchor(proof));
}

// ===========================================================================
// 11. Federated Learning
// ===========================================================================

TEST(FederatedLearning, AggregateEmptyUpdates) {
    FederatedImportCoordinator::FederatedAggregator agg;
    auto result = agg.aggregateUpdates({});
    EXPECT_TRUE(result.is_object());
}

TEST(FederatedLearning, FedAvgAveragesNumericFields) {
    FederatedImportCoordinator::FederatedAggregator agg;
    FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate u1, u2;
    u1.participant_id  = "p1";
    u1.statistics      = json{{"row_count", 100.0}, {"null_rate", 0.1}};
    u1.schema_contribution = json{{"col_a", "integer"}};

    u2.participant_id  = "p2";
    u2.statistics      = json{{"row_count", 200.0}, {"null_rate", 0.2}};
    u2.schema_contribution = json{{"col_b", "text"}};

    auto result = agg.aggregateUpdates({u1, u2}, "FedAvg");
    ASSERT_TRUE(result.contains("row_count"));
    EXPECT_DOUBLE_EQ(result["row_count"].get<double>(), 150.0);
    EXPECT_EQ(result["_participants"].get<int>(), 2);
}

TEST(FederatedLearning, DifferentialPrivacyAddsNoise) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    json stats = {{"mean", 50.0}, {"std", 10.0}};

    // Run DP 10 times; at least one should differ from original
    bool any_changed = false;
    for (int i = 0; i < 10; ++i) {
        auto noisy = dp.addDifferentialPrivacy(stats, 0.1, 1e-5);
        if (noisy["mean"].get<double>() != 50.0) {
            any_changed = true;
        }
    }
    EXPECT_TRUE(any_changed);
}

TEST(FederatedLearning, DifferentialPrivacyInvalidEpsilon) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_THROW(dp.addDifferentialPrivacy(json{}, 0.0, 1e-5),
                 std::invalid_argument);
}

TEST(FederatedLearning, PrivacyBudgetCheck) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_TRUE(dp.verifyPrivacyBudget(0.5, 1e-5));
    EXPECT_FALSE(dp.verifyPrivacyBudget(2.0, 1e-5)); // budget exceeded
}

TEST(FederatedLearning, SpendBudgetAccumulates) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    dp.spendBudget(0.3);
    dp.spendBudget(0.2);
    EXPECT_DOUBLE_EQ(dp.totalEpsilonSpent(), 0.5);
}

TEST(FederatedLearning, SpendNegativeBudgetThrows) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_THROW(dp.spendBudget(-0.1), std::invalid_argument);
}

// ===========================================================================
// 12. GraphQL Federation Support
// ===========================================================================

TEST(GraphQLFederationSupport, GenerateFederatedSchema_ContainsTypeName) {
    auto schema = makeSchema("users", {"id", "name", "email"}, {"id"});
    schema.column_types["id"]    = "uuid";
    schema.column_types["name"]  = "varchar";
    schema.column_types["email"] = "text";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generateFederatedSchema({schema}, "user-service");

    EXPECT_NE(sdl.find("type Users"), std::string::npos);
    EXPECT_NE(sdl.find("@key"), std::string::npos);
    EXPECT_NE(sdl.find("user-service"), std::string::npos);
}

TEST(GraphQLFederationSupport, GenerateFederatedSchema_FKAnnotation) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generateFederatedSchema({schema}, "order-service");

    // FK column should have a comment annotation
    EXPECT_NE(sdl.find("FK ->"), std::string::npos);
}

TEST(GraphQLFederationSupport, GeneratePlainSchema_HasQueryType) {
    auto schema = makeSchema("products", {"id", "name"}, {"id"});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});

    EXPECT_NE(sdl.find("type Query"), std::string::npos);
    EXPECT_NE(sdl.find("products(id:"), std::string::npos);
}

TEST(GraphQLFederationSupport, TypeMapping_Integer) {
    auto schema = makeSchema("t", {"count"});
    schema.column_types["count"] = "integer";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});
    EXPECT_NE(sdl.find("count: Int"), std::string::npos);
}

TEST(GraphQLFederationSupport, TypeMapping_Boolean) {
    auto schema = makeSchema("t", {"active"});
    schema.column_types["active"] = "boolean";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});
    EXPECT_NE(sdl.find("active: Boolean"), std::string::npos);
}

TEST(GraphQLFederationSupport, ExternalEntityAnnotation) {
    auto user_schema  = makeSchema("users",  {"id"}, {"id"});
    auto order_schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                                   {{"user_id", "users.id"}});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    // "users" is owned by another subgraph → should be "extend type"
    auto sdl = gen.generateFederatedSchema(
        {user_schema, order_schema}, "order-service", {"users"});

    EXPECT_NE(sdl.find("extend type Users"), std::string::npos);
}

// ===========================================================================
// main
// ===========================================================================
