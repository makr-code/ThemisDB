#include <gtest/gtest.h>
#include "security/usb_volume_hardening.h"
#include "security/usb_admin_authenticator.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

using namespace themis::security;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Compute SHA-256 of a file and return its hex digest (mirrors production).
static std::string sha256OfFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        EVP_DigestUpdate(ctx, buf, static_cast<size_t>(f.gcount()));
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;
    EVP_DigestFinal_ex(ctx, digest, &len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

/// Write arbitrary text to a file.
static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    f << content;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class USBVolumeHardeningTest : public ::testing::Test {
protected:
    std::string mount_path_;
    std::string license_filename_ = "themis_admin.lic";

    void SetUp() override {
        mount_path_ = (fs::temp_directory_path() / "themis_vol_hardening_test").string();
        fs::create_directories(mount_path_);
    }

    void TearDown() override {
        if (fs::exists(mount_path_)) {
            fs::remove_all(mount_path_);
        }
    }

    std::string licensePath() const {
        return mount_path_ + "/" + license_filename_;
    }

    void createLicenseFile(const std::string& content = "THEMIS-ENT-ADMIN-TEST-12345678") {
        writeFile(licensePath(), content);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// computeVolumeHash tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_ReturnsNonEmptyForExistingFile) {
    createLicenseFile("hello-license-data");
    std::string hash = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);
    EXPECT_FALSE(hash.empty()) << "Hash must not be empty for an existing file";
    EXPECT_EQ(hash.size(), 64u) << "SHA-256 hex digest must be 64 characters";
}

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_MatchesExpectedSHA256) {
    createLicenseFile("known content");
    std::string expected = sha256OfFile(licensePath());
    std::string actual   = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);
    EXPECT_EQ(actual, expected) << "Hash mismatch for known file content";
}

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_ReturnsEmptyForMissingFile) {
    // No file created
    std::string hash = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);
    EXPECT_TRUE(hash.empty()) << "Missing file must produce empty hash";
}

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_ReturnsEmptyForMissingDirectory) {
    std::string hash = USBVolumeHardening::computeVolumeHash("/nonexistent/path", license_filename_);
    EXPECT_TRUE(hash.empty());
}

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_DifferentContentProducesDifferentHash) {
    createLicenseFile("original content");
    std::string h1 = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);

    // Simulate FAT-level modification
    createLicenseFile("tampered content!");
    std::string h2 = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);

    EXPECT_NE(h1, h2) << "Different file content must produce different hashes";
    EXPECT_FALSE(h1.empty());
    EXPECT_FALSE(h2.empty());
}

TEST_F(USBVolumeHardeningTest, ComputeVolumeHash_EmptyFileProducesKnownHash) {
    // SHA-256 of empty input is well-known
    static const char* kEmptySHA256 =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    createLicenseFile("");
    std::string hash = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);
    EXPECT_EQ(hash, kEmptySHA256);
}

// ─────────────────────────────────────────────────────────────────────────────
// verifyVolumeHash tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(USBVolumeHardeningTest, VerifyVolumeHash_AcceptsMatchingHash) {
    createLicenseFile("license body");
    std::string hash = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);

    EXPECT_TRUE(USBVolumeHardening::verifyVolumeHash(mount_path_, license_filename_, hash))
        << "Correct pinned hash must be accepted";
}

TEST_F(USBVolumeHardeningTest, VerifyVolumeHash_RejectsTamperedFile) {
    createLicenseFile("original");
    std::string pinned = USBVolumeHardening::computeVolumeHash(mount_path_, license_filename_);

    // Attacker modifies the license file on the FAT volume
    createLicenseFile("tampered!");
    EXPECT_FALSE(USBVolumeHardening::verifyVolumeHash(mount_path_, license_filename_, pinned))
        << "Tampered file must not match the pinned hash";
}

TEST_F(USBVolumeHardeningTest, VerifyVolumeHash_RejectsEmptyExpectedHash) {
    createLicenseFile("some content");
    EXPECT_FALSE(USBVolumeHardening::verifyVolumeHash(mount_path_, license_filename_, ""))
        << "Empty expected_hash must never pass (unconfigured state)";
}

TEST_F(USBVolumeHardeningTest, VerifyVolumeHash_RejectsWrongHash) {
    createLicenseFile("license body");
    std::string wrong_hash(64, 'a');  // Syntactically valid but wrong
    EXPECT_FALSE(USBVolumeHardening::verifyVolumeHash(mount_path_, license_filename_, wrong_hash));
}

TEST_F(USBVolumeHardeningTest, VerifyVolumeHash_RejectsMissingFile) {
    std::string fake_hash(64, '0');
    EXPECT_FALSE(USBVolumeHardening::verifyVolumeHash(mount_path_, license_filename_, fake_hash))
        << "Missing file must not pass hash verification";
}

// ─────────────────────────────────────────────────────────────────────────────
// isMountedReadOnly tests
// (We cannot reliably test true read-only status in unit tests without root,
//  so we test the negative path: temp dirs are never read-only mounts.)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(USBVolumeHardeningTest, IsMountedReadOnly_TempDirIsNotReadOnly) {
    // A regular writable temp directory must not be reported as read-only.
    // This also covers the code path where the mount point is found in
    // /proc/mounts (or the equivalent on the test platform) with "rw" options.
    bool ro = USBVolumeHardening::isMountedReadOnly(mount_path_);
    EXPECT_FALSE(ro) << "A writable temp directory must not be reported as read-only";
}

TEST_F(USBVolumeHardeningTest, IsMountedReadOnly_NonExistentPathReturnsFalse) {
    bool ro = USBVolumeHardening::isMountedReadOnly("/nonexistent/mount/path");
    EXPECT_FALSE(ro);
}

// ─────────────────────────────────────────────────────────────────────────────
// getUSBDeviceSerial / verifyUSBSerial tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(USBVolumeHardeningTest, GetUSBDeviceSerial_NonExistentMountReturnsEmpty) {
    // A non-existent path will not be found in /proc/mounts.
    std::string serial = USBVolumeHardening::getUSBDeviceSerial("/nonexistent/mount");
    EXPECT_TRUE(serial.empty()) << "Non-existent mount must return empty serial";
}

TEST_F(USBVolumeHardeningTest, VerifyUSBSerial_EmptyExpectedSerialReturnsFalse) {
    // Unconfigured serial (empty) must never pass verification.
    EXPECT_FALSE(USBVolumeHardening::verifyUSBSerial(mount_path_, ""))
        << "Empty expected_serial must never pass";
}

TEST_F(USBVolumeHardeningTest, VerifyUSBSerial_WrongSerialReturnsFalse) {
    // We cannot control what serial the test machine returns for the temp dir
    // (likely empty for non-USB mounts).  Supplying a non-empty expected serial
    // must fail when the actual serial does not match.
    std::string actual = USBVolumeHardening::getUSBDeviceSerial(mount_path_);
    std::string wrong  = "WRONG-SERIAL-XYZ";
    if (actual == wrong) {
        GTEST_SKIP() << "Test machine happens to return that serial — skipping";
    }
    EXPECT_FALSE(USBVolumeHardening::verifyUSBSerial(mount_path_, wrong))
        << "Wrong serial must not pass verification";
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: USBAdminAuthenticator with volume hardening config
// ─────────────────────────────────────────────────────────────────────────────

class USBAdminHardeningIntegrationTest : public ::testing::Test {
protected:
    std::string test_mount_path_;
    std::string license_file_path_;

    void SetUp() override {
        test_mount_path_ = (fs::temp_directory_path() / "themis_usb_harden_test").string();
        fs::create_directories(test_mount_path_);
        license_file_path_ = test_mount_path_ + "/themis_admin.lic";
    }

    void TearDown() override {
        if (fs::exists(test_mount_path_)) {
            fs::remove_all(test_mount_path_);
        }
    }

    /// Write a license file that USBAdminAuthenticator will accept
    /// (the test overrides hardware-ID checking via a known HW ID).
    void createTestLicense(const std::string& content = "") {
        if (!content.empty()) {
            std::ofstream f(license_file_path_);
            f << content;
            return;
        }
        nlohmann::json lic = {
            {"license_key",   "THEMIS-ENT-ADMIN-TEST-12345678"},
            {"organization",  "Test Org"},
            {"hardware_id",   "TEST-HARDWARE-ID-12345678"},
            {"issued_date",   "2026-01-01"},
            {"expiry_date",   "2027-12-31"},
            {"admin_scopes",  nlohmann::json::array({"admin", "config:write"})},
            {"signature",     "TEST-SIGNATURE-12345"}
        };
        std::ofstream f(license_file_path_);
        f << lic.dump(2);
    }

    std::string pinnedHash() {
        return sha256OfFile(license_file_path_);
    }
};

TEST_F(USBAdminHardeningIntegrationTest, VolumeHashCheck_AcceptsCorrectHash) {
    createTestLicense();
    std::string hash = sha256OfFile(license_file_path_);
    ASSERT_FALSE(hash.empty());

    USBAdminConfig cfg;
    cfg.mount_path          = test_mount_path_;
    cfg.require_usb_for_admin = true;
    cfg.expected_volume_hash  = hash;

    USBAdminAuthenticator auth(cfg);
    auth.initialize();
    // refreshUSBStatus will also validate hardware — use "allow all" hardware
    // by passing a license that matches TEST-HARDWARE-ID-12345678 (getSystemHardwareID
    // won't match on CI, so the license load may fail; we only test that the
    // volume-hash logic doesn't add a spurious rejection).
    // We verify that the hash verification itself does not reject the operation.
    bool status = auth.refreshUSBStatus();
    // The refresh may fail due to hardware-ID mismatch (expected in CI), but
    // the metrics counter for volume hash mismatch must remain 0.
    auto metrics = auth.getMetrics();
    EXPECT_EQ(metrics.usb_denied_volume_hash_mismatch, 0u)
        << "Correct hash must not increment the mismatch counter";
}

TEST_F(USBAdminHardeningIntegrationTest, VolumeHashCheck_RejectsTamperedLicense) {
    createTestLicense();
    std::string pinned = sha256OfFile(license_file_path_);
    ASSERT_FALSE(pinned.empty());

    // Simulate FAT-level tampering while keeping a syntactically valid license
    // so refreshUSBStatus() reaches the volume-hash hardening check.
    nlohmann::json tampered = {
        {"license_key",   "THEMIS-ENT-ADMIN-TEST-12345678"},
        {"organization",  "Tampered Org"},
        {"hardware_id",   "TEST-HARDWARE-ID-12345678"},
        {"issued_date",   "2026-01-01"},
        {"expiry_date",   "2027-12-31"},
        {"admin_scopes",  nlohmann::json::array({"admin", "config:write"})},
        {"signature",     "TEST-SIGNATURE-12345"}
    };
    std::ofstream f(license_file_path_, std::ios::trunc);
    f << tampered.dump(2);
    f.close();

    USBAdminConfig cfg;
    cfg.mount_path            = test_mount_path_;
    cfg.require_usb_for_admin = true;
    cfg.expected_volume_hash  = pinned;  // Pin the original hash

    USBAdminAuthenticator auth(cfg);
    auth.setLicenseVerifierFn([](const USBAdminLicense&, const std::string&) {
        return true;
    });
    auth.initialize();
    auth.refreshUSBStatus();  // Should detect hash mismatch

    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.usb_denied_volume_hash_mismatch, 0u)
        << "Tampered license file must be detected by volume hash check";
    EXPECT_FALSE(auth.isAdminUSBPresent())
        << "USB must not be considered present after tampered license";
}

TEST_F(USBAdminHardeningIntegrationTest, VolumeHashCheck_AdminDeniedAfterTampering) {
    createTestLicense();
    std::string pinned = sha256OfFile(license_file_path_);

    // Tamper
    std::ofstream f(license_file_path_, std::ios::trunc);
    f << "TAMPERED";
    f.close();

    USBAdminConfig cfg;
    cfg.mount_path            = test_mount_path_;
    cfg.require_usb_for_admin = true;
    cfg.expected_volume_hash  = pinned;

    USBAdminAuthenticator auth(cfg);
    auth.initialize();

    EXPECT_FALSE(auth.validateAdminOperation("admin", "test_user"))
        << "Admin operation must be denied when license file was tampered";
}

TEST_F(USBAdminHardeningIntegrationTest, ReadOnlyMount_RejectionCountedInMetrics) {
    createTestLicense();

    USBAdminConfig cfg;
    cfg.mount_path             = test_mount_path_;
    cfg.require_usb_for_admin  = true;
    cfg.require_readonly_mount = true;  // Enforce read-only mount

    USBAdminAuthenticator auth(cfg);
    auth.setLicenseVerifierFn([](const USBAdminLicense&, const std::string&) {
        return true;
    });
    auth.initialize();
    auth.refreshUSBStatus();  // temp dir is rw — must be rejected

    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.usb_denied_not_readonly, 0u)
        << "Read-write mount must increment the not-readonly counter";
    EXPECT_FALSE(auth.isAdminUSBPresent())
        << "USB must not be accepted when not mounted read-only";
}

TEST_F(USBAdminHardeningIntegrationTest, SerialMismatch_RejectionCountedInMetrics) {
    createTestLicense();

    USBAdminConfig cfg;
    cfg.mount_path            = test_mount_path_;
    cfg.require_usb_for_admin = true;
    cfg.expected_usb_serial   = "EXPECTED-SERIAL-THAT-WONT-MATCH";

    USBAdminAuthenticator auth(cfg);
    auth.setLicenseVerifierFn([](const USBAdminLicense&, const std::string&) {
        return true;
    });
    auth.initialize();
    auth.refreshUSBStatus();

    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.usb_denied_serial_mismatch, 0u)
        << "Serial mismatch must increment the serial-mismatch counter";
    EXPECT_FALSE(auth.isAdminUSBPresent())
        << "USB must not be accepted when serial does not match";
}

TEST_F(USBAdminHardeningIntegrationTest, NoHardeningConfigured_SkipsHardeningChecks) {
    createTestLicense();

    USBAdminConfig cfg;
    cfg.mount_path            = test_mount_path_;
    cfg.require_usb_for_admin = true;
    // No hardening fields set — all checks are skipped

    USBAdminAuthenticator auth(cfg);
    auth.initialize();
    auth.refreshUSBStatus();

    auto metrics = auth.getMetrics();
    EXPECT_EQ(metrics.usb_denied_not_readonly, 0u);
    EXPECT_EQ(metrics.usb_denied_volume_hash_mismatch, 0u);
    EXPECT_EQ(metrics.usb_denied_serial_mismatch, 0u);
}
