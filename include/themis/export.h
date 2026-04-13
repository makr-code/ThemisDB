/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export.h                                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:21:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     146                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Export Macros for Modular Architecture
// This file provides DLL export/import macros for all ThemisDB modules
// Windows requires explicit __declspec(dllexport/dllimport) for shared libraries

#ifndef THEMIS_EXPORT_H
#define THEMIS_EXPORT_H

// Detect platform
#if defined(_WIN32) || defined(_WIN64)
    #define THEMIS_PLATFORM_WINDOWS
#endif

// Base export macro helper
#ifdef THEMIS_PLATFORM_WINDOWS
    #define THEMIS_EXPORT_MACRO __declspec(dllexport)
    #define THEMIS_IMPORT_MACRO __declspec(dllimport)
    #define THEMIS_HIDDEN_MACRO
#else
    #if __GNUC__ >= 4
        #define THEMIS_EXPORT_MACRO __attribute__((visibility("default")))
        #define THEMIS_IMPORT_MACRO __attribute__((visibility("default")))
        #define THEMIS_HIDDEN_MACRO __attribute__((visibility("hidden")))
    #else
        #define THEMIS_EXPORT_MACRO
        #define THEMIS_IMPORT_MACRO
        #define THEMIS_HIDDEN_MACRO
    #endif
#endif

// Module-specific export macros
// Each module defines THEMIS_<MODULE>_EXPORTS when building the DLL

// themis_base - Core types, interfaces, utilities
#ifdef THEMIS_BASE_EXPORTS
    #define THEMIS_BASE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_BASE_API THEMIS_IMPORT_MACRO
#endif

// themis_storage - Storage engine and indexes
#ifdef THEMIS_STORAGE_EXPORTS
    #define THEMIS_STORAGE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_STORAGE_API THEMIS_IMPORT_MACRO
#endif

// themis_query - Query engine and AQL
#ifdef THEMIS_QUERY_EXPORTS
    #define THEMIS_QUERY_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_QUERY_API THEMIS_IMPORT_MACRO
#endif

// themis_security - Encryption, PKI, RBAC
#ifdef THEMIS_SECURITY_EXPORTS
    #define THEMIS_SECURITY_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_SECURITY_API THEMIS_IMPORT_MACRO
#endif

// themis_network - HTTP/gRPC servers
#ifdef THEMIS_NETWORK_EXPORTS
    #define THEMIS_NETWORK_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_NETWORK_API THEMIS_IMPORT_MACRO
#endif

// themis_transaction - Transaction management
#ifdef THEMIS_TRANSACTION_EXPORTS
    #define THEMIS_TRANSACTION_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_TRANSACTION_API THEMIS_IMPORT_MACRO
#endif

// themis_sharding - Distributed system (optional)
#ifdef THEMIS_SHARDING_EXPORTS
    #define THEMIS_SHARDING_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_SHARDING_API THEMIS_IMPORT_MACRO
#endif

// themis_llm - LLM integration (optional)
#ifdef THEMIS_LLM_EXPORTS
    #define THEMIS_LLM_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_LLM_API THEMIS_IMPORT_MACRO
#endif

// themis_content - Content processors (optional)
#ifdef THEMIS_CONTENT_EXPORTS
    #define THEMIS_CONTENT_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_CONTENT_API THEMIS_IMPORT_MACRO
#endif

// themis_timeseries - Time-series support (optional)
#ifdef THEMIS_TIMESERIES_EXPORTS
    #define THEMIS_TIMESERIES_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_TIMESERIES_API THEMIS_IMPORT_MACRO
#endif

// themis_graph - Graph analytics (optional)
#ifdef THEMIS_GRAPH_EXPORTS
    #define THEMIS_GRAPH_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_GRAPH_API THEMIS_IMPORT_MACRO
#endif

// themis_geo - Geospatial features (optional)
#ifdef THEMIS_GEO_EXPORTS
    #define THEMIS_GEO_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_GEO_API THEMIS_IMPORT_MACRO
#endif

// Legacy compatibility: themis_core for monolithic builds
#ifdef THEMIS_CORE_EXPORTS
    #define THEMIS_CORE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_CORE_API THEMIS_IMPORT_MACRO
#endif

#endif // THEMIS_EXPORT_H
