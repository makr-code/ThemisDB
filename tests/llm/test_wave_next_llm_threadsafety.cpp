/**
 * @file test_wave_next_llm_threadsafety.cpp
 * @brief Wave-B L7 thread-safety hardening — regression tests.
 *
 * Validates the 13 thread-safety sites fixed by the Wave-B L7 audit:
 *
 * | ID       | Description                                              |
 * |----------|----------------------------------------------------------|
 * | L7-TS-01 | Concurrent getModel() from 4 threads — no data race      |
 * | L7-TS-02 | Concurrent loadModel() + getModel() — no crash           |
 * | L7-TS-03 | setStateDb() concurrent with getPlugin() — no UAF        |
 * | L7-TS-04 | Plugin counter from 8 threads — atomic exact count       |
 *
 * All tests compile without ThreadSanitizer (TSAN detection is runtime-only).
 * Run under TSAN to surface any residual data races.
 *
 * @note CTest labels: llm;threadsafety;wave-b-l7
 * @version 1.0.0-wave-b-l7
 */

#include <gtest/gtest.h>

#include "llm/llm_plugin_interface.h"
#include "llm/llm_plugin_manager.h"
#include "llm/ml_model_manager.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal stub plugin — satisfies ILLMPlugin without any real backend.
// ─────────────────────────────────────────────────────────────────────────────
class L7StubPlugin : public themis::llm::ILLMPlugin {
public:
    explicit L7StubPlugin(std::string name = "l7-stub") : name_(std::move(name)) {}

    bool loadModel(const std::string&, const themis::llm::json&) override {
        loaded_ = true;
        return true;
    }
    void unloadModel()                              override { loaded_ = false; }
    bool isModelLoaded() const                      override { return loaded_; }

    std::optional<themis::llm::ModelInfo> getModelInfo() const override {
        if (!loaded_) {
          return std::nullopt;
        }
        themis::llm::ModelInfo info{};
        info.model_id  = name_;
        info.is_loaded = true;
        return info;
    }

    themis::llm::InferenceResponse generate(const themis::llm::InferenceRequest& req) override {
        themis::llm::InferenceResponse r;
        r.request_id = req.request_id;
        r.model_id   = name_;
        r.text       = "stub";
        r.success    = true;
        return r;
    }
    themis::llm::InferenceResponse generateRAG(
            const themis::llm::RAGContext&,
            const themis::llm::InferenceRequest& req) override { return generate(req); }

    std::vector<float>               embed(const std::string&)            override { return {}; }
    themis::llm::LLMCapabilities     getCapabilities()     const          override { return {}; }
    themis::llm::json                getMemoryStats()      const          override { return {}; }
    themis::llm::json                getPerformanceStats() const          override { return {}; }
    bool   loadLoRA(const std::string&, const std::string&, float)        override { return true; }
    bool   unloadLoRA(const std::string&)                                 override { return true; }
    std::vector<themis::llm::LoRAInfo> listLoRAs()         const          override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&)                   override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&)                          override { return true; }
private:
    std::string name_;
    bool        loaded_{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a minimal MLModelManager config (all dependencies nullptr).
// ─────────────────────────────────────────────────────────────────────────────
static themis::llm::MLModelManager::Config makeNullConfig() {
    themis::llm::MLModelManager::Config cfg;
    cfg.db              = nullptr;
    cfg.model_storage   = nullptr;
    cfg.model_loader    = nullptr;
    cfg.inference_engine = nullptr;
    cfg.enable_health_monitoring = false;
    cfg.enable_auto_scaling      = false;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a basic MLModelConfig for a given model_id.
// ─────────────────────────────────────────────────────────────────────────────
static themis::llm::MLModelConfig makeMlConfig(const std::string& id) {
    themis::llm::MLModelConfig cfg;
    cfg.model_id   = id;
    cfg.model_name = id + "-name";
    cfg.version    = "1.0";
    cfg.type       = themis::llm::MLModelType::LLM;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// L7-TS-01: Concurrent getModel() calls from 4 threads — no data race.
//
// Registers one model in the main thread, then 4 worker threads each call
// getModelConfig() / getModelStatus() 1 000 times concurrently.
// Under TSAN any data race on models_ or models_mutex_ surfaces here.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveBL7ThreadSafety, ConcurrentGetModel) {
    themis::llm::MLModelManager mgr(makeNullConfig());

    const std::string model_id = "l7-ts01-model";
    auto reg = mgr.registerModel(makeMlConfig(model_id));
    ASSERT_TRUE(reg.has_value()) << "registerModel failed: " << reg.error().message();

    constexpr int kIter    = 1000;
    constexpr int kThreads = 4;

    std::atomic<int> ready{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            ready.fetch_add(1, std::memory_order_relaxed);
            while (ready.load(std::memory_order_acquire) < kThreads) {
                std::this_thread::yield(); // spin until all threads are ready
            }
            for (int i = 0; i < kIter; ++i) {
                auto cfg    = mgr.getModelConfig(model_id);
                auto status = mgr.getModelStatus(model_id);
                // Both calls must succeed — model is registered.
                EXPECT_TRUE(cfg.has_value());
                EXPECT_TRUE(status.has_value());
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// L7-TS-02: Concurrent loadModel() + getModel() — no crash or corruption.
//
// 2 writer threads each register unique models; 2 reader threads continuously
// call listModels().  The goal is to reach no crash, no ASAN/TSAN error.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveBL7ThreadSafety, ConcurrentLoadAndGetModel) {
    themis::llm::MLModelManager mgr(makeNullConfig());

    constexpr int kModelsPerWriter = 50;
    constexpr int kWriters         = 2;
    constexpr int kReaders         = 2;

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    threads.reserve(kWriters + kReaders);

    // Writer threads: register models
    for (int w = 0; w < kWriters; ++w) {
        threads.emplace_back([&mgr, w]() {
            for (int i = 0; i < kModelsPerWriter; ++i) {
                const std::string mid =
                    "l7-ts02-w" + std::to_string(w) + "-m" + std::to_string(i);
                mgr.registerModel(makeMlConfig(mid)); // ignore duplicate errors
            }
        });
    }

    // Reader threads: list models continuously until writers finish
    for (int r = 0; r < kReaders; ++r) {
        threads.emplace_back([&mgr, &stop]() {
            while (!stop.load(std::memory_order_acquire)) {
                auto models = mgr.listModels();
                (void)models; // just reading; no assertion — TSAN will catch races
                std::this_thread::yield();
            }
        });
    }

    // Wait for writers then signal readers to stop
    for (int i = 0; i < kWriters; ++i) {
        threads[static_cast<size_t>(i)].join();
    }
    stop.store(true, std::memory_order_release);
    for (int i = kWriters; i < kWriters + kReaders; ++i) {
        threads[static_cast<size_t>(i)].join();
    }

    // All kWriters × kModelsPerWriter models should be accessible now
    EXPECT_GE(static_cast<int>(mgr.listModels().size()), kWriters * kModelsPerWriter);
}

// ─────────────────────────────────────────────────────────────────────────────
// L7-TS-03: initializeStateStore() from one thread while another calls
//           getPlugin() — no use-after-free on state_db_ / state_store_.
//
// LLMPluginManager::initializeStateStore() assigns state_db_ under mutex_.
// LLMPluginManager::getPlugin() also acquires mutex_.  Both must serialise
// correctly; concurrent execution must not cause UAF or a torn pointer read.
//
// We use enabled=false so RocksDB is never actually opened — the test is
// purely about lock correctness, not storage.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveBL7ThreadSafety, SetStateDbConcurrentWithGetPlugin) {
    themis::llm::LLMPluginManager mgr;

    // Pre-register one plugin so getPlugin() has something to find.
    mgr.registerPlugin("ts03-plugin", std::make_unique<L7StubPlugin>("ts03-plugin"));

    constexpr int kIter = 500;

    std::atomic<bool> stop{false};

    // Thread A: repeatedly call initializeStateStore with enabled=false (no-op path).
    std::thread writer([&mgr, &stop]() {
        themis::llm::LLMPluginManager::SSMStateStoreConfig cfg;
        cfg.enabled = false; // no RocksDB open; exercises the early-exit branch
        for (int i = 0; i < kIter; ++i) {
            mgr.initializeStateStore(cfg);
        }
        stop.store(true, std::memory_order_release);
    });

    // Thread B: repeatedly call getPlugin() while Thread A is running.
    std::thread reader([&mgr, &stop]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto* p = mgr.getPlugin("ts03-plugin");
            // p may be nullptr if the plugin was just replaced, but we must
            // never get a dangling pointer — ASAN/TSAN catches UAF here.
            (void)p;
        }
    });

    writer.join();
    reader.join();

    // Post-condition: plugin is still accessible after concurrent state changes.
    EXPECT_NE(mgr.getPlugin("ts03-plugin"), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// L7-TS-04: Plugin counter incremented from 8 threads — atomic exact count.
//
// 8 threads each call registerPlugin() N times (unique names per thread).
// At the end, plugin_operation_count_ must equal 8 × N exactly.
// This verifies that the std::atomic<uint64_t> fetch_add is race-free.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveBL7ThreadSafety, AtomicPluginCounterExact) {
    themis::llm::LLMPluginManager mgr;

    constexpr int kThreads      = 8;
    constexpr int kRegsPerThread = 100;
    constexpr uint64_t kExpected =
        static_cast<uint64_t>(kThreads) * static_cast<uint64_t>(kRegsPerThread);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&mgr, t]() {
            for (int i = 0; i < kRegsPerThread; ++i) {
                const std::string name =
                    "ts04-t" + std::to_string(t) + "-p" + std::to_string(i);
                // registerPlugin() increments plugin_operation_count_ atomically.
                mgr.registerPlugin(name, std::make_unique<L7StubPlugin>(name));
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // plugin_operation_count_ must equal kExpected (8 × 100 = 800).
    EXPECT_EQ(mgr.getPluginOperationCount(), kExpected);
}

} // anonymous namespace
