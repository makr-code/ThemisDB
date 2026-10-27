/**
 * @file bench_updates_schema_diversity.cpp
 * @brief Benchmark suite for schema migration operations using the public API.
 */

#include <benchmark/benchmark.h>

#include "updates/schema_migration.h"

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace updates {
namespace benchmark {

class InMemoryMigrationStorage final : public IMigrationStorage {
public:
    bool put(const std::string& key, const std::string& value) override {
        rows_[key] = value;
        return true;
    }

    bool get(const std::string& key, std::string& value) override {
        auto it = rows_.find(key);
        if (it == rows_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    bool remove(const std::string& key) override {
        return rows_.erase(key) > 0;
    }

    bool listKeys(std::vector<std::string>& out) override {
        for (const auto& [key, _] : rows_) {
            out.push_back(key);
        }
        return true;
    }

private:
    std::unordered_map<std::string, std::string> rows_;
};

class SchemaMigrationBenchmark : public ::benchmark::Fixture {
protected:
    std::unique_ptr<InMemoryMigrationStorage> storage_;

    void SetUp(const ::benchmark::State&) override {
        storage_ = std::make_unique<InMemoryMigrationStorage>();
    }

    void TearDown(const ::benchmark::State&) override {
        storage_.reset();
    }
};

BENCHMARK_F(SchemaMigrationBenchmark, AddColumn)(::benchmark::State& state) {
    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        SchemaMigration migration("1.5.0");
        migration.addColumn("users", ColumnDef{"new_col", "INTEGER", true, "0", "benchmark column"});

        const auto result = migration.apply(*storage_);
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (!result.success) {
            state.SkipWithError("Schema migration failed");
        }

        state.counters["migration_time_ms"] = static_cast<double>(elapsed_ms);
        state.counters["operation_count"] = static_cast<double>(migration.operationCount());
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_F(SchemaMigrationBenchmark, AddIndex)(::benchmark::State& state) {
    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        SchemaMigration migration("1.5.1");
        migration.addIndex("users", IndexDef{"users_email_idx", {"email"}, false, true});

        const auto result = migration.apply(*storage_);
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (!result.success) {
            state.SkipWithError("Index migration failed");
        }

        state.counters["migration_time_ms"] = static_cast<double>(elapsed_ms);
        state.counters["operation_count"] = static_cast<double>(migration.operationCount());
        ::benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_REGISTER_F(SchemaMigrationBenchmark, AddColumn)
    ->Iterations(50)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/SchemaMigration/AddColumn");

BENCHMARK_REGISTER_F(SchemaMigrationBenchmark, AddIndex)
    ->Iterations(50)
    ->Unit(::benchmark::kMillisecond)
    ->Name("Updates/SchemaMigration/AddIndex");

} // namespace benchmark
} // namespace updates
} // namespace themis

