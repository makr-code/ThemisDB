/**
 * @file mock_provider_factory.h
 * @brief Header-only mock provider factory for AQL Phase 5 unified testing.
 *
 * Provides MockInferProvider, MockRAGProvider, and MockEmbedProvider with
 * configurable failure modes (success, fail-after-N, timeout, latency injection).
 *
 * Usage:
 * @code
 *   MockInferProvider infer(MockProviderConfig::FailAfterN(3));
 *   auto result = infer.infer("SELECT users WHERE ...");
 *   EXPECT_TRUE(result.success);  // First 3 calls succeed
 * @endcode
 *
 * @note All providers are header-only and thread-safe for concurrent test use.
 * @note No real LLM/embedding infrastructure is required.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "aql/aql_error_types.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Provider Configuration
// ============================================================================

/// @brief Configures the failure behavior of a mock provider.
struct MockProviderConfig {
    enum class FailureMode {
        ALWAYS_SUCCEED,        ///< All requests succeed
        ALWAYS_FAIL,           ///< All requests fail with provider error
        FAIL_AFTER_N,          ///< First N requests succeed, then always fail
        TIMEOUT_AFTER_N,       ///< First N requests succeed, then always time out
        INTERMITTENT_FAIL,     ///< Fails every Nth request (cycle)
    };

    FailureMode mode          = FailureMode::ALWAYS_SUCCEED;
    int         threshold_n   = 0;       ///< N for FAIL_AFTER_N / TIMEOUT_AFTER_N / INTERMITTENT_FAIL
    int         latency_ms    = 0;       ///< Artificial latency per call (ms)
    std::string error_message = "Mock provider error";

    /// Factory: always succeed
    static MockProviderConfig AlwaysSucceed() {
        return {FailureMode::ALWAYS_SUCCEED, 0, 0, ""};
    }
    /// Factory: always fail
    static MockProviderConfig AlwaysFail(const std::string& msg = "Provider unavailable") {
        return {FailureMode::ALWAYS_FAIL, 0, 0, msg};
    }
    /// Factory: succeed N times then fail
    static MockProviderConfig FailAfterN(int n, const std::string& msg = "Sustained failure") {
        return {FailureMode::FAIL_AFTER_N, n, 0, msg};
    }
    /// Factory: succeed N times then time out
    static MockProviderConfig TimeoutAfterN(int n, int latency_ms = 100) {
        return {FailureMode::TIMEOUT_AFTER_N, n, latency_ms, "Provider timeout"};
    }
    /// Factory: fail every Nth call
    static MockProviderConfig IntermittentFail(int every_n, const std::string& msg = "Intermittent failure") {
        return {FailureMode::INTERMITTENT_FAIL, every_n, 0, msg};
    }
};

// ============================================================================
// Shared Call Tracker (thread-safe)
// ============================================================================

class MockCallTracker {
public:
    void recordCall(bool success) {
        total_calls_.fetch_add(1, std::memory_order_relaxed);
        if (success) {
            success_calls_.fetch_add(1, std::memory_order_relaxed);
        } else {
            failure_calls_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    int totalCalls()   const { return total_calls_.load(std::memory_order_relaxed); }
    int successCalls() const { return success_calls_.load(std::memory_order_relaxed); }
    int failureCalls() const { return failure_calls_.load(std::memory_order_relaxed); }
    void reset() {
        total_calls_.store(0);
        success_calls_.store(0);
        failure_calls_.store(0);
    }

private:
    std::atomic<int> total_calls_{0};
    std::atomic<int> success_calls_{0};
    std::atomic<int> failure_calls_{0};
};

// ============================================================================
// Result Type
// ============================================================================

struct MockProviderResult {
    bool        success       = false;
    std::string response;        ///< Response content on success
    std::string error_message;   ///< Error message on failure
    std::string error_category;  ///< AQL error category string

    static MockProviderResult Ok(const std::string& response) {
        return {true, response, "", ""};
    }
    static MockProviderResult Fail(const std::string& msg, const std::string& category = ProviderError::InferFailed) {
        return {false, "", msg, category};
    }
    static MockProviderResult Timeout(const std::string& msg = "Provider request timed out") {
        return {false, "", msg, ProviderError::ProviderTimeout};
    }
};

// ============================================================================
// Base Mock Provider
// ============================================================================

class MockProviderBase {
public:
    explicit MockProviderBase(const MockProviderConfig& cfg = MockProviderConfig::AlwaysSucceed(),
                               std::shared_ptr<MockCallTracker> tracker = nullptr)
        : config_(cfg)
        , call_count_(0)
        , tracker_(tracker ? tracker : std::make_shared<MockCallTracker>())
    {}

    virtual ~MockProviderBase() = default;

    MockCallTracker& tracker() { return *tracker_; }
    const MockCallTracker& tracker() const { return *tracker_; }

protected:
    /// Evaluate whether this call should succeed or fail
    MockProviderResult evaluateCall(const std::string& success_response) {
        int n = call_count_.fetch_add(1, std::memory_order_relaxed) + 1;

        if (config_.latency_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.latency_ms));
        }

        bool should_fail = false;
        bool is_timeout  = false;

        switch (config_.mode) {
            case MockProviderConfig::FailureMode::ALWAYS_SUCCEED:
                break;
            case MockProviderConfig::FailureMode::ALWAYS_FAIL:
                should_fail = true;
                break;
            case MockProviderConfig::FailureMode::FAIL_AFTER_N:
                should_fail = (n > config_.threshold_n);
                break;
            case MockProviderConfig::FailureMode::TIMEOUT_AFTER_N:
                if (n > config_.threshold_n) { should_fail = true; is_timeout = true; }
                break;
            case MockProviderConfig::FailureMode::INTERMITTENT_FAIL:
                if (config_.threshold_n > 0) {
                    should_fail = (n % config_.threshold_n == 0);
                }
                break;
        }

        MockProviderResult result = {};
        if (should_fail) {
            result = is_timeout ? MockProviderResult::Timeout()
                                : MockProviderResult::Fail(config_.error_message);
        } else {
            result = MockProviderResult::Ok(success_response);
        }

        tracker_->recordCall(result.success);
        return result;
    }

private:
    MockProviderConfig config_;
    std::atomic<int>   call_count_;
    std::shared_ptr<MockCallTracker> tracker_;
};

// ============================================================================
// MockInferProvider — simulates LLM inference
// ============================================================================

class MockInferProvider : public MockProviderBase {
public:
    explicit MockInferProvider(const MockProviderConfig& cfg = MockProviderConfig::AlwaysSucceed(),
                                std::shared_ptr<MockCallTracker> tracker = nullptr)
        : MockProviderBase(cfg, tracker) {}

    /// Simulate an inference call. Returns an AQL-like response.
    MockProviderResult infer(const std::string& prompt) {
        // Deterministic mock AQL response based on prompt length
        std::string mock_aql = "FOR doc IN collection FILTER doc.id == "
                               + std::to_string(prompt.size() % 1000)
                               + " RETURN doc";
        return evaluateCall(mock_aql);
    }
};

// ============================================================================
// MockRAGProvider — simulates RAG context retrieval
// ============================================================================

class MockRAGProvider : public MockProviderBase {
public:
    explicit MockRAGProvider(const MockProviderConfig& cfg = MockProviderConfig::AlwaysSucceed(),
                              std::shared_ptr<MockCallTracker> tracker = nullptr)
        : MockProviderBase(cfg, tracker) {}

    /// Simulate a RAG retrieval call. Returns mock few-shot examples.
    MockProviderResult retrieve(const std::string& query) {
        std::string mock_context = "Example: FOR u IN users FILTER u.name == 'Alice' RETURN u\n"
                                   "Query: " + query.substr(0, std::min(query.size(), std::size_t(40)));
        return evaluateCall(mock_context);
    }
};

// ============================================================================
// MockEmbedProvider — simulates embedding generation
// ============================================================================

class MockEmbedProvider : public MockProviderBase {
public:
    explicit MockEmbedProvider(const MockProviderConfig& cfg = MockProviderConfig::AlwaysSucceed(),
                                std::shared_ptr<MockCallTracker> tracker = nullptr)
        : MockProviderBase(cfg, tracker) {}

    struct EmbeddingResult {
        bool success = false;
        std::vector<float> embedding;
        std::string error_message = {};
        std::string error_category = {};
    };

    /// Simulate an embedding generation call. Returns a 128-dim mock embedding.
    EmbeddingResult embed(const std::string& text) {
        auto base_result = evaluateCall("embedding_ok");
        if (!base_result.success) {
            return {false, {}, base_result.error_message, base_result.error_category};
        }
        // Deterministic 128-dim mock embedding: each element = (char_code % 100) / 100.0f
        std::vector<float> vec(128);
        for (std::size_t i = 0; i < vec.size(); ++i) {
            vec[i] = static_cast<float>((text.empty() ? 0 : text[i % text.size()]) % 100) / 100.0f;
        }
        return {true, vec, "", ""};
    }
};

// ============================================================================
// Factory
// ============================================================================

/// @brief Create all three mock providers sharing a single call tracker.
struct MockProviderSet {
    std::shared_ptr<MockCallTracker> tracker;
    std::shared_ptr<MockInferProvider>  infer;
    std::shared_ptr<MockRAGProvider>    rag;
    std::shared_ptr<MockEmbedProvider>  embed;
};

inline MockProviderSet makeMockProviderSet(
    const MockProviderConfig& infer_cfg = MockProviderConfig::AlwaysSucceed(),
    const MockProviderConfig& rag_cfg   = MockProviderConfig::AlwaysSucceed(),
    const MockProviderConfig& embed_cfg = MockProviderConfig::AlwaysSucceed())
{
    auto tracker = std::make_shared<MockCallTracker>();
    return {
        tracker,
        std::make_shared<MockInferProvider>(infer_cfg, tracker),
        std::make_shared<MockRAGProvider>(rag_cfg, tracker),
        std::make_shared<MockEmbedProvider>(embed_cfg, tracker)
    };
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
