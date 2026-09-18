/**
 * @file themis_tool_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Plugin Interface**: Abstract interface for plugin system.
 *       No .cpp implementation needed. Implementations provided by plugins.
 */


#pragma once

#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <future>

namespace themis::llm {

using json = nlohmann::json;

class IThemisTool : public plugins::IThemisPlugin {
public:
    ~IThemisTool() override = default;

    // ── IThemisPlugin overrides ──────────────────────────────────────────────

    plugins::PluginType getType() const final {
        return plugins::PluginType::AGENTIC_TOOL;
    }

    /**
     * @brief ── Tool-specific API ────────────────────────────────────────────────────
     * @return Return value.
     */

    virtual json inputSchema() const = 0;

    /**
     * @brief Output Schema.
     * @return Return value.
     */
    virtual json outputSchema() const = 0;

    /**
     * @brief Execute.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    virtual json execute(const json& input) = 0;

    /**
     * @brief Execute Async.
     * @param[in] input Input parameter.
     * @return Return value.
     * @details Calls: std::async(), execute().
     */
    virtual std::future<json> executeAsync(const json& input) {
        return std::async(std::launch::async,
                          [this, input]() mutable { return execute(input); });
    }
};

} // namespace themis::llm

/**
 * @brief Convenience macro for tool DLL entry points.
 *
 * Usage:
 * @code
 * class MySearchTool : public themis::llm::IThemisTool { ... };
 * THEMIS_TOOL_IMPL(MySearchTool)
 * @endcode
 */
#define THEMIS_TOOL_IMPL(ToolClass) \
    extern "C" themis::plugins::IThemisPlugin* createPlugin() { \
        return new ToolClass(); \
    } \
    extern "C" void destroyPlugin(themis::plugins::IThemisPlugin* p) { \
        delete p; \
    }
