/**
 * @file llama_lora_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <llama.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <vector>
#include <mutex>
#include <cstdint>
#include <unordered_map>

// Platform-specific dynamic library loading
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// ═══════════════════════════════════════════════════════════
// Dynamic LoRA API Loading
// ═══════════════════════════════════════════════════════════
//
// This implementation provides real LoRA adapter support via dynamic
// API detection. It checks if llama.cpp was compiled with LoRA support
// and uses the real functions when available, falling back to stub
// behavior only when explicitly disabled or unavailable.
//
// Key Features:
// - Runtime detection of llama.cpp LoRA API availability
// - Zero-copy adapter application via llama.cpp native functions
// - Thread-safe initialization and error handling
// - Graceful degradation when LoRA support is not available
//
// Compatibility:
// - llama.cpp b1000+ (with LoRA adapter API)
// - Older llama.cpp versions (graceful fallback)
//
// ═══════════════════════════════════════════════════════════

namespace {
    // Function pointer types for llama.cpp LoRA API
    using llama_lora_adapter_init_fn = void* (*)(struct llama_model*, const char*);
    using llama_lora_adapter_set_fn = int (*)(struct llama_context*, void*, float);
    using llama_lora_adapter_remove_fn = int (*)(struct llama_context*, void*);
    using llama_lora_adapter_clear_fn = int (*)(struct llama_context*);
    using llama_lora_adapter_free_fn = void (*)(void*);
    
    // API function pointers (initialized once)
    llama_lora_adapter_init_fn g_llama_lora_adapter_init = nullptr;
    llama_lora_adapter_set_fn g_llama_lora_adapter_set = nullptr;
    llama_lora_adapter_remove_fn g_llama_lora_adapter_remove = nullptr;
    llama_lora_adapter_clear_fn g_llama_lora_adapter_clear = nullptr;
    llama_lora_adapter_free_fn g_llama_lora_adapter_free = nullptr;
    
    std::once_flag g_lora_api_init_flag;
    bool g_lora_api_available = false;

    // Override function pointers for testing (set via themis_lora_inject_api_functions()
    // before any LoRA call).  When g_lora_api_override_active is true, these take
    // precedence over the dlsym-detected pointers so tests can run without a real
    // llama.cpp LoRA build.
    llama_lora_adapter_init_fn   g_override_lora_init   = nullptr;
    llama_lora_adapter_set_fn    g_override_lora_set    = nullptr;
    llama_lora_adapter_remove_fn g_override_lora_remove = nullptr;
    llama_lora_adapter_clear_fn  g_override_lora_clear  = nullptr;
    llama_lora_adapter_free_fn   g_override_lora_free   = nullptr;
    bool g_lora_api_override_active = false;
    std::mutex g_legacy_adapter_mutex;
    std::unordered_map<struct llama_context*, std::vector<void*>> g_legacy_adapters;
    
    /**
     * @brief Initialize Lo RAAPI.
     * @details Calls: spdlog::info(), GetModuleHandle(), GetProcAddress(), dlsym(), spdlog::warn().
     */
    void initializeLoRAAPI() {
        spdlog::info("Initializing llama.cpp LoRA API detection...");
        
        // Try to get function pointers from current process
        // Note: If llama.cpp is statically linked, these symbols should be available
        #ifdef _WIN32
        // On Windows, get handle to current process
        HMODULE hModule = GetModuleHandle(nullptr);
        g_llama_lora_adapter_init = reinterpret_cast<llama_lora_adapter_init_fn>(
            GetProcAddress(hModule, "llama_lora_adapter_init")
        );
        g_llama_lora_adapter_set = reinterpret_cast<llama_lora_adapter_set_fn>(
            GetProcAddress(hModule, "llama_lora_adapter_set")
        );
        g_llama_lora_adapter_remove = reinterpret_cast<llama_lora_adapter_remove_fn>(
            GetProcAddress(hModule, "llama_lora_adapter_remove")
        );
        g_llama_lora_adapter_clear = reinterpret_cast<llama_lora_adapter_clear_fn>(
            GetProcAddress(hModule, "llama_lora_adapter_clear")
        );
        g_llama_lora_adapter_free = reinterpret_cast<llama_lora_adapter_free_fn>(
            GetProcAddress(hModule, "llama_lora_adapter_free")
        );
        #else
        // On Unix-like systems, use dlsym
        g_llama_lora_adapter_init = reinterpret_cast<llama_lora_adapter_init_fn>(
            dlsym(RTLD_DEFAULT, "llama_lora_adapter_init")
        );
        g_llama_lora_adapter_set = reinterpret_cast<llama_lora_adapter_set_fn>(
            dlsym(RTLD_DEFAULT, "llama_lora_adapter_set")
        );
        g_llama_lora_adapter_remove = reinterpret_cast<llama_lora_adapter_remove_fn>(
            dlsym(RTLD_DEFAULT, "llama_lora_adapter_remove")
        );
        g_llama_lora_adapter_clear = reinterpret_cast<llama_lora_adapter_clear_fn>(
            dlsym(RTLD_DEFAULT, "llama_lora_adapter_clear")
        );
        g_llama_lora_adapter_free = reinterpret_cast<llama_lora_adapter_free_fn>(
            dlsym(RTLD_DEFAULT, "llama_lora_adapter_free")
        );
        #endif
        
        // Check if all critical functions are available
        if (g_llama_lora_adapter_init && g_llama_lora_adapter_set) {
            g_lora_api_available = true;
            spdlog::info("✓ llama.cpp LoRA API detected and loaded successfully");
            spdlog::info("  - llama_lora_adapter_init: {}", 
                         g_llama_lora_adapter_init ? "available" : "missing");
            spdlog::info("  - llama_lora_adapter_set: {}", 
                         g_llama_lora_adapter_set ? "available" : "missing");
            spdlog::info("  - llama_lora_adapter_remove: {}", 
                         g_llama_lora_adapter_remove ? "available" : "missing");
            spdlog::info("  - llama_lora_adapter_clear: {}", 
                         g_llama_lora_adapter_clear ? "available" : "missing");
            spdlog::info("  - llama_lora_adapter_free: {}", 
                         g_llama_lora_adapter_free ? "available" : "missing");
        } else {
            g_lora_api_available = false;
            spdlog::warn("✗ llama.cpp LoRA API not available in this build");
            spdlog::warn("  LoRA adapter functionality will be disabled");
            spdlog::warn("  To enable: rebuild llama.cpp with LLAMA_LORA=ON");
            
            // Log which specific functions are missing
            if (!g_llama_lora_adapter_init) {
                spdlog::warn("  - Missing: llama_lora_adapter_init");
            }
            if (!g_llama_lora_adapter_set) {
                spdlog::warn("  - Missing: llama_lora_adapter_set");
            }
        }
    }
    
    /**
     * @brief Ensure APIInitialized.
     * @details Calls: std::call_once().
     */
    inline void ensureAPIInitialized() {
        std::call_once(g_lora_api_init_flag, initializeLoRAAPI);
    }
}

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

extern "C" {

/**
 * @brief Llama lora adapter init.
 * @param[in,out] model Input/output parameter.
 * @param[in] path_lora Input parameter.
 * @return Pointer to the result.
 */
void* llama_lora_adapter_init(struct llama_model* model, const char* path_lora);
/**
 * @brief Llama lora adapter set with scale.
 * @param[in,out] ctx Input/output parameter.
 * @param[in,out] adapter Input/output parameter.
 * @param[in] scale Input parameter.
 * @return Return value.
 */
int llama_lora_adapter_set_with_scale(struct llama_context* ctx, void* adapter, float scale);
/**
 * @brief Llama lora adapter free.
 * @param[in,out] adapter Input/output parameter.
 */
void llama_lora_adapter_free(void* adapter);

/**
 * @brief Llama lora adapter set path.
 * @param[in,out] ctx Input/output parameter.
 * @param[in] adapter_path Path to the adapter.
 * @return Return value.
 * @details Calls: ensureAPIInitialized(), spdlog::error(), llama_get_model(), llama_lora_adapter_init(), llama_lora_adapter_set_with_scale(), llama_lora_adapter_free(), lock(), push_back().
 */
int llama_lora_adapter_set_path(struct llama_context* ctx, const char* adapter_path) {
    ensureAPIInitialized();
    
    if (!ctx) {
        spdlog::error("llama_lora_adapter_set: null context provided");
        return -1;
    }
    
    if (!adapter_path || adapter_path[0] == '\0') {
        spdlog::error("llama_lora_adapter_set: null or empty adapter path");
        return -1;
    }
    
    // Check if LoRA API is available
    if (!g_lora_api_available) {
        spdlog::error("llama_lora_adapter_set: LoRA API not available in this llama.cpp build");
        spdlog::error("  Adapter path: {}", adapter_path);
        spdlog::error("  Rebuild llama.cpp with LLAMA_LORA=ON to enable LoRA support");
        return -1;
    }
    
    auto* model = const_cast<struct llama_model*>(llama_get_model(ctx));
    if (!model) {
        spdlog::error("llama_lora_adapter_set: unable to resolve llama model from context");
        return -1;
    }

    void* adapter = llama_lora_adapter_init(model, adapter_path);
    if (!adapter) {
        return -1;
    }

    const int result = llama_lora_adapter_set_with_scale(ctx, adapter, 1.0f);
    if (result != 0) {
        llama_lora_adapter_free(adapter);
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(g_legacy_adapter_mutex);
        g_legacy_adapters[ctx].push_back(adapter);
    }

    return 0;
}

/**
 * @brief Llama lora adapter init.
 * @param[in,out] model Input/output parameter.
 * @param[in] path_lora Input parameter.
 * @return Pointer to the result.
 * @details Calls: ensureAPIInitialized(), spdlog::error(), spdlog::info(), fn().
 */
void* llama_lora_adapter_init(struct llama_model* model, const char* path_lora) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_init : g_llama_lora_adapter_init;
    const bool avail = g_lora_api_override_active ? (g_override_lora_init != nullptr) : g_lora_api_available;

    if (!avail || !fn) {
        spdlog::error("llama_lora_adapter_init: LoRA API not available");
        return nullptr;
    }
    
    if (!model) {
        spdlog::error("llama_lora_adapter_init: null model provided");
        return nullptr;
    }
    
    if (!path_lora || path_lora[0] == '\0') {
        spdlog::error("llama_lora_adapter_init: null or empty adapter path");
        return nullptr;
    }
    
    spdlog::info("Loading LoRA adapter from: {}", path_lora);
    
    // Call real (or injected) llama.cpp function
    void* adapter = fn(model, path_lora);
    
    if (adapter) {
        spdlog::info("✓ LoRA adapter loaded successfully");
    } else {
        spdlog::error("✗ Failed to load LoRA adapter from: {}", path_lora);
    }
    
    return adapter;
}

/**
 * @brief Llama lora adapter set with scale.
 * @param[in,out] ctx Input/output parameter.
 * @param[in,out] adapter Input/output parameter.
 * @param[in] scale Input parameter.
 * @return Return value.
 * @details Calls: ensureAPIInitialized(), spdlog::error(), spdlog::debug(), fn().
 */
int llama_lora_adapter_set_with_scale(struct llama_context* ctx, void* adapter, float scale) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_set : g_llama_lora_adapter_set;
    const bool avail = g_lora_api_override_active ? (g_override_lora_set != nullptr) : g_lora_api_available;

    if (!avail || !fn) {
        spdlog::error("llama_lora_adapter_set_with_scale: LoRA API not available");
        return -1;
    }
    
    if (!ctx || !adapter) {
        spdlog::error("llama_lora_adapter_set_with_scale: null context or adapter");
        return -1;
    }
    
    spdlog::debug("Applying LoRA adapter with scale: {}", scale);
    
    // Call real (or injected) llama.cpp function
    int result = fn(ctx, adapter, scale);
    
    if (result == 0) {
        spdlog::debug("✓ LoRA adapter applied successfully");
    } else {
        spdlog::error("✗ Failed to apply LoRA adapter (error: {})", result);
    }
    
    return result;
}

/**
 * @brief Llama lora adapter remove.
 * @param[in,out] ctx Input/output parameter.
 * @param[in,out] adapter Input/output parameter.
 * @return Return value.
 * @details Calls: ensureAPIInitialized(), set_fn(), spdlog::error(), spdlog::debug(), fn(), lock(), find(), end().
 */
int llama_lora_adapter_remove(struct llama_context* ctx, void* adapter) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_remove : g_llama_lora_adapter_remove;
    auto* set_fn    = g_lora_api_override_active ? g_override_lora_set    : g_llama_lora_adapter_set;
    const bool avail = g_lora_api_override_active ? (g_override_lora_remove != nullptr || g_override_lora_set != nullptr)
                                                   : g_lora_api_available;

    if (!avail || !fn) {
        // If remove is not available, try setting scale to 0
        if (set_fn) {
            return set_fn(ctx, adapter, 0.0f);
        }
        spdlog::error("llama_lora_adapter_remove: LoRA API not available");
        return -1;
    }
    
    if (!ctx || !adapter) {
        spdlog::error("llama_lora_adapter_remove: null context or adapter");
        return -1;
    }
    
    spdlog::debug("Removing LoRA adapter from context");
    
    // Call real (or injected) llama.cpp function
    int result = fn(ctx, adapter);
    
    if (result == 0) {
        spdlog::debug("✓ LoRA adapter removed successfully");
        {
            std::lock_guard<std::mutex> lock(g_legacy_adapter_mutex);
            auto it = g_legacy_adapters.find(ctx);
            if (it != g_legacy_adapters.end()) {
                auto& adapters = it->second;
                adapters.erase(std::remove(adapters.begin(), adapters.end(), adapter), adapters.end());
                if (adapters.empty()) {
                    g_legacy_adapters.erase(it);
                }
            }
        }
        if (g_lora_api_override_active ? g_override_lora_free != nullptr : g_llama_lora_adapter_free != nullptr) {
            llama_lora_adapter_free(adapter);
        }
    } else {
        spdlog::warn("✗ Failed to remove LoRA adapter (error: {})", result);
    }
    
    return result;
}

/**
 * @brief Llama lora adapter clear.
 * @param[in,out] ctx Input/output parameter.
 * @return Return value.
 * @details Calls: ensureAPIInitialized(), spdlog::error(), lock(), find(), end(), std::move(), erase(), empty().
 */
int llama_lora_adapter_clear(struct llama_context* ctx) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_clear : g_llama_lora_adapter_clear;
    const bool avail = g_lora_api_override_active ? (g_override_lora_clear != nullptr) : g_lora_api_available;

    if (!ctx) {
        spdlog::error("llama_lora_adapter_clear: null context");
        return -1;
    }

    if (!avail || !fn) {
        std::vector<void*> legacy_adapters;
        {
            std::lock_guard<std::mutex> lock(g_legacy_adapter_mutex);
            auto it = g_legacy_adapters.find(ctx);
            if (it != g_legacy_adapters.end()) {
                legacy_adapters = std::move(it->second);
                g_legacy_adapters.erase(it);
            }
        }
        if (legacy_adapters.empty()) {
            spdlog::warn("llama_lora_adapter_clear: LoRA API not available");
            return -1;
        }
        for (void* adapter : legacy_adapters) {
            llama_lora_adapter_free(adapter);
        }
        return 0;
    }
    
    spdlog::debug("Clearing all LoRA adapters from context");
    
    // Call real (or injected) llama.cpp function
    int result = fn(ctx);
    
    if (result == 0) {
        spdlog::debug("✓ All LoRA adapters cleared successfully");
        std::vector<void*> legacy_adapters;
        {
            std::lock_guard<std::mutex> lock(g_legacy_adapter_mutex);
            auto it = g_legacy_adapters.find(ctx);
            if (it != g_legacy_adapters.end()) {
                legacy_adapters = std::move(it->second);
                g_legacy_adapters.erase(it);
            }
        }
        for (void* adapter : legacy_adapters) {
            llama_lora_adapter_free(adapter);
        }
    } else {
        spdlog::warn("✗ Failed to clear LoRA adapters (error: {})", result);
    }
    
    return result;
}

/**
 * @brief Llama lora adapter free.
 * @param[in,out] adapter Input/output parameter.
 * @details Calls: ensureAPIInitialized(), spdlog::warn(), spdlog::debug(), fn().
 */
void llama_lora_adapter_free(void* adapter) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_free : g_llama_lora_adapter_free;
    const bool avail = g_lora_api_override_active ? (g_override_lora_free != nullptr) : g_lora_api_available;

    if (!avail || !fn) {
        spdlog::warn("llama_lora_adapter_free: LoRA API not available, handle not freed");
        return;
    }
    
    if (!adapter) {
        spdlog::debug("llama_lora_adapter_free: null adapter (nothing to free)");
        return;
    }
    
    spdlog::debug("Freeing LoRA adapter handle");
    
    // Call real (or injected) llama.cpp function
    fn(adapter);
    
    spdlog::debug("✓ LoRA adapter handle freed");
}

/**
 * @brief Themis llama lora available.
 * @return True when the operation succeeds.
 * @details Calls: ensureAPIInitialized().
 */
bool themis_llama_lora_available() {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    return g_lora_api_override_active ? (g_override_lora_init != nullptr && g_override_lora_set != nullptr)
                                      : g_lora_api_available;
}

/**
 * @brief Llama lora adapter set.
 * @param[in,out] ctx Input/output parameter.
 * @param[in] adapter_index Input parameter.
 * @param[in] scale Input parameter.
 * @return Return value.
 * @details Calls: ensureAPIInitialized(), spdlog::error(), spdlog::debug(), fn().
 */
int llama_lora_adapter_set(struct llama_context* ctx, int adapter_index, float scale) {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    auto* fn        = g_lora_api_override_active ? g_override_lora_set : g_llama_lora_adapter_set;
    const bool avail = g_lora_api_override_active ? (g_override_lora_set != nullptr) : g_lora_api_available;
    
    if (!ctx) {
        spdlog::error("llama_lora_adapter_set: null context provided");
        return -1;
    }
    
    // Validate adapter index (should not be negative or zero in normal usage)
    // Note: MultiLoRAManager stores handles as positive integers
    if (adapter_index <= 0) {
        spdlog::error("llama_lora_adapter_set: invalid adapter handle ({})", adapter_index);
        spdlog::error("  Adapter handles should be positive integers obtained from adapter loading");
        return -1;
    }
    
    if (!avail) {
        spdlog::error("llama_lora_adapter_set: LoRA API not available in this llama.cpp build");
        spdlog::error("  Rebuild llama.cpp with LLAMA_LORA=ON to enable LoRA support");
        return -1;
    }
    
    // Convert integer handle back to pointer
    void* adapter = reinterpret_cast<void*>(static_cast<uintptr_t>(adapter_index));
    
    spdlog::debug("Applying LoRA adapter (handle: 0x{:x}) with scale: {}", 
                  static_cast<uintptr_t>(adapter_index), scale);
    
    // Call the real (or injected) llama.cpp function
    if (!fn) {
        spdlog::error("llama_lora_adapter_set function pointer not initialized");
        return -1;
    }
    
    int result = fn(ctx, adapter, scale);
    
    if (result == 0) {
        spdlog::debug("✓ LoRA adapter applied successfully");
    } else {
        spdlog::error("✗ Failed to apply LoRA adapter (error: {})", result);
        spdlog::error("  This may indicate:");
        spdlog::error("    - Invalid adapter handle");
        spdlog::error("    - Adapter incompatible with model");
        spdlog::error("    - Context already has maximum adapters applied");
    }
    
    return result;
}

/**
 * @brief Themis lora inject api functions.
 * @param[in,out] init_fn Input/output parameter.
 * @param[in,out] set_fn Input/output parameter.
 * @param[in,out] remove_fn Input/output parameter.
 * @param[in,out] clear_fn Input/output parameter.
 * @param[in,out] free_fn Input/output parameter.
 */
void themis_lora_inject_api_functions(
    void* init_fn,
    void* set_fn,
    void* remove_fn,
    void* clear_fn,
    void* free_fn)
{
    g_override_lora_init   = reinterpret_cast<llama_lora_adapter_init_fn>(init_fn);
    g_override_lora_set    = reinterpret_cast<llama_lora_adapter_set_fn>(set_fn);
    g_override_lora_remove = reinterpret_cast<llama_lora_adapter_remove_fn>(remove_fn);
    g_override_lora_clear  = reinterpret_cast<llama_lora_adapter_clear_fn>(clear_fn);
    g_override_lora_free   = reinterpret_cast<llama_lora_adapter_free_fn>(free_fn);
    g_lora_api_override_active = (init_fn != nullptr && set_fn != nullptr);
    if (g_lora_api_override_active) {
        g_lora_api_available = true;
        spdlog::debug("LoRA API injected for testing (init={}, set={})",
                      init_fn != nullptr, set_fn != nullptr);
    } else {
        g_lora_api_available = false;
        spdlog::debug("LoRA API injection cleared; reverted to dlsym-detected path");
    }
}

} // extern "C"
