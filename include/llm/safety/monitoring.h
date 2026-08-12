/**
 * @file monitoring.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/** @brief Safety monitoring. */
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
