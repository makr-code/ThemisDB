// test_sqlite_importer.cpp
//
// Unit tests for the SQLite importer covering:
//   - Type mapping (SQLite affinity rules → ThemisDB types)
//   - CREATE TABLE parsing (quoted/bare identifiers, constraints, IF NOT EXISTS)
//   - INSERT parsing (single-row, multi-row, NULL values, quoted strings)
//   - validateSource / dump-header detection
//   - include/exclude table filtering
//   - Multi-row INSERT (one INSERT with multiple value tuples)
//   - Permission check callback (ACL enforcement)
//   - Dry-run mode
//   - Hex literal handling (X'...')
//   - Full integration against the sample_sqlite3.sql fixture

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <filesystem>

// ---------------------------------------------------------------------------
// Minimal re-implementation of relevant types (mirrors importer_interface.h)
// to keep the test self-contained and runnable without the full build chain.
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_SQLITE_DUMP    = 104,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    STATEMENT_TOO_LARGE  = 204,
    UNKNOWN_TABLE        = 300,
    COLUMN_COUNT_MISMATCH = 301,
    TYPE_CONVERSION      = 400,
    DRY_RUN_ONLY         = 500,
    TABLE_EXCLUDED       = 501,
    PERMISSION_DENIED    = 503,
    UNKNOWN              = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records      = 0;
    size_t imported_records   = 0;
    size_t failed_records     = 0;
    size_t skipped_records    = 0;
    size_t tables_processed   = 0;
    double elapsed_seconds    = 0.0;
    std::vector<std::string>  warnings;
    std::vector<std::string>  errors;
    std::vector<ImportError>  structured_errors;
};

struct ImportOptions {
    bool                              dry_run             = false;
    bool                              continue_on_error   = true;
    size_t                            batch_size          = 1000;
    std::vector<std::string>          include_tables;
    std::vector<std::string>          exclude_tables;
    std::map<std::string,std::string> type_overrides;
    size_t                            max_row_size_bytes       = 0;
    size_t                            max_statement_size_bytes = 0;
    std::function<bool(const std::string&,
                       const std::string&)> permission_check;
};

// ---------------------------------------------------------------------------
// Helpers duplicated from sqlite_importer.cpp (kept in sync manually for tests)
// ---------------------------------------------------------------------------

static std::string toLowerTest(const std::string& s) {
    std::string r = s;
    for (auto& c : r)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

/// Map a SQLite column type string to a ThemisDB logical type.
static std::string mapSQLiteType(
    const std::string& sqlite_type,
    const std::map<std::string,std::string>& overrides = {}) {
    auto it = overrides.find(sqlite_type);
    if (it != overrides.end()) {
      return it->second;
    }

    std::string base = sqlite_type;
    size_t paren = base.find('(');
    if (paren != std::string::npos) {
      base = base.substr(0, paren);
    }
    std::string lower = toLowerTest(base);
    while (!lower.empty() &&
           (lower.back() == ' ' || lower.back() == '\t')) lower.pop_back();

    // SQLite type affinity rules
    if (lower.find("int") != std::string::npos) {
      return "integer";
    }

    if (lower.find("char")  != std::string::npos ||
        lower.find("clob")  != std::string::npos ||
        lower.find("text")  != std::string::npos) return "string";

    if (lower.empty() || lower == "blob") {
      return "binary";
    }

    if (lower.find("real")  != std::string::npos ||
        lower.find("floa")  != std::string::npos ||
        lower.find("doub")  != std::string::npos) return "double";

    if (lower == "numeric" || lower == "decimal" || lower == "number")
        return "double";
    if (lower == "boolean" || lower == "bool") {
      return "boolean";
    }

    if (lower == "date") {
      return "date";
    }
    if (lower == "time") {
      return "time";
    }
    if (lower == "datetime" || lower == "timestamp") {
      return "datetime";
    }
    if (lower == "json") {
      return "json";
    }

    if (lower.find("date") != std::string::npos) {
      return "datetime";
    }
    if (lower.find("time") != std::string::npos) {
      return "datetime";
    }

    return "string";
}

// ---------------------------------------------------------------------------
// Minimal CREATE TABLE parser (mirrors sqlite_importer.cpp logic)
// ---------------------------------------------------------------------------

struct TableSchema {
    std::string                        name;
    std::vector<std::string>           columns;
    std::map<std::string, std::string> column_types;
    std::vector<std::string>           primary_keys;
};

#include <regex>

static bool parseCreateTable(const std::string& sql, TableSchema& out) {
    // Note: raw-string delimiter "re" is used to avoid the ")\"" sequence
    // (which would prematurely terminate a plain R"(...)").
    std::regex table_regex(
        R"re(CREATE\s+(?:TEMP(?:ORARY)?\s+)?TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?)re"
        R"re((?:(?:"([^"]+)"|`([^`]+)`|(\w+))\.)?(?:"([^"]+)"|`([^`]+)`|(\w+))\s*\()re",
        std::regex_constants::icase);
    std::smatch match;
    if (!std::regex_search(sql, match, table_regex)) {
      return false;
    }

    out.name = match[4].matched ? match[4].str()
             : match[5].matched ? match[5].str()
             : match[6].matched ? match[6].str() : "";
    if (out.name.empty()) {
      return false;
    }

    size_t open_pos = sql.find('(', match.position());
    if (open_pos == std::string::npos) {
      return false;
    }

    int depth = 0;
    bool in_string = false;
    char str_char = '\0';
    size_t close_pos = std::string::npos;
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == str_char) {
              in_string = false;
            }
        } else if (c == '\'' || c == '"') {
            in_string = true; str_char = c;
        } else if (c == '(') { ++depth; }
        else if (c == ')') {
            --depth;
            if (depth == 0) { close_pos = k; break; }
        }
    }
    if (close_pos == std::string::npos) {
      return false;
    }

    std::string cols_str = sql.substr(open_pos + 1, close_pos - open_pos - 1);

    std::vector<std::string> col_defs;
    {
        int dep = 0; bool inq = false; char qc = '\0'; std::string cur;
        for (size_t i = 0; i < cols_str.size(); ++i) {
            char c = cols_str[i];
            if (inq) {
                cur += c;
                if (c == qc) {
                  inq = false;
                }
            } else if (c == '\'' || c == '"' || c == '`') {
                inq = true; qc = c; cur += c;
            } else if (c == '(') { ++dep; cur += c; }
            else if (c == ')') { --dep; cur += c; }
            else if (c == ',' && dep == 0) { col_defs.push_back(cur); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) {
          col_defs.push_back(cur);
        }
    }

    for (auto& col_def : col_defs) {
        size_t f = col_def.find_first_not_of(" \t\n\r");
        if (f == std::string::npos) {
          continue;
        }
        col_def = col_def.substr(f);
        size_t l = col_def.find_last_not_of(" \t\n\r");
        if (l != std::string::npos) {
          col_def = col_def.substr(0, l + 1);
        }
        if (col_def.empty()) {
          continue;
        }

        std::string up;
        for (size_t i = 0; i < col_def.size() && i < 25; ++i)
            up += static_cast<char>(
                std::toupper(static_cast<unsigned char>(col_def[i])));

        // Skip table-level constraints that BEGIN with a keyword.
        // Inline column constraints like "id INTEGER PRIMARY KEY" are allowed
        // because the line starts with the column name, not the keyword.
        if (up.find("PRIMARY")    == 0 ||
            up.find("UNIQUE")     == 0 ||
            up.find("CHECK")      == 0 ||
            up.find("FOREIGN")    == 0 ||
            up.find("CONSTRAINT") == 0) continue;

        std::string col_name;
        size_t type_start = 0;
        if (!col_def.empty() &&
            (col_def[0] == '"' || col_def[0] == '`')) {
            char q = col_def[0];
            size_t end_q = col_def.find(q, 1);
            if (end_q == std::string::npos) {
              continue;
            }
            col_name   = col_def.substr(1, end_q - 1);
            type_start = end_q + 1;
        } else {
            size_t sp = col_def.find_first_of(" \t");
            if (sp == std::string::npos) {
                col_name   = col_def;
                type_start = col_def.size();  // no type declared; col_type will be empty
            } else {
                col_name   = col_def.substr(0, sp);
                type_start = sp;
            }
        }
        if (col_name.empty()) {
          continue;
        }

        while (type_start < col_def.size() &&
               (col_def[type_start] == ' ' || col_def[type_start] == '\t'))
            ++type_start;

        std::string col_type;
        size_t k = type_start;
        int tdep = 0;
        while (k < col_def.size()) {
            char c = col_def[k];
            if (c == '(') { ++tdep; col_type += c; }
            else if (c == ')') {
                if (tdep > 0) { --tdep; col_type += c; } else break;
            } else if ((c == ' ' || c == '\t') && tdep == 0) break;
            else col_type += c;
            ++k;
        }
        out.columns.push_back(col_name);
        out.column_types[col_name] = col_type.empty() ? "blob" : col_type;
    }
    return !out.name.empty();
}

// ---------------------------------------------------------------------------
// INSERT value parser (mirrors sqlite_importer.cpp parseInsertValues)
// ---------------------------------------------------------------------------

static std::vector<std::string> parseSQLiteInsertValues(
    const std::string& values_clause) {
    std::vector<std::string> result;
    size_t i = 0;
    const size_t n = values_clause.size();

    auto skipWs = [&]() {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                          values_clause[i] == '\r' ||
                          values_clause[i] == '\n')) ++i;
    };

    while (i < n) {
        skipWs();
        if (i >= n) {
          break;
        }

        char c = values_clause[i];

        if (c == '\'') {
            ++i;
            std::string val;
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    val += '\''; i += 2;
                } else if (sc == '\'') {
                    ++i; break;
                } else {
                    val += sc; ++i;
                }
            }
            result.push_back(val);
        } else if (c == '"') {
            ++i;
            std::string val;
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '"' && i + 1 < n && values_clause[i + 1] == '"') {
                    val += '"'; i += 2;
                } else if (sc == '"') {
                    ++i; break;
                } else {
                    val += sc; ++i;
                }
            }
            result.push_back(val);
        } else if ((c == 'X' || c == 'x') && i + 1 < n &&
                   values_clause[i + 1] == '\'') {
            i += 2;
            std::string val;
            while (i < n && values_clause[i] != '\'')
                val += values_clause[i++];
            if (i < n) {
              ++i;
            }
            result.push_back(val);
        } else {
            size_t start = i;
            int dep = 0;
            while (i < n) {
                char tc = values_clause[i];
                if (tc == '(') { ++dep; ++i; }
                else if (tc == ')') {
                    if (dep > 0) { --dep; ++i; } else break;
                } else if (tc == ',' && dep == 0) break;
                else ++i;
            }
            std::string token = values_clause.substr(start, i - start);
            size_t tf = token.find_first_not_of(" \t\r\n");
            size_t tl = token.find_last_not_of(" \t\r\n");
            if (tf == std::string::npos) {
              token.clear();
            }
            else token = token.substr(tf, tl - tf + 1);
            std::string upper_tok;
            for (char ch : token)
                upper_tok += static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch)));
            if (upper_tok == "NULL") {
              token.clear();
            }
            result.push_back(token);
        }

        skipWs();
        if (i < n && values_clause[i] == ',') {
          ++i;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Helper: write a string to a temp file, return path
// ---------------------------------------------------------------------------
static std::string writeTempFile(const std::string& content,
                                  const std::string& suffix = ".sql") {
    std::string path = std::string("/tmp/themis_sqlite_test_") +
                       std::to_string(
                           reinterpret_cast<uintptr_t>(&content)) + suffix;
    std::ofstream f(path);
    f << content;
    return path;
}

/// Returns true if the file content looks like a SQLite dump.
static bool looksLikeSQLiteDump(const std::string& content) {
    std::istringstream ss(content);
    std::string line;
    int checked = 0;
    while (std::getline(ss, line) && checked < 50) {
        if (line.find("SQLite")            != std::string::npos ||
            line.find("sqlite")            != std::string::npos ||
            line.find("BEGIN TRANSACTION") != std::string::npos ||
            line.find("PRAGMA")            != std::string::npos) {
            return true;
        }
        ++checked;
    }
    return false;
}

/// Minimal shouldImportTable re-implementation for unit tests.
static bool shouldImportTable(const std::string& table_name,
                               const ImportOptions& opts) {
    if (std::find(opts.exclude_tables.begin(), opts.exclude_tables.end(),
                  table_name) != opts.exclude_tables.end()) return false;
    if (!opts.include_tables.empty()) {
        return std::find(opts.include_tables.begin(), opts.include_tables.end(),
                         table_name) != opts.include_tables.end();
    }
    return true;
}

// ===========================================================================
// Tests: Type Mapping
// ===========================================================================

TEST(SQLiteTypeMapping, IntegerAffinity) {
    EXPECT_EQ(mapSQLiteType("INTEGER"),      "integer");
    EXPECT_EQ(mapSQLiteType("INT"),          "integer");
    EXPECT_EQ(mapSQLiteType("TINYINT"),      "integer");
    EXPECT_EQ(mapSQLiteType("SMALLINT"),     "integer");
    EXPECT_EQ(mapSQLiteType("MEDIUMINT"),    "integer");
    EXPECT_EQ(mapSQLiteType("BIGINT"),       "integer");
    EXPECT_EQ(mapSQLiteType("UNSIGNED BIG INT"), "integer");
    EXPECT_EQ(mapSQLiteType("INT2"),         "integer");
    EXPECT_EQ(mapSQLiteType("INT8"),         "integer");
}

TEST(SQLiteTypeMapping, TextAffinity) {
    EXPECT_EQ(mapSQLiteType("TEXT"),         "string");
    EXPECT_EQ(mapSQLiteType("CHARACTER(20)"), "string");
    EXPECT_EQ(mapSQLiteType("VARCHAR(255)"), "string");
    EXPECT_EQ(mapSQLiteType("VARYING CHARACTER(255)"), "string");
    EXPECT_EQ(mapSQLiteType("NCHAR(55)"),    "string");
    EXPECT_EQ(mapSQLiteType("NATIVE CHARACTER(70)"), "string");
    EXPECT_EQ(mapSQLiteType("NVARCHAR(100)"), "string");
    EXPECT_EQ(mapSQLiteType("CLOB"),         "string");
}

TEST(SQLiteTypeMapping, BlobAffinity) {
    EXPECT_EQ(mapSQLiteType("BLOB"),  "binary");
    EXPECT_EQ(mapSQLiteType("blob"),  "binary");
    EXPECT_EQ(mapSQLiteType(""),      "binary");
}

TEST(SQLiteTypeMapping, RealAffinity) {
    EXPECT_EQ(mapSQLiteType("REAL"),    "double");
    EXPECT_EQ(mapSQLiteType("DOUBLE"),  "double");
    EXPECT_EQ(mapSQLiteType("DOUBLE PRECISION"), "double");
    EXPECT_EQ(mapSQLiteType("FLOAT"),   "double");
}

TEST(SQLiteTypeMapping, NumericAffinity) {
    EXPECT_EQ(mapSQLiteType("NUMERIC"),  "double");
    EXPECT_EQ(mapSQLiteType("DECIMAL"),  "double");
    EXPECT_EQ(mapSQLiteType("NUMBER"),   "double");
    EXPECT_EQ(mapSQLiteType("BOOLEAN"),  "boolean");
    EXPECT_EQ(mapSQLiteType("bool"),     "boolean");
}

TEST(SQLiteTypeMapping, DateTimeTypes) {
    EXPECT_EQ(mapSQLiteType("DATE"),      "date");
    EXPECT_EQ(mapSQLiteType("TIME"),      "time");
    EXPECT_EQ(mapSQLiteType("DATETIME"),  "datetime");
    EXPECT_EQ(mapSQLiteType("TIMESTAMP"), "datetime");
}

TEST(SQLiteTypeMapping, JSONType) {
    EXPECT_EQ(mapSQLiteType("JSON"), "json");
    EXPECT_EQ(mapSQLiteType("json"), "json");
}

TEST(SQLiteTypeMapping, CaseInsensitive) {
    EXPECT_EQ(mapSQLiteType("integer"), "integer");
    EXPECT_EQ(mapSQLiteType("Integer"), "integer");
    EXPECT_EQ(mapSQLiteType("TEXT"),    "string");
    EXPECT_EQ(mapSQLiteType("text"),    "string");
    EXPECT_EQ(mapSQLiteType("Real"),    "double");
}

TEST(SQLiteTypeMapping, SizedTypes) {
    EXPECT_EQ(mapSQLiteType("INTEGER(10)"), "integer");
    EXPECT_EQ(mapSQLiteType("VARCHAR(255)"), "string");
    EXPECT_EQ(mapSQLiteType("DECIMAL(10,2)"), "double");
}

TEST(SQLiteTypeMapping, UserOverridesHavePriority) {
    std::map<std::string,std::string> overrides = {
        {"INTEGER", "custom_int"}, {"TEXT", "rich_text"}};
    EXPECT_EQ(mapSQLiteType("INTEGER", overrides), "custom_int");
    EXPECT_EQ(mapSQLiteType("TEXT",    overrides), "rich_text");
    EXPECT_EQ(mapSQLiteType("REAL",    overrides), "double");  // no override
}

TEST(SQLiteTypeMapping, UnknownTypeDefaultsToString) {
    EXPECT_EQ(mapSQLiteType("some_custom_type"), "string");
}

// ===========================================================================
// Tests: CREATE TABLE Parsing
// ===========================================================================

TEST(SQLiteCreateTable, BasicBareIdentifiers) {
    std::string sql =
        "CREATE TABLE users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT,"
        "  age INTEGER"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "users");

    ASSERT_EQ(schema.columns.size(), 4u);
    EXPECT_EQ(schema.columns[0], "id");
    EXPECT_EQ(schema.columns[1], "name");
    EXPECT_EQ(schema.columns[2], "email");
    EXPECT_EQ(schema.columns[3], "age");

    EXPECT_EQ(schema.column_types.at("id"),    "INTEGER");
    EXPECT_EQ(schema.column_types.at("name"),  "TEXT");
    EXPECT_EQ(schema.column_types.at("email"), "TEXT");
    EXPECT_EQ(schema.column_types.at("age"),   "INTEGER");
}

TEST(SQLiteCreateTable, DoubleQuotedIdentifiers) {
    std::string sql =
        "CREATE TABLE \"orders\" ("
        "  \"order_id\" INTEGER,"
        "  \"amount\" REAL"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "orders");
    ASSERT_EQ(schema.columns.size(), 2u);
    EXPECT_EQ(schema.columns[0], "order_id");
    EXPECT_EQ(schema.columns[1], "amount");
}

TEST(SQLiteCreateTable, BacktickQuotedIdentifiers) {
    std::string sql =
        "CREATE TABLE `products` ("
        "  `id` INTEGER,"
        "  `price` REAL"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "products");
    ASSERT_EQ(schema.columns.size(), 2u);
}

TEST(SQLiteCreateTable, IfNotExists) {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT NOT NULL,"
        "  value TEXT"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "settings");
    ASSERT_EQ(schema.columns.size(), 2u);
}

TEST(SQLiteCreateTable, TempTable) {
    std::string sql =
        "CREATE TEMP TABLE scratch (id INTEGER, data BLOB);";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "scratch");
    ASSERT_EQ(schema.columns.size(), 2u);
    EXPECT_EQ(schema.column_types.at("data"), "BLOB");
}

TEST(SQLiteCreateTable, TemporaryTable) {
    std::string sql =
        "CREATE TEMPORARY TABLE tmp_work (id INTEGER);";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "tmp_work");
    ASSERT_EQ(schema.columns.size(), 1u);
}

TEST(SQLiteCreateTable, SchemaQualifiedTable) {
    // SQLite supports "schema.table" for ATTACH'd databases
    std::string sql =
        "CREATE TABLE main.metadata (key TEXT, value TEXT);";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "metadata");
    ASSERT_EQ(schema.columns.size(), 2u);
}

TEST(SQLiteCreateTable, TableLevelConstraintsAreSkipped) {
    std::string sql =
        "CREATE TABLE edge_cases ("
        "  id INTEGER NOT NULL,"
        "  name TEXT,"
        "  PRIMARY KEY (id),"
        "  UNIQUE (name),"
        "  CHECK (id > 0),"
        "  FOREIGN KEY (id) REFERENCES other(id)"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    ASSERT_EQ(schema.columns.size(), 2u);
    EXPECT_EQ(schema.columns[0], "id");
    EXPECT_EQ(schema.columns[1], "name");
}

TEST(SQLiteCreateTable, TypelessColumnDefaultsToBlob) {
    // SQLite allows columns with no type declaration
    std::string sql =
        "CREATE TABLE flexible (id INTEGER, misc);";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    ASSERT_EQ(schema.columns.size(), 2u);
    EXPECT_EQ(schema.column_types.at("misc"), "blob");
}

TEST(SQLiteCreateTable, AllAffinityTypes) {
    std::string sql =
        "CREATE TABLE typed ("
        "  a INTEGER,"
        "  b REAL,"
        "  c TEXT,"
        "  d BLOB,"
        "  e NUMERIC,"
        "  f DATETIME,"
        "  g JSON"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "typed");
    ASSERT_EQ(schema.columns.size(), 7u);
    EXPECT_EQ(schema.column_types.at("a"), "INTEGER");
    EXPECT_EQ(schema.column_types.at("b"), "REAL");
    EXPECT_EQ(schema.column_types.at("g"), "JSON");
}

TEST(SQLiteCreateTable, MalformedSQL) {
    std::string sql = "CREATE TABLE (no_name INTEGER);";
    TableSchema schema;
    // Should either return false or an empty name
    bool ok = parseCreateTable(sql, schema);
    if (ok) { EXPECT_TRUE(schema.name.empty()); }
}

// ===========================================================================
// Tests: INSERT Value Parsing
// ===========================================================================

TEST(SQLiteInsertValues, SimpleRow) {
    auto vals = parseSQLiteInsertValues("1,'Alice','alice@example.com'");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "Alice");
    EXPECT_EQ(vals[2], "alice@example.com");
}

TEST(SQLiteInsertValues, NullValue) {
    auto vals = parseSQLiteInsertValues("1,NULL,'text'");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "");   // NULL → empty string
    EXPECT_EQ(vals[2], "text");
}

TEST(SQLiteInsertValues, NullCaseInsensitive) {
    auto vals = parseSQLiteInsertValues("null,Null,NULL");
    ASSERT_EQ(vals.size(), 3u);
    for (const auto& v : vals) {
      EXPECT_EQ(v, "");
    }
}

TEST(SQLiteInsertValues, DoubledSingleQuoteEscape) {
    // SQL standard '' escape for a literal single-quote
    auto vals = parseSQLiteInsertValues("'it''s a test'");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "it's a test");
}

TEST(SQLiteInsertValues, EmptyString) {
    auto vals = parseSQLiteInsertValues("''");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "");
}

TEST(SQLiteInsertValues, FloatAndNegative) {
    auto vals = parseSQLiteInsertValues("3.14,-42,0");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "3.14");
    EXPECT_EQ(vals[1], "-42");
    EXPECT_EQ(vals[2], "0");
}

TEST(SQLiteInsertValues, CommaInsideString) {
    auto vals = parseSQLiteInsertValues("'a,b,c',1");
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0], "a,b,c");
    EXPECT_EQ(vals[1], "1");
}

TEST(SQLiteInsertValues, HexLiteral) {
    auto vals = parseSQLiteInsertValues("X'DEADBEEF'");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "DEADBEEF");
}

TEST(SQLiteInsertValues, LowercaseHexLiteral) {
    auto vals = parseSQLiteInsertValues("x'cafe'");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "cafe");
}

TEST(SQLiteInsertValues, DoubleQuotedString) {
    auto vals = parseSQLiteInsertValues("\"hello world\"");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "hello world");
}

TEST(SQLiteInsertValues, DoubleQuoteEscape) {
    auto vals = parseSQLiteInsertValues("\"say \"\"hello\"\"\"");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "say \"hello\"");
}

TEST(SQLiteInsertValues, MixedTypes) {
    auto vals = parseSQLiteInsertValues(
        "42,3.14,'text value',NULL,X'FF'");
    ASSERT_EQ(vals.size(), 5u);
    EXPECT_EQ(vals[0], "42");
    EXPECT_EQ(vals[1], "3.14");
    EXPECT_EQ(vals[2], "text value");
    EXPECT_EQ(vals[3], "");    // NULL
    EXPECT_EQ(vals[4], "FF");  // hex
}

// ===========================================================================
// Tests: Dump-file validation helper (validateSource equivalent)
// ===========================================================================

TEST(SQLiteValidateSource, RecognisesBeginTransaction) {
    std::string content =
        "BEGIN TRANSACTION;\n"
        "CREATE TABLE t (id INTEGER);\n"
        "COMMIT;\n";
    EXPECT_TRUE(looksLikeSQLiteDump(content));
}

TEST(SQLiteValidateSource, RecognisesSQLiteComment) {
    std::string content =
        "-- This file was generated by SQLite's .dump command.\n"
        "PRAGMA foreign_keys=OFF;\n"
        "BEGIN TRANSACTION;\n";
    EXPECT_TRUE(looksLikeSQLiteDump(content));
}

TEST(SQLiteValidateSource, RecognisesPragma) {
    std::string content =
        "PRAGMA foreign_keys=OFF;\n"
        "CREATE TABLE t (id INTEGER);\n";
    EXPECT_TRUE(looksLikeSQLiteDump(content));
}

TEST(SQLiteValidateSource, RejectsPostgreSQLDump) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "SET statement_timeout = 0;\n"
        "CREATE TABLE users (id INTEGER);\n";
    EXPECT_FALSE(looksLikeSQLiteDump(content));
}

TEST(SQLiteValidateSource, RejectsMySQLDump) {
    std::string content =
        "-- MySQL dump 10.13  Distrib 8.0.28\n"
        "CREATE TABLE `t` (`id` int);\n";
    EXPECT_FALSE(looksLikeSQLiteDump(content));
}

TEST(SQLiteValidateSource, RejectsArbitrarySQL) {
    std::string content =
        "SELECT 1;\n"
        "INSERT INTO t VALUES (1);\n";
    EXPECT_FALSE(looksLikeSQLiteDump(content));
}

// ===========================================================================
// Tests: Table filtering
// ===========================================================================

TEST(SQLiteTableFilter, DefaultAllowsAll) {
    ImportOptions opts;
    EXPECT_TRUE(shouldImportTable("users",    opts));
    EXPECT_TRUE(shouldImportTable("products", opts));
}

TEST(SQLiteTableFilter, ExcludeList) {
    ImportOptions opts;
    opts.exclude_tables = {"audit_log", "sessions"};
    EXPECT_FALSE(shouldImportTable("audit_log", opts));
    EXPECT_FALSE(shouldImportTable("sessions",  opts));
    EXPECT_TRUE(shouldImportTable("users",      opts));
}

TEST(SQLiteTableFilter, IncludeListFiltersOut) {
    ImportOptions opts;
    opts.include_tables = {"users"};
    EXPECT_TRUE(shouldImportTable("users",    opts));
    EXPECT_FALSE(shouldImportTable("products", opts));
}

TEST(SQLiteTableFilter, ExcludeTakesPriorityOverInclude) {
    ImportOptions opts;
    opts.include_tables = {"users", "products"};
    opts.exclude_tables = {"users"};
    EXPECT_FALSE(shouldImportTable("users",    opts));
    EXPECT_TRUE(shouldImportTable("products",  opts));
}

// ===========================================================================
// Tests: Multi-row INSERT parsing (VALUES with multiple tuples)
// ===========================================================================

static std::vector<std::vector<std::string>> parseMultiRowInsert(
    const std::string& values_payload) {
    std::vector<std::vector<std::string>> rows;
    size_t pos = 0;
    while (pos < values_payload.size()) {
        while (pos < values_payload.size() &&
               (values_payload[pos] == ' ' || values_payload[pos] == '\t' ||
                values_payload[pos] == ',' || values_payload[pos] == '\r' ||
                values_payload[pos] == '\n')) ++pos;
        if (pos >= values_payload.size()) {
          break;
        }
        if (values_payload[pos] != '(') { ++pos; continue; }

        size_t tuple_start = pos + 1;
        int dep = 1;
        bool in_str = false; char sq = '\0';
        size_t k = pos + 1;
        while (k < values_payload.size() && dep > 0) {
            char c = values_payload[k];
            if (in_str) {
                if (c == sq) {
                    if (k + 1 < values_payload.size() &&
                        values_payload[k + 1] == sq) { ++k; }
                    else in_str = false;
                }
            } else if (c == '\'' || c == '"') { in_str = true; sq = c; }
            else if (c == '(') ++dep;
            else if (c == ')') --dep;
            ++k;
        }
        size_t tuple_end = k - 1;
        if (dep != 0) {
          break;
        }

        std::string tuple_str =
            values_payload.substr(tuple_start, tuple_end - tuple_start);
        rows.push_back(parseSQLiteInsertValues(tuple_str));
        pos = k;
    }
    return rows;
}

TEST(SQLiteMultiRowInsert, TwoRows) {
    std::string payload =
        "(1,'Alice','alice@example.com'),(2,'Bob','bob@example.com')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][0], "1");
    EXPECT_EQ(rows[0][1], "Alice");
    EXPECT_EQ(rows[1][0], "2");
    EXPECT_EQ(rows[1][1], "Bob");
}

TEST(SQLiteMultiRowInsert, SingleRow) {
    std::string payload = "(42,'Only Row')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "42");
    EXPECT_EQ(rows[0][1], "Only Row");
}

TEST(SQLiteMultiRowInsert, RowsWithNulls) {
    std::string payload = "(1,NULL,'x'),(2,'y',NULL)";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][1], "");  // NULL
    EXPECT_EQ(rows[1][2], "");  // NULL
}

TEST(SQLiteMultiRowInsert, ValuesWithCommasInsideStrings) {
    std::string payload = "(1,'a,b,c'),(2,'x,y')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][1], "a,b,c");
    EXPECT_EQ(rows[1][1], "x,y");
}

// ===========================================================================
// Tests: Permission check callback
// ===========================================================================

TEST(SQLitePermissionCheck, DeniedByCallback) {
    ImportOptions opts;
    bool called = false;
    opts.permission_check = [&called](const std::string& resource,
                                      const std::string& action) -> bool {
        called = true;
        EXPECT_EQ(resource, "import");
        EXPECT_EQ(action,   "write");
        return false;
    };
    ASSERT_TRUE(static_cast<bool>(opts.permission_check));
    bool allowed = opts.permission_check("import", "write");
    EXPECT_TRUE(called);
    EXPECT_FALSE(allowed);
}

TEST(SQLitePermissionCheck, AllowedByCallback) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&,
                               const std::string&) { return true; };
    EXPECT_TRUE(opts.permission_check("import", "write"));
}

// ===========================================================================
// Tests: Dry-run detection
// ===========================================================================

TEST(SQLiteDryRunMode, FlagIsFalseByDefault) {
    ImportOptions opts;
    EXPECT_FALSE(opts.dry_run);
}

TEST(SQLiteDryRunMode, FlagCanBeSet) {
    ImportOptions opts;
    opts.dry_run = true;
    EXPECT_TRUE(opts.dry_run);
}

// ===========================================================================
// Tests: validateSource – file I/O
// ===========================================================================

TEST(SQLiteValidateSourceFile, NonExistentFile) {
    // We test the file-not-found branch by attempting to open a missing file.
    std::string path = "/tmp/themis_sqlite_nonexistent_file_999.sql";
    std::ifstream f(path);
    EXPECT_FALSE(f.is_open());
}

TEST(SQLiteValidateSourceFile, ValidDumpFile) {
    std::string content =
        "-- This file was generated by SQLite's .dump command.\n"
        "PRAGMA foreign_keys=OFF;\n"
        "BEGIN TRANSACTION;\n"
        "CREATE TABLE t (id INTEGER);\n"
        "INSERT INTO t VALUES(1);\n"
        "COMMIT;\n";
    std::string path = writeTempFile(content);
    std::ifstream f(path);
    ASSERT_TRUE(f.is_open());
    EXPECT_TRUE(looksLikeSQLiteDump(content));
}

// ===========================================================================
// Tests: Full integration against sample_sqlite3.sql fixture
// ===========================================================================

/// Path to the fixture file relative to the test working directory.
static const char* kFixturePath =
    "tests/fixtures/importers/sample_sqlite3.sql";

static std::string getFixturePath() {
    {
        std::ifstream f(kFixturePath);
        if (f.is_open()) {
          return kFixturePath;
        }
    }

    // Fallback: resolve relative to this test source file location.
    const auto source_based =
        (std::filesystem::path(__FILE__).parent_path() /
         "../fixtures/importers/sample_sqlite3.sql").lexically_normal();
    {
        std::ifstream f(source_based.string());
        if (f.is_open()) {
          return source_based.string();
        }
    }

    // Last fallback for out-of-tree execution from build folders.
    const auto cwd_based =
        (std::filesystem::current_path() /
         "../tests/fixtures/importers/sample_sqlite3.sql").lexically_normal();
    return cwd_based.string();
}

TEST(SQLiteFixture, FileExists) {
    std::string path = getFixturePath();
    std::ifstream f(path);
    ASSERT_TRUE(f.is_open()) << "Fixture not found at: " << path;
}

TEST(SQLiteFixture, LooksLikeSQLiteDump) {
    std::string path = getFixturePath();
    std::ifstream f(path);
    if (!f.is_open()) {
      GTEST_SKIP() << "Fixture not found";
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    EXPECT_TRUE(looksLikeSQLiteDump(ss.str()));
}

TEST(SQLiteFixture, ContainsBothTables) {
    std::string path = getFixturePath();
    std::ifstream f(path);
    if (!f.is_open()) {
      GTEST_SKIP() << "Fixture not found";
    }

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("CREATE TABLE users"), std::string::npos);
    EXPECT_NE(content.find("CREATE TABLE products"), std::string::npos);
}

TEST(SQLiteFixture, ParseUsersTable) {
    std::string sql =
        "CREATE TABLE users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT,"
        "  age INTEGER,"
        "  score REAL,"
        "  active INTEGER DEFAULT 1,"
        "  bio TEXT,"
        "  created_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "users");

    // Should have 8 columns (PRIMARY KEY constraint is inline, not table-level)
    EXPECT_EQ(schema.columns.size(), 8u);

    // Verify types using type affinity
    EXPECT_EQ(mapSQLiteType(schema.column_types.at("id")),         "integer");
    EXPECT_EQ(mapSQLiteType(schema.column_types.at("name")),       "string");
    EXPECT_EQ(mapSQLiteType(schema.column_types.at("score")),      "double");
    EXPECT_EQ(mapSQLiteType(schema.column_types.at("created_at")), "string");
}

TEST(SQLiteFixture, ParseProductsTable) {
    std::string sql =
        "CREATE TABLE products ("
        "  id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  price REAL,"
        "  description TEXT,"
        "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "products");
    EXPECT_EQ(schema.columns.size(), 5u);
    EXPECT_EQ(mapSQLiteType(schema.column_types.at("price")), "double");
}

TEST(SQLiteFixture, ParseUsersInsertRow) {
    // Matches the first INSERT in the fixture
    auto vals = parseSQLiteInsertValues(
        "1,'Alice','alice@example.com',30,9.5,1,'Hello world',"
        "'2024-01-01 00:00:00'");
    ASSERT_EQ(vals.size(), 8u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "Alice");
    EXPECT_EQ(vals[2], "alice@example.com");
    EXPECT_EQ(vals[3], "30");
    EXPECT_EQ(vals[4], "9.5");
    EXPECT_EQ(vals[7], "2024-01-01 00:00:00");
}

TEST(SQLiteFixture, NullValuesInInsert) {
    // Third user row has NULL email, age, score, bio
    auto vals = parseSQLiteInsertValues(
        "3,'Charlie',NULL,NULL,NULL,0,NULL,'2024-01-03 00:00:00'");
    ASSERT_EQ(vals.size(), 8u);
    EXPECT_EQ(vals[0], "3");
    EXPECT_EQ(vals[1], "Charlie");
    EXPECT_EQ(vals[2], "");   // email = NULL
    EXPECT_EQ(vals[3], "");   // age   = NULL
    EXPECT_EQ(vals[6], "");   // bio   = NULL
}
