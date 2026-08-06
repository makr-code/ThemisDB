/*
 * ThemisDB | File: bench_process_concurrency_gates.cpp | Version: 1.0.0
 * Phase 5: Process Module Performance & Hardening
 * 
 * Concurrency Performance Gates (CP):
 * | Gate ID | Metric                              | Target       |
 * |---------|-------------------------------------|--------------|
 * | CP-01   | Concurrent CRUD Ops (100 models)    | ≥ 50k ops/s  |
 * | CP-02   | Concurrent CRUD Ops (1k models)     | ≥ 40k ops/s  |
 * | CP-03   | Concurrent Import (100 BPMN files)  | ≥ 20k ops/s  |
 * | CP-04   | Concurrent Export (100 models)      | ≥ 15k ops/s  |
 * | CP-05   | Concurrent Linking (100 models)     | ≥ 10k ops/s  |
 * | CP-06   | Concurrent Retrieval (1k models)    | ≥ 30k ops/s  |
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <sstream>
#include <iostream>

namespace themis::process::benchmark {

// ============================================================================
// Constants and Configuration
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kMediumDatasetSize = 1000;
constexpr int kNumThreads = 4;

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

/**
 * @brief Simulated ProcessModel for benchmarking
 */
struct SimProcessModel {
    std::string id;
    std::string name;
    std::string description;
    std::string state;  // DRAFT, ACTIVE, DEPRECATED
    int revision{0};
    int64_t created_ms{0};
    int64_t modified_ms{0};
};

/**
 * @brief Thread-safe model store simulation
 */
class ConcurrentModelStore {
private:
    std::mutex mu_;
    std::vector<SimProcessModel> models_;
    int64_t op_counter_{0};

public:
    ConcurrentModelStore() = default;
    ~ConcurrentModelStore() = default;

    // Simulated Create operation
    bool create(const SimProcessModel& model) {
        std::lock_guard<std::mutex> lock(mu_);
        models_.push_back(model);
        op_counter_++;
        return true;
    }

    // Simulated Read operation
    bool read(const std::string& id, SimProcessModel& out) {
        std::lock_guard<std::mutex> lock(mu_);
        for (const auto& m : models_) {
            if (m.id == id) {
                out = m;
                op_counter_++;
                return true;
            }
        }
        return false;
    }

    // Simulated Update operation
    bool update(const std::string& id, const SimProcessModel& model) {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& m : models_) {
            if (m.id == id) {
                m = model;
                m.revision++;
                m.modified_ms = std::chrono::system_clock::now().time_since_epoch().count();
                op_counter_++;
                return true;
            }
        }
        return false;
    }

    // Simulated Delete operation
    bool delete_model(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = std::find_if(models_.begin(), models_.end(),
                               [&id](const SimProcessModel& m) { return m.id == id; });
        if (it != models_.end()) {
            models_.erase(it);
            op_counter_++;
            return true;
        }
        return false;
    }

    // Simulated List operation
    std::vector<SimProcessModel> list() {
        std::lock_guard<std::mutex> lock(mu_);
        op_counter_++;
        return models_;
    }

    int64_t op_counter() const { return op_counter_; }
    size_t size() const { return models_.size(); }

    void clear() {
        std::lock_guard<std::mutex> lock(mu_);
        models_.clear();
        op_counter_ = 0;
    }
};

/**
 * @brief Generate realistic BPMN XML content
 */
static std::string generateBpmnXml(const std::string& model_name) {
    return R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <bpmn:process id=")" + model_name + R"(" name=")" + model_name + R"(">
    <bpmn:startEvent id="start_)" + model_name + R"(" name="Start"/>
    <bpmn:task id="task1_)" + model_name + R"(" name="Task 1"/>
    <bpmn:task id="task2_)" + model_name + R"(" name="Task 2"/>
    <bpmn:exclusiveGateway id="gateway_)" + model_name + R"(" name="Decision"/>
    <bpmn:endEvent id="end_)" + model_name + R"(" name="End"/>
  </bpmn:process>
</bpmn:definitions>)";
}

/**
 * @brief Simulate BPMN import operation
 */
static SimProcessModel simulateBpmnImport(const std::string& xml_content,
                                          int64_t now_ms) {
    SimProcessModel m;
    m.id = "proc_" + std::to_string(std::hash<std::string>{}(xml_content));
    m.name = "BpmnModel_" + m.id.substr(5, 8);
    m.description = "Imported from BPMN";
    m.state = "DRAFT";
    m.revision = 1;
    m.created_ms = now_ms;
    m.modified_ms = now_ms;
    return m;
}

/**
 * @brief Simulate BPMN export operation
 */
static std::string simulateBpmnExport(const SimProcessModel& model) {
    return generateBpmnXml(model.name);
}

/**
 * @brief Simulate linking operation
 */
static bool simulateLink(const SimProcessModel& source,
                        const SimProcessModel& target) {
    // Check for basic linking validity
    return !source.id.empty() && !target.id.empty() && source.id != target.id;
}

// ============================================================================
// CP-01: Concurrent CRUD Ops (Small Dataset - 100 models)
// ============================================================================

static void BM_CP01_ConcurrentCrud_Small(benchmark::State& state) {
    const int num_models = kSmallDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    // Pre-populate store
    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    for (int i = 0; i < num_models; ++i) {
        SimProcessModel m;
        m.id = "model_" + std::to_string(i);
        m.name = "Model_" + std::to_string(i);
        m.state = "ACTIVE";
        m.created_ms = now_ms;
        m.modified_ms = now_ms;
        store->create(m);
    }

    int64_t ops_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        // Distribute work across threads
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_ops(thread_count, 0);

        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_ops, t, num_models]() {
                std::mt19937 gen(kCanonicalRngSeed + t);
                std::uniform_int_distribution<> op_dist(0, 3);  // 0=R, 1=U, 2=D, 3=C
                std::uniform_int_distribution<> model_dist(0, num_models - 1);

                for (int i = 0; i < 100; ++i) {
                    int op = op_dist(gen);
                    int model_idx = model_dist(gen);
                    std::string model_id = "model_" + std::to_string(model_idx);

                    switch (op) {
                        case 0: {  // Read
                            SimProcessModel m;
                            store->read(model_id, m);
                            thread_ops[t]++;
                            break;
                        }
                        case 1: {  // Update
                            SimProcessModel m;
                            if (store->read(model_id, m)) {
                                m.revision++;
                                store->update(model_id, m);
                                thread_ops[t]++;
                            }
                            break;
                        }
                        case 2: {  // Delete
                            store->delete_model(model_id);
                            thread_ops[t]++;
                            break;
                        }
                        case 3: {  // Create
                            SimProcessModel m;
                            m.id = "model_" + std::to_string(model_idx) + "_new";
                            m.name = "New_Model";
                            store->create(m);
                            thread_ops[t]++;
                            break;
                        }
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t ops : thread_ops) {
            ops_done += ops;
        }
    }

    state.SetItemsProcessed(ops_done);
    state.counters["ops_per_sec"] = benchmark::Counter(ops_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP01_ConcurrentCrud_Small)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// CP-02: Concurrent CRUD Ops (Medium Dataset - 1k models)
// ============================================================================

static void BM_CP02_ConcurrentCrud_Medium(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    // Pre-populate store
    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    for (int i = 0; i < num_models; ++i) {
        SimProcessModel m;
        m.id = "model_" + std::to_string(i);
        m.name = "Model_" + std::to_string(i);
        m.state = "ACTIVE";
        m.created_ms = now_ms;
        m.modified_ms = now_ms;
        store->create(m);
    }

    int64_t ops_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_ops(thread_count, 0);
        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_ops, t, num_models]() {
                std::mt19937 gen(kCanonicalRngSeed + t);
                std::uniform_int_distribution<> op_dist(0, 3);
                std::uniform_int_distribution<> model_dist(0, num_models - 1);

                for (int i = 0; i < 100; ++i) {
                    int op = op_dist(gen);
                    int model_idx = model_dist(gen);
                    std::string model_id = "model_" + std::to_string(model_idx);

                    switch (op) {
                        case 0: {
                            SimProcessModel m;
                            store->read(model_id, m);
                            thread_ops[t]++;
                            break;
                        }
                        case 1: {
                            SimProcessModel m;
                            if (store->read(model_id, m)) {
                                m.revision++;
                                store->update(model_id, m);
                                thread_ops[t]++;
                            }
                            break;
                        }
                        case 2: {
                            store->delete_model(model_id);
                            thread_ops[t]++;
                            break;
                        }
                        case 3: {
                            SimProcessModel m;
                            m.id = "model_" + std::to_string(model_idx) + "_new";
                            m.name = "New_Model";
                            store->create(m);
                            thread_ops[t]++;
                            break;
                        }
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t ops : thread_ops) {
            ops_done += ops;
        }
    }

    state.SetItemsProcessed(ops_done);
    state.counters["ops_per_sec"] = benchmark::Counter(ops_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP02_ConcurrentCrud_Medium)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// CP-03: Concurrent Import (100 BPMN files)
// ============================================================================

static void BM_CP03_ConcurrentImport_Bpmn(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    int64_t imports_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_imports(thread_count, 0);
        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_imports, t, num_files]() {
                int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

                for (int i = t; i < num_files; i += kNumThreads) {
                    std::string xml = generateBpmnXml("model_" + std::to_string(i));
                    SimProcessModel model = simulateBpmnImport(xml, now_ms);
                    store->create(model);
                    thread_imports[t]++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t imports : thread_imports) {
            imports_done += imports;
        }
    }

    state.SetItemsProcessed(imports_done);
    state.counters["imports_per_sec"] = benchmark::Counter(imports_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP03_ConcurrentImport_Bpmn)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// CP-04: Concurrent Export (100 models)
// ============================================================================

static void BM_CP04_ConcurrentExport(benchmark::State& state) {
    const int num_models = kSmallDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    for (int i = 0; i < num_models; ++i) {
        SimProcessModel m;
        m.id = "model_" + std::to_string(i);
        m.name = "Model_" + std::to_string(i);
        m.state = "ACTIVE";
        m.created_ms = now_ms;
        m.modified_ms = now_ms;
        store->create(m);
    }

    int64_t exports_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_exports(thread_count, 0);
        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_exports, t, num_models]() {
                for (int i = t; i < num_models; i += kNumThreads) {
                    std::string model_id = "model_" + std::to_string(i);
                    SimProcessModel m;
                    if (store->read(model_id, m)) {
                        std::string xml = simulateBpmnExport(m);
                        benchmark::DoNotOptimize(xml);
                        thread_exports[t]++;
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t exports : thread_exports) {
            exports_done += exports;
        }
    }

    state.SetItemsProcessed(exports_done);
    state.counters["exports_per_sec"] = benchmark::Counter(exports_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP04_ConcurrentExport)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// CP-05: Concurrent Linking (100 models)
// ============================================================================

static void BM_CP05_ConcurrentLinking(benchmark::State& state) {
    const int num_models = kSmallDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    for (int i = 0; i < num_models; ++i) {
        SimProcessModel m;
        m.id = "model_" + std::to_string(i);
        m.name = "Model_" + std::to_string(i);
        m.state = "ACTIVE";
        m.created_ms = now_ms;
        m.modified_ms = now_ms;
        store->create(m);
    }

    int64_t links_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_links(thread_count, 0);
        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_links, t, num_models]() {
                std::mt19937 gen(kCanonicalRngSeed + t);
                std::uniform_int_distribution<> model_dist(0, num_models - 1);

                for (int link_idx = 0; link_idx < 50; ++link_idx) {
                    int source_idx = model_dist(gen);
                    int target_idx = model_dist(gen);

                    if (source_idx != target_idx) {
                        SimProcessModel source, target;
                        std::string src_id = "model_" + std::to_string(source_idx);
                        std::string tgt_id = "model_" + std::to_string(target_idx);

                        if (store->read(src_id, source) && store->read(tgt_id, target)) {
                            if (simulateLink(source, target)) {
                                thread_links[t]++;
                            }
                        }
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t links : thread_links) {
            links_done += links;
        }
    }

    state.SetItemsProcessed(links_done);
    state.counters["links_per_sec"] = benchmark::Counter(links_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP05_ConcurrentLinking)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// CP-06: Concurrent Retrieval (1k models)
// ============================================================================

static void BM_CP06_ConcurrentRetrieval_Medium(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    const int thread_count = kNumThreads;
    auto store = std::make_unique<ConcurrentModelStore>();

    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    for (int i = 0; i < num_models; ++i) {
        SimProcessModel m;
        m.id = "model_" + std::to_string(i);
        m.name = "Model_" + std::to_string(i);
        m.state = "ACTIVE";
        m.created_ms = now_ms;
        m.modified_ms = now_ms;
        store->create(m);
    }

    int64_t queries_done = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_queries(thread_count, 0);
        state.ResumeTiming();

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&store, &thread_queries, t, num_models]() {
                std::mt19937 gen(kCanonicalRngSeed + t);
                std::uniform_int_distribution<> model_dist(0, num_models - 1);

                for (int q = 0; q < 100; ++q) {
                    int model_idx = model_dist(gen);
                    std::string model_id = "model_" + std::to_string(model_idx);
                    SimProcessModel m;
                    if (store->read(model_id, m)) {
                        thread_queries[t]++;
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t queries : thread_queries) {
            queries_done += queries;
        }
    }

    state.SetItemsProcessed(queries_done);
    state.counters["queries_per_sec"] = benchmark::Counter(queries_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_CP06_ConcurrentRetrieval_Medium)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();
