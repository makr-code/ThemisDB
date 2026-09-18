/**
 * @file ingestion_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

// Forward declarations
struct IngestionJob;
enum class IngestionJobType;

class IngestionPlugin {
public:
    /**
     * @brief Ingestion Plugin.
     * @return Return value.
     */
    virtual ~IngestionPlugin() = default;
    
    [[nodiscard]] virtual std::string name() const = 0;
    
    [[nodiscard]] virtual std::string version() const = 0;
    
    [[nodiscard]] virtual std::vector<IngestionJobType> supportedTypes() const = 0;
    
    /**
     * @brief Process Job.
     * @param[in,out] job Input/output parameter.
     */
    virtual void processJob(IngestionJob& job) = 0;
    
    [[nodiscard]] virtual size_t estimateJobSize(const IngestionJob& job) = 0;
    
    [[nodiscard]] virtual json getConfig() const = 0;
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    virtual void setConfig(const json& config) = 0;
};

struct IngestionSource {
    std::string source_id;          ///< Unique identifier
    std::string plugin_name;        ///< Which plugin handles this
    IngestionJobType type;          ///< Job type
    std::string location;           ///< URL, path, connection string
    json config;                    ///< Plugin-specific configuration
    int priority = 0;               ///< Higher = preferred in conflicts
    std::vector<std::string> tags;  ///< Classification tags
    bool incremental = true;        ///< Only fetch new data
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static IngestionSource fromJson(const json& j);
};

} // namespace content
} // namespace themis
