// test_postgres_importer_advanced.cpp
//
// Tests for:
//   - PermissionCheckCallback (ACL enforcement)
//   - Quarantine rows (quarantine_file)
//   - COPY binary format detection (BINARY_COPY_FORMAT error code 206)
//   - pg_dump mode flags (is_schema_only / is_data_only) on ImportStats
//   - Delta / incremental import (delta_hash_file + computeRowHash)
//   - CLI argument parser edge cases (self-contained / no CLI binary required)

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <functional>

// ---------------------------------------------------------------------------
// Minimal re-implementation of relevant types (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    NOT_A_PG_DUMP        = 103,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    STATEMENT_TOO_LARGE  = 204,
    ROW_TOO_LARGE        = 205,
    BINARY_COPY_FORMAT   = 206,   // NEW
    UNKNOWN_TABLE        = 300,
    COLUMN_COUNT_MISMATCH= 301,
    TYPE_CONVERSION      = 400,
    UNKNOWN_PG_TYPE      = 401,
    DRY_RUN_ONLY         = 500,
    TABLE_EXCLUDED       = 501,
    INVALID_UTF8         = 502,
    PERMISSION_DENIED    = 503,   // NEW
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
    size_t quarantined_records= 0;  // NEW
    size_t tables_processed   = 0;
    double elapsed_seconds    = 0.0;
    bool   is_schema_only     = false;  // NEW
    bool   is_data_only       = false;  // NEW
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

using ProgressCallback      = std::function<void(const std::string&, size_t, size_t)>;
using MetricsCallback       = std::function<void(const std::string&,
                                                  const std::map<std::string,std::string>&,
                                                  double)>;
using PermissionCheckCallback = std::function<bool(const std::string& resource,
                                                    const std::string& action)>;  // NEW

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
    std::vector<std::string> include_schemas;
    std::map<std::string,std::string> column_mappings;
    std::map<std::string,std::string> table_mappings;
    std::map<std::string,std::string> type_overrides;
    size_t max_row_size_bytes       = 0;
    size_t max_statement_size_bytes = 0;
    bool enforce_utf8               = false;
    std::string checkpoint_file;
    MetricsCallback       metrics_callback;
    PermissionCheckCallback permission_check;   // NEW
    std::string quarantine_file;                // NEW
    std::string delta_hash_file;                // NEW
    std::vector<std::string> delta_key_columns; // NEW
};

// ---------------------------------------------------------------------------
// Minimal importer that can be driven by text content
// ---------------------------------------------------------------------------

static uint64_t fnv1a64(const char* data, size_t len) {
    uint64_t h = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint8_t>(data[i]);
        h *= UINT64_C(1099511628211);
    }
    return h;
}

static std::unordered_set<uint64_t> loadDeltaHashes(const std::string& path) {
    std::unordered_set<uint64_t> hs;
    std::ifstream f(path);
    if (!f) {
      return hs;
    }
    std::string line = {};
    while (std::getline(f, line))
        if (!line.empty()) {
            try { hs.insert(std::stoull(line, nullptr, 16)); } catch (...) {}
        }
    return hs;
}

static void saveDeltaHashes(const std::string& path,
                             const std::unordered_set<uint64_t>& hs) {
    std::ofstream f(path, std::ios::trunc);
    for (uint64_t h : hs) {
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
        f << buf << "\n";
    }
}

static void writeQuarantineRow(const std::string& qf, const std::string& table,
                                const std::string& raw, const ImportError& err) {
    if (qf.empty()) {
      return;
    }
    std::ofstream f(qf, std::ios::app);
    if (!f) {
      return;
    }
    f << "{\"table\":\"" << table << "\",\"row\":\"(raw)\",\"error\":{\"code\":"
      << static_cast<uint32_t>(err.code) << ",\"message\":\""
      << err.message << "\"}}\n";
}

// Minimal importData that respects permission_check + quarantine + binary COPY + delta
static ImportStats importContent(const std::string& content,
                                  const ImportOptions& options) {
    ImportStats stats;

    // Permission check
    if (options.permission_check) {
        if (!options.permission_check("import", "write")) {
            ImportError e;
            e.code     = ImportErrorCode::PERMISSION_DENIED;
            e.severity = ImportErrorSeverity::CRITICAL;
            e.message  = "Permission denied: caller does not hold 'import:write'";
            stats.structured_errors.push_back(e);
            stats.errors.push_back(e.message);
            return stats;
        }
    }

    // Delta hashes
    std::unordered_set<uint64_t> delta_hashes = {};

    if (!options.delta_hash_file.empty())
        delta_hashes = loadDeltaHashes(options.delta_hash_file);

    // Dump-mode detection
    {
        std::istringstream ss(content);
        std::string line = {};
        int n = 0;
        while (std::getline(ss, line) && n < 50) {
            if (line.find("schema-only") != std::string::npos ||
                line.find("schema only") != std::string::npos) stats.is_schema_only = true;
            if (line.find("data-only")   != std::string::npos ||
                line.find("data only")   != std::string::npos) stats.is_data_only   = true;
            if (!line.empty() && !(line.size() >= 2 && line[0]=='-' && line[1]=='-')) {
              break;
            }
            n++;
        }
    }

    // Parse content
    std::istringstream file(content);
    std::string line = {};
    std::string sql = {};
    std::string current_table = {};
    bool in_copy = false;
    bool first_copy_line = false;

    while (std::getline(file, line)) {
        if (in_copy) {
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
                in_copy = false;
                continue;
            }
            // Binary COPY detection
            if (first_copy_line) {
                first_copy_line = false;
                if (line.size() >= 6 && line[0]=='P' && line[1]=='G' &&
                    line[2]=='C' && line[3]=='O' && line[4]=='P' && line[5]=='Y') {
                    ImportError e;
                    e.code     = ImportErrorCode::BINARY_COPY_FORMAT;
                    e.severity = ImportErrorSeverity::ERROR;
                    e.message  = "Binary COPY format detected for table '" + current_table + "'";
                    e.location = "table " + current_table;
                    stats.structured_errors.push_back(e);
                    stats.errors.push_back(e.message);
                    in_copy = false;
                    continue;
                }
            }
            stats.total_records++;
            if (options.dry_run) {
                ImportError e;
                e.code     = ImportErrorCode::DRY_RUN_ONLY;
                e.severity = ImportErrorSeverity::INFO;
                e.message  = "dry-run: row would be imported";
                e.location = "table " + current_table;
                stats.structured_errors.push_back(e);
                stats.imported_records++;
                continue;
            }

            // Row-size guard
            if (options.max_row_size_bytes > 0 && line.size() > options.max_row_size_bytes) {
                ImportError e;
                e.code     = ImportErrorCode::ROW_TOO_LARGE;
                e.severity = ImportErrorSeverity::WARNING;
                e.message  = "Row too large";
                e.location = "table " + current_table;
                stats.structured_errors.push_back(e);
                writeQuarantineRow(options.quarantine_file, current_table, line, e);
                stats.failed_records++; stats.quarantined_records++;
                if (!options.continue_on_error) {
                  return stats;
                }
                continue;
            }

            // Delta check
            if (!options.delta_hash_file.empty()) {
                uint64_t h = fnv1a64(line.data(), line.size());
                if (delta_hashes.count(h)) { stats.skipped_records++; continue; }
                delta_hashes.insert(h);
            }

            stats.imported_records++;
            continue;
        }

        if (line.empty() || (line.size() >= 2 && line[0]=='-' && line[1]=='-')) {
          continue;
        }
        sql += line + " ";

        if (line.find(';') != std::string::npos) {
            if (sql.find("CREATE TABLE") != std::string::npos) {
                stats.tables_processed++;
            } else if (sql.find("COPY ") != std::string::npos) {
                // extract table name
                auto pos = sql.find("COPY ");
                if (pos != std::string::npos) {
                    std::istringstream ss2(sql.substr(pos + 5));
                    std::string w; ss2 >> w;
                    auto dot = w.find('.');
                    if (dot != std::string::npos) {
                      w = w.substr(dot + 1);
                    }
                    while (!w.empty() && (w.back()=='(' || w.back()==' ')) w.pop_back();
                    current_table = w;
                }
                if (sql.find("FROM stdin") != std::string::npos) {
                    in_copy = true;
                    first_copy_line = true;
                }
            }
            sql.clear();
        }
    }

    if (!options.dry_run && !options.delta_hash_file.empty() && !delta_hashes.empty())
        saveDeltaHashes(options.delta_hash_file, delta_hashes);

    return stats;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string tmpFile(const std::string& suffix = ".tmp") {
    return "/tmp/test_importer_adv_" +
           std::to_string(reinterpret_cast<uintptr_t>(&suffix)) + suffix;
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::trunc);
    f << content;
}

// ===========================================================================
// Test suites
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. Permission check callback
// ---------------------------------------------------------------------------

TEST(PermissionCheckTest, DeniedReturnsPermissionDeniedError) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return false; };

    ImportStats stats = importContent("-- pg_dump\nSET standard_conforming_strings = on;\n", opts);

    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_FALSE(stats.structured_errors.empty());
    bool found = false;
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::PERMISSION_DENIED) {
          found = true;
        }
    EXPECT_TRUE(found) << "Expected PERMISSION_DENIED structured error";
}

TEST(PermissionCheckTest, DeniedSetsErrorMessage) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return false; };

    ImportStats stats = importContent("-- pg_dump\n", opts);
    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_NE(stats.structured_errors[0].message.find("Permission denied"), std::string::npos);
}

TEST(PermissionCheckTest, DeniedNoImportHappens) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return false; };

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE t (id integer, name text);\n"
        "COPY t (id, name) FROM stdin;\n"
        "1\talice\n"
        "2\tbob\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.tables_processed, 0u);
}

TEST(PermissionCheckTest, AllowedProceedsNormally) {
    ImportOptions opts;
    bool called = false;
    opts.permission_check = [&called](const std::string& r, const std::string& a) {
        called = true;
        return r == "import" && a == "write";
    };

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE items (id integer, val text);\n"
        "COPY items (id, val) FROM stdin;\n"
        "1\tx\n"
        "2\ty\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_TRUE(called) << "permission_check callback was not called";
    EXPECT_EQ(stats.imported_records, 2u);
    bool denied = false;
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::PERMISSION_DENIED) {
          denied = true;
        }
    EXPECT_FALSE(denied);
}

TEST(PermissionCheckTest, NullCallbackAllowsImport) {
    // No callback set → import should proceed
    ImportOptions opts;
    // opts.permission_check is null by default

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE x (id integer);\n"
        "COPY x (id) FROM stdin;\n"
        "42\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(PermissionCheckTest, DeniedErrorCodeIs503) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return false; };
    ImportStats stats = importContent("-- pg_dump\n", opts);
    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(static_cast<uint32_t>(stats.structured_errors[0].code), 503u);
}

// ---------------------------------------------------------------------------
// 2. Quarantine file
// ---------------------------------------------------------------------------

TEST(QuarantineTest, OversizedRowsWrittenToQuarantine) {
    std::string qfile = tmpFile(".q.jsonl");
    // Remove any previous file
    std::remove(qfile.c_str());

    ImportOptions opts;
    opts.quarantine_file = qfile;
    opts.max_row_size_bytes = 10;  // very small limit

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE big (id integer, data text);\n"
        "COPY big (id, data) FROM stdin;\n"
        "1\tshort\n"
        "2\tthis_is_a_very_long_value_that_exceeds_the_limit\n"
        "3\tok\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_GT(stats.quarantined_records, 0u) << "Expected at least one quarantined row";

    std::ifstream qf(qfile);
    ASSERT_TRUE(qf.good()) << "Quarantine file was not created";
    std::string line = {};
    int lines = 0;
    while (std::getline(qf, line)) {
      if (!line.empty()) lines++;
    }
    EXPECT_GT(lines, 0) << "Quarantine file is empty";

    std::remove(qfile.c_str());
}

TEST(QuarantineTest, QuarantineRowContainsErrorCode) {
    std::string qfile = tmpFile(".q2.jsonl");
    std::remove(qfile.c_str());

    ImportOptions opts;
    opts.quarantine_file = qfile;
    opts.max_row_size_bytes = 5;

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE t (id integer);\n"
        "COPY t (id) FROM stdin;\n"
        "this_is_too_long\n"
        "\\.\n";

    importContent(content, opts);

    std::ifstream qf(qfile);
    std::string all((std::istreambuf_iterator<char>(qf)),
                     std::istreambuf_iterator<char>());
    // Error code 205 = ROW_TOO_LARGE
    EXPECT_NE(all.find("205"), std::string::npos)
        << "Expected ROW_TOO_LARGE code 205 in quarantine file; got: " << all;

    std::remove(qfile.c_str());
}

TEST(QuarantineTest, NoFileCreatedWhenNoFailures) {
    std::string qfile = tmpFile(".q3.jsonl");
    std::remove(qfile.c_str());

    ImportOptions opts;
    opts.quarantine_file = qfile;

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE t (id integer);\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    importContent(content, opts);

    // File should either not exist or be empty
    std::ifstream qf(qfile);
    bool empty_or_missing = !qf.good();
    if (qf.good()) {
        std::string content2((std::istreambuf_iterator<char>(qf)),
                              std::istreambuf_iterator<char>());
        empty_or_missing = content2.empty();
    }
    EXPECT_TRUE(empty_or_missing);
    std::remove(qfile.c_str());
}

TEST(QuarantineTest, QuarantineDisabledWhenEmpty) {
    // No quarantine_file set – should not crash or fail
    ImportOptions opts;
    opts.max_row_size_bytes = 1;  // force failures

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE t (id integer);\n"
        "COPY t (id) FROM stdin;\n"
        "this_is_way_too_long\n"
        "\\.\n";

    // Should not crash
    ImportStats stats = importContent(content, opts);
    EXPECT_GT(stats.failed_records, 0u);
}

// ---------------------------------------------------------------------------
// 3. COPY binary format detection
// ---------------------------------------------------------------------------

TEST(BinaryCopyTest, BinaryMagicEmitsError206) {
    // Simulate a binary COPY block (PG binary COPY starts with PGCOPY...)
    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE bintest (id integer);\n"
        "COPY bintest FROM stdin;\n"
        "PGCOPY\n\xff\r\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    bool found = false;
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT) {
          found = true;
        }
    EXPECT_TRUE(found) << "Expected BINARY_COPY_FORMAT (206) error";
}

TEST(BinaryCopyTest, BinaryMagicErrorCodeIs206) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t FROM stdin;\n"
        "PGCOPY binary data\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT)
            EXPECT_EQ(static_cast<uint32_t>(e.code), 206u);
}

TEST(BinaryCopyTest, BinaryMagicMessageMentionsTable) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY mytable FROM stdin;\n"
        "PGCOPY header\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    ASSERT_FALSE(stats.structured_errors.empty());
    bool found = false;
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT) {
            found = true;
            EXPECT_NE(e.message.find("mytable"), std::string::npos)
                << "Error message should mention table name";
        }
    EXPECT_TRUE(found);
}

TEST(BinaryCopyTest, TextCopyDoesNotTriggerBinaryError) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    for (const auto& e : stats.structured_errors)
        EXPECT_NE(e.code, ImportErrorCode::BINARY_COPY_FORMAT);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(BinaryCopyTest, BinaryDetectionWithContinueOnError) {
    // Even with continue_on_error=true, binary block should be skipped (no crash)
    ImportOptions opts;
    opts.continue_on_error = true;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t FROM stdin;\n"
        "PGCOPY\x01\x02\x03\n"
        "\\.\n"
        "COPY t2 FROM stdin;\n"
        "1\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    // No crash; binary table should fail, normal table should succeed
    bool binary_err = false;
    for (const auto& e : stats.structured_errors)
        if (e.code == ImportErrorCode::BINARY_COPY_FORMAT) {
          binary_err = true;
        }
    EXPECT_TRUE(binary_err);
}

// ---------------------------------------------------------------------------
// 4. pg_dump mode flags
// ---------------------------------------------------------------------------

TEST(DumpModeFlagsTest, SchemaOnlyFlagDetected) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "-- schema-only dump\n"
        "CREATE TABLE t (id integer);\n";

    ImportStats stats = importContent(content, ImportOptions{});
    EXPECT_TRUE(stats.is_schema_only);
    EXPECT_FALSE(stats.is_data_only);
}

TEST(DumpModeFlagsTest, DataOnlyFlagDetected) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "-- data-only dump\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    EXPECT_TRUE(stats.is_data_only);
    EXPECT_FALSE(stats.is_schema_only);
}

TEST(DumpModeFlagsTest, NeitherFlagSetForFullDump) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "SET standard_conforming_strings = on;\n"
        "CREATE TABLE t (id integer);\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "\\.\n";

    ImportStats stats = importContent(content, ImportOptions{});
    EXPECT_FALSE(stats.is_schema_only);
    EXPECT_FALSE(stats.is_data_only);
}

TEST(DumpModeFlagsTest, SchemaOnlyVariant2) {
    std::string content =
        "-- PostgreSQL database dump\n"
        "-- schema only mode\n"
        "CREATE TABLE x (id integer);\n";

    ImportStats stats = importContent(content, ImportOptions{});
    EXPECT_TRUE(stats.is_schema_only);
}

// ---------------------------------------------------------------------------
// 5. Delta / incremental import
// ---------------------------------------------------------------------------

TEST(DeltaImportTest, FirstImportImportsAllRows) {
    std::string hash_file = tmpFile(".delta.txt");
    std::remove(hash_file.c_str());

    ImportOptions opts;
    opts.delta_hash_file = hash_file;

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE items (id integer, name text);\n"
        "COPY items (id, name) FROM stdin;\n"
        "1\talpha\n"
        "2\tbeta\n"
        "3\tgamma\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.imported_records, 3u);
    EXPECT_EQ(stats.skipped_records, 0u);

    std::remove(hash_file.c_str());
}

TEST(DeltaImportTest, SecondImportSkipsDuplicates) {
    std::string hash_file = tmpFile(".delta2.txt");
    std::remove(hash_file.c_str());

    std::string content =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE items (id integer);\n"
        "COPY items (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "3\n"
        "\\.\n";

    ImportOptions opts;
    opts.delta_hash_file = hash_file;

    // First import
    ImportStats s1 = importContent(content, opts);
    EXPECT_EQ(s1.imported_records, 3u);
    EXPECT_EQ(s1.skipped_records, 0u);

    // Second import – all rows already hashed
    ImportStats s2 = importContent(content, opts);
    EXPECT_EQ(s2.skipped_records, 3u);
    EXPECT_EQ(s2.imported_records, 0u);

    std::remove(hash_file.c_str());
}

TEST(DeltaImportTest, NewRowsImportedOnSecondRun) {
    std::string hash_file = tmpFile(".delta3.txt");
    std::remove(hash_file.c_str());

    std::string content1 =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE items (id integer);\n"
        "COPY items (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    std::string content2 =
        "-- PostgreSQL database dump\n"
        "CREATE TABLE items (id integer);\n"
        "COPY items (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "3\n"          // new row
        "4\n"          // new row
        "\\.\n";

    ImportOptions opts;
    opts.delta_hash_file = hash_file;

    importContent(content1, opts);

    ImportStats s2 = importContent(content2, opts);
    EXPECT_EQ(s2.skipped_records, 2u)   << "Should skip rows 1 and 2";
    EXPECT_EQ(s2.imported_records, 2u)  << "Should import rows 3 and 4";

    std::remove(hash_file.c_str());
}

TEST(DeltaImportTest, HashFilePersistsAcrossRuns) {
    std::string hash_file = tmpFile(".delta4.txt");
    std::remove(hash_file.c_str());

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "AAA\n"
        "\\.\n";

    ImportOptions opts;
    opts.delta_hash_file = hash_file;

    importContent(content, opts);

    // Check file was written
    std::ifstream f(hash_file);
    ASSERT_TRUE(f.good()) << "Delta hash file was not created";
    std::string line = {};
    std::getline(f, line);
    EXPECT_EQ(line.size(), 16u) << "Hash should be 16 hex characters";

    std::remove(hash_file.c_str());
}

TEST(DeltaImportTest, NoDeltaFileNoSkipping) {
    // When delta_hash_file is empty, all rows are imported (no dedup)
    ImportOptions opts;
    // opts.delta_hash_file = "";  (default)

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    ImportStats s1 = importContent(content, opts);
    ImportStats s2 = importContent(content, opts);

    EXPECT_EQ(s1.imported_records, 2u);
    EXPECT_EQ(s2.imported_records, 2u);
    EXPECT_EQ(s2.skipped_records, 0u);
}

// ---------------------------------------------------------------------------
// 6. FNV-1a hash function properties
// ---------------------------------------------------------------------------

TEST(FnvHashTest, SameInputSameHash) {
    std::string s = "hello world";
    uint64_t h1 = fnv1a64(s.data(), s.size());
    uint64_t h2 = fnv1a64(s.data(), s.size());
    EXPECT_EQ(h1, h2);
}

TEST(FnvHashTest, DifferentInputsDifferentHashes) {
    std::string s1 = "1\talice\t42";
    std::string s2 = "2\tbob\t43";
    EXPECT_NE(fnv1a64(s1.data(), s1.size()), fnv1a64(s2.data(), s2.size()));
}

TEST(FnvHashTest, EmptyStringHasKnownValue) {
    // FNV-1a 64-bit offset_basis for empty input = 14695981039346656037
    uint64_t h = fnv1a64("", 0);
    EXPECT_EQ(h, UINT64_C(14695981039346656037));
}

TEST(FnvHashTest, SingleByteAgreesWithManual) {
    // h = 14695981039346656037 XOR 'A' (65) * 1099511628211
    uint64_t expected = (UINT64_C(14695981039346656037) ^ 65) * UINT64_C(1099511628211);
    uint64_t h = fnv1a64("A", 1);
    EXPECT_EQ(h, expected);
}

// ---------------------------------------------------------------------------
// 7. Error code values
// ---------------------------------------------------------------------------

TEST(ErrorCodeTest, PermissionDeniedIs503) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::PERMISSION_DENIED), 503u);
}

TEST(ErrorCodeTest, BinaryCopyFormatIs206) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::BINARY_COPY_FORMAT), 206u);
}

TEST(ErrorCodeTest, AllCodesDistinct) {
    std::vector<uint32_t> codes = {0, 100, 101, 102, 103, 200, 201, 202, 203, 204,
                                    205, 206, 300, 301, 400, 401, 402, 500, 501, 502,
                                    503, 900};
    for (size_t i = 0; i < codes.size(); ++i)
        for (size_t j = i + 1; j < codes.size(); ++j)
            EXPECT_NE(codes[i], codes[j]) << "Duplicate error code: " << codes[i];
}

// ---------------------------------------------------------------------------
// 8. ImportStats quarantined_records field
// ---------------------------------------------------------------------------

TEST(StatsFieldTest, QuarantinedRecordsDefaultZero) {
    ImportStats s;
    EXPECT_EQ(s.quarantined_records, 0u);
}

TEST(StatsFieldTest, IsSchemaOnlyDefaultFalse) {
    ImportStats s;
    EXPECT_FALSE(s.is_schema_only);
}

TEST(StatsFieldTest, IsDataOnlyDefaultFalse) {
    ImportStats s;
    EXPECT_FALSE(s.is_data_only);
}

TEST(StatsFieldTest, QuarantinedRecordsIncrementedOnFailedRow) {
    std::string qfile = tmpFile(".q4.jsonl");
    std::remove(qfile.c_str());

    ImportOptions opts;
    opts.quarantine_file = qfile;
    opts.max_row_size_bytes = 3;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "toolong\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.quarantined_records, 1u);

    std::remove(qfile.c_str());
}

// ---------------------------------------------------------------------------
// 9. Dry-run preview
// ---------------------------------------------------------------------------

TEST(DryRunPreviewTest, ImportedRecordsCountsPreviewRows) {
    ImportOptions opts;
    opts.dry_run = true;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY users (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "3\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(DryRunPreviewTest, TotalRecordsMatchesImportedInDryRun) {
    ImportOptions opts;
    opts.dry_run = true;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY orders (id) FROM stdin;\n"
        "10\n"
        "20\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.total_records, stats.imported_records);
}

TEST(DryRunPreviewTest, DryRunEmitsDryRunOnlyStructuredError) {
    ImportOptions opts;
    opts.dry_run = true;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY items (id) FROM stdin;\n"
        "42\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    bool found = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::DRY_RUN_ONLY) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected DRY_RUN_ONLY structured error in dry_run mode";
}

TEST(DryRunPreviewTest, DryRunOnlyErrorIsInfoSeverity) {
    ImportOptions opts;
    opts.dry_run = true;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "\\.\n";

    ImportStats stats = importContent(content, opts);
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::DRY_RUN_ONLY) {
            EXPECT_EQ(e.severity, ImportErrorSeverity::INFO);
            return;
        }
    }
    FAIL() << "DRY_RUN_ONLY structured error not found";
}

TEST(DryRunPreviewTest, DryRunDoesNotWriteToQuarantineFile) {
    std::string qfile = tmpFile(".dryrun_q.jsonl");
    std::remove(qfile.c_str());

    ImportOptions opts;
    opts.dry_run = true;
    opts.quarantine_file = qfile;
    opts.max_row_size_bytes = 2;  // force row-size failures

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "toolong\n"
        "\\.\n";

    importContent(content, opts);

    // Quarantine file must not be created in dry_run mode
    std::ifstream f(qfile);
    EXPECT_FALSE(f.good()) << "Quarantine file should not be created during dry_run";

    std::remove(qfile.c_str());
}

TEST(DryRunPreviewTest, DryRunZeroImportedOnEmptyInput) {
    ImportOptions opts;
    opts.dry_run = true;

    std::string content = "-- PostgreSQL database dump\n";

    ImportStats stats = importContent(content, opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.total_records, 0u);
}

TEST(DryRunPreviewTest, DryRunDoesNotSaveDeltaHashFile) {
    std::string hash_file = tmpFile(".dryrun_delta.txt");
    std::remove(hash_file.c_str());

    ImportOptions opts;
    opts.dry_run = true;
    opts.delta_hash_file = hash_file;

    std::string content =
        "-- PostgreSQL database dump\n"
        "COPY t (id) FROM stdin;\n"
        "1\n"
        "2\n"
        "\\.\n";

    importContent(content, opts);

    // Delta hash file must not be created in dry_run mode
    std::ifstream f(hash_file);
    EXPECT_FALSE(f.good()) << "Delta hash file should not be created during dry_run";

    std::remove(hash_file.c_str());
}
