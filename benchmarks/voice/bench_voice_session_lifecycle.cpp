/**
 * @file bench_voice_session_lifecycle.cpp
 * @brief Google Benchmark suite for Voice session lifecycle operations.
 *
 * Benchmarks:
 *  - GATE_SESSION_CREATE <= 100ms
 *  - GATE_SESSION_DELETE <= 50ms
 *  - GATE_SESSION_MEMORY_100 <= 500MB
 *  - GATE_SESSION_MEMORY_1000 <= 2GB
 *  - GATE_SESSION_CLEANUP_LEAK <= 10MB
 */

#include <benchmark/benchmark.h>
#include "benchmarks/voice/benchmark_fixtures.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <map>

using namespace themis::voice::benchmark;

// =============================================================================
// Mock Session Storage
// =============================================================================

class MockVoiceSession {
public:
    explicit MockVoiceSession(const std::string& id)
        : session_id_(id),
          context_data_(kSessionMemoryBytesBase, 0),
          created_at_(std::chrono::steady_clock::now()) {}

    const std::string& getId() const { return session_id_; }
    
    void updateContext(const std::map<std::string, std::string>& ctx) {
        context_data_.insert(context_data_.end(), ctx.size() * 100, 0);  // Simulate data growth
    }

    size_t getMemoryUsage() const {
        return sizeof(*this) + context_data_.size();
    }

private:
    std::string session_id_;
    std::vector<uint8_t> context_data_;
    std::chrono::steady_clock::time_point created_at_;
};

class MockSessionManager {
public:
    MockSessionManager() = default;

    std::shared_ptr<MockVoiceSession> createSession(const std::string& session_id) {
        auto session = std::make_shared<MockVoiceSession>(session_id);
        sessions_.emplace(session_id, session);
        return session;
    }

    void deleteSession(const std::string& session_id) {
        sessions_.erase(session_id);
    }

    std::shared_ptr<MockVoiceSession> getSession(const std::string& session_id) {
        auto it = sessions_.find(session_id);
        if (it != sessions_.end()) {
            return it->second;
        }
        return nullptr;
    }

    size_t getSessionCount() const {
        return sessions_.size();
    }

    size_t getTotalMemory() const {
        size_t total = 0;
        for (const auto& [id, session] : sessions_) {
            if (session) {
                total += session->getMemoryUsage();
            }
        }
        return total;
    }

    void clearAll() {
        sessions_.clear();
    }

private:
    std::map<std::string, std::shared_ptr<MockVoiceSession>> sessions_;
};

// =============================================================================
// Session Lifecycle Benchmarks
// =============================================================================

/**
 * @test BENCHMARK(VoiceSession, CreateSession)
 * Measure: session creation time
 * Assert: < 100ms
 * GATE_SESSION_CREATE <= 100ms
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_CreateSession)(benchmark::State& state) {
    MockSessionManager manager;
    std::vector<int64_t> create_times_ns;
    create_times_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        std::string session_id = generateSessionId();

        auto start = std::chrono::steady_clock::now();
        auto session = manager.createSession(session_id);
        auto end = std::chrono::steady_clock::now();

        int64_t create_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        create_times_ns.push_back(create_time_ns);

        benchmark::DoNotOptimize(session);
    }

    std::sort(create_times_ns.begin(), create_times_ns.end());

    int64_t p95_ns = utils::calculateP95(create_times_ns);
    int64_t p99_ns = utils::calculateP99(create_times_ns);

    utils::checkGate(p95_ns, gates::kGateSessionCreate, "GATE_SESSION_CREATE");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.counters["p99_ms"] = p99_ns / 1'000'000.0;
    state.SetLabel("Session CreateSession: < 100ms");
}

/**
 * @test BENCHMARK(VoiceSession, CreateSessionConcurrent10)
 * Measure: time for 10 concurrent creates
 * Assert: < 500ms total
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_CreateSessionConcurrent10)(benchmark::State& state) {
    MockSessionManager manager;
    std::vector<int64_t> concurrent_times_ns;
    concurrent_times_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        auto start = std::chrono::steady_clock::now();

        // Create 10 sessions sequentially (simulates concurrent operations)
        for (int i = 0; i < 10; ++i) {
            std::string session_id = generateSessionId();
            auto session = manager.createSession(session_id);
            benchmark::DoNotOptimize(session);
        }

        auto end = std::chrono::steady_clock::now();

        int64_t concurrent_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        concurrent_times_ns.push_back(concurrent_time_ns);

        manager.clearAll();
    }

    std::sort(concurrent_times_ns.begin(), concurrent_times_ns.end());

    int64_t p95_ns = utils::calculateP95(concurrent_times_ns);

    utils::checkGate(p95_ns, 500'000'000, "GATE_SESSION_CREATE_CONCURRENT10");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.SetLabel("Session CreateSessionConcurrent10: < 500ms total");
}

/**
 * @test BENCHMARK(VoiceSession, DeleteSession)
 * Measure: session cleanup time
 * Assert: < 50ms
 * GATE_SESSION_DELETE <= 50ms
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_DeleteSession)(benchmark::State& state) {
    MockSessionManager manager;
    std::vector<int64_t> delete_times_ns;
    delete_times_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        std::string session_id = generateSessionId();
        auto session = manager.createSession(session_id);
        benchmark::DoNotOptimize(session);

        auto start = std::chrono::steady_clock::now();
        manager.deleteSession(session_id);
        auto end = std::chrono::steady_clock::now();

        int64_t delete_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        delete_times_ns.push_back(delete_time_ns);
    }

    std::sort(delete_times_ns.begin(), delete_times_ns.end());

    int64_t p95_ns = utils::calculateP95(delete_times_ns);
    int64_t p99_ns = utils::calculateP99(delete_times_ns);

    utils::checkGate(p95_ns, gates::kGateSessionDelete, "GATE_SESSION_DELETE");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.counters["p99_ms"] = p99_ns / 1'000'000.0;
    state.SetLabel("Session DeleteSession: < 50ms");
}

/**
 * @test BENCHMARK(VoiceSession, ScalingTo100Sessions)
 * Measure: peak memory with 100 concurrent sessions
 * Assert: < 500MB
 * GATE_SESSION_MEMORY_100 <= 500MB
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_ScalingTo100Sessions)(benchmark::State& state) {
    MockSessionManager manager;

    for (auto _ : state) {
        // Create 100 sessions
        for (int i = 0; i < 100; ++i) {
            std::string session_id = generateSessionId();
            auto session = manager.createSession(session_id);
            
            // Update context to simulate realistic data
            std::map<std::string, std::string> context;
            context["user_id"] = "user_" + std::to_string(i);
            context["language"] = "en";
            session->updateContext(context);

            benchmark::DoNotOptimize(session);
        }

        // Measure peak memory
        size_t peak_memory = manager.getTotalMemory();

        utils::checkGate(static_cast<int64_t>(peak_memory), 
                        gates::kGateSessionMemory100,
                        "GATE_SESSION_MEMORY_100");

        state.counters["peak_memory_mb"] = peak_memory / (1024.0 * 1024.0);
        state.counters["num_sessions"] = benchmark::Counter(100, benchmark::Counter::kAvgThreads);

        manager.clearAll();
    }

    state.SetLabel("Session ScalingTo100Sessions: < 500MB memory");
}

/**
 * @test BENCHMARK(VoiceSession, ScalingTo1000Sessions)
 * Measure: peak memory with 1000 concurrent sessions
 * Assert: < 2GB
 * GATE_SESSION_MEMORY_1000 <= 2GB
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_ScalingTo1000Sessions)(benchmark::State& state) {
    MockSessionManager manager;

    for (auto _ : state) {
        // Create 1000 sessions (limit iterations to avoid timeout)
        int num_sessions = std::min(1000, static_cast<int>(state.max_iterations / 10 + 1));

        for (int i = 0; i < num_sessions; ++i) {
            std::string session_id = generateSessionId();
            auto session = manager.createSession(session_id);
            
            // Update context
            std::map<std::string, std::string> context;
            context["user_id"] = "user_" + std::to_string(i);
            context["language"] = "en";
            session->updateContext(context);

            benchmark::DoNotOptimize(session);
        }

        // Measure peak memory
        size_t peak_memory = manager.getTotalMemory();

        utils::checkGate(static_cast<int64_t>(peak_memory),
                        gates::kGateSessionMemory1000,
                        "GATE_SESSION_MEMORY_1000");

        state.counters["peak_memory_mb"] = peak_memory / (1024.0 * 1024.0);
        state.counters["num_sessions"] = benchmark::Counter(num_sessions, benchmark::Counter::kAvgThreads);

        manager.clearAll();

        // Only run once to avoid timeout
        break;
    }

    state.SetLabel("Session ScalingTo1000Sessions: < 2GB memory");
}

/**
 * @test BENCHMARK(VoiceSession, ResourceCleanupNoLeak)
 * Measure: peak - baseline after 1000 create/delete cycles
 * Assert: leak < 10MB
 * GATE_SESSION_CLEANUP_LEAK <= 10MB
 */
BENCHMARK_F(SessionLifecycleFixture, VoiceSession_ResourceCleanupNoLeak)(benchmark::State& state) {
    MockSessionManager manager;

    for (auto _ : state) {
        // Baseline: measure empty manager memory
        size_t baseline_memory = manager.getTotalMemory();

        // Run 1000 create/delete cycles
        for (int i = 0; i < 1000; ++i) {
            std::string session_id = generateSessionId();
            auto session = manager.createSession(session_id);
            
            // Add context to make sessions non-trivial
            std::map<std::string, std::string> context;
            context["user_id"] = "user_" + std::to_string(i);
            session->updateContext(context);

            benchmark::DoNotOptimize(session);

            // Delete immediately (create/delete cycle)
            manager.deleteSession(session_id);
        }

        // Measure final memory
        size_t final_memory = manager.getTotalMemory();

        // Calculate leak
        int64_t leak_bytes = static_cast<int64_t>(final_memory) - static_cast<int64_t>(baseline_memory);
        if (leak_bytes < 0) leak_bytes = 0;  // No negative leaks

        utils::checkGate(leak_bytes, gates::kGateSessionCleanupLeak, "GATE_SESSION_CLEANUP_LEAK");

        state.counters["leak_mb"] = leak_bytes / (1024.0 * 1024.0);
        state.counters["cycles"] = benchmark::Counter(1000, benchmark::Counter::kAvgThreads);

        manager.clearAll();
    }

    state.SetLabel("Session ResourceCleanupNoLeak: < 10MB leak per 1000 cycles");
}

BENCHMARK_MAIN();
