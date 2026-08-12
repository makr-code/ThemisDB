#include <benchmark/benchmark.h>

#include "analytics/analytics_export.h"
#include "analytics/arrow_export.h"

#include <array>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

using themis::analytics::ArrowRecordBatch;
using themis::analytics::ExportFormat;
using themis::analytics::ExportOptions;
using themis::analytics::ExportStatus;
using themis::analytics::ExporterFactory;

namespace {

constexpr std::size_t kRows = 1'000'000;

const ArrowRecordBatch& syntheticDataset1M() {
    static const ArrowRecordBatch batch = [] {
        ArrowRecordBatch b;
        b.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
        b.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});
        b.addColumn({"category", ArrowRecordBatch::DataType::STRING, false});
        b.addColumn({"active", ArrowRecordBatch::DataType::BOOLEAN, false});

        std::vector<std::variant<std::nullptr_t, int64_t, double, std::string, bool>> row(4);
        static constexpr std::array<const char*, 8> categories{
            "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta"};

        for (std::size_t i = 0; i < kRows; ++i) {
            row[0] = static_cast<int64_t>(i);
            row[1] = static_cast<double>(i % 10'000) * 0.01;
            row[2] = std::string(categories[i % categories.size()]);
            row[3] = (i % 2) == 0;
            b.appendRow(row);
        }
        return b;
    }();
    return batch;
}

} // namespace

static void BM_CsvExport_1M(benchmark::State& state) {
    const auto& batch = syntheticDataset1M();
    std::filesystem::create_directories("./data");
    const std::string output_path = "./data/bench_csv_export_1m.csv";

    auto exporter = ExporterFactory::createDefaultExporter();
    if (!exporter) {
        state.SkipWithError("CSV exporter unavailable");
        return;
    }

    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.compress = false;
    options.batch_size = 10'000;

    for (auto _ : state) {
        const auto result = exporter->exportToFile(batch, output_path, options);
        if (result.status != ExportStatus::SUCCESS) {
            state.SkipWithError(result.message.c_str());
            break;
        }
        benchmark::DoNotOptimize(result.bytes_written);
        std::filesystem::remove(output_path);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * kRows));
    state.SetLabel("CSV/1M rows");
}

BENCHMARK(BM_CsvExport_1M)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
