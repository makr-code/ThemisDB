/**
 * @file bench_voice_streaming_throughput.cpp
 * @brief Google Benchmark suite for Voice streaming throughput and latency.
 *
 * Benchmarks:
 *  - GATE_STREAM_THROUGHPUT >= 1000 chunks/sec
 *  - GATE_STREAM_LATENCY_P95 <= 50ms per chunk
 *  - GATE_STREAM_MEMORY <= 10MB per session
 *  - GATE_STREAM_RECOVERY <= 500ms
 */

#include <benchmark/benchmark.h>
#include "benchmarks/voice/benchmark_fixtures.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <thread>

using namespace themis::voice::benchmark;

// =============================================================================
// Streaming Throughput Benchmarks
// =============================================================================

/**
 * @test BENCHMARK(VoiceStreaming, ChunkThroughput)
 * Measure: N chunks through stream
 * Assert: >= 1000 chunks/sec
 * GATE_STREAM_THROUGHPUT >= 1000 chunks/sec
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_ChunkThroughput)(benchmark::State& state) {
    // Generate 100ms audio chunks (typical streaming chunk size)
    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms per chunk
    int64_t total_chunks = 0;

    for (auto _ : state) {
        // Process 100 chunks per iteration
        for (int i = 0; i < 100; ++i) {
            bool added = stream_buffer_->addChunk(chunk);
            if (!added) {
                stream_buffer_->clear();
                stream_buffer_->addChunk(chunk);
            }

            // Simulate processing by getting chunk back
            std::vector<uint8_t> retrieved;
            stream_buffer_->getChunk(retrieved);

            benchmark::DoNotOptimize(retrieved);
            ++total_chunks;
        }
    }

    // Calculate throughput: chunks per second
    // Each chunk is 100ms, so chunks per second = (total_chunks / iterations) * (1000 / 100)
    double throughput = static_cast<double>(total_chunks) / state.elapsed_real_time();

    utils::checkGate(static_cast<int64_t>(throughput * 1'000'000'000), 
                     gates::kGateStreamThroughput * 1'000'000'000,
                     "GATE_STREAM_THROUGHPUT");

    state.counters["chunks_per_sec"] = benchmark::Counter(throughput, benchmark::Counter::kIsRate);
    state.SetLabel("Streaming ChunkThroughput: >= 1000 chunks/sec");
}

/**
 * @test BENCHMARK(VoiceStreaming, ChunkLatency)
 * Measure: per-chunk latency (chunk arrival + processing)
 * Assert: p95 < 50ms per chunk
 * GATE_STREAM_LATENCY_P95 <= 50ms per chunk
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_ChunkLatency)(benchmark::State& state) {
    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms chunk
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations * 100);  // ~100 chunks per iteration

    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto start = std::chrono::steady_clock::now();
            stream_buffer_->addChunk(chunk);
            
            std::vector<uint8_t> retrieved;
            stream_buffer_->getChunk(retrieved);
            auto end = std::chrono::steady_clock::now();

            int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            latencies_ns.push_back(latency_ns);
            benchmark::DoNotOptimize(retrieved);
        }
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);

    utils::checkGate(p95_ns, gates::kGateStreamLatencyP95, "GATE_STREAM_LATENCY_P95");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.SetLabel("Streaming ChunkLatency: p95 < 50ms per chunk");
}

/**
 * @test BENCHMARK(VoiceStreaming, BufferOverhead)
 * Measure: buffer memory per session
 * Assert: < 10MB per active stream
 * GATE_STREAM_MEMORY <= 10MB per session
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_BufferOverhead)(benchmark::State& state) {
    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms chunk

    for (auto _ : state) {
        // Add 1000 chunks to stream (simulating buffered data)
        for (int i = 0; i < 1000; ++i) {
            stream_buffer_->addChunk(chunk);
            benchmark::DoNotOptimize(chunk);
        }

        // Measure memory usage
        size_t memory_bytes = stream_buffer_->getMemoryUsage();

        // Check gate
        utils::checkGate(static_cast<int64_t>(memory_bytes), 
                         gates::kGateStreamMemory,
                         "GATE_STREAM_MEMORY");

        state.counters["buffer_memory_mb"] = memory_bytes / (1024.0 * 1024.0);
        
        // Drain buffer for next iteration
        stream_buffer_->clear();
    }

    state.SetLabel("Streaming BufferOverhead: < 10MB per session");
}

/**
 * @test BENCHMARK(VoiceStreaming, MultipleStreamsThroughput)
 * Measure: total throughput for 10 concurrent streams
 * Assert: >= 5000 chunks/sec combined
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_MultipleStreamsThroughput)(benchmark::State& state) {
    // Create 10 mock stream buffers (simulating concurrent sessions)
    std::vector<std::unique_ptr<MockStreamBuffer>> streams;
    for (int i = 0; i < 10; ++i) {
        streams.push_back(std::make_unique<MockStreamBuffer>());
    }

    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms chunk
    int64_t total_chunks = 0;

    for (auto _ : state) {
        // Process 10 chunks per stream per iteration
        for (auto& stream : streams) {
            for (int i = 0; i < 10; ++i) {
                stream->addChunk(chunk);
                
                std::vector<uint8_t> retrieved;
                stream->getChunk(retrieved);

                benchmark::DoNotOptimize(retrieved);
                ++total_chunks;
            }
        }
    }

    // Calculate combined throughput
    double combined_throughput = static_cast<double>(total_chunks) / state.elapsed_real_time();

    // Gate check: 5000 chunks/sec combined
    utils::checkGate(static_cast<int64_t>(combined_throughput),
                     5000,
                     "GATE_STREAM_MULTI_THROUGHPUT");

    state.counters["total_chunks_per_sec"] = benchmark::Counter(combined_throughput, benchmark::Counter::kIsRate);
    state.SetLabel("Streaming MultipleStreamsThroughput: >= 5000 chunks/sec");
}

/**
 * @test BENCHMARK(VoiceStreaming, StreamRebalancing)
 * Measure: rebalance time for pause/resume operations
 * Assert: < 100ms rebalance time
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_StreamRebalancing)(benchmark::State& state) {
    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms chunk
    std::vector<int64_t> rebalance_times_ns;
    rebalance_times_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        // Add some chunks
        for (int i = 0; i < 10; ++i) {
            stream_buffer_->addChunk(chunk);
        }

        // Simulate pause (buffer snapshot)
        size_t paused_buffer_size = stream_buffer_->getMemoryUsage();

        // Measure rebalance time (simulated as buffer reorganization)
        auto start = std::chrono::steady_clock::now();
        
        // Simulate rebalance by draining and refilling
        stream_buffer_->clear();
        for (int i = 0; i < 10; ++i) {
            stream_buffer_->addChunk(chunk);
        }
        
        auto end = std::chrono::steady_clock::now();

        int64_t rebalance_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        rebalance_times_ns.push_back(rebalance_ns);

        benchmark::DoNotOptimize(paused_buffer_size);
    }

    std::sort(rebalance_times_ns.begin(), rebalance_times_ns.end());

    int64_t p99_rebalance_ns = utils::calculateP99(rebalance_times_ns);

    utils::checkGate(p99_rebalance_ns, 100'000'000, "GATE_STREAM_REBALANCE");

    state.counters["rebalance_p99_ms"] = p99_rebalance_ns / 1'000'000.0;
    state.SetLabel("Streaming StreamRebalancing: < 100ms rebalance time");
}

/**
 * @test BENCHMARK(VoiceStreaming, ConnectionLossRecovery)
 * Measure: time to recover from loss (reconnect latency)
 * Assert: < 500ms recovery time
 * GATE_STREAM_RECOVERY <= 500ms
 */
BENCHMARK_F(StreamingThroughputFixture, VoiceStreaming_ConnectionLossRecovery)(benchmark::State& state) {
    std::vector<uint8_t> chunk = generateAudio(100);  // 100ms chunk
    std::vector<int64_t> recovery_times_ns;
    recovery_times_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        // Setup: establish connection (add chunks)
        for (int i = 0; i < 10; ++i) {
            stream_buffer_->addChunk(chunk);
        }

        // Simulate connection loss (clear buffer)
        stream_buffer_->clear();

        // Measure recovery time (reconnect and resume streaming)
        auto start = std::chrono::steady_clock::now();
        
        // Recovery: re-establish and add first chunk
        stream_buffer_->addChunk(chunk);
        
        // Small sleep to simulate network stack recovery
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Verify connectivity by getting chunk
        std::vector<uint8_t> retrieved;
        stream_buffer_->getChunk(retrieved);
        
        auto end = std::chrono::steady_clock::now();

        int64_t recovery_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        recovery_times_ns.push_back(recovery_ns);

        benchmark::DoNotOptimize(retrieved);
    }

    std::sort(recovery_times_ns.begin(), recovery_times_ns.end());

    int64_t p99_recovery_ns = utils::calculateP99(recovery_times_ns);

    utils::checkGate(p99_recovery_ns, gates::kGateStreamRecovery, "GATE_STREAM_RECOVERY");

    state.counters["recovery_p99_ms"] = p99_recovery_ns / 1'000'000.0;
    state.SetLabel("Streaming ConnectionLossRecovery: < 500ms recovery time");
}

BENCHMARK_MAIN();
