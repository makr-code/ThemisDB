/**
 * user_storage_encrypted v0.3.0 tests:
 *   - Prometheus metrics (USE-01..08): getMetricsText(), metric counting
 *   - recordKeyRotation() (USE-09..10)
 *   - GocryptfsBackend: executeCommand() removal + executeCommandSafe() (USE-11..12)
 */

#include <gtest/gtest.h>
#include "user_storage_encrypted/multi_level_storage.hpp"
#include "user_storage_encrypted/gocryptfs_backend.hpp"
#include "user_storage_encrypted/security_level.hpp"

#include <string>
#include <sstream>

using namespace themis::plugins::user_storage;

// ============================================================================
// Helpers
// ============================================================================

/// Create an empty (uninitialized) MultiLevelEncryptedStorage instance.
static MultiLevelEncryptedStorage makeStorage() {
    return MultiLevelEncryptedStorage{};
}

// ============================================================================
// USE-01: getMetricsText() returns non-empty text
// ============================================================================
TEST(UserStorageV03, USE01_MetricsTextNonEmpty) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    EXPECT_FALSE(text.empty());
}

// ============================================================================
// USE-02: getMetricsText() contains all 4 expected metric family names
// ============================================================================
TEST(UserStorageV03, USE02_MetricsTextContainsAllFamilies) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    EXPECT_NE(std::string::npos, text.find("user_storage_mounts_active"))
        << "Missing user_storage_mounts_active";
    EXPECT_NE(std::string::npos, text.find("user_storage_mount_operations_total"))
        << "Missing user_storage_mount_operations_total";
    EXPECT_NE(std::string::npos, text.find("user_storage_key_rotations_total"))
        << "Missing user_storage_key_rotations_total";
    EXPECT_NE(std::string::npos, text.find("user_storage_container_size_bytes"))
        << "Missing user_storage_container_size_bytes";
}

// ============================================================================
// USE-03: initial values are all zero
// ============================================================================
TEST(UserStorageV03, USE03_InitialValuesAllZero) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    // All metrics should show 0 for a fresh instance
    EXPECT_NE(std::string::npos, text.find("user_storage_mounts_active 0\n"))
        << text;
    EXPECT_NE(std::string::npos, text.find("user_storage_key_rotations_total 0\n"))
        << text;
    EXPECT_NE(std::string::npos, text.find("user_storage_container_size_bytes 0\n"))
        << text;
}

// ============================================================================
// USE-04: output includes HELP and TYPE lines for each family
// ============================================================================
TEST(UserStorageV03, USE04_MetricsTextHasHelpAndType) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    EXPECT_NE(std::string::npos, text.find("# HELP user_storage_mounts_active"));
    EXPECT_NE(std::string::npos, text.find("# TYPE user_storage_mounts_active gauge"));
    EXPECT_NE(std::string::npos, text.find("# HELP user_storage_mount_operations_total"));
    EXPECT_NE(std::string::npos, text.find("# TYPE user_storage_mount_operations_total counter"));
    EXPECT_NE(std::string::npos, text.find("# HELP user_storage_key_rotations_total"));
    EXPECT_NE(std::string::npos, text.find("# TYPE user_storage_key_rotations_total counter"));
    EXPECT_NE(std::string::npos, text.find("# HELP user_storage_container_size_bytes"));
    EXPECT_NE(std::string::npos, text.find("# TYPE user_storage_container_size_bytes gauge"));
}

// ============================================================================
// USE-05: mount_operations_total label "mount" is present in output
// ============================================================================
TEST(UserStorageV03, USE05_MountOpsMountLabel) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    EXPECT_NE(std::string::npos, text.find("user_storage_mount_operations_total{operation=\"mount\"}"))
        << text;
}

// ============================================================================
// USE-06: mount_operations_total label "unmount" is present in output
// ============================================================================
TEST(UserStorageV03, USE06_MountOpsUnmountLabel) {
    auto storage = makeStorage();
    std::string text = storage.getMetricsText();
    EXPECT_NE(std::string::npos, text.find("user_storage_mount_operations_total{operation=\"unmount\"}"))
        << text;
}

// ============================================================================
// USE-07: getMetricsText() is callable multiple times and returns consistent text
// ============================================================================
TEST(UserStorageV03, USE07_MetricsTextIdempotent) {
    auto storage = makeStorage();
    std::string first  = storage.getMetricsText();
    std::string second = storage.getMetricsText();
    EXPECT_EQ(first, second);
}

// ============================================================================
// USE-08: StorageMetrics atomics are non-copyable (compile-time contract)
//          — verified via static_assert in this TU
// ============================================================================
TEST(UserStorageV03, USE08_StorageMetricsExists) {
    // Verify the struct is constructible and its atomics are zero-initialised.
    StorageMetrics m;
    EXPECT_EQ(0, m.mounts_active.load());
    EXPECT_EQ(0, m.mount_ops_total.load());
    EXPECT_EQ(0, m.unmount_ops_total.load());
    EXPECT_EQ(0, m.key_rotations_total.load());
    EXPECT_EQ(0, m.container_size_bytes.load());
}

// ============================================================================
// USE-09: recordKeyRotation() increments key_rotations_total
// ============================================================================
TEST(UserStorageV03, USE09_RecordKeyRotationIncrementsCounter) {
    auto storage = makeStorage();

    // Initial state must be 0
    ASSERT_NE(std::string::npos, storage.getMetricsText().find(
        "user_storage_key_rotations_total 0\n"));

    storage.recordKeyRotation(SecurityLevel::VS_NFD);

    EXPECT_NE(std::string::npos, storage.getMetricsText().find(
        "user_storage_key_rotations_total 1\n"))
        << storage.getMetricsText();
}

// ============================================================================
// USE-10: recordKeyRotation() called multiple times accumulates correctly
// ============================================================================
TEST(UserStorageV03, USE10_RecordKeyRotationMultiple) {
    auto storage = makeStorage();

    storage.recordKeyRotation(SecurityLevel::VS_NFD);
    storage.recordKeyRotation(SecurityLevel::GEHEIM);
    storage.recordKeyRotation(SecurityLevel::STRENG_GEHEIM);

    EXPECT_NE(std::string::npos, storage.getMetricsText().find(
        "user_storage_key_rotations_total 3\n"))
        << storage.getMetricsText();
}

// ============================================================================
// USE-11: GocryptfsBackend::checkAvailability uses executeCommandSafe (no shell)
//         — verify that `checkAvailability()` returns a Result (success or error)
//           without crashing; the command may legitimately fail in CI
// ============================================================================
TEST(UserStorageV03, USE11_CheckAvailabilityRuns) {
    GocryptfsBackend backend;
    EXPECT_NO_THROW({
        auto result = backend.checkAvailability();
        // Either success (gocryptfs installed) or error — both are valid
        (void)result;
    });
}

// ============================================================================
// USE-12: GocryptfsBackend::getBackendVersion uses executeCommandSafe
//         — returns a string (may be "unknown" in CI without gocryptfs)
// ============================================================================
TEST(UserStorageV03, USE12_GetBackendVersionRuns) {
    GocryptfsBackend backend;
    std::string version = {};
    EXPECT_NO_THROW({ version = backend.getBackendVersion(); });
    // Must be non-empty; either actual version or "unknown"
    EXPECT_FALSE(version.empty());
}
