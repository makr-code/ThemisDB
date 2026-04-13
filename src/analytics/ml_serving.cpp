/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ml_serving.cpp                                     ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     716                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • cc27518101  2026-03-17  fix(analytics): MLServingEngine::infer() TOCTOU + full-in... ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 197b8b5b19  2026-02-24  feat(analytics): integrate ONNX Runtime and TensorFlow Se... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ML Serving Integration – Implementation
 *
 * Conditionally compiled sections:
 *
 *   THEMIS_HAS_ONNX         → ONNX Runtime C++ API integration
 *   THEMIS_HAS_TF_SERVING   → TensorFlow Serving REST API via libcurl
 *   THEMIS_HAS_CURL         → libcurl (implicit requirement for TF Serving)
 *
 * When a flag is absent the corresponding backend sets isAvailable() = false
 * and every infer() call returns MLServingStatus::UNAVAILABLE.
 */

#include "analytics/ml_serving.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <spdlog/spdlog.h>

// ─── ONNX Runtime ─────────────────────────────────────────────────────────
#ifdef THEMIS_HAS_ONNX
// Try the subdirectory-style path first (vcpkg default), then flat path for
// toolchains that add the onnxruntime directory directly to the include path.
#  if __has_include(<onnxruntime/onnxruntime_cxx_api.h>)
#    include <onnxruntime/onnxruntime_cxx_api.h>
#  else
#    include <onnxruntime_cxx_api.h>
#  endif
#endif

// ─── TF Serving HTTP client ────────────────────────────────────────────────
#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

namespace themisdb {
namespace analytics {

// ============================================================================
// MLTensor helpers
// ============================================================================

std::size_t MLTensor::numElements() const noexcept {
    if (shape.empty()) return data.size();
    std::size_t n = 1;
    for (auto d : shape) {
        if (d <= 0) return 0;
        n *= static_cast<std::size_t>(d);
    }
    return n;
}

// ============================================================================
// Helper: wall-clock timer
// ============================================================================

namespace {

class Stopwatch {
public:
    Stopwatch() : start_(std::chrono::steady_clock::now()) {}
    double elapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_).count();
    }
private:
    std::chrono::steady_clock::time_point start_;
};

} // anonymous namespace

// ============================================================================
// ONNXServingBackend – Impl
// ============================================================================

#ifdef THEMIS_HAS_ONNX

struct ONNXServingBackend::Impl {
    ONNXBackendConfig          config;
    Ort::Env                   env;
    Ort::SessionOptions        session_opts;
    // Per-model sessions stored as shared_ptr so handles can be retained
    // outside the map lock while ONNX Run() executes concurrently.
    std::map<std::string, std::shared_ptr<Ort::Session>> sessions;
    mutable std::shared_mutex  sessions_mutex;   // shared=read, exclusive=write

    // Per-model loading mutexes: serialise concurrent loads of the *same*
    // model without blocking inferences for unrelated models.
    std::map<std::string, std::shared_ptr<std::mutex>> model_load_mutexes;
    std::mutex                 model_load_mutexes_lock;

    explicit Impl(const ONNXBackendConfig& cfg)
        : config(cfg)
        , env(ORT_LOGGING_LEVEL_WARNING, "ThemisDB-MLServing")
    {
        // Thread-count settings
        int intra = (cfg.intra_op_threads > 0)
                        ? cfg.intra_op_threads
                        : static_cast<int>(std::thread::hardware_concurrency());
        int inter = (cfg.inter_op_threads > 0)
                        ? cfg.inter_op_threads
                        : 1;
        session_opts.SetIntraOpNumThreads(intra);
        session_opts.SetInterOpNumThreads(inter);
        session_opts.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // CUDA provider (best-effort; silently skipped if unavailable)
        if (cfg.enable_cuda) {
            OrtCUDAProviderOptions cuda_opts{};
            session_opts.AppendExecutionProvider_CUDA(cuda_opts);
        }
    }

    // Returns a shared_ptr to the session for model_name, loading it lazily.
    // Must be called WITHOUT holding sessions_mutex.
    //
    // Design:
    //  1. Fast path   – shared_lock on sessions_mutex; return if already loaded.
    //  2. Load path   – per-model mutex serialises concurrent loads of the same
    //                   model; unrelated models are never blocked.
    //  3. Double-check after acquiring the per-model lock (another thread may
    //                   have already finished loading).
    //  4. Actual load – I/O happens only under the per-model lock, NOT under
    //                   sessions_mutex.
    //  5. Store       – exclusive write lock on sessions_mutex to insert.
    //
    // Note: model_load_mutexes grows proportionally to the number of distinct
    // model names ever requested (typically a small, bounded set).
    std::shared_ptr<Ort::Session> getOrLoadSession(const std::string& model_name) {
        // Fast path: session already loaded.
        {
            std::shared_lock<std::shared_mutex> sessions_read_lock(sessions_mutex);
            auto it = sessions.find(model_name);
            if (it != sessions.end()) {
                return it->second;
            }
        }

        // Slow path: need to load. Obtain a per-model mutex so that concurrent
        // requests for the *same* model queue behind each other while requests
        // for *different* models proceed in parallel.
        std::shared_ptr<std::mutex> load_mutex;
        {
            std::lock_guard<std::mutex> mlk(model_load_mutexes_lock);
            auto& entry = model_load_mutexes[model_name];
            if (!entry) entry = std::make_shared<std::mutex>();
            load_mutex = entry;
        }

        std::lock_guard<std::mutex> load_lock(*load_mutex);

        // Double-check: another thread may have finished loading while we
        // waited on the per-model mutex.
        {
            std::shared_lock<std::shared_mutex> sessions_read_lock(sessions_mutex);
            auto it = sessions.find(model_name);
            if (it != sessions.end()) {
                return it->second;
            }
        }

        // Actually load the model (potentially slow I/O – held only under the
        // per-model lock, not under sessions_mutex).
        auto session = loadSession(model_name);
        if (!session) {
            return nullptr;
        }

        // Store in the session map under an exclusive write lock.
        {
            std::unique_lock<std::shared_mutex> sessions_write_lock(sessions_mutex);
            sessions[model_name] = session;
        }
        return session;
    }

    // Load a session for the given model name.
    // Returns nullptr on failure. Must be called WITHOUT holding sessions_mutex.
    std::shared_ptr<Ort::Session> loadSession(const std::string& model_name) {
        auto model_path = config.model_directory + "/" + model_name + ".onnx";
        try {
#ifdef _WIN32
            std::wstring wpath(model_path.begin(), model_path.end());
            auto session = std::make_shared<Ort::Session>(env, wpath.c_str(), session_opts);
#else
            auto session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_opts);
#endif
            spdlog::info("MLServing[ONNX]: loaded model '{}'", model_name);
            return session;
        } catch (const Ort::Exception& e) {
            spdlog::error("MLServing[ONNX]: failed to load '{}': {}", model_name, e.what());
            return nullptr;
        }
    }
};

ONNXServingBackend::ONNXServingBackend(const ONNXBackendConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

ONNXServingBackend::~ONNXServingBackend() = default;

std::string ONNXServingBackend::backendName() const {
    return std::string("ONNX Runtime ") + OrtGetApiBase()->GetVersionString();
}

bool ONNXServingBackend::isAvailable() const {
    return true; // ONNX Runtime linked at compile time
}

MLServingResponse ONNXServingBackend::infer(const MLServingRequest& req) {
    Stopwatch sw;
    MLServingResponse resp;

    if (req.inputs.empty()) {
        resp.status        = MLServingStatus::INVALID_INPUT;
        resp.error_message = "No input tensors provided";
        return resp;
    }

    // Obtain a shared_ptr to the session in a single brief critical section.
    // sessions_mutex is released before ONNX Run() so independent-model
    // inferences can proceed concurrently (fixes 5a TOCTOU + 5b global lock).
    auto session_ptr = impl_->getOrLoadSession(req.model_name);
    if (!session_ptr) {
        resp.status        = MLServingStatus::UNAVAILABLE;
        resp.error_message = "Model '" + req.model_name + "' could not be loaded";
        resp.latency_ms    = sw.elapsedMs();
        return resp;
    }

    try {
        auto& session = *session_ptr;  // held via shared_ptr – no global lock needed

        Ort::AllocatorWithDefaultOptions allocator;

        // Build input names and tensors
        std::vector<const char*>     input_names;
        std::vector<Ort::Value>      input_tensors;

        auto memory_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        for (const auto& t : req.inputs) {
            input_names.push_back(t.name.c_str());

            // Cast to non-const for the API (data is copied internally)
            std::vector<float> data_copy = t.data;
            std::vector<int64_t> shape   = t.shape;

            input_tensors.push_back(Ort::Value::CreateTensor<float>(
                memory_info,
                data_copy.data(),
                data_copy.size(),
                shape.data(),
                shape.size()));
        }

        // Collect output names from session metadata
        std::size_t out_count = session.GetOutputCount();
        std::vector<std::string>  out_name_strs;
        std::vector<const char*>  output_names;
        for (std::size_t i = 0; i < out_count; ++i) {
            auto name_alloc = session.GetOutputNameAllocated(i, allocator);
            out_name_strs.emplace_back(name_alloc.get());
        }
        for (const auto& s : out_name_strs) {
            output_names.push_back(s.c_str());
        }

        // Run inference
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            input_names.data(),
            input_tensors.data(),
            input_names.size(),
            output_names.data(),
            output_names.size());

        // Convert outputs
        for (std::size_t i = 0; i < output_tensors.size(); ++i) {
            const auto& ort_t     = output_tensors[i];
            auto        type_info = ort_t.GetTensorTypeAndShapeInfo();
            auto        ort_shape = type_info.GetShape();

            MLTensor out_tensor;
            out_tensor.name = out_name_strs[i];
            out_tensor.shape.assign(ort_shape.begin(), ort_shape.end());

            const float* ptr      = ort_t.GetTensorData<float>();
            std::size_t  n_elems  = type_info.GetElementCount();
            out_tensor.data.assign(ptr, ptr + n_elems);

            resp.outputs.push_back(std::move(out_tensor));
        }

        resp.status      = MLServingStatus::OK;
        resp.latency_ms  = sw.elapsedMs();

    } catch (const Ort::Exception& e) {
        resp.status        = MLServingStatus::BACKEND_ERROR;
        resp.error_message = std::string("ONNX Runtime error: ") + e.what();
        resp.latency_ms    = sw.elapsedMs();
        spdlog::error("MLServing[ONNX]: inference error for '{}': {}",
                      req.model_name, e.what());
    }

    return resp;
}

#else // THEMIS_HAS_ONNX not defined

struct ONNXServingBackend::Impl {
    ONNXBackendConfig config;
    explicit Impl(const ONNXBackendConfig& cfg) : config(cfg) {}
};

ONNXServingBackend::ONNXServingBackend(const ONNXBackendConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

ONNXServingBackend::~ONNXServingBackend() = default;

std::string ONNXServingBackend::backendName() const {
    return "ONNX Runtime (unavailable – rebuild with THEMIS_HAS_ONNX=1)";
}

bool ONNXServingBackend::isAvailable() const { return false; }

MLServingResponse ONNXServingBackend::infer(const MLServingRequest& req) {
    spdlog::warn("MLServing[ONNX]: backend unavailable – rebuild with "
                 "-DTHEMIS_HAS_ONNX=1 and install onnxruntime via vcpkg");
    MLServingResponse resp;
    resp.status        = MLServingStatus::UNAVAILABLE;
    resp.error_message = "ONNX Runtime backend not compiled in. "
                         "Rebuild with -DTHEMIS_HAS_ONNX=1.";
    return resp;
}

#endif // THEMIS_HAS_ONNX

// ============================================================================
// TFServingBackend – Impl
// ============================================================================

#if defined(THEMIS_HAS_TF_SERVING) && defined(THEMIS_HAS_CURL)

namespace {

/// libcurl write callback – appends received data to a std::string.
std::size_t curlWriteCallback(char* ptr, std::size_t size,
                               std::size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

} // anonymous namespace

struct TFServingBackend::Impl {
    TFServingConfig config;
    explicit Impl(const TFServingConfig& cfg) : config(cfg) {}
};

TFServingBackend::TFServingBackend(const TFServingConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

TFServingBackend::~TFServingBackend() = default;

std::string TFServingBackend::backendName() const {
    return "TensorFlow Serving REST (libcurl) @ " + impl_->config.base_url;
}

bool TFServingBackend::isAvailable() const { return true; }

MLServingResponse TFServingBackend::infer(const MLServingRequest& req) {
    using json = nlohmann::json;
    Stopwatch sw;
    MLServingResponse resp;

    if (req.inputs.empty()) {
        resp.status        = MLServingStatus::INVALID_INPUT;
        resp.error_message = "No input tensors provided";
        return resp;
    }

    // Build JSON payload: { "inputs": { "<name>": [[...]] } }
    json payload;
    json inputs_json = json::object();
    for (const auto& t : req.inputs) {
        // Flatten tensor into nested array according to shape
        // For simplicity we pass a 1-D array when the shape is {1, N} or {N}
        inputs_json[t.name] = json(t.data.begin(), t.data.end());
    }
    payload["inputs"] = inputs_json;

    std::string json_body = payload.dump();

    // Build URL
    std::string url = impl_->config.base_url
        + "/v1/models/" + req.model_name;
    if (!req.model_version.empty()) {
        url += "/versions/" + req.model_version;
    }
    url += ":predict";

    // Perform HTTP POST via libcurl
    std::string response_body;

    CURL* curl = curl_easy_init();
    if (!curl) {
        resp.status        = MLServingStatus::BACKEND_ERROR;
        resp.error_message = "Failed to initialise libcurl handle";
        return resp;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!impl_->config.api_key.empty()) {
        std::string auth_header = "Authorization: Bearer " + impl_->config.api_key;
        headers = curl_slist_append(headers, auth_header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,
                     static_cast<long>(impl_->config.timeout_ms));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,
                     impl_->config.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,
                     impl_->config.verify_ssl ? 2L : 0L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    resp.latency_ms = sw.elapsedMs();

    if (res != CURLE_OK) {
        resp.status        = MLServingStatus::BACKEND_ERROR;
        resp.error_message = std::string("libcurl error: ") + curl_easy_strerror(res);
        spdlog::error("MLServing[TF]: curl error for '{}': {}",
                      req.model_name, resp.error_message);
        return resp;
    }
    if (http_code != 200) {
        resp.status        = MLServingStatus::BACKEND_ERROR;
        resp.error_message = "HTTP " + std::to_string(http_code)
                             + ": " + response_body;
        spdlog::error("MLServing[TF]: server error for '{}': HTTP {}",
                      req.model_name, http_code);
        return resp;
    }

    // Parse JSON response: { "outputs": { "<name>": [...] } }
    try {
        auto jresp = json::parse(response_body);

        if (!jresp.contains("outputs")) {
            resp.status        = MLServingStatus::BACKEND_ERROR;
            resp.error_message = "TF Serving response missing 'outputs' field";
            return resp;
        }

        const auto& joutputs = jresp["outputs"];
        if (joutputs.is_object()) {
            for (auto& [name, val] : joutputs.items()) {
                MLTensor t;
                t.name = name;
                if (val.is_array()) {
                    // Flatten potentially nested arrays to 1-D float vector
                    std::function<void(const json&)> flatten =
                        [&](const json& node) {
                            if (node.is_array()) {
                                for (const auto& elem : node) flatten(elem);
                            } else if (node.is_number()) {
                                t.data.push_back(node.get<float>());
                            }
                        };
                    flatten(val);
                    t.shape = { static_cast<int64_t>(t.data.size()) };
                }
                resp.outputs.push_back(std::move(t));
            }
        }
        resp.status = MLServingStatus::OK;

    } catch (const json::exception& e) {
        resp.status        = MLServingStatus::BACKEND_ERROR;
        resp.error_message = std::string("JSON parse error: ") + e.what();
        spdlog::error("MLServing[TF]: JSON parse error for '{}': {}",
                      req.model_name, e.what());
    }

    return resp;
}

#else // TF Serving not available

struct TFServingBackend::Impl {
    TFServingConfig config;
    explicit Impl(const TFServingConfig& cfg) : config(cfg) {}
};

TFServingBackend::TFServingBackend(const TFServingConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

TFServingBackend::~TFServingBackend() = default;

std::string TFServingBackend::backendName() const {
#if !defined(THEMIS_HAS_CURL)
    return "TF Serving (unavailable – rebuild with THEMIS_HAS_CURL=1)";
#else
    return "TF Serving (unavailable – rebuild with THEMIS_HAS_TF_SERVING=1)";
#endif
}

bool TFServingBackend::isAvailable() const { return false; }

MLServingResponse TFServingBackend::infer(const MLServingRequest& req) {
#if !defined(THEMIS_HAS_CURL)
    spdlog::warn("MLServing[TF]: libcurl not compiled in – "
                 "rebuild with THEMIS_HAS_CURL=1");
    const char* msg = "TF Serving backend requires libcurl. "
                      "Rebuild with -DTHEMIS_HAS_CURL=1.";
#else
    spdlog::warn("MLServing[TF]: backend disabled – "
                 "rebuild with THEMIS_HAS_TF_SERVING=1");
    const char* msg = "TF Serving backend not compiled in. "
                      "Rebuild with -DTHEMIS_HAS_TF_SERVING=1.";
#endif
    MLServingResponse resp;
    resp.status        = MLServingStatus::UNAVAILABLE;
    resp.error_message = msg;
    return resp;
}

#endif // THEMIS_HAS_TF_SERVING && THEMIS_HAS_CURL

// ============================================================================
// MLServingClient – Impl
// ============================================================================

struct MLServingClient::Impl {
    std::unique_ptr<IMLServingBackend> backend;
    MLBackendType                      requested_type;

    Impl(const MLServingConfig& cfg) : requested_type(cfg.backend) {
        switch (cfg.backend) {
            case MLBackendType::ONNX_RUNTIME:
                backend = std::make_unique<ONNXServingBackend>(cfg.onnx_config);
                break;
            case MLBackendType::TF_SERVING:
                backend = std::make_unique<TFServingBackend>(cfg.tf_config);
                break;
            case MLBackendType::AUTO:
            default: {
                // Prefer ONNX Runtime; fall back to TF Serving.
                auto onnx = std::make_unique<ONNXServingBackend>(cfg.onnx_config);
                if (onnx->isAvailable()) {
                    backend = std::move(onnx);
                } else {
                    auto tf = std::make_unique<TFServingBackend>(cfg.tf_config);
                    backend = std::move(tf);
                }
                break;
            }
        }
        spdlog::info("MLServingClient: active backend = '{}'",
                     backend->backendName());
    }
};

MLServingClient::MLServingClient(const MLServingConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

MLServingClient::~MLServingClient() = default;

bool MLServingClient::isBackendAvailable(MLBackendType type) const {
    if (type == MLBackendType::AUTO) {
        return impl_->backend && impl_->backend->isAvailable();
    }
    // Construct a temporary backend to check availability
    if (type == MLBackendType::ONNX_RUNTIME) {
        ONNXServingBackend tmp;
        return tmp.isAvailable();
    }
    TFServingBackend tmp;
    return tmp.isAvailable();
}

std::string MLServingClient::activeBackendName() const {
    return impl_->backend ? impl_->backend->backendName() : "(none)";
}

MLServingResponse MLServingClient::infer(const MLServingRequest& req) {
    if (!impl_->backend) {
        MLServingResponse resp;
        resp.status        = MLServingStatus::UNAVAILABLE;
        resp.error_message = "No backend configured";
        return resp;
    }
    return impl_->backend->infer(req);
}

MLServingResponse MLServingClient::inferFromDataPoint(
    const std::string& model_name,
    const DataPoint&   point,
    const std::string& input_name)
{
    // Extract numeric features in sorted (deterministic) order
    auto field_names = point.numericFieldNames();
    std::sort(field_names.begin(), field_names.end());

    std::vector<float> values;
    values.reserve(field_names.size());
    for (const auto& fname : field_names) {
        // Try double first, then int64
        if (auto v = point.get<double>(fname)) {
            values.push_back(static_cast<float>(*v));
        } else if (auto v = point.get<int64_t>(fname)) {
            values.push_back(static_cast<float>(*v));
        }
    }

    if (values.empty()) {
        MLServingResponse resp;
        resp.status        = MLServingStatus::INVALID_INPUT;
        resp.error_message = "DataPoint has no numeric features";
        return resp;
    }

    MLServingRequest req;
    req.model_name = model_name;
    req.inputs.push_back(MLTensor{
        input_name,
        { 1, static_cast<int64_t>(values.size()) },
        std::move(values)
    });

    return infer(req);
}

std::unique_ptr<IMLServingBackend>
MLServingClient::makeONNXBackend(const ONNXBackendConfig& config) {
    return std::make_unique<ONNXServingBackend>(config);
}

std::unique_ptr<IMLServingBackend>
MLServingClient::makeTFServingBackend(const TFServingConfig& config) {
    return std::make_unique<TFServingBackend>(config);
}

// ============================================================================
// Helpers
// ============================================================================

std::string mlServingStatusName(MLServingStatus status) {
    switch (status) {
        case MLServingStatus::OK:            return "OK";
        case MLServingStatus::UNAVAILABLE:   return "UNAVAILABLE";
        case MLServingStatus::INVALID_INPUT: return "INVALID_INPUT";
        case MLServingStatus::BACKEND_ERROR: return "BACKEND_ERROR";
        default:                             return "UNKNOWN";
    }
}

std::string mlBackendTypeName(MLBackendType type) {
    switch (type) {
        case MLBackendType::AUTO:         return "AUTO";
        case MLBackendType::ONNX_RUNTIME: return "ONNX_RUNTIME";
        case MLBackendType::TF_SERVING:   return "TF_SERVING";
        default:                          return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb
