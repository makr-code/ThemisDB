/**
 * @file include/graph/namespace_config.h
 * @brief Namespace wrapper macros for Unity build consistency
 * 
 * When MSVC_UNITY_BUILD=ON, all themis_graph sources are concatenated into a single
 * compilation unit. To maintain namespace consistency across all 21 files:
 * 
 * - File 1 (ai_hardware_dispatcher.cpp): Opens THEMIS_NS_OPEN()
 * - Files 2-20: Use NO namespace macros (implicit inheritance from File 1)
 * - File 21 (graph_query_rewriter.cpp): Closes THEMIS_NS_CLOSE()
 * 
 * Each file still includes their own headers which may be in different namespaces
 * for standalone compilation, but the macro wrapper ensures Unity mode consistency.
 */

#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Namespace configuration for Unity builds
// ─────────────────────────────────────────────────────────────────────────────

#if defined(THEMIS_UNITY_BUILD_FIRST_FILE)
    // File 1 only: Open all namespaces
    #define THEMIS_NS_OPEN()  namespace themis { namespace graph {
    #define THEMIS_NS_CLOSE() } /* namespace graph */ } /* namespace themis */
    #define THEMIS_NS_MID_FILE_OPEN()
    #define THEMIS_NS_MID_FILE_CLOSE()

#elif defined(THEMIS_UNITY_BUILD_LAST_FILE)
    // File 21 only: Close all namespaces
    #define THEMIS_NS_OPEN()
    #define THEMIS_NS_CLOSE() } /* namespace graph */ } /* namespace themis */
    #define THEMIS_NS_MID_FILE_OPEN()
    #define THEMIS_NS_MID_FILE_CLOSE()

#elif defined(THEMIS_UNITY_BUILD_MID_FILE)
    // Files 2-20: No-op macros (implicit namespace inheritance)
    #define THEMIS_NS_OPEN()
    #define THEMIS_NS_CLOSE()
    #define THEMIS_NS_MID_FILE_OPEN()   // For cleanup of any redundant opens
    #define THEMIS_NS_MID_FILE_CLOSE()  // For cleanup of any redundant closes

#else
    // Standalone compilation: Each file uses its own namespace pattern
    #define THEMIS_NS_OPEN()    // Override per-file if needed
    #define THEMIS_NS_CLOSE()   // Override per-file if needed
    #define THEMIS_NS_MID_FILE_OPEN()
    #define THEMIS_NS_MID_FILE_CLOSE()
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Helper for removing redundant namespace declarations in mid-files
// ─────────────────────────────────────────────────────────────────────────────

/**
 * For mid-files (2-20), use this helper to suppress redundant namespace opens
 * at the file level, while preserving internal scoping if needed.
 * 
 * Example usage in graph_query_optimizer.cpp (File 10):
 * 
 *   // OLD: namespace themis { namespace graph { ... } }
 *   // NEW: SUPPRESS_REDUNDANT_NS { ... }
 * 
 * Where SUPPRESS_REDUNDANT_NS is empty in Unity mode, but defines local scope
 * for standalone builds.
 */
#if defined(THEMIS_UNITY_BUILD_MID_FILE)
    #define SUPPRESS_REDUNDANT_NS  // Empty in Unity mode
#else
    #define SUPPRESS_REDUNDANT_NS  namespace themis { namespace graph
#endif
