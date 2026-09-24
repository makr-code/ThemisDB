// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief OpenTelemetry OTLP span emission for RAG operations.
///
/// Emits trace spans for each RAG operation (retrieval, reranking, freshness
/// check) with structured attributes and metrics. Supports baggage propagation
/// for distributed tracing and flame graph analysis.
///
/// @details
/// Span types:
/// - "rag.query": User query processing
/// - "rag.retrieve": Document retrieval
/// - "rag.rerank": Reranking execution
/// - "rag.refresh": Index refresh
/// - "rag.sla_check": SLA compliance check
///
/// Standard attributes:
/// - query_id: Unique query identifier
/// - user_id: User making query
/// - tenant_id: Tenant identifier
/// - latency_ms: Span duration
/// - success: true/false
/// - error_type: If failed
///
/// Metrics emitted (via OTLP metrics):
/// - rag.query.latency_ms: Query latency histogram
/// - rag.retrieval.count: Retrieval operation count
/// - rag.rerank.success_rate: % queries successfully reranked
/// - rag.staleness_ms: Index staleness gauge
/// - rag.cost_usd: Per-query cost
///
/// @code
/// auto tracer = std::make_unique<OTELSpanEmitter>("RAG");
/// {
///   auto span = tracer->StartSpan("rag.query", parent_context);
///   span->SetAttribute("query_id", query_id);
///   span->SetAttribute("user_id", user_id);
///   // Perform operation...
///   span->SetAttribute("result_count", 42);
///   span->EndSpan();  // Automatically emitted to OTLP
/// }
/// @endcode
class OTELSpanEmitter : public std::enable_shared_from_this<OTELSpanEmitter> {
 public:
  /// @brief Span context for distributed tracing.
  struct SpanContext {
    std::string trace_id;        ///< Distributed trace ID
    std::string parent_span_id;  ///< Parent span ID (optional)
    std::string baggage;         ///< W3C baggage header (optional)
  };

  /// @brief In-flight span.
  class Span {
   public:
    /// @brief Set string attribute.
    void SetAttribute(const std::string& key, const std::string& value);

    /// @brief Set numeric attribute.
    void SetAttribute(const std::string& key, uint64_t value);

    /// @brief Set boolean attribute.
    void SetAttribute(const std::string& key, bool value);

    /// @brief Record event with attributes.
    void RecordEvent(
        const std::string& event_name,
        const std::map<std::string, std::string>& attributes = {});

    /// @brief End span and emit to OTLP.
    void EndSpan();

    /// @brief End span with error.
    void EndSpanWithError(const std::string& error_type, const std::string& message);

    /// @brief Constructor.
    Span(const std::string& span_name, std::shared_ptr<OTELSpanEmitter> emitter);

   private:
    friend class OTELSpanEmitter;

    std::string span_name_;
    std::string span_id_;
    std::string trace_id_;
    std::string parent_span_id_;
    int64_t start_time_us_;
    std::map<std::string, std::string> string_attributes_;
    std::map<std::string, uint64_t> numeric_attributes_;
    std::map<std::string, bool> bool_attributes_;
    std::vector<std::pair<std::string, std::map<std::string, std::string>>> events_;
    std::weak_ptr<OTELSpanEmitter> emitter_;
  };

  /// @brief Constructor.
  /// @param service_name Service name (e.g., "RAG").
  /// @param otlp_endpoint OTLP exporter endpoint (default: localhost:4317).
  explicit OTELSpanEmitter(
      const std::string& service_name,
      const std::string& otlp_endpoint = "localhost:4317");

  /// @brief Start new span.
  /// @param span_name Span type (e.g., "rag.query").
  /// @param parent_context Parent span context (optional).
  /// @return Span handle for recording attributes/events.
  std::shared_ptr<Span> StartSpan(
      const std::string& span_name,
      const std::optional<SpanContext>& parent_context = std::nullopt);

  /// @brief Set W3C baggage.
  /// @param baggage W3C baggage header value.
  void SetBaggage(const std::string& baggage);

  /// @brief Get current W3C baggage.
  /// @return Current baggage header.
  std::string GetBaggage();

  /// @brief Enable/disable batch export.
  /// @param enabled true to batch spans before export.
  void SetBatchExportEnabled(bool enabled);

  /// @brief Set batch export size.
  /// @param batch_size Export after this many spans.
  void SetBatchExportSize(uint32_t batch_size);

  /// @brief Flush pending spans to OTLP.
  /// @return true if flush successful.
  bool Flush();

  /// @brief Get telemetry stats.
  /// @return Map of stat_name → count.
  std::map<std::string, uint64_t> GetStats();

 private:
  friend class Span;

  /// @brief Internal span buffer entry.
  struct SpanBuffer {
    std::string span_name;
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    int64_t start_time_us;
    int64_t end_time_us;
    std::map<std::string, std::string> string_attributes;
    std::map<std::string, uint64_t> numeric_attributes;
    std::map<std::string, bool> bool_attributes;
  };

  std::string service_name_;
  std::string otlp_endpoint_;
  std::string baggage_;
  void* otel_exporter_;  ///< Opaque OTEL exporter handle (nullptr = no-op)
  bool batch_export_enabled_;
  uint32_t batch_export_size_;
  uint32_t pending_spans_;
  uint64_t emitted_spans_;     ///< Total spans exported
  uint64_t dropped_spans_;     ///< Spans dropped (buffer overflow/errors)
  std::vector<SpanBuffer> pending_span_buffer_;

  /// @brief Emit single span to buffer for batched export.
  void EmitSpan(Span* span);

  /// @brief Export buffered spans to OTLP collector.
   /// @return true if export successful.
   bool ExportSpans();

 };

}  // namespace themis::rag
