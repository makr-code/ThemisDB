// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_user_storage_encrypted_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the user_storage_encrypted module.
 *
 * Test IDs: USE-01 through USE-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/user_storage_encrypted/user_storage_encrypted_api_contract.h
 * @see src/user_storage_encrypted/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "user_storage_encrypted/user_storage_encrypted_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace user_storage_encrypted {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// USE-01 — Error code uniqueness
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE01_ErrorCodeUniqueness) {
    std::set<int32_t> seen = {};

    const int32_t codes[] = {
        static_cast<int32_t>(UserStorageEncryptedError::kMountFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kUnmountFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kKeyDerivationFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kRotationFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kBackendUnavailable),
        static_cast<int32_t>(UserStorageEncryptedError::kInvalidPath),
        static_cast<int32_t>(UserStorageEncryptedError::kPermissionDenied),
        static_cast<int32_t>(UserStorageEncryptedError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 8u);
    (void)kSeed;
}

// ============================================================================
// USE-02 — Error code range [8600, 8699]
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(UserStorageEncryptedError::kMountFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kUnmountFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kKeyDerivationFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kRotationFailed),
        static_cast<int32_t>(UserStorageEncryptedError::kBackendUnavailable),
        static_cast<int32_t>(UserStorageEncryptedError::kInvalidPath),
        static_cast<int32_t>(UserStorageEncryptedError::kPermissionDenied),
        static_cast<int32_t>(UserStorageEncryptedError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8600) << "Code " << c << " below reserved base 8600";
        EXPECT_LE(c, 8699) << "Code " << c << " above reserved max 8699";
    }
}

// ============================================================================
// USE-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE03_SwitchDispatch) {
    auto describe = [](UserStorageEncryptedError e) -> const char* {
        switch (e) {
            case UserStorageEncryptedError::kSuccess:             return "success";
            case UserStorageEncryptedError::kMountFailed:         return "mount_failed";
            case UserStorageEncryptedError::kUnmountFailed:       return "unmount_failed";
            case UserStorageEncryptedError::kKeyDerivationFailed: return "key_derivation_failed";
            case UserStorageEncryptedError::kRotationFailed:      return "rotation_failed";
            case UserStorageEncryptedError::kBackendUnavailable:  return "backend_unavailable";
            case UserStorageEncryptedError::kInvalidPath:         return "invalid_path";
            case UserStorageEncryptedError::kPermissionDenied:    return "permission_denied";
            case UserStorageEncryptedError::kInternalError:       return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(UserStorageEncryptedError::kSuccess),             "success");
    EXPECT_STREQ(describe(UserStorageEncryptedError::kMountFailed),         "mount_failed");
    EXPECT_STREQ(describe(UserStorageEncryptedError::kBackendUnavailable),  "backend_unavailable");
    EXPECT_STREQ(describe(UserStorageEncryptedError::kInternalError),       "internal_error");
}

// ============================================================================
// USE-04 — EncryptedMountDescriptor default values
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE04_EncryptedMountDescriptorDefaults) {
    EncryptedMountDescriptor desc;
    EXPECT_TRUE(desc.container_path.empty());
    EXPECT_TRUE(desc.mount_point.empty());
    EXPECT_FALSE(desc.read_only);
    EXPECT_EQ(desc.timeout, kMaxMountTimeout);
}

// ============================================================================
// USE-05 — KeyDerivationRequest default values
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE05_KeyDerivationRequestDefaults) {
    KeyDerivationRequest req;
    EXPECT_TRUE(req.passphrase.empty());
    EXPECT_TRUE(req.salt.empty());
    EXPECT_EQ(req.iterations, 100'000u);
}

// ============================================================================
// USE-06 — Copy semantics for EncryptedMountDescriptor
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE06_EncryptedMountDescriptorCopy) {
    EncryptedMountDescriptor src;
    src.container_path = "/data/enc/user-42";
    src.mount_point    = "/mnt/user-42";
    src.read_only      = false;

    EncryptedMountDescriptor copy = src;
    EXPECT_EQ(copy.container_path, src.container_path);
    EXPECT_EQ(copy.mount_point,    src.mount_point);
    EXPECT_EQ(copy.read_only,      src.read_only);
}

// ============================================================================
// USE-07 — Move semantics for KeyDerivationRequest
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE07_KeyDerivationRequestMove) {
    KeyDerivationRequest src;
    src.passphrase = "super-secret-passphrase";
    src.iterations = 200'000;

    KeyDerivationRequest moved = std::move(src);
    EXPECT_EQ(moved.passphrase, "super-secret-passphrase");
    EXPECT_EQ(moved.iterations, 200'000u);
}

// ============================================================================
// USE-08 — isUserStorageEncryptedFailClosed predicate
// ============================================================================

TEST(UserStorageEncryptedContractHardening, USE08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kBackendUnavailable));
    EXPECT_TRUE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kKeyDerivationFailed));
    EXPECT_TRUE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kInternalError));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kSuccess));
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kMountFailed));
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kUnmountFailed));
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kRotationFailed));
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kInvalidPath));
    EXPECT_FALSE(isUserStorageEncryptedFailClosed(UserStorageEncryptedError::kPermissionDenied));
}

} // namespace test
} // namespace user_storage_encrypted
} // namespace themis
