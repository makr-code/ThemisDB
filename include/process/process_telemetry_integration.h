/**
 * @file process_telemetry_integration.h
 * @brief OpenTelemetry integration for federated consensus tracing.
 * @version 2.1.0
 * @date 2026-08-06
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_INTEGRATION_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_INTEGRATION_H

#include <string>
#include <cstdint>
#include <memory>
#include <map>

namespace themis {
namespace process {

// Forward declarations
class ProcessTelemetryIntegrationImpl;
class DistributedSpan;

/**
 * @brief Configuration for telemetry integration.
 */
struct TelemetryConfig {
  std::string otel_exporter_endpoint = "http://localhost:4317";
  double sampling_rate = 1.0;  // 0.0 - 1.0
  uint64_t batch_size = 100;
  uint32_t export_interval_ms = 5000;
};

/**
 * @brief W3C Trace Context for distributed tracing.
 */
struct TraceContext {
  std::string trace_id;
  std::string span_id;
  std::string parent_span_id;
  uint8_t trace_flags = 0x01;
};

/**
 * @brief Telemetry statistics.
 */
struct TelemetryStats {
  uint64_t spans_created = 0;
  uint64_t spans_exported = 0;
  uint64_t avg_overhead_ms = 0;
  uint64_t max_overhead_ms = 0;
  uint64_t budget_violations = 0;
};

/**
 * @class ProcessTelemetryIntegration
 * @brief Distributed tracing and telemetry coordination engine.
 *
 * Provides distributed tracing via OpenTelemetry spans, correlation ID
 * propagation across federation boundaries, W3C Trace Context compliance,
 * and overhead budget enforcement (<5%).
 */
class ProcessTelemetryIntegration {
 public:
  /**
   * @brief Factory method to create telemetry integration.
   */
  static std::unique_ptr<ProcessTelemetryIntegration> Create(
      const TelemetryConfig& config,
      const std::string& node_id);

  /**
   * @brief Constructor.
   */
  explicit ProcessTelemetryIntegration(
      std::unique_ptr<ProcessTelemetryIntegrationImpl> impl);

  /**
   * @brief Destructor.
   */
  ~ProcessTelemetryIntegration();

  /**
   * @brief Create a new distributed trace span.
   */
  std::shared_ptr<DistributedSpan> CreateSpan(const std::string& operation_name);

  /**
   * @brief Set current trace context (for parent span linking).
   */
  void SetCurrentTraceContext(const TraceContext& context);

  /**
   * @brief Get current trace context (for header propagation).
   */
  TraceContext GetCurrentTraceContext() const;

  /**
   * @brief Record span in history and export to OTel collector.
   */
  void RecordSpan(const std::shared_ptr<DistributedSpan>& span);

  /**
   * @brief Validate tracing overhead budget (< 5% of operation latency).
   */
  bool ValidateOverheadBudget(const std::string& operation_name,
                              uint64_t operation_latency_ms,
                              uint64_t tracing_overhead_ms);

  /**
   * @brief Get telemetry statistics.
   */
  TelemetryStats GetStats() const;

  /**
   * @brief Export all recorded spans to OpenTelemetry collector.
   */
  bool ExportSpans();

 private:
  std::unique_ptr<ProcessTelemetryIntegrationImpl> impl_;
};

}  // namespace process
}  // namespace themis

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_INTEGRATION_H
