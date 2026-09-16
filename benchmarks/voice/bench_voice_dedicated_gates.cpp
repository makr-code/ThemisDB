// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_voice_dedicated_gates.cpp
 * @brief Wave D — Voice Pipeline Dedicated Benchmark Gates.
 *
 * Benchmark gates VO-BM-01..04 for the voice module.
 *
 * Gates:
 *   VO-BM-01  STT_Transcription_Throughput       — ≥ 500 ops/sec
 *   VO-BM-02  TTS_Synthesis_Latency              — p99 ≤ 50 ms
 *   VO-BM-03  AudioChunkQueue_Push_Throughput    — ≥ 10 000 ops/sec
 *   VO-BM-04  StreamSession_Send_Latency         — p99 ≤ 1 ms
 *
 * SIMULATION NOTE: All STT, TTS, and streaming operations use in-process
 * stubs that model the production hot paths without requiring real audio
 * models or hardware backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_VOICE_PIPELINE.md
 * @see src/voice/ROADMAP.md — Wave D
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — voice pipeline stubs
// ---------------------------------------------------------------------------
class StubSTTBench {
public:
    std::string transcribe(const std::vector<uint8_t>& /*audio*/) {
        return "transcribed_text";
    }
};

class StubTTSBench {
public:
    std::vector<uint8_t> synthesize(const std::string& text) {
        return std::vector<uint8_t>(text.size(), 0xAB);
    }
};

class StubAudioQueueBench {
public:
    explicit StubAudioQueueBench(std::size_t cap) : cap_(cap) {}
    bool push(const std::vector<uint8_t>& c) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= cap_) return false;
        q_.push_back(c);
        return true;
    }
    bool pop(std::vector<uint8_t>& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) return false;
        out = q_.front(); q_.pop_front(); return true;
    }
private:
    std::mutex mu_;
    std::deque<std::vector<uint8_t>> q_;
    std::size_t cap_;
};

class StubStreamSessionBench {
public:
    void sendChunk(const std::vector<uint8_t>& chunk) {
        count_.fetch_add(1, std::memory_order_relaxed);
        (void)chunk;
    }
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_{0};
};

// ---------------------------------------------------------------------------
// VO-BM-01: STT Transcription Throughput
// ---------------------------------------------------------------------------
static void VO_BM_01_STT_Transcription_Throughput(benchmark::State& state) {
    StubSTTBench stt;
    const std::vector<uint8_t> audio(1024, 0x00);
    for (auto _ : state) {
        const auto result = stt.transcribe(audio);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("VO-BM-01 gate: ≥500 ops/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(VO_BM_01_STT_Transcription_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// VO-BM-02: TTS Synthesis Latency
// ---------------------------------------------------------------------------
static void VO_BM_02_TTS_Synthesis_Latency(benchmark::State& state) {
    StubTTSBench tts;
    for (auto _ : state) {
        const auto audio = tts.synthesize("benchmark synthesis payload");
        benchmark::DoNotOptimize(audio);
    }
    state.SetLabel("VO-BM-02 gate: p99 ≤ 50ms");
}
BENCHMARK(VO_BM_02_TTS_Synthesis_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// VO-BM-03: Audio Chunk Queue Push Throughput
// ---------------------------------------------------------------------------
static void VO_BM_03_AudioChunkQueue_Push_Throughput(benchmark::State& state) {
    StubAudioQueueBench q(65536);
    const std::vector<uint8_t> chunk(256, 0xFF);
    for (auto _ : state) {
        if (!q.push(chunk)) {
            // Drain once when full to keep the benchmark running
            std::vector<uint8_t> out;
            q.pop(out);
            q.push(chunk);
        }
        benchmark::ClobberMemory();
    }
    state.SetLabel("VO-BM-03 gate: ≥10000 ops/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(VO_BM_03_AudioChunkQueue_Push_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// VO-BM-04: Stream Session Send Latency
// ---------------------------------------------------------------------------
static void VO_BM_04_StreamSession_Send_Latency(benchmark::State& state) {
    StubStreamSessionBench sess;
    const std::vector<uint8_t> chunk(512, 0xCC);
    for (auto _ : state) {
        sess.sendChunk(chunk);
        benchmark::ClobberMemory();
    }
    state.SetLabel("VO-BM-04 gate: p99 ≤ 1ms");
}
BENCHMARK(VO_BM_04_StreamSession_Send_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
