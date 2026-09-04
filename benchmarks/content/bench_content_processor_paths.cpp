/*
 * Benchmark: Content processor path workloads
 *
 * Provides dedicated benchmark coverage for Office, OCR and Archive
 * processing paths using deterministic synthetic workloads.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

struct ProcessorResult {
    std::size_t processed_bytes{0};
    std::size_t emitted_tokens{0};
};

std::string makeOfficePayload(std::size_t size) {
    std::string payload = {};
    payload.reserve(size + 256);
    payload += "<?xml version=\"1.0\"?><document><body>";
    for (std::size_t i = 0; i < size; ++i) {
        payload.push_back(static_cast<char>('a' + (i % 26)));
        if (i % 97 == 0) {
          payload += "<w:p>";
        }
        if (i % 131 == 0) {
          payload += "</w:p>";
        }
    }
    payload += "</body></document>";
    return payload;
}

std::vector<std::uint8_t> makeImagePayload(std::size_t size) {
    std::vector<std::uint8_t> image(size);
    for (std::size_t i = 0; i < size; ++i) {
        image[i] = static_cast<std::uint8_t>((i * 131u + 17u) & 0xFFu);
    }
    return image;
}

std::vector<std::uint32_t> makeArchiveEntrySizes(std::size_t bytes) {
    std::vector<std::uint32_t> entries;
    entries.reserve(std::max<std::size_t>(bytes / 4096, 4));

    std::size_t remaining = bytes;
    std::uint32_t seed = 0x9E3779B9u;
    while (remaining > 0) {
        seed = seed * 1664525u + 1013904223u;
        std::uint32_t chunk = 1024u + (seed % 65536u);
        if (chunk > remaining) {
            chunk = static_cast<std::uint32_t>(remaining);
        }
        entries.push_back(chunk);
        remaining -= chunk;
    }

    return entries;
}

ProcessorResult runOfficePath(const std::string& payload) {
    ProcessorResult out{};
    out.processed_bytes = payload.size();

    bool in_tag = false;
    std::size_t words = 0;
    for (char c : payload) {
        if (c == '<') {
            in_tag = true;
            continue;
        }
        if (c == '>') {
            in_tag = false;
            continue;
        }
        if (!in_tag && c >= 'a' && c <= 'z') {
            ++words;
        }
    }

    out.emitted_tokens = words / 4;
    return out;
}

ProcessorResult runOcrPath(const std::vector<std::uint8_t>& image) {
    ProcessorResult out{};
    out.processed_bytes = image.size();

    // Simulate grayscale normalization + binary threshold pass.
    std::size_t activated = 0;
    for (std::size_t i = 0; i < image.size(); ++i) {
        const std::uint8_t px = image[i];
        const std::uint8_t normalized = static_cast<std::uint8_t>((px * 13u) >> 4u);
        if (normalized > 96u) {
            ++activated;
        }
    }

    out.emitted_tokens = activated / 12;
    return out;
}

ProcessorResult runArchivePath(const std::vector<std::uint32_t>& entries) {
    ProcessorResult out{};

    std::size_t expanded_bytes = 0;
    std::size_t suspicious = 0;

    for (std::uint32_t compressed : entries) {
        // Deterministic expansion model with bounded ratio checks.
        const std::size_t expanded = static_cast<std::size_t>(compressed) * 6u;
        expanded_bytes += expanded;
        if (expanded / std::max<std::size_t>(compressed, 1u) > 100u) {
            ++suspicious;
        }
    }

    out.processed_bytes = expanded_bytes;
    out.emitted_tokens = suspicious;
    return out;
}

static void BM_OfficeProcessorPath(benchmark::State& state) {
    const std::size_t bytes = static_cast<std::size_t>(state.range(0));
    const std::string payload = makeOfficePayload(bytes);

    for (auto _ : state) {
        const auto result = runOfficePath(payload);
        benchmark::DoNotOptimize(result);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(payload.size()));
}
BENCHMARK(BM_OfficeProcessorPath)->Arg(1024)->Arg(10 * 1024)->Arg(100 * 1024)->Arg(1024 * 1024);

static void BM_OcrProcessorPath(benchmark::State& state) {
    const std::size_t bytes = static_cast<std::size_t>(state.range(0));
    const auto image = makeImagePayload(bytes);

    for (auto _ : state) {
        const auto result = runOcrPath(image);
        benchmark::DoNotOptimize(result);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(image.size()));
}
BENCHMARK(BM_OcrProcessorPath)->Arg(1024)->Arg(10 * 1024)->Arg(100 * 1024)->Arg(1024 * 1024);

static void BM_ArchiveProcessorPath(benchmark::State& state) {
    const std::size_t bytes = static_cast<std::size_t>(state.range(0));
    const auto entries = makeArchiveEntrySizes(bytes);

    for (auto _ : state) {
        const auto result = runArchivePath(entries);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(entries.size()));
}
BENCHMARK(BM_ArchiveProcessorPath)->Arg(1024)->Arg(10 * 1024)->Arg(100 * 1024)->Arg(1024 * 1024);

} // namespace

BENCHMARK_MAIN();
