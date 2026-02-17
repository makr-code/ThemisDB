#pragma once

// DLL Export/Import macros for Windows
#ifdef _WIN32
    #if defined(THEMIS_BASE_EXPORTS) || defined(THEMIS_TEST_BUILD)
        #define THEMIS_BASE_API __declspec(dllexport)
    #else
        #define THEMIS_BASE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_BASE_API
#endif
