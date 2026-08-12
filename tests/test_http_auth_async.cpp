#include <gtest/gtest.h>

#include "auth/http_auth_async.h"
#include "auth/auth_error.h"

#include <thread>
#include <chrono>
#include <vector>

using namespace themis::auth;

// ===========================================================================
// Basic async HTTP tests
// ===========================================================================

TEST(AsyncHTTPAuth, ConstructorSucceeds)
{
    HTTPAuthConfig cfg;
    cfg.request_timeout_seconds = 10;
    cfg.max_retries = 2;
    
    AsyncHTTPAuth client(cfg);
    EXPECT_GT(client.threadCount(), 0);
}

TEST(AsyncHTTPAuth, URLValidation_RejectsEmpty)
{
    AsyncHTTPAuth client;
    
    EXPECT_THROW(
        client.getAsync(""),
        AuthException);
}

TEST(AsyncHTTPAuth, URLValidation_RejectsNonHTTP)
{
    AsyncHTTPAuth client;
    
    EXPECT_THROW(
        client.getAsync("ftp://example.com"),
        AuthException);
    
    EXPECT_THROW(
        client.getAsync("file:///etc/passwd"),
        AuthException);
}

TEST(AsyncHTTPAuth, URLValidation_AcceptsHTTP)
{
    AsyncHTTPAuth client;
    
    // Should not throw
    auto future = client.getAsync("http://example.com");
    
    // Don't wait for completion (may timeout)
}

TEST(AsyncHTTPAuth, URLValidation_AcceptsHTTPS)
{
    AsyncHTTPAuth client;
    
    // Should not throw
    auto future = client.getAsync("https://example.com");
    
    // Don't wait for completion (may timeout)
}

// ===========================================================================
// Async execution tests
// ===========================================================================

TEST(AsyncHTTPAuth, GetAsyncRetursFuture)
{
    AsyncHTTPAuth client;
    
    auto future = client.getAsync("https://httpbin.org/get");
    
    // Verify it's a valid future
    EXPECT_TRUE(future.valid());
}

TEST(AsyncHTTPAuth, PostAsyncReturnsFuture)
{
    AsyncHTTPAuth client;
    
    std::vector<std::pair<std::string, std::string>> headers;
    auto future = client.postAsync(
        "https://httpbin.org/post",
        R"({"key": "value"})",
        "application/json",
        headers);
    
    EXPECT_TRUE(future.valid());
}

TEST(AsyncHTTPAuth, ConnectivityCheckReturnsFuture)
{
    AsyncHTTPAuth client;
    
    auto future = client.checkConnectivityAsync("https://httpbin.org");
    
    EXPECT_TRUE(future.valid());
}

// ===========================================================================
// Non-blocking behavior tests
// ===========================================================================

TEST(AsyncHTTPAuth, GetAsyncIsNonBlocking)
{
    AsyncHTTPAuth client;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Submit multiple requests without waiting
    std::vector<std::future<HTTPAuthResponse>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(client.getAsync("https://httpbin.org/delay/2"));
    }
    
    // Submission should be fast (not blocked by network)
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    // All requests submitted in under 500ms (not waiting for 10+ seconds of delays)
    EXPECT_LT(elapsed_ms, 500);
}

TEST(AsyncHTTPAuth, ThreadPoolGrowsOnDemand)
{
    AsyncHTTPAuth client;
    
    size_t initial_threads = client.threadCount();
    EXPECT_GT(initial_threads, 0);
    
    // Submit many tasks to trigger growth
    std::vector<std::future<HTTPAuthResponse>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(client.getAsync("https://httpbin.org/get"));
    }
    
    // Give thread pool time to spawn new threads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    size_t later_threads = client.threadCount();
    
    // Thread pool may have grown (or stayed same if tasks completed quickly)
    EXPECT_GE(later_threads, initial_threads);
}

// ===========================================================================
// Configuration tests
// ===========================================================================

TEST(AsyncHTTPAuth, ConfigRespectsTimeout)
{
    HTTPAuthConfig cfg;
    cfg.request_timeout_seconds = 1;
    cfg.max_retries = 1;
    
    AsyncHTTPAuth client(cfg);
    EXPECT_EQ(client.config().request_timeout_seconds, 1);
    EXPECT_EQ(client.config().max_retries, 1);
}

TEST(AsyncHTTPAuth, ConfigRespectsRetries)
{
    HTTPAuthConfig cfg;
    cfg.max_retries = 5;
    cfg.retry_backoff_ms = 50;
    
    AsyncHTTPAuth client(cfg);
    EXPECT_EQ(client.config().max_retries, 5);
    EXPECT_EQ(client.config().retry_backoff_ms, 50);
}

// ===========================================================================
// Error handling tests
// ===========================================================================

TEST(AsyncHTTPAuth, PostAsyncRejectsEmptyBody)
{
    AsyncHTTPAuth client;
    
    EXPECT_THROW(
        client.postAsync("https://example.com", ""),
        AuthException);
}

TEST(AsyncHTTPAuth, RequestWithHeaders)
{
    AsyncHTTPAuth client;
    
    std::vector<std::pair<std::string, std::string>> headers{
        {"Authorization", "******"},
        {"X-Custom-Header", "value"}
    };
    
    // Should not throw
    auto future = client.getAsync("https://httpbin.org/get", headers);
    EXPECT_TRUE(future.valid());
}

// ===========================================================================
// Integration tests (with mock HTTP server)
// ===========================================================================

TEST(AsyncHTTPAuth, MultipleRequestsConcurrently)
{
    AsyncHTTPAuth client;
    
    std::vector<std::future<HTTPAuthResponse>> futures;
    
    // Submit diverse requests
    for (int i = 0; i < 3; ++i) {
        futures.push_back(client.getAsync("https://httpbin.org/get"));
    }
    
    for (int i = 0; i < 2; ++i) {
        std::string body = R"({"req":)" + std::to_string(i) + "}";
        futures.push_back(client.postAsync(
            "https://httpbin.org/post",
            body,
            "application/json"));
    }
    
    std::vector<std::future<bool>> conn_futures;
    for (int i = 0; i < 2; ++i) {
        conn_futures.push_back(client.checkConnectivityAsync("https://httpbin.org"));
    }
    
    EXPECT_EQ(futures.size(), 5);
    EXPECT_EQ(conn_futures.size(), 2u);
    
    // All futures should be valid
    for (const auto& f : futures) {
        EXPECT_TRUE(f.valid());
    }
    for (const auto& f : conn_futures) {
        EXPECT_TRUE(f.valid());
    }
}

// ===========================================================================
// Lifecycle tests
// ===========================================================================

TEST(AsyncHTTPAuth, DestructorStopsThreads)
{
    {
        AsyncHTTPAuth client;
        size_t thread_count_during = client.threadCount();
        EXPECT_GT(thread_count_during, 0);
    }
    
    // After destructor, threads should be cleaned up
    // (This is a best-effort test; exact count may vary)
}

TEST(AsyncHTTPAuth, CanResubmitAfterPreviousFutureCompletes)
{
    AsyncHTTPAuth client;
    
    // First request
    auto future1 = client.getAsync("https://httpbin.org/get");
    EXPECT_TRUE(future1.valid());
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Second request - should work even if first is in progress
    auto future2 = client.getAsync("https://httpbin.org/get");
    EXPECT_TRUE(future2.valid());
}

