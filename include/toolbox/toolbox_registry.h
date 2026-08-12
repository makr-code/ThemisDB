/**
 * @file toolbox_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// toolbox_registry.h — Process-global IngestionToolbox registry
//
// Provides a controlled global registration point so that any ThemisDB module
// can access a pre-configured IngestionToolbox without explicit dependency
// injection.  The registry is NOT a raw singleton: it has explicit lifecycle
// management (initialize / reset) and preserves testability.
//
// ## Dual access pattern
//
// 1. **Global (production, server bootstrap)**
//    ```cpp
//    // bootstrap
//    themis::toolbox::initializeToolbox(
//        themis::toolbox::ToolboxBuilder()
//            .withTextBackend(llm_backend)
//            .build());
//
//    // anywhere in the process
//    auto entities = themis::toolbox::extractEntities(text);
//    auto metrics  = themis::toolbox::getMetricsText();
//    ```
//
// 2. **Injected (tests, isolated subsystems)**
//    ```cpp
//    auto toolbox = std::make_shared<IngestionToolbox>();
//    toolbox->setTextBackend(mock_backend);
//    auto entities = toolbox->extractEntities(text);
//    ```
//
// Both patterns coexist; the injected pattern is unaffected by this header.
//
// ## Thread-safety
// `initialize()` and `reset()` must be called from a single thread (during
// startup / teardown, or between test cases).  `instance()` and the free
// functions are fully thread-safe.

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/base_entity.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxRegistry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Process-global registry for the shared `IngestionToolbox` instance.
 *
 * ## Lifecycle
 *
 * 1. **Bootstrap** (server `main()` or test `SetUpTestSuite()`):
 *    ```cpp
 *    ToolboxRegistry::initialize(
 *        ToolboxBuilder()
 *            .withTextBackend(llm)
 *            .build());
 *    ```
 * 2. **Use** (anywhere, any thread):
 *    ```cpp
 *    auto t = ToolboxRegistry::instance(); // shared_ptr<IngestionToolbox>
 *    ```
 * 3. **Teardown** (test suites or graceful shutdown):
 *    ```cpp
 *    ToolboxRegistry::reset();
 *    ```
 *
 * ## Design notes
 *
 * - No lazy construction.  `instance()` throws `std::logic_error` if the
 *   registry has not been initialised.  This prevents subtle "first-call
 *   initialises with defaults" bugs.
 * - `reset()` exists exclusively for test isolation.  Do not call it in
 *   production code.
 * - The underlying `IngestionToolbox` remains fully accessible via the
 *   injected pattern; registering a global instance is opt-in.
 */
class ToolboxRegistry {
public:
    // Non-constructible; use the static API only.
    ToolboxRegistry() = delete;

    /**
     * @brief Register a process-global `IngestionToolbox` instance.
     *
     * Must be called exactly once before any `instance()` call.  Calling
     * `initialize()` a second time replaces the existing instance (last-write
     * wins); this is intentional to support live reconfiguration of the
     * default toolbox (e.g. hot-reload of the LLM backend).
     *
     * @param toolbox  Pre-configured toolbox.  Must not be null.
     * @throws std::invalid_argument when @p toolbox is null.
     */
    static void initialize(std::shared_ptr<IngestionToolbox> toolbox);

    /**
     * @brief Access the registered global `IngestionToolbox`.
     *
     * @return Shared pointer to the active toolbox.
     * @throws std::logic_error when `initialize()` has not been called.
     */
    static std::shared_ptr<IngestionToolbox> instance();

    /**
     * @brief Return `true` when a toolbox has been registered.
     *
     * Use this guard in server health-check paths that must verify that the
     * toolbox has been fully configured before serving requests.
     */
    static bool isInitialized() noexcept;

    /**
     * @brief Clear the registered instance.
     *
     * Intended for **test isolation only**.  After `reset()` any call to
     * `instance()` will throw `std::logic_error` until `initialize()` is
     * called again.
     *
     * @warning Not safe to call while any thread is executing a toolbox
     *          operation.  Always call from a single-threaded context
     *          (e.g. `TearDownTestSuite()`).
     */
    static void reset() noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free functions — generic toolbox access for all ThemisDB modules
// ─────────────────────────────────────────────────────────────────────────────
//
// These wrappers delegate to the global `ToolboxRegistry::instance()`.
// All functions throw `std::logic_error` if the registry is not initialised.
//
// Modules that use the injected pattern should call methods on their own
// `IngestionToolbox` instance rather than using these free functions.

/**
 * @brief Register the process-global toolbox.  Convenience alias for
 *        `ToolboxRegistry::initialize(toolbox)`.
 */
void initializeToolbox(std::shared_ptr<IngestionToolbox> toolbox);

/**
 * @brief Return the process-global toolbox.  Convenience alias for
 *        `ToolboxRegistry::instance()`.
 */
std::shared_ptr<IngestionToolbox> globalToolbox();

/**
 * @brief Extract entities from text using the global `IngestionToolbox`.
 *
 * Delegates to `ToolboxRegistry::instance()->extractEntities(text, mime, filename)`.
 *
 * @param text      UTF-8 text to process.
 * @param mime      MIME type hint (default: "text/plain").
 * @param filename  Filename hint (default: "input.txt").
 * @return Extracted entity nodes; empty on failure.
 * @throws std::logic_error when the registry is not initialised.
 */
std::vector<ingestion::BaseEntity> extractEntities(
    const std::string& text,
    const std::string& mime     = "text/plain",
    const std::string& filename = "input.txt");

/**
 * @brief Extract the full `BaseEntitySet` from text using the global toolbox.
 *
 * Delegates to `ToolboxRegistry::instance()->extractEntitySet(text, mime, filename)`.
 *
 * @param text      UTF-8 text to process.
 * @param mime      MIME type hint (default: "text/plain").
 * @param filename  Filename hint (default: "input.txt").
 * @return Full entity set including nodes, edges, and vector chunks.
 * @throws std::logic_error when the registry is not initialised.
 */
ingestion::BaseEntitySet extractEntitySet(
    const std::string& text,
    const std::string& mime     = "text/plain",
    const std::string& filename = "input.txt");

/**
 * @brief Produce Prometheus text-format metrics for the global toolbox.
 *
 * Delegates to `ToolboxRegistry::instance()->getMetricsText()`.
 *
 * @return Prometheus text payload, or "" if no calls have been recorded.
 * @throws std::logic_error when the registry is not initialised.
 */
std::string getMetricsText();

} // namespace toolbox
} // namespace themis
