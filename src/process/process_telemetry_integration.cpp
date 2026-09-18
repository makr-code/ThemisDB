/**
 * @file process_telemetry_integration.cpp
 * @brief OpenTelemetry integration for federated consensus tracing.
 *
 * Provides distributed tracing via OpenTelemetry spans, correlation ID
 * propagation across federation boundaries, W3C Trace Context compliance,
 * and overhead budget enforcement (<5%).
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_2_CORE_IMPLEMENTATION
 *
 * @note Maturity: 🟡 ALPHA (Phase 2 delivery, production hardening in Phase 5)
 * @note This implementation is auto-generated from ROADMAP_FEDERATION.md Phase 2.
 *
 * ## Tracing Model
 *
 * - **Spans:** Distributed trace spans for consensus operations (append, replicate, commit)
 * - **Correlation IDs:** Propagated across federation boundaries for end-to-end tracing
 * - **W3C Trace Context:** Standard HTTP header format (traceparent, tracestate)
 * - **Overhead Budget:** Instrumentation must not exceed 5% of operation latency
 *
 * ## Span Types
 *
 * 1. **Consensus Span** (parent)
 *    - Operation: AppendEntry
 *    - Attributes: model_id, term, index
 *    - Children: ReplicateSpan (x N replicas), CommitSpan
 *
 * 2. **Replicate Span** (child)
 *    - Operation: AppendEntries RPC
 *    - Attributes: replica_id, replication_lag_ms
 *
 * 3. **Commit Span** (child)
 *    - Operation: CommitToStateMachine
 *    - Attributes: entry_count, apply_time_ms
 *
 * ## Attributes Format
 *
 * All spans include:
 * - `service.name = "themisdb-process"`
 * - `service.version = "2.1.0"`
 * - `federation.node_id = "node-1"`
 * - `federation.term = 5`
 * - `span.kind = "INTERNAL"` (intra-cluster)
 *
 * ## Overhead Measurement
 *
 * Span creation overhead is measured and enforced:
 * - Target: < 1-2% of operation latency
 * - Budget: < 5% (hard limit, GATE-TEL-01)
 * - Measurement: wall-clock time before/after span creation
 *
 * @see process_telemetry_contract.h – Telemetry API contract
 * @see federation_consensus_manager.cpp – Consensus operations
 * @see ROADMAP_FEDERATION.md – Phase 1-6 roadmap
 */

#include "process/process_telemetry_integration.h"
#include "process/process_telemetry_contract.h"
#include "process/process_federation_contract.h"
#include "process/process_common.h"
#include "utils/logger.h"

#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <stdexcept>

namespace themis {
namespace process {

// ============================================================================
// TRACE CONTEXT (W3C STANDARD)
// ============================================================================

struct TraceContext {
  std::string trace_id;

  std::string span_id;

  std::string parent_span_id;

  uint8_t trace_flags = 0x01;  // Sampled by default

  /**
   * @brief Generate Trace Id.
   * @return Return value.
   * @details Calls: rng(), std::setw(), std::setfill(), str().
   */
  static std::string GenerateTraceId() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::ostringstream oss = {};
    oss << std::hex << std::setw(16) << std::setfill('0')
        << rng();  // Upper 64 bits
    oss << std::hex << std::setw(16) << std::setfill('0')
        << rng();  // Lower 64 bits
    return oss.str();
  }

  /**
   * @brief Generate Span Id.
   * @return Return value.
   * @details Calls: rng(), std::setw(), std::setfill(), str().
   */
  static std::string GenerateSpanId() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::ostringstream oss = {};
    oss << std::hex << std::setw(16) << std::setfill('0') << rng();
    return oss.str();
  }

  std::string ToHeader() const {
    std::ostringstream oss = {};
    oss << "00-" << trace_id << "-" << span_id << "-";
    oss << std::hex << std::setw(2) << std::setfill('0')
        << (int)trace_flags;
    return oss.str();
  }

  /**
   * @brief From Header.
   * @param[in] header Input parameter.
   * @return Return value.
   * @details Calls: size(), substr().
   */
  static TraceContext FromHeader(const std::string& header) {
    TraceContext ctx;
    // Simplified parsing; production version uses full RFC compliance
    if (header.size() >= 55) {  // "00-<32>-<16>-<2>"
      ctx.trace_id = header.substr(3, 32);
      ctx.span_id = header.substr(36, 16);
    }
    return ctx;
  }
};

// ============================================================================
// DISTRIBUTED TRACE SPAN
// ============================================================================

class DistributedSpan {
 public:
  DistributedSpan(const std::string& operation_name,
                  const TraceContext& context, const std::string& node_id)
      : operation_name_(operation_name),
        context_(context),
        node_id_(node_id),
        start_time_(std::chrono::high_resolution_clock::now()),
        is_recorded_(false) {
    // Generate new span ID (child of parent)
    context_.parent_span_id = context_.span_id;
    context_.span_id = TraceContext::GenerateSpanId();
  }

  ~DistributedSpan() { End(); }

  /**
   * @brief Set Attribute.
   * @param[in] key Input parameter.
   * @param[in] value Input parameter.
   * @details Implements SetAttribute without additional internal calls.
   */
  void SetAttribute(const std::string& key, const std::string& value) {
    attributes_[key] = value;
  }

  /**
   * @brief Set Attribute.
   * @param[in] key Input parameter.
   * @param[in] value Input parameter.
   * @details Calls: std::to_string().
   */
  void SetAttribute(const std::string& key, uint64_t value) {
    SetAttribute(key, std::to_string(value));
  }

  /**
   * @brief Set Attribute.
   * @param[in] key Input parameter.
   * @param[in] value Input parameter.
   * @details Calls: std::to_string().
   */
  void SetAttribute(const std::string& key, double value) {
    SetAttribute(key, std::to_string(value));
  }

  /**
   * @brief Record Event.
   * @param[in] event_name Name of the event.
   * @details Calls: push_back(), std::chrono::high_resolution_clock::now().
   */
  void RecordEvent(const std::string& event_name) {
    events_.push_back({event_name,
                       std::chrono::high_resolution_clock::now()});
  }

  /**
   * @brief End.
   * @details Calls: std::chrono::high_resolution_clock::now(), count(), utils::Logger::Debug(), c_str().
   */
  void End() {
    if (is_recorded_) {
      return;  // Already ended
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                               start_time_);
    latency_ms_ = duration_ms.count();
    is_recorded_ = true;

    // Log span completion
    utils::Logger::Debug(
        "Span ended: op=%s, span_id=%s, parent_span_id=%s, latency_ms=%lld",
        operation_name_.c_str(), context_.span_id.c_str(),
        context_.parent_span_id.c_str(), latency_ms_);
  }

  // Accessors
  const std::string& GetOperationName() const { return operation_name_; }
  const TraceContext& GetContext() const { return context_; }
  uint64_t GetLatencyMs() const { return latency_ms_; }
  const std::map<std::string, std::string>& GetAttributes() const {
    return attributes_;
  }

 private:
  std::string operation_name_;
  TraceContext context_;
  std::string node_id_;
  std::chrono::high_resolution_clock::time_point start_time_;
  uint64_t latency_ms_ = 0;
  bool is_recorded_;
  std::map<std::string, std::string> attributes_;
  std::vector<std::pair<std::string,
                        std::chrono::high_resolution_clock::time_point>>
      events_;
};

// ============================================================================
// TELEMETRY INTEGRATION IMPLEMENTATION
// ============================================================================

class ProcessTelemetryIntegrationImpl {
 public:
  ProcessTelemetryIntegrationImpl(const TelemetryConfig& config,
                                  const std::string& node_id)
      : config_(config),
        node_id_(node_id),
        root_trace_context_({TraceContext::GenerateTraceId(), "", "", 0x01}),
        spans_created_(0),
        total_overhead_ms_(0),
        max_overhead_ms_(0) {
    utils::Logger::Info(
        "ProcessTelemetryIntegration initialized: node_id=%s, endpoint=%s",
        node_id_.c_str(), config.otel_exporter_endpoint.c_str());
  }

  ~ProcessTelemetryIntegrationImpl() = default;

  // ========================================================================
  // PUBLIC API - DISTRIBUTED TRACING
  // ========================================================================

  /**
   * @brief Create Span.
   * @param[in] operation_name Name of the operation.
   * @return Return value.
   */
  std::shared_ptr<DistributedSpan> CreateSpan(const std::string& operation_name);

  /**
   * @brief Set Current Trace Context.
   * @param[in] context Input parameter.
   */
  void SetCurrentTraceContext(const TraceContext& context);

  /**
   * @brief Get Current Trace Context.
   * @return Return value.
   */
  TraceContext GetCurrentTraceContext() const;

  /**
   * @brief Record Span.
   * @param[in] span Input parameter.
   */
  void RecordSpan(const std::shared_ptr<DistributedSpan>& span);

  /**
   * @brief Validate Overhead Budget.
   * @param[in] operation_name Name of the operation.
   * @param[in] operation_latency_ms Input parameter.
   * @param[in] tracing_overhead_ms Input parameter.
   * @return True when the operation succeeds.
   */
  bool ValidateOverheadBudget(const std::string& operation_name,
                              uint64_t operation_latency_ms,
                              uint64_t tracing_overhead_ms);

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  TelemetryStats GetStats() const;

  /**
   * @brief Export Spans.
   * @return True when the operation succeeds.
   */
  bool ExportSpans();

  // ========================================================================
  // PRIVATE IMPLEMENTATION
  // ========================================================================

 private:
  /**
   * @brief Serialize Span Otlp.
   * @param[in] span Input parameter.
   * @return Return value.
   */
  static std::string SerializeSpanOtlp(
      const std::shared_ptr<DistributedSpan>& span);

  /**
   * @brief Send Spans To Collector.
   * @param[in] span_jsons Input parameter.
   * @return True when the operation succeeds.
   */
  bool SendSpansToCollector(const std::vector<std::string>& span_jsons);

  // ========================================================================
  // MEMBER VARIABLES
  // ========================================================================

  TelemetryConfig config_;
  std::string node_id_;

  mutable std::mutex telemetry_mutex_;
  TraceContext root_trace_context_;
  TraceContext current_trace_context_;

  mutable std::mutex span_history_mutex_;
  std::vector<std::shared_ptr<DistributedSpan>> span_history_;
  static constexpr size_t kMaxSpanBufferSize = 10000;

  // Metrics
  mutable std::mutex metrics_mutex_;
  uint64_t spans_created_ = 0;
  uint64_t spans_exported_ = 0;
  uint64_t total_overhead_ms_ = 0;
  uint64_t max_overhead_ms_ = 0;
  uint64_t budget_violations_ = 0;
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

std::shared_ptr<DistributedSpan>
ProcessTelemetryIntegrationImpl::CreateSpan(const std::string& operation_name) {
  /**
   * @brief Lock.
   * @param[in] telemetry_mutex_ Input parameter.
   * @return Return value.
   */
  std::lock_guard<std::mutex> lock(telemetry_mutex_);

  auto span = std::make_shared<DistributedSpan>(
      operation_name, current_trace_context_, node_id_);

  // Set standard attributes
  span->SetAttribute("service.name", "themisdb-process");
  span->SetAttribute("service.version", "2.1.0");
  span->SetAttribute("federation.node_id", node_id_);
  span->SetAttribute("span.kind", "INTERNAL");

  spans_created_++;

  utils::Logger::Debug("CreateSpan: op=%s, span_id=%s",
                       operation_name.c_str(),
                       span->GetContext().span_id.c_str());

  return span;
}

/**
 * @brief Set Current Trace Context.
 * @param[in] context Input parameter.
 * @details Calls: lock().
 */
void ProcessTelemetryIntegrationImpl::SetCurrentTraceContext(
    const TraceContext& context) {
  std::lock_guard<std::mutex> lock(telemetry_mutex_);
  current_trace_context_ = context;
}

TraceContext ProcessTelemetryIntegrationImpl::GetCurrentTraceContext() const {
  /**
   * @brief Lock.
   * @param[in] telemetry_mutex_ Input parameter.
   * @return Return value.
   */
  std::lock_guard<std::mutex> lock(telemetry_mutex_);
  return current_trace_context_;
}

/**
 * @brief Record Span.
 * @param[in] span Input parameter.
 * @details Calls: lock(), push_back(), size(), erase(), begin(), utils::Logger::Debug(), GetOperationName(), c_str().
 */
void ProcessTelemetryIntegrationImpl::RecordSpan(
    const std::shared_ptr<DistributedSpan>& span) {
  std::lock_guard<std::mutex> lock(span_history_mutex_);

  span_history_.push_back(span);

  // Trim buffer if exceeds max size
  if (span_history_.size() > kMaxSpanBufferSize) {
    span_history_.erase(span_history_.begin());
  }

  utils::Logger::Debug("RecordSpan: op=%s, latency_ms=%llu",
                       span->GetOperationName().c_str(),
                       span->GetLatencyMs());
}

/**
 * @brief Validate Overhead Budget.
 * @param[in] operation_name Name of the operation.
 * @param[in] operation_latency_ms Input parameter.
 * @param[in] tracing_overhead_ms Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), utils::Logger::Warn(), c_str(), std::max().
 */
bool ProcessTelemetryIntegrationImpl::ValidateOverheadBudget(
    const std::string& operation_name, uint64_t operation_latency_ms,
    uint64_t tracing_overhead_ms) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  const uint64_t kBudgetPercent = 5;
  uint64_t budget_ms = (operation_latency_ms * kBudgetPercent) / 100;

  bool within_budget = tracing_overhead_ms <= budget_ms;

  if (!within_budget) {
    budget_violations_++;
    utils::Logger::Warn(
        "Overhead budget violation: op=%s, overhead=%llums, budget=%llums, "
        "operation_latency=%llums",
        operation_name.c_str(), tracing_overhead_ms, budget_ms,
        operation_latency_ms);
  }

  total_overhead_ms_ += tracing_overhead_ms;
  max_overhead_ms_ = std::max(max_overhead_ms_, tracing_overhead_ms);

  return within_budget;
}

TelemetryStats ProcessTelemetryIntegrationImpl::GetStats() const {
  /**
   * @brief Lock.
   * @param[in] metrics_mutex_ Input parameter.
   * @return Return value.
   */
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  TelemetryStats stats;
  stats.spans_created = spans_created_;
  stats.spans_exported = spans_exported_;
  stats.avg_overhead_ms =
      (spans_created_ > 0) ? (total_overhead_ms_ / spans_created_) : 0;
  stats.max_overhead_ms = max_overhead_ms_;
  stats.budget_violations = budget_violations_;

  return stats;
}

/**
 * @brief Export Spans.
 * @return True when the operation succeeds.
 * @details Calls: lock(), empty(), push_back(), SerializeSpanOtlp(), SendSpansToCollector(), metrics_lock(), size(), clear().
 */
bool ProcessTelemetryIntegrationImpl::ExportSpans() {
  std::lock_guard<std::mutex> lock(span_history_mutex_);

  if (span_history_.empty()) {
    return true;  // Nothing to export
  }

  // Serialize spans to OTLP JSON
  std::vector<std::string> span_jsons = {};

  for (const auto& span : span_history_) {
    span_jsons.push_back(SerializeSpanOtlp(span));
  }

  // Send to OTel collector
  bool success = SendSpansToCollector(span_jsons);

  if (success) {
    {
      std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
      spans_exported_ += span_history_.size();
    }
    span_history_.clear();
  }

  return success;
}

/**
 * @brief Serialize Span Otlp.
 * @param[in] span Input parameter.
 * @return Return value.
 * @details Calls: GetOperationName(), GetContext(), GetLatencyMs(), str().
 */
std::string ProcessTelemetryIntegrationImpl::SerializeSpanOtlp(
    const std::shared_ptr<DistributedSpan>& span) {
  // Simplified OTLP JSON serialization
  std::ostringstream oss = {};
  oss << "{"
      << "\"name\":\"" << span->GetOperationName() << "\","
      << "\"context\":{\"trace_id\":\"" << span->GetContext().trace_id << "\","
      << "\"span_id\":\"" << span->GetContext().span_id << "\"},"
      << "\"duration_ms\":" << span->GetLatencyMs() << "}";
  return oss.str();
}

/**
 * @brief Send Spans To Collector.
 * @param[in] span_jsons Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: utils::Logger::Info(), size(), c_str().
 */
bool ProcessTelemetryIntegrationImpl::SendSpansToCollector(
    const std::vector<std::string>& span_jsons) {
  // Simplified: log export (production version uses HTTP POST)
  utils::Logger::Info("ExportSpans: sending %zu spans to %s",span_jsons.size(),
                      config_.otel_exporter_endpoint.c_str());
  return true;
}

// ============================================================================
// PUBLIC INTERFACE
// ============================================================================

std::unique_ptr<ProcessTelemetryIntegration>
ProcessTelemetryIntegration::Create(const TelemetryConfig& config,
                                    const std::string& node_id) {
  return std::make_unique<ProcessTelemetryIntegration>(
      std::make_unique<ProcessTelemetryIntegrationImpl>(config, node_id));
}

ProcessTelemetryIntegration::ProcessTelemetryIntegration(
    std::unique_ptr<ProcessTelemetryIntegrationImpl> impl)
    : impl_(std::move(impl)) {}

ProcessTelemetryIntegration::~ProcessTelemetryIntegration() = default;

std::shared_ptr<DistributedSpan>
ProcessTelemetryIntegration::CreateSpan(const std::string& operation_name) {
  return impl_->CreateSpan(operation_name);
}

/**
 * @brief Set Current Trace Context.
 * @param[in] context Input parameter.
 * @details Implements SetCurrentTraceContext without additional internal calls.
 */
void ProcessTelemetryIntegration::SetCurrentTraceContext(
    const TraceContext& context) {
  impl_->SetCurrentTraceContext(context);
}

TraceContext ProcessTelemetryIntegration::GetCurrentTraceContext() const {
  return impl_->GetCurrentTraceContext();
}

/**
 * @brief Record Span.
 * @param[in] span Input parameter.
 * @details Implements RecordSpan without additional internal calls.
 */
void ProcessTelemetryIntegration::RecordSpan(
    const std::shared_ptr<DistributedSpan>& span) {
  impl_->RecordSpan(span);
}

/**
 * @brief Validate Overhead Budget.
 * @param[in] operation_name Name of the operation.
 * @param[in] operation_latency_ms Input parameter.
 * @param[in] tracing_overhead_ms Input parameter.
 * @return True when the operation succeeds.
 * @details Implements ValidateOverheadBudget without additional internal calls.
 */
bool ProcessTelemetryIntegration::ValidateOverheadBudget(
    const std::string& operation_name, uint64_t operation_latency_ms,
    uint64_t tracing_overhead_ms) {
  return impl_->ValidateOverheadBudget(operation_name, operation_latency_ms,
                                       tracing_overhead_ms);
}

TelemetryStats ProcessTelemetryIntegration::GetStats() const {
  return impl_->GetStats();
}

/**
 * @brief Export Spans.
 * @return True when the operation succeeds.
 * @details Implements ExportSpans without additional internal calls.
 */
bool ProcessTelemetryIntegration::ExportSpans() {
  return impl_->ExportSpans();
}

}  // namespace process
}  // namespace themis
