/*
 * ThemisDB | File: monitoring.h | Version: 0.0.1 | Last Modified: 2026-06-01 11:06:12
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 58
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace themis::llm::safety {

enum class SafetyEventType {
    ALLOWED,
    REVIEW,
    BLOCKED
};

struct SafetyEvent {
    std::string request_id;
    SafetyEventType type = SafetyEventType::ALLOWED;
    std::string reason;
    double confidence = 0.0;
    std::int64_t unix_ms = 0;
};

struct SafetyCountersSnapshot {
    std::uint64_t allowed = 0;
    std::uint64_t review = 0;
    std::uint64_t blocked = 0;
};

class SafetyMonitoring {
public:
    using ExporterSink = std::function<void(const SafetyEvent&)>;

    bool setDurableSinkPath(const std::string& path);
    void clearDurableSinkPath();

    void setExporterSink(ExporterSink sink);
    void clearExporterSink();

    void record(const SafetyEvent& event);
    SafetyCountersSnapshot snapshot() const;

private:
    static std::string toJsonLine(const SafetyEvent& event);

    std::atomic<std::uint64_t> allowed_{0};
    std::atomic<std::uint64_t> review_{0};
    std::atomic<std::uint64_t> blocked_{0};

    std::optional<std::string> durable_sink_path_;
    ExporterSink exporter_sink_;

    mutable std::mutex sink_mutex_;
};

} // namespace themis::llm::safety
