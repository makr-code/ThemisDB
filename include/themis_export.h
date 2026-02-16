#pragma once

// DLL Export/Import macros for Windows
#ifdef _WIN32
    #ifdef THEMIS_BASE_EXPORTS
        #define THEMIS_BASE_API __declspec(dllexport)
    #else
        #define THEMIS_BASE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_BASE_API
#endif
