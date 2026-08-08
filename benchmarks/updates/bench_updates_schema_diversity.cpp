/**
 * @file bench_updates_schema_diversity.cpp
 * @brief Benchmark suite for schema migration on large datasets
 * @version 1.0.0
 * 
 * Benchmark Suite: Schema Migration Diversity
 * 
 * Workloads:
 *  - ADD COLUMN (10K-100K rows)
 *  - ALTER TABLE (index changes)
 *  - RENAME COLUMN (cross-reference updates)
 *  - DROP COLUMN with cascade
 * 
 * Metrics: migration time, throughput, memory usage
 */

#include <benchmark/benchmark.h>
#include <chrono>
#include <vector>
#include <memory>

#include "updates/schema_migration.h"
#include "updates/in_place_schema_migrator.h"

namespace themis {
namespace updates {
namespace benchmark {

// ============================================================================
// Test Fixtures
// ============================================================================

class SchemaMigrationBenchmark : public ::benchmark::Fixture {
protected:
    std::unique_ptr<SchemaMigrationService> migration_;
    
    void SetUp(const ::benchmark::State& state) override {
        migration_ = std::make_unique<SchemaMigrationService>();
        
        // Prepare tables with varying row counts
        if (state.range(0) == 1) {
            migration_->createTable("test_table", 10000);     // 10K rows
        } else if (state.range(0) == 2) {
            migration_->createTable("test_table", 50000);     // 50K rows
        } else {
            migration_->createTable("test_table", 100000);    // 100K rows
        }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        migration_.reset();
    }
};

// ============================================================================
// Benchmark 1: ADD COLUMN
// ============================================================================

BENCHMARK_F(SchemaMigrationBenchmark, AddColumn)(
    ::benchmark::State& state) {
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Prepare fresh table
        migration_->dropTable("test_table");
        size_t row_count = (state.range(0) == 1) ? 10000 : 
                          (state.range(0) == 2) ? 50000 : 100000;
        migration_->createTable("test_table", row_count);
        
        state.ResumeTiming();
        
        // Execute ADD COLUMN
        auto session = migration_->beginSession("test_table");
        session->addColumnChange("ADD COLUMN new_col INT DEFAULT 0");
        
        auto start = std::chrono::steady_clock::now();
        auto result = migration_->execute(session);
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        if (!result.success) {
            state.SkipWithError("Migration failed");
        }
        
        state.counters["row_count"] = row_count;
        state.counters["migration_time_ms"] = 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        state.counters["throughput_rows_per_sec"] = 
            static_cast<double>(row_count) / 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 1000;
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * 1024);
}

// ============================================================================
// Benchmark 2: ALTER TABLE (Index Changes)
// ============================================================================

BENCHMARK_F(SchemaMigrationBenchmark, AlterTableIndexes)(
    ::benchmark::State& state) {
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Prepare fresh table
        migration_->dropTable("test_table");
        size_t row_count = (state.range(0) == 1) ? 10000 : 
                          (state.range(0) == 2) ? 50000 : 100000;
        migration_->createTable("test_table", row_count);
        
        state.ResumeTiming();
        
        // Execute ALTER TABLE with index change
        auto session = migration_->beginSession("test_table");
        session->addColumnChange("CREATE INDEX idx_col1 ON test_table(col1)");
        session->addColumnChange("DROP INDEX idx_old ON test_table");
        
        auto start = std::chrono::steady_clock::now();
        auto result = migration_->execute(session);
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        if (!result.success) {
            state.SkipWithError("Migration failed");
        }
        
        state.counters["row_count"] = row_count;
        state.counters["migration_time_ms"] = 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark 3: RENAME COLUMN (Cross-Reference Updates)
// ============================================================================

BENCHMARK_F(SchemaMigrationBenchmark, RenameColumnWithCrossReferences)(
    ::benchmark::State& state) {
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Prepare tables with foreign key references
        migration_->dropTable("child_table");
        migration_->dropTable("test_table");
        
        size_t row_count = (state.range(0) == 1) ? 10000 : 
                          (state.range(0) == 2) ? 50000 : 100000;
        
        migration_->createTable("test_table", row_count);
        migration_->createTable("child_table", row_count);
        migration_->addForeignKey("child_table", "test_table", "id");
        
        state.ResumeTiming();
        
        // Rename column with cross-reference
        auto session = migration_->beginSession("test_table");
        session->addColumnChange("RENAME COLUMN id TO entity_id");
        
        auto start = std::chrono::steady_clock::now();
        auto result = migration_->execute(session);
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        if (!result.success) {
            state.SkipWithError("Migration failed");
        }
        
        // Verify foreign key updated
        auto fk_info = migration_->getForeignKeyInfo("child_table");
        if (fk_info.referenced_column != "entity_id") {
            state.SkipWithError("Foreign key not updated");
        }
        
        state.counters["row_count"] = row_count;
        state.counters["migration_time_ms"] = 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark 4: DROP COLUMN with Cascade
// ============================================================================

BENCHMARK_F(SchemaMigrationBenchmark, DropColumnWithCascade)(
    ::benchmark::State& state) {
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Prepare fresh table with indexes and constraints
        migration_->dropTable("test_table");
        size_t row_count = (state.range(0) == 1) ? 10000 : 
                          (state.range(0) == 2) ? 50000 : 100000;
        migration_->createTable("test_table", row_count);
        migration_->addIndexOnColumn("test_table", "col_to_drop");
        migration_->addConstraintOnColumn("test_table", "col_to_drop");
        
        state.ResumeTiming();
        
        // Execute DROP COLUMN with cascade
        auto session = migration_->beginSession("test_table");
        session->addColumnChange("DROP COLUMN col_to_drop CASCADE");
        
        auto start = std::chrono::steady_clock::now();
        auto result = migration_->execute(session);
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        if (!result.success) {
            state.SkipWithError("Migration failed");
        }
        
        // Verify index and constraint removed
        auto indexes = migration_->getIndexes("test_table");
        auto constraints = migration_->getConstraints("test_table");
        
        for (const auto& idx : indexes) {
            if (idx.column == "col_to_drop") {
                state.SkipWithError("Index not dropped");
            }
        }
        
        for (const auto& con : constraints) {
            if (con.column == "col_to_drop") {
                state.SkipWithError("Constraint not dropped");
            }
        }
        
        state.counters["row_count"] = row_count;
        state.counters["migration_time_ms"] = 
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark 5: Memory Usage During Migration
// ============================================================================

BENCHMARK_F(SchemaMigrationBenchmark, MemoryUsageDuringMigration)(
    ::benchmark::State& state) {
    
    for (auto _ : state) {
        state.PauseTiming();
        
        migration_->dropTable("test_table");
        size_t row_count = (state.range(0) == 1) ? 10000 : 
                          (state.range(0) == 2) ? 50000 : 100000;
        migration_->createTable("test_table", row_count);
        
        size_t mem_before = migration_->getMemoryUsage();
        
        state.ResumeTiming();
        
        auto session = migration_->beginSession("test_table");
        session->addColumnChange("ADD COLUMN new_col INT DEFAULT 0");
        
        size_t peak_mem = 0;
        auto monitor_fn = [&]() {
            while (!session->isComplete()) {
                size_t current_mem = migration_->getMemoryUsage();
                peak_mem = std::max(peak_mem, current_mem);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };
        
        std::thread monitor_thread(monitor_fn);
        auto result = migration_->execute(session);
        monitor_thread.join();
        
        state.PauseTiming();
        
        size_t mem_after = migration_->getMemoryUsage();
        size_t mem_growth = peak_mem - mem_before;
        
        if (!result.success) {
            state.SkipWithError("Migration failed");
        }
        
        state.counters["row_count"] = row_count;
        state.counters["peak_memory_mb"] = peak_mem / (1024 * 1024);
        state.counters["memory_growth_mb"] = mem_growth / (1024 * 1024);
        state.counters["memory_growth_pct"] = 
            (static_cast<double>(mem_growth) / mem_before) * 100;
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Performance Gates
// ============================================================================

/**
 * UPDP-6: Schema migration scalability: 100K rows < 10 seconds
 * 
 * Expected: Migration of 100K row table in < 10 seconds
 * Measurement: migration_time_ms from DropColumnWithCascade (worst case)
 * 
 * Covers:
 *  - All column operations (ADD, ALTER, RENAME, DROP)
 *  - Large row counts
 *  - Index/constraint handling
 */

/**
 * UPDP-Extended-3: Memory efficiency
 * 
 * Expected: Memory growth <= 5% of table size during migration
 * Measurement: memory_growth_pct from MemoryUsageDuringMigration
 */

/**
 * UPDP-Extended-4: Throughput scaling
 * 
 * Expected: Throughput increases monotonically with operation type
 * Measurement: throughput_rows_per_sec from AddColumn, AlterTableIndexes, etc.
 */

} // namespace benchmark
} // namespace updates
} // namespace themis

// Register benchmarks with different row count parameters
BENCHMARK_SUITE(SchemaMigrationBenchmark)
  ->Arg(1)    // 10K rows
  ->Arg(2)    // 50K rows  
  ->Arg(3);   // 100K rows
