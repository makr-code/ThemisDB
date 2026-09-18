/**
 * @file monitoring.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

class SafetyMonitoring {
public:
    using ExporterSink = std::function<void(const SafetyEvent&)>;

    /**
     * @brief Set Durable Sink Path.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool setDurableSinkPath(const std::string& path);
    /**
     * @brief Clear Durable Sink Path.
     */
    void clearDurableSinkPath();

    /**
     * @brief Set Exporter Sink.
     * @param[in] sink Input parameter.
     */
    void setExporterSink(ExporterSink sink);
    /**
     * @brief Clear Exporter Sink.
     */
    void clearExporterSink();

    /**
     * @brief Record.
     * @param[in] event Input parameter.
     */
    void record(const SafetyEvent& event);
    /**
     * @brief Snapshot.
     * @return Return value.
     */
    SafetyCountersSnapshot snapshot() const;

private:
    /**
     * @brief To Json Line.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    static std::string toJsonLine(const SafetyEvent& event);

    std::atomic<std::uint64_t> allowed_{0};
    std::atomic<std::uint64_t> review_{0};
    std::atomic<std::uint64_t> blocked_{0};

    std::optional<std::string> durable_sink_path_;
    ExporterSink exporter_sink_;

    mutable std::mutex sink_mutex_;
};

} // namespace themis::llm::safety
