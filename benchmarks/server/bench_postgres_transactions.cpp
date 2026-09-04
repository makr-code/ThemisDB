// PostgreSQL Wire Protocol - Transaction and Connection Benchmarks
// Measures transaction state transitions and connection overhead

#include <benchmark/benchmark.h>

#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include <string>
#include <vector>
#include <map>

// ============================================================================
// Transaction State Machine Benchmarks
// ============================================================================

enum class TransactionState {
    IDLE,
    IN_TRANSACTION,
    FAILED
};

static void BM_TransactionStateTransition(benchmark::State& state) {
    TransactionState txnState = TransactionState::IDLE;
    
    for (auto _ : state) {
        // Simulate BEGIN
        if (txnState == TransactionState::IDLE) {
            txnState = TransactionState::IN_TRANSACTION;
        }
        
        // Simulate COMMIT
        if (txnState == TransactionState::IN_TRANSACTION) {
            txnState = TransactionState::IDLE;
        }
        
        benchmark::DoNotOptimize(txnState);
    }
    
    state.SetItemsProcessed(state.iterations() * 2); // 2 transitions per iteration
}

static void BM_TransactionWithRollback(benchmark::State& state) {
    TransactionState txnState = TransactionState::IDLE;
    
    for (auto _ : state) {
        // BEGIN
        txnState = TransactionState::IN_TRANSACTION;
        
        // Simulate error
        txnState = TransactionState::FAILED;
        
        // ROLLBACK
        txnState = TransactionState::IDLE;
        
        benchmark::DoNotOptimize(txnState);
    }
    
    state.SetItemsProcessed(state.iterations() * 3);
}

// ============================================================================
// Prepared Statement Cache Benchmarks
// ============================================================================

struct PreparedStatement {
    std::string query = {};
    std::vector<int32_t> paramTypes;
};

static void BM_PreparedStatementLookup(benchmark::State& state) {
    int num_statements = state.range(0);
    std::map<std::string, PreparedStatement> cache;
    
    // Populate cache
    for (int i = 0; i < num_statements; ++i) {
        std::string name = "stmt_" + std::to_string(i);
        cache[name] = {"SELECT * FROM table", {23, 25}};
    }
    
    for (auto _ : state) {
        // Lookup a statement
        std::string lookup_name = "stmt_" + std::to_string(state.iterations() % num_statements);
        auto it = cache.find(lookup_name);
        
        if (it != cache.end()) {
            benchmark::DoNotOptimize(it->second);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_PreparedStatementInsert(benchmark::State& state) {
    std::map<std::string, PreparedStatement> cache;
    int counter = 0;
    
    for (auto _ : state) {
        std::string name = "stmt_" + std::to_string(counter++);
        PreparedStatement stmt{"SELECT * FROM table WHERE id = $1", {23}};
        cache[name] = stmt;
        
        benchmark::DoNotOptimize(cache);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_PreparedStatementDelete(benchmark::State& state) {
    int num_statements = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup
        std::map<std::string, PreparedStatement> cache = {};

        for (int i = 0; i < num_statements; ++i) {
            std::string name = "stmt_" + std::to_string(i);
            cache[name] = {"SELECT * FROM table", {23}};
        }
        
        state.ResumeTiming();
        
        // Delete a statement
        std::string delete_name = "stmt_0";
        cache.erase(delete_name);
        
        benchmark::DoNotOptimize(cache);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Portal Management Benchmarks
// ============================================================================

struct Portal {
    std::string statementName = {};
    std::vector<std::string> params;
    size_t currentRow = {};
    std::vector<std::vector<std::string>> cachedResults;
    bool resultsComplete = {};
};

static void BM_PortalCreation(benchmark::State& state) {
    for (auto _ : state) {
        Portal portal;
        portal.statementName = "stmt1";
        portal.params = {"value1", "value2", "value3"};
        portal.currentRow = 0;
        portal.resultsComplete = false;
        
        benchmark::DoNotOptimize(portal);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_PortalResultCaching(benchmark::State& state) {
    int num_rows = state.range(0);
    
    for (auto _ : state) {
        Portal portal;
        portal.statementName = "stmt1";
        portal.currentRow = 0;
        portal.resultsComplete = false;
        
        // Cache results
        for (int i = 0; i < num_rows; ++i) {
            std::vector<std::string> row = {"col1_" + std::to_string(i), 
                                           "col2_" + std::to_string(i),
                                           "col3_" + std::to_string(i)};
            portal.cachedResults.push_back(row);
        }
        portal.resultsComplete = true;
        
        benchmark::DoNotOptimize(portal);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * num_rows * 3 * 10); // approx bytes
}

static void BM_PortalResultStreaming(benchmark::State& state) {
    int total_rows = 1000;
    int max_rows = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup portal with cached results
        Portal portal;
        portal.currentRow = 0;
        for (int i = 0; i < total_rows; ++i) {
            portal.cachedResults.push_back({"val1", "val2", "val3"});
        }
        portal.resultsComplete = true;
        
        state.ResumeTiming();
        
        // Stream results in chunks
        int fetches = 0;
        while (portal.currentRow < portal.cachedResults.size()) {
            size_t rows_to_send = std::min(static_cast<size_t>(max_rows),
                                           portal.cachedResults.size() - portal.currentRow);
            
            // Simulate sending rows
            for (size_t i = 0; i < rows_to_send; ++i) {
                benchmark::DoNotOptimize(portal.cachedResults[portal.currentRow + i]);
            }
            
            portal.currentRow += rows_to_send;
            fetches++;
        }
        
        benchmark::DoNotOptimize(fetches);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Authentication Overhead Benchmarks
// ============================================================================

static void BM_ProtocolVersionValidation(benchmark::State& state) {
    const int32_t POSTGRES_PROTOCOL_V3 = 196608;
    
    for (auto _ : state) {
        int32_t clientVersion = POSTGRES_PROTOCOL_V3;
        bool valid = (clientVersion == POSTGRES_PROTOCOL_V3);
        
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_StartupParameterParsing(benchmark::State& state) {
    std::map<std::string, std::string> params = {
        {"user", "testuser"},
        {"database", "testdb"},
        {"application_name", "myapp"},
        {"client_encoding", "UTF8"}
    };
    
    for (auto _ : state) {
        std::string user = params["user"];
        std::string database = params["database"];
        
        bool valid = !user.empty() && !database.empty();
        
        benchmark::DoNotOptimize(valid);
        benchmark::DoNotOptimize(user);
        benchmark::DoNotOptimize(database);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// ReadyForQuery Message Benchmarks
// ============================================================================

static void BM_ReadyForQueryEncoding(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<uint8_t> message;
        message.push_back('Z'); // ReadyForQuery
        
        int32_t length = 5; // length field + status byte
        message.push_back((length >> 24) & 0xFF);
        message.push_back((length >> 16) & 0xFF);
        message.push_back((length >> 8) & 0xFF);
        message.push_back(length & 0xFF);
        
        message.push_back('I'); // IDLE status
        
        benchmark::DoNotOptimize(message);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Register Benchmarks
// ============================================================================

BENCHMARK(BM_TransactionStateTransition);
BENCHMARK(BM_TransactionWithRollback);

BENCHMARK(BM_PreparedStatementLookup)->Range(1, 1024);
BENCHMARK(BM_PreparedStatementInsert);
BENCHMARK(BM_PreparedStatementDelete)->Range(1, 1024);

BENCHMARK(BM_PortalCreation);
BENCHMARK(BM_PortalResultCaching)->Range(1, 10000);
BENCHMARK(BM_PortalResultStreaming)->Range(1, 1000);

BENCHMARK(BM_ProtocolVersionValidation);
BENCHMARK(BM_StartupParameterParsing);
BENCHMARK(BM_ReadyForQueryEncoding);

#endif // THEMIS_ENABLE_POSTGRES_WIRE

BENCHMARK_MAIN();
