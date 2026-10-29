#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct SqlScanResult {
    size_t rows = 0;
    size_t bytes = 0;
};

std::string generateSqlDump(size_t copy_rows, size_t insert_rows, size_t num_tables) {
    std::ostringstream oss;
    oss << "-- PostgreSQL-style dump\n";
    for (size_t t = 0; t < num_tables; ++t) {
        oss << "CREATE TABLE bench_table_" << t << " (id INTEGER PRIMARY KEY, name TEXT, value DOUBLE PRECISION);\n";
        if (copy_rows > 0) {
            oss << "COPY bench_table_" << t << " (id, name, value) FROM stdin;\n";
            for (size_t i = 1; i <= copy_rows / std::max<size_t>(1, num_tables); ++i) {
                oss << i << "\titem_" << i << "\t" << (i * 1.25) << "\n";
            }
            oss << "\\.\n";
        }
        for (size_t i = 1; i <= insert_rows / std::max<size_t>(1, num_tables); ++i) {
            oss << "INSERT INTO bench_table_" << t << " VALUES (" << (copy_rows + i) << ", 'item_" << (copy_rows + i) << "', " << (i * 1.25) << ");\n";
        }
    }
    return oss.str();
}

std::string generateSqliteDump(size_t insert_rows, size_t num_tables) {
    std::ostringstream oss;
    oss << "PRAGMA foreign_keys=OFF;\nBEGIN TRANSACTION;\n";
    for (size_t t = 0; t < num_tables; ++t) {
        oss << "CREATE TABLE bench_table_" << t << " (id INTEGER PRIMARY KEY, name TEXT, value REAL);\n";
        for (size_t i = 1; i <= insert_rows / std::max<size_t>(1, num_tables); ++i) {
            oss << "INSERT INTO bench_table_" << t << " VALUES (" << i << ", 'item_" << i << "', " << (i * 1.25) << ");\n";
        }
    }
    oss << "COMMIT;\n";
    return oss.str();
}

std::string generateMongoNdjson(size_t num_docs, bool bson_types) {
    std::ostringstream oss;
    for (size_t i = 1; i <= num_docs; ++i) {
        if (bson_types) {
            oss << "{\"_id\":{\"$oid\":\"" << std::string(24 - std::to_string(i).size(), '0') << i << "\"},\"name\":\"doc_" << i << "\",\"seq\":{\"$numberLong\":\"" << i << "\"},\"active\":true}\n";
        } else {
            oss << "{\"_id\":\"id" << i << "\",\"name\":\"doc_" << i << "\",\"seq\":" << i << ",\"active\":" << (i % 2 == 0 ? "true" : "false") << ",\"score\":" << (i * 1.25) << "}\n";
        }
    }
    return oss.str();
}

std::string generateMongoJsonArray(size_t num_docs) {
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 1; i <= num_docs; ++i) {
        oss << "{\"_id\":\"id" << i << "\",\"name\":\"doc_" << i << "\",\"seq\":" << i << ",\"active\":" << (i % 2 == 0 ? "true" : "false") << ",\"score\":" << (i * 1.25) << "}";
        if (i < num_docs) {
            oss << ",\n";
        }
    }
    oss << "\n]\n";
    return oss.str();
}

std::string generateMysqlDump(size_t num_rows, size_t num_tables) {
    std::ostringstream oss;
    oss << "-- MySQL dump\n";
    for (size_t t = 0; t < num_tables; ++t) {
        oss << "CREATE TABLE bench_table_" << t << " (id INTEGER PRIMARY KEY, name TEXT, value DOUBLE);\n";
        for (size_t i = 1; i <= num_rows / std::max<size_t>(1, num_tables); ++i) {
            oss << "INSERT INTO bench_table_" << t << " VALUES (" << i << ", 'user_" << i << "', " << (i * 1.25) << ");\n";
        }
    }
    return oss.str();
}

std::string generateKafkaJson(size_t payload_bytes) {
    std::string filler(payload_bytes > 40 ? payload_bytes - 40 : 1, 'x');
    return std::string("{\"id\":1,\"ts\":\"2026-01-01T00:00:00Z\",\"data\":\"") + filler + "\"}";
}

SqlScanResult scanSqlLikePayload(const std::string& content, bool dry_run) {
    SqlScanResult out;
    out.bytes = content.size();
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("INSERT INTO", 0) == 0 || line.rfind("COPY ", 0) == 0) {
            if (!dry_run) {
                ++out.rows;
            }
        }
    }
    return out;
}

SqlScanResult scanMongoPayload(const std::string& content, bool is_json_array) {
    SqlScanResult out;
    out.bytes = content.size();
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            continue;
        }
        if (is_json_array && line.find('{') != std::string::npos) {
            ++out.rows;
            continue;
        }
        if (!is_json_array && line.find('{') == 0) {
            ++out.rows;
        }
    }
    return out;
}

void BM_Importer_Postgres(benchmark::State& state) {
    const size_t copy_rows = static_cast<size_t>(state.range(0));
    const size_t insert_rows = static_cast<size_t>(state.range(1));
    const size_t num_tables = static_cast<size_t>(state.range(2));
    const std::string payload = generateSqlDump(copy_rows, insert_rows, num_tables);
    for (auto _ : state) {
        const auto scan = scanSqlLikePayload(payload, false);
        benchmark::DoNotOptimize(scan.rows);
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(copy_rows + insert_rows) * static_cast<int64_t>(state.iterations()));
}

void BM_Importer_Sqlite(benchmark::State& state) {
    const size_t insert_rows = static_cast<size_t>(state.range(0));
    const size_t num_tables = static_cast<size_t>(state.range(1));
    const std::string payload = generateSqliteDump(insert_rows, num_tables);
    for (auto _ : state) {
        const auto scan = scanSqlLikePayload(payload, false);
        benchmark::DoNotOptimize(scan.rows);
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(insert_rows) * static_cast<int64_t>(state.iterations()));
}

void BM_Importer_MongoNdjson(benchmark::State& state) {
    const size_t num_docs = static_cast<size_t>(state.range(0));
    const std::string payload = generateMongoNdjson(num_docs, false);
    for (auto _ : state) {
        const auto scan = scanMongoPayload(payload, false);
        benchmark::DoNotOptimize(scan.rows);
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(num_docs) * static_cast<int64_t>(state.iterations()));
}

void BM_Importer_MongoJsonArray(benchmark::State& state) {
    const size_t num_docs = static_cast<size_t>(state.range(0));
    const std::string payload = generateMongoJsonArray(num_docs);
    for (auto _ : state) {
        const auto scan = scanMongoPayload(payload, true);
        benchmark::DoNotOptimize(scan.rows);
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(num_docs) * static_cast<int64_t>(state.iterations()));
}

void BM_Importer_Mysql(benchmark::State& state) {
    const size_t num_rows = static_cast<size_t>(state.range(0));
    const size_t num_tables = static_cast<size_t>(state.range(1));
    const std::string payload = generateMysqlDump(num_rows, num_tables);
    for (auto _ : state) {
        const auto scan = scanSqlLikePayload(payload, false);
        benchmark::DoNotOptimize(scan.rows);
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(num_rows) * static_cast<int64_t>(state.iterations()));
}

void BM_Importer_KafkaJsonParse(benchmark::State& state) {
    const size_t payload_bytes = static_cast<size_t>(state.range(0));
    const std::string payload = generateKafkaJson(payload_bytes);
    for (auto _ : state) {
        const auto parsed = nlohmann::json::parse(payload);
        benchmark::DoNotOptimize(parsed.size());
    }
    state.SetBytesProcessed(static_cast<int64_t>(payload.size()) * static_cast<int64_t>(state.iterations()));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

void BM_Importer_ConflictResolve(benchmark::State& state) {
    const size_t row_count = static_cast<size_t>(state.range(0));
    const size_t field_count = static_cast<size_t>(state.range(1));
    const size_t dup_rate = static_cast<size_t>(state.range(2));
    std::vector<nlohmann::json> rows;
    rows.reserve(row_count);
    for (size_t i = 0; i < row_count; ++i) {
        nlohmann::json obj = nlohmann::json::object();
        obj["id"] = static_cast<int>(i);
        for (size_t f = 0; f < field_count; ++f) {
            obj["field_" + std::to_string(f)] = static_cast<int>((i * (f + 1)) % (dup_rate == 0 ? 1 : dup_rate + 1));
        }
        rows.emplace_back(obj);
    }
    for (auto _ : state) {
        size_t seen = 0;
        for (const auto& row : rows) {
            seen += static_cast<size_t>(row["id"].get<int>() % 2 == 0);
        }
        benchmark::DoNotOptimize(seen);
    }
    state.SetItemsProcessed(static_cast<int64_t>(row_count) * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_Importer_Postgres)
    ->Args({10'000u, 0u, 1u})
    ->Args({100'000u, 0u, 1u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_Sqlite)
    ->Args({10'000u, 1u})
    ->Args({100'000u, 1u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_MongoNdjson)
    ->Args({10'000u})
    ->Args({100'000u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_MongoJsonArray)
    ->Args({10'000u})
    ->Args({100'000u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_Mysql)
    ->Args({10'000u, 1u})
    ->Args({100'000u, 1u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_KafkaJsonParse)
    ->Args({256u})
    ->Args({1024u})
    ->Args({4096u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Importer_ConflictResolve)
    ->Args({100'000u, 10u, 10u})
    ->Args({100'000u, 100u, 10u})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

} // namespace
