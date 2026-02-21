/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export.h                                           ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:43:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Export macros for modular ThemisDB libraries
// This file will be used post-v1.3.0 when modular build is implemented
// See docs/architecture/MODULARIZATION_PLAN.md for details

#pragma once

// Platform-specific export/import macros
#if defined(_WIN32) || defined(_WIN64)
    #define THEMIS_EXPORT_HELPER_DLL __declspec(dllexport)
    #define THEMIS_IMPORT_HELPER_DLL __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
    #define THEMIS_EXPORT_HELPER_DLL __attribute__((visibility("default")))
    #define THEMIS_IMPORT_HELPER_DLL __attribute__((visibility("default")))
#else
    #define THEMIS_EXPORT_HELPER_DLL
    #define THEMIS_IMPORT_HELPER_DLL
#endif

// themis_base module exports
#ifdef THEMIS_BASE_EXPORTS
    #define THEMIS_BASE_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_BASE_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_storage module exports
#ifdef THEMIS_STORAGE_EXPORTS
    #define THEMIS_STORAGE_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_STORAGE_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_query module exports
#ifdef THEMIS_QUERY_EXPORTS
    #define THEMIS_QUERY_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_QUERY_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_security module exports
#ifdef THEMIS_SECURITY_EXPORTS
    #define THEMIS_SECURITY_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_SECURITY_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_sharding module exports
#ifdef THEMIS_SHARDING_EXPORTS
    #define THEMIS_SHARDING_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_SHARDING_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_llm module exports
#ifdef THEMIS_LLM_EXPORTS
    #define THEMIS_LLM_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_LLM_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_content module exports
#ifdef THEMIS_CONTENT_EXPORTS
    #define THEMIS_CONTENT_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_CONTENT_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_timeseries module exports
#ifdef THEMIS_TIMESERIES_EXPORTS
    #define THEMIS_TIMESERIES_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_TIMESERIES_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_network module exports
#ifdef THEMIS_NETWORK_EXPORTS
    #define THEMIS_NETWORK_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_NETWORK_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_geo module exports
#ifdef THEMIS_GEO_EXPORTS
    #define THEMIS_GEO_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_GEO_API THEMIS_IMPORT_HELPER_DLL
#endif

// themis_graph module exports
#ifdef THEMIS_GRAPH_EXPORTS
    #define THEMIS_GRAPH_API THEMIS_EXPORT_HELPER_DLL
#else
    #define THEMIS_GRAPH_API THEMIS_IMPORT_HELPER_DLL
#endif

// Legacy compatibility: When building monolithic core (pre-v1.3.0)
// all APIs default to the current behavior
#ifndef THEMIS_BASE_API
    #define THEMIS_BASE_API
#endif
#ifndef THEMIS_STORAGE_API
    #define THEMIS_STORAGE_API
#endif
#ifndef THEMIS_QUERY_API
    #define THEMIS_QUERY_API
#endif
#ifndef THEMIS_SECURITY_API
    #define THEMIS_SECURITY_API
#endif
#ifndef THEMIS_SHARDING_API
    #define THEMIS_SHARDING_API
#endif
#ifndef THEMIS_LLM_API
    #define THEMIS_LLM_API
#endif
#ifndef THEMIS_CONTENT_API
    #define THEMIS_CONTENT_API
#endif
#ifndef THEMIS_TIMESERIES_API
    #define THEMIS_TIMESERIES_API
#endif
#ifndef THEMIS_NETWORK_API
    #define THEMIS_NETWORK_API
#endif
#ifndef THEMIS_GEO_API
    #define THEMIS_GEO_API
#endif
#ifndef THEMIS_GRAPH_API
    #define THEMIS_GRAPH_API
#endif
