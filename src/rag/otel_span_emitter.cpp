// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/otel_span_emitter.h"

#include <chrono>

namespace themis::rag {

// Span implementation
void OTELSpanEmitter::Span::SetAttribute(
    const std::string& key,
    const std::string& value) {
  string_attributes_[key] = value;
}

void OTELSpanEmitter::Span::SetAttribute(const std::string& key, uint64_t value) {
  numeric_attributes_[key] = value;
}

void OTELSpanEmitter::Span::SetAttribute(const std::string& key, bool value) {
  bool_attributes_[key] = value;
}

void OTELSpanEmitter::Span::RecordEvent(
    const std::string& event_name,
    const std::map<std::string, std::string>& attributes) {
  events_.push_back({event_name, attributes});
}

void OTELSpanEmitter::Span::EndSpan() {
  // TODO: Emit span to OTLP exporter via emitter_
}

void OTELSpanEmitter::Span::EndSpanWithError(
    const std::string& error_type,
    const std::string& message) {
  SetAttribute("error.type", error_type);
  SetAttribute("error.message", message);
  EndSpan();
}

OTELSpanEmitter::Span::Span(
    const std::string& span_name,
    std::shared_ptr<OTELSpanEmitter> emitter)
    : span_name_(span_name),
      emitter_(emitter),
      start_time_us_(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count()) {
  // Generate trace and span IDs
  trace_id_ = "trace_" + std::to_string(start_time_us_);
  span_id_ = "span_" + std::to_string(start_time_us_);
}

// OTELSpanEmitter implementation
OTELSpanEmitter::OTELSpanEmitter(
    const std::string& service_name,
    const std::string& otlp_endpoint)
    : service_name_(service_name),
      otlp_endpoint_(otlp_endpoint),
      otel_exporter_(nullptr),
      batch_export_enabled_(false),
      batch_export_size_(100),
      pending_spans_(0) {}

std::shared_ptr<OTELSpanEmitter::Span> OTELSpanEmitter::StartSpan(
    const std::string& span_name,
    const std::optional<SpanContext>& parent_context) {
  auto span = std::make_shared<Span>(span_name, nullptr);  // Pass nullptr for now

  if (parent_context) {
    span->trace_id_ = parent_context->trace_id;
    span->parent_span_id_ = parent_context->parent_span_id;
  }

  pending_spans_++;
  return span;
}

void OTELSpanEmitter::SetBaggage(const std::string& baggage) {
  baggage_ = baggage;
}

std::string OTELSpanEmitter::GetBaggage() {
  return baggage_;
}

void OTELSpanEmitter::SetBatchExportEnabled(bool enabled) {
  batch_export_enabled_ = enabled;
}

void OTELSpanEmitter::SetBatchExportSize(uint32_t batch_size) {
  batch_export_size_ = batch_size;
}

bool OTELSpanEmitter::Flush() {
  // TODO: Flush all pending spans to OTLP exporter
  pending_spans_ = 0;
  return true;
}

std::map<std::string, uint64_t> OTELSpanEmitter::GetStats() {
  std::map<std::string, uint64_t> stats;
  stats["pending_spans"] = pending_spans_;
  stats["batch_export_enabled"] = batch_export_enabled_ ? 1 : 0;
  stats["batch_export_size"] = batch_export_size_;
  return stats;
}

}  // namespace themis::rag
