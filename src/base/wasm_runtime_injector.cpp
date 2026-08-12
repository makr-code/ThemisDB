/**
 * @file wasm_runtime_injector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/base/wasm_runtime_injector.h"

#include <algorithm>
#include <mutex>
#include <vector>

#include "utils/logger.h"

namespace themis {
namespace modules {

// ─────────────────────────────────────────────────────────────────────────────
// Registry (process-global, protected by a mutex for reads after init)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct Registry {
    std::mutex mu;
    std::vector<WasmRuntimeDescriptor> entries;
};

Registry &globalRegistry() {
    static Registry r;
    return r;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// WasmRuntimeInjector implementation
// ─────────────────────────────────────────────────────────────────────────────

void WasmRuntimeInjector::registerRuntime(WasmRuntimeDescriptor desc) {
    auto &reg = globalRegistry();
    std::lock_guard<std::mutex> lock(reg.mu);

    // Replace existing entry with the same name
    for (auto &e : reg.entries) {
        if (e.name == desc.name) {
            THEMIS_INFO("WasmRuntimeInjector: replacing runtime '{}'", desc.name);
            e = std::move(desc);
            return;
        }
    }
    THEMIS_INFO("WasmRuntimeInjector: registered runtime '{}' (priority={})", desc.name, desc.priority);
    reg.entries.push_back(std::move(desc));
}

std::unique_ptr<IWasmRuntime> WasmRuntimeInjector::create(const std::string &runtime_name) {
    auto &reg = globalRegistry();
    std::lock_guard<std::mutex> lock(reg.mu);

    if (reg.entries.empty()) {
        THEMIS_WARN("WasmRuntimeInjector::create: no WASM runtime backends registered");
        return nullptr;
    }

    if (runtime_name.empty()) {
        // Auto-select: highest priority
        const WasmRuntimeDescriptor *best = nullptr;
        for (const auto &e : reg.entries) {
            if (best == nullptr || e.priority > best->priority) {
                best = &e;
            }
        }
        if (best && best->factory) {
            THEMIS_INFO("WasmRuntimeInjector: auto-selected runtime '{}'", best->name);
            return best->factory();
        }
        return nullptr;
    }

    // Named lookup
    for (const auto &e : reg.entries) {
        if (e.name == runtime_name && e.factory) {
            return e.factory();
        }
    }

    THEMIS_WARN("WasmRuntimeInjector::create: runtime '{}' not found", runtime_name);
    return nullptr;
}

bool WasmRuntimeInjector::available() noexcept {
    auto &reg = globalRegistry();
    std::lock_guard<std::mutex> lock(reg.mu);
    return !reg.entries.empty();
}

std::vector<std::string> WasmRuntimeInjector::registeredNames() {
    auto &reg = globalRegistry();
    std::lock_guard<std::mutex> lock(reg.mu);

    // Copy and sort by descending priority
    std::vector<const WasmRuntimeDescriptor *> ptrs;
    ptrs.reserve(reg.entries.size());
    for (const auto &e : reg.entries) {
        ptrs.push_back(&e);
    }
    std::sort(ptrs.begin(), ptrs.end(),
              [](const WasmRuntimeDescriptor *a, const WasmRuntimeDescriptor *b) { return a->priority > b->priority; });

    std::vector<std::string> names;
    names.reserve(ptrs.size());
    for (const auto *p : ptrs) {
        names.push_back(p->name);
    }
    return names;
}

void WasmRuntimeInjector::clearAll() {
    auto &reg = globalRegistry();
    std::lock_guard<std::mutex> lock(reg.mu);
    reg.entries.clear();
}

} // namespace modules
} // namespace themis
