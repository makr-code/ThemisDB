/**
 * @file test_parallel_file_downloads.cpp
 * @brief Focused unit tests for ParallelDownloader (Issue #128 / v1.6.0)
 *
 * Covers all four acceptance criteria:
 *   AC1 – Configurable concurrency level
 *   AC2 – Bandwidth throttling
 *   AC3 – Priority queue for critical files
 *   AC4 – Resume support per file
 *
 * Tests are self-contained and do not hit the network; all HTTP transfers
 * are driven by an injected FetchFn stub.
 */

#include <gtest/gtest.h>

#include "updates/parallel_downloader.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::updates;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Write `content` to `path`, creating parent dirs as needed.
static void writeFile(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
}

/// Return a temp directory path unique to this test run.
static std::string tmpDir(const std::string& label) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_pdl_" + label + "_" + std::to_string(ts)))
               .string();
}

/// Compute SHA-256 hex-string of `data` using OpenSSL (mirrors the private helper).
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
static std::string sha256hex(const std::string& data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;
    EVP_DigestFinal_ex(ctx, digest, &len);
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < len; ++i)
        oss << std::setw(2) << static_cast<int>(digest[i]);
    return oss.str();
}

/**
 * @brief Build a FetchFn that writes `content` to `dest` and reports success.
 *
 * @param content  String to write as the "downloaded" file body.
 * @param append   If true, append (simulates the resume path).
 */
static ParallelDownloader::FetchFn makeFetchFn(
    const std::string& content,
    bool append = false)
{
    return [content, append](
        const std::string& /*url*/,
        const std::string& dest,
        uint64_t           resume_offset,
        long               /*connect_s*/,
        long               /*transfer_s*/,
        uint64_t*          out_bytes,
        uint64_t*          out_total,
        std::string*       out_error) -> bool
    {
        const auto mode =
            (append && resume_offset > 0)
                ? (std::ios::binary | std::ios::app)
                : (std::ios::binary | std::ios::trunc);
        std::ofstream f(dest, mode);
        if (!f.is_open()) {
            if (out_error) {
              *out_error = "cannot open dest";
            }
            return false;
        }
        // Write only the bytes beyond resume_offset
        const size_t write_from = static_cast<size_t>(
            std::min(resume_offset, static_cast<uint64_t>(content.size())));
        const std::string_view tail(content.data() + write_from,
                                    content.size() - write_from);
        f.write(tail.data(), static_cast<std::streamsize>(tail.size()));
        if (out_bytes) {
          *out_bytes = tail.size();
        }
        if (out_total) {
          *out_total = content.size();
        }
        if (out_error) *out_error = {};
        return true;
    };
}

/// FetchFn that always fails.
static bool alwaysFail(
    const std::string&, const std::string&, uint64_t,
    long, long, uint64_t* b, uint64_t* t, std::string* e)
{
    if (b) {
      *b = 0;
    }
    if (t) {
      *t = 0;
    }
    if (e) {
      *e = "injected failure";
    }
    return false;
}

} // anonymous namespace

// ============================================================================
// AC1 – Configurable concurrency level
// ============================================================================

class ParallelDownloaderConcurrencyTest : public ::testing::Test {};

TEST_F(ParallelDownloaderConcurrencyTest, DefaultConcurrencyIsPositive) {
    ParallelDownloader dl;
    EXPECT_GE(dl.getConcurrency(), 1u);
}

TEST_F(ParallelDownloaderConcurrencyTest, SetConcurrencyRoundTrip) {
    ParallelDownloader dl;
    dl.setConcurrency(4);
    EXPECT_EQ(dl.getConcurrency(), 4u);
}

TEST_F(ParallelDownloaderConcurrencyTest, SetConcurrencyToOne) {
    ParallelDownloader dl;
    dl.setConcurrency(1);
    EXPECT_EQ(dl.getConcurrency(), 1u);
}

TEST_F(ParallelDownloaderConcurrencyTest, SetConcurrencyToLargeValue) {
    ParallelDownloader dl;
    dl.setConcurrency(32);
    EXPECT_EQ(dl.getConcurrency(), 32u);
}

TEST_F(ParallelDownloaderConcurrencyTest, ZeroConcurrencyThrows) {
    ParallelDownloader dl;
    EXPECT_THROW(dl.setConcurrency(0), std::invalid_argument);
}

TEST_F(ParallelDownloaderConcurrencyTest, MultipleTasksRunConcurrently) {
    // Inject a fetch function that records the maximum observed in-flight count.
    std::atomic<int> in_flight{0};
    std::atomic<int> peak_in_flight{0};

    ParallelDownloader::FetchFn fn =
        [&in_flight, &peak_in_flight](
            const std::string&, const std::string& dest,
            uint64_t, long, long, uint64_t* b, uint64_t* t, std::string*) -> bool
    {
        const int current = ++in_flight;
        int expected = peak_in_flight.load();
        while (current > expected &&
               !peak_in_flight.compare_exchange_weak(expected, current)) {}

        // Simulate a small amount of work so threads overlap
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        --in_flight;
        std::ofstream(dest, std::ios::binary | std::ios::trunc).put('x');
        if (b) {
          *b = 1;
        }
        if (t) {
          *t = 1;
        }
        return true;
    };

    const std::string dir = tmpDir("concurrency");
    ParallelDownloader dl;
    dl.setConcurrency(4);
    dl.setFetchFunction(fn);

    std::vector<DownloadTask> tasks;
    for (int i = 0; i < 8; ++i) {
        DownloadTask task;
        task.url  = "https://example.com/f" + std::to_string(i);
        task.dest = dir + "/f" + std::to_string(i);
        tasks.push_back(task);
    }

    const auto results = dl.downloadAll(tasks);

    EXPECT_EQ(results.size(), 8u);
    for (const auto& r : results) {
      EXPECT_TRUE(r.success);
    }

    // With concurrency = 4 and 8 tasks, we should see at least 2 in-flight
    EXPECT_GE(peak_in_flight.load(), 2);

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderConcurrencyTest, EmptyTaskListReturnsEmpty) {
    ParallelDownloader dl;
    dl.setFetchFunction(&alwaysFail);
    const auto results = dl.downloadAll({});
    EXPECT_TRUE(results.empty());
}

TEST_F(ParallelDownloaderConcurrencyTest, SingleTaskSucceeds) {
    const std::string dir  = tmpDir("single");
    const std::string body = "hello world";

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(body));

    DownloadTask task;
    task.url  = "https://example.com/file";
    task.dest = dir + "/file.txt";

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderConcurrencyTest, BatchStatsAggregated) {
    const std::string dir  = tmpDir("stats");
    const std::string body = "data";

    ParallelDownloader dl;
    dl.setConcurrency(2);
    dl.setFetchFunction(makeFetchFn(body));

    std::vector<DownloadTask> tasks(4);
    for (int i = 0; i < 4; ++i) {
        tasks[i].url  = "https://example.com/f" + std::to_string(i);
        tasks[i].dest = dir + "/f" + std::to_string(i);
    }

    dl.downloadAll(tasks);
    const auto stats = dl.lastBatchStats();

    EXPECT_EQ(stats.total_tasks, 4u);
    EXPECT_EQ(stats.succeeded,   4u);
    EXPECT_EQ(stats.failed,      0u);
    EXPECT_GE(stats.total_bytes, 4u * body.size());

    fs::remove_all(dir);
}

// ============================================================================
// AC2 – Bandwidth throttling
// ============================================================================

class ParallelDownloaderBandwidthTest : public ::testing::Test {};

TEST_F(ParallelDownloaderBandwidthTest, DefaultBandwidthLimitIsZero) {
    ParallelDownloader dl;
    EXPECT_EQ(dl.getBandwidthLimit(), 0u);
}

TEST_F(ParallelDownloaderBandwidthTest, SetBandwidthLimitRoundTrip) {
    ParallelDownloader dl;
    dl.setBandwidthLimit(100ULL * 1024 * 1024);
    EXPECT_EQ(dl.getBandwidthLimit(), 100ULL * 1024 * 1024);
}

TEST_F(ParallelDownloaderBandwidthTest, ZeroBandwidthMeansUnlimited) {
    ParallelDownloader dl;
    dl.setBandwidthLimit(0);
    EXPECT_EQ(dl.getBandwidthLimit(), 0u);
}

TEST_F(ParallelDownloaderBandwidthTest, DownloadSucceedsWithThrottleSet) {
    // With a generous throttle the download should still complete quickly.
    const std::string dir  = tmpDir("bw");
    const std::string body = "throttled content";

    ParallelDownloader dl;
    dl.setConcurrency(2);
    dl.setBandwidthLimit(10ULL * 1024 * 1024);  // 10 MB/s
    dl.setFetchFunction(makeFetchFn(body));

    std::vector<DownloadTask> tasks(3);
    for (int i = 0; i < 3; ++i) {
        tasks[i].url  = "https://example.com/f" + std::to_string(i);
        tasks[i].dest = dir + "/f" + std::to_string(i);
    }

    const auto results = dl.downloadAll(tasks);
    for (const auto& r : results) {
      EXPECT_TRUE(r.success);
    }

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderBandwidthTest, BytesDownloadedReflected) {
    const std::string dir  = tmpDir("bw_bytes");
    const std::string body(512, 'A');

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setBandwidthLimit(0);
    dl.setFetchFunction(makeFetchFn(body));

    DownloadTask task;
    task.url  = "https://example.com/big";
    task.dest = dir + "/big.bin";

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
    EXPECT_EQ(results[0].bytes_downloaded, body.size());

    fs::remove_all(dir);
}

// ============================================================================
// AC3 – Priority queue for critical files
// ============================================================================

class ParallelDownloaderPriorityTest : public ::testing::Test {};

TEST_F(ParallelDownloaderPriorityTest, HighPriorityTasksScheduledFirst) {
    // With concurrency = 1 all tasks run sequentially; the order must follow
    // descending priority.
    std::vector<int> execution_order;
    std::mutex        order_mutex;

    // task 0: priority 1  (low)
    // task 1: priority 10 (high)
    // task 2: priority 5  (medium)
    // Expected execution order by index: 1, 2, 0

    ParallelDownloader::FetchFn fn =
        [&execution_order, &order_mutex](
            const std::string& url,
            const std::string& dest,
            uint64_t, long, long,
            uint64_t* b, uint64_t* t, std::string*) -> bool
    {
        // Extract the task index from the URL suffix
        const int idx = std::stoi(url.substr(url.rfind('_') + 1));
        {
            std::lock_guard<std::mutex> lk(order_mutex);
            execution_order.push_back(idx);
        }
        std::ofstream(dest, std::ios::binary | std::ios::trunc).put('x');
        if (b) {
          *b = 1;
        }
        if (t) {
          *t = 1;
        }
        return true;
    };

    const std::string dir = tmpDir("priority");
    ParallelDownloader dl;
    dl.setConcurrency(1);   // serial so order is deterministic
    dl.setFetchFunction(fn);

    std::vector<DownloadTask> tasks(3);
    tasks[0].url      = "https://example.com/file_0";
    tasks[0].dest     = dir + "/f0";
    tasks[0].priority = 1;

    tasks[1].url      = "https://example.com/file_1";
    tasks[1].dest     = dir + "/f1";
    tasks[1].priority = 10;

    tasks[2].url      = "https://example.com/file_2";
    tasks[2].dest     = dir + "/f2";
    tasks[2].priority = 5;

    const auto results = dl.downloadAll(tasks);
    for (const auto& r : results) {
      EXPECT_TRUE(r.success);
    }

    // Verify execution order: highest priority first
    ASSERT_EQ(execution_order.size(), 3u);
    EXPECT_EQ(execution_order[0], 1);  // priority 10
    EXPECT_EQ(execution_order[1], 2);  // priority 5
    EXPECT_EQ(execution_order[2], 0);  // priority 1

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderPriorityTest, EqualPriorityAllComplete) {
    const std::string dir  = tmpDir("eq_pri");
    const std::string body = "equal";

    ParallelDownloader dl;
    dl.setConcurrency(2);
    dl.setFetchFunction(makeFetchFn(body));

    std::vector<DownloadTask> tasks(6);
    for (int i = 0; i < 6; ++i) {
        tasks[i].url      = "https://example.com/f" + std::to_string(i);
        tasks[i].dest     = dir + "/f" + std::to_string(i);
        tasks[i].priority = 5;
    }

    const auto results = dl.downloadAll(tasks);
    EXPECT_EQ(results.size(), 6u);
    for (const auto& r : results) {
      EXPECT_TRUE(r.success);
    }

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderPriorityTest, ResultsInInputOrder) {
    // Results must be returned in the original task order regardless of
    // scheduling order.
    const std::string dir  = tmpDir("result_order");
    const std::string body = "x";

    ParallelDownloader dl;
    dl.setConcurrency(4);
    dl.setFetchFunction(makeFetchFn(body));

    const int N = 10;
    std::vector<DownloadTask> tasks(N);
    for (int i = 0; i < N; ++i) {
        tasks[i].url      = "https://example.com/f" + std::to_string(i);
        tasks[i].dest     = dir + "/f" + std::to_string(i);
        tasks[i].priority = N - i;  // descending so scheduling != input order
    }

    const auto results = dl.downloadAll(tasks);
    ASSERT_EQ(static_cast<int>(results.size()), N);
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(results[i].task.url, tasks[i].url)
            << "Result at index " << i << " does not match input task";
    }

    fs::remove_all(dir);
}

// ============================================================================
// AC4 – Resume support per file
// ============================================================================

class ParallelDownloaderResumeTest : public ::testing::Test {};

TEST_F(ParallelDownloaderResumeTest, ResumeEnabledByDefault) {
    DownloadTask task;
    EXPECT_TRUE(task.enable_resume);
}

TEST_F(ParallelDownloaderResumeTest, PartialFileIsResumed) {
    // Write a partial file, then download the rest.
    const std::string dir     = tmpDir("resume");
    const std::string dest    = dir + "/file.bin";
    const std::string content = "HELLO_WORLD";

    fs::create_directories(dir);
    // Pre-write the first 6 bytes ("HELLO_")
    writeFile(dest, "HELLO_");

    // Fetch function appends from the resume offset
    ParallelDownloader::FetchFn fn = makeFetchFn(content, /*append=*/true);

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(fn);

    DownloadTask task;
    task.url           = "https://example.com/file";
    task.dest          = dest;
    task.enable_resume = true;

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
    EXPECT_TRUE(results[0].was_resumed);
    // Only the remaining 5 bytes ("WORLD") were downloaded in this session
    EXPECT_EQ(results[0].bytes_downloaded, content.size() - 6u);

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderResumeTest, FreshDownloadWhenResumeDisabled) {
    const std::string dir     = tmpDir("no_resume");
    const std::string dest    = dir + "/file.bin";
    const std::string content = "FRESH";

    fs::create_directories(dir);
    writeFile(dest, "STALE");  // existing partial file

    ParallelDownloader::FetchFn fn = makeFetchFn(content, /*append=*/false);

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(fn);

    DownloadTask task;
    task.url           = "https://example.com/file";
    task.dest          = dest;
    task.enable_resume = false;

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[0].was_resumed);

    // Destination must contain only the fresh content
    {
        std::ifstream f(dest, std::ios::binary);
        const std::string actual{std::istreambuf_iterator<char>(f),
                                  std::istreambuf_iterator<char>()};
        EXPECT_EQ(actual, content);
    }

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderResumeTest, NoExistingFileNoResume) {
    const std::string dir     = tmpDir("fresh");
    const std::string content = "BRAND_NEW";

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(content));

    DownloadTask task;
    task.url           = "https://example.com/new";
    task.dest          = dir + "/new.bin";
    task.enable_resume = true;

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[0].was_resumed);  // no existing file → no resume
    EXPECT_EQ(results[0].bytes_downloaded, content.size());

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderResumeTest, MultipleFilesIndependentResume) {
    // File 0: partial (resume expected)
    // File 1: does not exist (fresh download)
    const std::string dir = tmpDir("multi_resume");
    fs::create_directories(dir);

    const std::string body0 = "ABCDEFGHIJ";
    const std::string body1 = "XYZ";

    // Pre-write 5 bytes of file 0
    writeFile(dir + "/f0", body0.substr(0, 5));

    // Fetch functions that remember which files they handled
    std::vector<bool> was_appended(2, false);

    ParallelDownloader::FetchFn fn0 =
        [&was_appended, body0](
            const std::string&, const std::string& dest,
            uint64_t resume_offset, long, long,
            uint64_t* b, uint64_t* t, std::string*) -> bool
    {
        was_appended[0] = (resume_offset > 0);
        const auto mode = (resume_offset > 0)
            ? (std::ios::binary | std::ios::app)
            : (std::ios::binary | std::ios::trunc);
        std::ofstream f(dest, mode);
        const std::string_view tail(body0.data() + resume_offset,
                                    body0.size() - resume_offset);
        f.write(tail.data(), static_cast<std::streamsize>(tail.size()));
        if (b) {
          *b = tail.size();
        }
        if (t) {
          *t = body0.size();
        }
        return true;
    };

    ParallelDownloader::FetchFn fn1 = makeFetchFn(body1);

    // Use a muxing fetch function that dispatches by dest suffix
    ParallelDownloader::FetchFn mux =
        [fn0, fn1, &dir](
            const std::string& url,
            const std::string& dest,
            uint64_t ro, long cs, long ts,
            uint64_t* b, uint64_t* t, std::string* e) -> bool
    {
        if (dest.find("f0") != std::string::npos)
            return fn0(url, dest, ro, cs, ts, b, t, e);
        return fn1(url, dest, ro, cs, ts, b, t, e);
    };

    ParallelDownloader dl;
    dl.setConcurrency(2);
    dl.setFetchFunction(mux);

    DownloadTask t0;
    t0.url           = "https://example.com/f0";
    t0.dest          = dir + "/f0";
    t0.enable_resume = true;

    DownloadTask t1;
    t1.url           = "https://example.com/f1";
    t1.dest          = dir + "/f1";
    t1.enable_resume = true;

    const auto results = dl.downloadAll({t0, t1});
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].success);
    EXPECT_TRUE(results[1].success);
    EXPECT_TRUE(results[0].was_resumed);
    EXPECT_FALSE(results[1].was_resumed);

    fs::remove_all(dir);
}

// ============================================================================
// Hash verification
// ============================================================================

class ParallelDownloaderHashTest : public ::testing::Test {};

TEST_F(ParallelDownloaderHashTest, CorrectHashAccepted) {
    const std::string dir  = tmpDir("hash_ok");
    const std::string body = "the quick brown fox";
    const std::string hash = sha256hex(body);

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(body));

    DownloadTask task;
    task.url           = "https://example.com/file";
    task.dest          = dir + "/file.txt";
    task.expected_hash = hash;

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderHashTest, WrongHashRejectedAndFileRemoved) {
    const std::string dir  = tmpDir("hash_fail");
    const std::string body = "hello";

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(body));

    DownloadTask task;
    task.url           = "https://example.com/file";
    task.dest          = dir + "/file.txt";
    task.expected_hash = std::string(64, '0');  // wrong hash

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].success);
    EXPECT_NE(std::string::npos, results[0].error_message.find("Hash mismatch"));

    // The corrupt file must be removed
    EXPECT_FALSE(fs::exists(task.dest));

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderHashTest, EmptyHashSkipsVerification) {
    const std::string dir  = tmpDir("no_hash");
    const std::string body = "no check needed";

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(body));

    DownloadTask task;
    task.url           = "https://example.com/file";
    task.dest          = dir + "/file.txt";
    task.expected_hash = "";  // intentionally empty

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);

    fs::remove_all(dir);
}

// ============================================================================
// Error handling & retries
// ============================================================================

class ParallelDownloaderErrorTest : public ::testing::Test {};

TEST_F(ParallelDownloaderErrorTest, FailedTaskReportedNotThrown) {
    const std::string dir = tmpDir("fail");
    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(&alwaysFail);

    DownloadTask task;
    task.url        = "https://example.com/bad";
    task.dest       = dir + "/bad.bin";
    task.max_retries = 0;  // no retries

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].success);
    EXPECT_FALSE(results[0].error_message.empty());

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderErrorTest, RetriesConsumedOnFailure) {
    std::atomic<int> call_count{0};
    ParallelDownloader::FetchFn fn =
        [&call_count](
            const std::string&, const std::string&,
            uint64_t, long, long,
            uint64_t* b, uint64_t* t, std::string* e) -> bool
    {
        ++call_count;
        if (b) {
          *b = 0;
        }
        if (t) {
          *t = 0;
        }
        if (e) {
          *e = "transient";
        }
        return false;
    };

    const std::string dir = tmpDir("retry");
    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(fn);

    DownloadTask task;
    task.url         = "https://example.com/retry";
    task.dest        = dir + "/retry.bin";
    task.max_retries = 2;

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].success);
    // 1 initial attempt + 2 retries = 3 total calls
    EXPECT_EQ(call_count.load(), 3);
    EXPECT_EQ(results[0].retries_used, 2);

    fs::remove_all(dir);
}

TEST_F(ParallelDownloaderErrorTest, OneFailDoesNotPreventOthersFromSucceeding) {
    std::atomic<int> call_index{0};
    const std::string dir  = tmpDir("partial_fail");
    const std::string body = "ok";

    // task 0 and 2 succeed; task 1 fails
    ParallelDownloader::FetchFn fn =
        [&call_index, &dir, body](
            const std::string& url,
            const std::string& dest,
            uint64_t, long, long,
            uint64_t* b, uint64_t* t, std::string* e) -> bool
    {
        if (url.find("fail") != std::string::npos) {
            if (e) {
              *e = "injected";
            }
            return false;
        }
        std::ofstream(dest, std::ios::binary | std::ios::trunc)
            .write(body.data(), static_cast<std::streamsize>(body.size()));
        if (b) {
          *b = body.size();
        }
        if (t) {
          *t = body.size();
        }
        return true;
    };

    ParallelDownloader dl;
    dl.setConcurrency(3);
    dl.setFetchFunction(fn);

    DownloadTask t0; t0.url = "https://example.com/ok0";  t0.dest = dir + "/f0"; t0.max_retries = 0;
    DownloadTask t1; t1.url = "https://example.com/fail"; t1.dest = dir + "/f1"; t1.max_retries = 0;
    DownloadTask t2; t2.url = "https://example.com/ok2";  t2.dest = dir + "/f2"; t2.max_retries = 0;

    const auto results = dl.downloadAll({t0, t1, t2});
    ASSERT_EQ(results.size(), 3u);
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[1].success);
    EXPECT_TRUE(results[2].success);

    const auto stats = dl.lastBatchStats();
    EXPECT_EQ(stats.succeeded, 2u);
    EXPECT_EQ(stats.failed,    1u);

    fs::remove_all(dir);
}

// ============================================================================
// Progress callback
// ============================================================================

class ParallelDownloaderProgressTest : public ::testing::Test {};

TEST_F(ParallelDownloaderProgressTest, ProgressCallbackInvoked) {
    const std::string dir  = tmpDir("progress");
    const std::string body = "progress test body";

    std::atomic<int>        cb_count{0};
    std::vector<std::string> messages;
    std::mutex               msg_mutex;

    ParallelDownloader dl;
    dl.setConcurrency(1);
    dl.setFetchFunction(makeFetchFn(body));
    dl.setProgressCallback(
        [&cb_count, &messages, &msg_mutex](
            size_t, uint64_t, uint64_t, const std::string& msg)
        {
            ++cb_count;
            std::lock_guard<std::mutex> lk(msg_mutex);
            messages.push_back(msg);
        });

    DownloadTask task;
    task.url  = "https://example.com/progress";
    task.dest = dir + "/progress.bin";

    const auto results = dl.downloadAll({task});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
    EXPECT_GE(cb_count.load(), 2);  // at least "downloading" + "done"

    const bool has_done =
        std::any_of(messages.begin(), messages.end(),
                    [](const std::string& m) { return m == "done"; });
    EXPECT_TRUE(has_done);

    fs::remove_all(dir);
}
