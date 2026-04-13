/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_grammar_adapter.cpp                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:26:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     242                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <llama.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <cstdint>

// Platform-specific dynamic library loading
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// ═══════════════════════════════════════════════════════════
// Dynamic Grammar API Loading
// ═══════════════════════════════════════════════════════════
//
// This implementation provides real grammar-constrained generation support
// via dynamic API detection. It checks if llama.cpp was compiled with grammar
// support and uses the real functions when available, falling back to stub
// behavior only when explicitly disabled or unavailable.
//
// Key Features:
// - Runtime detection of llama.cpp grammar API availability
// - Grammar-constrained token generation via llama.cpp native functions
// - Thread-safe initialization and error handling
// - Graceful degradation when grammar support is not available
//
// Compatibility:
// - llama.cpp with grammar support
// - Older llama.cpp versions (graceful fallback)
//
// ═══════════════════════════════════════════════════════════

namespace {
    // Function pointer types for llama.cpp Grammar API
    using llama_grammar_init_fn = struct llama_grammar* (*)(
        const struct llama_vocab*, 
        const char*, 
        const char*
    );
    using llama_grammar_free_fn = void (*)(struct llama_grammar*);
    using llama_grammar_sample_fn = void (*)(
        const struct llama_grammar*,
        const struct llama_context*,
        struct llama_token_data_array*
    );
    using llama_grammar_accept_fn = void (*)(
        struct llama_grammar*,
        const struct llama_context*,
        int
    );
    
    // API function pointers (initialized once)
    llama_grammar_init_fn g_llama_grammar_init = nullptr;
    llama_grammar_free_fn g_llama_grammar_free = nullptr;
    llama_grammar_sample_fn g_llama_grammar_sample = nullptr;
    llama_grammar_accept_fn g_llama_grammar_accept = nullptr;
    
    std::once_flag g_grammar_api_init_flag;
    bool g_grammar_api_available = false;
    
    /**
     * @brief Initialize Grammar API function pointers via dynamic lookup
     * 
     * Attempts to load Grammar functions from the llama.cpp library at runtime.
     * This allows ThemisDB to work with both grammar-enabled and standard llama.cpp builds.
     */
    void initializeGrammarAPI() {
        spdlog::info("Initializing llama.cpp Grammar API detection...");
        
        // Try to get function pointers from current process
        // Note: If llama.cpp is statically linked, these symbols should be available
        #ifdef _WIN32
        // On Windows, get handle to current process
        HMODULE hModule = GetModuleHandle(nullptr);
        g_llama_grammar_init = reinterpret_cast<llama_grammar_init_fn>(
            GetProcAddress(hModule, "llama_grammar_init")
        );
        g_llama_grammar_free = reinterpret_cast<llama_grammar_free_fn>(
            GetProcAddress(hModule, "llama_grammar_free")
        );
        g_llama_grammar_sample = reinterpret_cast<llama_grammar_sample_fn>(
            GetProcAddress(hModule, "llama_grammar_sample")
        );
        g_llama_grammar_accept = reinterpret_cast<llama_grammar_accept_fn>(
            GetProcAddress(hModule, "llama_grammar_accept")
        );
        #else
        // On Unix-like systems, use dlsym
        g_llama_grammar_init = reinterpret_cast<llama_grammar_init_fn>(
            dlsym(RTLD_DEFAULT, "llama_grammar_init")
        );
        g_llama_grammar_free = reinterpret_cast<llama_grammar_free_fn>(
            dlsym(RTLD_DEFAULT, "llama_grammar_free")
        );
        g_llama_grammar_sample = reinterpret_cast<llama_grammar_sample_fn>(
            dlsym(RTLD_DEFAULT, "llama_grammar_sample")
        );
        g_llama_grammar_accept = reinterpret_cast<llama_grammar_accept_fn>(
            dlsym(RTLD_DEFAULT, "llama_grammar_accept")
        );
        #endif
        
        // Check if all critical functions are available
        g_grammar_api_available = (
            g_llama_grammar_init != nullptr &&
            g_llama_grammar_free != nullptr &&
            g_llama_grammar_sample != nullptr &&
            g_llama_grammar_accept != nullptr
        );
        
        if (g_grammar_api_available) {
            spdlog::info("✓ llama.cpp Grammar API detected and available");
        } else {
            spdlog::warn("✗ llama.cpp Grammar API not available");
            spdlog::warn("  Grammar-constrained generation will be disabled");
            spdlog::warn("  To enable: Rebuild llama.cpp with grammar support");
        }
    }
    
    /**
     * @brief Ensure Grammar API is initialized before use
     */
    inline void ensureGrammarAPIInitialized() {
        std::call_once(g_grammar_api_init_flag, initializeGrammarAPI);
    }
} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// Public Grammar API Functions
// ═══════════════════════════════════════════════════════════

extern "C" {

/**
 * @brief Check if llama.cpp Grammar API is available
 * @return true if all Grammar functions are available, false otherwise
 */
bool themis_llama_grammar_available() {
    ensureGrammarAPIInitialized();
    return g_grammar_api_available;
}

/**
 * @brief Initialize a grammar from EBNF text
 * @param vocab Vocabulary from llama model
 * @param grammar_str EBNF grammar string
 * @param start_rule Starting rule name (e.g., "root")
 * @return Grammar handle, or nullptr if failed or API unavailable
 */
struct llama_grammar* llama_grammar_init(
    const struct llama_vocab* vocab,
    const char* grammar_str,
    const char* start_rule
) {
    ensureGrammarAPIInitialized();
    
    if (!g_grammar_api_available || !g_llama_grammar_init) {
        spdlog::warn("llama_grammar_init called but API is not available");
        return nullptr;
    }
    
    return g_llama_grammar_init(vocab, grammar_str, start_rule);
}

/**
 * @brief Free grammar resources
 * @param grammar Grammar handle to free
 */
void llama_grammar_free(struct llama_grammar* grammar) {
    ensureGrammarAPIInitialized();
    
    if (!g_grammar_api_available || !g_llama_grammar_free || !grammar) {
        return;
    }
    
    g_llama_grammar_free(grammar);
}

/**
 * @brief Apply grammar constraints to token candidates
 * @param grammar Grammar handle
 * @param ctx Context handle
 * @param candidates Token candidates array (will be filtered in-place)
 */
void llama_grammar_sample(
    const struct llama_grammar* grammar,
    const struct llama_context* ctx,
    struct llama_token_data_array* candidates
) {
    ensureGrammarAPIInitialized();
    
    if (!g_grammar_api_available || !g_llama_grammar_sample || !grammar || !ctx || !candidates) {
        return;
    }
    
    g_llama_grammar_sample(grammar, ctx, candidates);
}

/**
 * @brief Update grammar state after token generation
 * @param grammar Grammar handle
 * @param ctx Context handle
 * @param token The token that was generated
 */
void llama_grammar_accept(
    struct llama_grammar* grammar,
    const struct llama_context* ctx,
    int token
) {
    ensureGrammarAPIInitialized();
    
    if (!g_grammar_api_available || !g_llama_grammar_accept || !grammar || !ctx) {
        return;
    }
    
    g_llama_grammar_accept(grammar, ctx, token);
}

} // extern "C"
