/**
 * @file otlp_exporter.h
 * @brief Asynchronous OpenTelemetry Protocol (OTLP/HTTP) span export.
 *
 * @details Provides a background-threaded span exporter that batches and sends
 * request traces to an OpenTelemetry-compatible collector via HTTP POST,
 * following the OTLP JSON trace format.
 *
 * Core components:
 *  - `OtlpExporterConfig`: Configuration for endpoint, retry logic, batch sizes
 *  - `SpanData`: Lightweight span descriptor (trace ID, span ID, name, timestamps, attributes)
 *  - `OtlpExporter`: Producer/consumer pipeline with dedicated background flush thread
 *
 * Thread-safety and lifecycle:
 *  - `enqueue(SpanData)` is called from HTTP-handling threads (fast, mostly lock-free)
 *  - Background thread wakes periodically (flush_interval_ms) or when queue size exceeds batch_size
 *  - Batches are serialized to OTLP JSON and POSTed to the configured collector
 *  - On transient failures (HTTP 429/503, network errors), exponential backoff retries
 *    occur before the batch is dropped
 *  - `start()` launches the background thread; `stop()` flushes remaining spans and joins
 *
 * Observability guarantees:
 *  - Span export is bounded: max_queue_size prevents runaway memory accumulation
 *  - Export overhead is isolated to background thread (no critical-path latency)
 *  - Failed batches are logged but do not block ongoing span collection
 *  - Compatible with any OpenTelemetry collector that accepts OTLP/JSON (Jaeger, Zipkin, etc.)
 *
 * ### Usage
 * ```cpp
 * OtlpExporterConfig config;
 * config.endpoint = "http://jaeger:4318/v1/traces";
 * config.batch_size = 128;
 * config.flush_interval_ms = 5000;
 *
 * OtlpExporter exporter(config);
 * exporter.start();
 *
 * // From HTTP handler threads:
 * SpanData span;
 * span.trace_id = correlation_id;
 * span.name = "HTTP GET /v1/entity";
 * span.start_time_unix_nano = start_ns;
 * span.end_time_unix_nano = end_ns;
 * span.status_code = 1; // OK
 * exporter.enqueue(span);
 *
 * // At shutdown:
 * exporter.stop(); // flushes and joins background thread
 * ```
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <curl/curl.h>

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/registry.h>
#endif

namespace themis {
namespace api {

/**
 * @brief Configuration for the OTLP HTTP span exporter.
 *
 * Mirrors the fields in `config/networking/otlp.yaml`.
 *
 * ### Defaults
 * All default values match the YAML config.  Callers may construct a default-
 * initialised instance and override only the fields they care about.
 */
struct OtlpExporterConfig {
    bool        enabled        = false;
    std::string endpoint       = "http://localhost:4318/v1/traces";
    std::string service_name   = "themisdb";
    std::string service_version;
    int         timeout_ms     = 5000;
    size_t      max_queue_size = 8192;
    size_t      batch_size     = 64;
    int         flush_interval_ms = 5000;

    // TLS (leave empty to use plain HTTP)
    std::string tls_ca_cert;
    std::string tls_client_cert;
    std::string tls_client_key;

    // Optional Bearer token; placed in `Authorization: Bearer <token>`.
    std::string auth_header;

    // Extra static HTTP headers sent with every export request.
    std::unordered_map<std::string, std::string> extra_headers;

    // Retry configuration for transient export failures.
    // On a retriable error (HTTP 429/503 or a curl transport error), the
    // exporter retries up to `max_export_retries` times before dropping the
    // batch.  The wait before retry i (1-based) is:
    //   delay = retry_initial_delay_ms * 2^(i-1)   (100 ms, 200 ms, 400 ms …)
    int max_export_retries     = 3;   ///< Number of retry attempts after initial failure (0 = no retries).
    int retry_initial_delay_ms = 100; ///< Initial retry back-off delay in milliseconds (>= 1); doubled each attempt.
};

/**
 * @brief Lightweight span descriptor used by OtlpExporter.
 *
 * Callers fill in the fields they know; the exporter serialises the struct
 * into OTLP JSON format.
 */
struct SpanData {
    std::string  trace_id;       ///< 32 hex chars (128-bit); e.g. from X-Correlation-ID
    std::string  span_id;        ///< 16 hex chars (64-bit); unique per request leg
    std::string  parent_span_id; ///< 16 hex chars, or empty for root spans
    std::string  name;           ///< Span name, e.g. "HTTP GET /v1/entity/{id}"
    int64_t      start_time_unix_nano = 0; ///< epoch nanoseconds
    int64_t      end_time_unix_nano   = 0; ///< epoch nanoseconds
    int          status_code = 0; ///< 0 = Unset, 1 = OK, 2 = Error (OTLP StatusCode)
    std::string  status_message;

    /// Per-span key/value attributes (string values only for simplicity).
    std::unordered_map<std::string, std::string> attributes;
};

/**
 * @brief Asynchronous OTLP/HTTP span exporter.
 *
 * Implements a background-thread producer/consumer pipeline:
 *  - `enqueue(SpanData)` is called from HTTP-handling threads (fast, lock-free for
 *    normal operation).
 *  - A single background thread batches queued spans and sends them to the
 *    configured OTLP collector using a synchronous libcurl HTTP POST.
 *
 * The JSON payload sent to the collector follows the OTLP JSON trace format:
 *   https://opentelemetry.io/docs/specs/otlp/#json-encoding
 *
 * ### Thread safety
 * `enqueue()` is safe to call from any thread concurrently.
 * `start()` / `stop()` should only be called once at server startup/shutdown.
 *
 * ### Lifecycle
 * ```cpp
 * OtlpExporter exporter(config);
 * exporter.start();                   // launches background flush thread
 * // … normal operation …
 * exporter.enqueue(span);
 * // …
 * exporter.stop();                    // flushes remaining spans and joins thread
 * ```
 */
class OtlpExporter {
public:
    explicit OtlpExporter(OtlpExporterConfig config = {});
    ~OtlpExporter();

    OtlpExporter(const OtlpExporter&) = delete;
    OtlpExporter& operator=(const OtlpExporter&) = delete;
    OtlpExporter(OtlpExporter&&) = delete;
    OtlpExporter& operator=(OtlpExporter&&) = delete;

    /**
     * @brief Start the background flush thread.
     *
     * No-op if `config.enabled` is false or if already started.
     */
    void start();

    /**
     * @brief Stop the background flush thread and flush remaining spans.
     *
     * Blocks until the background thread has exited.
     */
    void stop();

    /**
     * @brief Enqueue a finished span for asynchronous export.
     *
     * If the queue is full (`max_queue_size`), the oldest span is dropped and
     * a warning is logged.  This call never blocks.
     *
     * No-op if `config.enabled` is false.
     *
     * @param span  Completed span to export.
     */
    void enqueue(SpanData span);

    /**
     * @brief Return the total number of spans successfully exported since start().
     */
    uint64_t exportedSpanCount() const noexcept;

    /**
     * @brief Return the total number of spans dropped due to a full queue since start().
     */
    uint64_t droppedSpanCount() const noexcept;

    /// Return the current configuration.
    const OtlpExporterConfig& config() const noexcept { return config_; }

#ifdef THEMIS_HAS_PROMETHEUS
    /**
     * @brief Register OTLP span counters in a Prometheus registry.
     *
     * Must be called before start().  Registers:
     *  - `otlp_spans_exported_total`
     *  - `otlp_spans_dropped_total`
     *
     * No-op when Prometheus support is not compiled in.
     *
     * @param registry  Shared Prometheus registry instance.
     */
    void setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry);
#endif

private:
    void flushLoop();
    void flushBatch(std::vector<SpanData>& batch);

    static std::string buildOtlpJson(const OtlpExporterConfig& cfg,
                                     const std::vector<SpanData>& spans);

    OtlpExporterConfig      config_;

    std::deque<SpanData>    queue_;
    mutable std::mutex      queue_mutex_;
    std::condition_variable queue_cv_;

    std::thread             flush_thread_;
    std::atomic<bool>       stop_{false};
    std::atomic<uint64_t>   exported_count_{0};
    std::atomic<uint64_t>   dropped_count_{0};

    // Persistent libcurl handle — created once in start(), reused per flush batch.
    CURL*              curl_handle_  = nullptr;
    struct curl_slist* curl_headers_ = nullptr;

#ifdef THEMIS_HAS_PROMETHEUS
    std::shared_ptr<prometheus::Registry> prom_registry_;
    prometheus::Counter* prom_exported_{nullptr};
    prometheus::Counter* prom_dropped_{nullptr};
#endif
};

} // namespace api
} // namespace themis

