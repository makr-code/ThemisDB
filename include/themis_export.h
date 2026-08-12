/**
 * @file themis_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// DLL Export/Import macros for Windows.
// THEMIS_BASE_EXPORTS  — defined by the library CMake target (THEMIS_BASE_EXPORTS=1)
// THEMIS_TEST_BUILD    — defined by test executables (target_compile_definitions THEMIS_TEST_BUILD=1)
//                        Test builds use dllexport so that test executables can link against
//                        internal symbols without a separate import library.
#ifndef THEMIS_BASE_API
#ifdef _WIN32
    #if defined(THEMIS_BASE_EXPORTS) || defined(THEMIS_TEST_BUILD)
        #define THEMIS_BASE_API __declspec(dllexport)
    #else
        #define THEMIS_BASE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_BASE_API
#endif
#endif
