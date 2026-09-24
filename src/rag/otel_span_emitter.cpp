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
  // Calculate span duration
  auto end_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  int64_t duration_us = end_time_us - start_time_us_;

  // Add duration attribute
  numeric_attributes_["duration_us"] = duration_us;

  // Emit span via emitter if available
  auto emitter = emitter_.lock();
  if (emitter) {
    emitter->EmitSpan(this);
  }
}

void OTELSpanEmitter::Span::EndSpanWithError(
    const std::string& error_type,
    const std::string& message) {
  SetAttribute("error.type", error_type);
  SetAttribute("error.message", message);
  SetAttribute("error", true);
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
      pending_spans_(0),
      emitted_spans_(0),
      dropped_spans_(0) {}

std::shared_ptr<OTELSpanEmitter::Span> OTELSpanEmitter::StartSpan(
    const std::string& span_name,
    const std::optional<SpanContext>& parent_context) {
  // Create span with shared_ptr to this emitter for emission
  auto span = std::make_shared<Span>(span_name, shared_from_this());

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
  // Flush all pending spans to OTLP exporter
  if (!pending_span_buffer_.empty()) {
    return ExportSpans();
  }
  pending_spans_ = 0;
  return true;
}

std::map<std::string, uint64_t> OTELSpanEmitter::GetStats() {
  std::map<std::string, uint64_t> stats;
  stats["pending_spans"] = pending_spans_;
  stats["emitted_spans"] = emitted_spans_;
  stats["dropped_spans"] = dropped_spans_;
  stats["batch_export_enabled"] = batch_export_enabled_ ? 1 : 0;
  stats["batch_export_size"] = batch_export_size_;
  return stats;
}

void OTELSpanEmitter::EmitSpan(Span* span) {
  // Add span to buffer for batched export
  pending_span_buffer_.push_back({
      span->span_name_,
      span->trace_id_,
      span->span_id_,
      span->parent_span_id_,
      span->start_time_us_,
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count(),
      span->string_attributes_,
      span->numeric_attributes_,
      span->bool_attributes_
  });

  pending_spans_--;

  // Check if batch export threshold reached
  if (batch_export_enabled_ &&
      pending_span_buffer_.size() >= batch_export_size_) {
    ExportSpans();
  }
}

bool OTELSpanEmitter::ExportSpans() {
  // Serialize buffered spans to OTLP format and export
  // TODO: Implement actual OTLP export logic
  // For now, count spans as emitted and clear buffer
  
  if (pending_span_buffer_.empty()) {
    return true;
  }

  // Production implementation would:
  // 1. Serialize to OTLP protobuf format
  // 2. Send via gRPC to OTLP collector
  // 3. Handle retries on transient failures
  // 4. Track export metrics (success/failure)
  
  // Temporary: Just clear the buffer
  size_t num_spans = pending_span_buffer_.size();
  emitted_spans_ += num_spans;
  pending_span_buffer_.clear();
  
  return true;
}

}  // namespace themis::rag
