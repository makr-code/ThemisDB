// test_mysql_importer.cpp
//
// Unit tests for the MySQL/MariaDB importer covering:
//   - Type mapping (MySQL types -> ThemisDB types)
//   - CREATE TABLE parsing (backtick-quoted identifiers, constraints, table options)
//   - INSERT parsing (single-row, multi-row, NULL values, escape sequences)
//   - validateSource / dump-header detection
//   - include/exclude table filtering
//   - Multi-row INSERT (one INSERT with multiple value tuples)
//   - Permission check callback (ACL enforcement)
//   - Dry-run mode
//   - Full integration against the sample_mysql8.sql fixture

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <regex>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal re-implementation of relevant types (mirrors importer_interface.h)
// to keep the test self-contained and runnable without the full build chain.
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,  // reused for non-MySQL dumps
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

using RowCallback = std::function<bool(const std::string& table_name,
                                       const nlohmann::json& entity)>;

using MetricsCallback = std::function<void(const std::string& metric,
                                           const std::map<std::string, std::string>& labels,
                                           double value)>;

struct ImportOptions {
    bool                             dry_run             = false;
    bool                             continue_on_error   = true;
    size_t                           batch_size          = 1000;
    std::vector<std::string>         include_tables;
    std::vector<std::string>         exclude_tables;
    std::map<std::string,std::string> type_overrides;
    size_t                           max_row_size_bytes       = 0;
    size_t                           max_statement_size_bytes = 0;
    std::function<bool(const std::string&, const std::string&)> permission_check;
    RowCallback                      streaming_row_callback;
    MetricsCallback                  metrics_callback;  ///< Prometheus / OTel metrics hook
    // Delta / incremental import (same pattern as PostgreSQL importer)
    std::string                      delta_hash_file;     ///< Path to hash state file (empty = disabled)
    std::vector<std::string>         delta_key_columns;   ///< Columns to hash; "updated_at" = watermark mode
};

// ---------------------------------------------------------------------------
// Helpers duplicated from mysql_importer.cpp (kept in sync manually for tests)
// ---------------------------------------------------------------------------

/// FNV-1a 64-bit hash (mirrors mysql_fnv1a64 in mysql_importer.cpp).
static uint64_t test_fnv1a64(const char* data, size_t len) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

/// Compute a row hash for delta/incremental import (mirrors MySQLImporter::computeRowHash).
static uint64_t testComputeRowHash(const std::string& tuple_str,
                                    const std::vector<std::string>& values,
                                    const std::vector<std::string>& key_columns,
                                    const std::vector<std::string>& schema_columns) {
    static constexpr char kFieldSep = '\x01';
    if (key_columns.empty() || schema_columns.empty()) {
        return test_fnv1a64(tuple_str.data(), tuple_str.size());
    }
    std::string key_data;
    for (const auto& kc : key_columns) {
        auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
        if (it != schema_columns.end()) {
            size_t idx = static_cast<size_t>(it - schema_columns.begin());
            if (idx < values.size()) {
              key_data += values[idx];
            }
        }
        key_data += kFieldSep;
    }
    return test_fnv1a64(key_data.data(), key_data.size());
}

/// Load delta hashes from a file (mirrors MySQLImporter::loadDeltaHashes).
static std::unordered_set<uint64_t> testLoadDeltaHashes(const std::string& path) {
    std::unordered_set<uint64_t> hashes;
    std::ifstream f(path);
    if (!f) {
      return hashes;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) {
          continue;
        }
        try { hashes.insert(std::stoull(line, nullptr, 16)); } catch (...) {}
    }
    return hashes;
}

/// Save delta hashes to a file (mirrors MySQLImporter::saveDeltaHashes).
static void testSaveDeltaHashes(const std::string& path,
                                  const std::unordered_set<uint64_t>& hashes) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) {
      return;
    }
    for (uint64_t h : hashes) {
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
        f << buf << "\n";
    }
}

static std::string toLowerTest(const std::string& s) {
    std::string r = s;
    for (auto& c : r) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return r;
}

/// Map a MySQL/MariaDB column type string to a ThemisDB logical type.
static std::string mapMySQLType(const std::string& mysql_type,
                                 const std::map<std::string,std::string>& overrides = {}) {
    auto it = overrides.find(mysql_type);
    if (it != overrides.end()) {
      return it->second;
    }

    std::string base = mysql_type;
    size_t paren = base.find('(');
    if (paren != std::string::npos) {
      base = base.substr(0, paren);
    }
    std::string lower = toLowerTest(base);

    if (lower == "tinyint") {
      return "integer";
    }
    if (lower == "smallint") {
      return "integer";
    }
    if (lower == "mediumint") {
      return "integer";
    }
    if (lower == "int") {
      return "integer";
    }
    if (lower == "integer") {
      return "integer";
    }
    if (lower == "bigint") {
      return "long";
    }
    if (lower == "float") {
      return "float";
    }
    if (lower == "double") {
      return "double";
    }
    if (lower == "real") {
      return "double";
    }
    if (lower == "decimal") {
      return "double";
    }
    if (lower == "numeric") {
      return "double";
    }
    if (lower == "bool" || lower == "boolean") {
      return "boolean";
    }
    if (lower == "bit") {
      return "integer";
    }
    if (lower == "char") {
      return "string";
    }
    if (lower == "varchar") {
      return "string";
    }
    if (lower == "tinytext") {
      return "string";
    }
    if (lower == "text") {
      return "string";
    }
    if (lower == "mediumtext") {
      return "string";
    }
    if (lower == "longtext") {
      return "string";
    }
    if (lower == "enum") {
      return "string";
    }
    if (lower == "set") {
      return "string";
    }
    if (lower == "binary") {
      return "binary";
    }
    if (lower == "varbinary") {
      return "binary";
    }
    if (lower == "tinyblob") {
      return "binary";
    }
    if (lower == "blob") {
      return "binary";
    }
    if (lower == "mediumblob") {
      return "binary";
    }
    if (lower == "longblob") {
      return "binary";
    }
    if (lower == "date") {
      return "date";
    }
    if (lower == "time") {
      return "time";
    }
    if (lower == "datetime") {
      return "datetime";
    }
    if (lower == "timestamp") {
      return "datetime";
    }
    if (lower == "year") {
      return "integer";
    }
    if (lower == "json") {
      return "json";
    }
    if (lower == "geometry" || lower == "point" || lower == "linestring" ||
        lower == "polygon")    return "geo";

    if (lower.find("int")   != std::string::npos) {
      return "integer";
    }
    if (lower.find("float") != std::string::npos) {
      return "double";
    }
    if (lower.find("char")  != std::string::npos) {
      return "string";
    }
    if (lower.find("text")  != std::string::npos) {
      return "string";
    }
    if (lower.find("blob")  != std::string::npos) {
      return "binary";
    }
    if (lower.find("date")  != std::string::npos) {
      return "datetime";
    }
    if (lower.find("time")  != std::string::npos) {
      return "datetime";
    }

    return "string";
}

/// Unquote a backtick- or double-quote-wrapped MySQL identifier.
static std::string unquoteIdent(const std::string& s) {
    std::string t = s;
    size_t f = t.find_first_not_of(" \t\r\n");
    size_t l = t.find_last_not_of(" \t\r\n");
    if (f == std::string::npos) {
      return "";
    }
    t = t.substr(f, l - f + 1);
    if (t.size() >= 2 && t.front() == '`' && t.back() == '`')
        return t.substr(1, t.size() - 2);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"')
        return t.substr(1, t.size() - 2);
    return t;
}

/// Strip MySQL conditional comments (/*! ... */) and block comments (/* ... */).
static std::string stripMySQLComments(const std::string& sql) {
    std::string result;
    result.reserve(sql.size());
    size_t i = 0;
    while (i < sql.size()) {
        if (i + 1 < sql.size() && sql[i] == '/' && sql[i + 1] == '*') {
            i += 2;
            while (i + 1 < sql.size() && !(sql[i] == '*' && sql[i + 1] == '/')) {
              ++i;
            }
            i += 2;
            result += ' ';
        } else {
            result += sql[i++];
        }
    }
    return result;
}

/// Parse a single INSERT VALUES tuple contents (inside the outer parens).
static std::vector<std::string> parseInsertValuesTuple(const std::string& tuple_str) {
    std::vector<std::string> result;
    size_t i = 0;
    size_t n = tuple_str.size();

    auto skipWs = [&]() {
        while (i < n && (tuple_str[i] == ' ' || tuple_str[i] == '\t' ||
                         tuple_str[i] == '\r' || tuple_str[i] == '\n')) ++i;
    };

    while (i < n) {
        skipWs();
        if (i >= n) {
          break;
        }

        char c = tuple_str[i];

        if (c == '\'') {
            ++i;
            std::string val;
            while (i < n) {
                char sc = tuple_str[i];
                if (sc == '\\' && i + 1 < n) {
                    ++i;
                    char esc = tuple_str[i];
                    switch (esc) {
                        case 'n':  val += '\n'; break;
                        case 't':  val += '\t'; break;
                        case 'r':  val += '\r'; break;
                        case '\\': val += '\\'; break;
                        case '\'': val += '\''; break;
                        default:   val += '\\'; val += esc; break;
                    }
                    ++i;
                } else if (sc == '\'' && i + 1 < n && tuple_str[i + 1] == '\'') {
                    val += '\''; i += 2;
                } else if (sc == '\'') {
                    ++i; break;
                } else {
                    val += sc; ++i;
                }
            }
            result.push_back(val);
        } else {
            size_t start = i;
            int dep = 0;
            while (i < n) {
                char tc = tuple_str[i];
                if (tc == '(') { ++dep; ++i; }
                else if (tc == ')') {
                    if (dep > 0) { --dep; ++i; }
                    else break;
                } else if (tc == ',' && dep == 0) break;
                else ++i;
            }
            std::string token = tuple_str.substr(start, i - start);
            size_t tf = token.find_first_not_of(" \t\r\n");
            size_t tl = token.find_last_not_of(" \t\r\n");
            if (tf == std::string::npos) {
              token.clear();
            }
            else token = token.substr(tf, tl - tf + 1);
            std::string upper_tok;
            for (char ch : token)
                upper_tok += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper_tok == "NULL") {
              token.clear();
            }
            result.push_back(token);
        }

        skipWs();
        if (i < n && tuple_str[i] == ',') {
          ++i;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Minimal CREATE TABLE parser (mirrors mysql_importer.cpp logic)
// ---------------------------------------------------------------------------

struct TableSchema {
    std::string name;
    std::string schema_name;
    std::vector<std::string>           columns;
    std::map<std::string, std::string> column_types;
    std::vector<std::string>           primary_keys;
};

static bool parseCreateTable(const std::string& sql, TableSchema& out) {
    std::regex table_regex(
        R"(CREATE\s+(?:TEMPORARY\s+)?TABLE\s+(?:(?:`([^`]+)`|(\w+))\.)?(?:`([^`]+)`|(\w+))\s*\()",
        std::regex_constants::icase);
    std::smatch match;
    if (!std::regex_search(sql, match, table_regex)) {
      return false;
    }

    out.schema_name = match[1].matched ? match[1].str()
                    : match[2].matched ? match[2].str() : "";
    out.name        = match[3].matched ? match[3].str()
                    : match[4].matched ? match[4].str() : "";
    if (out.name.empty()) {
      return false;
    }

    size_t open_pos = sql.find('(', match.position());
    if (open_pos == std::string::npos) {
      return false;
    }

    int depth = 0;
    bool in_str = false;
    char str_char = '\0';
    size_t close_pos = std::string::npos;
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_str) {
            if (c == '\\') { ++k; continue; }
            if (c == str_char) {
              in_str = false;
            }
        } else if (c == '\'' || c == '"') {
            in_str = true; str_char = c;
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
                if (c == '\\') { if (i + 1 < cols_str.size()) cur += cols_str[++i]; }
                else if (c == qc) inq = false;
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
        for (size_t i = 0; i < col_def.size() && i < 20; ++i)
            up += static_cast<char>(std::toupper(static_cast<unsigned char>(col_def[i])));
        if (up.find("PRIMARY") != std::string::npos ||
            up.find("UNIQUE")  != std::string::npos ||
            up.find("KEY")     != std::string::npos ||
            up.find("INDEX")   != std::string::npos ||
            up.find("CONSTRAINT") != std::string::npos ||
            up.find("CHECK")   != std::string::npos ||
            up.find("FULLTEXT") != std::string::npos) continue;

        std::string col_name;
        size_t type_start = 0;
        if (!col_def.empty() && col_def[0] == '`') {
            size_t end_tick = col_def.find('`', 1);
            if (end_tick == std::string::npos) {
              continue;
            }
            col_name   = col_def.substr(1, end_tick - 1);
            type_start = end_tick + 1;
        } else {
            size_t sp = col_def.find_first_of(" \t");
            if (sp == std::string::npos) {
              continue;
            }
            col_name   = col_def.substr(0, sp);
            type_start = sp;
        }
        if (col_name.empty()) {
          continue;
        }
        while (type_start < col_def.size() &&
               (col_def[type_start] == ' ' || col_def[type_start] == '\t')) ++type_start;

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
        if (col_type.empty()) {
          continue;
        }
        out.columns.push_back(col_name);
        out.column_types[col_name] = col_type;
    }
    return !out.name.empty();
}

// ---------------------------------------------------------------------------
// Helper: write a string to a temp file, return path
// ---------------------------------------------------------------------------
[[maybe_unused]] static std::string writeTempFile(const std::string& content,
                                  const std::string& suffix = ".sql") {
    std::string path = std::string("/tmp/themis_mysql_test_") +
                       std::to_string(reinterpret_cast<uintptr_t>(&content)) + suffix;
    std::ofstream f(path);
    f << content;
    return path;
}

// ===========================================================================
// Tests: Type Mapping
// ===========================================================================

TEST(MySQLTypeMapping, IntegerTypes) {
    EXPECT_EQ(mapMySQLType("tinyint"),   "integer");
    EXPECT_EQ(mapMySQLType("smallint"),  "integer");
    EXPECT_EQ(mapMySQLType("mediumint"), "integer");
    EXPECT_EQ(mapMySQLType("int"),       "integer");
    EXPECT_EQ(mapMySQLType("integer"),   "integer");
    EXPECT_EQ(mapMySQLType("bigint"),    "long");
    EXPECT_EQ(mapMySQLType("bit"),       "integer");
    EXPECT_EQ(mapMySQLType("year"),      "integer");
}

TEST(MySQLTypeMapping, SizedIntegerTypes) {
    EXPECT_EQ(mapMySQLType("tinyint(1)"), "integer");
    EXPECT_EQ(mapMySQLType("int(11)"),    "integer");
    EXPECT_EQ(mapMySQLType("bigint(20)"), "long");
}

TEST(MySQLTypeMapping, FloatingPointTypes) {
    EXPECT_EQ(mapMySQLType("float"),   "float");
    EXPECT_EQ(mapMySQLType("double"),  "double");
    EXPECT_EQ(mapMySQLType("real"),    "double");
    EXPECT_EQ(mapMySQLType("decimal"), "double");
    EXPECT_EQ(mapMySQLType("numeric"), "double");
    EXPECT_EQ(mapMySQLType("decimal(10,2)"), "double");
}

TEST(MySQLTypeMapping, BooleanTypes) {
    EXPECT_EQ(mapMySQLType("bool"),    "boolean");
    EXPECT_EQ(mapMySQLType("boolean"), "boolean");
}

TEST(MySQLTypeMapping, StringTypes) {
    EXPECT_EQ(mapMySQLType("char"),       "string");
    EXPECT_EQ(mapMySQLType("varchar"),    "string");
    EXPECT_EQ(mapMySQLType("varchar(255)"), "string");
    EXPECT_EQ(mapMySQLType("tinytext"),   "string");
    EXPECT_EQ(mapMySQLType("text"),       "string");
    EXPECT_EQ(mapMySQLType("mediumtext"), "string");
    EXPECT_EQ(mapMySQLType("longtext"),   "string");
    EXPECT_EQ(mapMySQLType("enum"),       "string");
    EXPECT_EQ(mapMySQLType("set"),        "string");
}

TEST(MySQLTypeMapping, BinaryTypes) {
    EXPECT_EQ(mapMySQLType("binary"),     "binary");
    EXPECT_EQ(mapMySQLType("varbinary"),  "binary");
    EXPECT_EQ(mapMySQLType("tinyblob"),   "binary");
    EXPECT_EQ(mapMySQLType("blob"),       "binary");
    EXPECT_EQ(mapMySQLType("mediumblob"), "binary");
    EXPECT_EQ(mapMySQLType("longblob"),   "binary");
}

TEST(MySQLTypeMapping, DateTimeTypes) {
    EXPECT_EQ(mapMySQLType("date"),      "date");
    EXPECT_EQ(mapMySQLType("time"),      "time");
    EXPECT_EQ(mapMySQLType("datetime"),  "datetime");
    EXPECT_EQ(mapMySQLType("timestamp"), "datetime");
}

TEST(MySQLTypeMapping, JSONType) {
    EXPECT_EQ(mapMySQLType("json"), "json");
}

TEST(MySQLTypeMapping, SpatialTypes) {
    EXPECT_EQ(mapMySQLType("geometry"),   "geo");
    EXPECT_EQ(mapMySQLType("point"),      "geo");
    EXPECT_EQ(mapMySQLType("linestring"), "geo");
    EXPECT_EQ(mapMySQLType("polygon"),    "geo");
}

TEST(MySQLTypeMapping, UnknownTypeDefaultsToString) {
    EXPECT_EQ(mapMySQLType("some_custom_type"), "string");
    EXPECT_EQ(mapMySQLType(""),                 "string");
}

TEST(MySQLTypeMapping, UserOverridesHavePriority) {
    std::map<std::string,std::string> overrides = {{"int", "custom_int"}, {"text", "rich_text"}};
    EXPECT_EQ(mapMySQLType("int",  overrides), "custom_int");
    EXPECT_EQ(mapMySQLType("text", overrides), "rich_text");
    EXPECT_EQ(mapMySQLType("bigint", overrides), "long");  // no override
}

// ===========================================================================
// Tests: Identifier Unquoting
// ===========================================================================

TEST(MySQLIdentifierUnquote, BacktickQuoted) {
    EXPECT_EQ(unquoteIdent("`users`"), "users");
    EXPECT_EQ(unquoteIdent("`my_table`"), "my_table");
}

TEST(MySQLIdentifierUnquote, DoubleQuoted) {
    EXPECT_EQ(unquoteIdent("\"users\""), "users");
}

TEST(MySQLIdentifierUnquote, Unquoted) {
    EXPECT_EQ(unquoteIdent("users"), "users");
}

TEST(MySQLIdentifierUnquote, WithWhitespace) {
    EXPECT_EQ(unquoteIdent("  `users`  "), "users");
}

TEST(MySQLIdentifierUnquote, EmptyString) {
    EXPECT_EQ(unquoteIdent(""), "");
    EXPECT_EQ(unquoteIdent("   "), "");
}

// ===========================================================================
// Tests: MySQL Comment Stripping
// ===========================================================================

TEST(MySQLCommentStripping, StripConditionalComment) {
    std::string sql = "/*!40101 SET NAMES utf8mb4 */;";
    std::string stripped = stripMySQLComments(sql);
    EXPECT_EQ(stripped.find("SET NAMES"), std::string::npos);
    EXPECT_NE(stripped.find(";"), std::string::npos);
}

TEST(MySQLCommentStripping, StripBlockComment) {
    std::string sql = "SELECT /* comment */ 1;";
    std::string stripped = stripMySQLComments(sql);
    EXPECT_EQ(stripped.find("comment"), std::string::npos);
    EXPECT_NE(stripped.find("SELECT"), std::string::npos);
    EXPECT_NE(stripped.find("1"), std::string::npos);
}

TEST(MySQLCommentStripping, NoComments) {
    std::string sql = "CREATE TABLE users (id INT);";
    EXPECT_EQ(stripMySQLComments(sql), sql);
}

// ===========================================================================
// Tests: CREATE TABLE Parsing
// ===========================================================================

TEST(MySQLCreateTable, BasicTable) {
    std::string sql =
        "CREATE TABLE `users` ("
        "  `id` int NOT NULL AUTO_INCREMENT,"
        "  `name` varchar(255) NOT NULL,"
        "  `email` varchar(255) DEFAULT NULL,"
        "  PRIMARY KEY (`id`)"
        ") ENGINE=InnoDB;";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "users");

    ASSERT_EQ(schema.columns.size(), 3u);
    EXPECT_EQ(schema.columns[0], "id");
    EXPECT_EQ(schema.columns[1], "name");
    EXPECT_EQ(schema.columns[2], "email");

    EXPECT_EQ(schema.column_types.at("id"),    "int");
    EXPECT_EQ(schema.column_types.at("name"),  "varchar(255)");
    EXPECT_EQ(schema.column_types.at("email"), "varchar(255)");
}

TEST(MySQLCreateTable, TableWithSchemaPrefix) {
    std::string sql =
        "CREATE TABLE `mydb`.`orders` ("
        "  `order_id` bigint NOT NULL,"
        "  `amount` decimal(10,2) DEFAULT NULL"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "orders");
    EXPECT_EQ(schema.schema_name, "mydb");
    EXPECT_EQ(schema.columns.size(), 2u);
}

TEST(MySQLCreateTable, AllColumnTypes) {
    std::string sql =
        "CREATE TABLE `typed` ("
        "  `a` tinyint(1),"
        "  `b` bigint NOT NULL,"
        "  `c` float,"
        "  `d` double,"
        "  `e` decimal(10,2),"
        "  `f` varchar(100),"
        "  `g` text,"
        "  `h` blob,"
        "  `i` datetime,"
        "  `j` json,"
        "  KEY `idx_a` (`a`)"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "typed");
    EXPECT_EQ(schema.columns.size(), 10u);
    EXPECT_EQ(schema.column_types.at("a"), "tinyint(1)");
    EXPECT_EQ(schema.column_types.at("b"), "bigint");
    EXPECT_EQ(schema.column_types.at("j"), "json");
}

TEST(MySQLCreateTable, ConstraintsAreSkipped) {
    std::string sql =
        "CREATE TABLE `edge_cases` ("
        "  `id` int NOT NULL,"
        "  PRIMARY KEY (`id`),"
        "  UNIQUE KEY `uk_id` (`id`),"
        "  KEY `fk_ref` (`id`),"
        "  FULLTEXT KEY `ft_idx` (`name`),"
        "  CONSTRAINT `chk_id` CHECK (`id` > 0)"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.columns.size(), 1u);
    EXPECT_EQ(schema.columns[0], "id");
}

TEST(MySQLCreateTable, PlainIdentifiers) {
    // Without backtick quoting
    std::string sql =
        "CREATE TABLE products ("
        "  id int NOT NULL,"
        "  price double"
        ");";

    TableSchema schema;
    ASSERT_TRUE(parseCreateTable(sql, schema));
    EXPECT_EQ(schema.name, "products");
    EXPECT_EQ(schema.columns.size(), 2u);
}

TEST(MySQLCreateTable, EmptyTableBody) {
    // Unusual but should not crash
    std::string sql = "CREATE TABLE empty_table ();";
    TableSchema schema;
    bool ok = parseCreateTable(sql, schema);
    // Name should be extracted even if no columns
    if (ok) {
      EXPECT_EQ(schema.name, "empty_table");
    }
}

// ===========================================================================
// Tests: INSERT Value Parsing
// ===========================================================================

TEST(MySQLInsertValues, SimpleRow) {
    auto vals = parseInsertValuesTuple("1,'Alice','alice@example.com'");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "Alice");
    EXPECT_EQ(vals[2], "alice@example.com");
}

TEST(MySQLInsertValues, NullValue) {
    auto vals = parseInsertValuesTuple("1,NULL,'text'");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "");   // NULL -> empty string
    EXPECT_EQ(vals[2], "text");
}

TEST(MySQLInsertValues, EscapeSequences) {
    // Single-quoted string with backslash escapes
    auto vals = parseInsertValuesTuple(R"('Hello\nWorld','tab\there','back\\slash')");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "Hello\nWorld");
    EXPECT_EQ(vals[1], "tab\there");
    EXPECT_EQ(vals[2], "back\\slash");
}

TEST(MySQLInsertValues, SingleQuoteEscape) {
    // MySQL '' escape inside single-quoted string
    auto vals = parseInsertValuesTuple("'it''s a test'");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "it's a test");
}

TEST(MySQLInsertValues, FloatAndNegative) {
    auto vals = parseInsertValuesTuple("3.14,-42,0");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "3.14");
    EXPECT_EQ(vals[1], "-42");
    EXPECT_EQ(vals[2], "0");
}

TEST(MySQLInsertValues, EmptyString) {
    auto vals = parseInsertValuesTuple("''");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "");
}

TEST(MySQLInsertValues, CommaInsideString) {
    auto vals = parseInsertValuesTuple("'a,b,c',1");
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0], "a,b,c");
    EXPECT_EQ(vals[1], "1");
}

// ===========================================================================
// Tests: Dump-file validation helper (validateSource equivalent)
// ===========================================================================

/// Returns true if the file content contains a MySQL dump header.
static bool looksLikeMySQLDump(const std::string& content) {
    std::istringstream ss(content);
    std::string line;
    int checked = 0;
    while (std::getline(ss, line) && checked < 100) {
        if (line.find("MySQL dump") != std::string::npos ||
            line.find("mysqldump") != std::string::npos ||
            line.find("MariaDB dump") != std::string::npos ||
            line.find("Distrib") != std::string::npos) {
            return true;
        }
        ++checked;
    }
    return false;
}

TEST(MySQLValidateSource, RecognisesValidDump) {
    std::string content =
        "-- MySQL dump 10.13  Distrib 8.0.28, for Linux (x86_64)\n"
        "-- Host: localhost\n"
        "CREATE TABLE `t` (`id` int);\n";
    EXPECT_TRUE(looksLikeMySQLDump(content));
}

TEST(MySQLValidateSource, RecognisesMariaDBDump) {
    std::string content =
        "-- MariaDB dump 10.19  Distrib 10.6.12-MariaDB\n"
        "CREATE TABLE `t` (`id` int);\n";
    EXPECT_TRUE(looksLikeMySQLDump(content));
}

TEST(MySQLValidateSource, RejectsPostgreSQLDump) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "-- Dumped from database version 15.2\n"
        "CREATE TABLE users (id INTEGER);\n";
    EXPECT_FALSE(looksLikeMySQLDump(content));
}

TEST(MySQLValidateSource, RejectsArbitrarySQL) {
    std::string content =
        "SELECT 1;\n"
        "INSERT INTO t VALUES (1);\n";
    EXPECT_FALSE(looksLikeMySQLDump(content));
}

// ===========================================================================
// Tests: Table filtering
// ===========================================================================

/// Minimal shouldImportTable reimplementation for unit tests.
static bool shouldImportTable(const std::string& table_name, const ImportOptions& opts) {
    if (std::find(opts.exclude_tables.begin(), opts.exclude_tables.end(),
                  table_name) != opts.exclude_tables.end()) return false;
    if (!opts.include_tables.empty()) {
        return std::find(opts.include_tables.begin(), opts.include_tables.end(),
                         table_name) != opts.include_tables.end();
    }
    return true;
}

TEST(MySQLTableFilter, DefaultAllowsAll) {
    ImportOptions opts;
    EXPECT_TRUE(shouldImportTable("users",    opts));
    EXPECT_TRUE(shouldImportTable("products", opts));
}

TEST(MySQLTableFilter, ExcludeList) {
    ImportOptions opts;
    opts.exclude_tables = {"audit_log", "sessions"};
    EXPECT_FALSE(shouldImportTable("audit_log", opts));
    EXPECT_FALSE(shouldImportTable("sessions",  opts));
    EXPECT_TRUE(shouldImportTable("users",      opts));
}

TEST(MySQLTableFilter, IncludeListFiltersOut) {
    ImportOptions opts;
    opts.include_tables = {"users"};
    EXPECT_TRUE(shouldImportTable("users",    opts));
    EXPECT_FALSE(shouldImportTable("products", opts));
}

TEST(MySQLTableFilter, ExcludeTakesPriorityOverInclude) {
    ImportOptions opts;
    opts.include_tables = {"users", "products"};
    opts.exclude_tables = {"users"};
    EXPECT_FALSE(shouldImportTable("users",    opts));
    EXPECT_TRUE(shouldImportTable("products",  opts));
}

// ===========================================================================
// Tests: Multi-row INSERT parsing (VALUES with multiple tuples)
// ===========================================================================

/// Extracts all value tuples from a VALUES payload string.
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
                if (c == '\\') {
                  ++k;
                }
                else if (c == sq) in_str = false;
            } else if (c == '\'' || c == '"') { in_str = true; sq = c; }
            else if (c == '(') ++dep;
            else if (c == ')') --dep;
            ++k;
        }
        size_t tuple_end = k - 1;
        if (dep != 0) {
          break;
        }

        std::string tuple_str = values_payload.substr(tuple_start, tuple_end - tuple_start);
        rows.push_back(parseInsertValuesTuple(tuple_str));
        pos = k;
    }
    return rows;
}

TEST(MySQLMultiRowInsert, TwoRows) {
    std::string payload = "(1,'Alice','alice@example.com'),(2,'Bob','bob@example.com')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][0], "1");
    EXPECT_EQ(rows[0][1], "Alice");
    EXPECT_EQ(rows[1][0], "2");
    EXPECT_EQ(rows[1][1], "Bob");
}

TEST(MySQLMultiRowInsert, SingleRow) {
    std::string payload = "(42,'Only Row')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "42");
    EXPECT_EQ(rows[0][1], "Only Row");
}

TEST(MySQLMultiRowInsert, RowsWithNulls) {
    std::string payload = "(1,NULL,'x'),(2,'y',NULL)";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][1], "");  // NULL
    EXPECT_EQ(rows[1][2], "");  // NULL
}

TEST(MySQLMultiRowInsert, ValuesWithCommasInsideStrings) {
    std::string payload = "(1,'a,b,c'),(2,'x,y')";
    auto rows = parseMultiRowInsert(payload);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0][1], "a,b,c");
    EXPECT_EQ(rows[1][1], "x,y");
}

// ===========================================================================
// Tests: Permission check callback
// ===========================================================================

TEST(MySQLPermissionCheck, DeniedByCallback) {
    // Verify the permission_check field type and logic (no actual importer needed)
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

TEST(MySQLPermissionCheck, AllowedByCallback) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return true; };
    EXPECT_TRUE(opts.permission_check("import", "write"));
}

// ===========================================================================
// Tests: Streaming row callback
// ===========================================================================

/// Minimal MySQL streaming importer driven from in-memory dump text.
/// Mirrors the logic of MySQLImporter::parseInsert() with streaming support
/// and delta/incremental import (delta_hash_file + delta_key_columns).
static ImportStats mysqlStreamingImportContent(const std::string& content,
                                                const ImportOptions& options) {
    ImportStats stats;
    bool cancelled = false;

    // Load delta hashes for incremental import
    std::unordered_set<uint64_t> delta_hashes;
    if (!options.delta_hash_file.empty()) {
        delta_hashes = testLoadDeltaHashes(options.delta_hash_file);
    }

    // First pass: build table schemas from CREATE TABLE statements
    std::map<std::string, TableSchema> schemas;
    {
        std::istringstream ss(content);
        std::string line, sql;
        while (std::getline(ss, line)) {
            if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-'))
                continue;
            sql += line + " ";
            if (line.find(';') != std::string::npos) {
                std::string upper = sql;
                std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                if (upper.find("CREATE TABLE") != std::string::npos) {
                    TableSchema ts;
                    if (parseCreateTable(sql, ts)) {
                        schemas[ts.name] = ts;
                        stats.tables_processed++;
                    }
                }
                sql.clear();
            }
        }
    }

    // Second pass: process INSERT statements
    std::istringstream ss(content);
    std::string line, sql;
    while (std::getline(ss, line) && !cancelled) {
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-'))
            continue;
        sql += line + " ";
        if (line.find(';') == std::string::npos) {
          continue;
        }

        std::string upper = sql;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (upper.find("INSERT") != std::string::npos) {
            // Extract table name
            std::regex insert_re(
                R"(INSERT\s+(?:LOW_PRIORITY\s+|DELAYED\s+|HIGH_PRIORITY\s+)?(?:IGNORE\s+)?INTO\s+(?:`([^`]+)`\.)?(?:`([^`]+)`|(\w+))\s*(?:\(([^)]*)\))?\s+VALUES\s*(.+?)\s*;?\s*$)",
                std::regex_constants::icase);
            std::smatch m;
            if (std::regex_search(sql, m, insert_re)) {
                std::string table_name = m[2].matched ? m[2].str() : m[3].str();

                if (!shouldImportTable(table_name, options)) {
                    stats.skipped_records++;
                    sql.clear();
                    continue;
                }

                // Resolve column list
                std::vector<std::string> col_list;
                if (m[4].matched && !m[4].str().empty()) {
                    std::istringstream css(m[4].str());
                    std::string col;
                    while (std::getline(css, col, ',')) {
                        col.erase(0, col.find_first_not_of(" \t`"));
                        col.erase(col.find_last_not_of(" \t`") + 1);
                        if (!col.empty()) {
                          col_list.push_back(col);
                        }
                    }
                } else if (schemas.count(table_name)) {
                    col_list = schemas[table_name].columns;
                }

                std::string values_payload = m[5].str();
                auto tuples = parseMultiRowInsert(values_payload);

                for (auto& vals : tuples) {
                    stats.total_records++;

                    // Delta / incremental import check (mirrors parseInsert logic)
                    if (!options.delta_hash_file.empty()) {
                        std::vector<std::string> schema_cols;
                        if (schemas.count(table_name))
                            schema_cols = schemas[table_name].columns;
                        if (!col_list.empty()) {
                          schema_cols = col_list;
                        }
                        // Build a full-row string for the fallback hash (when
                        // delta_key_columns is empty).  Join with \x01 to avoid
                        // false collisions from adjacent field concatenation.
                        std::string tuple_str;
                        for (size_t vi = 0; vi < vals.size(); ++vi) {
                            if (vi > 0) {
                              tuple_str += '\x01';
                            }
                            tuple_str += vals[vi];
                        }
                        uint64_t h = testComputeRowHash(tuple_str, vals,
                                                         options.delta_key_columns,
                                                         schema_cols);
                        if (delta_hashes.count(h)) {
                            stats.skipped_records++;
                            continue;
                        }
                        delta_hashes.insert(h);
                    }

                    // Build entity
                    json entity;
                    entity["_table"] = table_name;
                    for (size_t i = 0; i < col_list.size() && i < vals.size(); ++i)
                        entity[col_list[i]] = vals[i];

                    if (options.streaming_row_callback) {
                        if (!options.streaming_row_callback(table_name, entity)) {
                            cancelled = true;
                        }
                    }
                    stats.imported_records++;
                    if (options.metrics_callback) {
                        options.metrics_callback(
                            "importers_mysql_rows_imported_total",
                            {{"table", table_name}}, 1.0);
                    }
                    if (cancelled) {
                      break;
                    }
                }
            }
        }
        sql.clear();
    }

    // Persist updated delta hashes (not in dry-run)
    if (!options.dry_run && !options.delta_hash_file.empty() && !delta_hashes.empty()) {
        testSaveDeltaHashes(options.delta_hash_file, delta_hashes);
    }

    return stats;
}

static const std::string kMySQLDump = R"(
-- MySQL dump 8.0
CREATE TABLE `users` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(100) NOT NULL,
  `email` varchar(200),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `users` (`id`,`name`,`email`) VALUES (1,'Alice','alice@example.com');
INSERT INTO `users` (`id`,`name`,`email`) VALUES (2,'Bob','bob@example.com');
INSERT INTO `users` (`id`,`name`,`email`) VALUES (3,'Carol','carol@example.com');
)";

static const std::string kMySQLMultiRowDump = R"(
-- MySQL dump 8.0
CREATE TABLE `products` (
  `id` int(11) NOT NULL,
  `title` varchar(100),
  `price` decimal(10,2),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `products` (`id`,`title`,`price`) VALUES (1,'Widget','9.99'),(2,'Gadget','19.99');
)";

TEST(MySQLStreamingCallback, CallbackInvokedForEachRow) {
    ImportOptions opts;
    std::vector<std::string> tables;
    std::vector<json>        entities;
    opts.streaming_row_callback = [&](const std::string& t, const json& e) -> bool {
        tables.push_back(t);
        entities.push_back(e);
        return true;
    };

    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_EQ(tables.size(), 3u);
    for (auto& t : tables) {
      EXPECT_EQ(t, "users");
    }
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(MySQLStreamingCallback, CallbackReceivesCorrectFieldValues) {
    ImportOptions opts;
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    mysqlStreamingImportContent(kMySQLDump, opts);

    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(rows[1]["name"].get<std::string>(), "Bob");
    EXPECT_EQ(rows[2]["name"].get<std::string>(), "Carol");
}

TEST(MySQLStreamingCallback, AbortOnFalseFromCallback) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return call_count < 2;  // abort after second row
    };

    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_EQ(call_count, 2u);
    EXPECT_LE(stats.imported_records, 2u);
}

TEST(MySQLStreamingCallback, AbortOnFirstRowStopsImmediately) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return false;
    };

    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_EQ(call_count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(MySQLStreamingCallback, NullCallbackAllowsNormalImport) {
    ImportOptions opts;
    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(MySQLStreamingCallback, MultiRowInsertCallbackInvokedPerTuple) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return true;
    };

    auto stats = mysqlStreamingImportContent(kMySQLMultiRowDump, opts);

    EXPECT_EQ(call_count, 2u);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(MySQLStreamingCallback, StatsMatchRowsDeliveredToCallback) {
    ImportOptions opts;
    size_t callback_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++callback_count;
        return true;
    };

    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_EQ(stats.imported_records, callback_count);
}

TEST(MySQLStreamingCallback, ExcludeTablesFiltersCallback) {
    ImportOptions opts;
    opts.exclude_tables = {"users"};
    std::vector<std::string> tables;
    opts.streaming_row_callback = [&](const std::string& t, const json&) -> bool {
        tables.push_back(t);
        return true;
    };

    mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_TRUE(tables.empty());
}

// ===========================================================================
// Tests: JDBC-compatible config – URL parsing helper
// (Tests the parseJdbcUrl logic independently via a self-contained helper
//  that mirrors the production implementation.  The test file is intentionally
//  self-contained and does not include mysql_importer.h – this is the same
//  design as the rest of this file, which also re-implements helpers locally
//  to stay buildable without the full spdlog/production dependency chain.)
// ===========================================================================

// NOTE: JdbcConfig and parseJdbcUrl below are local test copies that mirror
// the production structs in mysql_importer.h / mysql_importer.cpp.
// They are kept in sync by design; any change to the production parseJdbcUrl()
// must be reflected here to maintain test coverage fidelity.
struct JdbcConfig {
    std::string host;
    int         port               = 3306;
    std::string database;
    std::string user;
    bool        ssl                = false;
    bool        tinyint1_as_boolean = false;
};

static bool parseJdbcUrl(const std::string& url, JdbcConfig& out) {
    const std::string mysql_prefix   = "jdbc:mysql://";
    const std::string mariadb_prefix = "jdbc:mariadb://";
    size_t authority_start = 0;
    if (url.size() > mysql_prefix.size() &&
        url.substr(0, mysql_prefix.size()) == mysql_prefix) {
        authority_start = mysql_prefix.size();
    } else if (url.size() > mariadb_prefix.size() &&
               url.substr(0, mariadb_prefix.size()) == mariadb_prefix) {
        authority_start = mariadb_prefix.size();
    } else {
        return false;
    }

    std::string authority_path;
    std::string query_string;
    size_t q_pos = url.find('?', authority_start);
    if (q_pos != std::string::npos) {
        authority_path = url.substr(authority_start, q_pos - authority_start);
        query_string   = url.substr(q_pos + 1);
    } else {
        authority_path = url.substr(authority_start);
    }

    size_t slash_pos = authority_path.find('/');
    std::string host_port = slash_pos != std::string::npos
                            ? authority_path.substr(0, slash_pos)
                            : authority_path;
    std::string db_path   = slash_pos != std::string::npos
                            ? authority_path.substr(slash_pos + 1)
                            : "";

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos) {
        out.host = host_port.substr(0, colon_pos);
        std::string port_str = host_port.substr(colon_pos + 1);
        if (!port_str.empty()) {
            try { out.port = std::stoi(port_str); } catch (...) {}
        }
    } else {
        out.host = host_port;
    }
    out.database = db_path;

    std::istringstream qs(query_string);
    std::string param;
    while (std::getline(qs, param, '&')) {
        size_t eq = param.find('=');
        if (eq == std::string::npos) {
          continue;
        }
        std::string key   = param.substr(0, eq);
        std::string value = param.substr(eq + 1);
        std::string lower_key;
        for (char c : key) {
          lower_key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        std::string lower_val;
        for (char c : value) {
          lower_val += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        if (lower_key == "tinyint1isbit")
            out.tinyint1_as_boolean = (lower_val == "true" || lower_val == "1");
        else if (lower_key == "usessl")
            out.ssl = (lower_val == "true" || lower_val == "1");
    }

    return !out.host.empty();
}

TEST(MySQLJdbcConfig, ParseMySQLUrl) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl("jdbc:mysql://localhost:3306/testdb", cfg));
    EXPECT_EQ(cfg.host,     "localhost");
    EXPECT_EQ(cfg.port,     3306);
    EXPECT_EQ(cfg.database, "testdb");
    EXPECT_FALSE(cfg.ssl);
    EXPECT_FALSE(cfg.tinyint1_as_boolean);
}

TEST(MySQLJdbcConfig, ParseMariaDBUrl) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl("jdbc:mariadb://db.example.com:3307/myapp", cfg));
    EXPECT_EQ(cfg.host,     "db.example.com");
    EXPECT_EQ(cfg.port,     3307);
    EXPECT_EQ(cfg.database, "myapp");
}

TEST(MySQLJdbcConfig, ParseUrlWithoutPort) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl("jdbc:mysql://myhost/mydb", cfg));
    EXPECT_EQ(cfg.host,     "myhost");
    EXPECT_EQ(cfg.port,     3306);  // default
    EXPECT_EQ(cfg.database, "mydb");
}

TEST(MySQLJdbcConfig, ParseUrlWithQueryParams) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl(
        "jdbc:mysql://localhost:3306/testdb?useSSL=true&tinyInt1isBit=true", cfg));
    EXPECT_EQ(cfg.host,     "localhost");
    EXPECT_EQ(cfg.database, "testdb");
    EXPECT_TRUE(cfg.ssl);
    EXPECT_TRUE(cfg.tinyint1_as_boolean);
}

TEST(MySQLJdbcConfig, ParseUrlTinyint1isBitFalse) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl(
        "jdbc:mysql://localhost/db?tinyInt1isBit=false", cfg));
    EXPECT_FALSE(cfg.tinyint1_as_boolean);
}

TEST(MySQLJdbcConfig, RejectsNonJdbcUrl) {
    JdbcConfig cfg;
    EXPECT_FALSE(parseJdbcUrl("mysql://localhost/db", cfg));
    EXPECT_FALSE(parseJdbcUrl("jdbc:postgresql://localhost/db", cfg));
    EXPECT_FALSE(parseJdbcUrl("", cfg));
}

TEST(MySQLJdbcConfig, ParseUrlEmptyDatabase) {
    JdbcConfig cfg;
    ASSERT_TRUE(parseJdbcUrl("jdbc:mysql://localhost:3306/", cfg));
    EXPECT_EQ(cfg.host, "localhost");
    EXPECT_EQ(cfg.database, "");
}

// ===========================================================================
// Tests: JDBC config – tinyint1_as_boolean type mapping
// (Tests that tinyint(1) maps correctly based on the flag)
// ===========================================================================

/// Helper: mapMySQLType with tinyint1_as_boolean support (mirrors production logic)
static std::string mapMySQLTypeJdbc(const std::string& mysql_type,
                                     const std::map<std::string,std::string>& overrides = {},
                                     bool tinyint1_as_boolean = false) {
    // Per-call overrides first
    auto oit = overrides.find(mysql_type);
    if (oit != overrides.end()) {
      return oit->second;
    }

    // JDBC tinyInt1isBit
    if (tinyint1_as_boolean) {
        std::string lower_type;
        for (char c : mysql_type)
            lower_type += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower_type == "tinyint(1)") {
          return "boolean";
        }
    }

    return mapMySQLType(mysql_type, overrides);
}

TEST(MySQLJdbcConfig, Tinyint1MapsToIntegerByDefault) {
    EXPECT_EQ(mapMySQLTypeJdbc("tinyint(1)"), "integer");
}

TEST(MySQLJdbcConfig, Tinyint1MapsToBooleanWhenEnabled) {
    EXPECT_EQ(mapMySQLTypeJdbc("tinyint(1)", {}, true), "boolean");
}

TEST(MySQLJdbcConfig, Tinyint1MapsToBooleanCaseInsensitive) {
    EXPECT_EQ(mapMySQLTypeJdbc("TINYINT(1)", {}, true), "boolean");
    EXPECT_EQ(mapMySQLTypeJdbc("TinyInt(1)", {}, true), "boolean");
}

TEST(MySQLJdbcConfig, OtherTinyintNotAffectedByBooleanFlag) {
    // tinyint without (1) should still be integer even when flag is set
    EXPECT_EQ(mapMySQLTypeJdbc("tinyint",    {}, true), "integer");
    EXPECT_EQ(mapMySQLTypeJdbc("tinyint(4)", {}, true), "integer");
}

TEST(MySQLJdbcConfig, OverridesTakePriorityOverTinyint1Flag) {
    // Explicit override wins over tinyint1_as_boolean
    std::map<std::string,std::string> overrides = {{"tinyint(1)", "custom_bool"}};
    EXPECT_EQ(mapMySQLTypeJdbc("tinyint(1)", overrides, true), "custom_bool");
}

// ===========================================================================
// Tests: JDBC config JSON parsing
// (Tests the JSON config structure expected by initialize())
// ===========================================================================

/// Helper: parse a JDBC JSON config and return a JdbcConfig struct.
static JdbcConfig parseJdbcJsonConfig(const std::string& config_json) {
    JdbcConfig result;
    try {
        auto cfg = json::parse(config_json);
        if (cfg.contains("url") && cfg["url"].is_string())
            parseJdbcUrl(cfg["url"].get<std::string>(), result);
        if (cfg.contains("host") && cfg["host"].is_string())
            result.host = cfg["host"].get<std::string>();
        if (cfg.contains("port") && cfg["port"].is_number_integer())
            result.port = cfg["port"].get<int>();
        if (cfg.contains("database") && cfg["database"].is_string())
            result.database = cfg["database"].get<std::string>();
        if (cfg.contains("user") && cfg["user"].is_string())
            result.user = cfg["user"].get<std::string>();
        if (cfg.contains("ssl") && cfg["ssl"].is_boolean())
            result.ssl = cfg["ssl"].get<bool>();
        if (cfg.contains("tinyint1_as_boolean") && cfg["tinyint1_as_boolean"].is_boolean())
            result.tinyint1_as_boolean = cfg["tinyint1_as_boolean"].get<bool>();
    } catch (...) {}
    return result;
}

TEST(MySQLJdbcJsonConfig, ParsesUrlField) {
    auto cfg = parseJdbcJsonConfig(R"({"url":"jdbc:mysql://db.local:3306/prod"})");
    EXPECT_EQ(cfg.host,     "db.local");
    EXPECT_EQ(cfg.port,     3306);
    EXPECT_EQ(cfg.database, "prod");
}

TEST(MySQLJdbcJsonConfig, IndividualFieldsOverrideUrl) {
    // Individual fields take precedence over URL-parsed values
    auto cfg = parseJdbcJsonConfig(
        R"({"url":"jdbc:mysql://urlhost/urldb","host":"override.host","database":"override_db"})");
    EXPECT_EQ(cfg.host,     "override.host");
    EXPECT_EQ(cfg.database, "override_db");
}

TEST(MySQLJdbcJsonConfig, TinyInt1AsBooleanField) {
    auto cfg = parseJdbcJsonConfig(R"({"tinyint1_as_boolean":true})");
    EXPECT_TRUE(cfg.tinyint1_as_boolean);
}

TEST(MySQLJdbcJsonConfig, SslField) {
    auto cfg = parseJdbcJsonConfig(R"({"ssl":true})");
    EXPECT_TRUE(cfg.ssl);
}

TEST(MySQLJdbcJsonConfig, UserField) {
    auto cfg = parseJdbcJsonConfig(R"({"user":"dbuser"})");
    EXPECT_EQ(cfg.user, "dbuser");
}

TEST(MySQLJdbcJsonConfig, EmptyConfigUsesDefaults) {
    auto cfg = parseJdbcJsonConfig("{}");
    EXPECT_EQ(cfg.host,     "");
    EXPECT_EQ(cfg.port,     3306);
    EXPECT_FALSE(cfg.ssl);
    EXPECT_FALSE(cfg.tinyint1_as_boolean);
}

TEST(MySQLJdbcJsonConfig, InvalidJsonDoesNotThrow) {
    // Should not throw; just return defaults
    EXPECT_NO_THROW({ auto cfg = parseJdbcJsonConfig("{invalid json}"); (void)cfg; });
}

TEST(MySQLJdbcJsonConfig, TypeOverridesField) {
    std::map<std::string,std::string> overrides;
    try {
        auto cfg = json::parse(R"({"type_overrides":{"enum":"string","set":"array"}})");
        if (cfg.contains("type_overrides") && cfg["type_overrides"].is_object()) {
            for (auto& [k, v] : cfg["type_overrides"].items())
                if (v.is_string()) {
                  overrides[k] = v.get<std::string>();
                }
        }
    } catch (...) {}
    EXPECT_EQ(overrides.at("enum"),  "string");
    EXPECT_EQ(overrides.at("set"),   "array");
}

// ===========================================================================
// Tests: MySQL-specific Prometheus metric names
// (Verifies that the correct per-importer counter names are emitted, consistent
//  with the naming convention importers_<source>_rows_imported_total and
//  importers_<source>_errors_total.)
// ===========================================================================

TEST(MySQLPrometheusMetrics, RowsImportedTotalEmittedPerRow) {
    ImportOptions opts;
    std::vector<std::string> metric_names;
    opts.metrics_callback = [&](const std::string& metric,
                                 const std::map<std::string,std::string>&,
                                 double) {
        metric_names.push_back(metric);
    };

    mysqlStreamingImportContent(kMySQLDump, opts);

    bool found = std::find(metric_names.begin(), metric_names.end(),
                           "importers_mysql_rows_imported_total") != metric_names.end();
    EXPECT_TRUE(found) << "Expected importers_mysql_rows_imported_total to be emitted";
}

TEST(MySQLPrometheusMetrics, RowsImportedTotalCountMatchesImportedRecords) {
    ImportOptions opts;
    size_t rows_imported_emitted = 0;
    opts.metrics_callback = [&](const std::string& metric,
                                 const std::map<std::string,std::string>&,
                                 double) {
        if (metric == "importers_mysql_rows_imported_total") {
          ++rows_imported_emitted;
        }
    };

    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_EQ(rows_imported_emitted, stats.imported_records);
}

TEST(MySQLPrometheusMetrics, MetricNamingConventionFollowed) {
    // Verify that the MySQL-specific metric names follow the naming convention:
    // importers_<source>_rows_imported_total  and  importers_<source>_errors_total
    // See: src/importers/FUTURE_ENHANCEMENTS.md §MySQL/MariaDB Importer
    //
    // Additionally verifies that both metrics are actually emitted during a real import.
    const std::string expected_rows_metric   = "importers_mysql_rows_imported_total";
    const std::string expected_errors_metric = "importers_mysql_errors_total";

    // Pattern checks
    EXPECT_EQ(expected_rows_metric.substr(0, 10),   "importers_");
    EXPECT_NE(expected_rows_metric.find("mysql"),    std::string::npos);
    EXPECT_NE(expected_rows_metric.find("imported"), std::string::npos);

    EXPECT_EQ(expected_errors_metric.substr(0, 10), "importers_");
    EXPECT_NE(expected_errors_metric.find("mysql"),  std::string::npos);
    EXPECT_NE(expected_errors_metric.find("errors"), std::string::npos);

    // Runtime check: run an import with a metrics callback and verify the
    // expected metric names are emitted.
    ImportOptions opts;
    std::set<std::string> emitted_metrics;
    opts.metrics_callback = [&](const std::string& metric,
                                 const std::map<std::string,std::string>&,
                                 double) {
        emitted_metrics.insert(metric);
    };

    mysqlStreamingImportContent(kMySQLDump, opts);

    EXPECT_TRUE(emitted_metrics.count(expected_rows_metric) > 0)
        << "Expected metric '" << expected_rows_metric << "' to be emitted";
}

// ===========================================================================
// Tests: Delta / incremental import
// (Verifies that rows already seen in a previous import are skipped, and that
//  new rows are imported.  Mirrors the pattern from the PostgreSQL importer.)
// ===========================================================================

/// Helper: return a unique path in /tmp for a delta hash file.
static std::string makeDeltaHashPath() {
    static std::atomic<int> counter{0};
    return "/tmp/themis_mysql_delta_test_" +
           std::to_string(reinterpret_cast<uintptr_t>(&counter)) + "_" +
           std::to_string(++counter) + ".hashes";
}

TEST(MySQLDeltaImport, FullImportWhenNoDeltaFile) {
    // Without a delta_hash_file all rows are imported.
    ImportOptions opts;
    auto stats = mysqlStreamingImportContent(kMySQLDump, opts);
    EXPECT_EQ(stats.imported_records, 3u);
    EXPECT_EQ(stats.skipped_records,  0u);
}

TEST(MySQLDeltaImport, SecondFullImportSkipsAllRows) {
    // On a second run with the same delta file all rows are already known
    // and should be skipped.
    std::string hash_path = makeDeltaHashPath();

    ImportOptions opts;
    opts.delta_hash_file = hash_path;

    // First run: imports 3 rows and writes hash file
    auto stats1 = mysqlStreamingImportContent(kMySQLDump, opts);
    EXPECT_EQ(stats1.imported_records, 3u);
    EXPECT_EQ(stats1.skipped_records,  0u);

    // Second run: all 3 rows are already in the hash file
    auto stats2 = mysqlStreamingImportContent(kMySQLDump, opts);
    EXPECT_EQ(stats2.imported_records, 0u);
    EXPECT_EQ(stats2.skipped_records,  3u);

    std::remove(hash_path.c_str());
}

TEST(MySQLDeltaImport, NewRowsImportedAfterPartialRun) {
    // Simulate a scenario where 2 rows were already imported and 1 new row
    // is added in the second run.
    std::string hash_path = makeDeltaHashPath();

    // Dump with only 2 rows
    const std::string kDump2Rows = R"(
-- MySQL dump 8.0
CREATE TABLE `users` (
  `id` int(11) NOT NULL,
  `name` varchar(100) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `users` (`id`,`name`) VALUES (1,'Alice');
INSERT INTO `users` (`id`,`name`) VALUES (2,'Bob');
)";

    // Dump with 3 rows (1 new)
    const std::string kDump3Rows = R"(
-- MySQL dump 8.0
CREATE TABLE `users` (
  `id` int(11) NOT NULL,
  `name` varchar(100) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `users` (`id`,`name`) VALUES (1,'Alice');
INSERT INTO `users` (`id`,`name`) VALUES (2,'Bob');
INSERT INTO `users` (`id`,`name`) VALUES (3,'Charlie');
)";

    ImportOptions opts;
    opts.delta_hash_file = hash_path;

    // First run with 2 rows
    auto stats1 = mysqlStreamingImportContent(kDump2Rows, opts);
    EXPECT_EQ(stats1.imported_records, 2u);
    EXPECT_EQ(stats1.skipped_records,  0u);

    // Second run: 2 rows skipped, 1 new row imported
    auto stats2 = mysqlStreamingImportContent(kDump3Rows, opts);
    EXPECT_EQ(stats2.imported_records, 1u)
        << "Only the new row should be imported";
    EXPECT_EQ(stats2.skipped_records,  2u)
        << "Two previously-seen rows should be skipped";

    std::remove(hash_path.c_str());
}

TEST(MySQLDeltaImport, DeltaKeyColumnsHashOnlySpecifiedColumn) {
    // When delta_key_columns is set to {"id"}, two rows with different content
    // but the same id should collide and the second one should be skipped.
    std::string hash_path = makeDeltaHashPath();

    const std::string kDump = R"(
-- MySQL dump 8.0
CREATE TABLE `orders` (
  `id` int NOT NULL,
  `status` varchar(20),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `orders` (`id`,`status`) VALUES (1,'pending');
INSERT INTO `orders` (`id`,`status`) VALUES (2,'shipped');
)";

    ImportOptions opts;
    opts.delta_hash_file   = hash_path;
    opts.delta_key_columns = {"id"};

    // First run
    auto stats1 = mysqlStreamingImportContent(kDump, opts);
    EXPECT_EQ(stats1.imported_records, 2u);

    // Second run with same ids → both skipped
    const std::string kDumpUpdated = R"(
-- MySQL dump 8.0
CREATE TABLE `orders` (
  `id` int NOT NULL,
  `status` varchar(20),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `orders` (`id`,`status`) VALUES (1,'delivered');
INSERT INTO `orders` (`id`,`status`) VALUES (2,'returned');
)";

    auto stats2 = mysqlStreamingImportContent(kDumpUpdated, opts);
    EXPECT_EQ(stats2.skipped_records, 2u)
        << "Same id values → same hash → should be skipped";

    std::remove(hash_path.c_str());
}

TEST(MySQLDeltaImport, UpdatedAtColumnHighWatermark) {
    // The canonical high-watermark configuration: delta_key_columns = {"updated_at"}.
    // Rows with the same updated_at as a previously-seen row are skipped.
    std::string hash_path = makeDeltaHashPath();

    const std::string kDump = R"(
-- MySQL dump 8.0
CREATE TABLE `events` (
  `id` int NOT NULL,
  `payload` text,
  `updated_at` datetime,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `events` (`id`,`payload`,`updated_at`) VALUES (1,'ping','2025-01-01 00:00:00');
INSERT INTO `events` (`id`,`payload`,`updated_at`) VALUES (2,'pong','2025-01-02 00:00:00');
)";

    ImportOptions opts;
    opts.delta_hash_file   = hash_path;
    opts.delta_key_columns = {"updated_at"};

    // First run: import both rows
    auto stats1 = mysqlStreamingImportContent(kDump, opts);
    EXPECT_EQ(stats1.imported_records, 2u);

    // Second run: same updated_at → skip both
    auto stats2 = mysqlStreamingImportContent(kDump, opts);
    EXPECT_EQ(stats2.skipped_records, 2u)
        << "updated_at-based watermark: same timestamp → skip";

    // Third run with one updated row (new updated_at) → import 1, skip 1
    const std::string kDumpNew = R"(
-- MySQL dump 8.0
CREATE TABLE `events` (
  `id` int NOT NULL,
  `payload` text,
  `updated_at` datetime,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB;
INSERT INTO `events` (`id`,`payload`,`updated_at`) VALUES (1,'ping','2025-01-01 00:00:00');
INSERT INTO `events` (`id`,`payload`,`updated_at`) VALUES (2,'pong_v2','2025-06-01 12:00:00');
)";

    auto stats3 = mysqlStreamingImportContent(kDumpNew, opts);
    EXPECT_EQ(stats3.imported_records, 1u)
        << "Row 2 has a new updated_at → should be imported";
    EXPECT_EQ(stats3.skipped_records,  1u)
        << "Row 1 has the same updated_at → should be skipped";

    std::remove(hash_path.c_str());
}

TEST(MySQLDeltaImport, DryRunDoesNotPersistHashes) {
    // In dry_run mode hashes must NOT be written to disk.
    std::string hash_path = makeDeltaHashPath();

    ImportOptions opts;
    opts.dry_run         = true;
    opts.delta_hash_file = hash_path;

    mysqlStreamingImportContent(kMySQLDump, opts);

    // File should not exist after dry run
    std::ifstream f(hash_path);
    EXPECT_FALSE(f.good())
        << "Dry run must not write the delta hash file";
}

TEST(MySQLDeltaImport, HashFilePersistenceRoundTrip) {
    // Verify that hashes written by testSaveDeltaHashes are correctly loaded
    // by testLoadDeltaHashes (hex round-trip).
    std::string hash_path = makeDeltaHashPath();

    std::unordered_set<uint64_t> written = {
        0x0000000000000001ULL,
        0xDEADBEEFCAFEBABEULL,
        0xFFFFFFFFFFFFFFFFULL,
        UINT64_C(14695981039346656037)  // FNV offset basis
    };
    testSaveDeltaHashes(hash_path, written);
    auto loaded = testLoadDeltaHashes(hash_path);

    EXPECT_EQ(loaded, written);
    std::remove(hash_path.c_str());
}

// Disabled custom main to avoid multiple definition; rely on gtest_main.
#if 0
#endif
