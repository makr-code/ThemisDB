// Tests for Phase 2 async import API and Phase 3 import REST/metrics API.
//
// These tests are self-contained – they re-use the same stand-alone
// StringImporter harness from the other test files and test the new
// ImportHandle / ImportJobRegistry types + ImportApiHandler logic
// without requiring a running HTTP server.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <future>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Minimal importer types (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS = 0,
    FILE_OPEN_FAILED      = 101,
    FILE_READ_FAILED      = 102,
    NOT_A_PG_DUMP         = 103,
    PARSE_CREATE_TABLE    = 200,
    PARSE_COPY_HEADER     = 202,
    PARSE_COPY_ROW        = 203,
    STATEMENT_TOO_LARGE   = 204,
    ROW_TOO_LARGE         = 205,
    COLUMN_COUNT_MISMATCH = 301,
    UNKNOWN               = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records    = 0;
    size_t imported_records = 0;
    size_t failed_records   = 0;
    size_t skipped_records  = 0;
    size_t tables_processed = 0;
    double elapsed_seconds  = 0.0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

struct ImportOptions {
    bool   dry_run           = false;
    bool   continue_on_error = true;
    size_t batch_size        = 1000;
    std::string default_namespace = "imported";
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::map<std::string, std::string> type_overrides;
    size_t max_row_size_bytes       = 0;
    size_t max_statement_size_bytes = 0;
    bool   enforce_utf8             = false;
    std::string checkpoint_file;
};

// ---------------------------------------------------------------------------
// ImportStatus / ImportHandle (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

enum class ImportStatus { PENDING, RUNNING, COMPLETED, CANCELLED, FAILED };

struct ImportHandle {
    std::string id;
    std::atomic<size_t> current_records{0};
    std::atomic<size_t> total_records{0};
    std::atomic<bool>   running{false};
    std::string         stage;
    mutable std::mutex  stage_mutex;
    std::shared_future<ImportStats> future;
    int64_t started_at_ms  = 0;
    int64_t finished_at_ms = 0;

    ImportHandle() = default;
    ImportHandle(const ImportHandle&) = delete;
    ImportHandle& operator=(const ImportHandle&) = delete;

    ImportStatus getStatus() const {
        if (running.load()) {
          return ImportStatus::RUNNING;
        }
        if (!future.valid()) {
          return ImportStatus::PENDING;
        }
        if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return ImportStatus::RUNNING;
        return ImportStatus::COMPLETED;
    }

    std::string getStage() const {
        std::lock_guard<std::mutex> lk(stage_mutex);
        return stage;
    }
    void setStage(const std::string& s) {
        std::lock_guard<std::mutex> lk(stage_mutex);
        stage = s;
    }
};

// ---------------------------------------------------------------------------
// ImportJobRegistry (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

class ImportJobRegistry {
public:
    void add(std::shared_ptr<ImportHandle> h) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_[h->id] = h;
    }
    std::shared_ptr<ImportHandle> get(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = jobs_.find(id);
        return it != jobs_.end() ? it->second : nullptr;
    }
    std::vector<std::shared_ptr<ImportHandle>> all() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::shared_ptr<ImportHandle>> out;
        for (auto& [k, v] : jobs_) {
          out.push_back(v);
        }
        return out;
    }
    void remove(const std::string& id) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_.erase(id);
    }
    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return jobs_.size();
    }
private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<ImportHandle>> jobs_;
};

// ---------------------------------------------------------------------------
// Minimal async importer (mirrors what PostgreSQLImporter::importDataAsync does)
// ---------------------------------------------------------------------------

static ImportStats runSyncImport(const std::string& /*content*/,
                                  const ImportOptions& opts,
                                  std::shared_ptr<ImportHandle> handle) {
    ImportStats stats;
    stats.tables_processed = 2;
    const size_t rows = opts.dry_run ? 0 : 6;
    stats.total_records    = rows;
    stats.imported_records = rows;

    for (size_t i = 0; i < rows; ++i) {
        handle->current_records.store(i + 1);
        handle->total_records.store(rows);
        handle->setStage("copying row " + std::to_string(i + 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    stats.elapsed_seconds = 0.012;
    return stats;
}

static std::shared_ptr<ImportHandle> startAsync(const std::string& content,
                                                  const ImportOptions& opts) {
    auto handle  = std::make_shared<ImportHandle>();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    handle->id = "import-" + std::to_string(ms) + "-" +
                 std::to_string(reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    handle->started_at_ms = ms;
    handle->running.store(true);
    handle->setStage("pending");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::thread([content, opts, handle, promise]() mutable {
        ImportStats stats = runSyncImport(content, opts, handle);
        handle->running.store(false);
        handle->setStage("completed");
        handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        promise->set_value(std::move(stats));
    }).detach();

    return handle;
}

// ---------------------------------------------------------------------------
// Minimal Prometheus text-format builder (mirrors ImportApiHandler::handleMetrics)
// ---------------------------------------------------------------------------

static std::string buildPrometheusText(const ImportJobRegistry& registry) {
    auto jobs = registry.all();
    size_t imported = 0, failed = 0, skipped = 0;
    size_t running = 0, completed = 0;
    double duration = 0.0;

    for (auto& h : jobs) {
        switch (h->getStatus()) {
            case ImportStatus::RUNNING:   ++running;   break;
            case ImportStatus::COMPLETED: ++completed; break;
            default: break;
        }
        if (h->getStatus() == ImportStatus::COMPLETED &&
            h->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                auto stats = h->future.get();
                imported += stats.imported_records;
                failed   += stats.failed_records;
                skipped  += stats.skipped_records;
                duration += stats.elapsed_seconds;
            } catch (...) {}
        }
    }

    std::ostringstream o;
    o << "themisdb_import_jobs_total{status=\"running\"} " << running << "\n"
      << "themisdb_import_jobs_total{status=\"completed\"} " << completed << "\n"
      << "themisdb_import_rows_total{status=\"imported\"} " << imported << "\n"
      << "themisdb_import_rows_total{status=\"failed\"} "   << failed   << "\n"
      << "themisdb_import_rows_total{status=\"skipped\"} "  << skipped  << "\n"
      << "themisdb_import_duration_seconds_total " << duration << "\n";
    return o.str();
}

// ===========================================================================
// Tests: ImportHandle
// ===========================================================================

TEST(ImportHandleTest, InitialStatusIsPending) {
    ImportHandle h;
    EXPECT_EQ(h.getStatus(), ImportStatus::PENDING);
}

TEST(ImportHandleTest, IdAssignment) {
    auto h = std::make_shared<ImportHandle>();
    h->id = "import-123";
    EXPECT_EQ(h->id, "import-123");
}

TEST(ImportHandleTest, StageSetGetThreadSafe) {
    ImportHandle h;
    h.setStage("copying table users");
    EXPECT_EQ(h.getStage(), "copying table users");
}

TEST(ImportHandleTest, AtomicCountersAreReadable) {
    ImportHandle h;
    h.current_records.store(42);
    h.total_records.store(100);
    EXPECT_EQ(h.current_records.load(), 42u);
    EXPECT_EQ(h.total_records.load(), 100u);
}

TEST(ImportHandleTest, RunningTrueGivesRunningStatus) {
    auto h = std::make_shared<ImportHandle>();
    auto promise = std::make_shared<std::promise<ImportStats>>();
    h->future = promise->get_future().share();
    h->running.store(true);
    EXPECT_EQ(h->getStatus(), ImportStatus::RUNNING);
    // Cleanup: fulfil promise
    promise->set_value(ImportStats{});
}

TEST(ImportHandleTest, CompletedAfterPromiseFulfilled) {
    auto h = std::make_shared<ImportHandle>();
    auto promise = std::make_shared<std::promise<ImportStats>>();
    h->future = promise->get_future().share();
    h->running.store(false);
    promise->set_value(ImportStats{});
    EXPECT_EQ(h->getStatus(), ImportStatus::COMPLETED);
}

// ===========================================================================
// Tests: ImportJobRegistry
// ===========================================================================

TEST(ImportJobRegistryTest, AddAndGetById) {
    ImportJobRegistry reg;
    auto h = std::make_shared<ImportHandle>();
    h->id = "abc123";
    reg.add(h);
    auto found = reg.get("abc123");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "abc123");
}

TEST(ImportJobRegistryTest, GetNonExistentReturnsNull) {
    ImportJobRegistry reg;
    EXPECT_EQ(reg.get("does-not-exist"), nullptr);
}

TEST(ImportJobRegistryTest, AllReturnsCopyOfAllJobs) {
    ImportJobRegistry reg;
    for (int i = 0; i < 5; ++i) {
        auto h = std::make_shared<ImportHandle>();
        h->id = "job-" + std::to_string(i);
        reg.add(h);
    }
    EXPECT_EQ(reg.all().size(), 5u);
}

TEST(ImportJobRegistryTest, RemoveDeletesJob) {
    ImportJobRegistry reg;
    auto h = std::make_shared<ImportHandle>();
    h->id = "del-me";
    reg.add(h);
    EXPECT_EQ(reg.size(), 1u);
    reg.remove("del-me");
    EXPECT_EQ(reg.size(), 0u);
    EXPECT_EQ(reg.get("del-me"), nullptr);
}

TEST(ImportJobRegistryTest, MultipleRegistriesAreIndependent) {
    ImportJobRegistry r1, r2;
    auto h = std::make_shared<ImportHandle>();
    h->id = "x";
    r1.add(h);
    EXPECT_NE(r1.get("x"), nullptr);
    EXPECT_EQ(r2.get("x"), nullptr);
}

// ===========================================================================
// Tests: importDataAsync (via stand-alone helper)
// ===========================================================================

TEST(AsyncImportTest, HandlerIsNotNullAfterStart) {
    ImportOptions opts;
    auto handle = startAsync("-- dummy dump\n", opts);
    ASSERT_NE(handle, nullptr);
    EXPECT_FALSE(handle->id.empty());
}

TEST(AsyncImportTest, StatusTransitionsToRunningThenCompleted) {
    ImportOptions opts;
    auto handle = startAsync("-- dummy dump\n", opts);

    // At least one of the first polls should see RUNNING or already COMPLETED
    ImportStatus seen_after_start = handle->getStatus();
    EXPECT_TRUE(seen_after_start == ImportStatus::RUNNING ||
                seen_after_start == ImportStatus::COMPLETED);

    // Wait up to 1 s for completion
    handle->future.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(handle->getStatus(), ImportStatus::COMPLETED);
}

TEST(AsyncImportTest, FinalStatsAvailableAfterCompletion) {
    ImportOptions opts;
    auto handle = startAsync("-- dummy dump\n", opts);
    handle->future.wait_for(std::chrono::seconds(2));
    ASSERT_EQ(handle->getStatus(), ImportStatus::COMPLETED);
    auto stats = handle->future.get();
    EXPECT_EQ(stats.imported_records, 6u);
    EXPECT_EQ(stats.tables_processed, 2u);
}

TEST(AsyncImportTest, DryRunImportsZeroRecords) {
    ImportOptions opts;
    opts.dry_run = true;
    auto handle = startAsync("-- dummy dump\n", opts);
    handle->future.wait_for(std::chrono::seconds(2));
    auto stats = handle->future.get();
    EXPECT_EQ(stats.imported_records, 0u);
}

TEST(AsyncImportTest, LiveProgressCounterIncrements) {
    ImportOptions opts;
    auto handle = startAsync("-- dummy dump\n", opts);

    size_t max_seen = 0;
    for (int i = 0; i < 20; ++i) {
        max_seen = std::max(max_seen, handle->current_records.load());
        if (handle->getStatus() == ImportStatus::COMPLETED) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    handle->future.wait_for(std::chrono::seconds(2));
    // At least some rows should have been counted live
    EXPECT_GT(handle->current_records.load(), 0u);
}

TEST(AsyncImportTest, FinishedAtMsSetAfterCompletion) {
    ImportOptions opts;
    auto handle = startAsync("-- dummy dump\n", opts);
    handle->future.wait_for(std::chrono::seconds(2));
    EXPECT_GT(handle->finished_at_ms, 0LL);
    EXPECT_GE(handle->finished_at_ms, handle->started_at_ms);
}

TEST(AsyncImportTest, MultipleJobsRunIndependently) {
    ImportOptions opts;
    auto h1 = startAsync("-- dump1\n", opts);
    auto h2 = startAsync("-- dump2\n", opts);
    EXPECT_NE(h1->id, h2->id);
    h1->future.wait_for(std::chrono::seconds(2));
    h2->future.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(h1->getStatus(), ImportStatus::COMPLETED);
    EXPECT_EQ(h2->getStatus(), ImportStatus::COMPLETED);
    EXPECT_EQ(h1->future.get().imported_records, 6u);
    EXPECT_EQ(h2->future.get().imported_records, 6u);
}

TEST(AsyncImportTest, JobRegistryTracksMultipleJobs) {
    ImportJobRegistry reg;
    ImportOptions opts;
    for (int i = 0; i < 3; ++i) {
        auto h = startAsync("-- dump\n", opts);
        reg.add(h);
    }
    EXPECT_EQ(reg.size(), 3u);
    for (auto& h : reg.all()) {
        h->future.wait_for(std::chrono::seconds(2));
        EXPECT_EQ(h->getStatus(), ImportStatus::COMPLETED);
    }
}

// ===========================================================================
// Tests: Prometheus text-format metrics endpoint
// ===========================================================================

TEST(ImportMetricsEndpointTest, EmptyRegistryProducesValidPrometheus) {
    ImportJobRegistry reg;
    std::string prom = buildPrometheusText(reg);
    EXPECT_NE(prom.find("themisdb_import_jobs_total{status=\"running\"} 0"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_import_jobs_total{status=\"completed\"} 0"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_import_rows_total{status=\"imported\"} 0"), std::string::npos);
}

TEST(ImportMetricsEndpointTest, CompletedJobRowsAreAggregated) {
    ImportJobRegistry reg;
    ImportOptions opts;
    for (int i = 0; i < 2; ++i) {
        auto h = startAsync("-- dump\n", opts);
        h->future.wait_for(std::chrono::seconds(2));
        reg.add(h);
    }
    std::string prom = buildPrometheusText(reg);
    // 2 jobs × 6 rows = 12 imported
    EXPECT_NE(prom.find("themisdb_import_rows_total{status=\"imported\"} 12"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_import_jobs_total{status=\"completed\"} 2"), std::string::npos);
}

TEST(ImportMetricsEndpointTest, RunningJobCountedCorrectly) {
    ImportJobRegistry reg;
    ImportOptions opts;
    // Submit job but don't wait for it to finish
    auto h = startAsync("-- dump\n", opts);
    if (h->getStatus() == ImportStatus::RUNNING) {
        reg.add(h);
        std::string prom = buildPrometheusText(reg);
        EXPECT_NE(prom.find("themisdb_import_jobs_total{status=\"running\"} 1"), std::string::npos);
    }
    // Cleanup
    h->future.wait_for(std::chrono::seconds(2));
}

TEST(ImportMetricsEndpointTest, PrometheusTextContainsAllRequiredMetricNames) {
    ImportJobRegistry reg;
    std::string prom = buildPrometheusText(reg);
    EXPECT_NE(prom.find("themisdb_import_jobs_total"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_import_rows_total"), std::string::npos);
    EXPECT_NE(prom.find("themisdb_import_duration_seconds_total"), std::string::npos);
}

TEST(ImportMetricsEndpointTest, DurationAccumulatesAcrossJobs) {
    ImportJobRegistry reg;
    ImportOptions opts;
    for (int i = 0; i < 3; ++i) {
        auto h = startAsync("-- dump\n", opts);
        h->future.wait_for(std::chrono::seconds(2));
        reg.add(h);
    }
    std::string prom = buildPrometheusText(reg);
    // 3 × 0.012 = 0.036s; line should not be zero
    EXPECT_EQ(prom.find("themisdb_import_duration_seconds_total 0\n"), std::string::npos);
}

// ===========================================================================
// Tests: ImportApiHandler option parsing (JSON → ImportOptions round-trip)
// ===========================================================================

static ImportOptions optionsFromJson(const std::string& json_str) {
    // Minimal re-implementation of ImportApiHandler::optionsFromJson
    // GAP-020: mirrors the production cap of 100,000 for batch_size.
    static constexpr size_t kMaxBatchSize = 100'000;
    auto j = nlohmann::json::parse(json_str);
    ImportOptions opts;
    if (j.contains("dry_run") && j["dry_run"].is_boolean())
        opts.dry_run = j["dry_run"].get<bool>();
    if (j.contains("continue_on_error") && j["continue_on_error"].is_boolean())
        opts.continue_on_error = j["continue_on_error"].get<bool>();
    if (j.contains("batch_size") && j["batch_size"].is_number_unsigned()) {
        const size_t requested = j["batch_size"].get<size_t>();
        opts.batch_size = std::min(requested, kMaxBatchSize);
    }
    if (j.contains("default_namespace") && j["default_namespace"].is_string())
        opts.default_namespace = j["default_namespace"].get<std::string>();
    if (j.contains("enforce_utf8") && j["enforce_utf8"].is_boolean())
        opts.enforce_utf8 = j["enforce_utf8"].get<bool>();
    if (j.contains("include_tables") && j["include_tables"].is_array())
        for (auto& t : j["include_tables"])
            if (t.is_string()) {
              opts.include_tables.push_back(t.get<std::string>());
            }
    if (j.contains("type_overrides") && j["type_overrides"].is_object())
        for (auto& [k, v] : j["type_overrides"].items())
            if (v.is_string()) {
              opts.type_overrides[k] = v.get<std::string>();
            }
    return opts;
}

TEST(ImportApiHandlerOptionsTest, DryRunParsed) {
    auto opts = optionsFromJson(R"({"dry_run": true})");
    EXPECT_TRUE(opts.dry_run);
}

TEST(ImportApiHandlerOptionsTest, BatchSizeParsed) {
    auto opts = optionsFromJson(R"({"batch_size": 5000})");
    EXPECT_EQ(opts.batch_size, 5000u);
}

TEST(ImportApiHandlerOptionsTest, EnforceUtf8Parsed) {
    auto opts = optionsFromJson(R"({"enforce_utf8": true})");
    EXPECT_TRUE(opts.enforce_utf8);
}

TEST(ImportApiHandlerOptionsTest, IncludeTablesParsed) {
    auto opts = optionsFromJson(R"({"include_tables": ["users", "orders"]})");
    ASSERT_EQ(opts.include_tables.size(), 2u);
    EXPECT_EQ(opts.include_tables[0], "users");
    EXPECT_EQ(opts.include_tables[1], "orders");
}

TEST(ImportApiHandlerOptionsTest, TypeOverridesParsed) {
    auto opts = optionsFromJson(R"({"type_overrides": {"bigint": "integer", "jsonb": "json"}})");
    EXPECT_EQ(opts.type_overrides.at("bigint"), "integer");
    EXPECT_EQ(opts.type_overrides.at("jsonb"), "json");
}

TEST(ImportApiHandlerOptionsTest, UnknownFieldsIgnored) {
    // Should not throw
    EXPECT_NO_THROW({
        auto opts = optionsFromJson(R"({"unknown_field": 42, "dry_run": false})");
        EXPECT_FALSE(opts.dry_run);
    });
}

TEST(ImportApiHandlerOptionsTest, EmptyJsonGivesDefaults) {
    auto opts = optionsFromJson("{}");
    EXPECT_FALSE(opts.dry_run);
    EXPECT_TRUE(opts.continue_on_error);
    EXPECT_EQ(opts.batch_size, 1000u);
}

// ===========================================================================
// Tests: S3 API route validation helpers
// (mirror the logic in ImportApiHandler::handleStartS3Import)
// ===========================================================================

/// Minimal re-implementation of S3Importer::parseS3Url for isolated testing.
static bool parseS3UrlForApi(const std::string& url,
                               std::string& bucket, std::string& key) {
    static const std::string prefix = "s3://";
    if (url.size() < prefix.size() || url.substr(0, prefix.size()) != prefix)
        return false;
    std::string rest = url.substr(prefix.size());
    auto slash = rest.find('/');
    if (slash == std::string::npos) {
        bucket = rest; key.clear();
    } else {
        bucket = rest.substr(0, slash); key = rest.substr(slash + 1);
    }
    return !bucket.empty();
}

TEST(ImportApiS3RouteTest, ValidS3UrlAccepted) {
    std::string bucket, key;
    EXPECT_TRUE(parseS3UrlForApi("s3://my-bucket/data/file.csv", bucket, key));
    EXPECT_EQ(bucket, "my-bucket");
    EXPECT_EQ(key, "data/file.csv");
}

TEST(ImportApiS3RouteTest, ValidS3PrefixUrlAccepted) {
    std::string bucket, key;
    EXPECT_TRUE(parseS3UrlForApi("s3://logs/2026/", bucket, key));
    EXPECT_EQ(bucket, "logs");
    EXPECT_EQ(key, "2026/");
}

TEST(ImportApiS3RouteTest, RejectHttpUrl) {
    std::string bucket, key;
    EXPECT_FALSE(parseS3UrlForApi("https://s3.amazonaws.com/bucket/key",
                                   bucket, key));
}

TEST(ImportApiS3RouteTest, RejectFileUrl) {
    std::string bucket, key;
    EXPECT_FALSE(parseS3UrlForApi("/local/path/data.csv", bucket, key));
}

TEST(ImportApiS3RouteTest, RejectEmptyString) {
    std::string bucket, key;
    EXPECT_FALSE(parseS3UrlForApi("", bucket, key));
}

TEST(ImportApiS3RouteTest, RejectS3SchemeOnly) {
    std::string bucket, key;
    EXPECT_FALSE(parseS3UrlForApi("s3://", bucket, key));
}

TEST(ImportApiS3RouteTest, ValidS3UrlNoKey) {
    std::string bucket, key;
    EXPECT_TRUE(parseS3UrlForApi("s3://bucket-only", bucket, key));
    EXPECT_EQ(bucket, "bucket-only");
    EXPECT_TRUE(key.empty());
}

/// Simulates ImportApiHandler::handleStartS3Import validation logic.
/// Returns HTTP-like status: 200 (OK), 400 (bad request), 501 (not configured).
static int simulateS3RouteValidation(const std::string& source_path,
                                      bool s3_importer_configured) {
    if (!s3_importer_configured) {
      return 501;
    }
    if (source_path.empty()) {
      return 400;
    }
    std::string bucket, key;
    if (!parseS3UrlForApi(source_path, bucket, key)) {
      return 400;
    }
    return 200;
}

TEST(ImportApiS3RouteTest, Returns501WhenS3ImporterNotConfigured) {
    EXPECT_EQ(simulateS3RouteValidation("s3://bucket/key", false), 501);
}

TEST(ImportApiS3RouteTest, Returns400ForInvalidUrl) {
    EXPECT_EQ(simulateS3RouteValidation("/not/s3", true), 400);
    EXPECT_EQ(simulateS3RouteValidation("", true), 400);
    EXPECT_EQ(simulateS3RouteValidation("http://bucket/key", true), 400);
}

TEST(ImportApiS3RouteTest, Returns200ForValidS3Url) {
    EXPECT_EQ(simulateS3RouteValidation("s3://bucket/data.csv", true), 200);
    EXPECT_EQ(simulateS3RouteValidation("s3://bucket/prefix/", true), 200);
    EXPECT_EQ(simulateS3RouteValidation("s3://bucket", true), 200);
}

// ===========================================================================
// GAP-020 — batch_size DoS cap (CWE-400)
// ===========================================================================

// GAP-020-01: batch_size at the maximum boundary is preserved exactly.
TEST(ImportApiHandlerOptionsTest, GAP020_BatchSizeAtMaxBoundary_Preserved) {
    static constexpr size_t kMaxBatchSize = 100'000;
    auto opts = optionsFromJson(R"({"batch_size": 100000})");
    EXPECT_EQ(opts.batch_size, kMaxBatchSize);
}

// GAP-020-02: batch_size exceeding the cap is clamped to the cap.
TEST(ImportApiHandlerOptionsTest, GAP020_OversizeBatchSize_ClampedToCap) {
    auto opts = optionsFromJson(R"({"batch_size": 9999999999})");
    EXPECT_LE(opts.batch_size, 100'000u)
        << "batch_size must be clamped to prevent DoS via memory exhaustion";
}

// GAP-020-03: Normal batch_size below the cap passes through unchanged.
TEST(ImportApiHandlerOptionsTest, GAP020_NormalBatchSize_Unchanged) {
    auto opts = optionsFromJson(R"({"batch_size": 500})");
    EXPECT_EQ(opts.batch_size, 500u);
}
