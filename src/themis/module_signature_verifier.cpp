/**
 * @file module_signature_verifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Authenticode (Windows) and GPG (Linux) signature verification for ThemisDB modules.
// Uses WinVerifyTrust on Windows and posix_spawn (no shell) on Linux.
//
// Roadmap item: Phase 4 – Authenticode (Windows) and GPG (Linux) signature verification
// Issue: #2473

#include "themis/module_signature_verifier.h"

#include <filesystem>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#   include <windows.h>
#   include <wintrust.h>
#   include <softpub.h>
#   include <wincrypt.h>
#   pragma comment(lib, "wintrust.lib")
#   pragma comment(lib, "crypt32.lib")
#else
#   include <spawn.h>
#   include <unistd.h>
#   include <sys/wait.h>
#   include <fcntl.h>
#endif

namespace themis {
namespace modules {

// ============================================================================
// Cross-platform dispatcher
// ============================================================================

ModuleSignatureVerificationResult ModuleSignatureVerifier::verifySignature(
    const std::string& modulePath,
    [[maybe_unused]] const std::string& signaturePath)
{
    ModuleSignatureVerificationResult result;

#ifdef _WIN32
    // unused on Windows
    result.platform = "windows_authenticode";
    result.success  = verifyAuthenticodeSignature(modulePath, result.signerInfo);
    if (!result.success && result.signerInfo.empty()) {
        result.errorMessage = "Authenticode verification failed for: " + modulePath;
    }
#elif defined(__linux__)
    result.platform = "linux_gpg";
    result.success  = verifyGPGSignature(modulePath, signaturePath, result.signerInfo);
    if (!result.success && result.errorMessage.empty()) {
        result.errorMessage = "GPG signature verification failed for: " + modulePath;
    }
#else
    result.errorMessage = "Signature verification not supported on this platform";
    spdlog::warn("ModuleSignatureVerifier::verifySignature: {}", result.errorMessage);
#endif

    return result;
}

// ============================================================================
// Windows – Authenticode via WinVerifyTrust
// ============================================================================

#ifdef _WIN32

static constexpr DWORD kCertNameBufSize = 256;

bool ModuleSignatureVerifier::verifyAuthenticodeSignature(
    const std::string& modulePath,
    std::string& signerInfo)
{
    signerInfo.clear();

    // Convert UTF-8 path to wide string for the Windows API.
    int wideLen = MultiByteToWideChar(CP_UTF8, 0,
                                      modulePath.c_str(),
                                      static_cast<int>(modulePath.size()),
                                      nullptr, 0);
    if (wideLen == 0) {
        spdlog::error("ModuleSignatureVerifier::verifyAuthenticodeSignature: "
                      "path conversion failed for: {}", modulePath);
        return false;
    }
    std::wstring widePath(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
                        modulePath.c_str(),
                        static_cast<int>(modulePath.size()),
                        &widePath[0], wideLen);

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

    // Extract signer CN when verification succeeded.
    if (status == ERROR_SUCCESS) {
        CRYPT_PROVIDER_DATA* provData =
            WTHelperProvDataFromStateData(trustData.hWVTStateData);
        if (provData) {
            CRYPT_PROVIDER_SGNR* signer =
                WTHelperGetProvSignerFromChain(provData, 0, FALSE, 0);
            if (signer &&
                signer->pChainContext &&
                signer->pChainContext->rgpChain &&
                signer->pChainContext->rgpChain[0]->cElement > 0) {
                PCCERT_CONTEXT cert =
                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
                if (cert) {
                    char nameBuf[kCertNameBufSize] = {};
                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                                       nullptr, nameBuf, kCertNameBufSize);
                    signerInfo = nameBuf;
                }
            }
        }
    }

    // Always close the state handle.
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &actionId, &trustData);

    if (status == ERROR_SUCCESS) {
        spdlog::info("ModuleSignatureVerifier: Authenticode OK for '{}' (signer: {})",
                     modulePath, signerInfo);
        return true;
    }

    switch (status) {
        case TRUST_E_NOSIGNATURE:
            spdlog::warn("ModuleSignatureVerifier: no Authenticode signature on '{}'",
                         modulePath);
            break;
        case TRUST_E_EXPLICIT_DISTRUST:
            spdlog::error("ModuleSignatureVerifier: Authenticode explicitly distrusted: '{}'",
                          modulePath);
            break;
        case TRUST_E_SUBJECT_NOT_TRUSTED:
            spdlog::error("ModuleSignatureVerifier: Authenticode subject not trusted: '{}'",
                          modulePath);
            break;
        default:
            spdlog::error("ModuleSignatureVerifier: Authenticode failed (code {}) for '{}'",
                          status, modulePath);
            break;
    }
    return false;
}

#endif // _WIN32

// ============================================================================
// Linux – GPG via posix_spawn (no shell, no injection risk)
// ============================================================================

#ifdef __linux__

static constexpr std::size_t kGPGReadBufSize = 256;
static const char* const kGpgBin = "/usr/bin/gpg";

bool ModuleSignatureVerifier::verifyGPGSignature(
    const std::string& modulePath,
    const std::string& signaturePath,
    std::string& signerInfo)
{
    signerInfo.clear();

    // Auto-detect detached signature file.
    std::string sigFile = signaturePath;
    if (sigFile.empty()) {
        for (const auto& ext : {".asc", ".sig", ".gpg"}) {
            std::string candidate = modulePath + ext;
            if (std::filesystem::exists(candidate)) {
                sigFile = candidate;
                break;
            }
        }
    }

    if (sigFile.empty()) {
        spdlog::warn("ModuleSignatureVerifier::verifyGPGSignature: "
                     "no signature file found for '{}'", modulePath);
        return false;
    }

    // Build argv without going through a shell.
    // argv must be char* (not const char*) per POSIX, but posix_spawn does not
    // modify the strings.
    char* const argv[] = {
        const_cast<char*>("gpg"),
        const_cast<char*>("--verify"),
        const_cast<char*>(sigFile.c_str()),
        const_cast<char*>(modulePath.c_str()),
        nullptr
    };

    // Create a pipe to capture combined stdout + stderr.
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        spdlog::error("ModuleSignatureVerifier::verifyGPGSignature: "
                      "pipe() failed for '{}'", modulePath);
        return false;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    // Redirect both stdout and stderr into the write end of the pipe.
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);

    pid_t pid;
    int spawnRet = posix_spawn(&pid, kGpgBin, &actions, nullptr, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]); // parent closes the write end

    if (spawnRet != 0) {
        close(pipefd[0]);
        spdlog::error("ModuleSignatureVerifier::verifyGPGSignature: "
                      "posix_spawn failed for '{}'", modulePath);
        return false;
    }

    // Drain the pipe.
    char buf[kGPGReadBufSize];
    std::string output = {};
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        output += buf;
    }
    close(pipefd[0]);

    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    int exitCode = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;

    if (exitCode == 0 && output.find("Good signature") != std::string::npos) {
        // Extract signer identity from 'Good signature from "..."' line.
        const std::string marker = "from \"";
        std::size_t pos = output.find(marker);
        if (pos != std::string::npos) {
            std::size_t end = output.find('"', pos + marker.size());
            if (end != std::string::npos) {
                signerInfo = output.substr(pos + marker.size(),
                                           end - pos - marker.size());
            }
        }
        spdlog::info("ModuleSignatureVerifier: GPG OK for '{}' (signer: {})",
                     modulePath, signerInfo);
        return true;
    }

    spdlog::warn("ModuleSignatureVerifier: GPG verification failed for '{}': {}",
                 modulePath, output);
    return false;
}

#endif // __linux__

} // namespace modules
} // namespace themis

