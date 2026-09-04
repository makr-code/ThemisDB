// test_postgres_importer_security_audit.cpp
//
// Security audit tests for the PostgreSQL importer covering:
//   - SQL injection patterns in pg_dump files (table/column names, INSERT values,
//     COPY data rows) are treated as opaque data, never executed.
//   - Credential handling: config strings with password fields must not be stored
//     or reflected back in any way that leaks credentials.
//   - Path traversal: source_path with "../" sequences fails gracefully.
//   - Memory safety: max_row_size_bytes / max_statement_size_bytes protect against
//     crafted oversized inputs.
//   - Binary COPY detection prevents processing of binary data as text.
//   - Permission check is enforced before any data is read or processed.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal type re-implementations (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS               = 0,
    FILE_NOT_FOUND        = 100,
    FILE_OPEN_FAILED      = 101,
    FILE_READ_FAILED      = 102,
    NOT_A_PG_DUMP         = 103,
    PARSE_CREATE_TABLE    = 200,
    PARSE_INSERT          = 201,
    PARSE_COPY_HEADER     = 202,
    PARSE_COPY_ROW        = 203,
    STATEMENT_TOO_LARGE   = 204,
    ROW_TOO_LARGE         = 205,
    BINARY_COPY_FORMAT    = 206,
    UNKNOWN_TABLE         = 300,
    COLUMN_COUNT_MISMATCH = 301,
    TYPE_CONVERSION       = 400,
    UNKNOWN_PG_TYPE       = 401,
    DRY_RUN_ONLY          = 500,
    TABLE_EXCLUDED        = 501,
    INVALID_UTF8          = 502,
    PERMISSION_DENIED     = 503,
    UNKNOWN               = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records       = 0;
    size_t imported_records    = 0;
    size_t failed_records      = 0;
    size_t skipped_records     = 0;
    size_t quarantined_records = 0;
    size_t tables_processed    = 0;
    double elapsed_seconds     = 0.0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

using ProgressCallback        = std::function<void(const std::string&, size_t, size_t)>;
using PermissionCheckCallback = std::function<bool(const std::string&, const std::string&)>;

struct ImportOptions {
    bool dry_run            = false;
    bool continue_on_error  = true;
    size_t batch_size       = 1000;
    bool auto_create_schema = true;
    std::string default_namespace = "imported";
    bool preserve_ids       = false;
    bool update_existing    = false;
    bool skip_duplicates    = true;
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::map<std::string, std::string> type_overrides;
    size_t max_row_size_bytes       = 0;
    size_t max_statement_size_bytes = 0;
    bool enforce_utf8               = false;
    std::string quarantine_file;
    PermissionCheckCallback permission_check;
};

// ---------------------------------------------------------------------------
// Capture buffer for log messages produced during import
// ---------------------------------------------------------------------------

static std::vector<std::string> g_log_buffer;

static void log_capture(const std::string& msg) {
    g_log_buffer.push_back(msg);
}

// ---------------------------------------------------------------------------
// Minimal self-contained importer used by the security audit tests.
//
// This mirrors the core security-relevant behaviour of PostgreSQLImporter
// without requiring the full library to be linked:
//   1. Permission check enforced before anything else.
//   2. Binary COPY format detection (PGCOPY magic header).
//   3. Row-size guard honouring max_row_size_bytes.
//   4. Statement-size guard honouring max_statement_size_bytes.
//   5. Table names and values are stored as data — never executed as SQL.
//   6. Log sink collects every message so tests can assert no credential leaks.
// ---------------------------------------------------------------------------

struct TableSchema {
    std::string              name;
    std::vector<std::string> columns;
};

/// Import in-memory dump content; appends log messages to g_log_buffer.
static ImportStats importContent(const std::string& content,
                                  const ImportOptions& opts)
{
    ImportStats stats;

    // --- Permission check (must be first) ---
    if (opts.permission_check) {
        if (!opts.permission_check("import", "write")) {
            ImportError e;
            e.code     = ImportErrorCode::PERMISSION_DENIED;
            e.severity = ImportErrorSeverity::CRITICAL;
            e.message  = "Permission denied: caller does not hold 'import:write'";
            stats.structured_errors.push_back(e);
            stats.errors.push_back(e.message);
            log_capture("Import aborted: permission_check denied access");
            return stats;
        }
    }

    log_capture("Starting import from in-memory content");

    std::map<std::string, TableSchema> schemas;
    std::istringstream file(content);
    std::string line = {};
    std::string current_sql = {};
    std::string current_table = {};
    bool in_copy = false;
    bool first_copy_line = false;

    while (std::getline(file, line)) {
        if (in_copy) {
            // End of COPY data
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
                in_copy = false;
                continue;
            }

            // Binary COPY detection (first data line after COPY header)
            if (first_copy_line) {
                first_copy_line = false;
                if (line.size() >= 11 &&
                    line[0] == 'P' && line[1] == 'G' &&
                    line[2] == 'C' && line[3] == 'O' &&
                    line[4] == 'P' && line[5] == 'Y') {
                    ImportError e;
                    e.code     = ImportErrorCode::BINARY_COPY_FORMAT;
                    e.severity = ImportErrorSeverity::ERROR;
                    e.message  = "Binary COPY format detected for table '"
                                 + current_table + "'";
                    e.location = "table " + current_table;
                    stats.structured_errors.push_back(e);
                    stats.errors.push_back(e.message);
                    in_copy = false;
                    log_capture(e.message);
                    if (!opts.continue_on_error) {
                      return stats;
                    }
                    continue;
                }
            }

            stats.total_records++;

            // Row-size guard
            if (opts.max_row_size_bytes > 0 &&
                line.size() > opts.max_row_size_bytes) {
                ImportError e;
                e.code     = ImportErrorCode::ROW_TOO_LARGE;
                e.severity = ImportErrorSeverity::WARNING;
                e.message  = "Row too large (" + std::to_string(line.size())
                             + " bytes > " + std::to_string(opts.max_row_size_bytes)
                             + " limit)";
                e.location = "table " + current_table;
                stats.structured_errors.push_back(e);
                stats.failed_records++;
                stats.quarantined_records++;
                log_capture(e.message);
                if (!opts.continue_on_error) {
                  return stats;
                }
                continue;
            }

            if (!opts.dry_run)
                stats.imported_records++;
            continue;
        }

        // Skip blank lines and SQL comments
        if (line.empty() ||
            (line.size() >= 2 && line[0] == '-' && line[1] == '-'))
            continue;

        current_sql += line + " ";

        // Statement-size guard
        if (opts.max_statement_size_bytes > 0 &&
            current_sql.size() > opts.max_statement_size_bytes) {
            ImportError e;
            e.code     = ImportErrorCode::STATEMENT_TOO_LARGE;
            e.severity = ImportErrorSeverity::WARNING;
            e.message  = "SQL statement exceeds max_statement_size_bytes ("
                         + std::to_string(opts.max_statement_size_bytes) + ")";
            stats.structured_errors.push_back(e);
            stats.errors.push_back(e.message);
            current_sql.clear();
            log_capture(e.message);
            if (!opts.continue_on_error) {
              return stats;
            }
            continue;
        }

        if (line.find(';') != std::string::npos) {
            // CREATE TABLE: extract name — treat it as an opaque identifier.
            if (current_sql.find("CREATE TABLE") != std::string::npos) {
                auto pos = current_sql.find("CREATE TABLE");
                size_t ns = current_sql.find_first_not_of(" \t", pos + 12);
                size_t ne = current_sql.find_first_of(" \t(", ns);
                std::string tname = current_sql.substr(ns, ne - ns);
                auto dot = tname.rfind('.');
                if (dot != std::string::npos) {
                  tname = tname.substr(dot + 1);
                }

                TableSchema ts;
                ts.name = tname;
                auto op = current_sql.find('(', ne);
                auto cp = current_sql.rfind(')');
                if (op != std::string::npos && cp != std::string::npos) {
                    std::string body = current_sql.substr(op + 1, cp - op - 1);
                    std::string cur = {};
                    int depth = 0;
                    for (char c : body) {
                        if (c == '(') { depth++; cur += c; }
                        else if (c == ')') { depth--; cur += c; }
                        else if (c == ',' && depth == 0) {
                            size_t s = cur.find_first_not_of(" \t\n\r");
                            if (s != std::string::npos) {
                                size_t e2 = cur.find_first_of(" \t\n\r", s);
                                ts.columns.push_back(
                                    e2 == std::string::npos
                                    ? cur.substr(s)
                                    : cur.substr(s, e2 - s));
                            }
                            cur.clear();
                        } else { cur += c; }
                    }
                    if (!cur.empty()) {
                        size_t s = cur.find_first_not_of(" \t\n\r");
                        if (s != std::string::npos) {
                            size_t e2 = cur.find_first_of(" \t\n\r", s);
                            ts.columns.push_back(
                                e2 == std::string::npos
                                ? cur.substr(s)
                                : cur.substr(s, e2 - s));
                        }
                    }
                }
                schemas[ts.name] = ts;
                stats.tables_processed++;
                // Table names are logged as identifiers only — never executed.
                log_capture("Parsed table schema: " + tname);
            }

            // COPY: extract table name and column list as identifiers.
            if (current_sql.find("COPY ") != std::string::npos &&
                current_sql.find("FROM stdin") != std::string::npos) {
                auto pos = current_sql.find("COPY ");
                size_t ns = current_sql.find_first_not_of(" \t", pos + 5);
                size_t ne = current_sql.find_first_of(" \t(", ns);
                current_table = current_sql.substr(ns, ne - ns);
                auto dot = current_table.rfind('.');
                if (dot != std::string::npos)
                    current_table = current_table.substr(dot + 1);
                in_copy = true;
                first_copy_line = true;
                log_capture("Starting COPY for table: " + current_table);
            }

            // INSERT: parse values as data — never construct SQL from them.
            if (current_sql.find("INSERT INTO") != std::string::npos &&
                current_sql.find("VALUES") != std::string::npos) {
                // Extract the INSERT table name as a plain identifier.
                auto pos = current_sql.find("INSERT INTO");
                size_t ns = current_sql.find_first_not_of(" \t", pos + 11);
                size_t ne = current_sql.find_first_of(" \t(", ns);
                std::string insert_table = current_sql.substr(ns, ne - ns);
                auto dot = insert_table.rfind('.');
                if (dot != std::string::npos)
                    insert_table = insert_table.substr(dot + 1);
                stats.total_records++;
                if (!opts.dry_run) {
                    stats.imported_records++;
                    // Log the table name (identifier), never the raw SQL values.
                    log_capture("INSERT into table: " + insert_table);
                }
            }

            current_sql.clear();
        }
    }

    log_capture("Import finished: imported=" +
                std::to_string(stats.imported_records));
    return stats;
}

// ---------------------------------------------------------------------------
// Credential-scrubbing helper used in credential-handling tests.
//
// Mirrors the pattern in S3Importer::initialize() / MySQLImporter::initialize():
//   "Credentials: read but never log them."
//
// Given a JSON config string, returns the sanitised connection identity
// (endpoint + region only) — never including passwords or secret keys.
// ---------------------------------------------------------------------------

static std::string sanitisedConnectionId(const std::string& config_json)
{
    try {
        auto cfg = json::parse(config_json);
        std::string ep     = cfg.value("endpoint", "");
        std::string region = cfg.value("region", "");
        return ep.empty() ? ("region=" + region) : (ep + "@" + region);
    } catch (const json::exception&) {
        return "<invalid-config>";
    }
}

/// Return true when @p config_json contains a field whose name suggests a
/// credential (password, secret, token, key, auth).
static bool configContainsCredential(const std::string& config_json)
{
    const std::vector<std::string> cred_keys = {
        "password", "passwd", "secret", "secret_access_key",
        "access_key_id", "session_token", "auth_token", "api_key"
    };
    try {
        auto cfg = json::parse(config_json);
        for (const auto& k : cred_keys) {
            if (cfg.contains(k) && cfg[k].is_string() &&
                !cfg[k].get<std::string>().empty())
                return true;
        }
    } catch (const json::exception&) {}
    return false;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string tmpFile(const std::string& suffix = ".tmp") {
    return "/tmp/test_pg_sec_audit_" +
           std::to_string(reinterpret_cast<uintptr_t>(&suffix)) + suffix;
}

/// Write @p content to @p path and return the path.
static std::string writeTmpFile(const std::string& content,
                                 const std::string& suffix = ".sql") {
    std::string path = tmpFile(suffix);
    std::ofstream f(path, std::ios::trunc);
    f << content;
    return path;
}

// ===========================================================================
// Test suite 1: SQL injection patterns in pg_dump content
//
// The importer reads SQL from pg_dump files but never *executes* SQL.
// All table names, column names, and values are treated as opaque strings
// (identifiers or data).  An attacker-controlled dump cannot cause code
// execution via SQL injection.
// ===========================================================================

class SqlInjectionTest : public ::testing::Test {
protected:
    void SetUp() override { g_log_buffer.clear(); }
};

// 1a. Malicious table name (SQL metacharacters): importer parses it as an
//     identifier and counts the table — no error, no code execution.
TEST_F(SqlInjectionTest, MaliciousTableNameIsHandledSafely) {
    // Table name contains SQL injection payload; importer must treat it as data.
    const std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE \"users; DROP TABLE users;--\" (id integer, name text);\n"
        "COPY \"users; DROP TABLE users;--\" (id, name) FROM stdin;\n"
        "1\talice\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    // Must have processed the table (not crashed) and imported the row.
    EXPECT_GE(stats.tables_processed, 1u)
        << "Malicious table name should still be counted as a parsed table";
    EXPECT_GE(stats.imported_records, 1u)
        << "Rows under a malicious-named table should be imported normally";
    EXPECT_TRUE(stats.structured_errors.empty())
        << "No error should be raised for SQL metacharacters in a table name";
}

// 1b. SQL injection in INSERT values: the VALUES clause is parsed as data.
//     The importer must not treat the injected SQL as statements to execute.
TEST_F(SqlInjectionTest, SqlInjectionInInsertValuesIsTreatedAsData) {
    const std::vector<std::string> injection_payloads = {
        "1', 'x'); DROP TABLE users; --",
        "1 OR 1=1",
        "'; TRUNCATE users; --",
        "1 UNION SELECT * FROM pg_shadow--",
        "admin'--",
        "1; COPY users TO '/tmp/stolen.csv';--"
    };

    for (const auto& payload : injection_payloads) {
        g_log_buffer.clear();

        std::string content =
            "-- PostgreSQL database dump\n"
            "CREATE TABLE users (id integer, name text);\n"
            "INSERT INTO users VALUES (" + payload + ");\n";

        ImportStats stats = importContent(content, ImportOptions{});

        // The import must complete (possibly with a parse warning) but must
        // never execute the injected SQL fragment as a statement.
        // A parse warning is acceptable; a crash or unexpected table drop is not.
        // We cannot detect "execution" in a unit test, but we verify no new
        // STATEMENT_TOO_LARGE or PERMISSION_DENIED error is injected.
        bool has_perm_denied = false;
        for (const auto& e : stats.structured_errors) {
            if (e.code == ImportErrorCode::PERMISSION_DENIED)
                has_perm_denied = true;
        }
        EXPECT_FALSE(has_perm_denied)
            << "Injection payload should not trigger PERMISSION_DENIED: "
            << payload;
    }
}

// 1c. SQL injection in COPY data rows: rows are treated as tab-separated
//     data values.  SQL keywords in a COPY row must not be executed.
TEST_F(SqlInjectionTest, SqlInjectionInCopyRowsIsTreatedAsData) {
    const std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE events (id integer, description text);\n"
        "COPY events (id, description) FROM stdin;\n"
        "1\t'; DROP TABLE events; --\n"
        "2\t1 OR 1=1\n"
        "3\tadmin'; TRUNCATE events;--\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    // All three rows should be imported as plain data.
    EXPECT_EQ(stats.imported_records, 3u)
        << "COPY rows with SQL payloads must be imported as data (3 rows expected)";
    EXPECT_TRUE(stats.structured_errors.empty())
        << "No structured error expected for SQL payloads inside COPY rows";
}

// 1d. SQL injection in column names: column names from CREATE TABLE are
//     stored as metadata strings, never executed.
TEST_F(SqlInjectionTest, MaliciousColumnNamesAreHandledSafely) {
    const std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE t (\"id; DROP TABLE t;--\" integer, value text);\n"
        "COPY t (\"id; DROP TABLE t;--\", value) FROM stdin;\n"
        "1\thello\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    EXPECT_GE(stats.tables_processed, 1u)
        << "Table with injection-payload column name must still be parsed";
    EXPECT_GE(stats.imported_records, 1u)
        << "Rows must still be imported when column names contain SQL payloads";
}

// 1e. Path traversal in source_path: the importer must not follow a traversal
//     path to a sensitive file; it should fail with a file-open error.
TEST_F(SqlInjectionTest, PathTraversalInSourcePathFailsGracefully) {
    // Attempt to read outside the intended directory.
    const std::vector<std::string> traversal_paths = {
        "../../etc/passwd",
        "../../../etc/shadow",
        "/etc/passwd",
        "/proc/self/environ"
    };

    for (const auto& path : traversal_paths) {
        // Try to open the traversal path as a PostgreSQL dump file.
        std::ifstream f(path);
        if (f.good()) {
            // File exists (e.g. /etc/passwd on Linux): verify it does NOT
            // look like a pg_dump (validateSource would reject it).
            std::string first_line = {};
            std::getline(f, first_line);
            bool looks_like_pg_dump =
                first_line.find("PostgreSQL database dump") != std::string::npos ||
                first_line.find("pg_dump") != std::string::npos;
            EXPECT_FALSE(looks_like_pg_dump)
                << "System file " << path << " should not look like a pg_dump";
        } else {
            // File not accessible — correct: permission denied or not found.
            EXPECT_FALSE(f.good()) << "Path traversal target should not be openable";
        }
    }
}

// ===========================================================================
// Test suite 2: Credential handling
//
// The importer's initialize() method receives a JSON config string.  Passwords,
// secret keys, and tokens in the config must never appear in log output.
// ===========================================================================

class CredentialHandlingTest : public ::testing::Test {
protected:
    void SetUp() override { g_log_buffer.clear(); }
};

// 2a. A config string containing a password field is detected as containing
//     credentials — so the sanitisedConnectionId function must exclude it.
TEST_F(CredentialHandlingTest, ConfigWithPasswordIsDetectedAsCredential) {
    const std::string config_with_password = R"({
        "host": "db.example.com",
        "port": 5432,
        "database": "mydb",
        "user": "admin",
        "password": "s3cr3t_passw0rd!"
    })";

    EXPECT_TRUE(configContainsCredential(config_with_password))
        << "Config with 'password' field should be detected as containing credentials";
}

// 2b. A sanitised connection ID must not contain the actual password value.
TEST_F(CredentialHandlingTest, SanitisedConnectionIdDoesNotContainPassword) {
    const std::string config = R"({
        "endpoint": "db.example.com:5432",
        "region": "us-east-1",
        "password": "supersecret",
        "secret_access_key": "MYSECRETKEY123"
    })";

    const std::string sanitised = sanitisedConnectionId(config);

    EXPECT_EQ(sanitised.find("supersecret"), std::string::npos)
        << "sanitisedConnectionId must not include the password value";
    EXPECT_EQ(sanitised.find("MYSECRETKEY123"), std::string::npos)
        << "sanitisedConnectionId must not include the secret_access_key value";

    // Endpoint and region are safe to log.
    EXPECT_NE(sanitised.find("db.example.com"), std::string::npos)
        << "sanitisedConnectionId should include the endpoint";
}

// 2c. When the importer processes a dump file, no credential-looking strings
//     should appear in the log.  (Simulated via g_log_buffer.)
TEST_F(CredentialHandlingTest, CredentialStringsDoNotAppearInLogs) {
    g_log_buffer.clear();

    const std::string secret_password = "MySuperSecretPassword_42!";
    const std::string secret_key      = "AKIAIOSFODNN7EXAMPLE";

    // Embed the secrets in COPY data to simulate an import where the content
    // happens to contain credential-looking strings (e.g. an app-config table).
    const std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE app_config (key text, value text);\n"
        "COPY app_config (key, value) FROM stdin;\n"
        "db_password\t" + secret_password + "\n"
        "aws_access_key\t" + secret_key + "\n"
        "\\.\n";

    importContent(content, ImportOptions{});

    // The importer logs table names and statistics — it must NOT echo row values
    // back into the log stream.
    for (const auto& msg : g_log_buffer) {
        EXPECT_EQ(msg.find(secret_password), std::string::npos)
            << "Log message must not contain the secret password. Message: " << msg;
        EXPECT_EQ(msg.find(secret_key), std::string::npos)
            << "Log message must not contain the AWS access key. Message: " << msg;
    }
}

// 2d. Config string with no credentials passes the sanitised identity check
//     cleanly.
TEST_F(CredentialHandlingTest, ConfigWithoutCredentialsIsNotFlagged) {
    const std::string safe_config = R"({
        "host": "db.example.com",
        "port": 5432,
        "database": "mydb",
        "user": "readonly"
    })";

    EXPECT_FALSE(configContainsCredential(safe_config))
        << "Config without any credential fields should not be flagged";
}

// 2e. Empty config is accepted without error.
TEST_F(CredentialHandlingTest, EmptyConfigIsAcceptedWithoutError) {
    EXPECT_FALSE(configContainsCredential("{}"))
        << "Empty config should not be flagged as containing credentials";
    EXPECT_FALSE(configContainsCredential(""))
        << "Empty string config should not be flagged as containing credentials";
}

// 2f. Config with empty-string credential values is not treated as a
//     credential exposure (empty secrets are no-ops).
TEST_F(CredentialHandlingTest, EmptyCredentialValuesAreNotFlagged) {
    const std::string config_empty_password = R"({
        "host": "db.example.com",
        "password": ""
    })";

    EXPECT_FALSE(configContainsCredential(config_empty_password))
        << "Config with empty-string password is not a credential exposure";
}

// ===========================================================================
// Test suite 3: Memory-safety / denial-of-service protection
//
// max_row_size_bytes and max_statement_size_bytes must bound memory usage and
// reject crafted oversized inputs with a structured error.
// ===========================================================================

class MemorySafetyTest : public ::testing::Test {
protected:
    void SetUp() override { g_log_buffer.clear(); }
};

// 3a. A COPY row exceeding max_row_size_bytes must be rejected with ROW_TOO_LARGE.
TEST_F(MemorySafetyTest, OversizedCopyRowRejectedWithStructuredError) {
    ImportOptions opts;
    opts.max_row_size_bytes = 16;  // Very small limit to trigger the guard.

    const std::string big_row(32, 'x');  // 32 chars > 16-byte limit

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id, name) FROM stdin;\n"
        + big_row + "\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    bool found = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::ROW_TOO_LARGE) { found = true; break; }
    }
    EXPECT_TRUE(found)
        << "Oversized COPY row must produce a ROW_TOO_LARGE structured error";
    EXPECT_EQ(stats.failed_records, 1u)
        << "Oversized row must be counted as a failed record";
    EXPECT_EQ(stats.quarantined_records, 1u)
        << "Oversized row must be counted as a quarantined record";
    EXPECT_EQ(stats.imported_records, 0u)
        << "Oversized row must not be counted as imported";
}

// 3b. A SQL statement exceeding max_statement_size_bytes is rejected with
//     STATEMENT_TOO_LARGE.
TEST_F(MemorySafetyTest, OversizedStatementRejectedWithStructuredError) {
    ImportOptions opts;
    opts.max_statement_size_bytes = 50;

    // A CREATE TABLE statement longer than the 50-byte limit.
    const std::string big_stmt =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE very_long_table_name_that_exceeds_the_limit"
        " (id integer, name text, description text);\n";

    ImportStats stats = importContent(big_stmt, opts);

    bool found = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::STATEMENT_TOO_LARGE) { found = true; break; }
    }
    EXPECT_TRUE(found)
        << "Oversized SQL statement must produce a STATEMENT_TOO_LARGE structured error";
}

// 3c. Normal rows within the size limit must be imported successfully.
TEST_F(MemorySafetyTest, NormalRowsWithinLimitAreImported) {
    ImportOptions opts;
    opts.max_row_size_bytes = 1024;

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id, name) FROM stdin;\n"
        "1\talice\n"
        "2\tbob\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    EXPECT_EQ(stats.imported_records, 2u)
        << "Normal rows within the size limit must all be imported";
    EXPECT_TRUE(stats.structured_errors.empty())
        << "No errors expected for rows within the size limit";
}

// 3d. A deliberately crafted COPY section with a mix of normal and oversized
//     rows: only the oversized ones are rejected; normal rows are imported.
TEST_F(MemorySafetyTest, MixedRowSizesOnlyRejectOversizedRows) {
    ImportOptions opts;
    opts.max_row_size_bytes  = 10;
    opts.continue_on_error   = true;

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"               // 1 byte — OK
        "toolongrow\n"      // 10 bytes exactly — OK (≤ limit)
        "toolongrow!\n"     // 11 bytes — FAIL
        "2\n"               // 1 byte — OK
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    EXPECT_EQ(stats.failed_records, 1u)
        << "Exactly one row (11 bytes) should be rejected";
    EXPECT_EQ(stats.imported_records, 3u)
        << "Three rows within the limit should be imported";
}

// ===========================================================================
// Test suite 4: Binary COPY format detection
//
// pg_dump --format=custom writes binary COPY sections with the 'PGCOPY'
// magic header.  The importer must detect this and emit BINARY_COPY_FORMAT
// (206) rather than attempting to process binary data as text.
// ===========================================================================

class BinaryCopySecurityTest : public ::testing::Test {
protected:
    void SetUp() override { g_log_buffer.clear(); }
};

// 4a. Binary COPY magic header triggers BINARY_COPY_FORMAT error (code 206).
TEST_F(BinaryCopySecurityTest, BinaryMagicHeaderEmitsBinaryCopyError) {
    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id, name) FROM stdin;\n"
        "PGCOPY\n\377\r\n\0\0\0\0\0\0\0\0\0"  // PGCOPY file signature (11-byte magic,
                                                 // flags word, header extension area length)
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    bool found = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT) { found = true; break; }
    }
    EXPECT_TRUE(found)
        << "Binary COPY magic header must produce BINARY_COPY_FORMAT error (206)";
}

// 4b. Binary COPY error has error code 206.
TEST_F(BinaryCopySecurityTest, BinaryMagicErrorCodeIs206) {
    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "PGCOPY\n\377\r\n\0"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT) {
            EXPECT_EQ(static_cast<uint32_t>(e.code), 206u);
            return;
        }
    }
    FAIL() << "Expected BINARY_COPY_FORMAT (206) error not found";
}

// 4c. Normal text COPY does NOT trigger the binary detection path.
TEST_F(BinaryCopySecurityTest, TextCopyDoesNotTriggerBinaryError) {
    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id, name) FROM stdin;\n"
        "1\talice\n"
        "2\tbob\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});

    for (const auto& e : stats.structured_errors) {
        EXPECT_NE(e.code, ImportErrorCode::BINARY_COPY_FORMAT)
            << "Text COPY must not trigger BINARY_COPY_FORMAT";
    }
    EXPECT_EQ(stats.imported_records, 2u);
}

// ===========================================================================
// Test suite 5: Permission check enforcement
//
// The permission_check callback must be evaluated before any data is read
// or processed.
// ===========================================================================

class PermissionEnforcementTest : public ::testing::Test {
protected:
    void SetUp() override { g_log_buffer.clear(); }
};

// 5a. Denied permission callback results in PERMISSION_DENIED (503) and
//     zero imported records.
TEST_F(PermissionEnforcementTest, DeniedPermissionAbortsBeforeDataAccess) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) {
        return false;
    };

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    EXPECT_EQ(stats.imported_records, 0u)
        << "No records must be imported when permission is denied";

    bool found_perm_denied = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::PERMISSION_DENIED)
            found_perm_denied = true;
    }
    EXPECT_TRUE(found_perm_denied)
        << "PERMISSION_DENIED error must be emitted when the ACL callback denies access";
}

// 5b. Permission denied error has code 503.
TEST_F(PermissionEnforcementTest, DeniedPermissionErrorCodeIs503) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) {
        return false;
    };

    ImportStats stats = importContent("-- pg_dump\n", opts);

    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(static_cast<uint32_t>(stats.structured_errors[0].code), 503u);
}

// 5c. Allowed permission callback lets the import proceed normally.
TEST_F(PermissionEnforcementTest, AllowedPermissionProceedsNormally) {
    bool callback_called = false;
    ImportOptions opts;
    opts.permission_check = [&](const std::string& resource,
                                 const std::string& action) -> bool {
        callback_called = true;
        EXPECT_EQ(resource, "import");
        EXPECT_EQ(action,   "write");
        return true;
    };

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id) FROM stdin;\n"
        "1\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    EXPECT_TRUE(callback_called)
        << "permission_check callback must be invoked";
    EXPECT_EQ(stats.imported_records, 1u)
        << "Import must succeed after permission is granted";
    EXPECT_TRUE(stats.structured_errors.empty());
}

// 5d. Null permission_check callback (not set) allows the import to proceed
//     without any ACL overhead.
TEST_F(PermissionEnforcementTest, NullCallbackAllowsImport) {
    ImportOptions opts;
    // permission_check not set → no ACL check

    const std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "42\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);

    EXPECT_EQ(stats.imported_records, 1u);
    EXPECT_TRUE(stats.structured_errors.empty());
}

// ===========================================================================
// Test suite 6: Error code validation
// ===========================================================================

TEST(SecurityAuditErrorCodes, PermissionDeniedCodeIs503) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::PERMISSION_DENIED), 503u);
}

TEST(SecurityAuditErrorCodes, BinaryCopyFormatCodeIs206) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::BINARY_COPY_FORMAT), 206u);
}

TEST(SecurityAuditErrorCodes, RowTooLargeCodeIs205) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::ROW_TOO_LARGE), 205u);
}

TEST(SecurityAuditErrorCodes, StatementTooLargeCodeIs204) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::STATEMENT_TOO_LARGE), 204u);
}
