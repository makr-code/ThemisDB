/**
 * @file bench_aql_functions.cpp
 * @brief Benchmarks for the modular AQL Function Registry
 * 
 * Measures performance impact and query cost for all function categories.
 */

#include <benchmark/benchmark.h>
#include "query/functions/function_registry.h"
#include "query/functions/string_functions.h"
#include "query/functions/math_functions.h"
#include "query/functions/array_functions.h"
#include "query/functions/date_functions.h"
#include "query/functions/document_functions.h"
#include "query/functions/geo_functions.h"
#include "query/functions/vector_functions.h"
#include "query/functions/graph_functions.h"
#include "query/functions/collection_functions.h"
// #include "query/functions/excel_functions.h"  // v1.4.0: Not yet available

using namespace themis::query::functions;

// ============================================================================
// Setup
// ============================================================================

class AQLFunctionBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) {
        registerBuiltinFunctions();
    }
    
    FunctionContext ctx;
};

// ============================================================================
// String Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, StringLength)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    std::string testStr(state.range(0), 'x');
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("LENGTH", {testStr}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, StringLength)
    ->Range(8, 8<<10)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, StringConcat)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("CONCAT", {"Hello", " ", "World", "!"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, StringRegexTest)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("REGEX_TEST", {"test@example.com", "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, LevenshteinDistance)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("LEVENSHTEIN_DISTANCE", {"kitten", "sitting"}, ctx));
    }
}

// ============================================================================
// Math Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, MathSqrt)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SQRT", {12345.6789}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, MathTrigonometry)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SIN", {1.5708}, ctx));
        benchmark::DoNotOptimize(reg.call("COS", {1.5708}, ctx));
        benchmark::DoNotOptimize(reg.call("TAN", {0.7854}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, MathAggregateSum)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json arr = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        arr.push_back(i);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SUM", {arr}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, MathAggregateSum)
    ->Range(8, 8<<10)
    ->Complexity();

// ============================================================================
// Array Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, ArrayFlatten)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json nested = nlohmann::json::array({
        nlohmann::json::array({1, 2, 3}),
        nlohmann::json::array({4, 5, 6}),
        nlohmann::json::array({7, 8, 9})
    });
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("FLATTEN", {nested}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, ArrayUnique)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json arr = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        arr.push_back(i % 100);  // Create duplicates
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("UNIQUE", {arr}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, ArrayUnique)
    ->Range(8, 8<<10)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, ArraySorted)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json arr = nlohmann::json::array();
    for (int i = state.range(0); i > 0; --i) {
        arr.push_back(i);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SORTED", {arr}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, ArraySorted)
    ->Range(8, 8<<10)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, ArrayUnionIntersection)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json arr1 = nlohmann::json::array({1, 2, 3, 4, 5});
    nlohmann::json arr2 = nlohmann::json::array({4, 5, 6, 7, 8});
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("UNION", {arr1, arr2}, ctx));
        benchmark::DoNotOptimize(reg.call("INTERSECTION", {arr1, arr2}, ctx));
    }
}

// ============================================================================
// Date Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, DateNow)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("NOW", {}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, DateFormat)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    int64_t timestamp = 1701388800000;  // 2023-12-01
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("DATE_FORMAT", {timestamp, "%Y-%m-%d %H:%M:%S"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, DateArithmetic)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    int64_t timestamp = 1701388800000;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("DATE_ADD", {timestamp, 30, "day"}, ctx));
        benchmark::DoNotOptimize(reg.call("DATE_SUBTRACT", {timestamp, 1, "month"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, DateWorkdays)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    int64_t start = 1701388800000;   // 2023-12-01
    int64_t end = 1704067200000;     // 2024-01-01
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("WORKDAYS", {start, end}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, DateIntervals)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("DAYS", {30}, ctx));
        benchmark::DoNotOptimize(reg.call("HOURS", {24}, ctx));
        benchmark::DoNotOptimize(reg.call("MINUTES", {60}, ctx));
    }
}

// ============================================================================
// Geo Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, GeoPoint)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("ST_POINT", {13.4, 52.5}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, GeoDistance)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json berlin = {{"type", "Point"}, {"coordinates", {13.4, 52.5}}};
    nlohmann::json munich = {{"type", "Point"}, {"coordinates", {11.6, 48.1}}};
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("GEO_DISTANCE", {berlin, munich}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, GeoContains)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json polygon = {
        {"type", "Polygon"},
        {"coordinates", {{
            {0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}, {0.0, 0.0}
        }}}
    };
    nlohmann::json point = {{"type", "Point"}, {"coordinates", {5.0, 5.0}}};
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("ST_CONTAINS", {polygon, point}, ctx));
    }
}

// ============================================================================
// CRS Transformation Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, CrsTransformUtmToWgs84)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json utmPoint = {{"type", "Point"}, {"coordinates", {500000.0, 5500000.0}}};
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("ST_TRANSFORM", {utmPoint, 25832, 4326}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, CrsTransformWgs84ToUtm)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json wgsPoint = {{"type", "Point"}, {"coordinates", {13.4, 52.5}}};
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("ST_TRANSFORM", {wgsPoint, 4326, 25833}, ctx));
    }
}

// ============================================================================
// Vector Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, VectorCosineSimilarity)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json v1 = nlohmann::json::array();
    nlohmann::json v2 = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        v1.push_back(static_cast<double>(i) / state.range(0));
        v2.push_back(static_cast<double>(state.range(0) - i) / state.range(0));
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("COSINE_SIMILARITY", {v1, v2}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, VectorCosineSimilarity)
    ->Range(8, 1<<12)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, VectorEuclideanDistance)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json v1 = nlohmann::json::array({1.0, 2.0, 3.0, 4.0, 5.0});
    nlohmann::json v2 = nlohmann::json::array({5.0, 4.0, 3.0, 2.0, 1.0});
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("EUCLIDEAN_DISTANCE", {v1, v2}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, VectorNormalize)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    nlohmann::json v = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        v.push_back(static_cast<double>(i));
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("L2_NORMALIZE", {v}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, VectorNormalize)
    ->Range(8, 1<<12)
    ->Complexity();

// ============================================================================
// Graph Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, GraphShortestPath)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a chain graph
    nlohmann::json edges = nlohmann::json::array();
    for (int i = 0; i < state.range(0) - 1; ++i) {
        edges.push_back({
            {"_from", "n/" + std::to_string(i)},
            {"_to", "n/" + std::to_string(i + 1)}
        });
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SHORTEST_PATH", {
            "n/0", 
            "n/" + std::to_string(state.range(0) - 1), 
            edges, 
            "outbound"
        }, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, GraphShortestPath)
    ->Range(8, 256)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, GraphPageRank)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a small graph
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "a/1"}, {"_to", "a/2"}},
        {{"_from", "a/2"}, {"_to", "a/3"}},
        {{"_from", "a/3"}, {"_to", "a/1"}},
        {{"_from", "a/1"}, {"_to", "a/3"}}
    });
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("PAGERANK", {edges}, ctx));
    }
}

// ============================================================================
// Collection Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, JsonParse)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    std::string jsonStr = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]";
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("JSON", {jsonStr}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, DictConstruction)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("DICT", {"name", "Alice", "age", 30, "city", "Berlin"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, HolidaysLookup)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("HOLIDAYS", {"DE_2024"}, ctx));
    }
}

// ============================================================================
// Logical Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, LogicalAndOr)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("AND", {true, true, true, true}, ctx));
        benchmark::DoNotOptimize(reg.call("OR", {false, false, false, true}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, LogicalIf)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("IF", {true, "yes", "no"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, LogicalSwitch)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SWITCH", {2, 1, "one", 2, "two", 3, "three", "default"}, ctx));
    }
}

// ============================================================================
// Excel Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, ExcelVlookup)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json table = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        table.push_back(nlohmann::json::array({
            "ID" + std::to_string(i),
            "Name" + std::to_string(i),
            i * 100
        }));
    }
    
    std::string searchKey = "ID" + std::to_string(state.range(0) / 2);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("VLOOKUP", {searchKey, table, 3}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, ExcelVlookup)
    ->Range(8, 1024)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, ExcelSumproduct)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json arr1 = nlohmann::json::array();
    nlohmann::json arr2 = nlohmann::json::array();
    for (int i = 0; i < state.range(0); ++i) {
        arr1.push_back(i);
        arr2.push_back(i * 2);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("SUMPRODUCT", {arr1, arr2}, ctx));
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(AQLFunctionBenchmark, ExcelSumproduct)
    ->Range(8, 1<<10)
    ->Complexity();

BENCHMARK_F(AQLFunctionBenchmark, ExcelPmt)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("PMT", {0.06/12, 360, 200000}, ctx));
    }
}

// ============================================================================
// Security Function Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, SecurityValidateEmail)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("IS_EMAIL", {"test@example.com"}, ctx));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, SecurityMask)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("MASK", {"1234567890123456"}, ctx));
    }
}

// ============================================================================
// Registry Overhead Benchmarks
// ============================================================================

BENCHMARK_F(AQLFunctionBenchmark, RegistryLookup)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.hasFunction("COSINE_SIMILARITY"));
    }
}

BENCHMARK_F(AQLFunctionBenchmark, RegistryCall)(benchmark::State& state) {
    auto& reg = FunctionRegistry::instance();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(reg.call("ABS", {-42}, ctx));
    }
}

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
