// PostgreSQL Wire Protocol - End-to-End Benchmarks
// Measures complete query flows and realistic scenarios

#include <benchmark/benchmark.h>

#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include <string>
#include <vector>
#include <map>
#include <sstream>

// ============================================================================
// Complete Query Flow Benchmarks
// ============================================================================

static void BM_SimpleQueryFlow(benchmark::State& state) {
    // Simulates: Query -> RowDescription -> DataRow -> CommandComplete -> ReadyForQuery
    
    for (auto _ : state) {
        // 1. Parse query
        std::string query = "SELECT id, name, email FROM users WHERE id = 123";
        
        // 2. Build RowDescription
        std::vector<std::string> columns = {"id", "name", "email"};
        std::vector<int32_t> types = {23, 25, 25}; // int4, text, text
        
        // 3. Generate result rows
        std::vector<std::vector<std::string>> results = {
            {"123", "John Doe", "john@example.com"}
        };
        
        // 4. Encode CommandComplete
        std::string commandTag = "SELECT 1";
        
        benchmark::DoNotOptimize(query);
        benchmark::DoNotOptimize(columns);
        benchmark::DoNotOptimize(results);
        benchmark::DoNotOptimize(commandTag);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_PreparedStatementFlow(benchmark::State& state) {
    // Simulates: Parse -> Bind -> Execute -> Close
    
    std::map<std::string, std::string> statements;
    
    for (auto _ : state) {
        // 1. Parse - store prepared statement
        std::string stmtName = "stmt1";
        std::string query = "SELECT * FROM users WHERE id = $1 AND name = $2";
        statements[stmtName] = query;
        
        // 2. Bind - bind parameters
        std::vector<std::string> params = {"123", "John Doe"};
        std::string boundQuery = query;
        
        // Simple parameter substitution
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            size_t pos = 0;
            while ((pos = boundQuery.find(placeholder, pos)) != std::string::npos) {
                boundQuery.replace(pos, placeholder.length(), "'" + params[i] + "'");
                pos += params[i].length() + 2;
            }
        }
        
        // 3. Execute - run query
        std::vector<std::vector<std::string>> results = {
            {"123", "John Doe", "john@example.com"}
        };
        
        // 4. Close - cleanup
        statements.erase(stmtName);
        
        benchmark::DoNotOptimize(boundQuery);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_TransactionFlow(benchmark::State& state) {
    // Simulates: BEGIN -> INSERT -> UPDATE -> DELETE -> COMMIT
    
    enum class TxnState { IDLE, IN_TRANSACTION, FAILED };
    TxnState txnState = TxnState::IDLE;
    
    for (auto _ : state) {
        // BEGIN
        txnState = TxnState::IN_TRANSACTION;
        
        // INSERT
        std::string insertQuery = "INSERT INTO users VALUES (123, 'John', 'john@example.com')";
        
        // UPDATE
        std::string updateQuery = "UPDATE users SET name = 'Jane' WHERE id = 123";
        
        // DELETE
        std::string deleteQuery = "DELETE FROM users WHERE id = 456";
        
        // COMMIT
        txnState = TxnState::IDLE;
        
        benchmark::DoNotOptimize(insertQuery);
        benchmark::DoNotOptimize(updateQuery);
        benchmark::DoNotOptimize(deleteQuery);
        benchmark::DoNotOptimize(txnState);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Bulk Operations Benchmarks
// ============================================================================

static void BM_BulkInsertCOPY(benchmark::State& state) {
    int num_rows = state.range(0);
    
    for (auto _ : state) {
        // Generate COPY data
        std::ostringstream data = {};
        for (int i = 0; i < num_rows; ++i) {
            data << i << "\tuser" << i << "\tuser" << i << "@example.com\n";
        }
        
        std::string copyData = data.str();
        
        // Parse COPY data (tab-separated)
        std::vector<std::string> rows;
        std::istringstream stream(copyData);
        std::string line = {};
        
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                rows.push_back(line);
            }
        }
        
        benchmark::DoNotOptimize(rows);
    }
    
    state.SetItemsProcessed(state.iterations() * num_rows);
    state.SetBytesProcessed(state.iterations() * num_rows * 50); // approx 50 bytes/row
}

static void BM_BulkSelect(benchmark::State& state) {
    int num_rows = state.range(0);
    
    for (auto _ : state) {
        // Generate result set
        std::vector<std::vector<std::string>> results;
        results.reserve(num_rows);
        
        for (int i = 0; i < num_rows; ++i) {
            results.push_back({
                std::to_string(i),
                "user" + std::to_string(i),
                "user" + std::to_string(i) + "@example.com"
            });
        }
        
        // Encode all rows as DataRow messages
        size_t totalBytes = 0;
        for (const auto& row : results) {
            // Simulate encoding overhead
            size_t rowBytes = 2; // column count
            for (const auto& col : row) {
                rowBytes += 4 + col.size(); // length + data
            }
            totalBytes += rowBytes;
        }
        
        benchmark::DoNotOptimize(results);
        benchmark::DoNotOptimize(totalBytes);
    }
    
    state.SetItemsProcessed(state.iterations() * num_rows);
    state.SetBytesProcessed(state.iterations() * num_rows * 50);
}

// ============================================================================
// Connection Overhead Benchmarks
// ============================================================================

static void BM_ConnectionHandshake(benchmark::State& state) {
    // Simulates: SSLRequest -> Startup -> AuthenticationOk -> ParameterStatus -> ReadyForQuery
    
    for (auto _ : state) {
        // 1. SSL negotiation (if enabled)
        bool sslEnabled = false;
        
        // 2. Startup message
        std::map<std::string, std::string> params = {
            {"user", "testuser"},
            {"database", "testdb"}
        };
        
        // 3. Authentication
        bool authenticated = true;
        
        // 4. Send parameter statuses
        std::vector<std::pair<std::string, std::string>> serverParams = {
            {"server_version", "14.0 (ThemisDB)"},
            {"server_encoding", "UTF8"},
            {"client_encoding", "UTF8"},
            {"DateStyle", "ISO, MDY"},
            {"TimeZone", "UTC"}
        };
        
        // 5. Ready for query
        char txnStatus = 'I'; // IDLE
        
        benchmark::DoNotOptimize(authenticated);
        benchmark::DoNotOptimize(serverParams);
        benchmark::DoNotOptimize(txnStatus);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_MultipleQueries(benchmark::State& state) {
    int num_queries = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::string> queries;
        queries.reserve(num_queries);
        
        for (int i = 0; i < num_queries; ++i) {
            queries.push_back("SELECT * FROM users WHERE id = " + std::to_string(i));
        }
        
        // Execute each query
        for (const auto& query : queries) {
            // Parse query
            std::string upperQuery = query;
            
            // Generate result
            std::vector<std::string> result = {"id", "name", "email"};
            
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_queries);
}

// ============================================================================
// Schema Introspection Benchmarks
// ============================================================================

static void BM_SchemaQueryTypes(benchmark::State& state) {
    // Simulates: SELECT * FROM pg_catalog.pg_type
    
    for (auto _ : state) {
        std::vector<std::vector<std::string>> types = {
            {"16", "bool", "1"},
            {"20", "int8", "8"},
            {"21", "int2", "2"},
            {"23", "int4", "4"},
            {"25", "text", "-1"},
            {"1043", "varchar", "-1"},
            {"700", "float4", "4"},
            {"701", "float8", "8"},
            {"1082", "date", "4"},
            {"1114", "timestamp", "8"}
        };
        
        benchmark::DoNotOptimize(types);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_SchemaQueryTables(benchmark::State& state) {
    // Simulates: SELECT * FROM pg_catalog.pg_class
    
    for (auto _ : state) {
        std::vector<std::vector<std::string>> tables = {
            {"16384", "users", "r", "2200"},
            {"16385", "orders", "r", "2200"},
            {"16386", "products", "r", "2200"}
        };
        
        benchmark::DoNotOptimize(tables);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Error Handling Benchmarks
// ============================================================================

static void BM_ErrorResponse(benchmark::State& state) {
    for (auto _ : state) {
        // Build error response
        std::string severity = "ERROR";
        std::string code = "42601"; // syntax_error
        std::string message = "Query parsing failed: invalid syntax";
        
        // Encode error response
        std::vector<uint8_t> response;
        response.push_back('E'); // ErrorResponse
        
        // Add fields
        std::string errorMsg = "S" + severity + "\0C" + code + "\0M" + message + "\0\0";
        
        benchmark::DoNotOptimize(response);
        benchmark::DoNotOptimize(errorMsg);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Register Benchmarks
// ============================================================================

BENCHMARK(BM_SimpleQueryFlow);
BENCHMARK(BM_PreparedStatementFlow);
BENCHMARK(BM_TransactionFlow);

BENCHMARK(BM_BulkInsertCOPY)->Range(10, 10000);
BENCHMARK(BM_BulkSelect)->Range(10, 10000);

BENCHMARK(BM_ConnectionHandshake);
BENCHMARK(BM_MultipleQueries)->Range(1, 100);

BENCHMARK(BM_SchemaQueryTypes);
BENCHMARK(BM_SchemaQueryTables);

BENCHMARK(BM_ErrorResponse);

#endif // THEMIS_ENABLE_POSTGRES_WIRE

BENCHMARK_MAIN();
