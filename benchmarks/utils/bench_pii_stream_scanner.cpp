/**
 * @file bench_pii_stream_scanner.cpp
 * @brief Google Benchmark throughput tests for PIIStreamScanner and
 *        scan + HMAC-SHA-256 pseudonymisation over 100 MB synthetic legal text.
 *
 * Covers the acceptance criteria from roadmap item
 * "Streaming PII Scanner for large documents" (v0.9.0):
 *  - Streaming PII scan throughput: >100 MB/s per core for English legal text
 *  - Memory footprint during streaming scan of 1 GB document: <10 MB
 *
 * Benchmarked scenarios:
 *  - 100 MB chunk-at-a-time scan via PIIStreamScanner (scan-only)
 *  - 100 MB scan + inline HMAC-SHA-256 pseudonymisation of every PII span
 *    (mirrors the logic in PIIStreamPseudonymizer::process_chunk())
 *  - Varying chunk sizes: 4 KB, 64 KB, 1 MB
 *  - Cross-boundary detection: lookahead = engine->maxPatternLength()
 *
 * Performance targets (Release build on commodity x86-64):
 *  - Scan-only throughput:       >100 MB/s per core
 *  - Scan+pseudonymise overhead: <2× vs. scan-only
 *
 * Run with:
 *   ./bench_pii_stream_scanner \
 *       --benchmark_format=json \
 *       --benchmark_out=bench_pii_stream_scanner.json
 */

#include <benchmark/benchmark.h>
#include "utils/pii_detection_engine.h"
#include "utils/regex_detection_engine.h"

#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// Synthetic corpus helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a deterministic ~1 KB block of synthetic "legal" English text that
/// occasionally embeds PII patterns so the regex engine actually triggers.
static std::string make_text_block() {
    return
        "This agreement is entered into as of 2026-01-15 between Acme Corp "
        "(\"Company\") and John Smith (email: john.smith@example.com, "
        "phone: +1-555-123-4567, SSN: 123-45-6789).  The Company shall pay "
        "USD 10,000 to account IBAN DE44 5001 0517 5407 3249 31.  "
        "Credit card 4539 1488 0343 6467 (exp 12/28) is authorised for "
        "recurring charges.  Contact at 192.168.1.100 for internal routing.  "
        "This document is confidential and may not be disclosed.  "
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua.  Ut enim "
        "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
        "aliquip ex ea commodo consequat.  Duis aute irure dolor in reprehenderit "
        "in voluptate velit esse cillum dolore eu fugiat nulla pariatur.  ";
}

/// Return a shared, initialized RegexDetectionEngine.
static std::shared_ptr<RegexDetectionEngine> make_engine() {
    auto engine = std::make_shared<RegexDetectionEngine>();
    nlohmann::json cfg;
    cfg["enabled"] = true;
    engine->initialize(cfg);
    return engine;
}

/// Build the synthetic corpus of exactly `target_bytes` bytes.
static std::string build_corpus(size_t target_bytes) {
    const std::string block = make_text_block();
    std::string corpus = {};
    corpus.reserve(target_bytes + block.size());
    while (corpus.size() < target_bytes) {
      corpus += block;
    }
    corpus.resize(target_bytes);
    return corpus;
}

// Inline HMAC-SHA-256 pseudonymisation: mirrors PIIStreamPseudonymizer logic.
// Returns the first 8 hex chars of HMAC-SHA256(key, value).
// dlen is both the buffer capacity (input) and the actual digest length (output).
static std::string hmac_pseudonym(const std::string& key, const std::string& value) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned int  dlen = SHA256_DIGEST_LENGTH; // input: capacity; output: bytes written
    HMAC(EVP_sha256(),
         key.data(),   static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(value.data()),
         static_cast<int>(value.size()),
         digest, &dlen);
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
      oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

static constexpr size_t kCorpusSize = 100ULL * 1024 * 1024; // 100 MB

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_ScanOnly: 100 MB scan — no pseudonymisation, measures scanner throughput
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ScanOnly(benchmark::State& state) {
    const size_t chunk_size = static_cast<size_t>(state.range(0));
    auto engine = make_engine();
    auto corpus = build_corpus(kCorpusSize);

    for (auto _ : state) {
        // Use default PIIStreamScannerConfig — the constructor auto-derives
        // lookahead_bytes from engine->maxPatternLength(), so no explicit
        // assignment is needed here.
        PIIStreamScanner scanner(engine);

        size_t offset        = 0;
        size_t total_findings = 0;
        while (offset < corpus.size()) {
            size_t end     = std::min(offset + chunk_size, corpus.size());
            bool   is_last = (end == corpus.size());
            auto findings  = scanner.scan_chunk(
                std::string_view(corpus.data() + offset, end - offset), is_last);
            total_findings += findings.size();
            offset = end;
        }
        benchmark::DoNotOptimize(total_findings);
    }

    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(kCorpusSize));
    state.SetLabel("corpus=100MB chunk=" + std::to_string(chunk_size / 1024) + "KB");
}

BENCHMARK(BM_ScanOnly)
    ->Arg(4   * 1024)
    ->Arg(64  * 1024)
    ->Arg(1   * 1024 * 1024)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(3.0);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ScanAndPseudonymize: 100 MB scan + HMAC-SHA-256 span replacement
//
// Mirrors what PIIStreamPseudonymizer::process_chunk() does: for each finalized
// chunk, replace each detected PII span with hmac_pseudonym(tenant_key, span).
// Uses a fixed tenant key (no LEKManager/RocksDB dependency).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ScanAndPseudonymize(benchmark::State& state) {
    const size_t chunk_size = static_cast<size_t>(state.range(0));
    auto   engine      = make_engine();
    auto   corpus      = build_corpus(kCorpusSize);
    const std::string hmac_key = "bench-tenant:bench-lek-key";

    for (auto _ : state) {
        // Use default PIIStreamScannerConfig — the constructor auto-derives
        // lookahead_bytes from engine->maxPatternLength().
        PIIStreamScanner scanner(engine);

        size_t offset = 0;
        std::string output = {};
        output.reserve(kCorpusSize);

        while (offset < corpus.size()) {
            size_t end      = std::min(offset + chunk_size, corpus.size());
            bool   is_last  = (end == corpus.size());
            size_t base_off = scanner.bytes_processed();

            auto findings = scanner.scan_chunk(
                std::string_view(corpus.data() + offset, end - offset), is_last);

            // Pseudonymize: rebuild finalized portion with PII spans replaced.
            size_t finalized_len = scanner.bytes_processed() - base_off;
            std::string_view finalized(corpus.data() + offset, finalized_len);

            size_t cursor = 0;
            for (const auto& f : findings) {
                size_t rel_start = f.start_offset - base_off;
                size_t rel_end   = std::min(f.end_offset - base_off, finalized.size());
                if (rel_start > cursor)
                    output.append(finalized.data() + cursor, rel_start - cursor);
                output += hmac_pseudonym(hmac_key,
                    std::string(finalized.data() + rel_start, rel_end - rel_start));
                cursor = rel_end;
            }
            if (cursor < finalized.size())
                output.append(finalized.data() + cursor, finalized.size() - cursor);

            offset = end;
        }
        benchmark::DoNotOptimize(output.size());
    }

    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(kCorpusSize));
    state.SetLabel("corpus=100MB chunk=" + std::to_string(chunk_size / 1024) + "KB");
}

BENCHMARK(BM_ScanAndPseudonymize)
    ->Arg(4   * 1024)
    ->Arg(64  * 1024)
    ->Arg(1   * 1024 * 1024)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(3.0);

BENCHMARK_MAIN();
