/**
 * @file ml_serving.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
 *                         API (POST `/v1/models/\<name\>:predict`).
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

struct MLTensor {
    std::string            name;   ///< Tensor name (must match model input/output name)
    std::vector<int64_t>   shape;  ///< Dimensions, e.g. {batch_size, num_features}
    std::vector<float>     data;   ///< Row-major float32 values

    /**
     * @brief Num Elements.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::size_t numElements() const noexcept;
};

// ============================================================================
// Request / Response
// ============================================================================

struct MLServingRequest {
    std::string              model_name;    ///< Model identifier
    std::string              model_version; ///< Version tag ("" = latest)
    std::vector<MLTensor>    inputs;        ///< Named input tensors
};

enum class MLServingStatus {
    OK,             ///< Inference completed successfully
    UNAVAILABLE,    ///< Backend or model not available
    INVALID_INPUT,  ///< Input shape/type mismatch
    BACKEND_ERROR,  ///< Internal backend error (see error_message)
    TIMEOUT,        ///< Inference did not complete within the policy deadline
    POLICY_REJECTED,///< Request rejected because the BoundedExecutionPolicy concurrency limit was exceeded
};

struct MLServingResponse {
    MLServingStatus       status       = MLServingStatus::OK;
    std::string           error_message;
    std::vector<MLTensor> outputs;      ///< Named output tensors
    double                latency_ms   = 0.0; ///< End-to-end call latency
    std::string           operation_id;
    std::string           correlation_id;
    std::string           failure_class = "none";
    std::vector<std::string> operator_hints;

    bool ok() const noexcept { return status == MLServingStatus::OK; }
};

// ============================================================================
// Backend interface
// ============================================================================

class IMLServingBackend {
public:
    /**
     * @brief IMLServing Backend.
     * @return Return value.
     */
    virtual ~IMLServingBackend() = default;

    [[nodiscard]] virtual std::string backendName() const = 0;

    [[nodiscard]] virtual bool isAvailable() const = 0;

    [[nodiscard]] virtual MLServingResponse infer(const MLServingRequest& req) = 0;
};

// ============================================================================
// ONNX Runtime backend
// ============================================================================

struct ONNXBackendConfig {
    std::string model_directory = "./models"; ///< Directory searched for *.onnx files
    bool        enable_cpu      = true;       ///< Use CPU execution provider
    bool        enable_cuda     = false;      ///< Use CUDA execution provider (if available)
    int         intra_op_threads = 0;         ///< 0 = auto (hardware_concurrency)
    int         inter_op_threads = 0;         ///< 0 = auto
    std::size_t memory_limit_mb  = 0;         ///< 0 = unlimited
};

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

struct TFServingConfig {
    std::string base_url                 = "https://localhost:8501"; ///< TF Serving REST API base URL
    int         timeout_ms               = 5000;                     ///< HTTP request timeout
    bool        verify_ssl               = true;                     ///< Verify TLS certificates
    bool        allow_insecure_transport = false;                    ///< Allow plaintext HTTP when explicitly enabled
    std::string api_key;                                              ///< Optional bearer token / API key
};

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

enum class MLBackendType {
    AUTO,         ///< Prefer ONNX Runtime; fall back to TF Serving
    ONNX_RUNTIME, ///< Force ONNX Runtime backend
    TF_SERVING    ///< Force TF Serving backend
};

struct MLServingConfig {
    MLBackendType     backend      = MLBackendType::AUTO;
    ONNXBackendConfig onnx_config;
    TFServingConfig   tf_config;

    ::themis::analytics::BoundedExecutionPolicy default_policy;
};

class MLServingClient {
public:
    explicit MLServingClient(const MLServingConfig& config = {});
    ~MLServingClient();


    /**
     * @brief Is Backend Available.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isBackendAvailable(MLBackendType type) const;

    /**
     * @brief Active Backend Name.
     * @return Return value.
     */
    std::string activeBackendName() const;


    /**
     * @brief Infer.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    MLServingResponse infer(const MLServingRequest& req);

    /**
     * @brief Infer.
     * @param[in] req Input parameter.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    MLServingResponse infer(const MLServingRequest&                      req,
                            const ::themis::analytics::BoundedExecutionPolicy& policy);

    MLServingResponse inferFromDataPoint(const std::string& model_name,
                                         const DataPoint&   point,
                                         const std::string& input_name = "input");

    // ─── Factory ─────────────────────────────────────────────────────────────

    static std::unique_ptr<IMLServingBackend>
    makeONNXBackend(const ONNXBackendConfig& config = {});

    static std::unique_ptr<IMLServingBackend>
    makeTFServingBackend(const TFServingConfig& config = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Helper utilities
// ============================================================================

/**
 * @brief Ml Serving Status Name.
 * @param[in] status Input parameter.
 * @return Return value.
 */
std::string mlServingStatusName(MLServingStatus status);

/**
 * @brief Ml Backend Type Name.
 * @param[in] type Input parameter.
 * @return Return value.
 */
std::string mlBackendTypeName(MLBackendType type);

} // namespace analytics
} // namespace themisdb
