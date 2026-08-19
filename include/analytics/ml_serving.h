/**
 * @file ml_serving.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Analytics ML Serving Integration
 *
 * Provides a unified abstraction for integrating external ML inference
 * engines with the ThemisDB analytics module.  Two backends are supported:
 *
 *   ONNXServingBackend  – Runs ONNX models locally via ONNX Runtime.
 *                         Requires compile-time flag THEMIS_HAS_ONNX=1 and
 *                         the onnxruntime library (vcpkg: onnxruntime).
 *
 *   TFServingBackend    – Calls a TensorFlow Serving instance over its REST
 *                         API (POST /v1/models/<name>:predict).
 *                         Requires compile-time flag THEMIS_HAS_TF_SERVING=1
 *                         and libcurl (THEMIS_HAS_CURL=1).
 *
 * When neither backend is available the MLServingClient returns an
 * UNAVAILABLE error so callers can degrade gracefully.
 *
 * DataPoint integration:
 *   MLServingClient::inferFromDataPoint() converts the numeric fields of a
 *   DataPoint (from analytics/anomaly_detection.h) into a flat float32 tensor
 *   and returns the model output as an MLServingResponse.  The field names are
 *   sorted deterministically (alphabetical) to match training-time conventions.
 *
 * Thread-safety:
 *   - MLServingClient is thread-safe after construction.
 *   - ONNXServingBackend::infer() is thread-safe (ONNX Runtime sessions are
 *     safe for concurrent inference).
 *   - TFServingBackend::infer() is thread-safe (libcurl handles are
 *     per-call).
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Re-use DataPoint for seamless integration with the anomaly detection module.
#include "analytics/anomaly_detection.h"
// BoundedExecutionPolicy for policy-aware inference overloads.
#include "analytics/analytics_api_contract.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class IMLServingBackend;
class ONNXServingBackend;
class TFServingBackend;
class MLServingClient;

// ============================================================================
// Tensor representation
// ============================================================================

/**
 * A named, shaped float32 tensor exchanged with an ML backend.
 *
 * All numeric data is stored as float32 to match the most common ML
 * framework convention.  Callers that need higher precision should cast
 * externally.
 */
struct MLTensor {
    std::string            name;   ///< Tensor name (must match model input/output name)
    std::vector<int64_t>   shape;  ///< Dimensions, e.g. {batch_size, num_features}
    std::vector<float>     data;   ///< Row-major float32 values

    /** Total number of elements (product of shape dimensions). */
    std::size_t numElements() const noexcept;
};

// ============================================================================
// Request / Response
// ============================================================================

/**
 * Inference request sent to an MLServingBackend.
 */
struct MLServingRequest {
    std::string              model_name;    ///< Model identifier
    std::string              model_version; ///< Version tag ("" = latest)
    std::vector<MLTensor>    inputs;        ///< Named input tensors
};

/** Status codes returned by MLServingResponse. */
enum class MLServingStatus {
    OK,             ///< Inference completed successfully
    UNAVAILABLE,    ///< Backend or model not available
    INVALID_INPUT,  ///< Input shape/type mismatch
    BACKEND_ERROR,  ///< Internal backend error (see error_message)
    TIMEOUT,        ///< Inference did not complete within the policy deadline
    POLICY_REJECTED,///< Request rejected because the BoundedExecutionPolicy concurrency limit was exceeded
};

/**
 * Inference response from an MLServingBackend.
 */
struct MLServingResponse {
    MLServingStatus       status       = MLServingStatus::OK;
    std::string           error_message;
    std::vector<MLTensor> outputs;      ///< Named output tensors
    double                latency_ms   = 0.0; ///< End-to-end call latency

    /** Returns true when status == OK. */
    bool ok() const noexcept { return status == MLServingStatus::OK; }
};

// ============================================================================
// Backend interface
// ============================================================================

/**
 * Abstract interface implemented by each ML inference backend.
 */
class IMLServingBackend {
public:
    virtual ~IMLServingBackend() = default;

    /** Human-readable name of this backend (e.g. "ONNX Runtime 1.17.0"). */
    [[nodiscard]] virtual std::string backendName() const = 0;

    /** Returns true if the backend is usable (libraries found, server reachable, etc.). */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /** Run synchronous inference. */
    [[nodiscard]] virtual MLServingResponse infer(const MLServingRequest& req) = 0;
};

// ============================================================================
// ONNX Runtime backend
// ============================================================================

/**
 * Configuration for ONNXServingBackend.
 */
struct ONNXBackendConfig {
    std::string model_directory = "./models"; ///< Directory searched for *.onnx files
    bool        enable_cpu      = true;       ///< Use CPU execution provider
    bool        enable_cuda     = false;      ///< Use CUDA execution provider (if available)
    int         intra_op_threads = 0;         ///< 0 = auto (hardware_concurrency)
    int         inter_op_threads = 0;         ///< 0 = auto
    std::size_t memory_limit_mb  = 0;         ///< 0 = unlimited
};

/**
 * ONNXServingBackend – loads and runs ONNX models via ONNX Runtime.
 *
 * When THEMIS_HAS_ONNX is defined the backend uses the real
 * onnxruntime C++ API.  When it is absent the backend reports
 * isAvailable() == false and every infer() call returns UNAVAILABLE.
 *
 * Models are loaded lazily on the first call to infer() with a new
 * model_name.  The model file is resolved as:
 *   <model_directory>/<model_name>.onnx
 *
 * Thread-safety: multiple threads may call infer() concurrently.
 */
class ONNXServingBackend : public IMLServingBackend {
public:
    explicit ONNXServingBackend(const ONNXBackendConfig& config = {});
    ~ONNXServingBackend() override;

    std::string backendName() const override;
    bool        isAvailable() const override;
    MLServingResponse infer(const MLServingRequest& req) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// TensorFlow Serving backend
// ============================================================================

/**
 * Configuration for TFServingBackend.
 */
struct TFServingConfig {
    std::string base_url                 = "https://localhost:8501"; ///< TF Serving REST API base URL
    int         timeout_ms               = 5000;                     ///< HTTP request timeout
    bool        verify_ssl               = true;                     ///< Verify TLS certificates
    bool        allow_insecure_transport = false;                    ///< Allow plaintext HTTP when explicitly enabled
    std::string api_key;                                              ///< Optional bearer token / API key
};

/**
 * TFServingBackend – calls a TensorFlow Serving instance over its REST API.
 *
 * Endpoint: POST <base_url>/v1/models/<model_name>[:predict]
 *   (optionally /versions/<version> when model_version is set)
 *
 * Requires THEMIS_HAS_TF_SERVING=1 (and THEMIS_HAS_CURL=1 transitively).
 * When either flag is absent the backend reports isAvailable() == false.
 *
 * The REST payload follows the TF Serving JSON API:
 *   { "inputs": { "<name>": [[...]] } }
 *
 * Thread-safety: each infer() creates an independent libcurl easy handle so
 * concurrent calls are safe.
 */
class TFServingBackend : public IMLServingBackend {
public:
    explicit TFServingBackend(const TFServingConfig& config = {});
    ~TFServingBackend() override;

    std::string backendName() const override;
    bool        isAvailable() const override;
    MLServingResponse infer(const MLServingRequest& req) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Unified client
// ============================================================================

/** Which backend the MLServingClient should use. */
enum class MLBackendType {
    AUTO,         ///< Prefer ONNX Runtime; fall back to TF Serving
    ONNX_RUNTIME, ///< Force ONNX Runtime backend
    TF_SERVING    ///< Force TF Serving backend
};

/**
 * Configuration for MLServingClient.
 */
struct MLServingConfig {
    MLBackendType     backend      = MLBackendType::AUTO;
    ONNXBackendConfig onnx_config;
    TFServingConfig   tf_config;

    /// Default execution policy applied automatically to every infer() call
    /// when the caller does not supply an explicit BoundedExecutionPolicy.
    /// When unconstrained (all fields zero, the default), the policy
    /// enforcement layer is bypassed entirely to avoid overhead.
    /// Set any non-zero field to activate per-client concurrency or
    /// latency enforcement without modifying individual call sites.
    ::themis::analytics::BoundedExecutionPolicy default_policy;
};

/**
 * MLServingClient – high-level, thread-safe interface to ML inference.
 *
 * Usage example:
 * @code
 *   using namespace themisdb::analytics;
 *
 *   MLServingClient client(MLServingConfig{
 *       .backend    = MLBackendType::ONNX_RUNTIME,
 *       .onnx_config = { .model_directory = "/opt/models" }
 *   });
 *
 *   if (!client.isBackendAvailable(MLBackendType::ONNX_RUNTIME)) {
 *       // handle graceful degradation
 *   }
 *
 *   MLServingRequest req;
 *   req.model_name = "churn_classifier";
 *   req.inputs.push_back({ "input", {1, 4}, {0.5f, 1.2f, -0.3f, 0.9f} });
 *
 *   auto resp = client.infer(req);
 *   if (resp.ok()) {
 *       // resp.outputs[0].data contains probabilities
 *   }
 * @endcode
 *
 * DataPoint integration:
 * @code
 *   DataPoint dp;
 *   dp.set("feature_a", 1.5);
 *   dp.set("feature_b", -0.3);
 *
 *   auto resp = client.inferFromDataPoint("churn_classifier", dp);
 * @endcode
 */
class MLServingClient {
public:
    explicit MLServingClient(const MLServingConfig& config = {});
    ~MLServingClient();

    // ─── Backend introspection ───────────────────────────────────────────────

    /** Returns true if the specified backend type is compiled in and available. */
    bool isBackendAvailable(MLBackendType type) const;

    /** Returns the name of the active backend. */
    std::string activeBackendName() const;

    // ─── Inference ──────────────────────────────────────────────────────────

    /** Run inference using the active backend. */
    MLServingResponse infer(const MLServingRequest& req);

    /**
     * @brief Run inference with bounded execution enforcement.
     *
     * Enforces the resource limits declared by @p policy before dispatching
     * to the underlying backend.  Specifically:
     *
     *   - **Concurrency**: if `policy.max_concurrent_requests > 0` and the
     *     number of in-flight `infer()` calls on this client instance already
     *     equals that limit, the call returns `POLICY_REJECTED` immediately.
     *   - **Timeout**: if `policy.max_latency_ms > 0` and the backend call
     *     has not returned within that many milliseconds the call returns
     *     `TIMEOUT`.  The backend call itself may continue in a detached
     *     thread; callers must not re-use the client for further calls until
     *     the previous call has fully returned.
     *
     * When `!policy.isConstrained()` this overload is equivalent to the
     * unconstrained `infer(req)`.
     *
     * @param req     Inference request.
     * @param policy  Resource limits to enforce.
     * @return MLServingResponse; status is `POLICY_REJECTED` or `TIMEOUT` on
     *         policy violation, otherwise same as the unconstrained overload.
     */
    MLServingResponse infer(const MLServingRequest&                      req,
                            const ::themis::analytics::BoundedExecutionPolicy& policy);

    /**
     * Convenience overload: converts the numeric fields of @p point into a
     * single flat float32 input tensor named "input" and calls infer().
     *
     * Fields are sorted alphabetically (consistent with AutoML feature
     * engineering conventions in this module).  Non-numeric fields are
     * silently ignored.
     *
     * @param model_name  Model to query.
     * @param point       DataPoint whose numeric fields form the input vector.
     * @param input_name  Name of the input tensor (default: "input").
     * @return MLServingResponse with status and output tensors.
     */
    MLServingResponse inferFromDataPoint(const std::string& model_name,
                                         const DataPoint&   point,
                                         const std::string& input_name = "input");

    // ─── Factory ─────────────────────────────────────────────────────────────

    /** Create an ONNX Runtime backend with the given config. */
    static std::unique_ptr<IMLServingBackend>
    makeONNXBackend(const ONNXBackendConfig& config = {});

    /** Create a TF Serving backend with the given config. */
    static std::unique_ptr<IMLServingBackend>
    makeTFServingBackend(const TFServingConfig& config = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Helper utilities
// ============================================================================

/** Returns a human-readable string for the given status code. */
std::string mlServingStatusName(MLServingStatus status);

/** Returns a human-readable string for the given backend type. */
std::string mlBackendTypeName(MLBackendType type);

} // namespace analytics
} // namespace themisdb
