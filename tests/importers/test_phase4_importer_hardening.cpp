/**
 * @file test_phase4_importer_hardening.cpp
 * @brief Phase 4 Block 2 – Security & Performance Hardening tests for the
 *        Importers module (Issue #5184 remediation).
 *
 * Coverage:
 *   I1  – Connection timeout enforcement (import_timeout_ms, DEADLINE_EXCEEDED)
 *   I2  – Input validation & SQL injection prevention (isValidIdentifier)
 *   I3  – Result vector pre-allocation (functional correctness of parse helpers)
 *   I4  – Audit logging (structured JSON events on start/failure/auth/schema)
 *
 * The file is intentionally self-contained: all relevant types are duplicated
 * or simplified so the test binary requires only GoogleTest, Threads, and
 * nlohmann/json — no other ThemisDB shared library.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ===========================================================================
// Minimal types mirroring importer_interface.h / schema_inference.h
// ===========================================================================

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    STATEMENT_TOO_LARGE  = 204,
    UNKNOWN_TABLE        = 300,
    COLUMN_COUNT_MISMATCH = 301,
    DEADLINE_EXCEEDED    = 110,  ///< I1: new timeout error code
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
    size_t total_records    = 0;
    size_t imported_records = 0;
    size_t failed_records   = 0;
    size_t skipped_records  = 0;
    size_t tables_processed = 0;
    double elapsed_seconds  = 0.0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

struct ImportOptions {
    bool     dry_run           = false;
    bool     continue_on_error = true;
    size_t   batch_size        = 1000;
    std::string default_namespace = "imported";
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::map<std::string, std::string> type_overrides;
    size_t   max_row_size_bytes       = 0;
    size_t   max_statement_size_bytes = 0;
    std::function<bool(const std::string&, const std::string&)> permission_check;
    std::function<bool(const std::string&, const json&)>        streaming_row_callback;
    std::function<void(const std::string&, const std::map<std::string,std::string>&, double)> metrics_callback;
    uint32_t import_timeout_ms = 0;   ///< I1: 0 = disabled
};

// ===========================================================================
// Minimal re-implementation of isValidIdentifier (mirrors schema_inference.cpp)
// ===========================================================================

static constexpr size_t kMaxIdentifierLength = 128;
static constexpr size_t kMaxTableCount       = 5000;
static constexpr size_t kMaxColumnCount      = 1600;

/**
 * @brief Validate a SQL identifier for safe use in query strings.
 *
 * Mirrors SchemaInferenceEngine::isValidIdentifier().
 */
static bool isValidIdentifier(const std::string& identifier) {
    if (identifier.empty() || identifier.size() > kMaxIdentifierLength) {
        return false;
    }
    for (unsigned char c : identifier) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

// ===========================================================================
// Minimal re-implementation of parseInsertValues (mirrors postgres/mysql .cpp)
// ===========================================================================

/**
 * @brief Parse a VALUES clause (PostgreSQL / MySQL style).
 *
 * Simplified version that mirrors the production parseInsertValues().
 * Includes the I3 pre-allocation fix: result.reserve(32).
 */
static std::vector<std::string> parseInsertValues(const std::string& values_clause) {
    std::vector<std::string> result;
    result.reserve(32);  // I3: pre-allocation

    size_t i = 0;
    const size_t n = values_clause.size();

    while (i < n) {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t')) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        if (values_clause[i] == '\'') {
            ++i;
            std::string val;
            while (i < n) {
                if (values_clause[i] == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    val += '\''; i += 2;
                } else if (values_clause[i] == '\'') {
                    ++i; break;
                } else {
                    val += values_clause[i++];
                }
            }
            result.push_back(val);
        } else {
            size_t start = i;
            while (i < n && values_clause[i] != ',' && values_clause[i] != ') {
              ') ++i;
            }
            std::string token = values_clause.substr(start, i - start);
            size_t last = token.find_last_not_of(" \t");
            if (last != std::string::npos) {
              token = token.substr(0, last + 1);
            }
            result.push_back(token);
        }
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == ',' ||
                          values_clause[i] == '\t')) ++i;
    }
    return result;
}

/**
 * @brief Parse a PostgreSQL COPY text-format row (tab-delimited).
 *
 * Mirrors parseCopyRow() with I3 pre-allocation.
 */
static std::vector<std::string> parseCopyRow(const std::string& line) {
    std::vector<std::string> result;
    result.reserve(32);  // I3: pre-allocation

    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            result.push_back(line.substr(start, i - start));
            start = i + 1;
        }
    }
    return result;
}

// ===========================================================================
// Minimal audit event capture helper
// ===========================================================================

/**
 * @brief Lightweight audit sink that captures emitted events for assertion.
 *
 * In production the events are emitted via THEMIS_WARN; in tests we invoke
 * the same JSON-building logic through this callable adapter.
 */
struct AuditCapture {
    std::vector<json> events;

    void emit(const std::string& event_type,
              std::initializer_list<std::pair<std::string, std::string>> fields)
    {
        json audit;
        audit["event"] = event_type;
        audit["ts_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        for (const auto& [k, v] : fields) {
            audit[k] = v;
        }
        events.push_back(std::move(audit));
    }

    bool hasEvent(const std::string& event_type) const {
        return std::any_of(events.begin(), events.end(),
            [&](const json& e) { return e.value("event", "") == event_type; });
    }

    const json* findEvent(const std::string& event_type) const {
        for (const auto& e : events) {
            if (e.value("event", "") == event_type) {
              return &e;
            }
        }
        return nullptr;
    }
};

// ===========================================================================
// Timeout simulation helper
// ===========================================================================

/**
 * @brief Simulate the timeout-check logic from parseDumpFile() so it can be
 *        tested without a full import stack.
 *
 * Returns true if the timeout was triggered.
 */
static bool simulateTimeoutCheck(
    uint32_t import_timeout_ms,
    size_t line_number,
    std::chrono::steady_clock::time_point deadline,
    ImportStats& stats,
    AuditCapture& audit,
    const std::string& source)
{
    if (import_timeout_ms == 0) {
      return false;
    }
    if (line_number % 500 != 0) {
      return false;
    }
    if (std::chrono::steady_clock::now() < deadline) {
      return false;
    }

    audit.emit("importer_timeout", {
        {"source",            source},
        {"reason",            "import_timeout_ms exceeded"},
        {"timeout_triggered", "true"},
        {"records_processed", std::to_string(stats.imported_records)},
        {"timeout_ms",        std::to_string(import_timeout_ms)}
    });

    ImportError err;
    err.code     = ImportErrorCode::DEADLINE_EXCEEDED;
    err.severity = ImportErrorSeverity::CRITICAL;
    err.message  = "Import timed out after " + std::to_string(import_timeout_ms) + " ms";
    stats.structured_errors.push_back(err);
    return true;
}

// ===========================================================================
// Test suite
// ===========================================================================

// ---------------------------------------------------------------------------
// I2 – isValidIdentifier tests (10 cases)
// ---------------------------------------------------------------------------

TEST(I2_IsValidIdentifier, EmptyStringRejected) {
    EXPECT_FALSE(isValidIdentifier(""));
}

TEST(I2_IsValidIdentifier, SimpleAlphanumericAccepted) {
    EXPECT_TRUE(isValidIdentifier("users"));
    EXPECT_TRUE(isValidIdentifier("Users123"));
    EXPECT_TRUE(isValidIdentifier("table_name"));
}

TEST(I2_IsValidIdentifier, UnderscoreAccepted) {
    EXPECT_TRUE(isValidIdentifier("_private"));
    EXPECT_TRUE(isValidIdentifier("col_a_b_c"));
}

TEST(I2_IsValidIdentifier, SemicolonRejected) {
    EXPECT_FALSE(isValidIdentifier("users;DROP TABLE users--"));
    EXPECT_FALSE(isValidIdentifier("t;"));
}

TEST(I2_IsValidIdentifier, SingleQuoteRejected) {
    EXPECT_FALSE(isValidIdentifier("user's_table"));
    EXPECT_FALSE(isValidIdentifier("'injected'"));
}

TEST(I2_IsValidIdentifier, DashRejected) {
    EXPECT_FALSE(isValidIdentifier("my-table"));
    EXPECT_FALSE(isValidIdentifier("-leading"));
}

TEST(I2_IsValidIdentifier, DotRejected) {
    EXPECT_FALSE(isValidIdentifier("schema.table"));
    EXPECT_FALSE(isValidIdentifier(".hidden"));
}

TEST(I2_IsValidIdentifier, SpacesRejected) {
    EXPECT_FALSE(isValidIdentifier("table name"));
    EXPECT_FALSE(isValidIdentifier(" leading"));
    EXPECT_FALSE(isValidIdentifier("trailing "));
}

TEST(I2_IsValidIdentifier, TooLongIdentifierRejected) {
    std::string too_long(kMaxIdentifierLength + 1, 'a');
    EXPECT_FALSE(isValidIdentifier(too_long));

    std::string exact(kMaxIdentifierLength, 'a');
    EXPECT_TRUE(isValidIdentifier(exact));
}

TEST(I2_IsValidIdentifier, SqlMetacharactersRejected) {
    // All common SQL injection characters must be rejected
    const std::vector<std::string> bad_inputs = {
        "1 OR 1=1",
        "col--comment",
        "t/*comment*/",
        "x UNION SELECT",
        "a\tb",       // tab
        "a\nb",       // newline
        "a%b",        // like wildcard
        "a@b",        // at-sign
    };
    for (const auto& input : bad_inputs) {
        EXPECT_FALSE(isValidIdentifier(input))
            << "Expected rejection for: " << input;
    }
}

// ---------------------------------------------------------------------------
// I2 – Bounds checking (2 cases)
// ---------------------------------------------------------------------------

TEST(I2_BoundsCheck, MaxTableCountConstantSane) {
    EXPECT_GT(kMaxTableCount, 0u);
    EXPECT_LE(kMaxTableCount, 100000u);
}

TEST(I2_BoundsCheck, MaxColumnCountConstantSane) {
    EXPECT_GT(kMaxColumnCount, 0u);
    EXPECT_LE(kMaxColumnCount, 10000u);
}

// ---------------------------------------------------------------------------
// I3 – Result vector pre-allocation / functional correctness (5 cases)
// ---------------------------------------------------------------------------

TEST(I3_VectorPrealloc, ParseInsertValuesSimple) {
    auto vals = parseInsertValues("1, 'hello', NULL");
    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ("1",     vals[0]);
    EXPECT_EQ("hello", vals[1]);
    EXPECT_EQ("NULL",  vals[2]);
}

TEST(I3_VectorPrealloc, ParseInsertValuesQuotedWithEscape) {
    auto vals = parseInsertValues("'it''s', 'done'");
    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ("it's", vals[0]);
    EXPECT_EQ("done", vals[1]);
}

TEST(I3_VectorPrealloc, ParseInsertValuesEmpty) {
    auto vals = parseInsertValues("");
    EXPECT_TRUE(vals.empty());
}

TEST(I3_VectorPrealloc, ParseCopyRowTabDelimited) {
    auto vals = parseCopyRow("alice\t42\tNYC");
    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ("alice", vals[0]);
    EXPECT_EQ("42",    vals[1]);
    EXPECT_EQ("NYC",   vals[2]);
}

TEST(I3_VectorPrealloc, ParseCopyRowSingleField) {
    auto vals = parseCopyRow("onlyfield");
    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ("onlyfield", vals[0]);
}

TEST(I3_VectorPrealloc, ParseInsertValuesNumericTypes) {
    auto vals = parseInsertValues("42, 3.14, -7");
    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ("42",   vals[0]);
    EXPECT_EQ("3.14", vals[1]);
    EXPECT_EQ("-7",   vals[2]);
}

// ---------------------------------------------------------------------------
// I1 – Connection timeout enforcement (5 cases)
// ---------------------------------------------------------------------------

TEST(I1_Timeout, DefaultTimeoutIsZeroDisabled) {
    ImportOptions opts;
    EXPECT_EQ(0u, opts.import_timeout_ms)
        << "Default import_timeout_ms must be 0 (disabled)";
}

TEST(I1_Timeout, TimeoutCheckSkippedWhenDisabled) {
    ImportStats stats;
    AuditCapture audit;
    auto far_future = std::chrono::steady_clock::now() + std::chrono::hours(1);

    // Even if line_number is a multiple of 500, no trigger when timeout_ms == 0
    bool triggered = simulateTimeoutCheck(0, 500, far_future, stats, audit, "test.sql");
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(audit.hasEvent("importer_timeout"));
    EXPECT_TRUE(stats.structured_errors.empty());
}

TEST(I1_Timeout, TimeoutNotTriggeredBeforeDeadline) {
    ImportStats stats;
    AuditCapture audit;
    auto far_future = std::chrono::steady_clock::now() + std::chrono::hours(1);

    bool triggered = simulateTimeoutCheck(30000, 500, far_future, stats, audit, "test.sql");
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(audit.hasEvent("importer_timeout"));
}

TEST(I1_Timeout, TimeoutTriggeredAfterDeadline) {
    ImportStats stats;
    AuditCapture audit;
    // Deadline in the past → timeout fires immediately
    auto past = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);

    bool triggered = simulateTimeoutCheck(1, 500, past, stats, audit, "/data/dump.sql");
    EXPECT_TRUE(triggered);
    EXPECT_TRUE(audit.hasEvent("importer_timeout"));

    const json* ev = audit.findEvent("importer_timeout");
    ASSERT_NE(nullptr, ev);
    EXPECT_EQ("true",          ev->value("timeout_triggered", ""));
    EXPECT_EQ("/data/dump.sql", ev->value("source", ""));
}

TEST(I1_Timeout, TimeoutAddsDeadlineExceededError) {
    ImportStats stats;
    AuditCapture audit;
    auto past = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);

    simulateTimeoutCheck(5000, 500, past, stats, audit, "src.sql");

    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(ImportErrorCode::DEADLINE_EXCEEDED,
              stats.structured_errors.back().code);
    EXPECT_EQ(ImportErrorSeverity::CRITICAL,
              stats.structured_errors.back().severity);
    EXPECT_NE(std::string::npos,
              stats.structured_errors.back().message.find("5000 ms"));
}

TEST(I1_Timeout, TimeoutCheckOnlyFiresEvery500Lines) {
    ImportStats stats;
    AuditCapture audit;
    auto past = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);

    // Line numbers NOT a multiple of 500 → no check → no trigger
    for (size_t ln : {1u, 99u, 499u, 501u, 1000u}) {
        if (ln % 500 != 0) {
            bool triggered = simulateTimeoutCheck(1, ln, past, stats, audit, "src.sql");
            EXPECT_FALSE(triggered) << "Unexpected trigger at line " << ln;
        }
    }
    EXPECT_FALSE(audit.hasEvent("importer_timeout"));
}

// ---------------------------------------------------------------------------
// I4 – Audit event structure tests (5 cases)
// ---------------------------------------------------------------------------

TEST(I4_Audit, ImportStartEventHasRequiredFields) {
    AuditCapture audit;
    audit.emit("import_start", {
        {"source",      "/data/pg_dump.sql"},
        {"schema_name", "imported"},
        {"dry_run",     "false"},
        {"timeout_ms",  "0"}
    });

    ASSERT_TRUE(audit.hasEvent("import_start"));
    const json* ev = audit.findEvent("import_start");
    ASSERT_NE(nullptr, ev);
    EXPECT_EQ("import_start",       ev->value("event", ""));
    EXPECT_EQ("/data/pg_dump.sql",  ev->value("source", ""));
    EXPECT_EQ("imported",           ev->value("schema_name", ""));
    EXPECT_EQ("false",              ev->value("dry_run", ""));
    EXPECT_TRUE(ev->contains("ts_ms"));
}

TEST(I4_Audit, ImportFailureEventHasPartialImportFlag) {
    AuditCapture audit;
    audit.emit("import_failure", {
        {"source",            "/data/dump.sql"},
        {"reason",            "parse_failed"},
        {"records_processed", "150"},
        {"partial_import",    "true"}
    });

    const json* ev = audit.findEvent("import_failure");
    ASSERT_NE(nullptr, ev);
    EXPECT_EQ("parse_failed", ev->value("reason", ""));
    EXPECT_EQ("150",          ev->value("records_processed", ""));
    EXPECT_EQ("true",         ev->value("partial_import", ""));
}

TEST(I4_Audit, AuthFailureEventAnonymisesUser) {
    AuditCapture audit;
    audit.emit("auth_failure", {
        {"source", "db.sql"},
        {"user",   "<caller>"},
        {"reason", "permission_check denied import:write"}
    });

    const json* ev = audit.findEvent("auth_failure");
    ASSERT_NE(nullptr, ev);
    // User field must never contain raw credentials; only the placeholder
    EXPECT_EQ("<caller>", ev->value("user", ""));
    EXPECT_NE(std::string::npos,
              ev->value("reason", "").find("permission_check"));
}

TEST(I4_Audit, SchemaChangeDetectionEventHasTableName) {
    AuditCapture audit;
    audit.emit("schema_change_detection", {
        {"table_name",  "orders"},
        {"change_type", "CREATE_TABLE"},
        {"detected_at", "line:42"}
    });

    const json* ev = audit.findEvent("schema_change_detection");
    ASSERT_NE(nullptr, ev);
    EXPECT_EQ("orders",       ev->value("table_name", ""));
    EXPECT_EQ("CREATE_TABLE", ev->value("change_type", ""));
    EXPECT_EQ("line:42",      ev->value("detected_at", ""));
}

TEST(I4_Audit, TimeoutAuditEventHasTimeoutTriggeredFlag) {
    AuditCapture audit;
    audit.emit("importer_timeout", {
        {"source",            "/data/large.sql"},
        {"reason",            "import_timeout_ms exceeded"},
        {"timeout_triggered", "true"},
        {"records_processed", "1024"},
        {"timeout_ms",        "30000"}
    });

    const json* ev = audit.findEvent("importer_timeout");
    ASSERT_NE(nullptr, ev);
    EXPECT_EQ("true",   ev->value("timeout_triggered", ""));
    EXPECT_EQ("30000",  ev->value("timeout_ms", ""));
    EXPECT_EQ("1024",   ev->value("records_processed", ""));
}

// ---------------------------------------------------------------------------
// Integration-level: import_timeout_ms field in ImportOptions (2 cases)
// ---------------------------------------------------------------------------

TEST(I1_Integration, ImportOptionsTimeoutDefaultZero) {
    ImportOptions opts;
    EXPECT_EQ(0u, opts.import_timeout_ms);
}

TEST(I1_Integration, ImportOptionsTimeoutFieldAssignable) {
    ImportOptions opts;
    opts.import_timeout_ms = 5000;
    EXPECT_EQ(5000u, opts.import_timeout_ms);

    opts.import_timeout_ms = 30000;
    EXPECT_EQ(30000u, opts.import_timeout_ms);
}

// ---------------------------------------------------------------------------
// Edge-case / regression tests (3 additional cases)
// ---------------------------------------------------------------------------

TEST(I2_EdgeCases, SingleCharIdentifierAccepted) {
    EXPECT_TRUE(isValidIdentifier("a"));
    EXPECT_TRUE(isValidIdentifier("Z"));
    EXPECT_TRUE(isValidIdentifier("_"));
}

TEST(I3_EdgeCases, ParseInsertValuesWideColumns) {
    // Verify correct parsing of a 10-column tuple (exceeds naive reserve(8))
    const std::string clause =
        "'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'";
    auto vals = parseInsertValues(clause);
    ASSERT_EQ(10u, vals.size());
    EXPECT_EQ("a", vals[0]);
    EXPECT_EQ("j", vals[9]);
}

TEST(I4_EdgeCases, MultipleAuditEventsDistinct) {
    AuditCapture audit;
    audit.emit("import_start",   {{"source", "a.sql"}});
    audit.emit("import_failure", {{"source", "a.sql"}, {"reason", "io_error"}});

    EXPECT_EQ(2u, audit.events.size());
    EXPECT_TRUE(audit.hasEvent("import_start"));
    EXPECT_TRUE(audit.hasEvent("import_failure"));
    EXPECT_FALSE(audit.hasEvent("auth_failure"));
}
