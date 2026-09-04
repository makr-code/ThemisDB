/**
 * @file bench_olap_analytics.cpp
 * @brief Productive Google Benchmark performance tests for OLAP analytics cases.
 *
 * Implements ≥4 analytics-targeted benchmark cases per Maßnahme #2
 * (PERFORMANCE_EXPECTATIONS.md §1.4):
 *   - BM_OLAP_GroupBy_Int       – aggregated GROUP BY over integer keys (1 M rows)
 *   - BM_OLAP_WindowFunction    – sliding-window running aggregation
 *   - BM_OLAP_MultiJoin         – join across ≥3 tables with GROUP BY aggregation
 *   - BM_OLAP_TopN_Sorted       – Top-N query with ORDER BY (heap-based)
 *
 * Output: JSON format for CI regression tracking
 * CI-Audit target: perf_audit check 2b PASS (productive_count >= 4)
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ═══════════════════════════════════════════════════════════
// Test Data Generation
// ═══════════════════════════════════════════════════════════

struct SalesRow {
    int64_t product_id;
    int64_t region_id;
    int64_t category_id;
    double  amount;
    int     quantity;
    int64_t timestamp;
};

struct OrderRow {
    int64_t order_id;
    int64_t customer_id;
    int64_t product_id;
    double  price;
};

struct CustomerRow {
    int64_t customer_id;
    int64_t region_id;
    int     age;
};

struct ProductRow {
    int64_t product_id;
    int64_t category_id;
    double  cost;
};

std::vector<SalesRow> generateSales(size_t n) {
    std::vector<SalesRow> data;
    data.reserve(n);
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> prod(0, 99);
    std::uniform_int_distribution<int64_t> region(0, 9);
    std::uniform_int_distribution<int64_t> cat(0, 4);
    std::uniform_real_distribution<double>  amt(10.0, 1000.0);
    std::uniform_int_distribution<int>      qty(1, 100);
    for (size_t i = 0; i < n; ++i) {
        data.push_back({prod(rng), region(rng), cat(rng),
                        amt(rng), qty(rng), static_cast<int64_t>(1640000000 + i)});
    }
    return data;
}

std::vector<OrderRow> generateOrders(size_t n) {
    std::vector<OrderRow> data;
    data.reserve(n);
    std::mt19937_64 rng(1);
    std::uniform_int_distribution<int64_t> cust(0, 9999);
    std::uniform_int_distribution<int64_t> prod(0, 99);
    std::uniform_real_distribution<double>  price(5.0, 500.0);
    for (size_t i = 0; i < n; ++i) {
        data.push_back({static_cast<int64_t>(i), cust(rng), prod(rng), price(rng)});
    }
    return data;
}

std::vector<CustomerRow> generateCustomers(size_t n) {
    std::vector<CustomerRow> data;
    data.reserve(n);
    std::mt19937_64 rng(2);
    std::uniform_int_distribution<int64_t> region(0, 9);
    std::uniform_int_distribution<int>      age(18, 80);
    for (size_t i = 0; i < n; ++i) {
        data.push_back({static_cast<int64_t>(i), region(rng), age(rng)});
    }
    return data;
}

std::vector<ProductRow> generateProducts(size_t n) {
    std::vector<ProductRow> data;
    data.reserve(n);
    std::mt19937_64 rng(3);
    std::uniform_int_distribution<int64_t> cat(0, 4);
    std::uniform_real_distribution<double>  cost(1.0, 200.0);
    for (size_t i = 0; i < n; ++i) {
        data.push_back({static_cast<int64_t>(i), cat(rng), cost(rng)});
    }
    return data;
}

} // namespace

// ═══════════════════════════════════════════════════════════
// Case 1: BM_OLAP_GroupBy_Int
// ═══════════════════════════════════════════════════════════

/**
 * GROUP BY over integer keys with SUM and COUNT aggregation.
 * Simulates: SELECT region_id, SUM(amount), COUNT(*) FROM sales GROUP BY region_id
 * Target: ≥500 K rows/sec at 1 M rows
 */
static void BM_OLAP_GroupBy_Int(benchmark::State& state) {
    const size_t row_count = static_cast<size_t>(state.range(0));
    auto data = generateSales(row_count);

    for (auto _ : state) {
        // group by region_id → {sum_amount, row_count}
        std::unordered_map<int64_t, std::pair<double, int64_t>> groups;
        groups.reserve(16);

        for (const auto& row : data) {
            auto& g = groups[row.region_id];
            g.first  += row.amount;
            g.second += 1;
        }

        benchmark::DoNotOptimize(groups.size());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(row_count));
    state.SetLabel("groupby_int");
}
BENCHMARK(BM_OLAP_GroupBy_Int)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// Case 2: BM_OLAP_WindowFunction
// ═══════════════════════════════════════════════════════════

/**
 * Sliding-window running aggregation over an ordered time series.
 * Simulates: SUM(amount) OVER (ORDER BY timestamp ROWS BETWEEN 99 PRECEDING AND CURRENT ROW)
 * Target: ≥200 K rows/sec at 1 M rows
 */
static void BM_OLAP_WindowFunction(benchmark::State& state) {
    const size_t row_count = static_cast<size_t>(state.range(0));
    const int    window_size = 100;

    auto data = generateSales(row_count);
    // Sort by timestamp to simulate ordered window input
    std::sort(data.begin(), data.end(),
              [](const SalesRow& a, const SalesRow& b) {
                  return a.timestamp < b.timestamp;
              });

    for (auto _ : state) {
        std::vector<double> running(row_count);
        double window_sum = 0.0;

        for (size_t i = 0; i < row_count; ++i) {
            window_sum += data[i].amount;
            if (static_cast<int>(i) >= window_size) {
                window_sum -= data[i - window_size].amount;
            }
            running[i] = window_sum;
        }

        benchmark::DoNotOptimize(running.back());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(row_count));
    state.SetLabel("window_func");
}
BENCHMARK(BM_OLAP_WindowFunction)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ═══════════════════════════════════════════════════════════
// Case 3: BM_OLAP_MultiJoin
// ═══════════════════════════════════════════════════════════

/**
 * Hash-join across 3 tables (orders × customers × products) with GROUP BY aggregation.
 * Simulates:
 *   SELECT c.region_id, p.category_id, SUM(o.price * o.product_id)
 *   FROM orders o
 *   JOIN customers c ON o.customer_id = c.customer_id
 *   JOIN products  p ON o.product_id  = p.product_id
 *   GROUP BY c.region_id, p.category_id
 * Target: ≥100 K rows/sec at 100 K orders
 */
static void BM_OLAP_MultiJoin(benchmark::State& state) {
    const size_t order_count    = static_cast<size_t>(state.range(0));
    const size_t customer_count = 10000;
    const size_t product_count  = 100;

    auto orders    = generateOrders(order_count);
    auto customers = generateCustomers(customer_count);
    auto products  = generateProducts(product_count);

    // Build lookup maps once outside the timed loop
    std::unordered_map<int64_t, const CustomerRow*> cust_map;
    cust_map.reserve(customer_count);
    for (const auto& c : customers) {
        cust_map[c.customer_id] = &c;
    }

    std::unordered_map<int64_t, const ProductRow*> prod_map;
    prod_map.reserve(product_count);
    for (const auto& p : products) {
        prod_map[p.product_id] = &p;
    }

    for (auto _ : state) {
        // JOIN + GROUP BY region_id × category_id → SUM(price)
        std::unordered_map<int64_t, double> result;
        result.reserve(64);

        for (const auto& o : orders) {
            auto cit = cust_map.find(o.customer_id);
            auto pit = prod_map.find(o.product_id);
            if (cit == cust_map.end() || pit == prod_map.end()) {
              continue;
            }

            // Composite key: region*10 + category (both small ranges)
            int64_t key = cit->second->region_id * 10 + pit->second->category_id;
            result[key] += o.price;
        }

        benchmark::DoNotOptimize(result.size());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(order_count));
    state.SetLabel("multi_join");
}
BENCHMARK(BM_OLAP_MultiJoin)
    ->Arg(10000)
    ->Arg(100000);

// ═══════════════════════════════════════════════════════════
// Case 4: BM_OLAP_TopN_Sorted
// ═══════════════════════════════════════════════════════════

/**
 * Top-N query: per-group aggregation followed by ORDER BY DESC LIMIT N.
 * Simulates:
 *   SELECT product_id, SUM(amount) AS total
 *   FROM sales
 *   GROUP BY product_id
 *   ORDER BY total DESC
 *   LIMIT 10
 * Target: ≥300 K rows/sec at 1 M rows
 */
static void BM_OLAP_TopN_Sorted(benchmark::State& state) {
    const size_t row_count = static_cast<size_t>(state.range(0));
    const int    top_n     = 10;

    auto data = generateSales(row_count);

    for (auto _ : state) {
        // Step 1: GROUP BY product_id → SUM(amount)
        std::unordered_map<int64_t, double> groups;
        groups.reserve(128);
        for (const auto& row : data) {
            groups[row.product_id] += row.amount;
        }

        // Step 2: heap-based Top-N extraction (min-heap of size N)
        using Pair = std::pair<double, int64_t>;
        std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> heap;

        for (const auto& [pid, total] : groups) {
            heap.push({total, pid});
            if (static_cast<int>(heap.size()) > top_n) {
                heap.pop();
            }
        }

        benchmark::DoNotOptimize(heap.size());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(row_count));
    state.SetLabel("topn_sorted");
}
BENCHMARK(BM_OLAP_TopN_Sorted)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

BENCHMARK_MAIN();
