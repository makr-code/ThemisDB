/**
 * @file export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Export Macros for Modular Architecture
// This file provides DLL export/import macros for all ThemisDB modules
// Windows requires explicit __declspec(dllexport/dllimport) for shared libraries

#pragma once

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
#ifndef THEMIS_BASE_API
#ifdef THEMIS_BASE_EXPORTS
    #define THEMIS_BASE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_BASE_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_storage - Storage engine and indexes
#ifndef THEMIS_STORAGE_API
#ifdef THEMIS_STORAGE_EXPORTS
    #define THEMIS_STORAGE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_STORAGE_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_query - Query engine and AQL
#ifndef THEMIS_QUERY_API
#if defined(THEMIS_QUERY_STATIC)
    #define THEMIS_QUERY_API
#elif defined(THEMIS_QUERY_EXPORTS)
    #define THEMIS_QUERY_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_QUERY_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_security - Encryption, PKI, RBAC
#ifndef THEMIS_SECURITY_API
#ifdef THEMIS_SECURITY_EXPORTS
    #define THEMIS_SECURITY_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_SECURITY_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_network - HTTP/gRPC servers
#ifndef THEMIS_NETWORK_API
#ifdef THEMIS_NETWORK_EXPORTS
    #define THEMIS_NETWORK_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_NETWORK_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_transaction - Transaction management
#ifndef THEMIS_TRANSACTION_API
#ifdef THEMIS_TRANSACTION_EXPORTS
    #define THEMIS_TRANSACTION_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_TRANSACTION_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_sharding - Distributed system (optional)
#ifndef THEMIS_SHARDING_API
#ifdef THEMIS_SHARDING_EXPORTS
    #define THEMIS_SHARDING_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_SHARDING_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_llm - LLM integration (optional)
#ifndef THEMIS_LLM_API
#ifdef THEMIS_LLM_EXPORTS
    #define THEMIS_LLM_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_LLM_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_content - Content processors (optional)
#ifndef THEMIS_CONTENT_API
#ifdef THEMIS_CONTENT_EXPORTS
    #define THEMIS_CONTENT_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_CONTENT_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_timeseries - Time-series support (optional)
#ifndef THEMIS_TIMESERIES_API
#ifdef THEMIS_TIMESERIES_EXPORTS
    #define THEMIS_TIMESERIES_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_TIMESERIES_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_graph - Graph analytics (optional)
#ifndef THEMIS_GRAPH_API
#ifdef THEMIS_GRAPH_EXPORTS
    #define THEMIS_GRAPH_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_GRAPH_API THEMIS_IMPORT_MACRO
#endif
#endif

// themis_geo - Geospatial features (optional)
#ifndef THEMIS_GEO_API
#ifdef THEMIS_GEO_EXPORTS
    #define THEMIS_GEO_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_GEO_API THEMIS_IMPORT_MACRO
#endif
#endif

// Legacy compatibility: themis_core for monolithic builds
#ifdef THEMIS_CORE_EXPORTS
    #define THEMIS_CORE_API THEMIS_EXPORT_MACRO
#else
    #define THEMIS_CORE_API THEMIS_IMPORT_MACRO
#endif
