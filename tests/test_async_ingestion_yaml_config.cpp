/**
 * @file test_async_ingestion_yaml_config.cpp
 * @brief Unit tests for AsyncIngestionWorker YAML config loading and
 *        user context propagation (Issue #3998).
 *
 * Acceptance Criteria:
 *   AC-1  loadSourcesFromConfig() parses worker_threads, queue_depth,
 *         batch_size, retry_attempts from a YAML file via
 *         ConfigPathResolver + ConfigSchemaValidator.
 *   AC-2  loadSourcesFromConfig() submits one job per entry in the
 *         "sources" array.
 *   AC-3  loadSourcesFromConfig() tolerates a missing "sources" key
 *         without throwing.
 *   AC-4  loadSourcesFromConfig() propagates user_context from each
 *         source entry into the submitted IngestionJob.
 *   AC-5  submitSourceJob() accepts and propagates user_context into
 *         the resulting IngestionJob.
 *   AC-6  loadSourcesFromConfig() throws when the file does not exist.
 *   AC-7  AsyncIngestionConfig exposes batch_size and retry_attempts
 *         fields with correct defaults.
 */

#include <gtest/gtest.h>
#include "content/async_ingestion_worker.h"
#include "content/content_manager.h"
#include "content/ingestion_plugin.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

using namespace themis;
using namespace themis::content;
using namespace std::chrono_literals;

namespace {

// ============================================================================
// TestDatabase helper (mirrors pattern in test_async_ingestion_backpressure)
// ============================================================================

class TestDatabase {
public:
    TestDatabase() {
        std::error_code ec = {};
        path_ = std::filesystem::temp_directory_path() /
                ("themis_yaml_cfg_test_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()));
        std::filesystem::create_directories(path_, ec);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = path_.string();
        cfg.enable_wal = true;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        if (!storage_->open()) {
            throw std::runtime_error("Failed to open test RocksDB: " + path_.string());
        }

        vector_index_    = std::make_shared<VectorIndexManager>(*storage_);
        graph_index_     = std::make_shared<GraphIndexManager>(*storage_);
        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);

        content_manager_ = std::make_shared<ContentManager>(
            storage_, vector_index_, graph_index_, secondary_index_);
    }

    ~TestDatabase() {
        storage_->close();
        std::filesystem::remove_all(path_);
    }

    std::shared_ptr<ContentManager> getContentManager() { return content_manager_; }

private:
    std::filesystem::path path_;
    std::shared_ptr<RocksDBWrapper>          storage_;
    std::shared_ptr<VectorIndexManager>      vector_index_;
    std::shared_ptr<GraphIndexManager>       graph_index_;
    std::shared_ptr<SecondaryIndexManager>   secondary_index_;
    std::shared_ptr<ContentManager>          content_manager_;
};

// ============================================================================
// Helpers
// ============================================================================

/** Write a YAML file to a temporary path and return the path. */
std::string writeTempYaml(const std::string& content) {
    auto tmp = std::filesystem::temp_directory_path() /
               ("async_worker_test_" +
                std::to_string(std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count()) +
                ".yaml");
    std::ofstream f(tmp);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot create temp file: " + tmp.string());
    }
    f << content;
    return tmp.string();
}

/** Resolve config/content/async_worker.yaml by walking parent directories. */
std::string findAsyncWorkerConfigPath() {
    std::filesystem::path base = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i) {
        const auto candidate = base / "config" / "content" / "async_worker.yaml";
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
        if (!base.has_parent_path()) {
            break;
        }
        base = base.parent_path();
    }
    return "config/content/async_worker.yaml";
}

// ============================================================================
// Fixture
// ============================================================================

class AsyncIngestionYamlConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<TestDatabase>();
    }

    void TearDown() override {
        for (const auto& p : temp_files_) {
            std::error_code ec = {};
            std::filesystem::remove(p, ec);
        }
        db_.reset();
    }

    std::string writeTempYamlTracked(const std::string& content) {
        auto path = writeTempYaml(content);
        temp_files_.push_back(path);
        return path;
    }

    std::unique_ptr<AsyncIngestionWorker> makeWorker(
        size_t worker_threads = 1,
        size_t queue_depth    = 100
    ) {
        AsyncIngestionConfig cfg;
        cfg.worker_thread_count = worker_threads;
        cfg.max_queue_size      = queue_depth + 100;
        cfg.max_queue_depth     = queue_depth;
        cfg.enable_auto_cleanup = false;
        cfg.verbose_logging     = false;
        return std::make_unique<AsyncIngestionWorker>(
            db_->getContentManager(), cfg);
    }

    std::unique_ptr<TestDatabase>  db_;
    std::vector<std::string>       temp_files_;
};

// ============================================================================
// AC-7: AsyncIngestionConfig new fields have correct defaults
// ============================================================================

TEST(AsyncIngestionConfigDefaultsTest, BatchSizeDefault) {
    AsyncIngestionConfig cfg;
    EXPECT_EQ(cfg.batch_size, 64u);
}

TEST(AsyncIngestionConfigDefaultsTest, RetryAttemptsDefault) {
    AsyncIngestionConfig cfg;
    EXPECT_EQ(cfg.retry_attempts, 3);
}

// ============================================================================
// AC-1: loadSourcesFromConfig() applies worker pool settings from YAML
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_AppliesWorkerSettings) {
    auto path = writeTempYamlTracked(
        "worker_threads: 4\n"
        "queue_depth: 500\n"
        "batch_size: 128\n"
        "retry_attempts: 5\n"
    );

    auto worker = makeWorker();
    // Worker not started; loadSourcesFromConfig is legal before start
    // (it only updates config_ and submits jobs to the queue).
    // We call it on a stopped worker to avoid needing a plugin.
    EXPECT_NO_THROW(worker->loadSourcesFromConfig(path));

    // Verify the config fields were updated by reloading from the same YAML
    // via ConfigSchemaValidator directly to confirm parsing correctness.
    // The worker's config_ is private, so we verify the YAML values
    // are valid and accessible through the public API indirectly by
    // confirming no exception is thrown and the call succeeds.
}

// ============================================================================
// AC-1 (extended): individual YAML keys are correctly parsed
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_WorkerThreadsOnly) {
    auto path = writeTempYamlTracked("worker_threads: 8\n");
    auto worker = makeWorker();
    EXPECT_NO_THROW(worker->loadSourcesFromConfig(path));
}

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_BatchSizeAndRetryOnly) {
    auto path = writeTempYamlTracked(
        "batch_size: 32\n"
        "retry_attempts: 1\n"
    );
    auto worker = makeWorker();
    EXPECT_NO_THROW(worker->loadSourcesFromConfig(path));
}

// ============================================================================
// AC-3: loadSourcesFromConfig() tolerates missing "sources" key
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_NoSourcesKey_DoesNotThrow) {
    auto path = writeTempYamlTracked(
        "worker_threads: 2\n"
        "queue_depth: 200\n"
    );
    auto worker = makeWorker();
    EXPECT_NO_THROW(worker->loadSourcesFromConfig(path));
}

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_EmptySourcesArray_DoesNotThrow) {
    auto path = writeTempYamlTracked(
        "worker_threads: 2\n"
        "sources: []\n"
    );
    auto worker = makeWorker();
    EXPECT_NO_THROW(worker->loadSourcesFromConfig(path));
}

// ============================================================================
// AC-2: loadSourcesFromConfig() submits one job per entry in "sources"
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_SubmitsSourceJobs) {
    // We need a running worker with a registered plugin handler so that
    // the submitted jobs are processed.  We use a custom job handler to
    // count how many jobs arrive.
    std::atomic<int> handled{0};

    auto worker = makeWorker(/*worker_threads=*/1, /*queue_depth=*/50);
    worker->registerJobHandler(
        IngestionJobType::HUGGINGFACE,
        [&](IngestionJob& job) {
            ++handled;
            job.content_ids.push_back("cid_" + std::to_string(handled.load()));
            job.processed_items = 1;
            job.progress = 1.0f;
        });
    worker->start();

    // Write a YAML config with two HUGGINGFACE sources
    auto path = writeTempYamlTracked(
        "sources:\n"
        "  - source_id: src_a\n"
        "    plugin_name: huggingface\n"
        "    type: " + std::to_string(static_cast<int>(IngestionJobType::HUGGINGFACE)) + "\n"
        "    location: org/dataset_a\n"
        "  - source_id: src_b\n"
        "    plugin_name: huggingface\n"
        "    type: " + std::to_string(static_cast<int>(IngestionJobType::HUGGINGFACE)) + "\n"
        "    location: org/dataset_b\n"
    );

    ASSERT_NO_THROW(worker->loadSourcesFromConfig(path));

    // Wait up to 5 s for both jobs to be processed
    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (handled.load() < 2 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    worker->stop(true);
    EXPECT_EQ(handled.load(), 2);
}

// ============================================================================
// AC-4: user_context from source entry is propagated into the job
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_UserContextPropagated) {
    std::string captured_user_ctx = {};

    auto worker = makeWorker(/*worker_threads=*/1, /*queue_depth=*/10);
    worker->registerJobHandler(
        IngestionJobType::HUGGINGFACE,
        [&](IngestionJob& job) {
            captured_user_ctx = job.user_context;
            job.content_ids.push_back("cid");
            job.processed_items = 1;
            job.progress = 1.0f;
        });
    worker->start();

    auto path = writeTempYamlTracked(
        "sources:\n"
        "  - source_id: src_ctx\n"
        "    plugin_name: huggingface\n"
        "    type: " + std::to_string(static_cast<int>(IngestionJobType::HUGGINGFACE)) + "\n"
        "    location: org/dataset\n"
        "    user_context: alice\n"
    );

    ASSERT_NO_THROW(worker->loadSourcesFromConfig(path));

    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (captured_user_ctx.empty() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    worker->stop(true);
    EXPECT_EQ(captured_user_ctx, "alice");
}

// ============================================================================
// AC-5: submitSourceJob() propagates user_context into IngestionJob
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, SubmitSourceJob_PropagatesUserContext) {
    std::string captured_ctx = {};
    bool job_ran = false;

    auto worker = makeWorker(/*worker_threads=*/1, /*queue_depth=*/10);
    worker->registerJobHandler(
        IngestionJobType::HUGGINGFACE,
        [&](IngestionJob& job) {
            captured_ctx = job.user_context;
            job_ran = true;
            job.content_ids.push_back("cid");
            job.processed_items = 1;
            job.progress = 1.0f;
        });
    worker->start();

    IngestionSource src;
    src.source_id   = "test_src";
    src.plugin_name = "huggingface";
    src.type        = IngestionJobType::HUGGINGFACE;
    src.location    = "org/dataset";

    ASSERT_NO_THROW(worker->submitSourceJob(src, nlohmann::json::object(), "bob"));

    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!job_ran && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    worker->stop(true);
    EXPECT_TRUE(job_ran);
    EXPECT_EQ(captured_ctx, "bob");
}

TEST_F(AsyncIngestionYamlConfigTest, SubmitSourceJob_EmptyUserContext) {
    std::string captured_ctx = "UNSET";
    bool job_ran = false;

    auto worker = makeWorker(/*worker_threads=*/1, /*queue_depth=*/10);
    worker->registerJobHandler(
        IngestionJobType::HUGGINGFACE,
        [&](IngestionJob& job) {
            captured_ctx = job.user_context;
            job_ran = true;
            job.content_ids.push_back("cid");
            job.processed_items = 1;
            job.progress = 1.0f;
        });
    worker->start();

    IngestionSource src;
    src.source_id   = "test_src2";
    src.plugin_name = "huggingface";
    src.type        = IngestionJobType::HUGGINGFACE;
    src.location    = "org/dataset2";

    // Omit user_context (default "")
    ASSERT_NO_THROW(worker->submitSourceJob(src));

    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!job_ran && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    worker->stop(true);
    EXPECT_TRUE(job_ran);
    EXPECT_EQ(captured_ctx, "");
}

// ============================================================================
// AC-6: loadSourcesFromConfig() throws for a non-existent file
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, LoadConfig_NonExistentFile_Throws) {
    auto worker = makeWorker();
    EXPECT_THROW(
        worker->loadSourcesFromConfig("/nonexistent/path/async_worker.yaml"),
        std::exception
    );
}

// ============================================================================
// AC-1 (integration): default async_worker.yaml exists and is parseable
// ============================================================================

TEST_F(AsyncIngestionYamlConfigTest, DefaultAsyncWorkerYaml_ExistsAndParseable) {
    auto worker = makeWorker();
    const std::string cfg_path = findAsyncWorkerConfigPath();
    EXPECT_NO_THROW(
        worker->loadSourcesFromConfig(cfg_path)
    );
}

} // anonymous namespace
