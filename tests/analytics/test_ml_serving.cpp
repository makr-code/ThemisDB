/**
 * ML Serving Integration – unit tests.
 *
 * These tests are designed to pass regardless of whether ONNX Runtime or
 * TF Serving are compiled in.  When a backend is unavailable the tests
 * verify graceful degradation (UNAVAILABLE status, no crash).
 *
 * Covers:
 *  - MLTensor::numElements() for various shape configurations
 *  - MLServingClient default construction (AUTO backend)
 *  - MLServingClient::isBackendAvailable() for all backend types
 *  - MLServingClient::activeBackendName() is non-empty
 *  - ONNXServingBackend: backendName() is non-empty
 *  - ONNXServingBackend::isAvailable() reflects compile-time flag
 *  - ONNXServingBackend::infer() returns UNAVAILABLE when backend absent
 *  - ONNXServingBackend::infer() returns INVALID_INPUT for empty request
 *  - TFServingBackend: backendName() is non-empty
 *  - TFServingBackend::isAvailable() reflects compile-time flags
 *  - TFServingBackend::infer() returns UNAVAILABLE when backend absent
 *  - TFServingBackend::infer() returns INVALID_INPUT for empty request
 *  - MLServingClient::infer() when no backend available → UNAVAILABLE
 *  - MLServingClient::inferFromDataPoint() with numeric DataPoint
 *  - MLServingClient::inferFromDataPoint() with empty DataPoint → INVALID_INPUT
 *  - Factory methods makeONNXBackend / makeTFServingBackend return non-null
 *  - mlServingStatusName() covers all status codes
 *  - mlBackendTypeName() covers all backend types
 *  - MLServingRequest / MLServingResponse fields are zero-initialised
 *  - Forced ONNX_RUNTIME / TF_SERVING backend construction does not crash
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include "analytics/ml_serving.h"

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

/// Build a DataPoint with n numeric features f0..f(n-1).
static DataPoint makeNumericPoint(int n, double base = 1.0) {
    DataPoint p;
    p.id           = "dp_test";
    p.timestamp_ms = 1000LL;
    for (int i = 0; i < n; ++i) {
        p.set("f" + std::to_string(i), base + static_cast<double>(i) * 0.1);
    }
    return p;
}

// ============================================================================
// MLTensor helpers
// ============================================================================

TEST(MLTensorTest, NumElementsEmpty) {
    MLTensor t;
    EXPECT_EQ(0u, t.numElements());
}

TEST(MLTensorTest, NumElementsDataOnly) {
    MLTensor t;
    t.data = {1.f, 2.f, 3.f};
    // No shape set → falls back to data.size()
    EXPECT_EQ(3u, t.numElements());
}

TEST(MLTensorTest, NumElements2D) {
    MLTensor t;
    t.shape = {2, 3};
    t.data  = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    EXPECT_EQ(6u, t.numElements());
}

TEST(MLTensorTest, NumElements1x4) {
    MLTensor t;
    t.shape = {1, 4};
    t.data.assign(4, 0.f);
    EXPECT_EQ(4u, t.numElements());
}

TEST(MLTensorTest, NumElementsZeroDimension) {
    MLTensor t;
    t.shape = {0, 4};
    EXPECT_EQ(0u, t.numElements());
}

// ============================================================================
// MLServingResponse defaults
// ============================================================================

TEST(MLServingResponseTest, DefaultValues) {
    MLServingResponse resp;
    EXPECT_EQ(MLServingStatus::OK, resp.status);
    EXPECT_TRUE(resp.ok());
    EXPECT_TRUE(resp.error_message.empty());
    EXPECT_TRUE(resp.outputs.empty());
    EXPECT_DOUBLE_EQ(0.0, resp.latency_ms);
}

TEST(MLServingResponseTest, NotOkWhenNonOkStatus) {
    MLServingResponse resp;
    resp.status = MLServingStatus::UNAVAILABLE;
    EXPECT_FALSE(resp.ok());
}

// ============================================================================
// Helper functions
// ============================================================================

TEST(HelperFunctionsTest, StatusNames) {
    EXPECT_EQ("OK",            mlServingStatusName(MLServingStatus::OK));
    EXPECT_EQ("UNAVAILABLE",   mlServingStatusName(MLServingStatus::UNAVAILABLE));
    EXPECT_EQ("INVALID_INPUT", mlServingStatusName(MLServingStatus::INVALID_INPUT));
    EXPECT_EQ("BACKEND_ERROR", mlServingStatusName(MLServingStatus::BACKEND_ERROR));
}

TEST(HelperFunctionsTest, BackendTypeNames) {
    EXPECT_EQ("AUTO",         mlBackendTypeName(MLBackendType::AUTO));
    EXPECT_EQ("ONNX_RUNTIME", mlBackendTypeName(MLBackendType::ONNX_RUNTIME));
    EXPECT_EQ("TF_SERVING",   mlBackendTypeName(MLBackendType::TF_SERVING));
}

// ============================================================================
// ONNXServingBackend
// ============================================================================

TEST(ONNXServingBackendTest, BackendNameIsNonEmpty) {
    ONNXServingBackend backend;
    EXPECT_FALSE(backend.backendName().empty());
}

TEST(ONNXServingBackendTest, IsAvailableMatchesCompileFlag) {
    ONNXServingBackend backend;
#ifdef THEMIS_HAS_ONNX
    EXPECT_TRUE(backend.isAvailable());
#else
    EXPECT_FALSE(backend.isAvailable());
#endif
}

TEST(ONNXServingBackendTest, InferEmptyRequestReturnsInvalidInput) {
    ONNXServingBackend backend;
    MLServingRequest req;
    req.model_name = "test_model";
    // inputs empty → should return INVALID_INPUT regardless of availability
    auto resp = backend.infer(req);
#ifdef THEMIS_HAS_ONNX
    EXPECT_EQ(MLServingStatus::INVALID_INPUT, resp.status);
#else
    // When unavailable, backend may return either UNAVAILABLE or INVALID_INPUT
    EXPECT_NE(MLServingStatus::OK, resp.status);
#endif
}

TEST(ONNXServingBackendTest, InferUnavailableReturnsUnavailable) {
#ifndef THEMIS_HAS_ONNX
    ONNXServingBackend backend;
    MLServingRequest req;
    req.model_name = "test_model";
    req.inputs.push_back(MLTensor{"input", {1, 2}, {0.5f, 1.0f}});
    auto resp = backend.infer(req);
    EXPECT_EQ(MLServingStatus::UNAVAILABLE, resp.status);
    EXPECT_FALSE(resp.error_message.empty());
#else
    GTEST_SKIP() << "ONNX Runtime is available; skipping unavailable path test";
#endif
}

TEST(ONNXServingBackendTest, CustomConfigConstruction) {
    ONNXBackendConfig cfg;
    cfg.model_directory  = "/tmp/onnx_models";
    cfg.enable_cpu       = true;
    cfg.enable_cuda      = false;
    cfg.intra_op_threads = 2;
    cfg.inter_op_threads = 1;
    // Must not throw
    EXPECT_NO_THROW(ONNXServingBackend backend(cfg));
}

// ============================================================================
// TFServingBackend
// ============================================================================

TEST(TFServingBackendTest, BackendNameIsNonEmpty) {
    TFServingBackend backend;
    EXPECT_FALSE(backend.backendName().empty());
}

TEST(TFServingBackendTest, IsAvailableMatchesCompileFlags) {
    TFServingBackend backend;
#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)
    EXPECT_TRUE(backend.isAvailable());
#else
    EXPECT_FALSE(backend.isAvailable());
#endif
}

TEST(TFServingBackendTest, InferEmptyRequestReturnsInvalidInput) {
    TFServingBackend backend;
    MLServingRequest req;
    req.model_name = "test_model";
#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)
    auto resp = backend.infer(req);
    EXPECT_EQ(MLServingStatus::INVALID_INPUT, resp.status);
#else
    auto resp = backend.infer(req);
    EXPECT_EQ(MLServingStatus::UNAVAILABLE, resp.status);
#endif
}

TEST(TFServingBackendTest, InferUnavailableReturnsUnavailable) {
#if !defined(THEMIS_HAS_TF_SERVING) || !defined(THEMIS_HAS_CURL)
    TFServingBackend backend;
    MLServingRequest req;
    req.model_name = "test_model";
    req.inputs.push_back(MLTensor{"input", {1, 2}, {0.5f, 1.0f}});
    auto resp = backend.infer(req);
    EXPECT_EQ(MLServingStatus::UNAVAILABLE, resp.status);
    EXPECT_FALSE(resp.error_message.empty());
#else
    GTEST_SKIP() << "TF Serving is available; skipping unavailable path test";
#endif
}

TEST(TFServingBackendTest, CustomConfigConstruction) {
    TFServingConfig cfg;
    cfg.base_url   = "http://localhost:8501";
    cfg.timeout_ms = 3000;
    cfg.verify_ssl = false;
    EXPECT_NO_THROW(TFServingBackend backend(cfg));
}

TEST(TFServingBackendTest, DefaultConfigIsSecureByDefault) {
    TFServingConfig cfg;
    EXPECT_EQ("https://localhost:8501", cfg.base_url);
    EXPECT_FALSE(cfg.allow_insecure_transport);
    EXPECT_TRUE(cfg.verify_ssl);
}

TEST(TFServingBackendTest, InferRejectsHttpTransportWithoutExplicitOptIn) {
#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)
    TFServingConfig cfg;
    cfg.base_url = "http://localhost:8501";
    TFServingBackend backend(cfg);

    MLServingRequest req;
    req.model_name = "test_model";
    req.inputs.push_back(MLTensor{"input", {1, 2}, {0.5f, 1.0f}});

    auto resp = backend.infer(req);
    EXPECT_EQ(MLServingStatus::INVALID_INPUT, resp.status);
    EXPECT_NE(resp.error_message.find("Insecure"), std::string::npos);
#else
    GTEST_SKIP() << "TF Serving unavailable; insecure transport gate is runtime-only.";
#endif
}

// ============================================================================
// MLServingClient – AUTO backend
// ============================================================================

TEST(MLServingClientTest, DefaultConstructionDoesNotThrow) {
    EXPECT_NO_THROW(MLServingClient client);
}

TEST(MLServingClientTest, ActiveBackendNameIsNonEmpty) {
    MLServingClient client;
    EXPECT_FALSE(client.activeBackendName().empty());
}

TEST(MLServingClientTest, IsBackendAvailableAutoReflectsActiveBackend) {
    MLServingClient client;
    // Consistency: AUTO availability matches the active backend
#if defined(THEMIS_HAS_ONNX) || (defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL))
    EXPECT_TRUE(client.isBackendAvailable(MLBackendType::AUTO));
#else
    EXPECT_FALSE(client.isBackendAvailable(MLBackendType::AUTO));
#endif
}

TEST(MLServingClientTest, IsBackendAvailableONNX) {
    MLServingClient client;
#ifdef THEMIS_HAS_ONNX
    EXPECT_TRUE(client.isBackendAvailable(MLBackendType::ONNX_RUNTIME));
#else
    EXPECT_FALSE(client.isBackendAvailable(MLBackendType::ONNX_RUNTIME));
#endif
}

TEST(MLServingClientTest, IsBackendAvailableTFServing) {
    MLServingClient client;
#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)
    EXPECT_TRUE(client.isBackendAvailable(MLBackendType::TF_SERVING));
#else
    EXPECT_FALSE(client.isBackendAvailable(MLBackendType::TF_SERVING));
#endif
}

// ============================================================================
// MLServingClient – forced backend construction
// ============================================================================

TEST(MLServingClientTest, ForcedONNXBackendConstruction) {
    MLServingConfig cfg;
    cfg.backend = MLBackendType::ONNX_RUNTIME;
    MLServingClient client(cfg);
    EXPECT_FALSE(client.activeBackendName().empty());
}

TEST(MLServingClientTest, ForcedTFServingBackendConstruction) {
    MLServingConfig cfg;
    cfg.backend                 = MLBackendType::TF_SERVING;
    cfg.tf_config.base_url      = "http://localhost:8501";
    cfg.tf_config.timeout_ms    = 100;
    MLServingClient client(cfg);
    EXPECT_FALSE(client.activeBackendName().empty());
}

// ============================================================================
// MLServingClient – infer() when backend unavailable
// ============================================================================

TEST(MLServingClientInferTest, InferUnavailableBackendNoBackend) {
    // Construct a client that uses a backend that is definitely unavailable.
    // We force ONNX_RUNTIME when it is not compiled in, or TF_SERVING otherwise.
    MLServingConfig cfg;
#ifndef THEMIS_HAS_ONNX
    cfg.backend = MLBackendType::ONNX_RUNTIME;
#elif !defined(THEMIS_HAS_TF_SERVING)
    cfg.backend = MLBackendType::TF_SERVING;
#else
    GTEST_SKIP() << "All backends available; skipping unavailability test";
    return;
#endif

    MLServingClient client(cfg);
    MLServingRequest req;
    req.model_name = "irrelevant";
    req.inputs.push_back(MLTensor{"input", {1, 2}, {0.f, 1.f}});

    auto resp = client.infer(req);
    EXPECT_EQ(MLServingStatus::UNAVAILABLE, resp.status);
    EXPECT_FALSE(resp.error_message.empty());
}

// ============================================================================
// MLServingClient – inferFromDataPoint()
// ============================================================================

TEST(MLServingClientInferFromDataPointTest, EmptyDataPointReturnsInvalidInput) {
    MLServingClient client;
    DataPoint empty_point;
    empty_point.id = "empty";

    auto resp = client.inferFromDataPoint("some_model", empty_point);
    EXPECT_EQ(MLServingStatus::INVALID_INPUT, resp.status);
    EXPECT_FALSE(resp.error_message.empty());
}

TEST(MLServingClientInferFromDataPointTest, NumericPointBuildsRequest) {
    // We can't verify the model output without an actual model, but we can
    // verify that the function does not crash and returns a non-OK status
    // (model file won't exist) rather than INVALID_INPUT.
    MLServingClient client;
    auto dp = makeNumericPoint(4);

    auto resp = client.inferFromDataPoint("nonexistent_model", dp);
    // Status should be anything but INVALID_INPUT (input was valid)
    EXPECT_NE(MLServingStatus::INVALID_INPUT, resp.status);
}

TEST(MLServingClientInferFromDataPointTest, OnlyStringFieldsReturnsInvalidInput) {
    MLServingClient client;
    DataPoint p;
    p.id = "str_only";
    p.set("label", std::string("cat"));
    p.set("name",  std::string("foo"));

    auto resp = client.inferFromDataPoint("some_model", p);
    EXPECT_EQ(MLServingStatus::INVALID_INPUT, resp.status);
}

TEST(MLServingClientInferFromDataPointTest, CustomInputName) {
    MLServingClient client;
    auto dp = makeNumericPoint(3);
    // Should not crash when given a custom input tensor name
    EXPECT_NO_THROW(client.inferFromDataPoint("m", dp, "features"));
}

// ============================================================================
// Factory methods
// ============================================================================

TEST(FactoryTest, MakeONNXBackendReturnsNonNull) {
    auto backend = MLServingClient::makeONNXBackend();
    ASSERT_NE(nullptr, backend.get());
    EXPECT_FALSE(backend->backendName().empty());
}

TEST(FactoryTest, MakeTFServingBackendReturnsNonNull) {
    auto backend = MLServingClient::makeTFServingBackend();
    ASSERT_NE(nullptr, backend.get());
    EXPECT_FALSE(backend->backendName().empty());
}

TEST(FactoryTest, MakeONNXBackendWithConfig) {
    ONNXBackendConfig cfg;
    cfg.model_directory  = "/tmp";
    cfg.intra_op_threads = 1;
    auto backend = MLServingClient::makeONNXBackend(cfg);
    ASSERT_NE(nullptr, backend.get());
}

TEST(FactoryTest, MakeTFServingBackendWithConfig) {
    TFServingConfig cfg;
    cfg.base_url   = "http://tf-serving.example.com:8501";
    cfg.timeout_ms = 2000;
    auto backend = MLServingClient::makeTFServingBackend(cfg);
    ASSERT_NE(nullptr, backend.get());
}

// ============================================================================
// Backend interface polymorphism
// ============================================================================

TEST(BackendInterfaceTest, PolymorphicCallDoesNotCrash) {
    std::vector<std::unique_ptr<IMLServingBackend>> backends;
    backends.push_back(MLServingClient::makeONNXBackend());
    backends.push_back(MLServingClient::makeTFServingBackend());

    for (const auto& b : backends) {
        EXPECT_FALSE(b->backendName().empty());

        MLServingRequest req;
        req.model_name = "test";
        req.inputs.push_back(MLTensor{"input", {1, 2}, {1.f, 2.f}});

        // Must not throw – status can be anything
        EXPECT_NO_THROW({
            auto infer_result = b->infer(req);
            static_cast<void>(infer_result);
        });
    }
}

// ============================================================================
// Concurrency – TOCTOU + full-inference-lock fix (Issue #5a / #5b)
// ============================================================================

// Two threads simultaneously infer on two *different* models.
// Neither thread should block the other, deadlock, or throw.
// This test validates the fix without requiring a real ONNX session:
// when ONNX is absent both threads get UNAVAILABLE immediately with
// no global serialisation; when ONNX is present the session is
// held via shared_ptr and Run() executes outside sessions_mutex.
TEST(ONNXServingBackendTest, ConcurrentInferDifferentModelsNoDeadlock) {
    ONNXServingBackend backend;

    std::atomic<bool> thread1_threw{false};
    std::atomic<bool> thread2_threw{false};

    auto make_req = [](const std::string& model_name) {
        MLServingRequest req;
        req.model_name = model_name;
        req.inputs.push_back(MLTensor{"input", {1, 4}, {1.0f, 2.0f, 3.0f, 4.0f}});
        return req;
    };

    // Use futures so a deadlock/regression causes the test to fail with a
    // clear timeout message rather than hanging the entire test binary.
    auto f1 = std::async(std::launch::async, [&] {
        try { (void)backend.infer(make_req("model_alpha")); }
        catch (...) { thread1_threw = true; }
    });
    auto f2 = std::async(std::launch::async, [&] {
        try { (void)backend.infer(make_req("model_beta")); }
        catch (...) { thread2_threw = true; }
    });

    constexpr auto kTimeout = std::chrono::seconds(10);
    ASSERT_EQ(f1.wait_for(kTimeout), std::future_status::ready) << "Thread 1 timed out – possible deadlock";
    ASSERT_EQ(f2.wait_for(kTimeout), std::future_status::ready) << "Thread 2 timed out – possible deadlock";

    EXPECT_FALSE(thread1_threw) << "Thread 1 threw an exception";
    EXPECT_FALSE(thread2_threw) << "Thread 2 threw an exception";
}

// Two threads infer on the *same* model concurrently.
// The per-model loading mutex must prevent a double-load race without
// causing a deadlock or exception.
TEST(ONNXServingBackendTest, ConcurrentInferSameModelNoDeadlock) {
    ONNXServingBackend backend;

    std::atomic<bool> any_threw{false};

    MLServingRequest req;
    req.model_name = "shared_model";
    req.inputs.push_back(MLTensor{"input", {1, 2}, {0.5f, 1.0f}});

    // Use futures with a hard timeout so a deadlock surfaces as a test failure
    // rather than an indefinite hang.
    auto f1 = std::async(std::launch::async, [&] {
        try { (void)backend.infer(req); }
        catch (...) { any_threw = true; }
    });
    auto f2 = std::async(std::launch::async, [&] {
        try { (void)backend.infer(req); }
        catch (...) { any_threw = true; }
    });

    constexpr auto kTimeout = std::chrono::seconds(10);
    ASSERT_EQ(f1.wait_for(kTimeout), std::future_status::ready) << "Thread 1 timed out – possible deadlock";
    ASSERT_EQ(f2.wait_for(kTimeout), std::future_status::ready) << "Thread 2 timed out – possible deadlock";

    EXPECT_FALSE(any_threw) << "A thread threw an unexpected exception";
}
