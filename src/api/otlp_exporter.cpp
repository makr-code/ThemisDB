/**
 * @file otlp_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "api/otlp_exporter.h"
#include <stdexcept>
#include "utils/logger.h"

#include <chrono>
#include <curl/curl.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#include "utils/logger.h"

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>
#endif

namespace themis {
namespace api {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// libcurl write callback — appends received bytes to a std::string.
static size_t curlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *buf = static_cast<std::string *>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

/// Convert a 128-bit hex string (correlation ID or UUID without dashes) to a
/// lowercase 32-hex-char trace ID string.  UUIDs with dashes are normalised.
static std::string normaliseTraceId(const std::string &raw) {
    // Strip dashes from UUIDs ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
    std::string out;
    out.reserve(32);
    for (char c : raw) {
        if (c != '-') {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    // Pad to 32 hex chars (zero-pad on the right if shorter, truncate if longer)
    if (out.size() < 32) {
        out.resize(32, '0');
    } else if (out.size() > 32) {
        out = out.substr(0, 32);
    }
    return out;
}

/// Build a random 16-hex-char span ID from the lower 64 bits of a trace ID.
/// When span_id is empty we derive it deterministically from trace_id + "span"
/// to avoid a full random generator in the hot path.
static std::string deriveSpanId(const std::string &trace_id_32) {
    // Use the last 16 hex chars of the trace ID as the span ID.
    if (trace_id_32.size() >= 16) {
        return trace_id_32.substr(16, 16);
    }
    return trace_id_32 + std::string(16 - trace_id_32.size(), '0');
}

/// StatusCode constants (OTLP spec):
static constexpr int kStatusUnset = 0;
static constexpr int kStatusOk    = 1;
static constexpr int kStatusError = 2;

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

OtlpExporter::OtlpExporter(OtlpExporterConfig config) : config_(std::move(config)) {}

OtlpExporter::~OtlpExporter() {
    stop();
}

// ---------------------------------------------------------------------------
// start() / stop()
// ---------------------------------------------------------------------------

void OtlpExporter::start() {
    if (!config_.enabled) {
        return;
    }
    if (flush_thread_.joinable()) {
        return; // already running
    }

    // -----------------------------------------------------------------------
    // Initialise a persistent libcurl handle so TCP connections are reused
    // across flush batches (avoids a new TCP/TLS handshake per batch).
    // -----------------------------------------------------------------------
    curl_handle_ = curl_easy_init();
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_URL, config_.endpoint.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_FORBID_REUSE, 0L);  // allow connection reuse
        curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPALIVE, 1L); // keep TCP alive

        // TLS settings
        if (!config_.tls_ca_cert.empty()) {
            curl_easy_setopt(curl_handle_, CURLOPT_CAINFO, config_.tls_ca_cert.c_str());
        }
        if (!config_.tls_client_cert.empty()) {
            curl_easy_setopt(curl_handle_, CURLOPT_SSLCERT, config_.tls_client_cert.c_str());
        }
        if (!config_.tls_client_key.empty()) {
            curl_easy_setopt(curl_handle_, CURLOPT_SSLKEY, config_.tls_client_key.c_str());
        }

        // Build static headers once (Content-Type, optional auth/custom headers)
        curl_headers_ = curl_slist_append(curl_headers_, "Content-Type: application/json");
        if (!config_.auth_header.empty()) {
            const std::string auth = "Authorization: Bearer " + config_.auth_header;
            curl_headers_          = curl_slist_append(curl_headers_, auth.c_str());
        }
        for (const auto &[k, v] : config_.extra_headers) {
            const std::string hdr = k + ": " + v;
            curl_headers_         = curl_slist_append(curl_headers_, hdr.c_str());
        }
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, curl_headers_);
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    } else {
        THEMIS_WARN("OtlpExporter: curl_easy_init() failed at start — will retry per batch");
    }

    // -----------------------------------------------------------------------
    // Register Prometheus counters (if a registry was provided).
    // -----------------------------------------------------------------------
#ifdef THEMIS_HAS_PROMETHEUS
    if (prom_registry_) {
        auto &exp_family = prometheus::BuildCounter()
                               .Name("otlp_spans_exported_total")
                               .Help("Total number of spans successfully exported to the OTLP collector.")
                               .Register(*prom_registry_);
        prom_exported_   = &exp_family.Add({{"service", config_.service_name}});

        auto &drop_family = prometheus::BuildCounter()
                                .Name("otlp_spans_dropped_total")
                                .Help("Total number of spans dropped due to a full queue or export failure.")
                                .Register(*prom_registry_);
        prom_dropped_     = &drop_family.Add({{"service", config_.service_name}});
    }
#endif

    stop_.store(false, std::memory_order_relaxed);
    try {
        flush_thread_ = std::thread(&OtlpExporter::flushLoop, this);
    } catch (const std::system_error &) {
        // Thread creation failed; clean up curl resources and re-throw.
        if (curl_headers_) {
            curl_slist_free_all(curl_headers_);
            curl_headers_ = nullptr;
        }
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
            curl_handle_ = nullptr;
        }
        THEMIS_ERROR("OtlpExporter: failed to create flush thread — exporter disabled");
        throw;
    } catch (const std::exception &) {
        // Thread creation failed; clean up curl resources and re-throw.
        if (curl_headers_) {
            curl_slist_free_all(curl_headers_);
            curl_headers_ = nullptr;
        }
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
            curl_handle_ = nullptr;
        }
        THEMIS_ERROR("OtlpExporter: failed to create flush thread — exporter disabled");
        throw;
    } catch (const std::string &) {
        // Thread creation failed; clean up curl resources and re-throw.
        if (curl_headers_) {
            curl_slist_free_all(curl_headers_);
            curl_headers_ = nullptr;
        }
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
            curl_handle_ = nullptr;
        }
        THEMIS_ERROR("OtlpExporter: failed to create flush thread — exporter disabled");
        throw;
    } catch (const char *) {
        // Thread creation failed; clean up curl resources and re-throw.
        if (curl_headers_) {
            curl_slist_free_all(curl_headers_);
            curl_headers_ = nullptr;
        }
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
            curl_handle_ = nullptr;
        }
        THEMIS_ERROR("OtlpExporter: failed to create flush thread — exporter disabled");
        throw;
    }
    THEMIS_INFO("OtlpExporter: started (endpoint={})", config_.endpoint);
}

void OtlpExporter::stop() {
    if (!flush_thread_.joinable()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        stop_.store(true, std::memory_order_relaxed);
    }
    queue_cv_.notify_all();
    flush_thread_.join();

    // Clean up persistent curl resources after the background thread has exited.
    if (curl_headers_) {
        curl_slist_free_all(curl_headers_);
        curl_headers_ = nullptr;
    }
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
        curl_handle_ = nullptr;
    }

    THEMIS_INFO("OtlpExporter: stopped (exported={}, dropped={})", exported_count_.load(), dropped_count_.load());
}

// ---------------------------------------------------------------------------
// enqueue()
// ---------------------------------------------------------------------------

void OtlpExporter::enqueue(SpanData span) {
    if (!config_.enabled) {
        return;
    }

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        if (queue_.size() >= config_.max_queue_size) {
            // Drop the oldest span to make room (O(1) with std::deque)
            queue_.pop_front();
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
#ifdef THEMIS_HAS_PROMETHEUS
            if (prom_dropped_) {
                prom_dropped_->Increment(1.0);
            }
#endif
            THEMIS_WARN("OtlpExporter: ERR_OTLP_QUEUE_FULL — queue at capacity ({}) and oldest span was "
                    "dropped; increase max_queue_size or reduce flush_interval_ms to avoid data loss",
                    config_.max_queue_size);
        }
        queue_.push_back(std::move(span));
    }
    queue_cv_.notify_one();
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

uint64_t OtlpExporter::exportedSpanCount() const noexcept {
    return exported_count_.load(std::memory_order_relaxed);
}

uint64_t OtlpExporter::droppedSpanCount() const noexcept {
    return dropped_count_.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Prometheus registry
// ---------------------------------------------------------------------------

#ifdef THEMIS_HAS_PROMETHEUS
void OtlpExporter::setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry) {
    prom_registry_ = std::move(registry);
}
#endif

// ---------------------------------------------------------------------------
// Background flush loop
// ---------------------------------------------------------------------------

void OtlpExporter::flushLoop() {
    const auto flush_interval = std::chrono::milliseconds(config_.flush_interval_ms);

    std::vector<SpanData> batch;
    batch.reserve(config_.batch_size);

    while (true) {
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            queue_cv_.wait_for(lk, flush_interval, [this] {
                return stop_.load(std::memory_order_relaxed) || queue_.size() >= config_.batch_size;
            });

            const size_t take = std::min(queue_.size(), config_.batch_size);
            if (take > 0) {
                const auto take_offset = static_cast<std::ptrdiff_t>(take);
                batch.assign(std::make_move_iterator(queue_.begin()),
                             std::make_move_iterator(queue_.begin() + take_offset));
                queue_.erase(queue_.begin(), queue_.begin() + take_offset);
            }
        }

        if (!batch.empty()) {
            flushBatch(batch);
            batch.clear();
        }

        if (stop_.load(std::memory_order_relaxed)) {
            // Drain the remaining queue before exiting
            std::vector<SpanData> remaining;
            {
                std::lock_guard<std::mutex> lk(queue_mutex_);
                remaining.assign(std::make_move_iterator(queue_.begin()), std::make_move_iterator(queue_.end()));
                queue_.clear();
            }
            if (!remaining.empty()) {
                flushBatch(remaining);
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// flushBatch() — builds OTLP JSON and sends via libcurl
// ---------------------------------------------------------------------------

namespace {
/// Returns true for HTTP status codes that are transient and safe to retry.
static bool isRetriableHttpCode(long code) noexcept {
    return code == 429 || code == 503;
}

/// Returns true for curl errors that are transient network/transport failures.
/// Configuration errors (bad URL, unsupported protocol, etc.) are excluded
/// because retrying them will never succeed.
static bool isRetriableCurlError(CURLcode code) noexcept {
    switch (code) {
        case CURLE_COULDNT_RESOLVE_HOST:
        [[fallthrough]];\n        case CURLE_COULDNT_CONNECT:
        [[fallthrough]];\n        case CURLE_OPERATION_TIMEDOUT:
        [[fallthrough]];\n        case CURLE_SEND_ERROR:
        [[fallthrough]];\n        case CURLE_RECV_ERROR:
        [[fallthrough]];\n        case CURLE_GOT_NOTHING:
        [[fallthrough]];\n        case CURLE_SSL_CONNECT_ERROR:
            return true;
        default:
            return false;
    }
}
} // namespace

void OtlpExporter::flushBatch(std::vector<SpanData> &batch) {
    const std::string payload = buildOtlpJson(config_, batch);

    const int max_attempts = 1 + std::max(0, config_.max_export_retries);
    int delay_ms           = std::max(1, config_.retry_initial_delay_ms);

    // Fall back to a temporary handle if the persistent one was not initialised
    // (e.g. start() was not called or curl_easy_init() failed at start time).
    CURL *curl                     = curl_handle_;
    bool owns_handle               = false;
    struct curl_slist *tmp_headers = nullptr;
    if (!curl) {
        curl        = curl_easy_init();
        owns_handle = true;
        if (!curl) {
            THEMIS_ERROR("OtlpExporter: ERR_OTLP_CURL_INIT_FAILED — curl_easy_init() returned null; "
                    "{} spans lost. Verify libcurl is correctly linked and the process has "
                    "sufficient memory.", batch.size());
            const auto n = static_cast<uint64_t>(batch.size());
            dropped_count_.fetch_add(n, std::memory_order_relaxed);
#ifdef THEMIS_HAS_PROMETHEUS
            if (prom_dropped_) {
                prom_dropped_->Increment(static_cast<double>(n));
            }
#endif
            return;
        }
        // Set required options on the temporary handle (same as in start())
        curl_easy_setopt(curl, CURLOPT_URL, config_.endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(config_.timeout_ms));
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (!config_.tls_ca_cert.empty()) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, config_.tls_ca_cert.c_str());
        }
        if (!config_.tls_client_cert.empty()) {
            curl_easy_setopt(curl, CURLOPT_SSLCERT, config_.tls_client_cert.c_str());
        }
        if (!config_.tls_client_key.empty()) {
            curl_easy_setopt(curl, CURLOPT_SSLKEY, config_.tls_client_key.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

        tmp_headers = curl_slist_append(tmp_headers, "Content-Type: application/json");
        if (!config_.auth_header.empty()) {
            const std::string auth = "Authorization: Bearer " + config_.auth_header;
            tmp_headers            = curl_slist_append(tmp_headers, auth.c_str());
        }
        for (const auto &[k, v] : config_.extra_headers) {
            const std::string hdr = k + ": " + v;
            tmp_headers           = curl_slist_append(tmp_headers, hdr.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, tmp_headers);
    }

    // -----------------------------------------------------------------------
    // Retry loop with exponential back-off.
    //
    // Policy:
    //   • max_attempts  = 1 + config_.max_export_retries  (first attempt is
    //                     attempt 0; retries start at attempt 1)
    //   • Initial delay = config_.retry_initial_delay_ms (default: 50 ms)
    //   • Back-off      = delay_ms *= 2 after each failed attempt
    //   • Retriable errors: CURLE_COULDNT_CONNECT, CURLE_OPERATION_TIMEDOUT,
    //                       CURLE_SEND_ERROR, CURLE_RECV_ERROR, HTTP 429, 503
    //   • Non-retriable : configuration errors, auth failures (4xx except 429)
    //
    // If all attempts fail the batch is counted in dropped_count_ and the
    // ERR_OTLP_EXPORT_FAILED message is emitted so operators can act on it.
    // See docs/API_TRANSPORT_RUNBOOK.md § ERR_OTLP_EXPORT_FAILED.
    // -----------------------------------------------------------------------
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (attempt > 0) {
            THEMIS_INFO("OtlpExporter: retry attempt {}/{} after {}ms back-off (ERR_OTLP_EXPORT_RETRY)",
                        attempt, config_.max_export_retries, delay_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            delay_ms *= 2;
        }

        std::string response_body;

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

        const CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
            const auto n = static_cast<uint64_t>(batch.size());
            exported_count_.fetch_add(n, std::memory_order_relaxed);
#ifdef THEMIS_HAS_PROMETHEUS
            if (prom_exported_) {
                prom_exported_->Increment(static_cast<double>(n));
            }
#endif
            THEMIS_DEBUG("OtlpExporter: exported {} spans (HTTP {})", batch.size(), http_code);
            if (owns_handle) {
                if (tmp_headers) {
                    curl_slist_free_all(tmp_headers);
                }
                curl_easy_cleanup(curl);
            }
            return;
        }

        const bool has_more_attempts = (attempt + 1 < max_attempts);

        if (res != CURLE_OK) {
            const bool retriable = has_more_attempts && isRetriableCurlError(res);
            THEMIS_WARN("OtlpExporter: ERR_OTLP_EXPORT_FAILED — curl error: {}{}",
                        curl_easy_strerror(res),
                        retriable ? " — will retry" : " — spans lost; check ERR_OTLP_COLLECTOR_UNREACHABLE");
            if (!retriable) {
                break;
            }
        } else {
            // Non-2xx HTTP response
            const bool retriable = has_more_attempts && isRetriableHttpCode(http_code);
            THEMIS_WARN("OtlpExporter: ERR_OTLP_EXPORT_FAILED — collector returned HTTP {}{}",
                        http_code,
                        retriable ? " — will retry" : " — spans lost; verify OTLP_ENDPOINT and collector health");
            if (!retriable) {
                break;
            }
        }
    }

    if (owns_handle) {
        if (tmp_headers) {
            curl_slist_free_all(tmp_headers);
        }
        curl_easy_cleanup(curl);
    }

    const auto n = static_cast<uint64_t>(batch.size());
    dropped_count_.fetch_add(n, std::memory_order_relaxed);
#ifdef THEMIS_HAS_PROMETHEUS
    if (prom_dropped_) {
        prom_dropped_->Increment(static_cast<double>(n));
    }
#endif
}

// ---------------------------------------------------------------------------
// buildOtlpJson() — OTLP JSON trace format
// ---------------------------------------------------------------------------
// Reference: https://opentelemetry.io/docs/specs/otlp/#json-encoding
// Format:
// {
//   "resourceSpans": [{
//     "resource": {"attributes": [{"key":"service.name","value":{"stringValue":"…"}}]},
//     "scopeSpans": [{
//       "scope": {"name":"themisdb"},
//       "spans": [ … SpanData … ]
//     }]
//   }]
// }

/*static*/
std::string OtlpExporter::buildOtlpJson(const OtlpExporterConfig &cfg, const std::vector<SpanData> &spans) {
    // Resource attributes
    json resource_attrs = json::array();
    auto addAttr        = [&](const std::string &key, const std::string &value) {
        resource_attrs.push_back({{"key", key}, {"value", {{"stringValue", value}}}});
    };
    addAttr("service.name", cfg.service_name);
    if (!cfg.service_version.empty()) {
        addAttr("service.version", cfg.service_version);
    }

    json span_array = json::array();
    for (const auto &s : spans) {
        const std::string trace_id_norm = normaliseTraceId(s.trace_id);
        const std::string span_id       = s.span_id.empty() ? deriveSpanId(trace_id_norm) : s.span_id.substr(0, 16);

        json span_obj;
        span_obj["traceId"]           = trace_id_norm;
        span_obj["spanId"]            = span_id;
        span_obj["name"]              = s.name;
        span_obj["startTimeUnixNano"] = std::to_string(s.start_time_unix_nano);
        span_obj["endTimeUnixNano"]   = std::to_string(s.end_time_unix_nano);
        span_obj["kind"]              = 2; // SPAN_KIND_SERVER

        if (!s.parent_span_id.empty()) {
            span_obj["parentSpanId"] = s.parent_span_id.substr(0, 16);
        }

        // Status
        json status;
        status["code"] = s.status_code;
        if (!s.status_message.empty()) {
            status["message"] = s.status_message;
        }
        span_obj["status"] = status;

        // Attributes
        json attrs = json::array();
        for (const auto &[k, v] : s.attributes) {
            attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
        }
        span_obj["attributes"] = attrs;

        span_array.push_back(span_obj);
    }

    json payload = {
        {"resourceSpans",
         json::array({{{"resource", {{"attributes", resource_attrs}}},
                       {"scopeSpans", json::array({{{"scope", {{"name", "themisdb"}}}, {"spans", span_array}}})}}})}};

    return payload.dump();
}

} // namespace api
} // namespace themis
