/**
 * @file monitoring.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/safety/monitoring.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <utility>

namespace themis::llm::safety {

namespace {

const char* toString(SafetyEventType type) {
    switch (type) {
        case SafetyEventType::ALLOWED: return "allowed";
        case SafetyEventType::REVIEW: return "review";
        case SafetyEventType::BLOCKED: return "blocked";
    }
    return "unknown";
}

std::string jsonEscape(const std::string& value) {
    std::string out = {};
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

} // namespace

bool SafetyMonitoring::setDurableSinkPath(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(sink_mutex_);
    std::filesystem::path sink_path(path);
    if (sink_path.has_parent_path()) {
        std::error_code ec = {};
        std::filesystem::create_directories(sink_path.parent_path(), ec);
        if (ec) {
            return false;
        }
    }

    std::ofstream test(path, std::ios::app);
    if (!test.is_open()) {
        return false;
    }

    durable_sink_path_ = path;
    return true;
}

void SafetyMonitoring::clearDurableSinkPath() {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    durable_sink_path_.reset();
}

void SafetyMonitoring::setExporterSink(ExporterSink sink) {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    exporter_sink_ = std::move(sink);
}

void SafetyMonitoring::clearExporterSink() {
    std::lock_guard<std::mutex> lock(sink_mutex_);
    exporter_sink_ = nullptr;
}

void SafetyMonitoring::record(const SafetyEvent& event) {
    switch (event.type) {
        case SafetyEventType::ALLOWED:
            allowed_.fetch_add(1, std::memory_order_relaxed);
            break;
        case SafetyEventType::REVIEW:
            review_.fetch_add(1, std::memory_order_relaxed);
            break;
        case SafetyEventType::BLOCKED:
            blocked_.fetch_add(1, std::memory_order_relaxed);
            break;
    }

    std::lock_guard<std::mutex> lock(sink_mutex_);

    if (exporter_sink_) {
        exporter_sink_(event);
    }

    if (durable_sink_path_) {
        std::ofstream out(*durable_sink_path_, std::ios::app);
        if (out.is_open()) {
            out << toJsonLine(event) << '\n';
        }
    }
}

SafetyCountersSnapshot SafetyMonitoring::snapshot() const {
    SafetyCountersSnapshot out;
    out.allowed = allowed_.load(std::memory_order_relaxed);
    out.review = review_.load(std::memory_order_relaxed);
    out.blocked = blocked_.load(std::memory_order_relaxed);
    return out;
}

std::string SafetyMonitoring::toJsonLine(const SafetyEvent& event) {
    std::ostringstream oss = {};
    oss << "{"
        << "\"request_id\":\"" << jsonEscape(event.request_id) << "\"," 
        << "\"type\":\"" << toString(event.type) << "\"," 
        << "\"reason\":\"" << jsonEscape(event.reason) << "\"," 
        << "\"confidence\":" << event.confidence << ","
        << "\"unix_ms\":" << event.unix_ms
        << "}";
    return oss.str();
}

} // namespace themis::llm::safety
