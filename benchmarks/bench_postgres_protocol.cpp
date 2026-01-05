// PostgreSQL Wire Protocol Benchmarks
// Benchmarks for protocol message encoding, decoding, and core operations

#include <benchmark/benchmark.h>

#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include "server/postgres_session.h"
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>

// ============================================================================
// Message Encoding Benchmarks
// ============================================================================

static void BM_EncodeSimpleQuery(benchmark::State& state) {
    std::string query = "SELECT * FROM users WHERE id = 123";
    
    for (auto _ : state) {
        std::vector<uint8_t> message;
        message.push_back('Q'); // Query message type
        
        int32_t length = query.size() + 4 + 1; // length + null terminator
        message.push_back((length >> 24) & 0xFF);
        message.push_back((length >> 16) & 0xFF);
        message.push_back((length >> 8) & 0xFF);
        message.push_back(length & 0xFF);
        
        message.insert(message.end(), query.begin(), query.end());
        message.push_back(0); // null terminator
        
        benchmark::DoNotOptimize(message);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_EncodeRowDescription(benchmark::State& state) {
    int num_fields = state.range(0);
    
    for (auto _ : state) {
        std::vector<uint8_t> payload;
        
        // Field count
        uint16_t fieldCount = num_fields;
        payload.push_back((fieldCount >> 8) & 0xFF);
        payload.push_back(fieldCount & 0xFF);
        
        for (int i = 0; i < num_fields; ++i) {
            std::string fieldName = "field" + std::to_string(i);
            payload.insert(payload.end(), fieldName.begin(), fieldName.end());
            payload.push_back(0); // null terminator
            
            // table OID, column number, type OID, type size, type modifier, format code
            int32_t tableOid = 0;
            int16_t colNum = i + 1;
            int32_t typeOid = 25; // text
            int16_t typeSize = -1;
            int32_t typeMod = -1;
            int16_t formatCode = 0;
            
            // Encode all fields
            payload.push_back((tableOid >> 24) & 0xFF);
            payload.push_back((tableOid >> 16) & 0xFF);
            payload.push_back((tableOid >> 8) & 0xFF);
            payload.push_back(tableOid & 0xFF);
            
            payload.push_back((colNum >> 8) & 0xFF);
            payload.push_back(colNum & 0xFF);
            
            payload.push_back((typeOid >> 24) & 0xFF);
            payload.push_back((typeOid >> 16) & 0xFF);
            payload.push_back((typeOid >> 8) & 0xFF);
            payload.push_back(typeOid & 0xFF);
            
            payload.push_back((typeSize >> 8) & 0xFF);
            payload.push_back(typeSize & 0xFF);
            
            payload.push_back((typeMod >> 24) & 0xFF);
            payload.push_back((typeMod >> 16) & 0xFF);
            payload.push_back((typeMod >> 8) & 0xFF);
            payload.push_back(typeMod & 0xFF);
            
            payload.push_back((formatCode >> 8) & 0xFF);
            payload.push_back(formatCode & 0xFF);
        }
        
        benchmark::DoNotOptimize(payload);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_EncodeDataRow(benchmark::State& state) {
    int num_columns = state.range(0);
    std::vector<std::string> values;
    
    for (int i = 0; i < num_columns; ++i) {
        values.push_back("value_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        std::vector<uint8_t> payload;
        
        // Column count
        uint16_t colCount = values.size();
        payload.push_back((colCount >> 8) & 0xFF);
        payload.push_back(colCount & 0xFF);
        
        for (const auto& value : values) {
            // Column length
            int32_t len = value.size();
            payload.push_back((len >> 24) & 0xFF);
            payload.push_back((len >> 16) & 0xFF);
            payload.push_back((len >> 8) & 0xFF);
            payload.push_back(len & 0xFF);
            
            // Column value
            payload.insert(payload.end(), value.begin(), value.end());
        }
        
        benchmark::DoNotOptimize(payload);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Parameter Escaping Benchmarks
// ============================================================================

static std::string escapeSQLString(const std::string& input) {
    std::string result;
    result.reserve(input.size() + 10);
    
    for (char c : input) {
        if (c == '\'') {
            result += "''";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\0') {
            continue;
        } else {
            result += c;
        }
    }
    
    return result;
}

static void BM_EscapeSQLString_Short(benchmark::State& state) {
    std::string input = "Hello World";
    
    for (auto _ : state) {
        auto result = escapeSQLString(input);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * input.size());
}

static void BM_EscapeSQLString_WithQuotes(benchmark::State& state) {
    std::string input = "It's a 'quoted' string with \\ backslashes";
    
    for (auto _ : state) {
        auto result = escapeSQLString(input);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * input.size());
}

static void BM_EscapeSQLString_Long(benchmark::State& state) {
    std::string input(1024, 'x');
    input += "'quoted'";
    
    for (auto _ : state) {
        auto result = escapeSQLString(input);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * input.size());
}

// ============================================================================
// Parameter Binding Benchmarks
// ============================================================================

static std::string bindParameterValue(const std::string& param, int32_t paramType) {
    if (param == "NULL") {
        return "NULL";
    }
    
    // For numeric types, validate and pass through
    if (paramType == 20 || paramType == 21 || paramType == 23 ||
        paramType == 700 || paramType == 701) {
        bool isValid = true;
        bool hasDot = false;
        for (size_t i = 0; i < param.size(); ++i) {
            char c = param[i];
            if (i == 0 && (c == '-' || c == '+')) continue;
            if (c == '.' && !hasDot) {
                hasDot = true;
                continue;
            }
            if (!std::isdigit(c)) {
                isValid = false;
                break;
            }
        }
        if (isValid && !param.empty()) {
            return param;
        }
    }
    
    return "'" + escapeSQLString(param) + "'";
}

static void BM_BindParameterValue_Numeric(benchmark::State& state) {
    std::string param = "12345";
    int32_t paramType = 23; // int4
    
    for (auto _ : state) {
        auto result = bindParameterValue(param, paramType);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_BindParameterValue_Text(benchmark::State& state) {
    std::string param = "Hello World!";
    int32_t paramType = 25; // text
    
    for (auto _ : state) {
        auto result = bindParameterValue(param, paramType);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_BindParameterValue_TextWithQuotes(benchmark::State& state) {
    std::string param = "It's a 'test' string";
    int32_t paramType = 25; // text
    
    for (auto _ : state) {
        auto result = bindParameterValue(param, paramType);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Query Substitution Benchmarks
// ============================================================================

static void BM_ParameterSubstitution(benchmark::State& state) {
    int num_params = state.range(0);
    std::string query = "SELECT * FROM users WHERE ";
    
    for (int i = 0; i < num_params; ++i) {
        if (i > 0) query += " AND ";
        query += "field" + std::to_string(i) + " = $" + std::to_string(i + 1);
    }
    
    std::vector<std::string> params;
    for (int i = 0; i < num_params; ++i) {
        params.push_back("value" + std::to_string(i));
    }
    
    for (auto _ : state) {
        std::string result = query;
        
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            std::string escapedValue = "'" + escapeSQLString(params[i]) + "'";
            
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), escapedValue);
                pos += escapedValue.length();
            }
        }
        
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// COPY Protocol Benchmarks
// ============================================================================

static void BM_COPYDataParsing(benchmark::State& state) {
    int num_rows = state.range(0);
    std::string data;
    
    for (int i = 0; i < num_rows; ++i) {
        data += "value1\tvalue2\tvalue3\n";
    }
    
    for (auto _ : state) {
        std::vector<std::string> rows;
        std::istringstream stream(data);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                rows.push_back(line);
            }
        }
        
        benchmark::DoNotOptimize(rows);
    }
    
    state.SetItemsProcessed(state.iterations() * num_rows);
    state.SetBytesProcessed(state.iterations() * data.size());
}

// ============================================================================
// Register Benchmarks
// ============================================================================

BENCHMARK(BM_EncodeSimpleQuery);
BENCHMARK(BM_EncodeRowDescription)->Range(1, 64);
BENCHMARK(BM_EncodeDataRow)->Range(1, 64);

BENCHMARK(BM_EscapeSQLString_Short);
BENCHMARK(BM_EscapeSQLString_WithQuotes);
BENCHMARK(BM_EscapeSQLString_Long);

BENCHMARK(BM_BindParameterValue_Numeric);
BENCHMARK(BM_BindParameterValue_Text);
BENCHMARK(BM_BindParameterValue_TextWithQuotes);

BENCHMARK(BM_ParameterSubstitution)->Range(1, 16);
BENCHMARK(BM_COPYDataParsing)->Range(1, 10000);

#endif // THEMIS_ENABLE_POSTGRES_WIRE

BENCHMARK_MAIN();
