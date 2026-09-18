/**
 * @file config_metrics_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <functional>
#include <mutex>

namespace prometheus {
class Registry;
}

namespace themis {
namespace config {

class ConfigMetricsExporter {
public:
    using GaugeSinkFn = std::function<void(const std::string& name, double value)>;

    ConfigMetricsExporter() = delete;

    /**
     * @brief Collect.
     * @return Return value.
     */
    static std::string collect();

    /**
     * @brief Update Metrics Collector.
     */
    static void updateMetricsCollector();

    /**
     * @brief Register With Registry.
     * @param[in] registry Input parameter.
     */
    static void registerWithRegistry(const std::shared_ptr<prometheus::Registry>& registry);

    /**
     * @brief Set Gauge Sink Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), gaugeSinkFnMutex(), gaugeSinkFnStorage(), std::move().
     */
    static void setGaugeSinkFn(GaugeSinkFn fn) {
        std::lock_guard<std::mutex> lk(gaugeSinkFnMutex());
        gaugeSinkFnStorage() = std::move(fn);
    }

private:
    /**
     * @brief Gauge Sink Fn Mutex.
     * @return Return value.
     * @details Implements gaugeSinkFnMutex without additional internal calls.
     */
    static std::mutex& gaugeSinkFnMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief Gauge Sink Fn Storage.
     * @return Return value.
     * @details Implements gaugeSinkFnStorage without additional internal calls.
     */
    static GaugeSinkFn& gaugeSinkFnStorage() {
        static GaugeSinkFn fn;
        return fn;
    }
};

} // namespace config
} // namespace themis
