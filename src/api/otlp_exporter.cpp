#include "api/otlp_exporter.h"
#include "utils/logger.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace themis {
namespace api {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// libcurl write callback — appends received bytes to a std::string.
static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

/// Convert a 128-bit hex string (correlation ID or UUID without dashes) to a
/// lowercase 32-hex-char trace ID string.  UUIDs with dashes are normalised.
static std::string normaliseTraceId(const std::string& raw)
{
    // Strip dashes from UUIDs ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
    std::string out;
    out.reserve(32);
    for (char c : raw) {
        if (c != '-') out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    // Pad to 32 hex chars (zero-pad on the right if shorter, truncate if longer)
    if (out.size() < 32) out.resize(32, '0');
    else if (out.size() > 32) out = out.substr(0, 32);
    return out;
}

/// Build a random 16-hex-char span ID from the lower 64 bits of a trace ID.
/// When span_id is empty we derive it deterministically from trace_id + "span"
/// to avoid a full random generator in the hot path.
static std::string deriveSpanId(const std::string& trace_id_32)
{
    // Use the last 16 hex chars of the trace ID as the span ID.
    if (trace_id_32.size() >= 16) return trace_id_32.substr(16, 16);
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

OtlpExporter::OtlpExporter(OtlpExporterConfig config)
    : config_(std::move(config))
{}

OtlpExporter::~OtlpExporter()
{
    stop();
}

// ---------------------------------------------------------------------------
// start() / stop()
// ---------------------------------------------------------------------------

void OtlpExporter::start()
{
    if (!config_.enabled) return;
    if (flush_thread_.joinable()) return; // already running

    stop_.store(false, std::memory_order_relaxed);
    flush_thread_ = std::thread(&OtlpExporter::flushLoop, this);
    THEMIS_INFO("OtlpExporter: started (endpoint={})", config_.endpoint);
}

void OtlpExporter::stop()
{
    if (!flush_thread_.joinable()) return;

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        stop_.store(true, std::memory_order_relaxed);
    }
    queue_cv_.notify_all();
    flush_thread_.join();
    THEMIS_INFO("OtlpExporter: stopped (exported={}, dropped={})",
                exported_count_.load(), dropped_count_.load());
}

// ---------------------------------------------------------------------------
// enqueue()
// ---------------------------------------------------------------------------

void OtlpExporter::enqueue(SpanData span)
{
    if (!config_.enabled) return;

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        if (queue_.size() >= config_.max_queue_size) {
            // Drop the oldest span to make room
            queue_.erase(queue_.begin());
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            THEMIS_WARN("OtlpExporter: queue full ({}) — oldest span dropped",
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
// Background flush loop
// ---------------------------------------------------------------------------

void OtlpExporter::flushLoop()
{
    const auto flush_interval =
        std::chrono::milliseconds(config_.flush_interval_ms);

    std::vector<SpanData> batch;
    batch.reserve(config_.batch_size);

    while (true) {
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            queue_cv_.wait_for(lk, flush_interval, [this] {
                return stop_.load(std::memory_order_relaxed)
                    || queue_.size() >= config_.batch_size;
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
                remaining.swap(queue_);
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

void OtlpExporter::flushBatch(std::vector<SpanData>& batch)
{
    const std::string payload = buildOtlpJson(config_, batch);

    CURL* curl = curl_easy_init();
    if (!curl) {
        THEMIS_ERROR("OtlpExporter: curl_easy_init() failed — {} spans lost", batch.size());
        return;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (!config_.auth_header.empty()) {
        const std::string auth = "Authorization: Bearer " + config_.auth_header;
        headers = curl_slist_append(headers, auth.c_str());
    }
    for (const auto& [k, v] : config_.extra_headers) {
        const std::string hdr = k + ": " + v;
        headers = curl_slist_append(headers, hdr.c_str());
    }

    std::string response_body;

    curl_easy_setopt(curl, CURLOPT_URL,            config_.endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_POST,            1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,      payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,   static_cast<long>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,      static_cast<long>(config_.timeout_ms));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,  1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_body);

    // TLS settings
    if (!config_.tls_ca_cert.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, config_.tls_ca_cert.c_str());
    if (!config_.tls_client_cert.empty())
        curl_easy_setopt(curl, CURLOPT_SSLCERT, config_.tls_client_cert.c_str());
    if (!config_.tls_client_key.empty())
        curl_easy_setopt(curl, CURLOPT_SSLKEY, config_.tls_client_key.c_str());

    const CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        THEMIS_WARN("OtlpExporter: export failed (curl error: {}) — {} spans lost",
                    curl_easy_strerror(res), batch.size());
    } else if (http_code < 200 || http_code >= 300) {
        THEMIS_WARN("OtlpExporter: collector returned HTTP {} — {} spans lost",
                    http_code, batch.size());
    } else {
        exported_count_.fetch_add(static_cast<uint64_t>(batch.size()),
                                  std::memory_order_relaxed);
        THEMIS_DEBUG("OtlpExporter: exported {} spans (HTTP {})", batch.size(), http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
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
std::string OtlpExporter::buildOtlpJson(const OtlpExporterConfig& cfg,
                                        const std::vector<SpanData>& spans)
{
    // Resource attributes
    json resource_attrs = json::array();
    auto addAttr = [&](const std::string& key, const std::string& value) {
        resource_attrs.push_back({{"key", key}, {"value", {{"stringValue", value}}}});
    };
    addAttr("service.name",    cfg.service_name);
    if (!cfg.service_version.empty())
        addAttr("service.version", cfg.service_version);

    json span_array = json::array();
    for (const auto& s : spans) {
        const std::string trace_id_norm = normaliseTraceId(s.trace_id);
        const std::string span_id = s.span_id.empty()
            ? deriveSpanId(trace_id_norm)
            : s.span_id.substr(0, 16);

        json span_obj;
        span_obj["traceId"]          = trace_id_norm;
        span_obj["spanId"]           = span_id;
        span_obj["name"]             = s.name;
        span_obj["startTimeUnixNano"] = std::to_string(s.start_time_unix_nano);
        span_obj["endTimeUnixNano"]   = std::to_string(s.end_time_unix_nano);
        span_obj["kind"]             = 2; // SPAN_KIND_SERVER

        if (!s.parent_span_id.empty()) {
            span_obj["parentSpanId"] = s.parent_span_id.substr(0, 16);
        }

        // Status
        json status;
        status["code"] = s.status_code;
        if (!s.status_message.empty()) status["message"] = s.status_message;
        span_obj["status"] = status;

        // Attributes
        json attrs = json::array();
        for (const auto& [k, v] : s.attributes) {
            attrs.push_back({{"key", k}, {"value", {{"stringValue", v}}}});
        }
        span_obj["attributes"] = attrs;

        span_array.push_back(span_obj);
    }

    json payload = {
        {"resourceSpans", json::array({
            {
                {"resource", {{"attributes", resource_attrs}}},
                {"scopeSpans", json::array({
                    {
                        {"scope", {{"name", "themisdb"}}},
                        {"spans", span_array}
                    }
                })}
            }
        })}
    };

    return payload.dump();
}

} // namespace api
} // namespace themis
