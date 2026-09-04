/**
 * @file bench_aql_mutations.cpp
 * @brief Google Benchmark suite for AQL mutation pipeline — EPIC-004 Phase 5.
 *
 * Measures end-to-end latency of the mutation pipeline stages:
 *   - BM_ParseInsert          — AQLParser::parseMutation() for INSERT
 *   - BM_ParseRemove          — AQLParser::parseMutation() for REMOVE
 *   - BM_ValidateInsert       — AqlMutationValidator::validate() for INSERT
 *   - BM_TranslateInsert      — AqlMutationTranslator::translate() for INSERT
 *   - BM_ExecuteInsert        — MutationExecutor::execute() for INSERT (in-memory)
 *   - BM_ExecuteInsertBatch   — N sequential INSERT executions (N = 10, 100, 1000)
 *   - BM_ExecuteRemove        — MutationExecutor::execute() for REMOVE (in-memory)
 *   - BM_TransactionBatch     — MutationTransactionContext: N puts + rollback
 *   - BM_ParseTransactionBlock— AQLParser::parseTransactionBlock() N statements
 */

#include <benchmark/benchmark.h>
#include "query/aql_parser.h"
#include "query/aql_mutation_validator.h"
#include "query/aql_translator.h"
#include "query/mutation_executor.h"
#include "query/mutation_execution_plan.h"
#include "query/mutation_transaction.h"

#include <atomic>
#include <map>
#include <optional>
#include <string>

using namespace themis::query;
using namespace themis;

// ============================================================================
// In-memory StorageContext — lightweight, allocation-minimal
// ============================================================================

struct BenchStorage : MutationExecutor::StorageContext {
    std::map<std::string, std::string> store;
    std::atomic<int>                   key_ctr{0};

    bool put(std::string_view /*col*/, std::string_view key, std::string_view val) override {
        store[std::string(key)] = std::string(val);
        return true;
    }
    bool remove(std::string_view /*col*/, std::string_view key) override {
        store.erase(std::string(key));
        return true;
    }
    bool exists(std::string_view /*col*/, std::string_view key) override {
        return store.count(std::string(key)) > 0;
    }
    std::string generateKey(std::string_view col) override {
        return std::string(col) + "_" + std::to_string(++key_ctr);
    }
    bool writeWAL(std::string_view /*col*/, const nlohmann::json& /*entry*/) override {
        return true;  // no-op for benchmarks
    }
    std::optional<std::string> get(std::string_view /*col*/, std::string_view key) override {
        auto it = store.find(std::string(key));
        if (it == store.end()) {
          return std::nullopt;
        }
        return it->second;
    }
};

// ============================================================================
// Shared fixture values — avoid repeated string construction inside hot loops
// ============================================================================
static const std::string kInsertAql   = "INSERT {name: 'Benchmark', value: 42} INTO bench_col";
static const std::string kRemoveAql   = "REMOVE 'bench_key' IN bench_col";
static const std::string kUpsertAql   =
    "UPSERT {_key: 'bench_key'} INSERT {name: 'new'} UPDATE {name: 'upd'} IN bench_col";

// ============================================================================
// BM_ParseInsert — latency of parsing a single INSERT statement
// ============================================================================
static void BM_ParseInsert(benchmark::State& state) {
    AQLParser parser;
    for (auto _ : state) {
        auto node = parser.parseMutation(kInsertAql);
        benchmark::DoNotOptimize(node);
    }
}
BENCHMARK(BM_ParseInsert);

// ============================================================================
// BM_ParseRemove — latency of parsing a single REMOVE statement
// ============================================================================
static void BM_ParseRemove(benchmark::State& state) {
    AQLParser parser;
    for (auto _ : state) {
        auto node = parser.parseMutation(kRemoveAql);
        benchmark::DoNotOptimize(node);
    }
}
BENCHMARK(BM_ParseRemove);

// ============================================================================
// BM_ValidateInsert — latency of semantic validation for INSERT
// ============================================================================
static void BM_ValidateInsert(benchmark::State& state) {
    AQLParser            parser;
    AqlMutationValidator validator;
    auto node = parser.parseMutation(kInsertAql);
    BENCHMARK_UNUSED(node);

    for (auto _ : state) {
        auto result = validator.validate(*node);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ValidateInsert);

// ============================================================================
// BM_TranslateInsert — latency of translating an INSERT node to an exec plan
// ============================================================================
static void BM_TranslateInsert(benchmark::State& state) {
    AQLParser             parser;
    AqlMutationTranslator translator;
    auto node = parser.parseMutation(kInsertAql);

    for (auto _ : state) {
        auto plan = translator.translate(node);
        benchmark::DoNotOptimize(plan);
    }
}
BENCHMARK(BM_TranslateInsert);

// ============================================================================
// BM_ExecuteInsert — latency of executing a single INSERT against in-memory storage
// ============================================================================
static void BM_ExecuteInsert(benchmark::State& state) {
    AQLParser             parser;
    AqlMutationTranslator translator;
    MutationExecutor      executor;
    BenchStorage          ctx;

    auto node = parser.parseMutation(kInsertAql);
    auto plan = translator.translate(node);

    for (auto _ : state) {
        ctx.store.clear();
        ctx.key_ctr.store(0);
        auto result = executor.execute(plan, ctx);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ExecuteInsert);

// ============================================================================
// BM_ExecuteInsertBatch — N sequential INSERT executions (range: 10/100/1000)
// ============================================================================
static void BM_ExecuteInsertBatch(benchmark::State& state) {
    const int64_t batch_size = state.range(0);
    AQLParser             parser;
    AqlMutationTranslator translator;
    MutationExecutor      executor;
    BenchStorage          ctx;

    // Pre-translate once; re-execute N times per iteration
    auto node = parser.parseMutation(kInsertAql);
    auto plan = translator.translate(node);

    for (auto _ : state) {
        ctx.store.clear();
        ctx.key_ctr.store(0);
        for (int64_t i = 0; i < batch_size; ++i) {
            auto result = executor.execute(plan, ctx);
            benchmark::DoNotOptimize(result);
        }
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_ExecuteInsertBatch)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// BM_ExecuteRemove — latency of executing a REMOVE against in-memory storage
// ============================================================================
static void BM_ExecuteRemove(benchmark::State& state) {
    AQLParser             parser;
    AqlMutationTranslator translator;
    MutationExecutor      executor;

    auto node = parser.parseMutation(kRemoveAql);
    auto plan = translator.translate(node);

    for (auto _ : state) {
        BenchStorage ctx;
        ctx.store["bench_key"] = R"({"value":1})";
        auto result = executor.execute(plan, ctx);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ExecuteRemove);

// ============================================================================
// BM_TransactionBatch — N puts via MutationTransactionContext then rollback
// ============================================================================
static void BM_TransactionBatch(benchmark::State& state) {
    const int64_t batch_size = state.range(0);

    for (auto _ : state) {
        BenchStorage base;
        MutationTransactionContext txn(base);

        for (int64_t i = 0; i < batch_size; ++i) {
            std::string key = "k" + std::to_string(i);
            txn.put("col", key, R"({"i":)" + std::to_string(i) + "}");
        }
        txn.rollback();
        benchmark::DoNotOptimize(base.store.size());
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_TransactionBatch)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// BM_ParseTransactionBlock — parsing a block with N mixed statements
// ============================================================================
static void BM_ParseTransactionBlock(benchmark::State& state) {
    const int64_t stmt_count = state.range(0);
    AQLParser parser;

    // Build a block string with stmt_count INSERT statements
    std::string aql = "BEGIN\n";
    for (int64_t i = 0; i < stmt_count; ++i) {
        aql += "  INSERT {n: " + std::to_string(i) + "} INTO bench_tbl\n";
    }
    aql += "COMMIT";

    for (auto _ : state) {
        auto result = parser.parseTransactionBlock(aql);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * stmt_count);
}
BENCHMARK(BM_ParseTransactionBlock)->Arg(1)->Arg(5)->Arg(20);

BENCHMARK_MAIN();
