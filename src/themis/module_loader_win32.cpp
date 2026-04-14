/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_loader_win32.cpp                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 9dde7cb67c  2026-03-12  feat(themis): implement module loader in src/themis/ with... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Windows-specific module loading helpers for the ThemisDB secure module loader.
//
// This file provides:
//   - Zone.Identifier NTFS alternate-data-stream detection (Mark of the Web)
//   - Authenticode (WinVerifyTrust) signature verification
//
// Migrated from src/base/module_loader.cpp to src/themis/ as part of the
// v1.7.0 modular build architecture.

#include "themis/base/module_loader.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32

#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace themis {
namespace modules {

static constexpr DWORD kZoneIdBufferSize   = 256;
static constexpr DWORD kCertNameBufferSize = 256;

// ============================================================================
// Zone.Identifier (Mark of the Web) detection
// ============================================================================

int ModuleLoader::getZoneIdentifier(const std::string& modulePath) const {
    // Read NTFS Zone.Identifier alternate data stream
    std::string adsPath = modulePath + ":Zone.Identifier";
    HANDLE hFile = CreateFileA(adsPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        spdlog::debug("No Zone.Identifier ADS for: {}", modulePath);
        return -1;
    }

    char buffer[kZoneIdBufferSize] = {};
    DWORD bytesRead = 0;
    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);
    CloseHandle(hFile);

    std::string content(buffer, bytesRead);
    // Zone.Identifier format: "[ZoneTransfer]\r\nZoneId=<N>"
    const std::string zoneIdKey = "ZoneId=";
    auto pos = content.find(zoneIdKey);
    if (pos == std::string::npos) {
        return -1;
    }
    try {
        return std::stoi(content.substr(pos + zoneIdKey.size()));
    } catch (...) {
        return -1;
    }
}

bool ModuleLoader::removeZoneIdentifier(const std::string& modulePath) {
    std::string adsPath = modulePath + ":Zone.Identifier";
    if (DeleteFileA(adsPath.c_str())) {
        spdlog::info("Removed Zone.Identifier ADS from: {}", modulePath);
        return true;
    }
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
        // Already absent – treat as success
        return true;
    }
    spdlog::warn("Failed to remove Zone.Identifier from {}: error {}", modulePath, err);
    return false;
}

// ============================================================================
// Authenticode signature verification via WinVerifyTrust
// ============================================================================

bool ModuleLoader::verifyAuthenticodeSignature(const std::string& modulePath,
                                               std::string& signerInfo) const {
    // Convert UTF-8 path to wide string for Windows API
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, modulePath.c_str(),
                                      static_cast<int>(modulePath.size()), nullptr, 0);
    if (wideLen == 0) {
        spdlog::error("verifyAuthenticodeSignature: path conversion failed for: {}",
                      modulePath);
        return false;
    }
    std::wstring widePath(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, modulePath.c_str(),
                        static_cast<int>(modulePath.size()), &widePath[0], wideLen);

    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = widePath.c_str();

    GUID actionId = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA trustData = {};
    trustData.cbStruct            = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice          = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice       = WTD_CHOICE_FILE;
    trustData.pFile               = &fileInfo;
    trustData.dwStateAction       = WTD_STATEACTION_VERIFY;
    trustData.dwProvFlags         = WTD_SAFER_FLAG;

    LONG status = WinVerifyTrust(nullptr, &actionId, &trustData);

    // Retrieve signer subject CN when verification succeeded
    if (status == ERROR_SUCCESS) {
        CRYPT_PROVIDER_DATA* provData = WTHelperProvDataFromStateData(trustData.hWVTStateData);
        if (provData) {
            CRYPT_PROVIDER_SGNR* signer =
                WTHelperGetProvSignerFromChain(provData, 0, FALSE, 0);
            if (signer && signer->pChainContext &&
                signer->pChainContext->rgpChain &&
                signer->pChainContext->rgpChain[0]->rgpElement &&
                signer->pChainContext->rgpChain[0]->cElement > 0) {
                PCCERT_CONTEXT cert =
                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
                if (cert) {
                    char nameBuffer[kCertNameBufferSize] = {};
                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                                       nullptr, nameBuffer, kCertNameBufferSize);
                    signerInfo = nameBuffer;
                }
            }
        }
    }

    // Always close the state handle
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &actionId, &trustData);

    if (status == ERROR_SUCCESS) {
        spdlog::info("Authenticode verification PASSED for: {} (signer: {})",
                     modulePath, signerInfo);
        return true;
    }

    switch (status) {
        case TRUST_E_NOSIGNATURE:
            spdlog::warn("Authenticode: no signature on: {}", modulePath);
            break;
        case TRUST_E_EXPLICIT_DISTRUST:
            spdlog::error("Authenticode: signature explicitly distrusted: {}", modulePath);
            break;
        case TRUST_E_SUBJECT_NOT_TRUSTED:
            spdlog::error("Authenticode: subject not trusted: {}", modulePath);
            break;
        default:
            spdlog::error("Authenticode verification failed (code {}) for: {}",
                          status, modulePath);
            break;
    }
    return false;
}

} // namespace modules
} // namespace themis

#endif // _WIN32
