/*
 * ThemisDB | File: themis_export.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 22
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #5313 Remediate code quality and security gaps in whisper module (2026-05-20T11:50:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

// DLL Export/Import macros for Windows
#ifndef THEMIS_BASE_API
#ifdef _WIN32
    #if defined(THEMIS_BASE_EXPORTS)
        #define THEMIS_BASE_API __declspec(dllexport)
    #else
        #define THEMIS_BASE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_BASE_API
#endif
#endif
