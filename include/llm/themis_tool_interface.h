/**
 * @file themis_tool_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Plugin Interface**: Abstract interface for plugin system.
 *       No .cpp implementation needed. Implementations provided by plugins.
 */

/*
 * ThemisDB | File: themis_tool_interface.h | Version: 0.0.2 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 125
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <future>

namespace themis::llm {

using json = nlohmann::json;

/**
 * @brief Interface for agentic tool plugins loaded dynamically via ToolRegistry.
 *
 * IThemisTool extends IThemisPlugin so that every tool is a first-class plugin
 * managed by PluginManager (dlopen/dlclose lifecycle, hot-reload, OCI
 * distribution, Ed25519 signing, capability escalation checks).
 *
 * ## DLL contract
 * Every tool shared library must export:
 * @code
 * extern "C" {
 *     THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin();
 *     THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin*);
 * }
 * @endcode
 * Use the THEMIS_TOOL_IMPL(ClassName) convenience macro (defined below).
 *
 * ## plugin.json type field
 * @code
 * { "type": "agentic_tool", ... }
 * @endcode
 *
 * Thread-Safety: execute() and executeAsync() MUST be thread-safe.
 */
class IThemisTool : public plugins::IThemisPlugin {
public:
    ~IThemisTool() override = default;

    // ── IThemisPlugin overrides ──────────────────────────────────────────────

    /// Returns PluginType::AGENTIC_TOOL.  Concrete tools must NOT override this.
    plugins::PluginType getType() const final {
        return plugins::PluginType::AGENTIC_TOOL;
    }

    // ── Tool-specific API ────────────────────────────────────────────────────

    /**
     * @brief JSON Schema (draft-07) describing the accepted input object.
     *
     * ToolRegistry validates the input against this schema before calling
     * execute(). Return an empty object {} to skip validation.
     *
     * @return JSON Schema object.
     */
    virtual json inputSchema() const = 0;

    /**
     * @brief JSON Schema (draft-07) describing the output object.
     *
     * Informational only; used for documentation and orchestrator planning.
     *
     * @return JSON Schema object.
     */
    virtual json outputSchema() const = 0;

    /**
     * @brief Execute the tool synchronously.
     *
     * @param input  Parsed JSON arguments (pre-validated against inputSchema()).
     * @return JSON result to inject into the prompt context.
     *
     * Error convention: on failure, return
     * @code
     * {"error": "<human-readable message>"}
     * @endcode
     * Throwing an exception is also acceptable; ToolRegistry catches it and
     * wraps it in the same error JSON format.
     */
    virtual json execute(const json& input) = 0;

    /**
     * @brief Execute the tool asynchronously.
     *
     * Default implementation launches execute() on a new thread via
     * std::async.  Override for custom thread-pool dispatch.
     *
     * @param input  Parsed JSON arguments.
     * @return Future resolving to the tool result.
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
    extern "C" { \
        THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() { \
            return new ToolClass(); \
        } \
        THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* p) { \
            delete p; \
        } \
    }
