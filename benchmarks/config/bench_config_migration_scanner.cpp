/*
 * bench_config_migration_scanner.cpp
 *
 * Google Benchmark measuring the throughput of the config_migration_scanner
 * logic over a synthetic file tree.
 *
 * Performance target (from src/config/FUTURE_ENHANCEMENTS.md):
 *   CLI scanner 10 K files < 5 s
 *
 * Benchmarks included
 * -------------------
 *   BM_ScanSingleFile_NoMatch     – scanFile() on a file with no legacy refs
 *   BM_ScanSingleFile_WithMatch   – scanFile() on a file containing a legacy ref
 *   BM_ShouldScanFile             – shouldScanFile() gate (hot path)
 *   BM_ScanTree_10K               – scan a 10 000-file tree; verifies < 5 s target
 *   BM_ScanTree_1K                – scan a 1 000-file tree (faster smoke-check)
 */

#include "config/config_migration_scanner_impl.h"

#include <benchmark/benchmark.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace themis {
namespace config {
namespace bench {

namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static void writeFile(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << content;
}

// ============================================================================
// Fixture – sets up and tears down a temporary directory tree
// ============================================================================

class MigrationScannerBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*state*/) override {
        temp_root_ = fs::temp_directory_path() /
                     ("themisdb_scanner_bench_" +
                      std::to_string(std::chrono::steady_clock::now()
                                         .time_since_epoch()
                                         .count()));
        fs::create_directories(temp_root_);

        // One file with no legacy references (common case)
        no_match_file_ = temp_root_ / "no_match.yaml";
        writeFile(no_match_file_, "setting: value\nother_key: 42\n");

        // One file with a known legacy path reference
        with_match_file_ = temp_root_ / "with_match.yaml";
        writeFile(with_match_file_,
                  "# references a legacy config path\n"
                  "path: config/pii_patterns.yaml\n");
    }

    void TearDown(const benchmark::State& /*state*/) override {
        std::error_code ec = {};
        fs::remove_all(temp_root_, ec);  // best-effort cleanup
    }

protected:
    fs::path temp_root_;
    fs::path no_match_file_;
    fs::path with_match_file_;
};

// ============================================================================
// Helper: create a synthetic file tree with the requested number of files
//
// Layout:
//   <root>/subdir_<N>/<file_<M>.yaml|.json|.toml|.ini|.env|.txt>
//
// One in every 20 scannable files contains a legacy path reference so that
// the scanner's inner loop is exercised in a realistic ratio.
// ============================================================================

static fs::path buildFileTree(int num_files) {
    fs::path root =
        fs::temp_directory_path() /
        ("themisdb_scanner_tree_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
         "_" + std::to_string(num_files));

    const std::vector<std::string> scannable_exts = {
        ".yaml", ".yml", ".json", ".toml", ".ini", ".env"};
    const std::string non_scannable_ext = ".txt";

    for (int i = 0; i < num_files; ++i) {
        // Distribute files across 100 subdirectories to mimic a real project
        fs::path dir = root / ("subdir_" + std::to_string(i % 100));

        // Alternate between scannable and non-scannable to keep 80 % scannable
        std::string ext = (i % 5 == 4) ? non_scannable_ext
                                        : scannable_exts[i % scannable_exts.size()];
        fs::path p = dir / ("file_" + std::to_string(i) + ext);

        // Every 20th scannable file gets a legacy path reference
        bool has_match = (ext != non_scannable_ext) && (i % 20 == 0);
        std::string content =
            has_match ? "path: config/pii_patterns.yaml\nkey: value\n"
                      : "setting: value\nother_key: 42\n";

        writeFile(p, content);
    }
    return root;
}

// ============================================================================
// 1. shouldScanFile() – file-extension gate (target: nanoseconds)
// ============================================================================

static void BM_ShouldScanFile(benchmark::State& state) {
    const std::vector<fs::path> paths = {
        fs::path("config.yaml"),
        fs::path("config.yml"),
        fs::path("config.json"),
        fs::path("config.toml"),
        fs::path("config.ini"),
        fs::path("config.env"),
        fs::path("README.md"),
        fs::path("source.cpp"),
        fs::path("header.h"),
        fs::path("data.txt"),
    };

    std::size_t idx = 0;
    for (auto _ : state) {
        bool result = cms::shouldScanFile(paths[idx % paths.size()]);
        benchmark::DoNotOptimize(result);
        ++idx;
    }
    state.SetLabel("file-extension gate (hot path)");
}
BENCHMARK(BM_ShouldScanFile)->Unit(benchmark::kNanosecond)->MinTime(0.5);

// ============================================================================
// 2. scanFile() – single file, no legacy path references
// ============================================================================

BENCHMARK_DEFINE_F(MigrationScannerBenchFixture, ScanSingleFile_NoMatch)(
        benchmark::State& state) {
    for (auto _ : state) {
        auto matches = cms::scanFile(no_match_file_);
        benchmark::DoNotOptimize(matches);
    }
    state.SetLabel("single YAML, 0 legacy refs");
}
BENCHMARK_REGISTER_F(MigrationScannerBenchFixture, ScanSingleFile_NoMatch)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 3. scanFile() – single file with one legacy path reference
// ============================================================================

BENCHMARK_DEFINE_F(MigrationScannerBenchFixture, ScanSingleFile_WithMatch)(
        benchmark::State& state) {
    for (auto _ : state) {
        auto matches = cms::scanFile(with_match_file_);
        benchmark::DoNotOptimize(matches);
    }
    state.SetLabel("single YAML, 1 legacy ref");
}
BENCHMARK_REGISTER_F(MigrationScannerBenchFixture, ScanSingleFile_WithMatch)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 4. Full tree scan – 1 000 files  (fast smoke-check)
// ============================================================================

static void BM_ScanTree_1K(benchmark::State& state) {
    static const int kFiles = 1000;
    fs::path root = buildFileTree(kFiles);

    for (auto _ : state) {
        std::vector<cms::ScanMatch> all_matches = {};

        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) {
              continue;
            }
            if (!cms::shouldScanFile(entry.path())) {
              continue;
            }
            auto m = cms::scanFile(entry.path());
            all_matches.insert(all_matches.end(), m.begin(), m.end());
        }
        benchmark::DoNotOptimize(all_matches);
    }

    state.SetLabel("1K files; expect < 0.5 s");

    std::error_code ec = {};
    fs::remove_all(root, ec);
}
BENCHMARK(BM_ScanTree_1K)->Unit(benchmark::kMillisecond)->Iterations(1);

// ============================================================================
// 5. Full tree scan – 10 000 files  (performance target: < 5 s)
// ============================================================================

static void BM_ScanTree_10K(benchmark::State& state) {
    static const int kFiles = 10000;
    fs::path root = buildFileTree(kFiles);

    for (auto _ : state) {
        std::vector<cms::ScanMatch> all_matches = {};

        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) {
              continue;
            }
            if (!cms::shouldScanFile(entry.path())) {
              continue;
            }
            auto m = cms::scanFile(entry.path());
            all_matches.insert(all_matches.end(), m.begin(), m.end());
        }
        benchmark::DoNotOptimize(all_matches);
        state.counters["matches"] =
            benchmark::Counter(static_cast<double>(all_matches.size()));
    }

    state.SetLabel("10K files; target < 5 s");

    std::error_code ec = {};
    fs::remove_all(root, ec);
}
// Single iteration: we're measuring wall-clock throughput for 10K files,
// not a tight loop.  MinTime(5.0) would defeat the purpose; instead we run
// exactly once and check the elapsed time reported by the benchmark runner.
BENCHMARK(BM_ScanTree_10K)->Unit(benchmark::kSecond)->Iterations(1);

} // namespace bench
} // namespace config
} // namespace themis

BENCHMARK_MAIN();
