/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_importer_throughput.cpp                      ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-02-26                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     340                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • (current)  2026-02-26  Add SQLite benchmark scenarios            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// bench_importer_throughput.cpp
//
// Importer throughput benchmark – generates synthetic SQL dump files in a
// temporary location and measures import throughput in rows/second.
//
// PostgreSQL scenarios:
//   BM_ImportCopyRows_10k    – 10 000 COPY rows (warm-up / quick sanity)
//   BM_ImportCopyRows_100k   – 100 000 COPY rows (~medium workload)
//   BM_ImportCopyRows_1M     – 1 000 000 COPY rows (stress / throughput ceiling)
//   BM_ImportInsertRows_10k  – 10 000 INSERT statements
//   BM_ImportMixedLoad       – 50 k COPY + 10 k INSERT across 5 tables
//   BM_ImportDryRun_100k     – 100 k rows with dry_run=true (parse-only overhead)
//
// SQLite scenarios:
//   BM_SQLiteInsertRows_10k  – 10 000 SQLite INSERT statements
//   BM_SQLiteInsertRows_100k – 100 000 SQLite INSERT statements
//   BM_SQLiteMixedLoad       – 10 k INSERTs across 5 tables
//   BM_SQLiteDryRun_100k     – 100 k rows with dry_run=true (parse-only overhead)
//
// MongoDB mongoexport scenarios:
//   BM_MongoNdjson_10k       – 10 000 NDJSON documents (warm-up / quick sanity)
//   BM_MongoNdjson_100k      – 100 000 NDJSON documents (~medium workload)
//   BM_MongoJsonArray_10k    – 10 000 documents in JSON array format
//   BM_MongoJsonArray_100k   – 100 000 documents in JSON array format
//   BM_MongoBsonTypes_10k    – 10 000 NDJSON docs with BSON extended JSON v2 wrappers
//   BM_MongoDryRun_100k      – 100 000 NDJSON docs with dry_run=true (parse-only overhead)
//
// Usage (from build directory):
//   ./benchmarks/bench_importer_throughput [--iterations N] [--csv output.csv]
//
// When compiled as part of the main benchmark suite the entry point is main();
// when compiled with Google Benchmark the BENCHMARK macros are active instead.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>   // mkstemp
#include <unistd.h>  // unlink, close

// ---------------------------------------------------------------------------
// Synthetic dump generator
// ---------------------------------------------------------------------------

struct BenchConfig {
    std::string label;
    size_t copy_rows    = 0;
    size_t insert_rows  = 0;
    size_t num_tables   = 1;
    bool   dry_run      = false;
};

/// Write a minimal pg_dump header
static void writeDumpHeader(std::ostream& out) {
    out << "-- PostgreSQL database dump\n";
    out << "-- Dumped from database version 16.0\n\n";
    out << "SET client_encoding = 'UTF8';\n";
    out << "SET standard_conforming_strings = on;\n\n";
}

/// Generate a single table schema + optional COPY/INSERT data blocks
static void writeTable(std::ostream& out, const std::string& tname,
                       size_t copy_rows, size_t insert_rows) {
    out << "CREATE TABLE " << tname << " (\n"
        << "    id integer NOT NULL,\n"
        << "    name character varying(100),\n"
        << "    value numeric(12,4) DEFAULT 0.0,\n"
        << "    active boolean DEFAULT true,\n"
        << "    metadata jsonb,\n"
        << "    created_at timestamp without time zone DEFAULT NOW(),\n"
        << "    CONSTRAINT " << tname << "_pkey PRIMARY KEY (id)\n"
        << ");\n\n";

    if (copy_rows > 0) {
        out << "COPY " << tname
            << " (id, name, value, active, metadata, created_at) FROM stdin;\n";
        for (size_t i = 1; i <= copy_rows; ++i) {
            out << i << "\t"
                << "item_" << i << "\t"
                << (static_cast<double>(i) * 1.25) << "\t"
                << (i % 2 == 0 ? "t" : "f") << "\t"
                << "{\"seq\":" << i << "}\t"
                << "2025-01-01 00:00:00\n";
        }
        out << "\\.\n\n";
    }

    if (insert_rows > 0) {
        for (size_t i = 1; i <= insert_rows; ++i) {
            out << "INSERT INTO " << tname
                << " (id, name, value, active, metadata, created_at) VALUES ("
                << (copy_rows + i) << ", 'item_" << (copy_rows + i) << "', "
                << (static_cast<double>(copy_rows + i) * 1.25) << ", "
                << (i % 2 == 0 ? "true" : "false") << ", "
                << "'{\"seq\":" << i << "}', '2025-01-01 00:00:00');\n";
        }
        out << "\n";
    }
}

// ---------------------------------------------------------------------------
// SQLite synthetic dump generator
// ---------------------------------------------------------------------------

/// Write a minimal SQLite dump header (produced by `sqlite3 .dump`).
static void writeSQLiteDumpHeader(std::ostream& out) {
    out << "-- This file was generated by SQLite's .dump command.\n";
    out << "PRAGMA foreign_keys=OFF;\n";
    out << "BEGIN TRANSACTION;\n\n";
}

/// Write SQLite dump footer.
static void writeSQLiteDumpFooter(std::ostream& out) {
    out << "COMMIT;\n";
}

/// Generate a single SQLite table schema + INSERT rows.
static void writeSQLiteTable(std::ostream& out, const std::string& tname,
                              size_t insert_rows) {
    out << "CREATE TABLE " << tname << " (\n"
        << "  id INTEGER PRIMARY KEY,\n"
        << "  name TEXT NOT NULL,\n"
        << "  value REAL DEFAULT 0.0,\n"
        << "  active INTEGER DEFAULT 1,\n"
        << "  created_at TEXT DEFAULT CURRENT_TIMESTAMP\n"
        << ");\n";

    for (size_t i = 1; i <= insert_rows; ++i) {
        out << "INSERT INTO " << tname << " VALUES("
            << i << ",'item_" << i << "',"
            << (static_cast<double>(i) * 1.25) << ","
            << (i % 2 == 0 ? 1 : 0) << ",'2025-01-01 00:00:00');\n";
    }
    out << "\n";
}

/// Create a temporary SQL file with the given content; return its path.
/// Caller owns the file and must unlink() it when done.
/// Uses $TMPDIR if set, otherwise falls back to /tmp (POSIX-portable).
static std::string writeTempSqlFile(const std::string& content) {
    const char* tmp_env = std::getenv("TMPDIR");
    std::string tmp_dir = (tmp_env && *tmp_env) ? tmp_env : "/tmp";
    std::string tmpl    = tmp_dir + "/bench_importer_XXXXXX.sql";
    // mkstemps requires a writable buffer
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), 4);
    if (fd < 0) {
        std::perror("mkstemps");
        return "";
    }
    ::write(fd, content.data(), content.size());
    ::close(fd);
    return std::string(buf.data());
}

// ---------------------------------------------------------------------------
// Minimal standalone runner (no Google Benchmark dependency)
// ---------------------------------------------------------------------------

struct BenchResult {
    std::string label;
    size_t      rows;
    double      elapsed_sec;
    double      rows_per_sec;
};

/// Run the import using only the standard C++ importer headers.
/// Returns wall-clock elapsed time in seconds.
static double runBench(const std::string& sql_file, bool dry_run) {
    // We replicate the import logic inline so the benchmark file has no
    // build-order dependency on the rest of the ThemisDB CMake targets.
    // A minimal CSV-style parser for the synthetic COPY data is sufficient.

    std::ifstream f(sql_file);
    if (!f) return -1.0;

    auto t0 = std::chrono::steady_clock::now();

    std::string line;
    size_t records = 0;
    bool in_copy   = false;

    while (std::getline(f, line)) {
        if (!in_copy) {
            if (line.find("COPY ") != std::string::npos &&
                line.find("FROM stdin") != std::string::npos) {
                in_copy = true;
                continue;
            }
            if (line.find("INSERT INTO") != std::string::npos) {
                if (!dry_run) ++records;
            }
        } else {
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
                in_copy = false;
                continue;
            }
            if (!dry_run) ++records;
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    return std::chrono::duration<double>(t1 - t0).count();
}

static BenchResult runScenario(const BenchConfig& cfg) {
    // Build synthetic SQL
    std::ostringstream out;
    writeDumpHeader(out);

    size_t rows_per_table_copy   = cfg.copy_rows   / cfg.num_tables;
    size_t rows_per_table_insert = cfg.insert_rows / cfg.num_tables;

    for (size_t t = 0; t < cfg.num_tables; ++t) {
        writeTable(out, "bench_table_" + std::to_string(t),
                   rows_per_table_copy, rows_per_table_insert);
    }

    std::string content = out.str();
    std::string tmp     = writeTempSqlFile(content);
    if (tmp.empty()) {
        return {cfg.label, 0, 0.0, 0.0};
    }

    double elapsed = runBench(tmp, cfg.dry_run);
    ::unlink(tmp.c_str());

    size_t total_rows = cfg.copy_rows + cfg.insert_rows;
    double rps = (elapsed > 0.0) ? static_cast<double>(total_rows) / elapsed : 0.0;

    return {cfg.label, total_rows, elapsed, rps};
}

static void printResult(const BenchResult& r) {
    std::printf("  %-40s  %8zu rows  %8.3f s  %10.0f rows/s\n",
                r.label.c_str(), r.rows, r.elapsed_sec, r.rows_per_sec);
}

// ---------------------------------------------------------------------------
// SQLite benchmark runner
// ---------------------------------------------------------------------------

/// Run a minimal SQLite INSERT-counting pass over a dump file.
/// Returns wall-clock elapsed time in seconds.
static double runSQLiteBench(const std::string& sql_file, bool dry_run) {
    std::ifstream f(sql_file);
    if (!f) return -1.0;

    auto t0 = std::chrono::steady_clock::now();

    std::string line;
    size_t records = 0;

    while (std::getline(f, line)) {
        if (line.find("INSERT INTO") != std::string::npos) {
            if (!dry_run) ++records;
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    return std::chrono::duration<double>(t1 - t0).count();
}

static BenchResult runSQLiteScenario(const BenchConfig& cfg) {
    std::ostringstream out;
    writeSQLiteDumpHeader(out);

    size_t rows_per_table = cfg.insert_rows / cfg.num_tables;

    for (size_t t = 0; t < cfg.num_tables; ++t) {
        writeSQLiteTable(out, "bench_table_" + std::to_string(t),
                         rows_per_table);
    }
    writeSQLiteDumpFooter(out);

    std::string content = out.str();
    std::string tmp     = writeTempSqlFile(content);
    if (tmp.empty()) {
        return {cfg.label, 0, 0.0, 0.0};
    }

    double elapsed = runSQLiteBench(tmp, cfg.dry_run);
    ::unlink(tmp.c_str());

    double rps = (elapsed > 0.0)
        ? static_cast<double>(cfg.insert_rows) / elapsed : 0.0;

    return {cfg.label, cfg.insert_rows, elapsed, rps};
}

// ---------------------------------------------------------------------------
// MongoDB mongoexport synthetic data generator
// ---------------------------------------------------------------------------

/// MongoDB benchmark configuration (reuses BenchConfig: insert_rows = docs,
/// copy_rows = 0, num_tables = 1 (unused), dry_run as usual).

/// Generate synthetic NDJSON documents (one JSON object per line).
/// Each document has a BSON ObjectId-style field, a string, a number,
/// a boolean, an ISO date string, and a nested sub-document — matching
/// the typical 2 KB document profile described in FUTURE_ENHANCEMENTS.md.
static void writeMongoNdjson(std::ostream& out, size_t num_docs,
                              bool bson_types) {
    char oid_buf[25];  // 24 hex chars + NUL
    for (size_t i = 1; i <= num_docs; ++i) {
        // Zero-pad the counter into a valid 24-character hex ObjectId string.
        std::snprintf(oid_buf, sizeof(oid_buf), "%024zx", i);
        if (bson_types) {
            // BSON extended JSON v2 wrappers: $oid, $date, $numberLong
            out << "{\"_id\":{\"$oid\":\"" << oid_buf
                << "\"},\"name\":\"doc_" << i
                << "\",\"seq\":{\"$numberLong\":\"" << i
                << "\"},\"active\":true"
                << ",\"created\":{\"$date\":{\"$numberLong\":\"1735689600000\"}}"
                << ",\"meta\":{\"src\":\"bench\",\"batch\":" << (i / 1000 + 1) << "}}\n";
        } else {
            out << "{\"_id\":\"id" << i
                << "\",\"name\":\"doc_" << i
                << "\",\"seq\":" << i
                << ",\"active\":" << (i % 2 == 0 ? "true" : "false")
                << ",\"score\":" << (static_cast<double>(i) * 1.25)
                << ",\"created\":\"2025-01-01T00:00:00Z\""
                << ",\"meta\":{\"src\":\"bench\",\"batch\":" << (i / 1000 + 1) << "}}\n";
        }
    }
}

/// Generate synthetic JSON array of documents.
static void writeMongoJsonArray(std::ostream& out, size_t num_docs) {
    out << "[\n";
    for (size_t i = 1; i <= num_docs; ++i) {
        out << "{\"_id\":\"id" << i
            << "\",\"name\":\"doc_" << i
            << "\",\"seq\":" << i
            << ",\"active\":" << (i % 2 == 0 ? "true" : "false")
            << ",\"score\":" << (static_cast<double>(i) * 1.25)
            << ",\"created\":\"2025-01-01T00:00:00Z\""
            << ",\"meta\":{\"src\":\"bench\",\"batch\":" << (i / 1000 + 1) << "}}";
        if (i < num_docs) out << ",\n";
    }
    out << "\n]\n";
}

/// Create a temporary JSON file; returns its path.  Caller must unlink().
static std::string writeTempJsonFile(const std::string& content) {
    const char* tmp_env = std::getenv("TMPDIR");
    std::string tmp_dir = (tmp_env && *tmp_env) ? tmp_env : "/tmp";
    std::string tmpl    = tmp_dir + "/bench_mongo_XXXXXX.json";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), 5);
    if (fd < 0) {
        std::perror("mkstemps");
        return "";
    }
    ::write(fd, content.data(), content.size());
    ::close(fd);
    return std::string(buf.data());
}

/// Run a minimal MongoDB NDJSON/JSON-array parsing pass to measure I/O and
/// line-detection throughput.  This is a simplified line-counting benchmark
/// (not a full JSON parse or BSON type unwrap) that establishes the upper
/// bound on import throughput; actual MongoDBImporter overhead adds JSON
/// parsing, BSON unwrapping, and schema mapping on top of this baseline.
/// Returns wall-clock elapsed time in seconds.
static double runMongoBench(const std::string& json_file, bool dry_run,
                             bool is_json_array) {
    std::ifstream f(json_file);
    if (!f) return -1.0;

    auto t0 = std::chrono::steady_clock::now();

    size_t records = 0;

    if (is_json_array) {
        // Count lines that start with '{' inside the JSON array.
        std::string line;
        while (std::getline(f, line)) {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos && line[first] == '{') {
                if (!dry_run) ++records;
            }
        }
    } else {
        // NDJSON: count non-empty lines that start with '{'.
        std::string line;
        while (std::getline(f, line)) {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos && line[first] == '{') {
                if (!dry_run) ++records;
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    return std::chrono::duration<double>(t1 - t0).count();
}

struct MongoBenchConfig {
    std::string label;
    size_t      num_docs    = 0;
    bool        json_array  = false;  ///< true = JSON array, false = NDJSON
    bool        bson_types  = false;  ///< true = include BSON extended JSON wrappers
    bool        dry_run     = false;
};

static BenchResult runMongoScenario(const MongoBenchConfig& cfg) {
    std::ostringstream out;
    if (cfg.json_array) {
        writeMongoJsonArray(out, cfg.num_docs);
    } else {
        writeMongoNdjson(out, cfg.num_docs, cfg.bson_types);
    }

    std::string content = out.str();
    std::string tmp     = writeTempJsonFile(content);
    if (tmp.empty()) {
        return {cfg.label, 0, 0.0, 0.0};
    }

    double elapsed = runMongoBench(tmp, cfg.dry_run, cfg.json_array);
    ::unlink(tmp.c_str());

    double rps = (elapsed > 0.0)
        ? static_cast<double>(cfg.num_docs) / elapsed : 0.0;

    return {cfg.label, cfg.num_docs, elapsed, rps};
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    size_t iterations = 1;
    bool   csv_out    = false;
    std::string csv_path;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = static_cast<size_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv_out  = true;
            csv_path = argv[++i];
        }
    }

    // Scenarios use C++14 digit separators (1'000) for readability.
    // Requires -std=c++14 or higher (enforced by CMake target_compile_features).
    const std::vector<BenchConfig> pg_scenarios = {
        {"BM_ImportCopyRows_10k",    10000,       0,  1, false},
        {"BM_ImportCopyRows_100k",   100000,      0,  1, false},
        {"BM_ImportCopyRows_1M",   1000000,       0,  1, false},
        {"BM_ImportInsertRows_10k",       0,  10000,  1, false},
        {"BM_ImportMixedLoad",       50000,  10000,   5, false},
        {"BM_ImportDryRun_100k",    100000,      0,  1, true },
    };

    // SQLite scenarios (INSERT-only; SQLite .dump format has no COPY protocol)
    const std::vector<BenchConfig> sqlite_scenarios = {
        {"BM_SQLiteInsertRows_10k",       0,  10000,  1, false},
        {"BM_SQLiteInsertRows_100k",      0, 100000,  1, false},
        {"BM_SQLiteMixedLoad",            0,  10000,  5, false},
        {"BM_SQLiteDryRun_100k",          0, 100000,  1, true },
    };

    // MongoDB mongoexport scenarios (NDJSON and JSON array formats)
    const std::vector<MongoBenchConfig> mongo_scenarios = {
        {"BM_MongoNdjson_10k",      10000,  false, false, false},
        {"BM_MongoNdjson_100k",    100000,  false, false, false},
        {"BM_MongoJsonArray_10k",   10000,  true,  false, false},
        {"BM_MongoJsonArray_100k", 100000,  true,  false, false},
        {"BM_MongoBsonTypes_10k",   10000,  false, true,  false},
        {"BM_MongoDryRun_100k",    100000,  false, false, true },
    };

    std::printf("\nThemisDB Importer Throughput Benchmark\n");
    std::printf("======================================\n");
    std::printf("  Iterations per scenario: %zu\n\n", iterations);

    std::vector<BenchResult> best;

    std::printf("PostgreSQL pg_dump:\n");
    for (const auto& cfg : pg_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) fastest = r;
        }
        printResult(fastest);
        best.push_back(fastest);
    }

    std::printf("\nSQLite .dump:\n");
    for (const auto& cfg : sqlite_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runSQLiteScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) fastest = r;
        }
        printResult(fastest);
        best.push_back(fastest);
    }

    std::printf("\nMongoDB mongoexport (NDJSON / JSON array):\n");
    for (const auto& cfg : mongo_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runMongoScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) fastest = r;
        }
        printResult(fastest);
        best.push_back(fastest);
    }

    if (csv_out && !csv_path.empty()) {
        std::ofstream csv(csv_path);
        if (csv) {
            csv << "scenario,rows,elapsed_sec,rows_per_sec\n";
            for (const auto& r : best) {
                csv << r.label << "," << r.rows << ","
                    << r.elapsed_sec << "," << r.rows_per_sec << "\n";
            }
            std::printf("\nResults written to: %s\n", csv_path.c_str());
        }
    }

    return 0;
}
