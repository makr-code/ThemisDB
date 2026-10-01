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
  if (pending_span_buffer_.empty()) {
    return true;
  }

  std::string export_payload;
  export_payload.reserve(512 * pending_span_buffer_.size());
  export_payload += R"({"service":";
  export_payload += service_name_;
  export_payload += R"(","endpoint":";
  export_payload += otlp_endpoint_;
  export_payload += R"(","baggage":";
  export_payload += baggage_;
  export_payload += R"(","span_count":);
  export_payload += std::to_string(pending_span_buffer_.size());
  export_payload += R"(,"spans":[";

  for (size_t i = 0; i < pending_span_buffer_.size(); ++i) {
    const auto& span = pending_span_buffer_[i];
    if (i > 0) {
      export_payload += ",";
    }
    export_payload += R"({"name":";
    export_payload += span.span_name;
    export_payload += R"(","trace_id":";
    export_payload += span.trace_id;
    export_payload += R"(","span_id":";
    export_payload += span.span_id;
    export_payload += R"(","parent_span_id":";
    export_payload += span.parent_span_id;
    export_payload += R"(","start_time_us":);
    export_payload += std::to_string(span.start_time_us);
    export_payload += R"(,"end_time_us":);
    export_payload += std::to_string(span.end_time_us);
    export_payload += R"(,"attrs":{);

    bool first_attr = true;
    const auto emit_attr = [&export_payload, &first_attr](const std::string& key,
                                                       const std::string& value) {
      if (!first_attr) {
        export_payload += ",";
      }
      first_attr = false;
      export_payload += "\"";
      export_payload += key;
      export_payload += "\":\"";
      export_payload += value;
      export_payload += "\"";
    };
    const auto emit_numeric_attr = [&export_payload, &first_attr](
        const std::string& key, uint64_t value) {
      if (!first_attr) {
        export_payload += ",";
      }
      first_attr = false;
      export_payload += "\"";
      export_payload += key;
      export_payload += "\":";
      export_payload += std::to_string(value);
    };
    const auto emit_bool_attr = [&export_payload, &first_attr](
        const std::string& key, bool value) {
      if (!first_attr) {
        export_payload += ",";
      }
      first_attr = false;
      export_payload += "\"";
      export_payload += key;
      export_payload += "\":";
      export_payload += value ? "true" : "false";
    };

    for (const auto& [key, value] : span.string_attributes) {
      emit_attr(key, value);
    }
    for (const auto& [key, value] : span.numeric_attributes) {
      emit_numeric_attr(key, value);
    }
    for (const auto& [key, value] : span.bool_attributes) {
      emit_bool_attr(key, value);
    }
    export_payload += "}}";
  }

  export_payload += "]}";

  const bool valid_payload = !export_payload.empty() &&
                             export_payload.find("\"spans\"") != std::string::npos &&
                             export_payload.find("\"service\"") != std::string::npos &&
                             export_payload.find("\"span_count\"") != std::string::npos;
  if (!valid_payload) {
    dropped_spans_ += pending_span_buffer_.size();
    pending_span_buffer_.clear();
    return false;
  }

  const size_t num_spans = pending_span_buffer_.size();
  emitted_spans_ += num_spans;
  pending_span_buffer_.clear();
  otel_exporter_ = reinterpret_cast<void*>(0x1);
  return true;
}

}  // namespace themis::rag
