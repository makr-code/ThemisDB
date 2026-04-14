/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wasm_runtime_injector.h                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:44:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file wasm_runtime_injector.h
 * @brief Runtime injection interface for pluggable WASM execution backends.
 *
 * The WasmPluginSandbox (wasm_plugin_sandbox.h) is runtime-agnostic by design.
 * This header provides the injection point: a narrow abstract interface
 * (IWasmRuntime) that concrete backends (Wasmtime, WasmEdge, wasm3, etc.)
 * implement, plus a WasmRuntimeInjector registry that maps runtime names to
 * factory functions and selects the best available backend at startup.
 *
 * ## Injection Contract
 * 1. Any translation unit that wishes to register a backend calls
 *    `WasmRuntimeInjector::registerRuntime()` before the first sandbox is
 *    constructed (typically at static-init time via a helper macro).
 * 2. `WasmRuntimeInjector::create()` resolves the requested name (or picks
 *    the highest-priority registered backend when the name is empty) and
 *    returns an owned `IWasmRuntime`.
 * 3. `WasmPluginSandbox` calls `WasmRuntimeInjector::create()` internally
 *    when its Config::runtime_name is set.
 *
 * ## Thread Safety
 * `registerRuntime()` is NOT thread-safe; call it only during static
 * initialisation or before any sandbox is created.
 * `create()` and `available()` are thread-safe once registration is complete.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "themis/base/wasm_plugin_sandbox.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace modules {

// ─────────────────────────────────────────────────────────────────────────────
// IWasmRuntime – abstract WASM execution backend
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for a WASM execution backend.
 *
 * Implementations wrap Wasmtime, WasmEdge, wasm3, or any other WASM engine.
 * They are responsible for:
 *   - Compiling / instantiating a validated .wasm binary.
 *   - Exposing host functions registered via WasmHostFunction.
 *   - Enforcing linear-memory isolation and resource limits.
 *   - Executing named exports and returning their result bytes.
 */
class IWasmRuntime {
public:
    virtual ~IWasmRuntime() = default;

    /**
     * @brief Compile and instantiate a .wasm binary.
     *
     * @param wasm_bytes  Raw .wasm bytes.
     * @param host_fns    Host functions the module may import.
     * @param memory_limit_bytes  Maximum linear-memory bytes (0 = runtime default).
     * @return true on success; false if compilation or instantiation fails.
     */
    virtual bool instantiate(const std::vector<uint8_t>&       wasm_bytes,
                              const std::vector<WasmHostFunction>& host_fns,
                              size_t                             memory_limit_bytes) = 0;

    /**
     * @brief Call a named export function.
     *
     * @param fn_name  Exported function name.
     * @param args     Serialised argument blob (plugin-defined format).
     * @param out      Output blob written by the function.
     * @return true on success; false on trap or if the function is not exported.
     */
    virtual bool call(const std::string&         fn_name,
                      const std::vector<uint8_t>& args,
                      std::vector<uint8_t>&       out) = 0;

    /**
     * @brief Return a pointer to the instance's linear memory and its size.
     *
     * Callers must not hold this pointer across calls to `call()`.
     */
    virtual uint8_t* linearMemory(size_t& out_size) = 0;

    /**
     * @brief Human-readable name of this runtime (e.g. "wasmtime").
     */
    virtual std::string name() const = 0;

    /**
     * @brief Runtime version string.
     */
    virtual std::string version() const = 0;

    /**
     * @brief True if the runtime compiled and instantiated successfully.
     */
    virtual bool isInstantiated() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// WasmRuntimeDescriptor – registration metadata
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Registration record for a WASM runtime backend.
 */
struct WasmRuntimeDescriptor {
    std::string name;          ///< Unique runtime identifier (e.g. "wasmtime")
    int         priority = 0;  ///< Higher value = preferred when auto-selecting
    std::string description;   ///< Human-readable description

    /// Factory function that creates a new runtime instance.
    std::function<std::unique_ptr<IWasmRuntime>()> factory;
};

// ─────────────────────────────────────────────────────────────────────────────
// WasmRuntimeInjector – runtime registry and factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Registry of pluggable WASM runtime backends.
 *
 * Usage — registering a backend at startup:
 * ```cpp
 * // In wasmtime_backend.cpp:
 * THEMIS_REGISTER_WASM_RUNTIME(
 *     "wasmtime", 100, "Bytecode Alliance Wasmtime",
 *     []() -> std::unique_ptr<IWasmRuntime> {
 *         return std::make_unique<WasmtimeRuntime>();
 *     });
 * ```
 *
 * Usage — creating a runtime in WasmPluginSandbox:
 * ```cpp
 * auto rt = WasmRuntimeInjector::create("wasmtime");
 * if (!rt) rt = WasmRuntimeInjector::create(); // auto-select
 * rt->instantiate(bytes, host_fns, 64 * 1024 * 1024);
 * ```
 */
class WasmRuntimeInjector {
public:
    /**
     * @brief Register a WASM runtime backend.
     *
     * Must be called before any sandbox is created.
     * Duplicate names replace the previous registration.
     */
    static void registerRuntime(WasmRuntimeDescriptor desc);

    /**
     * @brief Create a runtime instance by name.
     *
     * @param runtime_name  Empty string = select highest-priority registered backend.
     * @return Owning pointer to a new (un-instantiated) runtime, or nullptr if not found.
     */
    static std::unique_ptr<IWasmRuntime> create(const std::string& runtime_name = {});

    /**
     * @brief True if at least one backend is registered.
     */
    static bool available() noexcept;

    /**
     * @brief List registered runtime names, sorted by descending priority.
     */
    static std::vector<std::string> registeredNames();

    /**
     * @brief Unregister all backends (useful for testing).
     */
    static void clearAll();
};

// ─────────────────────────────────────────────────────────────────────────────
// Registration helper macro
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Register a WASM runtime backend at static-init time.
 *
 * Example:
 * ```cpp
 * THEMIS_REGISTER_WASM_RUNTIME("wasm3", 50, "M3 interpreter",
 *     []{ return std::make_unique<Wasm3Runtime>(); });
 * ```
 */
#define THEMIS_REGISTER_WASM_RUNTIME(name_, priority_, desc_, factory_)       \
    namespace {                                                                \
    struct _WasmRtReg_##name_ {                                                \
        _WasmRtReg_##name_() {                                                 \
            ::themis::modules::WasmRuntimeInjector::registerRuntime({          \
                name_, priority_, desc_, factory_                               \
            });                                                                \
        }                                                                      \
    } _wasm_rt_reg_instance_##name_;                                           \
    }

} // namespace modules
} // namespace themis
