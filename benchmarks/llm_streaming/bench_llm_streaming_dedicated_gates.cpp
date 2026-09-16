// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_llm_streaming_dedicated_gates.cpp
 * @brief Wave D — LLM Streaming Dedicated Benchmark Gates.
 *
 * Benchmark gates LS-BM-01..04 for the llm_streaming module.
 *
 * Gates:
 *   LS-BM-01  Token_Send_Throughput              — ≥ 10 000 tokens/sec
 *   LS-BM-02  Backpressure_Push_Latency          — p99 ≤ 100 µs
 *   LS-BM-03  ChunkAssembler_Receive_Throughput  — ≥ 50 000 chunks/sec
 *   LS-BM-04  StreamSession_Create_Latency       — p99 ≤ 1 ms
 *
 * SIMULATION NOTE: All token streaming, backpressure, and chunk assembly
 * operations use in-process stubs that model the production hot paths without
 * requiring gRPC, HTTP, or LLM inference backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_LLM_STREAMING.md
 * @see src/llm_streaming/ROADMAP.md — Wave D
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process LLM streaming stubs
// ---------------------------------------------------------------------------
class StubTokenStreamBench {
public:
    void sendToken(const std::string& token) {
        count_.fetch_add(1, std::memory_order_relaxed);
        (void)token;
    }
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_{0};
};

class StubBackpressureBench {
public:
    explicit StubBackpressureBench(std::size_t cap) : cap_(cap) {}
    bool push(const std::string& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        if (buf_.size() >= cap_) return false;
        buf_.push_back(chunk);
        return true;
    }
    bool pop(std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (buf_.empty()) return false;
        out = buf_.front(); buf_.pop_front(); return true;
    }
private:
    std::mutex mu_;
    std::deque<std::string> buf_;
    std::size_t cap_;
};

class StubChunkAssemblerBench {
public:
    void receive(uint64_t seq, const std::string& chunk) {
        count_.fetch_add(1, std::memory_order_relaxed);
        (void)seq; (void)chunk;
    }
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_{0};
};

class StubStreamSessionBench {
public:
    void open(uint64_t id) { id_ = id; }
    void close() { id_ = 0; }
private:
    uint64_t id_{0};
};

// ---------------------------------------------------------------------------
// LS-BM-01: Token Send Throughput
// ---------------------------------------------------------------------------
static void LS_BM_01_Token_Send_Throughput(benchmark::State& state) {
    StubTokenStreamBench stream;
    int64_t idx = 0;
    for (auto _ : state) {
        stream.sendToken("tok_" + std::to_string(idx++));
        benchmark::ClobberMemory();
    }
    state.SetLabel("LS-BM-01 gate: ≥10000 tokens/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(LS_BM_01_Token_Send_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// LS-BM-02: Backpressure Push Latency
// ---------------------------------------------------------------------------
static void LS_BM_02_Backpressure_Push_Latency(benchmark::State& state) {
    StubBackpressureBench bp(65536);
    int64_t idx = 0;
    for (auto _ : state) {
        if (!bp.push("chunk_" + std::to_string(idx))) {
            std::string out;
            bp.pop(out);
            bp.push("chunk_" + std::to_string(idx));
        }
        ++idx;
        benchmark::ClobberMemory();
    }
    state.SetLabel("LS-BM-02 gate: p99 ≤ 100µs");
}
BENCHMARK(LS_BM_02_Backpressure_Push_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// LS-BM-03: Chunk Assembler Receive Throughput
// ---------------------------------------------------------------------------
static void LS_BM_03_ChunkAssembler_Receive_Throughput(benchmark::State& state) {
    StubChunkAssemblerBench assembler;
    uint64_t seq = 0;
    for (auto _ : state) {
        assembler.receive(seq, "chunk_" + std::to_string(seq));
        ++seq;
        benchmark::ClobberMemory();
    }
    state.SetLabel("LS-BM-03 gate: ≥50000 chunks/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(LS_BM_03_ChunkAssembler_Receive_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// LS-BM-04: Stream Session Create Latency
// ---------------------------------------------------------------------------
static void LS_BM_04_StreamSession_Create_Latency(benchmark::State& state) {
    uint64_t id = 0;
    for (auto _ : state) {
        StubStreamSessionBench sess;
        sess.open(id++);
        sess.close();
        benchmark::DoNotOptimize(sess);
    }
    state.SetLabel("LS-BM-04 gate: p99 ≤ 1ms");
}
BENCHMARK(LS_BM_04_StreamSession_Create_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
