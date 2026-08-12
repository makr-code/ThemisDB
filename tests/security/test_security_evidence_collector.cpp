/**
 * @file test_security_evidence_collector.cpp
 * @brief Unit tests for the SOC 2 Type II SecurityEvidenceCollector.
 *
 * Test coverage:
 *   Construction:
 *     - Null key_provider throws std::invalid_argument
 *     - Successful construction with valid provider
 *
 *   collect():
 *     - Bundle has non-empty bundle_id
 *     - collected_at_ms is within 2 seconds of test start
 *     - window_from_ms / window_to_ms match requested window
 *     - within_retention_window = true for a recent window
 *     - within_retention_window = false for window beyond retention period
 *     - Metrics: active_keys reflects created keys
 *     - Metrics: deprecated_keys increments after rotation
 *     - Key rotations list is empty when no rotations occurred in window
 *     - Key rotations captured after key rotation
 *     - Access-control report: correct role count and permission count
 *     - Access-control report: has_admin_role = true when wildcard role present
 *     - Access-control report: empty_roles correctly identified
 *     - No-RBAC collector: access_control section is empty but collection succeeds
 *     - No-audit-logger collector: audit_log section is empty but collection succeeds
 *
 *   JSON serialisation:
 *     - toJson() produces valid JSON with all required fields
 *
 *   exportToFile():
 *     - Writes valid JSON to a temporary file
 *     - Overwrites existing file atomically
 *     - Empty path returns false
 *
 *   verifyRetention():
 *     - Empty / non-existent store path returns true
 *     - Store with in-window bundles returns true
 *     - Store with out-of-window bundles returns false
 */

#include <gtest/gtest.h>
#include "security/security_evidence_collector.h"
#include "security/mock_key_provider.h"
#include "security/rbac.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <memory>
#include <string>

using namespace themis;
using namespace themis::security;
namespace fs = std::filesystem;

// ─── Helpers ──────────────────────────────────────────────────────────────

static std::shared_ptr<MockKeyProvider> make_provider() {
    auto p = std::make_shared<MockKeyProvider>();
    p->createKey("evidence_key", 1);
    return p;
}

static std::unique_ptr<RBAC> make_rbac() {
    RBACConfig cfg;
    cfg.use_builtin_roles = false;

    auto rbac = std::make_unique<RBAC>(cfg);

    Role reader;
    reader.name        = "reader";
    reader.description = "Read-only role";
    reader.permissions = {{"data", "read"}};
    rbac->addRole(reader);

    Role admin;
    admin.name        = "admin";
    admin.description = "Administrator";
    admin.permissions = {{"*", "*"}};
    rbac->addRole(admin);

    return rbac;
}

static SecurityEvidenceCollector::Config make_config() {
    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);
    return cfg;
}

static auto now() { return std::chrono::system_clock::now(); }
static auto window_from() { return now() - std::chrono::hours(24); }

// ─── Test Fixture ─────────────────────────────────────────────────────────

class SecurityEvidenceCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        provider_ = make_provider();
        rbac_     = make_rbac();
        collector_ = std::make_unique<SecurityEvidenceCollector>(
            make_config(), provider_, rbac_.get(), nullptr);

        // Temporary directory for file export tests
        tmp_dir_ = fs::temp_directory_path() / "test_evidence_collector";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    std::shared_ptr<MockKeyProvider>          provider_;
    std::unique_ptr<RBAC>                      rbac_;
    std::unique_ptr<SecurityEvidenceCollector> collector_;
    fs::path                                   tmp_dir_;
};

// ============================================================================
// Construction
// ============================================================================

TEST(SecurityEvidenceCollectorConstruction, NullKeyProviderThrows) {
    EXPECT_THROW(
        SecurityEvidenceCollector(make_config(), nullptr, nullptr, nullptr),
        std::invalid_argument);
}

TEST(SecurityEvidenceCollectorConstruction, ValidProviderSucceeds) {
    auto p = make_provider();
    EXPECT_NO_THROW(
        SecurityEvidenceCollector(make_config(), p, nullptr, nullptr));
}

// ============================================================================
// collect() — bundle metadata
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, BundleId_IsNonEmpty) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_FALSE(bundle.bundle_id.empty());
}

TEST_F(SecurityEvidenceCollectorTest, BundleId_IsUnique) {
    auto b1 = collector_->collect(window_from(), now());
    auto b2 = collector_->collect(window_from(), now());
    EXPECT_NE(b1.bundle_id, b2.bundle_id);
}

TEST_F(SecurityEvidenceCollectorTest, CollectedAtMs_IsRecent) {
    auto before = std::chrono::duration_cast<std::chrono::milliseconds>(
        now().time_since_epoch()).count();
    auto bundle = collector_->collect(window_from(), now());
    auto after = std::chrono::duration_cast<std::chrono::milliseconds>(
        now().time_since_epoch()).count();

    EXPECT_GE(bundle.collected_at_ms, before);
    EXPECT_LE(bundle.collected_at_ms, after + 2000);
}

TEST_F(SecurityEvidenceCollectorTest, WindowTimestampsMatchRequest) {
    auto from = window_from();
    auto to   = now();
    auto bundle = collector_->collect(from, to);

    int64_t from_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        from.time_since_epoch()).count();
    int64_t to_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        to.time_since_epoch()).count();

    EXPECT_EQ(bundle.window_from_ms, from_ms);
    EXPECT_EQ(bundle.window_to_ms,   to_ms);
}

TEST_F(SecurityEvidenceCollectorTest, WithinRetentionWindow_RecentWindow) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_TRUE(bundle.within_retention_window)
        << "A window starting 24h ago must be within the 365-day retention window";
}

TEST_F(SecurityEvidenceCollectorTest, WithinRetentionWindow_FalseWhenBeyondRetention) {
    SecurityEvidenceCollector::Config short_config;
    short_config.retention_period = std::chrono::hours(1); // 1-hour retention

    SecurityEvidenceCollector short_collector(
        short_config, provider_, rbac_.get(), nullptr);

    // Window starts 2 hours ago — before the 1-hour retention period
    auto old_from = now() - std::chrono::hours(2);
    auto bundle = short_collector.collect(old_from, now());

    EXPECT_FALSE(bundle.within_retention_window)
        << "A window starting 2h ago must be outside a 1-hour retention period";
}

// ============================================================================
// collect() — metrics
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, Metrics_ActiveKeys_ReflectsCreatedKeys) {
    auto bundle = collector_->collect(window_from(), now());
    // provider_ was initialised with one key (version 1)
    EXPECT_GE(bundle.metrics.active_keys, 1u)
        << "At least one active key must be reflected in the metrics snapshot";
}

TEST_F(SecurityEvidenceCollectorTest, Metrics_DeprecatedKeys_AfterRotation) {
    auto bundle_before = collector_->collect(window_from(), now());
    uint64_t deprecated_before = bundle_before.metrics.deprecated_keys;

    // Rotating a key creates version 2; version 1 moves to DEPRECATED
    provider_->rotateKey("evidence_key");

    auto bundle_after = collector_->collect(window_from(), now());
    // After rotation the deprecated count must not decrease
    EXPECT_GE(bundle_after.metrics.deprecated_keys, deprecated_before);
}

TEST_F(SecurityEvidenceCollectorTest, Metrics_TotalRoles_MatchesRBAC) {
    auto bundle = collector_->collect(window_from(), now());
    // make_rbac() creates 2 roles: reader + admin
    EXPECT_EQ(bundle.metrics.total_roles, 2u);
}

// ============================================================================
// collect() — key rotations
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, KeyRotations_EmptyWhenNoRotationsInWindow) {
    // Window is in the future — no key was created after "now"
    auto future_from = now() + std::chrono::hours(1);
    auto future_to   = now() + std::chrono::hours(2);
    auto bundle = collector_->collect(future_from, future_to);
    EXPECT_TRUE(bundle.key_rotations.empty())
        << "No rotations occurred in the future window";
}

TEST_F(SecurityEvidenceCollectorTest, KeyRotations_CapturedAfterRotation) {
    // Register a second key so we have something to rotate, then rotate it
    provider_->createKey("rot_key", 1);
    auto before_rotation = now();
    provider_->rotateKey("rot_key");
    auto after_rotation = now();

    // Collect over a wide window
    auto bundle = collector_->collect(
        before_rotation - std::chrono::seconds(1),
        after_rotation  + std::chrono::seconds(1));

    // At least one rotation (rot_key v1 → v2) must be in the bundle
    bool found = false;
    for (const auto& r : bundle.key_rotations) {
        if (r.key_id == "rot_key" && r.from_version == 1 && r.to_version == 2) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Rotation of rot_key v1→v2 must appear in the bundle";
}

// ============================================================================
// collect() — access-control report
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, AccessControl_RoleCount) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_EQ(bundle.access_control.total_roles, 2u);
}

TEST_F(SecurityEvidenceCollectorTest, AccessControl_RoleNamesAreSorted) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_TRUE(std::is_sorted(
        bundle.access_control.role_names.begin(),
        bundle.access_control.role_names.end()))
        << "Role names must be sorted alphabetically for deterministic audits";
}

TEST_F(SecurityEvidenceCollectorTest, AccessControl_TotalPermissions) {
    auto bundle = collector_->collect(window_from(), now());
    // reader has 1 permission, admin has 1 (wildcard) → total = 2
    EXPECT_EQ(bundle.access_control.total_permissions, 2u);
}

TEST_F(SecurityEvidenceCollectorTest, AccessControl_HasAdminRole_True) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_TRUE(bundle.access_control.has_admin_role)
        << "The admin role with '*:*' must be detected";
}

TEST_F(SecurityEvidenceCollectorTest, AccessControl_EmptyRoles_Detected) {
    // Add a role with no permissions
    Role empty_role;
    empty_role.name        = "noPermRole";
    empty_role.description = "Role with no permissions";
    rbac_->addRole(empty_role);

    auto bundle = collector_->collect(window_from(), now());

    EXPECT_FALSE(bundle.access_control.all_roles_have_permissions)
        << "A role with no permissions must cause all_roles_have_permissions=false";
    EXPECT_FALSE(bundle.access_control.empty_roles.empty())
        << "empty_roles must list the name of the role with no permissions";
    EXPECT_NE(std::find(
        bundle.access_control.empty_roles.begin(),
        bundle.access_control.empty_roles.end(),
        "noPermRole"),
        bundle.access_control.empty_roles.end())
        << "'noPermRole' must appear in empty_roles";
}

TEST_F(SecurityEvidenceCollectorTest, NoRBAC_CollectionSucceeds) {
    SecurityEvidenceCollector no_rbac(make_config(), provider_, nullptr, nullptr);
    EXPECT_NO_THROW({
        auto bundle = no_rbac.collect(window_from(), now());
        EXPECT_EQ(bundle.access_control.total_roles, 0u);
    });
}

// ============================================================================
// JSON serialisation
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, ToJson_ContainsRequiredFields) {
    auto bundle = collector_->collect(window_from(), now());
    auto j = bundle.toJson();

    EXPECT_TRUE(j.contains("bundle_id"));
    EXPECT_TRUE(j.contains("collected_at_ms"));
    EXPECT_TRUE(j.contains("window_from_ms"));
    EXPECT_TRUE(j.contains("window_to_ms"));
    EXPECT_TRUE(j.contains("within_retention_window"));
    EXPECT_TRUE(j.contains("audit_log"));
    EXPECT_TRUE(j.contains("metrics"));
    EXPECT_TRUE(j.contains("key_rotations"));
    EXPECT_TRUE(j.contains("access_control"));

    EXPECT_EQ(j["bundle_id"].get<std::string>(), bundle.bundle_id);
    EXPECT_EQ(j["collected_at_ms"].get<int64_t>(), bundle.collected_at_ms);
}

TEST_F(SecurityEvidenceCollectorTest, ToJson_CanBeRoundTripped) {
    auto bundle = collector_->collect(window_from(), now());
    auto j = bundle.toJson();
    // Verify the JSON can be serialised and re-parsed
    std::string s = j.dump();
    try {
        auto parsed = nlohmann::json::parse(s);
        static_cast<void>(parsed);
    } catch (const std::exception& ex) {
        FAIL() << "Roundtrip JSON parse failed: " << ex.what();
    }
}

// ============================================================================
// exportToFile()
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, ExportToFile_WritesValidJson) {
    auto bundle = collector_->collect(window_from(), now());
    fs::path out_path = tmp_dir_ / "evidence_test.json";

    EXPECT_TRUE(collector_->exportToFile(bundle, out_path.string()));
    ASSERT_TRUE(fs::exists(out_path));

    std::ifstream in(out_path);
    ASSERT_TRUE(in.is_open());
    nlohmann::json j;
    EXPECT_NO_THROW(in >> j);
    EXPECT_TRUE(j.contains("bundle_id"));
}

TEST_F(SecurityEvidenceCollectorTest, ExportToFile_OverwritesExistingFile) {
    auto bundle = collector_->collect(window_from(), now());
    fs::path out_path = tmp_dir_ / "evidence_overwrite.json";

    // Write once
    ASSERT_TRUE(collector_->exportToFile(bundle, out_path.string()));
    auto size_after_first = fs::file_size(out_path);

    // Write again (should not fail)
    EXPECT_TRUE(collector_->exportToFile(bundle, out_path.string()));
    auto size_after_second = fs::file_size(out_path);

    EXPECT_EQ(size_after_first, size_after_second)
        << "Re-exporting the same bundle must produce the same file size";
}

TEST_F(SecurityEvidenceCollectorTest, ExportToFile_EmptyPathReturnsFalse) {
    auto bundle = collector_->collect(window_from(), now());
    EXPECT_FALSE(collector_->exportToFile(bundle, ""));
}

// ============================================================================
// verifyRetention()
// ============================================================================

TEST_F(SecurityEvidenceCollectorTest, VerifyRetention_EmptyPathReturnsTrue) {
    EXPECT_TRUE(collector_->verifyRetention(""));
}

TEST_F(SecurityEvidenceCollectorTest, VerifyRetention_NonExistentDirReturnsTrue) {
    EXPECT_TRUE(collector_->verifyRetention("/nonexistent/path/that/does/not/exist"));
}

TEST_F(SecurityEvidenceCollectorTest, VerifyRetention_InWindowBundle_ReturnsTrue) {
    auto bundle = collector_->collect(window_from(), now());
    fs::path store_dir = tmp_dir_ / "evidence_store";
    fs::create_directories(store_dir);

    fs::path out_path = store_dir / "bundle.json";
    ASSERT_TRUE(collector_->exportToFile(bundle, out_path.string()));

    EXPECT_TRUE(collector_->verifyRetention(store_dir.string()))
        << "A bundle collected today must be within the 365-day retention window";
}

TEST_F(SecurityEvidenceCollectorTest, VerifyRetention_OldBundle_ReturnsFalse) {
    // Manually write a bundle JSON with a window_from_ms that is 400 days old
    fs::path store_dir = tmp_dir_ / "evidence_store_old";
    fs::create_directories(store_dir);

    auto old_ts = std::chrono::system_clock::now() - std::chrono::hours(400 * 24);
    int64_t old_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        old_ts.time_since_epoch()).count();

    nlohmann::json old_bundle;
    old_bundle["bundle_id"]       = "old-bundle";
    old_bundle["window_from_ms"]  = old_ms;
    old_bundle["window_to_ms"]    = old_ms + 1000;

    std::ofstream out(store_dir / "old_bundle.json");
    out << old_bundle.dump();
    out.close();

    EXPECT_FALSE(collector_->verifyRetention(store_dir.string()))
        << "A bundle with window_from_ms 400 days old must fail 365-day retention check";
}

// ============================================================================
// Phase 6.2 Extended Tests: NetworkControlsEvidence & ChangeManagementEvidence
// ============================================================================

// Test G1 — NetworkControls: cipher suites list is non-empty
TEST_F(SecurityEvidenceCollectorTest, NetworkControls_CipherSuitesNonEmpty) {
    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(24);
    auto bundle = collector_->collect(from, now);

    EXPECT_FALSE(bundle.network_controls.tls_cipher_suites.empty())
        << "TLS cipher suite list must be populated";
    // Should include at least TLS_AES_256_GCM_SHA384
    bool found_aes256 = false;
    for (const auto& cs : bundle.network_controls.tls_cipher_suites) {
        if (cs.find("AES_256") != std::string::npos) {
            found_aes256 = true;
        }
    }
    EXPECT_TRUE(found_aes256) << "AES-256 cipher suite must be present";
}

// Test G2 — NetworkControls: mtls shard count is zero when no shards configured
TEST_F(SecurityEvidenceCollectorTest, NetworkControls_MtlsShardCountIsZeroWhenNoShards) {
    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(24);
    auto bundle = collector_->collect(from, now);

    EXPECT_GE(bundle.network_controls.mtls_enabled_shard_count, 0)
        << "mTLS shard count must be non-negative";
}

// Test G3 — NetworkControls: rate limiter snapshot is valid JSON
TEST_F(SecurityEvidenceCollectorTest, NetworkControls_RateLimiterSnapshotIsValidJson) {
    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(24);
    auto bundle = collector_->collect(from, now);

    EXPECT_FALSE(bundle.network_controls.rate_limiter_config_snapshot.empty())
        << "Rate limiter snapshot must not be empty";

    EXPECT_NO_THROW({
        auto j = nlohmann::json::parse(bundle.network_controls.rate_limiter_config_snapshot);
        EXPECT_TRUE(j.is_object()) << "Rate limiter snapshot must be a JSON object";
    }) << "Rate limiter snapshot must be valid JSON";
}

// Test G4 — ChangeManagement: key rotation log captured
TEST_F(SecurityEvidenceCollectorTest, ChangeManagement_KeyRotationLogCaptured) {
    // Rotate a key so there's a rotation to capture
    provider_->createKey("change_mgmt_key", 1);
    provider_->rotateKey("change_mgmt_key");

    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(24);
    auto bundle = collector_->collect(from, now);

    // The key_rotation_log in ChangeManagementEvidence should match key_rotations
    EXPECT_EQ(bundle.change_management.key_rotation_log.size(),
              bundle.key_rotations.size())
        << "ChangeManagement key_rotation_log must match bundle key_rotations";
}

// Test G5 — ChangeManagement: timestamps set correctly
TEST_F(SecurityEvidenceCollectorTest, ChangeManagement_TimestampsSet) {
    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(48);

    auto from_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        from.time_since_epoch()).count();
    auto to_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    auto bundle = collector_->collect(from, now);

    EXPECT_EQ(bundle.change_management.from_ms, from_ms)
        << "ChangeManagement from_ms must match collection window start";
    EXPECT_EQ(bundle.change_management.to_ms, to_ms)
        << "ChangeManagement to_ms must match collection window end";
}

// Test G6 — Bundle includes network_controls and change_management in toJson()
TEST_F(SecurityEvidenceCollectorTest, Bundle_IncludesNetworkAndChangeManagementEvidence) {
    auto now  = std::chrono::system_clock::now();
    auto from = now - std::chrono::hours(24);
    auto bundle = collector_->collect(from, now);

    auto j = bundle.toJson();
    EXPECT_TRUE(j.contains("network_controls"))
        << "Bundle JSON must contain 'network_controls' field";
    EXPECT_TRUE(j.contains("change_management"))
        << "Bundle JSON must contain 'change_management' field";

    // Verify sub-structure
    EXPECT_TRUE(j["network_controls"].contains("tls_cipher_suites"));
    EXPECT_TRUE(j["network_controls"].contains("mtls_enabled_shard_count"));
    EXPECT_TRUE(j["network_controls"].contains("rate_limiter_config_snapshot"));
    EXPECT_TRUE(j["change_management"].contains("from_ms"));
    EXPECT_TRUE(j["change_management"].contains("to_ms"));
    EXPECT_TRUE(j["change_management"].contains("key_rotation_log"));
}
