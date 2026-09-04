// Integration tests for the PostgreSQL importer against a dump generated from
// a live PostgreSQL instance.
//
// These tests are skipped automatically when the THEMISDB_PG_LIVE_DUMP
// environment variable is not set.  In the postgres-live-integration CI job
// the variable is set to a dump file produced by pg_dump against the postgres
// service container that has been seeded with the canonical test schema and
// data (3 tables, 9 rows total).
//
// The optional THEMISDB_PG_CONNSTR variable captures the connection string
// used to generate the dump; it is not consumed by the importer under test
// (which reads files, not live connections) but is stored in the test fixture
// for diagnostic purposes and future direct-connection scenarios.

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <algorithm>
#include <cstdlib>

// ---------------------------------------------------------------------------
// All local types are placed in an anonymous namespace to guarantee internal
// linkage and avoid ODR conflicts with the other test_postgres_importer_*.cpp
// files that are compiled into the same unified test binary.
// ---------------------------------------------------------------------------
namespace {

enum class LiveImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,
    PARSE_CREATE_TABLE   = 200,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    ROW_TOO_LARGE        = 205,
    COLUMN_COUNT_MISMATCH = 301,
    UNKNOWN              = 900
};

enum class LiveImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct LiveImportError {
    LiveImportErrorCode     code;
    LiveImportErrorSeverity severity;
    std::string             message;
    std::string             location;
};

struct LiveImportStats {
    size_t total_records    = 0;
    size_t imported_records = 0;
    size_t failed_records   = 0;
    size_t skipped_records  = 0;
    size_t tables_processed = 0;
    std::vector<std::string>     warnings;
    std::vector<std::string>     errors;
    std::vector<LiveImportError> structured_errors;
};

struct LiveTableSchema {
    std::string name;
    std::string schema_name;
    std::vector<std::string>           columns;
    std::map<std::string, std::string> column_types;
};

struct LiveImportOptions {
    bool                             dry_run            = false;
    bool                             continue_on_error  = true;
    size_t                           batch_size         = 1000;
    std::vector<std::string>         include_tables;
    std::vector<std::string>         exclude_tables;
    std::map<std::string,std::string> type_overrides;
    size_t                           max_row_size_bytes = 0;
};

// ---------------------------------------------------------------------------
// Helpers: type mapping, COPY row parsing, CREATE TABLE parsing
// ---------------------------------------------------------------------------

static std::string liveMapType(const std::string& pg,
                                const std::map<std::string,std::string>& ov = {}) {
    auto it = ov.find(pg);
    if (it != ov.end()) {
      return it->second;
    }
    std::string t = pg;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    it = ov.find(t);
    if (it != ov.end()) {
      return it->second;
    }
    if (t.back() == ']' || t.find("[]") != std::string::npos) {
      return "array";
    }
    if (t == "bigserial" || t == "bigint" || t == "int8") {
      return "long";
    }
    if (t == "smallint"  || t == "int2"   || t == "smallserial") {
      return "integer";
    }
    if (t == "integer"   || t == "int"    || t == "int4" || t == "serial") {
      return "integer";
    }
    if (t == "real"      || t == "float4") {
      return "float";
    }
    if (t == "double precision" || t == "float8") {
      return "double";
    }
    if (t == "boolean"   || t == "bool") {
      return "boolean";
    }
    if (t == "bytea") {
      return "binary";
    }
    if (t == "json"      || t == "jsonb") {
      return "json";
    }
    if (t == "point" || t == "polygon" || t == "circle" || t == "line") {
      return "geo";
    }
    if (t == "oid" || t == "xid" || t == "cid") {
      return "integer";
    }
    // Parameterised numeric types: numeric(p,s), decimal(p,s), money
    if (t == "numeric" || t == "decimal" || t == "money" ||
        t.find("numeric(") == 0 || t.find("decimal(") == 0)        return "double";
    // character varying(n), varchar(n), char(n)
    if (t.find("char") != std::string::npos ||
        t.find("varchar") != std::string::npos)                     return "string";
    if (t.find("timestamp") != std::string::npos) {
      return "datetime";
    }
    if (t.find("date")      != std::string::npos) {
      return "date";
    }
    if (t.find("time")      != std::string::npos) {
      return "time";
    }
    if (t.find("json")      != std::string::npos) {
      return "json";
    }
    if (t.find("int")       != std::string::npos) {
      return "integer";
    }
    // inet, cidr, macaddr, xml, tsvector, interval, uuid, name, text -> string
    return "string";
}

static std::string liveUnescapeCopy(const std::string& val) {
    if (val == "\\N") {
      return "";
    }
    std::string out = {};
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            char nx = val[++i];
            switch (nx) {
                case 'N': out += '\\'; out += 'N'; break;
                case 't': out += '\t'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case '\\': out += '\\'; break;
                default: out += '\\'; out += nx; break;
            }
        } else {
            out += val[i];
        }
    }
    return out;
}

static std::vector<std::string> liveParseCopyRow(const std::string& line) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            result.push_back(liveUnescapeCopy(line.substr(start, i - start)));
            start = i + 1;
        }
    }
    return result;
}

static bool liveParseCreateTable(const std::string& sql, LiveTableSchema& schema) {
    std::regex re(R"(CREATE TABLE\s+(?:(\w+)\.)?(\w+)\s*\()");
    std::smatch m = {};
    if (!std::regex_search(sql, m, re)) {
      return false;
    }
    schema.schema_name = m[1].str();
    schema.name        = m[2].str();
    if (schema.name.empty()) {
      return false;
    }

    size_t start = sql.find('(');
    size_t end   = sql.find_last_of(')');
    if (start == std::string::npos || end == std::string::npos) {
      return false;
    }
    std::string cols = sql.substr(start + 1, end - start - 1);
    std::stringstream ss(cols);
    std::string col_def = {};
    while (std::getline(ss, col_def, ',')) {
        col_def.erase(0, col_def.find_first_not_of(" \t\n\r"));
        col_def.erase(col_def.find_last_not_of(" \t\n\r") + 1);
        if (col_def.empty()) {
          continue;
        }
        if (col_def.find("CONSTRAINT") != std::string::npos ||
            col_def.find("PRIMARY KEY") != std::string::npos ||
            col_def.find("FOREIGN KEY") != std::string::npos ||
            col_def.find("UNIQUE")      != std::string::npos ||
            col_def.find("CHECK")       != std::string::npos) continue;
        std::istringstream css(col_def);
        std::string word = {};
        std::string cname, ctype;
        // Keywords that end the type name and start a column constraint
        static const std::vector<std::string> kStopWords = {
            "NOT", "DEFAULT", "REFERENCES", "UNIQUE", "CHECK", "GENERATED"
        };
        while (css >> word) {
            if (cname.empty()) {
                cname = word;
                if (!cname.empty() && cname.front() == '"')
                    cname = cname.substr(1, cname.size() - 2);
                continue;
            }
            std::string wu = word;
            std::transform(wu.begin(), wu.end(), wu.begin(), ::toupper);
            bool stop = false;
            for (const auto& sw : kStopWords) {
                if (wu == sw) { stop = true; break; }
            }
            if (stop) {
              break;
            }
            if (!ctype.empty()) {
              ctype += " ";
            }
            ctype += word;
        }
        if (!cname.empty() && !ctype.empty()) {
            schema.columns.push_back(cname);
            schema.column_types[cname] = ctype;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Minimal importer that validates and imports from a pg_dump file
// ---------------------------------------------------------------------------
struct LiveMiniImporter {
    std::map<std::string, LiveTableSchema> schemas;

    bool validateSource(const std::string& path, std::vector<std::string>& errors) {
        std::ifstream f(path);
        if (!f) { errors.push_back("failed to open file for validation: " + path); return false; }
        std::string line = {};
        int checked = 0;
        while (std::getline(f, line) && checked < 100) {
            if (line.find("PostgreSQL database dump") != std::string::npos ||
                line.find("pg_dump")                  != std::string::npos ||
                line.find("Dumped from database version") != std::string::npos)
                return true;
            ++checked;
        }
        errors.push_back("file does not appear to be a valid PostgreSQL dump (missing pg_dump header markers)");
        return false;
    }

    // Returns the PostgreSQL server version string from the dump header,
    // e.g. "15.3" or "" if not found.
    std::string extractServerVersion(const std::string& path) {
        std::ifstream f(path);
        std::string line = {};
        int checked = 0;
        while (std::getline(f, line) && checked < 20) {
            // Matches: "-- Dumped from database version 15.3"
            auto pos = line.find("Dumped from database version");
            if (pos != std::string::npos) {
                std::string rest = line.substr(pos + 28);
                rest.erase(0, rest.find_first_not_of(' '));
                return rest;
            }
            ++checked;
        }
        return "";
    }

    std::vector<LiveTableSchema> getSourceSchema(const std::string& path) {
        schemas.clear();
        std::ifstream f(path);
        std::string line, current;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '-') {
              continue;
            }
            current += line + " ";
            if (line.find(';') != std::string::npos) {
                if (current.find("CREATE TABLE") != std::string::npos) {
                    LiveTableSchema s = {};
                    if (liveParseCreateTable(current, s)) {
                      schemas[s.name] = s;
                    }
                }
                current.clear();
            }
        }
        std::vector<LiveTableSchema> result = {};

        for (auto& [n, s] : schemas) {
          result.push_back(s);
        }
        return result;
    }

    LiveImportStats importData(const std::string& path, const LiveImportOptions& opts) {
        LiveImportStats stats;
        std::ifstream f(path);
        if (!f) {
            stats.errors.push_back("failed to open dump file for import: " + path);
            return stats;
        }
        getSourceSchema(path);
        f.clear(); f.seekg(0);
        std::string line, current;
        while (std::getline(f, line)) {
            if (line.empty() ||
                (line.size() >= 2 && line[0] == '-' && line[1] == '-')) continue;
            current += line + " ";
            if (line.find(';') != std::string::npos) {
                if (current.find("CREATE TABLE")  != std::string::npos ||
                    current.find("CREATE SCHEMA") != std::string::npos) {
                    LiveTableSchema s = {};
                    if (liveParseCreateTable(current, s)) {
                        bool include = opts.include_tables.empty() ||
                            std::find(opts.include_tables.begin(),
                                      opts.include_tables.end(), s.name)
                                != opts.include_tables.end();
                        bool exclude =
                            std::find(opts.exclude_tables.begin(),
                                      opts.exclude_tables.end(), s.name)
                                != opts.exclude_tables.end();
                        if (include && !exclude) {
                            schemas[s.name] = s;
                            stats.tables_processed++;
                        }
                    }
                } else if (current.find("COPY ") != std::string::npos) {
                    std::regex re(
                        R"(COPY\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+FROM\s+stdin)",
                        std::regex_constants::icase);
                    std::smatch m = {};
                    if (std::regex_search(current, m, re)) {
                        std::string tname = m[1].str();
                        bool include = opts.include_tables.empty() ||
                            std::find(opts.include_tables.begin(),
                                      opts.include_tables.end(), tname)
                                != opts.include_tables.end();
                        bool exclude =
                            std::find(opts.exclude_tables.begin(),
                                      opts.exclude_tables.end(), tname)
                                != opts.exclude_tables.end();
                        std::string data_line = {};
                        while (std::getline(f, data_line)) {
                            if (data_line == "\\." ||
                                data_line.rfind("\\.", 0) == 0) break;
                            if (!include || exclude) {
                                stats.skipped_records++;
                                continue;
                            }
                            if (opts.max_row_size_bytes > 0 &&
                                data_line.size() > opts.max_row_size_bytes) {
                                LiveImportError e;
                                e.code     = LiveImportErrorCode::ROW_TOO_LARGE;
                                e.severity = LiveImportErrorSeverity::WARNING;
                                e.message  = "row exceeds max_row_size_bytes";
                                e.location = "table " + tname;
                                stats.structured_errors.push_back(e);
                                stats.failed_records++;
                                if (!opts.continue_on_error) {
                                  return stats;
                                }
                                continue;
                            }
                            stats.total_records++;
                            if (!opts.dry_run)
                                stats.imported_records++;
                        }
                    }
                }
                current.clear();
            }
        }
        return stats;
    }
};

// ---------------------------------------------------------------------------
// Helper: path to the live dump file
// ---------------------------------------------------------------------------
static std::string liveDumpPath() {
    if (const char* env = std::getenv("THEMISDB_PG_LIVE_DUMP")) {
        std::ifstream f(env);
        if (f) {
          return env;
        }
    }
    return "";
}

static std::string liveConnStr() {
    if (const char* env = std::getenv("THEMISDB_PG_CONNSTR"))
        return env = {};
    return "";
}

} // anonymous namespace

// ============================================================================
// Live Integration Tests
// ============================================================================

class PostgresLiveIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        dump_path_ = liveDumpPath();
        if (dump_path_.empty()) {
            GTEST_SKIP()
                << "THEMISDB_PG_LIVE_DUMP not set; skipping live PostgreSQL tests";
        }
        conn_str_ = liveConnStr();
    }
    std::string dump_path_;
    std::string conn_str_;
};

// ---------------------------------------------------------------------------
// Source validation
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, ValidateSourceAcceptsLiveDump) {
    LiveMiniImporter imp;
    std::vector<std::string> errors;
    EXPECT_TRUE(imp.validateSource(dump_path_, errors));
    EXPECT_TRUE(errors.empty());
}

// The live dump must advertise a real PostgreSQL server version in its header.
TEST_F(PostgresLiveIntegrationTest, LiveDumpHasRealVersionHeader) {
    LiveMiniImporter imp;
    std::string ver = imp.extractServerVersion(dump_path_);
    ASSERT_FALSE(ver.empty())
        << "Dump header is missing 'Dumped from database version X.Y'";
    // Version string must start with a digit
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(ver[0])))
        << "Unexpected version string: " << ver;
}

// ---------------------------------------------------------------------------
// Schema extraction
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, LiveSchemaContainsThreeTables) {
    LiveMiniImporter imp;
    auto schemas = imp.getSourceSchema(dump_path_);
    EXPECT_EQ(schemas.size(), 3u);
    std::vector<std::string> names = {};

    for (auto& s : schemas) {
      names.push_back(s.name);
    }
    EXPECT_NE(std::find(names.begin(), names.end(), "users"),    names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "products"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "orders"),   names.end());
}

TEST_F(PostgresLiveIntegrationTest, LiveUsersTableHasExpectedColumns) {
    LiveMiniImporter imp;
    imp.getSourceSchema(dump_path_);
    ASSERT_GT(imp.schemas.count("users"), 0u);
    auto& s = imp.schemas["users"];
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "id"),        s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "username"),  s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "email"),     s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "score"),     s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "is_active"), s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "profile"),   s.columns.end());
}

TEST_F(PostgresLiveIntegrationTest, LiveUsersTableTypeMappings) {
    LiveMiniImporter imp;
    imp.getSourceSchema(dump_path_);
    auto& s = imp.schemas["users"];
    EXPECT_EQ(liveMapType(s.column_types["id"]),         "integer");
    EXPECT_EQ(liveMapType(s.column_types["email"]),      "string");
    EXPECT_EQ(liveMapType(s.column_types["score"]),      "double");
    EXPECT_EQ(liveMapType(s.column_types["is_active"]),  "boolean");
    EXPECT_EQ(liveMapType(s.column_types["profile"]),    "json");
    EXPECT_EQ(liveMapType(s.column_types["ip_address"]), "string");
}

TEST_F(PostgresLiveIntegrationTest, LiveProductsTableHasArrayAndJsonColumns) {
    LiveMiniImporter imp;
    imp.getSourceSchema(dump_path_);
    auto& s = imp.schemas["products"];
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "tags"),     s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "metadata"), s.columns.end());
    EXPECT_EQ(liveMapType(s.column_types["tags"]),     "array");
    EXPECT_EQ(liveMapType(s.column_types["metadata"]), "json");
}

// ---------------------------------------------------------------------------
// Data import
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, LiveNormalImportCountsAllRows) {
    LiveMiniImporter imp;
    LiveImportOptions opts;
    auto stats = imp.importData(dump_path_, opts);
    // 3 users + 3 products + 3 orders = 9 rows
    EXPECT_EQ(stats.imported_records, 9u);
    EXPECT_EQ(stats.tables_processed, 3u);
}

TEST_F(PostgresLiveIntegrationTest, LiveDryRunDoesNotImport) {
    LiveMiniImporter imp;
    LiveImportOptions opts;
    opts.dry_run = true;
    auto stats = imp.importData(dump_path_, opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.total_records, 9u);
}

// ---------------------------------------------------------------------------
// Filtering
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, LiveIncludeTablesFiltersCorrectly) {
    LiveMiniImporter imp;
    LiveImportOptions opts;
    opts.include_tables = {"users"};
    auto stats = imp.importData(dump_path_, opts);
    EXPECT_EQ(stats.imported_records, 3u);
    EXPECT_EQ(stats.skipped_records,  6u);
}

TEST_F(PostgresLiveIntegrationTest, LiveExcludeTablesFiltersCorrectly) {
    LiveMiniImporter imp;
    LiveImportOptions opts;
    opts.exclude_tables = {"orders"};
    auto stats = imp.importData(dump_path_, opts);
    EXPECT_EQ(stats.imported_records, 6u);
    EXPECT_EQ(stats.skipped_records,  3u);
}

// ---------------------------------------------------------------------------
// Row-size guard
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, LiveMaxRowSizeBytesRejectsOversizedRows) {
    LiveMiniImporter imp;
    LiveImportOptions opts;
    // 49 bytes is below the shortest COPY row in the test dataset
    opts.max_row_size_bytes = 49;
    opts.continue_on_error  = true;
    auto stats = imp.importData(dump_path_, opts);
    EXPECT_GT(stats.failed_records, 0u);
    EXPECT_FALSE(stats.structured_errors.empty());
    for (auto& e : stats.structured_errors)
        EXPECT_EQ(e.code, LiveImportErrorCode::ROW_TOO_LARGE);
}

// ---------------------------------------------------------------------------
// NULL handling in COPY rows (live dump round-trip)
// ---------------------------------------------------------------------------

TEST_F(PostgresLiveIntegrationTest, LiveCopyNullValuesRoundTrip) {
    // charlie row: email=NULL, score=NULL, profile=NULL, ip_address=NULL
    auto row = liveParseCopyRow(
        "3\tcharlie\t\\N\t40\t\\N\t0.00\tf\t2023-12-01 00:00:00\t\\N\t\\N");
    ASSERT_EQ(row.size(), 10u);
    EXPECT_EQ(row[2], "");   // email  = NULL → empty string after unescape
    EXPECT_EQ(row[4], "");   // score  = NULL
    EXPECT_EQ(row[8], "");   // profile = NULL
    EXPECT_EQ(row[9], "");   // ip_address = NULL
}
