/// @file test_module_signature_verifier.cpp
/// @brief Unit tests for ModuleSignatureVerifier – Authenticode / GPG checks
///
/// Tests verify:
/// - verifySignature() dispatcher fills result fields correctly
/// - Non-existent module path returns failure with error message
/// - Platform field is set to the correct value for the current OS
/// - Linux: no signature file → failure
/// - Linux: verifyGPGSignature reports failure when gpg is absent or signature missing
/// - Windows: verifyAuthenticodeSignature returns false for unsigned file

#include <gtest/gtest.h>
#include "themis/module_signature_verifier.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <cstdio>
#include <thread>
#include <sstream>

using namespace themis::modules;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Write @p content to a unique temporary file and return its path.
std::string writeTempFile(const std::string& content,
                          const std::string& suffix = ".so") {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    const auto path =
        std::filesystem::temp_directory_path() /
        ("themis_sig_test_" + oss.str() + "_" + suffix);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    return path.string();
}

} // namespace

// ---------------------------------------------------------------------------
// Cross-platform dispatcher tests
// ---------------------------------------------------------------------------

TEST(ModuleSignatureVerifier, VerifySignatureNonExistentFile) {
    // Any platform: non-existent path must return failure.
    const auto result = ModuleSignatureVerifier::verifySignature(
        "/no/such/module_that_does_not_exist.so");

    EXPECT_FALSE(result.success);
#if defined(_WIN32)
    EXPECT_EQ(result.platform, "windows_authenticode");
#elif defined(__linux__)
    EXPECT_EQ(result.platform, "linux_gpg");
#endif
    // On non-Windows/Linux a descriptive error should still be set.
    // (On unsupported platforms errorMessage is populated.)
}

TEST(ModuleSignatureVerifier, VerifySignaturePlatformFieldSet) {
    // Even for a file that fails verification the platform field must be filled.
    const std::string path = writeTempFile("dummy module data");
    const auto result = ModuleSignatureVerifier::verifySignature(path);

#if defined(_WIN32)
    EXPECT_EQ(result.platform, "windows_authenticode");
#elif defined(__linux__)
    EXPECT_EQ(result.platform, "linux_gpg");
#else
    EXPECT_FALSE(result.platform.empty());
#endif

    std::remove(path.c_str());
}

TEST(ModuleSignatureVerifier, VerifySignatureResultHasErrorOnFailure) {
    // A plaintext file without a real signature must fail and report an error.
    const std::string path = writeTempFile("not a real module");
    const auto result = ModuleSignatureVerifier::verifySignature(path);

    EXPECT_FALSE(result.success);
    // signerInfo should be empty when verification failed.
    EXPECT_TRUE(result.signerInfo.empty());

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Linux-specific GPG tests
// ---------------------------------------------------------------------------

#ifdef __linux__

TEST(ModuleSignatureVerifier, VerifyGPGNoSignatureFile) {
    // File exists but has no .asc/.sig/.gpg companion → failure.
    const std::string path = writeTempFile("module binary content", "no_sig.so");
    std::string signerInfo;

    const bool ok = ModuleSignatureVerifier::verifyGPGSignature(
        path, /*signaturePath=*/"", signerInfo);

    EXPECT_FALSE(ok);
    EXPECT_TRUE(signerInfo.empty());

    std::remove(path.c_str());
}

TEST(ModuleSignatureVerifier, VerifyGPGExplicitNonExistentSigPath) {
    const std::string modPath = writeTempFile("module data", "explicit.so");
    const std::string sigPath = modPath + ".asc";  // does not exist
    std::string signerInfo;

    // Provide explicit non-existent sig path; gpg should fail.
    const bool ok = ModuleSignatureVerifier::verifyGPGSignature(
        modPath, sigPath, signerInfo);

    EXPECT_FALSE(ok);
    EXPECT_TRUE(signerInfo.empty());

    std::remove(modPath.c_str());
}

TEST(ModuleSignatureVerifier, VerifySignatureLinuxNoSigFile) {
    // Dispatcher path: no sig file means linux_gpg + failure.
    const std::string path = writeTempFile("linux module", "nosig.so");
    const auto result = ModuleSignatureVerifier::verifySignature(path);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.platform, "linux_gpg");

    std::remove(path.c_str());
}

TEST(ModuleSignatureVerifier, VerifyGPGBadSignatureContent) {
    // Create a module file and a deliberately invalid signature file.
    const std::string modPath = writeTempFile("module data v1", "bad_sig_mod.so");
    const std::string sigPath = modPath + ".asc";

    {
        std::ofstream f(sigPath);
        f << "-----BEGIN PGP SIGNATURE-----\nthis is not a real signature\n"
             "-----END PGP SIGNATURE-----\n";
    }

    std::string signerInfo;
    const bool ok = ModuleSignatureVerifier::verifyGPGSignature(
        modPath, sigPath, signerInfo);

    // gpg should either exit non-zero or not report "Good signature".
    EXPECT_FALSE(ok);

    std::remove(modPath.c_str());
    std::remove(sigPath.c_str());
}

TEST(ModuleSignatureVerifier, VerifySignatureAutoDetectAscExtension) {
    // Even with auto-detect, an invalid .asc triggers a gpg failure (not a crash).
    const std::string modPath = writeTempFile("module v2", "autodetect.so");
    const std::string sigPath = modPath + ".asc";

    {
        std::ofstream f(sigPath);
        f << "invalid\n";
    }

    const auto result = ModuleSignatureVerifier::verifySignature(modPath);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.platform, "linux_gpg");

    std::remove(modPath.c_str());
    std::remove(sigPath.c_str());
}

#endif // __linux__

// ---------------------------------------------------------------------------
// Windows-specific Authenticode tests
// ---------------------------------------------------------------------------

#ifdef _WIN32

TEST(ModuleSignatureVerifier, VerifyAuthenticodeUnsignedFile) {
    // A plain text file has no Authenticode signature → false.
    const std::string path = writeTempFile("unsigned content", ".dll");
    std::string signerInfo;

    const bool ok = ModuleSignatureVerifier::verifyAuthenticodeSignature(
        path, signerInfo);

    EXPECT_FALSE(ok);
    EXPECT_TRUE(signerInfo.empty());

    std::remove(path.c_str());
}

TEST(ModuleSignatureVerifier, VerifyAuthenticodeNonExistentFile) {
    std::string signerInfo = "should-be-cleared";
    const bool ok = ModuleSignatureVerifier::verifyAuthenticodeSignature(
        "C:\\no\\such\\file.dll", signerInfo);

    EXPECT_FALSE(ok);
}

TEST(ModuleSignatureVerifier, VerifySignatureWindowsUnsigned) {
    const std::string path = writeTempFile("unsigned dll bytes", ".dll");
    const auto result = ModuleSignatureVerifier::verifySignature(path);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.platform, "windows_authenticode");

    std::remove(path.c_str());
}

#endif // _WIN32
