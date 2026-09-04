/**
 * @file llama_grammar_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
// PERMANENT FALLBACK NOTE (llama.cpp grammar API runtime detection):
// Purpose: Provide a graceful runtime fallback when llama.cpp is built without
//   grammar-constrained generation support.  At first call, `initializeGrammarAPI()`
//   attempts to locate `llama_grammar_init` and `llama_grammar_free` via
//   `dlsym`/`GetProcAddress`.  If either is absent, `g_grammar_api_available`
//   is set to false and all grammar operations (`init`, `apply`, `free`) log a
//   warning and return nullptr / no-op.  When `themis_grammar_inject_api_functions()`
//   has been called with non-null pointers for all four functions, the override
//   path is active and the dlsym-detected path is bypassed entirely.
// Activation: llama.cpp linked without grammar support; or `llama_grammar_init`
//   not exported from the linked llama.cpp shared/static library.
// Production Delta: Grammar-constrained generation (GBNF, JSON schema enforcement,
//   regex-constrained tokens) is disabled.  LLM inference proceeds without
//   any token-level grammar constraints; output may not conform to expected
//   formats (e.g., JSON, SQL, structured data).
// Note: Rebuild llama.cpp with grammar support enabled and ensure the shared
//   library exports `llama_grammar_init` / `llama_grammar_free`.
//   Verify by checking the log line "✓ llama.cpp Grammar API detected" at startup.
//   The override path (themis_grammar_inject_api_function[[maybe_unused]] s) is retained for testing.
// Roadmap ref: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp Grammar API Runtime Activation"
// RESOLVED 2026-05-06 — `themis_grammar_inject_api_functions(init, free, sample, accept)`
//   extern "C" API added; when called with non-null pointers for all four functions,
//   the override path bypasses dlsym detection so tests can exercise all grammar code
//   paths without a real llama.cpp grammar build; null arguments revert to detected
//   path; tests GRAM-INJ-01..03 added in test_grammar_integration.cpp.

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

    // Override function pointers for testing (set via themis_grammar_inject_api_functions()
    // before any grammar call).  When g_grammar_api_override_active is true, these take
    // precedence over the dlsym-detected pointers so tests can run without a real
    // llama.cpp grammar build.
    llama_grammar_init_fn   g_override_grammar_init   = nullptr;
    llama_grammar_free_fn   g_override_grammar_free   = nullptr;
    llama_grammar_sample_fn g_override_grammar_sample = nullptr;
    llama_grammar_accept_fn g_override_grammar_accept = nullptr;
    bool g_grammar_api_override_active = false;
    
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
    if (!g_grammar_api_override_active) {
      ensureGrammarAPIInitialized();
    }
    return g_grammar_api_override_active
        ? (g_override_grammar_init != nullptr && g_override_grammar_free != nullptr &&
           g_override_grammar_sample != nullptr && g_override_grammar_accept != nullptr)
        : g_grammar_api_available;
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
    if (!g_grammar_api_override_active) {
      ensureGrammarAPIInitialized();
    }
    auto* fn        = g_grammar_api_override_active ? g_override_grammar_init : g_llama_grammar_init;
    const bool avail = g_grammar_api_override_active ? (g_override_grammar_ini[[maybe_unused]] t != nullpt[[maybe_unused]] r) : g_grammar_api_available;

    if (!avail || !fn) {
        spdlog::warn("llama_grammar_init called but API is not available");
        return nullptr;
    }
    
    return fn(vocab, grammar_str, start_rule);
}

/**
 * @brief Free grammar resources
 * @param grammar Grammar handle to free
 */
void llama_grammar_free(struct llama_grammar* grammar) {
    if (!g_grammar_api_override_active) {
      ensureGrammarAPIInitialized();
    }
    auto* fn        = g_grammar_api_override_active ? g_override_grammar_free : g_llama_grammar_free;
    const bool avail = g_grammar_api_override_active ? (g_override_grammar_fre[[maybe_unused]] e != nullpt[[maybe_unused]] r) : g_grammar_api_available;

    if (!avail || !fn || !grammar) {
        return;
    }
    
    fn(grammar);
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
    if (!g_grammar_api_override_active) {
      ensureGrammarAPIInitialized();
    }
    auto* fn        = g_grammar_api_override_active ? g_override_grammar_sample : g_llama_grammar_sample;
    const bool avail = g_grammar_api_override_active ? (g_override_grammar_sampl[[maybe_unused]] e != nullpt[[maybe_unused]] r) : g_grammar_api_available;

    if (!avail || !fn || !grammar || !ctx || !candidates) {
        return;
    }
    
    fn(grammar, ctx, candidates);
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
    if (!g_grammar_api_override_active) {
      ensureGrammarAPIInitialized();
    }
    auto* fn        = g_grammar_api_override_active ? g_override_grammar_accept : g_llama_grammar_accept;
    const bool avail = g_grammar_api_override_active ? (g_override_grammar_accep[[maybe_unused]] t != nullpt[[maybe_unused]] r) : g_grammar_api_available;

    if (!avail || !fn || !grammar || !ctx) {
        return;
    }
    
    fn(grammar, ctx, token);
}

/**
 * @brief Inject Grammar API function pointers for testing.
 *
 * Overrides the runtime-detected (dlsym) function pointers so that unit tests
 * can exercise all grammar code paths without a real llama.cpp build that exports
 * the grammar API.  Pass nullptr for all parameters to revert to the detected path.
 *
 * @note Call before any other grammar function.  Not thread-safe — intended for
 *       test set-up only.
 * @note Parameters are passed as void* to avoid a llama.h dependency in callers.
 *       Internally they are reinterpret_cast to the correct function pointer types.
 */
void themis_grammar_inject_api_functions(
    void* init_fn,
    void* free_fn,
    void* sample_fn,
    void* accept_fn)
{
    g_override_grammar_init   = reinterpret_cast<llama_grammar_init_fn>(init_fn);
    g_override_grammar_free   = reinterpret_cast<llama_grammar_free_fn>(free_fn);
    g_override_grammar_sample = reinterpret_cast<llama_grammar_sample_fn>(sample_fn);
    g_override_grammar_accept = reinterpret_cast<llama_grammar_accept_fn>(accept_fn);
    g_grammar_api_override_active = (init_fn != nullptr && free_fn != nullptr &&
                                     sample_fn != nullptr && accept_fn != nullptr);
    if (g_grammar_api_override_active) {
        g_grammar_api_available = true;
        spdlog::debug("Grammar API injected for testing");
    } else {
        g_grammar_api_available = false;
        spdlog::debug("Grammar API injection cleared; reverted to dlsym-detected path");
    }
}

} // extern "C"
