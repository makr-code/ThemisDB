/**
 * @file export_format_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "exporters/exporter_interface.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::exporters {

class ExportFormatRegistry {
public:
    using Factory = std::function<std::unique_ptr<IExporter>()>;

    /**
     * @brief Instance.
     * @return Return value.
     */
    static ExportFormatRegistry& instance();

    /**
     * @brief Register Format.
     * @param[in] format_key Input parameter.
     * @param[in] factory Input parameter.
     */
    void registerFormat(const std::string& format_key, Factory factory);

    /**
     * @brief Create Exporter.
     * @param[in] format_key Input parameter.
     * @return Return value.
     */
    std::unique_ptr<IExporter> createExporter(const std::string& format_key) const;

    /**
     * @brief Has Format.
     * @param[in] format_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasFormat(const std::string& format_key) const;

    /**
     * @brief Registered Formats.
     * @return Return value.
     */
    std::vector<std::string> registeredFormats() const;

    /**
     * @brief Register Builtins.
     */
    void registerBuiltins();

    /**
     * @brief Load Templates From Config.
     * @param[in] config_path Path to the retention policy configuration file.
     */
    void loadTemplatesFromConfig(const std::string& config_path);

    /**
     * @brief Load Templates From Json.
     * @param[in] json_str Input parameter.
     */
    void loadTemplatesFromJson(const std::string& json_str);

    /**
     * @brief Clear.
     */
    void clear();

private:
    ExportFormatRegistry() = default;
    ~ExportFormatRegistry() = default;
    ExportFormatRegistry(const ExportFormatRegistry&) = delete;
    ExportFormatRegistry& operator=(const ExportFormatRegistry&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Factory> formats_;
};

} // namespace themis::exporters
