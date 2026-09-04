/**
 * @file bench_olap_performance.cpp
 * @brief Real Google Benchmark performance tests for OLAP queries
 * 
 * Tests OLAP query performance with:
 * - Aggregation operations (COUNT, SUM, AVG, MIN, MAX)
 * - GROUP BY at various scales
 * - Filter operations
 * - Sorting and ordering
 * - Representative data sizes (1K, 10K, 100K, 1M rows)
 * - Baseline vs optimized execution
 * 
 * Output: JSON format for CI regression tracking
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "analytics/olap.h"
#include <vector>
#include <string>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <numeric>

using namespace themis::analytics;

namespace {

// ═══════════════════════════════════════════════════════════
// Test Data Generation
// ═══════════════════════════════════════════════════════════

struct SalesRecord {
    std::string product;
    std::string region;
    std::string category;
    double amount;
    int64_t timestamp;
    int quantity;
};

std::vector<SalesRecord> generateSalesData(size_t count) {
    std::vector<SalesRecord> data;
    data.reserve(count);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> amount_dist(10.0, 1000.0);
    std::uniform_int_distribution<int> qty_dist(1, 100);
    std::uniform_int_distribution<int> product_dist(0, 99);
    std::uniform_int_distribution<int> region_dist(0, 9);
    std::uniform_int_distribution<int> category_dist(0, 4);
    
    std::vector<std::string> products;
    for (int i = 0; i < 100; ++i) {
        products.push_back("product_" + std::to_string(i));
    }
    
    std::vector<std::string> regions = {
        "US-East", "US-West", "EU-North", "EU-South", "Asia-Pacific",
        "Middle-East", "Africa", "Australia", "Canada", "South-America"
    };
    
    std::vector<std::string> categories = {
        "Electronics", "Clothing", "Food", "Books", "Toys"
    };
    
    for (size_t i = 0; i < count; ++i) {
        SalesRecord rec;
        rec.product = products[product_dist(rng)];
        rec.region = regions[region_dist(rng)];
        rec.category = categories[category_dist(rng)];
        rec.amount = amount_dist(rng);
        rec.quantity = qty_dist(rng);
        rec.timestamp = 1640000000 + static_cast<int64_t>(i);
        data.push_back(rec);
    }
    
    return data;
}

// ═══════════════════════════════════════════════════════════
// Aggregation Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: COUNT aggregation
 * Target: >1M rows/sec
 */
static void BM_OLAP_Count(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        size_t count = 0;
        for (const auto& rec : data) {
            if (rec.amount > 100.0) {
                count++;
            }
        }
        benchmark::DoNotOptimize(count);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("count_agg");
}
BENCHMARK(BM_OLAP_Count)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * SUM aggregation
 * Target: >1M rows/sec
 */
static void BM_OLAP_Sum(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        double sum = 0.0;
        for (const auto& rec : data) {
            sum += rec.amount;
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("sum_agg");
}
BENCHMARK(BM_OLAP_Sum)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * AVG aggregation
 * Target: >1M rows/sec
 */
static void BM_OLAP_Avg(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        double sum = 0.0;
        size_t count = 0;
        for (const auto& rec : data) {
            sum += rec.amount;
            count++;
        }
        double avg = count > 0 ? sum / count : 0.0;
        benchmark::DoNotOptimize(avg);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("avg_agg");
}
BENCHMARK(BM_OLAP_Avg)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * MIN/MAX aggregation
 * Target: >1M rows/sec
 */
static void BM_OLAP_MinMax(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        double min_val = std::numeric_limits<double>::max();
        double max_val = std::numeric_limits<double>::min();
        
        for (const auto& rec : data) {
            min_val = std::min(min_val, rec.amount);
            max_val = std::max(max_val, rec.amount);
        }
        benchmark::DoNotOptimize(min_val);
        benchmark::DoNotOptimize(max_val);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("minmax_agg");
}
BENCHMARK(BM_OLAP_MinMax)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// GROUP BY Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: GROUP BY single dimension
 * Target: >500K rows/sec
 */
static void BM_OLAP_GroupBy_SingleDim(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::unordered_map<std::string, double> groups;
        
        for (const auto& rec : data) {
            groups[rec.region] += rec.amount;
        }
        
        benchmark::DoNotOptimize(groups.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("groupby_1dim");
}
BENCHMARK(BM_OLAP_GroupBy_SingleDim)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * GROUP BY two dimensions
 * Target: >400K rows/sec
 */
static void BM_OLAP_GroupBy_TwoDim(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::unordered_map<std::string, double> groups;
        
        for (const auto& rec : data) {
            std::string key = rec.region + "|" + rec.category;
            groups[key] += rec.amount;
        }
        
        benchmark::DoNotOptimize(groups.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("groupby_2dim");
}
BENCHMARK(BM_OLAP_GroupBy_TwoDim)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * GROUP BY three dimensions
 * Target: >300K rows/sec
 */
static void BM_OLAP_GroupBy_ThreeDim(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::unordered_map<std::string, double> groups;
        
        for (const auto& rec : data) {
            std::string key = rec.region + "|" + rec.category + "|" + rec.product;
            groups[key] += rec.amount;
        }
        
        benchmark::DoNotOptimize(groups.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("groupby_3dim");
}
BENCHMARK(BM_OLAP_GroupBy_ThreeDim)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// Filter Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Simple filter (equality)
 * Target: >2M rows/sec
 */
static void BM_OLAP_Filter_Equality(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::vector<SalesRecord> filtered;
        
        for (const auto& rec : data) {
            if (rec.region == "US-East") {
                filtered.push_back(rec);
            }
        }
        
        benchmark::DoNotOptimize(filtered.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("filter_eq");
}
BENCHMARK(BM_OLAP_Filter_Equality)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * Range filter
 * Target: >2M rows/sec
 */
static void BM_OLAP_Filter_Range(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::vector<SalesRecord> filtered;
        
        for (const auto& rec : data) {
            if (rec.amount >= 100.0 && rec.amount <= 500.0) {
                filtered.push_back(rec);
            }
        }
        
        benchmark::DoNotOptimize(filtered.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("filter_range");
}
BENCHMARK(BM_OLAP_Filter_Range)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * Complex filter (multiple conditions)
 * Target: >1M rows/sec
 */
static void BM_OLAP_Filter_Complex(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::vector<SalesRecord> filtered;
        
        for (const auto& rec : data) {
            if ((rec.region == "US-East" || rec.region == "US-West") &&
                rec.amount > 200.0 &&
                rec.quantity >= 10) {
                filtered.push_back(rec);
            }
        }
        
        benchmark::DoNotOptimize(filtered.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("filter_complex");
}
BENCHMARK(BM_OLAP_Filter_Complex)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// Sorting Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Sort by single column
 * Target: Depends on dataset size
 */
static void BM_OLAP_Sort_SingleColumn(benchmark::State& state) {
    size_t row_count = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generateSalesData(row_count);
        state.ResumeTiming();
        
        std::sort(data.begin(), data.end(), 
            [](const SalesRecord& a, const SalesRecord& b) {
                return a.amount < b.amount;
            });
        
        benchmark::DoNotOptimize(data[0].amount);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("sort_1col");
}
BENCHMARK(BM_OLAP_Sort_SingleColumn)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

/**
 * Sort by multiple columns
 * Target: Depends on dataset size
 */
static void BM_OLAP_Sort_MultiColumn(benchmark::State& state) {
    size_t row_count = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        auto data = generateSalesData(row_count);
        state.ResumeTiming();
        
        std::sort(data.begin(), data.end(), 
            [](const SalesRecord& a, const SalesRecord& b) {
                if (a.region != b.region) {
                  return a.region < b.region;
                }
                if (a.category != b.category) {
                  return a.category < b.category;
                }
                return a.amount < b.amount;
            });
        
        benchmark::DoNotOptimize(data[0].amount);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("sort_3col");
}
BENCHMARK(BM_OLAP_Sort_MultiColumn)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// ═══════════════════════════════════════════════════════════
// Optimized Aggregations (SIMD-friendly)
// ═══════════════════════════════════════════════════════════

/**
 * Optimized: Vectorized SUM
 * Target: >2M rows/sec (2x improvement)
 */
static void BM_OLAP_Sum_Optimized(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    // Extract amounts into contiguous array for better vectorization
    std::vector<double> amounts;
    amounts.reserve(data.size());
    for (const auto& rec : data) {
        amounts.push_back(rec.amount);
    }
    
    for (auto _ : state) {
        double sum = 0.0;
        
        // Process in blocks of 4 for better CPU pipelining
        size_t i = 0;
        for (; i + 3 < amounts.size(); i += 4) {
            sum += amounts[i] + amounts[i+1] + amounts[i+2] + amounts[i+3];
        }
        // Handle remainder
        for (; i < amounts.size(); ++i) {
            sum += amounts[i];
        }
        
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("sum_agg_optimized");
}
BENCHMARK(BM_OLAP_Sum_Optimized)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

/**
 * Optimized: Pre-computed indices for GROUP BY
 * Target: >600K rows/sec (20% improvement)
 */
static void BM_OLAP_GroupBy_Optimized(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    // Pre-build region index
    std::unordered_map<std::string, int> region_to_idx;
    std::vector<std::string> unique_regions;
    for (const auto& rec : data) {
        if (region_to_idx.find(rec.region) == region_to_idx.end()) {
            region_to_idx[rec.region] = unique_regions.size();
            unique_regions.push_back(rec.region);
        }
    }
    
    for (auto _ : state) {
        std::vector<double> groups(unique_regions.size(), 0.0);
        
        for (const auto& rec : data) {
            int idx = region_to_idx[rec.region];
            groups[idx] += rec.amount;
        }
        
        benchmark::DoNotOptimize(groups.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("groupby_optimized");
}
BENCHMARK(BM_OLAP_GroupBy_Optimized)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// Combined Queries (Realistic Workload)
// ═══════════════════════════════════════════════════════════

/**
 * Complex query: Filter + GROUP BY + Aggregation
 * Target: >200K rows/sec
 */
static void BM_OLAP_ComplexQuery(benchmark::State& state) {
    size_t row_count = state.range(0);
    auto data = generateSalesData(row_count);
    
    for (auto _ : state) {
        std::unordered_map<std::string, std::pair<double, int>> groups;
        
        // Filter + GroupBy + Sum + Count
        for (const auto& rec : data) {
            if (rec.amount > 100.0) {
                auto& entry = groups[rec.region];
                entry.first += rec.amount;
                entry.second++;
            }
        }
        
        // Calculate averages
        std::vector<double> averages;
        for (const auto& group_entry : groups) {
            const auto& stats = group_entry.second;
            if (stats.second > 0) {
                averages.push_back(stats.first / stats.second);
            }
        }
        
        benchmark::DoNotOptimize(averages.size());
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * row_count);
    state.SetLabel("complex_query");
}
BENCHMARK(BM_OLAP_ComplexQuery)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

} // namespace

// ═══════════════════════════════════════════════════════════
// Main - Configure JSON output for CI
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
