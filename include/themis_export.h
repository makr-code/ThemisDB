/*
 * ThemisDB | File: themis_export.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
