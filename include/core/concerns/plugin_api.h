/**
 * @file plugin_api.h
 * @brief ThemisDB adapter plugin ABI contract for runtime dynamic loading.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 *
 * ## Plugin ABI Contract
 *
 * Every shared library that provides a dynamically-loadable ThemisDB adapter
 * **must** export the C-linkage function:
 *
 * ```c
 * int themis_plugin_register(
 *     themis::core::concerns::AdapterRegistry* registry,
 *     const char* requested_adapter_id,
 *     uint32_t    host_api_version);
 * ```
 *
 * @param registry           The AdapterRegistry instance to register into.
 * @param requested_adapter_id The `adapter_id` string passed to
 *                           AdapterRegistry::loadFromPlugin().
 * @param host_api_version   The value of kPluginAbiVersion at load time.
 *                           Plugins should reject if this is incompatible.
 * @return 0 on success; any non-zero value causes loadFromPlugin() to return
 *         false and the library handle to be released.
 *
 * Inside the function the plugin calls `registry->registerAdapter<T>(...)` for
 * each adapter it provides.  Because AdapterRegistry is a fully typed header,
 * plugins compile against the same `adapter_registry.h` header to preserve
 * type safety.
 *
 * ### Minimal plugin example (C++)
 *
 * ```cpp
 * #include "core/concerns/adapter_registry.h"
 * #include "core/concerns/plugin_api.h"
 * #include "my_logger_impl.h"
 *
 * THEMIS_DEFINE_PLUGIN_INIT {
 *     (void)requested_adapter_id;
 *     if (host_api_version < 1) return 1; // incompatible
 *     registry->registerAdapter<ILogger>(
 *         "my_logger", std::make_shared<MyLoggerImpl>());
 *     return 0;
 * }
 * ```
 *
 * ### Security note
 *
 * The host validates file permissions and optionally verifies the plugin
 * library's SHA-256 digest via AdapterSignature before calling
 * `themis_plugin_register`.  Unsigned libraries are accepted only when
 * `AdapterTrustPolicy::kTrustAll` is active (development default).
 */

#pragma once

#include <cstdint>

// Forward-declare AdapterRegistry so the function pointer type compiles
// without pulling in the full header here.
namespace themis {
namespace core {
namespace concerns {
class AdapterRegistry;
} // namespace concerns
} // namespace core
} // namespace themis

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Version constant
// ---------------------------------------------------------------------------

/**
 * @brief ABI version of the ThemisDB plugin interface.
 *
 * Plugins receive this value as `host_api_version` and may reject loads if
 * the version is outside their supported range.  The host sets this constant
 * each time the plugin ABI changes in a breaking way.
 */
static constexpr uint32_t kPluginAbiVersion = 1u;

// ---------------------------------------------------------------------------
// Plugin entry-point symbol name
// ---------------------------------------------------------------------------

/**
 * @brief Name of the C-linkage symbol that plugins must export.
 *
 * AdapterRegistry::loadFromPlugin() looks up exactly this symbol name in the
 * loaded library.
 */
static constexpr const char* kPluginInitSymbol = "themis_plugin_register";

// ---------------------------------------------------------------------------
// Plugin entry-point function type
// ---------------------------------------------------------------------------

/**
 * @brief Function-pointer type matching the plugin's required export.
 *
 * @param registry             Destination registry.
 * @param requested_adapter_id Adapter identifier requested by the caller.
 * @param host_api_version     Host-side ABI version (== kPluginAbiVersion).
 * @return 0 on success; non-zero to signal an error.
 */
using ThemisPluginRegisterFn = int (*)(
    AdapterRegistry* registry,
    const char*      requested_adapter_id,
    uint32_t         host_api_version);

} // namespace concerns
} // namespace core
} // namespace themis

// ---------------------------------------------------------------------------
// Convenience macro for plugin authors
// ---------------------------------------------------------------------------

/**
 * @brief Emit the required extern-"C" entry point for a ThemisDB adapter plugin.
 *
 * Usage — place in exactly one translation unit in the plugin library:
 *
 * ```cpp
 * #include "core/concerns/plugin_api.h"
 *
 * THEMIS_DEFINE_PLUGIN_INIT {
 *     // register adapters here, return 0 on success
 * }
 * ```
 */
#define THEMIS_DEFINE_PLUGIN_INIT                                        \
    extern "C" int themis_plugin_register(                               \
        ::themis::core::concerns::AdapterRegistry* registry,             \
        const char*                               requested_adapter_id,  \
        uint32_t                                  host_api_version)
