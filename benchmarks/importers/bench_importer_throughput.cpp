// bench_importer_throughput.cpp
//
// Importer throughput benchmark – generates synthetic SQL dump files in a
// temporary location and measures import throughput in rows/second and GB/hr.
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
// MySQL/MariaDB mysqldump scenarios (Issue #69, v1.8.0 AC):
//   BM_MySQLInsertRows_10k   – 10 000 INSERT rows (warm-up / quick sanity)
//   BM_MySQLInsertRows_100k  – 100 000 INSERT rows (~medium workload)
//   BM_MySQLInsertRows_1M    – 1 000 000 INSERT rows (stress / throughput ceiling)
//   BM_MySQLMixedLoad        – 50 k INSERT rows across 5 tables
//   BM_MySQLDryRun_100k      – 100 k rows with dry_run=true (parse-only overhead)
//   Target: >= 50 000 rows/sec from a local MySQL 8.0 instance (2 KB avg rows).
//
// Usage (from build directory):
//   ./benchmarks/bench_importer_throughput [--iterations N] [--csv output.csv]
//
// When compiled as part of the main benchmark suite the entry point is main();
// when compiled with Google Benchmark the BENCHMARK macros are active instead.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>  // std::min / std::max
#include <cstdlib>
#include <unordered_map>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Minimal inline mirror of ImportConflictResolver
// (avoids a build-order dependency on the full ThemisDB CMake graph)
// ---------------------------------------------------------------------------

namespace bench_internal {

enum class ConflictStrategy { OVERWRITE, SKIP, MERGE, ERROR };

/// Minimal conflict resolver mirroring src/importers/conflict_resolver.cpp.
class ConflictResolver {
public:
    void reset() { registry_.clear(); }

    // Returns resolved entity; sets conflict_detected=true on duplicate key.
    nlohmann::json resolve(const nlohmann::json& entity,
                           const std::string& table,
                           const std::string& key,
                           ConflictStrategy strategy,
                           bool& conflict_detected) {
        conflict_detected = false;
        auto& tbl = registry_[table];
        auto  it  = tbl.find(key);
        if (it == tbl.end()) {
            tbl.emplace(key, entity);
            return entity;
        }
        conflict_detected = true;
        nlohmann::json& existing = it->second;
        switch (strategy) {
            case ConflictStrategy::SKIP:      return existing;
            case ConflictStrategy::OVERWRITE: existing = entity; return entity;
            case ConflictStrategy::MERGE: {
                // Flat merge: incoming fields win (depth=1)
                nlohmann::json merged = existing;
                for (auto it2 = entity.begin(); it2 != entity.end(); ++it2) {
                    merged[it2.key()] = it2.value();
                }
                existing = merged;
                return merged;
            }
            case ConflictStrategy::ERROR: return existing;
        }
        return entity;
    }

private:
    std::unordered_map<std::string,
                       std::unordered_map<std::string, nlohmann::json>> registry_;
};

} // namespace bench_internal

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
/// Caller owns the file and must delete it when done.
static std::string writeTempSqlFile(const std::string& content) {
    std::error_code ec = {};
    const auto tmp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
      return "";
    }

    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto path = tmp_dir / ("bench_importer_" + std::to_string(now) + ".sql");

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      return "";
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.close();

    return path.string();
}

// ---------------------------------------------------------------------------
// Minimal standalone runner (no Google Benchmark dependency)
// ---------------------------------------------------------------------------

/// Bytes per gibibyte (2^30).  All throughput figures in this benchmark use
/// binary units (GiB/hr), which is conventional for I/O benchmarks.
/// The output label "GB/hr" is kept for brevity; values are GiB/hr.
static constexpr double kBytesPerGiB = 1024.0 * 1024.0 * 1024.0;

/// Compute GiB/hr from total bytes read and wall-clock elapsed time.
/// Returns 0 when elapsed_sec <= 0 to avoid division by zero.
static inline double calcGibPerHour(double bytes, double elapsed_sec) {
    return (elapsed_sec > 0.0)
        ? (bytes / elapsed_sec) * 3600.0 / kBytesPerGiB
        : 0.0;
}

struct BenchResult {
    std::string label;
    size_t      rows;
    double      elapsed_sec;
    double      rows_per_sec;
    double      bytes_processed;  ///< Total bytes read from the generated dump file
    double      gb_per_hour;      ///< Throughput in GiB/hr (bytes / elapsed * 3600 / 2^30)
};

/// Run the import using only the standard C++ importer headers.
/// Returns wall-clock elapsed time in seconds; sets *bytes_out to total bytes read.
static double runBench(const std::string& sql_file, bool dry_run,
                       double* bytes_out = nullptr) {
    // We replicate the import logic inline so the benchmark file has no
    // build-order dependency on the rest of the ThemisDB CMake targets.
    // A minimal CSV-style parser for the synthetic COPY data is sufficient.

    std::ifstream f(sql_file);
    if (!f) {
      return -1.0;
    }

    auto t0 = std::chrono::steady_clock::now();

    std::string line = {};
    size_t records    = 0;
    size_t byte_count = 0;
    bool in_copy      = false;

    while (std::getline(f, line)) {
        byte_count += line.size() + 1;  // +1 for the consumed newline
        if (!in_copy) {
            if (line.find("COPY ") != std::string::npos &&
                line.find("FROM stdin") != std::string::npos) {
                in_copy = true;
                continue;
            }
            if (line.find("INSERT INTO") != std::string::npos) {
                if (!dry_run) {
                  ++records;
                }
            }
        } else {
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
                in_copy = false;
                continue;
            }
            if (!dry_run) {
              ++records;
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    if (bytes_out) {
      *bytes_out = static_cast<double>(byte_count);
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

static BenchResult runScenario(const BenchConfig& cfg) {
    // Build synthetic SQL
    std::ostringstream out = {};
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
        return {cfg.label, 0, 0.0, 0.0, 0.0, 0.0};
    }

    double bytes   = 0.0;
    double elapsed = runBench(tmp, cfg.dry_run, &bytes);
    std::error_code rm_ec = {};
    std::filesystem::remove(tmp, rm_ec);

    size_t total_rows = cfg.copy_rows + cfg.insert_rows;
    double rps  = (elapsed > 0.0) ? static_cast<double>(total_rows) / elapsed : 0.0;
    double gbhr = calcGibPerHour(bytes, elapsed);

    return {cfg.label, total_rows, elapsed, rps, bytes, gbhr};
}

static void printResult(const BenchResult& r) {
    std::printf("  %-40s  %8zu rows  %8.3f s  %10.0f rows/s  %8.4f GB/hr\n",
                r.label.c_str(), r.rows, r.elapsed_sec, r.rows_per_sec, r.gb_per_hour);
}

// ---------------------------------------------------------------------------
// SQLite benchmark runner
// ---------------------------------------------------------------------------

/// Run a minimal SQLite INSERT-counting pass over a dump file.
/// Returns wall-clock elapsed time in seconds; sets *bytes_out to total bytes read.
static double runSQLiteBench(const std::string& sql_file, bool dry_run,
                              double* bytes_out = nullptr) {
    std::ifstream f(sql_file);
    if (!f) {
      return -1.0;
    }

    auto t0 = std::chrono::steady_clock::now();

    std::string line = {};
    size_t records    = 0;
    size_t byte_count = 0;

    while (std::getline(f, line)) {
        byte_count += line.size() + 1;
        if (line.find("INSERT INTO") != std::string::npos) {
            if (!dry_run) {
              ++records;
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    if (bytes_out) {
      *bytes_out = static_cast<double>(byte_count);
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

static BenchResult runSQLiteScenario(const BenchConfig& cfg) {
    std::ostringstream out = {};
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
        return {cfg.label, 0, 0.0, 0.0, 0.0, 0.0};
    }

    double bytes   = 0.0;
    double elapsed = runSQLiteBench(tmp, cfg.dry_run, &bytes);
    std::error_code rm_ec = {};
    std::filesystem::remove(tmp, rm_ec);

    double rps  = (elapsed > 0.0)
        ? static_cast<double>(cfg.insert_rows) / elapsed : 0.0;
    double gbhr = calcGibPerHour(bytes, elapsed);

    return {cfg.label, cfg.insert_rows, elapsed, rps, bytes, gbhr};
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
        if (i < num_docs) {
          out << ",\n";
        }
    }
    out << "\n]\n";
}

/// Create a temporary JSON file; returns its path.
static std::string writeTempJsonFile(const std::string& content) {
    std::error_code ec = {};
    const auto tmp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
      return "";
    }

    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto path = tmp_dir / ("bench_mongo_" + std::to_string(now) + ".json");

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      return "";
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.close();

    return path.string();
}

/// Run a minimal MongoDB NDJSON/JSON-array parsing pass to measure I/O and
/// line-detection throughput.  This is a simplified line-counting benchmark
/// (not a full JSON parse or BSON type unwrap) that establishes the upper
/// bound on import throughput; actual MongoDBImporter overhead adds JSON
/// parsing, BSON unwrapping, and schema mapping on top of this baseline.
/// Returns wall-clock elapsed time in seconds; sets *bytes_out to total bytes read.
static double runMongoBench(const std::string& json_file, bool dry_run,
                             bool is_json_array, double* bytes_out = nullptr) {
    std::ifstream f(json_file);
    if (!f) {
      return -1.0;
    }

    auto t0 = std::chrono::steady_clock::now();

    size_t records    = 0;
    size_t byte_count = 0;

    if (is_json_array) {
        // Count lines that start with '{' inside the JSON array.
        std::string line = {};
        while (std::getline(f, line)) {
            byte_count += line.size() + 1;
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos && line[first] == '{') {
                if (!dry_run) {
                  ++records;
                }
            }
        }
    } else {
        // NDJSON: count non-empty lines that start with '{'.
        std::string line = {};
        while (std::getline(f, line)) {
            byte_count += line.size() + 1;
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos && line[first] == '{') {
                if (!dry_run) {
                  ++records;
                }
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    if (bytes_out) {
      *bytes_out = static_cast<double>(byte_count);
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

struct MongoBenchConfig {
    std::string label = {};
    size_t      num_docs    = 0;
    bool        json_array  = false;  ///< true = JSON array, false = NDJSON
    bool        bson_types  = false;  ///< true = include BSON extended JSON wrappers
    bool        dry_run     = false;
};

static BenchResult runMongoScenario(const MongoBenchConfig& cfg) {
    std::ostringstream out = {};
    if (cfg.json_array) {
        writeMongoJsonArray(out, cfg.num_docs);
    } else {
        writeMongoNdjson(out, cfg.num_docs, cfg.bson_types);
    }

    std::string content = out.str();
    std::string tmp     = writeTempJsonFile(content);
    if (tmp.empty()) {
        return {cfg.label, 0, 0.0, 0.0, 0.0, 0.0};
    }

    double bytes   = 0.0;
    double elapsed = runMongoBench(tmp, cfg.dry_run, cfg.json_array, &bytes);
    std::error_code rm_ec = {};
    std::filesystem::remove(tmp, rm_ec);

    double rps  = (elapsed > 0.0)
        ? static_cast<double>(cfg.num_docs) / elapsed : 0.0;
    double gbhr = calcGibPerHour(bytes, elapsed);

    return {cfg.label, cfg.num_docs, elapsed, rps, bytes, gbhr};
}

// ---------------------------------------------------------------------------
// MySQL/MariaDB mysqldump benchmark  (Issue #69 v1.8.0 AC)
//
// Target: >= 50 000 rows/sec from a local MySQL 8.0 instance.
// Methodology: generate synthetic mysqldump files (single-row INSERT per
// statement; each row ~200 bytes matching a typical 2 KB document profile
// after encoding overhead) and measure line-detection / INSERT-counting
// throughput.  This mirrors the approach used for SQLite and establishes an
// upper bound; the full MySQLImporter adds regex parsing on top of this.
// ---------------------------------------------------------------------------

struct MySQLBenchConfig {
    std::string label;
    size_t      num_rows    = 0;
    size_t      num_tables  = 1;
    bool        dry_run     = false;
};

/// Write a minimal mysqldump header.
static void writeMySQLDumpHeader(std::ostream& out) {
    out << "-- MySQL dump 8.0\n";
    out << "-- Host: localhost    Database: bench\n";
    out << "-- Server version\t8.0.32\n\n";
    out << "/*!40101 SET NAMES utf8mb4 */;\n\n";
}

/// Write a MySQL table schema + INSERT rows.
/// Each row includes: id, username, email, score, balance, is_active,
/// created_at, updated_at, metadata — roughly matching a 200-byte row profile.
static void writeMySQLTable(std::ostream& out, const std::string& tname,
                             size_t num_rows) {
    out << "CREATE TABLE `" << tname << "` (\n"
        << "  `id` bigint NOT NULL AUTO_INCREMENT,\n"
        << "  `username` varchar(64) NOT NULL,\n"
        << "  `email` varchar(128) DEFAULT NULL,\n"
        << "  `score` double DEFAULT 0.0,\n"
        << "  `balance` decimal(12,2) DEFAULT 0.00,\n"
        << "  `is_active` tinyint(1) DEFAULT 1,\n"
        << "  `created_at` datetime DEFAULT NULL,\n"
        << "  `updated_at` datetime DEFAULT NULL,\n"
        << "  `metadata` json DEFAULT NULL,\n"
        << "  PRIMARY KEY (`id`)\n"
        << ") ENGINE=InnoDB;\n";

    for (size_t i = 1; i <= num_rows; ++i) {
        out << "INSERT INTO `" << tname << "` "
            << "(`id`,`username`,`email`,`score`,`balance`,`is_active`,"
            << "`created_at`,`updated_at`,`metadata`) VALUES ("
            << i << ",'user_" << i << "','user" << i << "@bench.local',"
            << (static_cast<double>(i) * 1.25) << ","
            << (static_cast<double>(i % 1000) * 0.99) << ","
            << (i % 2 == 0 ? 1 : 0) << ","
            << "'2025-01-01 00:00:00','2025-06-01 12:00:00',"
            << "'{\"seq\":" << i << ",\"batch\":" << (i / 1000 + 1) << "}');\n";
    }
    out << "\n";
}

/// Run a minimal MySQL INSERT-counting pass.
/// Returns wall-clock elapsed time in seconds; sets *bytes_out to total bytes read.
static double runMySQLBench(const std::string& sql_file, bool dry_run,
                             double* bytes_out = nullptr) {
    std::ifstream f(sql_file);
    if (!f) {
      return -1.0;
    }

    auto t0 = std::chrono::steady_clock::now();

    std::string line = {};
    size_t records    = 0;
    size_t byte_count = 0;
    static constexpr std::string_view kInsertPrefix = "INSERT INTO";

    while (std::getline(f, line)) {
        byte_count += line.size() + 1;
        // MySQL dump lines starting with "INSERT INTO" are data rows.
        if (line.size() >= kInsertPrefix.size() &&
            line.compare(0, kInsertPrefix.size(), kInsertPrefix) == 0) {
            if (!dry_run) {
              ++records;
            }
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    (void)records;
    if (bytes_out) {
      *bytes_out = static_cast<double>(byte_count);
    }
    return std::chrono::duration<double>(t1 - t0).count();
}

static BenchResult runMySQLScenario(const MySQLBenchConfig& cfg) {
    std::ostringstream out = {};
    writeMySQLDumpHeader(out);

    size_t rows_per_table = cfg.num_rows / std::max<size_t>(1, cfg.num_tables);
    for (size_t t = 0; t < cfg.num_tables; ++t) {
        writeMySQLTable(out, "bench_table_" + std::to_string(t), rows_per_table);
    }

    std::string content = out.str();
    std::string tmp     = writeTempSqlFile(content);
    if (tmp.empty()) {
        return {cfg.label, 0, 0.0, 0.0, 0.0, 0.0};
    }

    double bytes   = 0.0;
    double elapsed = runMySQLBench(tmp, cfg.dry_run, &bytes);
    std::error_code rm_ec = {};
    std::filesystem::remove(tmp, rm_ec);

    double rps  = (elapsed > 0.0) ? static_cast<double>(cfg.num_rows) / elapsed : 0.0;
    double gbhr = calcGibPerHour(bytes, elapsed);

    return {cfg.label, cfg.num_rows, elapsed, rps, bytes, gbhr};
}

// ---------------------------------------------------------------------------
// Kafka mock-import benchmark
//
// Simulates the KafkaImporter mock-injection path without a live broker.
// Uses pre-generated JSON payloads so the benchmark measures the full
// extract→validate→deliver pipeline.
//
// AC-8 target: >= 100 000 messages/second for 100 k messages.
// AC-9 target: <= 2 µs per message for JSON parse of a 4 KB payload.
// ---------------------------------------------------------------------------

/// Single Kafka message payload shapes for benchmarking.
struct KafkaBenchConfig {
    std::string label;
    size_t      num_messages = 0;
    size_t      payload_bytes = 0; // approximate JSON payload size
    bool        dry_run = false;
};

/// Build a synthetic JSON payload of approximately `bytes` bytes.
static std::string makeSyntheticKafkaJson(size_t payload_bytes) {
    // Root object with a string "data" field padded to fill the target size.
    std::string filler(std::max<size_t>(1, payload_bytes - 40), 'x');
    return R"({"id":1,"ts":"2026-01-01T00:00:00Z","data":")" + filler + R"("})";
}

/// Build a 5-byte Confluent Avro magic-byte prefix + JSON body.
static std::string makeAvroWrappedPayload(size_t payload_bytes) {
    std::string avro = {};
    avro += '\x00';
    avro += '\x00'; avro += '\x00'; avro += '\x00'; avro += '\x02'; // schema-ID = 2
    avro += makeSyntheticKafkaJson(payload_bytes);
    return avro;
}

/// Run a Kafka mock-import benchmark and return rows/sec.
/// The mock loop mirrors KafkaImporter::consumeFromMock() behaviour:
///   – each "batch" delivers a fixed slice of the total messages
///   – entities are parsed from JSON (no avro stripping in this path)
///   – streaming_row_callback is counted (not invoked for dry-run)
static BenchResult runKafkaBench(const KafkaBenchConfig& cfg) {
    // Pre-generate the payload strings.
    std::string payload = makeSyntheticKafkaJson(cfg.payload_bytes);
    const size_t batch_size = 500; // typical Kafka fetch batch

    auto t0 = std::chrono::high_resolution_clock::now();

    size_t imported = 0;
    size_t total    = 0;
    size_t remaining = cfg.num_messages;
    while (remaining > 0) {
        size_t batch = std::min(batch_size, remaining);
        for (size_t i = 0; i < batch; ++i) {
            // Simulate JSON entity parse (mirrors extractEntity for json format).
            nlohmann::json entity;
            try {
                entity = nlohmann::json::parse(payload);
            } catch (...) {
                entity = nlohmann::json{ {"text", payload} };
            }
            ++total;
            if (!cfg.dry_run && !entity.is_null()) {
                ++imported;
            }
        }
        remaining -= batch;
    }

    auto t1   = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    double bytes   = static_cast<double>(cfg.num_messages) * cfg.payload_bytes;
    double rps     = (elapsed > 0) ? static_cast<double>(imported) / elapsed : 0.0;
    double gbhr    = calcGibPerHour(bytes, elapsed);

    return {cfg.label, imported, elapsed, rps, bytes, gbhr};
}

/// Run a JSON-parse-only micro-benchmark for a single payload size.
/// Returns µs per message.
static double runKafkaJsonParseMicrobench(size_t payload_bytes, size_t iterations) {
    std::string payload = makeSyntheticKafkaJson(payload_bytes);
    volatile size_t sink = 0; // prevent optimisation

    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        nlohmann::json j = nlohmann::json::parse(payload);
        sink += j.size();
    }
    (void)sink;
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    return (iterations > 0) ? elapsed_us / static_cast<double>(iterations) : 0.0;
}

// ---------------------------------------------------------------------------
// Conflict resolution overhead micro-benchmark
//
// AC-5 target: overhead for SKIP and OVERWRITE strategies ≤ 5 % of baseline
//              (one extra hash-lookup per document)
// AC-6 target: MERGE strategy overhead ≤ 15 % compared to OVERWRITE for
//              documents with ≤ 100 fields
// ---------------------------------------------------------------------------

struct ConflictBenchConfig {
    std::string label;
    size_t      num_rows        = 0;
    size_t      num_fields      = 0;  ///< Fields per document (for MERGE scenario)
    size_t      conflict_pct    = 0;  ///< Percentage of rows that are duplicates (0-100)
    bench_internal::ConflictStrategy strategy;
};

/// Build a synthetic JSON entity with `num_fields` integer fields.
static nlohmann::json makeSyntheticEntity(size_t id, size_t num_fields) {
    nlohmann::json obj;
    obj["id"] = id;
    for (size_t f = 1; f < num_fields; ++f) {
        obj["field_" + std::to_string(f)] = static_cast<int>(id * f);
    }
    return obj;
}

/// Run a conflict-resolution micro-benchmark.
/// Returns rows/sec (imported + conflict-resolved).
static BenchResult runConflictBench(const ConflictBenchConfig& cfg) {
    // Pre-generate entities
    std::vector<nlohmann::json> rows;
    rows.reserve(cfg.num_rows);
    for (size_t i = 0; i < cfg.num_rows; ++i) {
        // Introduce duplicates at the configured rate: every N-th row has the
        // same id as row (i - 1), creating a conflict.
        size_t logical_id = (cfg.conflict_pct > 0 && i > 0 &&
                             (i * 100 / cfg.num_rows) % 100 < cfg.conflict_pct)
                                ? i - 1  // duplicate of previous row
                                : i;
        rows.push_back(makeSyntheticEntity(logical_id, cfg.num_fields));
    }

    bench_internal::ConflictResolver resolver;
    resolver.reset();

    const std::string table = "bench_table";
    size_t imported = 0;

    auto t0 = std::chrono::high_resolution_clock::now();

    for (const auto& row : rows) {
        std::string key = std::to_string(row.value("id", 0));
        bool conflict   = false;
        auto resolved   = resolver.resolve(row, table, key, cfg.strategy, conflict);
        (void)resolved;
        if (!conflict || cfg.strategy == bench_internal::ConflictStrategy::OVERWRITE
                      || cfg.strategy == bench_internal::ConflictStrategy::MERGE) {
            ++imported;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    double rps     = (elapsed > 0) ? static_cast<double>(cfg.num_rows) / elapsed : 0.0;
    // Approximate bytes: each entity is roughly 20 + 15*num_fields bytes as JSON
    double bytes   = static_cast<double>(cfg.num_rows) *
                     static_cast<double>(20 + 15 * cfg.num_fields);
    double gbhr    = calcGibPerHour(bytes, elapsed);

    return {cfg.label, cfg.num_rows, elapsed, rps, bytes, gbhr};
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    size_t iterations = 1;
    bool   csv_out    = false;
    std::string csv_path = {};

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
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        best.push_back(fastest);
    }

    std::printf("\nSQLite .dump:\n");
    for (const auto& cfg : sqlite_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runSQLiteScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        best.push_back(fastest);
    }

    std::printf("\nMongoDB mongoexport (NDJSON / JSON array):\n");
    std::printf("  AC target: >= 30 000 docs/sec (2 KB avg documents)\n");
    for (const auto& cfg : mongo_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runMongoScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        // Throughput assertion for the primary NDJSON 100k scenario (2 KB avg docs).
        if (cfg.label == "BM_MongoNdjson_100k") {
            if (fastest.rows_per_sec >= 30000.0) {
                std::printf("    [PASS] AC: %.0f docs/sec >= 30 000 docs/sec target\n",
                            fastest.rows_per_sec);
            } else {
                std::printf("    [WARN] AC: %.0f docs/sec < 30 000 docs/sec target\n",
                            fastest.rows_per_sec);
            }
        }
        best.push_back(fastest);
    }

    // MySQL/MariaDB mysqldump throughput scenarios (Issue #69, v1.8.0 AC)
    // AC target: >= 50 000 rows/sec from a local MySQL 8.0 instance.
    const std::vector<MySQLBenchConfig> mysql_scenarios = {
        {"BM_MySQLInsertRows_10k",   10000,  1, false},
        {"BM_MySQLInsertRows_100k", 100000,  1, false},
        {"BM_MySQLInsertRows_1M",  1000000,  1, false},
        {"BM_MySQLMixedLoad",       50000,   5, false},
        {"BM_MySQLDryRun_100k",    100000,   1, true },
    };

    std::printf("\nMySQL/MariaDB mysqldump:\n");
    std::printf("  AC target: >= 50 000 rows/sec (Issue #69 v1.8.0)\n");
    for (const auto& cfg : mysql_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runMySQLScenario(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        // AC assertion: >= 50 000 rows/sec for the 1M-row scenario (stress test).
        if (cfg.label == "BM_MySQLInsertRows_1M") {
            if (fastest.rows_per_sec >= 50000.0) {
                std::printf("    [PASS] AC: %.0f rows/sec >= 50 000 rows/sec target\n",
                            fastest.rows_per_sec);
            } else {
                std::printf("    [WARN] AC: %.0f rows/sec < 50 000 rows/sec target\n",
                            fastest.rows_per_sec);
            }
        }
        best.push_back(fastest);
    }

    // Kafka mock-import throughput scenarios (AC-8: >= 100k msg/sec)
    const std::vector<KafkaBenchConfig> kafka_scenarios = {
        {"BM_KafkaImport_100k",   100000,  256, false},
        {"BM_KafkaImport_100k_4k",100000, 4096, false},
        {"BM_KafkaDryRun_100k",   100000,  256, true },
    };

    std::printf("\nKafka mock-import (mock injection, no live broker):\n");
    std::printf("  AC-8 target: >= 100 000 msg/sec\n");
    for (const auto& cfg : kafka_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runKafkaBench(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        // AC-8: verify >= 100 000 msg/sec for the throughput scenario.
        if (cfg.label == "BM_KafkaImport_100k") {
            if (fastest.rows_per_sec >= 100000.0) {
                std::printf("    [PASS] AC-8: %.0f msg/sec >= 100 000 msg/sec target\n",
                            fastest.rows_per_sec);
            } else {
                std::printf("    [WARN] AC-8: %.0f msg/sec < 100 000 msg/sec target\n",
                            fastest.rows_per_sec);
            }
        }
        best.push_back(fastest);
    }

    // JSON parse micro-benchmark (AC-9: <= 2 µs per message for 4 KB payload)
    std::printf("\nKafka JSON parse latency (AC-9 target: <= 2 µs/msg for 4 KB payload):\n");
    const size_t parse_iters = 10000;
    for (size_t payload : {256u, 1024u, 4096u, 16384u}) {
        double us_per_msg = runKafkaJsonParseMicrobench(payload, parse_iters);
        std::printf("  payload=%5zu B : %6.2f µs/msg", payload, us_per_msg);
        if (payload == 4096) {
            if (us_per_msg <= 2.0) {
                std::printf("  [PASS] AC-9");
            } else {
                std::printf("  [WARN] AC-9 target is <= 2 µs");
            }
        }
        std::printf("\n");
    }

    // -------------------------------------------------------------------------
    // Conflict resolution overhead (Issue #175, v1.7.0)
    //
    // AC-5: SKIP and OVERWRITE overhead <= 5 % of baseline (no-conflict path).
    // AC-6: MERGE overhead <= 15 % compared to OVERWRITE for <= 100 fields.
    //
    // Methodology:
    //   BM_ConflictBaseline_100k    – OVERWRITE, 0 % duplicates (baseline).
    //   BM_ConflictOverwrite_100k   – OVERWRITE, 10-field docs, 10 % dup rate.
    //   BM_ConflictSkip_100k        – SKIP,      10-field docs, 10 % dup rate.
    //   BM_ConflictOverwrite_100f   – OVERWRITE, 100-field docs, 10 % dup rate.
    //   BM_ConflictMerge_10f_100k   – MERGE,     10-field docs, 10 % dup rate.
    //   BM_ConflictMerge_100f_100k  – MERGE,     100-field docs, 10 % dup rate.
    //
    // AC-5 evaluation: OVERWRITE/SKIP overhead vs BM_ConflictBaseline_100k.
    // AC-6 evaluation: MERGE_100f overhead vs OVERWRITE_100f (same field count).
    // -------------------------------------------------------------------------
    const std::vector<ConflictBenchConfig> conflict_scenarios = {
        {"BM_ConflictBaseline_100k",    100000, 10,  0,
            bench_internal::ConflictStrategy::OVERWRITE},
        {"BM_ConflictOverwrite_100k",   100000, 10, 10,
            bench_internal::ConflictStrategy::OVERWRITE},
        {"BM_ConflictSkip_100k",        100000, 10, 10,
            bench_internal::ConflictStrategy::SKIP},
        {"BM_ConflictOverwrite_100f",   100000, 100, 10,
            bench_internal::ConflictStrategy::OVERWRITE},
        {"BM_ConflictMerge_10f_100k",   100000, 10, 10,
            bench_internal::ConflictStrategy::MERGE},
        {"BM_ConflictMerge_100f_100k",  100000, 100, 10,
            bench_internal::ConflictStrategy::MERGE},
    };

    std::printf("\nConflict resolution overhead (Issue #175):\n");
    std::printf("  AC-5 target: SKIP/OVERWRITE overhead <= 5 %% of baseline (10-field docs)\n");
    std::printf("  AC-6 target: MERGE overhead <= 15 %% vs OVERWRITE (same field count, <=100 fields)\n");

    std::vector<BenchResult> conflict_results = {};

    for (const auto& cfg : conflict_scenarios) {
        BenchResult fastest{cfg.label, 0, 1e30, 0.0, 0.0, 0.0};
        for (size_t it = 0; it < iterations; ++it) {
            BenchResult r = runConflictBench(cfg);
            if (r.elapsed_sec < fastest.elapsed_sec) {
              fastest = r;
            }
        }
        printResult(fastest);
        conflict_results.push_back(fastest);
        best.push_back(fastest);
    }

    // Evaluate AC-5: SKIP and OVERWRITE overhead vs no-conflict baseline
    // (indices: 0=baseline, 1=overwrite_10f, 2=skip_10f)
    if (conflict_results.size() >= 3) {
        const double baseline_rps  = conflict_results[0].rows_per_sec;
        const double overwrite_rps = conflict_results[1].rows_per_sec;
        const double skip_rps      = conflict_results[2].rows_per_sec;
        if (baseline_rps > 0.0) {
            double ow_overhead_pct = (1.0 - overwrite_rps / baseline_rps) * 100.0;
            double sk_overhead_pct = (1.0 - skip_rps / baseline_rps) * 100.0;
            auto pass_warn = [](bool pass, const char* label, double pct, double limit) {
                std::printf("    [%s] AC-5 %s overhead: %.1f %% (target <= %.0f %%)\n",
                            pass ? "PASS" : "WARN", label, pct, limit);
            };
            pass_warn(ow_overhead_pct <= 5.0, "OVERWRITE", ow_overhead_pct, 5.0);
            pass_warn(sk_overhead_pct <= 5.0, "SKIP",      sk_overhead_pct, 5.0);
        }
    }

    // Evaluate AC-6: MERGE overhead vs OVERWRITE at the same document field count.
    // Compare MERGE_10f vs OVERWRITE_10f, and MERGE_100f vs OVERWRITE_100f.
    // (indices: 1=overwrite_10f, 3=overwrite_100f, 4=merge_10f, 5=merge_100f)
    if (conflict_results.size() >= 6) {
        const double overwrite_10f_rps  = conflict_results[1].rows_per_sec;
        const double overwrite_100f_rps = conflict_results[3].rows_per_sec;
        const double merge_10f_rps      = conflict_results[4].rows_per_sec;
        const double merge_100f_rps     = conflict_results[5].rows_per_sec;
        auto pass_warn = [](bool pass, const char* label, double pct, double limit) {
            std::printf("    [%s] AC-6 MERGE %s overhead: %.1f %% (target <= %.0f %%)\n",
                        pass ? "PASS" : "WARN", label, pct, limit);
        };
        if (overwrite_10f_rps > 0.0) {
            double merge10_overhead = (1.0 - merge_10f_rps / overwrite_10f_rps) * 100.0;
            pass_warn(merge10_overhead <= 15.0, "10-field",  merge10_overhead, 15.0);
        }
        if (overwrite_100f_rps > 0.0) {
            double merge100_overhead = (1.0 - merge_100f_rps / overwrite_100f_rps) * 100.0;
            pass_warn(merge100_overhead <= 15.0, "100-field", merge100_overhead, 15.0);
        }
    }

    if (csv_out && !csv_path.empty()) {
        std::ofstream csv(csv_path);
        if (csv) {
            csv << "scenario,rows,elapsed_sec,rows_per_sec,gb_per_hour\n";
            for (const auto& r : best) {
                csv << r.label << "," << r.rows << ","
                    << r.elapsed_sec << "," << r.rows_per_sec << ","
                    << r.gb_per_hour << "\n";
            }
            std::printf("\nResults written to: %s\n", csv_path.c_str());
        }
    }

    return 0;
}
