/**
 * @file pkcs11_minimal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// =============================================================================
// PKCS#11 Minimal Header - DEVELOPMENT/TESTING ONLY
// =============================================================================
//
// ⚠️  WARNING: This is a MINIMAL subset of PKCS#11 types/constants.
//     It is NOT suitable for production use with real HSM devices.
//
// PURPOSE:
//   - Allow compilation without vendor-specific PKCS#11 headers
//   - Support development and testing with SoftHSM2
//   - Provide basic type definitions for PKCS#11 operations
//
// LIMITATIONS:
//   - Missing many PKCS#11 functions and constants
//   - No vendor-specific extensions
//   - May be incompatible with some HSM devices
//   - Not guaranteed to match vendor header definitions
//
// PRODUCTION REQUIREMENTS:
//   For production HSM deployments, you MUST:
//   1. Use vendor-provided PKCS#11 headers (cryptoki.h, pkcs11.h)
//   2. Install HSM vendor SDK:
//      - Thales Luna: Luna HSM Client SDK
//      - AWS CloudHSM: CloudHSM Client SDK
//      - Utimaco: CryptoServer SDK
//      - nCipher: nShield SDK
//   3. Link against vendor PKCS#11 library
//   4. Replace this header with vendor header in production builds
//
// COMPILE-TIME VALIDATION:
//   The build system should detect and warn about minimal header usage.
//   Set THEMIS_USE_VENDOR_PKCS11=ON to use vendor headers instead.
//
// MIGRATION PATH:
//   1. Install HSM vendor SDK
//   2. Set PKCS11_INCLUDE_DIR to vendor header location
//   3. Build with -DTHEMIS_USE_VENDOR_PKCS11=ON
//   4. Verify against vendor header definitions
//
// See: docs/security/PKCS11_INTEGRATION.md
// =============================================================================

// Minimal PKCS#11 type/constant declarations to avoid external header dependency.
// This is NOT a full PKCS#11 header; only what we need for basic sign/verify.
// For production replace with the vendor's official pkcs11.h.

#include <cstdint>
#include <cstddef>

extern "C" {

typedef uint32_t CK_RV;      // return value
typedef uint32_t CK_SLOT_ID;  // slot identifier
typedef uint32_t CK_SESSION_HANDLE; // session handle
typedef uint32_t CK_OBJECT_HANDLE;  // object handle
typedef uint32_t CK_ULONG;    // unsigned long value

typedef uint8_t CK_BYTE;
typedef uint8_t CK_BBOOL;
typedef CK_BYTE* CK_BYTE_PTR;

// Boolean constants
#define CK_TRUE  ((CK_BBOOL)1)
#define CK_FALSE ((CK_BBOOL)0)

typedef uint32_t CK_OBJECT_CLASS;
typedef uint32_t CK_CERTIFICATE_TYPE;

typedef struct CK_MECHANISM {
    uint32_t mechanism; // Mechanism type
    void*    pParameter;
    size_t   ulParameterLen;
} CK_MECHANISM;

typedef struct CK_ATTRIBUTE {
    uint32_t type;       // Attribute type (e.g. CKA_CLASS)
    void*    pValue;     // Pointer to value buffer
    size_t   ulValueLen; // Value length (in/out)
} CK_ATTRIBUTE; 

// Mechanism constants (subset)
#define CKM_RSA_PKCS 0x00000001U
#define CKM_RSA_PKCS_OAEP 0x00000009U
#define CKM_SHA256_RSA_PKCS 0x00000040U
#define CKM_ECDSA 0x00001041U
#define CKM_RSA_PKCS_KEY_PAIR_GEN 0x00000000U
#define CKM_SHA_1 0x00000220U
#define CKM_SHA256 0x00000250U

// MGF (Mask Generation Function) types for RSA-OAEP
typedef uint32_t CK_RSA_PKCS_MGF_TYPE;
#define CKG_MGF1_SHA1   0x00000001U
#define CKG_MGF1_SHA256 0x00000002U
#define CKG_MGF1_SHA384 0x00000003U
#define CKG_MGF1_SHA512 0x00000004U

// OAEP source type
typedef uint32_t CK_RSA_PKCS_OAEP_SOURCE_TYPE;
#define CKZ_DATA_SPECIFIED 0x00000001U

// RSA-OAEP mechanism parameters (PKCS#11 v2.20 §12.1.7)
typedef struct CK_RSA_PKCS_OAEP_PARAMS {
    uint32_t               hashAlg;       // Hash algorithm (e.g. CKM_SHA256)
    CK_RSA_PKCS_MGF_TYPE   mgf;           // Mask generation function
    CK_RSA_PKCS_OAEP_SOURCE_TYPE source;  // Source of encoding parameter
    void*                  pSourceData;   // Encoding parameter (NULL for CKZ_DATA_SPECIFIED with no label)
    CK_ULONG               ulSourceDataLen; // Length of encoding parameter
} CK_RSA_PKCS_OAEP_PARAMS;

// Object classes (subset)
#define CKO_PRIVATE_KEY 0x00000003U
#define CKO_PUBLIC_KEY  0x00000002U
#define CKO_CERTIFICATE 0x00000001U

// Certificate types
#define CKC_X_509 0x00000000U

// Attribute types (subset)
#define CKA_CLASS       0x00000000U
#define CKA_LABEL       0x00000003U
#define CKA_VALUE       0x00000011U
#define CKA_TOKEN       0x00000001U
#define CKA_PRIVATE     0x00000002U
#define CKA_SENSITIVE   0x00000103U
#define CKA_SIGN        0x00000108U
#define CKA_VERIFY      0x0000010AU
#define CKA_EXTRACTABLE 0x00000162U
#define CKA_MODULUS_BITS 0x00000121U
#define CKA_PUBLIC_EXPONENT 0x00000122U
#define CKA_CERTIFICATE_TYPE 0x00000080U

// Return values (subset)
#define CKR_OK                      0x00000000U
#define CKR_GENERAL_ERROR           0x00000005U
#define CKR_ARGUMENTS_BAD           0x00000007U
#define CKR_DEVICE_ERROR            0x00000030U
#define CKR_PIN_INCORRECT           0x000000A0U
#define CKR_SIGNATURE_INVALID       0x000000C0U
#define CKR_USER_ALREADY_LOGGED_IN  0x00000100U
#define CKR_USER_NOT_LOGGED_IN      0x00000101U

// Session flags (subset)
#define CKF_SERIAL_SESSION 0x00000004U

// Token/slot flags (subset)
#define CKF_TOKEN_PRESENT  0x00000001U
#define CKF_LOGIN_REQUIRED 0x00000004U

// Boolean values
#define CK_TRUE  ((CK_BBOOL)1)
#define CK_FALSE ((CK_BBOOL)0)

// User types
#define CKU_USER 1U

// Token information structure (minimal subset - label-based slot selection)
// NOTE: Field sizes follow PKCS#11 v2.20 §9.2. Labels are blank-padded UTF-8,
//       NOT null-terminated. Use the full 32 bytes for comparison.
typedef struct CK_TOKEN_INFO {
    uint8_t  label[32];           // Blank-padded token label (UTF-8, no null terminator)
    uint8_t  manufacturerID[32];  // Blank-padded manufacturer identifier
    uint8_t  model[16];           // Blank-padded model
    uint8_t  serialNumber[16];    // Blank-padded serial number
    uint32_t flags;               // Bit flags (CKF_TOKEN_PRESENT, CKF_LOGIN_REQUIRED, …)
    CK_ULONG ulMaxSessionCount;
    CK_ULONG ulSessionCount;
    CK_ULONG ulMaxRwSessionCount;
    CK_ULONG ulRwSessionCount;
    CK_ULONG ulMaxPinLen;
    CK_ULONG ulMinPinLen;
    CK_ULONG ulTotalPublicMemory;
    CK_ULONG ulFreePublicMemory;
    CK_ULONG ulTotalPrivateMemory;
    CK_ULONG ulFreePrivateMemory;
    struct { uint8_t major; uint8_t minor; } hardwareVersion;
    struct { uint8_t major; uint8_t minor; } firmwareVersion;
    uint8_t  utcTime[16];         // UTC time as "YYYYMMDDhhmmssxx" (not null-terminated)
} CK_TOKEN_INFO;
typedef CK_TOKEN_INFO* CK_TOKEN_INFO_PTR;

// Function list forward declaration
struct CK_FUNCTION_LIST;
typedef CK_FUNCTION_LIST* CK_FUNCTION_LIST_PTR;

typedef CK_RV (*CK_C_GetFunctionList)(CK_FUNCTION_LIST_PTR*);

// Function list structure (subset of pointers)
struct CK_FUNCTION_LIST {
    CK_RV (*C_Initialize)(void*);
    CK_RV (*C_Finalize)(void*);
    CK_RV (*C_GetSlotList)(uint8_t, CK_SLOT_ID*, uint32_t*);
    CK_RV (*C_GetTokenInfo)(CK_SLOT_ID, CK_TOKEN_INFO*);
    CK_RV (*C_OpenSession)(CK_SLOT_ID, uint32_t, void*, void*, CK_SESSION_HANDLE*);
    CK_RV (*C_CloseSession)(CK_SESSION_HANDLE);
    CK_RV (*C_Login)(CK_SESSION_HANDLE, uint32_t, CK_BYTE_PTR, uint32_t);
    CK_RV (*C_Logout)(CK_SESSION_HANDLE);
    CK_RV (*C_FindObjectsInit)(CK_SESSION_HANDLE, CK_ATTRIBUTE*, uint32_t);
    CK_RV (*C_FindObjects)(CK_SESSION_HANDLE, CK_OBJECT_HANDLE*, uint32_t, uint32_t*);
    CK_RV (*C_FindObjectsFinal)(CK_SESSION_HANDLE);
    CK_RV (*C_SignInit)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_OBJECT_HANDLE);
    CK_RV (*C_Sign)(CK_SESSION_HANDLE, CK_BYTE_PTR, uint32_t, CK_BYTE_PTR, uint32_t*);
    CK_RV (*C_VerifyInit)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_OBJECT_HANDLE);
    CK_RV (*C_Verify)(CK_SESSION_HANDLE, CK_BYTE_PTR, uint32_t, CK_BYTE_PTR, uint32_t);
    CK_RV (*C_EncryptInit)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_OBJECT_HANDLE);
    CK_RV (*C_Encrypt)(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG*);
    CK_RV (*C_DecryptInit)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_OBJECT_HANDLE);
    CK_RV (*C_Decrypt)(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG*);
    CK_RV (*C_GetAttributeValue)(CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE*, uint32_t);
    CK_RV (*C_GenerateKeyPair)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_ATTRIBUTE*, uint32_t, CK_ATTRIBUTE*, uint32_t, CK_OBJECT_HANDLE*, CK_OBJECT_HANDLE*);
    CK_RV (*C_CreateObject)(CK_SESSION_HANDLE, CK_ATTRIBUTE*, uint32_t, CK_OBJECT_HANDLE*);
};

} // extern "C"

// =============================================================================
// Compile-Time Validation
// =============================================================================
//
// Verify critical PKCS#11 constants at compile time to catch header mismatches
//
#ifdef THEMIS_ENABLE_HSM_REAL
    // These static assertions will fail if vendor headers define different values
    // This is a safety check to detect incompatibilities early
    
    static_assert(CKR_OK == 0x00000000U, 
        "PKCS#11 CKR_OK mismatch - vendor header may be incompatible");
    
    static_assert(CKR_GENERAL_ERROR == 0x00000005U,
        "PKCS#11 CKR_GENERAL_ERROR mismatch - vendor header may be incompatible");
    
    static_assert(CKM_RSA_PKCS == 0x00000001U,
        "PKCS#11 CKM_RSA_PKCS mismatch - vendor header may be incompatible");
    
    static_assert(CKO_PRIVATE_KEY == 0x00000003U,
        "PKCS#11 CKO_PRIVATE_KEY mismatch - vendor header may be incompatible");
    
    // Warning: Using minimal PKCS#11 header in production build
    #ifndef THEMIS_USE_VENDOR_PKCS11
        #ifdef _MSC_VER
            #pragma message("Warning: Using pkcs11_minimal.h with real HSM support. Consider using vendor PKCS#11 headers for production.")
        #else
            #warning "Using pkcs11_minimal.h with real HSM support. Consider using vendor PKCS#11 headers for production."
        #endif
    #endif
#endif
// =============================================================================
