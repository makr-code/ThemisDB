/**
 * @file themis_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// DLL Export/Import macros for Windows.
// THEMIS_BASE_EXPORTS  — defined by the library CMake target (THEMIS_BASE_EXPORTS=1)
// THEMIS_TEST_BUILD    — test-mode behavior. In monolithic builds it also enables exports;
//                        modular builds use the owning module's export macro instead.
#ifndef THEMIS_BASE_API
#ifdef _WIN32
    #if defined(THEMIS_BASE_EXPORTS) || (defined(THEMIS_TEST_BUILD) && !defined(THEMIS_MODULAR_BUILD))
        #define THEMIS_BASE_API __declspec(dllexport)
    #else
        #define THEMIS_BASE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_BASE_API
#endif
#endif
