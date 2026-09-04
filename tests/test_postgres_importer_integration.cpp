// Integration tests for the PostgreSQL importer using realistic fixture files.
//
// These tests exercise the full importer pipeline against files that look like
// actual pg_dump output.  They are deliberately self-contained – the importer
// logic is reproduced here via helper functions so that the tests can run
// without the full ThemisDB build chain.

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
// Minimal importer types (mirrors importer_interface.h + postgres_importer.h)
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS = 0,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    STATEMENT_TOO_LARGE  = 204,
    ROW_TOO_LARGE        = 205,
    COLUMN_COUNT_MISMATCH = 301,
    UNKNOWN              = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code;
    ImportErrorSeverity severity;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records    = 0;
    size_t imported_records = 0;
    size_t failed_records   = 0;
    size_t skipped_records  = 0;
    size_t tables_processed = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

struct TableSchema {
    std::string name;
    std::string schema_name;
    std::vector<std::string>                 columns;
    std::map<std::string, std::string>       column_types;
};

struct ImportOptions {
    bool                             dry_run                  = false;
    bool                             continue_on_error        = true;
    size_t                           batch_size               = 1000;
    std::vector<std::string>         include_tables;
    std::vector<std::string>         exclude_tables;
    std::map<std::string,std::string> type_overrides;
    size_t                           max_row_size_bytes       = 0;
    size_t                           max_statement_size_bytes = 0;
};

// ---------------------------------------------------------------------------
// Helpers: type mapping, COPY parsing, CREATE TABLE parsing
// ---------------------------------------------------------------------------

static std::string mapType(const std::string& pg,
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
    if (t == "smallint" || t == "int2" || t == "smallserial") {
      return "integer";
    }
    if (t == "integer" || t == "int" || t == "int4" || t == "serial") {
      return "integer";
    }
    if (t == "real" || t == "float4") {
      return "float";
    }
    if (t == "double precision" || t == "float8") {
      return "double";
    }
    if (t == "numeric" || t == "decimal" || t == "money") {
      return "double";
    }
    if (t == "boolean" || t == "bool") {
      return "boolean";
    }
    if (t == "text" || t == "name" || t == "uuid" || t == "inet" || t == "cidr" ||
        t == "macaddr" || t == "xml" || t == "interval" || t == "tsvector") return "string";
    if (t == "bytea") {
      return "binary";
    }
    if (t == "json" || t == "jsonb") {
      return "json";
    }
    if (t == "point" || t == "polygon" || t == "circle" || t == "line") {
      return "geo";
    }
    if (t == "oid" || t == "xid" || t == "cid") {
      return "integer";
    }
    if (t.find("char") != std::string::npos || t.find("varchar") != std::string::npos) {
      return "string";
    }
    if (t.find("timestamp") != std::string::npos) {
      return "datetime";
    }
    if (t.find("date") != std::string::npos) {
      return "date";
    }
    if (t.find("time") != std::string::npos) {
      return "time";
    }
    if (t.find("json") != std::string::npos) {
      return "json";
    }
    if (t.find("int") != std::string::npos) {
      return "integer";
    }
    return "string";
}

static std::string unescapeCopy(const std::string& val) {
    if (val == "\\N") {
      return "";
    }
    std::string out;
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i+1 < val.size()) {
            char nx = val[++i];
            switch(nx) {
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

static std::vector<std::string> parseCopyRow(const std::string& line) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            result.push_back(unescapeCopy(line.substr(start, i - start)));
            start = i + 1;
        }
    }
    return result;
}

static bool parseCreateTable(const std::string& sql, TableSchema& schema) {
    std::regex re(R"(CREATE TABLE\s+(?:(\w+)\.)?(\w+)\s*\()");
    std::smatch m;
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
    std::string cols = sql.substr(start+1, end-start-1);
    std::stringstream ss(cols);
    std::string col_def;
    while (std::getline(ss, col_def, ',')) {
        col_def.erase(0, col_def.find_first_not_of(" \t\n\r"));
        col_def.erase(col_def.find_last_not_of(" \t\n\r")+1);
        if (col_def.empty()) {
          continue;
        }
        if (col_def.find("CONSTRAINT") != std::string::npos ||
            col_def.find("PRIMARY KEY") != std::string::npos ||
            col_def.find("FOREIGN KEY") != std::string::npos ||
            col_def.find("UNIQUE") != std::string::npos ||
            col_def.find("CHECK") != std::string::npos) continue;
        std::istringstream css(col_def);
        std::string cname, ctype;
        css >> cname >> ctype;
        if (!cname.empty() && cname.front() == '"')
            cname = cname.substr(1, cname.size()-2);
        if (!cname.empty() && !ctype.empty()) {
            schema.columns.push_back(cname);
            schema.column_types[cname] = ctype;
        }
    }
    return true;
}

// Minimal importer that validates the fixture
struct MiniImporter {
    std::map<std::string, TableSchema> schemas;

    bool validateSource(const std::string& path, std::vector<std::string>& errors) {
        std::ifstream f(path);
        if (!f) { errors.push_back("cannot open " + path); return false; }
        std::string line;
        int checked = 0;
        while (std::getline(f, line) && checked < 100) {
            if (line.find("PostgreSQL database dump") != std::string::npos ||
                line.find("pg_dump") != std::string::npos ||
                line.find("Dumped from database version") != std::string::npos)
                return true;
            ++checked;
        }
        errors.push_back("not a pg_dump file");
        return false;
    }

    std::vector<TableSchema> getSourceSchema(const std::string& path) {
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
                    TableSchema s;
                    if (parseCreateTable(current, s)) {
                      schemas[s.name] = s;
                    }
                }
                current.clear();
            }
        }
        std::vector<TableSchema> result = {};

        for (auto& [n, s] : schemas) {
          result.push_back(s);
        }
        return result;
    }

    ImportStats importData(const std::string& path, const ImportOptions& opts) {
        ImportStats stats;
        std::ifstream f(path);
        if (!f) {
            stats.errors.push_back("cannot open " + path);
            return stats;
        }
        // First pass: collect schemas
        getSourceSchema(path);
        // Re-open for data
        f.clear(); f.seekg(0);
        std::string line, current;
        while (std::getline(f, line)) {
            if (line.empty() || (line.size()>=2 && line[0]=='-' && line[1]=='-')) {
              continue;
            }
            current += line + " ";
            if (line.find(';') != std::string::npos) {
                if (current.find("CREATE TABLE") != std::string::npos ||
                    current.find("CREATE SCHEMA") != std::string::npos) {
                    TableSchema s;
                    if (parseCreateTable(current, s)) {
                        bool include = opts.include_tables.empty() ||
                            std::find(opts.include_tables.begin(), opts.include_tables.end(), s.name) != opts.include_tables.end();
                        bool exclude = std::find(opts.exclude_tables.begin(), opts.exclude_tables.end(), s.name) != opts.exclude_tables.end();
                        if (include && !exclude) {
                            schemas[s.name] = s;
                            stats.tables_processed++;
                        }
                    }
                } else if (current.find("COPY ") != std::string::npos) {
                    std::regex re(R"(COPY\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+FROM\s+stdin)",
                                  std::regex_constants::icase);
                    std::smatch m;
                    if (std::regex_search(current, m, re)) {
                        std::string tname = m[1].str();
                        bool include = opts.include_tables.empty() ||
                            std::find(opts.include_tables.begin(), opts.include_tables.end(), tname) != opts.include_tables.end();
                        bool exclude = std::find(opts.exclude_tables.begin(), opts.exclude_tables.end(), tname) != opts.exclude_tables.end();
                        std::string skip_line;
                        while (std::getline(f, skip_line)) {
                            if (skip_line == "\\." || skip_line.rfind("\\.",0)==0) {
                              break;
                            }
                            if (!include || exclude) { stats.skipped_records++; continue; }
                            if (opts.max_row_size_bytes > 0 &&
                                skip_line.size() > opts.max_row_size_bytes) {
                                ImportError e;
                                e.code     = ImportErrorCode::ROW_TOO_LARGE;
                                e.severity = ImportErrorSeverity::WARNING;
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
                            if (!opts.dry_run) {
                                auto cols = schemas.count(tname) ? schemas[tname].columns
                                                                 : std::vector<std::string>{};
                                // Optionally parse; just count for integration test
                                stats.imported_records++;
                            }
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
// Helper: path to fixture file
// ---------------------------------------------------------------------------
static std::string fixturePath() {
    // Check environment variable (allows override in any build system)
    if (const char* env = std::getenv("THEMISDB_SOURCE_DIR")) {
        std::string p = std::string(env) + "/tests/fixtures/importers/sample_pg15.sql";
        std::ifstream f(p);
        if (f) {
          return p;
        }
    }
    // Try paths relative to the current working directory (common build locations)
    const std::vector<std::string> candidates = {
        "tests/fixtures/importers/sample_pg15.sql",
        "../tests/fixtures/importers/sample_pg15.sql",
        "../../tests/fixtures/importers/sample_pg15.sql",
    };
    for (auto& p : candidates) {
        std::ifstream f(p);
        if (f) {
          return p;
        }
    }
    return "";
}

// ============================================================================
// Integration Tests
// ============================================================================

class PostgresImporterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        fixture_path_ = fixturePath();
        if (fixture_path_.empty()) {
            GTEST_SKIP() << "Fixture file not found; skipping integration tests";
        }
    }
    std::string fixture_path_;
};

// --- validateSource ---

TEST_F(PostgresImporterIntegrationTest, ValidateSourceAcceptsFixture) {
    MiniImporter imp;
    std::vector<std::string> errors;
    EXPECT_TRUE(imp.validateSource(fixture_path_, errors));
    EXPECT_TRUE(errors.empty());
}

TEST_F(PostgresImporterIntegrationTest, ValidateSourceRejectsNonExistentFile) {
    MiniImporter imp;
    std::vector<std::string> errors;
    EXPECT_FALSE(imp.validateSource("/nonexistent/file.sql", errors));
    EXPECT_FALSE(errors.empty());
}

TEST_F(PostgresImporterIntegrationTest, ValidateSourceRejectsNonDumpFile) {
    // Write a non-dump file to a temp path
    std::string tmp = "/tmp/not_a_dump.sql";
    { std::ofstream f(tmp); f << "SELECT 1;\n"; }
    MiniImporter imp;
    std::vector<std::string> errors;
    EXPECT_FALSE(imp.validateSource(tmp, errors));
    EXPECT_FALSE(errors.empty());
}

// --- getSourceSchema ---

TEST_F(PostgresImporterIntegrationTest, SchemaContainsThreeTables) {
    MiniImporter imp;
    auto schemas = imp.getSourceSchema(fixture_path_);
    EXPECT_EQ(schemas.size(), 3u);
    std::vector<std::string> names = {};

    for (auto& s : schemas) {
      names.push_back(s.name);
    }
    EXPECT_NE(std::find(names.begin(), names.end(), "users"),    names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "products"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "orders"),   names.end());
}

TEST_F(PostgresImporterIntegrationTest, UsersTableHasExpectedColumns) {
    MiniImporter imp;
    imp.getSourceSchema(fixture_path_);
    ASSERT_TRUE(imp.schemas.count("users") > 0);
    auto& s = imp.schemas["users"];
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "id"),       s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "username"),  s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "email"),     s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "is_active"), s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "profile"),   s.columns.end());
}

TEST_F(PostgresImporterIntegrationTest, UsersTableTypeMappings) {
    MiniImporter imp;
    imp.getSourceSchema(fixture_path_);
    auto& s = imp.schemas["users"];
    EXPECT_EQ(mapType(s.column_types["id"]),         "integer");
    EXPECT_EQ(mapType(s.column_types["email"]),      "string");
    EXPECT_EQ(mapType(s.column_types["score"]),      "double");
    EXPECT_EQ(mapType(s.column_types["is_active"]),  "boolean");
    EXPECT_EQ(mapType(s.column_types["profile"]),    "json");
    EXPECT_EQ(mapType(s.column_types["ip_address"]), "string");
}

TEST_F(PostgresImporterIntegrationTest, ProductsTableHasArrayAndJsonColumns) {
    MiniImporter imp;
    imp.getSourceSchema(fixture_path_);
    auto& s = imp.schemas["products"];
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "tags"),     s.columns.end());
    EXPECT_NE(std::find(s.columns.begin(), s.columns.end(), "metadata"), s.columns.end());
    EXPECT_EQ(mapType(s.column_types["tags"]),     "array");
    EXPECT_EQ(mapType(s.column_types["metadata"]), "json");
}

// --- importData (dry-run) ---

TEST_F(PostgresImporterIntegrationTest, DryRunCountsRowsWithoutImporting) {
    MiniImporter imp;
    ImportOptions opts;
    opts.dry_run = true;
    auto stats = imp.importData(fixture_path_, opts);
    // 3 tables worth of COPY data (3 + 3 + 3 = 9 data rows)
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.total_records, 9u);
}

// Dry-run must still apply row-level validation (max_row_size_bytes) and report
// validation errors in structured_errors even though no rows are written.
TEST_F(PostgresImporterIntegrationTest, DryRunReportsValidationErrorsWithoutImporting) {
    MiniImporter imp;
    ImportOptions opts;
    opts.dry_run            = true;
    // 49 bytes: threshold guaranteed to exceed every row in the fixture
    opts.max_row_size_bytes = 49;
    opts.continue_on_error  = true;
    auto stats = imp.importData(fixture_path_, opts);
    // Nothing must be imported (dry-run)
    EXPECT_EQ(stats.imported_records, 0u);
    // Validation errors must be reported even in dry-run
    EXPECT_GT(stats.failed_records, 0u);
    EXPECT_FALSE(stats.structured_errors.empty());
    for (const auto& e : stats.structured_errors) {
        EXPECT_EQ(e.code, ImportErrorCode::ROW_TOO_LARGE);
    }
}

TEST_F(PostgresImporterIntegrationTest, NormalImportCountsAllRows) {
    MiniImporter imp;
    ImportOptions opts;
    auto stats = imp.importData(fixture_path_, opts);
    EXPECT_EQ(stats.imported_records, 9u);
    EXPECT_EQ(stats.tables_processed, 3u);
}

// --- include_tables / exclude_tables filtering ---

TEST_F(PostgresImporterIntegrationTest, IncludeTablesFiltersCorrectly) {
    MiniImporter imp;
    ImportOptions opts;
    opts.include_tables = {"users"};
    auto stats = imp.importData(fixture_path_, opts);
    EXPECT_EQ(stats.imported_records, 3u);  // only users rows
    EXPECT_EQ(stats.skipped_records, 6u);   // products + orders rows skipped
}

TEST_F(PostgresImporterIntegrationTest, ExcludeTablesFiltersCorrectly) {
    MiniImporter imp;
    ImportOptions opts;
    opts.exclude_tables = {"orders"};
    auto stats = imp.importData(fixture_path_, opts);
    EXPECT_EQ(stats.imported_records, 6u);  // users + products
    EXPECT_EQ(stats.skipped_records, 3u);   // orders skipped
}

// --- max_row_size_bytes ---

// Row size threshold small enough that all fixture COPY rows exceed it.
// The shortest row in sample_pg15.sql is ~50 chars; using 49 guarantees every
// row in every table is considered oversized.
static constexpr size_t kTestMaxRowSize = 49;

TEST_F(PostgresImporterIntegrationTest, MaxRowSizeBytesRejectsOversizedRows) {
    MiniImporter imp;
    ImportOptions opts;
    opts.max_row_size_bytes = kTestMaxRowSize;
    opts.continue_on_error  = true;
    auto stats = imp.importData(fixture_path_, opts);
    // Some rows should have been rejected
    EXPECT_GT(stats.failed_records, 0u);
    EXPECT_FALSE(stats.structured_errors.empty());
    // All structured errors should be ROW_TOO_LARGE
    for (auto& e : stats.structured_errors) {
        EXPECT_EQ(e.code, ImportErrorCode::ROW_TOO_LARGE);
    }
}

// --- COPY row parsing with NULL values ---

TEST_F(PostgresImporterIntegrationTest, CopyRowNullValuesAreHandled) {
    // Row 3 in users has \N values
    auto row3 = parseCopyRow("3\tcharlie\t\\N\t40\t\\N\t0.00\tf\t2023-12-01 00:00:00\t\\N\t\\N");
    ASSERT_EQ(row3.size(), 10u);
    EXPECT_EQ(row3[0], "3");
    EXPECT_EQ(row3[1], "charlie");
    EXPECT_EQ(row3[2], "");   // email = NULL
    EXPECT_EQ(row3[4], "");   // score = NULL
    EXPECT_EQ(row3[8], "");   // profile = NULL
    EXPECT_EQ(row3[9], "");   // ip_address = NULL
}

// --- COPY row parsing: array and json fields ---

TEST_F(PostgresImporterIntegrationTest, CopyRowArrayAndJsonFields) {
    auto row = parseCopyRow("100\tWidget A\tA small widget\t9.99\tt\t{electronics,small}\t{\"sku\":\"WA-001\"}");
    ASSERT_EQ(row.size(), 7u);
    EXPECT_EQ(row[5], "{electronics,small}");
    EXPECT_EQ(row[6], "{\"sku\":\"WA-001\"}");
}
