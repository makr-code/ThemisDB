/**
 * @file llama_lora_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=0, L=0
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
     * @brief Initialize LoRA API function pointers via dynamic lookup
     * 
     * Attempts to load LoRA functions from the llama.cpp library at runtime.
     * This allows ThemisDB to work with both LoRA-enabled and standard llama.cpp builds.
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
     * @brief Ensure API is initialized (thread-safe)
     */
    inline void ensureAPIInitialized() {
        std::call_once(g_lora_api_init_flag, initializeLoRAAPI);
    }
}

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

extern "C" {

void* llama_lora_adapter_init(struct llama_model* model, const char* path_lora);
int llama_lora_adapter_set_with_scale(struct llama_context* ctx, void* adapter, float scale);
void llama_lora_adapter_free(void* adapter);

/**
 * @brief Set/apply a LoRA adapter to a llama context
 * 
 * This function is called by MultiLoRAManager to apply loaded LoRA adapters
 * to inference contexts. It uses the real llama.cpp API when available.
 * 
 * @param ctx llama context pointer
 * @param adapter_path Path to LoRA adapter file (for compatibility with old signature)
 * @return 0 on success, -1 on error
 * 
 * @note The signature matches the legacy stub for backward compatibility.
 *       Modern code should use the full API with adapter handles.
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
 * @brief Initialize a LoRA adapter from file (modern API)
 * 
 * This is the proper way to load LoRA adapters in modern llama.cpp.
 * Returns an opaque handle to the loaded adapter.
 * 
 * @param model Base model pointer
 * @param path_lora Path to LoRA adapter file
 * @return Adapter handle on success, nullptr on error
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
 * @brief Apply LoRA adapter to context with scaling factor (modern API)
 * 
 * @param ctx Context to apply adapter to
 * @param adapter Adapter handle from llama_lora_adapter_init
 * @param scale Scaling factor (1.0 = full strength, 0.0 = disabled)
 * @return 0 on success, non-zero on error
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
 * @brief Remove LoRA adapter from context (modern API)
 * 
 * @param ctx Context to remove adapter from
 * @param adapter Adapter handle to remove
 * @return 0 on success, non-zero on error
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
 * @brief Clear all LoRA adapters from context (modern API)
 * 
 * @param ctx Context to clear adapters from
 * @return 0 on success, non-zero on error
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
 * @brief Free a LoRA adapter handle (modern API)
 * 
 * @param adapter Adapter handle to free
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
 * @brief Check if LoRA API is available at runtime
 * 
 * @return true if LoRA functions are available, false otherwise
 */
bool themis_llama_lora_available() {
    if (!g_lora_api_override_active) {
      ensureAPIInitialized();
    }
    return g_lora_api_override_active ? (g_override_lora_init != nullptr && g_override_lora_set != nullptr)
                                      : g_lora_api_available;
}

/**
 * @brief Apply LoRA adapter with integer handle (compatibility overload)
 * 
 * This overload is used by MultiLoRAManager which stores adapter handles as integers.
 * It casts the integer back to a void* pointer for the actual API call.
 * 
 * @param ctx Context to apply adapter to
 * @param adapter_index Integer representation of adapter handle
 * @param scale Scaling factor
 * @return 0 on success, -1 on error
 * 
 * @note This function assumes adapter_index was obtained from a valid adapter handle.
 *       Invalid adapter indices will be detected by llama.cpp and return an error.
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
 * @brief Inject LoRA API function pointers for testing.
 *
 * Overrides the runtime-detected (dlsym) function pointers so that unit tests
 * can exercise all LoRA code paths without a real llama.cpp build that exports
 * the LoRA API.  Pass nullptr for all parameters to revert to the detected path.
 *
 * @note Call before any other LoRA function.  Not thread-safe — intended for
 *       test set-up only.
 * @note Parameters are passed as void* to avoid a llama.h dependency in callers.
 *       Internally they are reinterpret_cast to the correct function pointer types.
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
