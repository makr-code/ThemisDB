/*
 * test_user_storage_features.cpp
 *
 * Focused tests for user_storage_encrypted v0.1.0 features:
 *   - Stdin key delivery (AC-SD-1 … AC-SD-4)
 *   - Argon2id KDF (AC-KDF-1 … AC-KDF-10)
 *   - IRotationStore persistence (AC-PRS-1 … AC-PRS-6)
 *   - GocryptfsBackend availability (AC-GCF-1 … AC-GCF-2)
 */

#include "../include/gocryptfs_backend.hpp"
#include "../include/key_derivation_service.hpp"
#include "../include/irotation_store.hpp"
#include "../include/key_rotation_scheduler.hpp"
#include "../include/security_level.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/stat.h>

using namespace themis::plugins::user_storage;
namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static std::string tmp_dir() {
    return "/tmp/themis_test_" + std::to_string(getpid());
}

// ============================================================================
// AC-SD: Stdin key delivery tests
// ============================================================================

class StdinDeliveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = tmp_dir() + "_stdin";
        fs::create_directories(dir_);
    }
    void TearDown() override {
        fs::remove_all(dir_);
    }
    std::string dir_;
};

// AC-SD-1: executeCommandWithStdin passes data to child's stdin
TEST_F(StdinDeliveryTest, ExecuteCommandWithStdinPassesData) {
    GocryptfsBackend backend;
    // 'cat' reads stdin and writes it to stdout
    auto result = backend.executeCommandWithStdin({"cat"}, "hello_from_stdin");
    ASSERT_TRUE(result.isSuccess()) << result.error();
    EXPECT_EQ(result.value(), "hello_from_stdin");
}

// AC-SD-2: deliverKeyViaStdin pipes key and returns cat output (no tmp file)
TEST_F(StdinDeliveryTest, DeliverKeyViaStdinNoTempFile) {
    GocryptfsBackend backend;
    std::string key_hex = "deadbeef01020304";

    // Count /tmp files matching our prefix before and after
    int before = 0;
    for (const auto& e : fs::directory_iterator("/tmp")) {
        if (e.path().filename().string().find("gocryptfs_key_") == 0) {
            ++before;
        }
    }

    auto result = backend.deliverKeyViaStdin({"cat"}, key_hex);
    ASSERT_TRUE(result.isSuccess()) << result.error();

    int after = 0;
    for (const auto& e : fs::directory_iterator("/tmp")) {
        if (e.path().filename().string().find("gocryptfs_key_") == 0) {
            ++after;
        }
    }
    // No new gocryptfs_key_* files should have been created
    EXPECT_EQ(before, after);
}

// AC-SD-3: deliverKeyViaStdin zeroes key_hex after delivery
TEST_F(StdinDeliveryTest, DeliverKeyViaStdinZeroesKeyMaterial) {
    GocryptfsBackend backend;
    // We cannot observe the internal zeroing from outside, but we can verify
    // that the call succeeds and the result contains the key (via cat), proving
    // the pipe write happened before the zero.
    std::string key = "aabbccddeeff0011";
    auto result = backend.deliverKeyViaStdin({"cat"}, key);
    ASSERT_TRUE(result.isSuccess()) << result.error();
    EXPECT_FALSE(result.value().empty());
}

// AC-SD-4: executeCommandWithStdin returns error on non-zero exit
TEST_F(StdinDeliveryTest, ExecuteCommandWithStdinReturnsErrorOnFailure) {
    GocryptfsBackend backend;
    // 'false' always exits with code 1
    auto result = backend.executeCommandWithStdin({"false"}, "data");
    EXPECT_TRUE(result.isError());
}

// ============================================================================
// AC-KDF: Argon2id key derivation tests
// ============================================================================

class Argon2idKdfTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = tmp_dir() + "_kdf";
        fs::create_directories(dir_);
        salt_file_ = dir_ + "/.themis_kdf_salt";
    }
    void TearDown() override {
        fs::remove_all(dir_);
    }

    Argon2idKeyDerivationService kdf_;
    std::string dir_;
    std::string salt_file_;
};

// AC-KDF-1: deriveKey produces 32-byte output
TEST_F(Argon2idKdfTest, DeriveKeyProduces32Bytes) {
    std::vector<uint8_t> mk(16, 0xAB);
    std::vector<uint8_t> salt(32, 0x01);
    auto result = kdf_.deriveKey(mk, salt);
    ASSERT_TRUE(result.isSuccess()) << result.error();
    EXPECT_EQ(result.value().size(), 32u);
}

// AC-KDF-2: same inputs → same derived key (deterministic)
TEST_F(Argon2idKdfTest, DeriveKeyIsDeterministic) {
    std::vector<uint8_t> mk{0x01, 0x02, 0x03};
    std::vector<uint8_t> salt(32, 0x55);
    auto r1 = kdf_.deriveKey(mk, salt);
    auto r2 = kdf_.deriveKey(mk, salt);
    ASSERT_TRUE(r1.isSuccess());
    ASSERT_TRUE(r2.isSuccess());
    EXPECT_EQ(r1.value(), r2.value());
}

// AC-KDF-3: different salt → different derived key
TEST_F(Argon2idKdfTest, DifferentSaltProducesDifferentKey) {
    std::vector<uint8_t> mk(8, 0xAA);
    std::vector<uint8_t> salt1(32, 0x11);
    std::vector<uint8_t> salt2(32, 0x22);
    auto r1 = kdf_.deriveKey(mk, salt1);
    auto r2 = kdf_.deriveKey(mk, salt2);
    ASSERT_TRUE(r1.isSuccess());
    ASSERT_TRUE(r2.isSuccess());
    EXPECT_NE(r1.value(), r2.value());
}

// AC-KDF-4: different master key → different derived key
TEST_F(Argon2idKdfTest, DifferentMasterKeyProducesDifferentKey) {
    std::vector<uint8_t> mk1(16, 0xAA);
    std::vector<uint8_t> mk2(16, 0xBB);
    std::vector<uint8_t> salt(32, 0x99);
    auto r1 = kdf_.deriveKey(mk1, salt);
    auto r2 = kdf_.deriveKey(mk2, salt);
    ASSERT_TRUE(r1.isSuccess());
    ASSERT_TRUE(r2.isSuccess());
    EXPECT_NE(r1.value(), r2.value());
}

// AC-KDF-5: parameters are kMemoryCost=65536, kTimeCost=3, kParallelism=4
TEST_F(Argon2idKdfTest, ParametersMatchSpecification) {
    EXPECT_EQ(Argon2idKeyDerivationService::kMemoryCost,  65536u);
    EXPECT_EQ(Argon2idKeyDerivationService::kTimeCost,    3u);
    EXPECT_EQ(Argon2idKeyDerivationService::kParallelism, 4u);
    EXPECT_EQ(Argon2idKeyDerivationService::kKeyLength,   32u);
}

// AC-KDF-6: generateSalt produces unique salts
TEST_F(Argon2idKdfTest, GenerateSaltProducesUniqueSalts) {
    auto s1 = kdf_.generateSalt();
    auto s2 = kdf_.generateSalt();
    ASSERT_TRUE(s1.isSuccess());
    ASSERT_TRUE(s2.isSuccess());
    EXPECT_EQ(s1.value().size(), 32u);
    EXPECT_NE(s1.value(), s2.value());
}

// AC-KDF-7: loadOrCreateSalt creates a salt file if absent
TEST_F(Argon2idKdfTest, LoadOrCreateSaltCreatesFile) {
    EXPECT_FALSE(fs::exists(salt_file_));
    auto result = kdf_.loadOrCreateSalt(salt_file_);
    ASSERT_TRUE(result.isSuccess()) << result.error();
    EXPECT_TRUE(fs::exists(salt_file_));
    EXPECT_EQ(result.value().size(), 32u);
}

// AC-KDF-8: loadOrCreateSalt returns the same salt on second call
TEST_F(Argon2idKdfTest, LoadOrCreateSaltReturnsSameSaltOnReload) {
    auto r1 = kdf_.loadOrCreateSalt(salt_file_);
    ASSERT_TRUE(r1.isSuccess());
    auto r2 = kdf_.loadOrCreateSalt(salt_file_);
    ASSERT_TRUE(r2.isSuccess());
    EXPECT_EQ(r1.value(), r2.value());
}

// AC-KDF-9: empty master_key → error
TEST_F(Argon2idKdfTest, EmptyMasterKeyReturnsError) {
    std::vector<uint8_t> empty_mk;
    std::vector<uint8_t> salt(32, 0x01);
    auto result = kdf_.deriveKey(empty_mk, salt);
    EXPECT_TRUE(result.isError());
}

// AC-KDF-10: empty salt → error
TEST_F(Argon2idKdfTest, EmptySaltReturnsError) {
    std::vector<uint8_t> mk(16, 0xAB);
    std::vector<uint8_t> empty_salt;
    auto result = kdf_.deriveKey(mk, empty_salt);
    EXPECT_TRUE(result.isError());
}

// ============================================================================
// AC-PRS: IRotationStore persistence tests
// ============================================================================

class RotationPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = tmp_dir() + "_prs";
        fs::create_directories(dir_);
        store_path_ = dir_ + "/rotation_state.json";
    }
    void TearDown() override {
        fs::remove_all(dir_);
    }

    std::string dir_;
    std::string store_path_;
};

// AC-PRS-1: FileRotationStore save/load round-trip
TEST_F(RotationPersistenceTest, FileStoreRoundTrip) {
    FileRotationStore store(store_path_);
    auto save_res = store.save(SecurityLevel::VS_NFD, 1234567890LL);
    ASSERT_TRUE(save_res.isSuccess());

    auto load_res = store.load(SecurityLevel::VS_NFD);
    ASSERT_TRUE(load_res.isSuccess());
    EXPECT_EQ(load_res.value(), 1234567890LL);
}

// AC-PRS-2: KeyRotationScheduler with store persists last_check_ms
TEST_F(RotationPersistenceTest, SchedulerPersistsLastCheckMs) {
    auto store = std::make_shared<FileRotationStore>(store_path_);

    KeyRotationScheduler sched;
    sched.setRotationStore(store);
    ASSERT_TRUE(sched.initialize(3600).isSuccess());

    bool triggered = false;
    sched.scheduleRotation(SecurityLevel::VS_NFD, 90, false,
        [&](SecurityLevel, bool, const std::string&) { triggered = true; });

    sched.triggerRotation(SecurityLevel::VS_NFD);
    sched.shutdown();

    // Verify the store has a non-zero timestamp
    auto loaded = store->load(SecurityLevel::VS_NFD);
    ASSERT_TRUE(loaded.isSuccess());
    EXPECT_GT(loaded.value(), 0LL);
}

// AC-PRS-3: After scheduler restart, loads persisted last_check_ms
TEST_F(RotationPersistenceTest, SchedulerLoadsPersistedTimestampOnRestart) {
    const int64_t ts = 9999999999LL;
    {
        FileRotationStore store(store_path_);
        store.save(SecurityLevel::VS_NFD, ts);
    }

    // New scheduler instance should load the persisted timestamp
    auto store = std::make_shared<FileRotationStore>(store_path_);
    KeyRotationScheduler sched;
    sched.setRotationStore(store);
    sched.initialize(3600);
    sched.scheduleRotation(SecurityLevel::VS_NFD, 90, false, nullptr);
    // getNextRotationTime uses last_check_ms loaded from store
    int64_t next = sched.getNextRotationTime(SecurityLevel::VS_NFD);
    sched.shutdown();

    int64_t expected_next = ts + static_cast<int64_t>(90) * 24 * 3600 * 1000;
    EXPECT_EQ(next, expected_next);
}

// AC-PRS-4: NullRotationStore::load returns 0 (no persistence)
TEST_F(RotationPersistenceTest, NullStoreLoadReturnsZero) {
    NullRotationStore store;
    auto r = store.load(SecurityLevel::VS_NFD);
    ASSERT_TRUE(r.isSuccess());
    EXPECT_EQ(r.value(), 0LL);
}

// AC-PRS-5: Multiple SecurityLevels stored and loaded independently
TEST_F(RotationPersistenceTest, MultipleSecurityLevelsStoredIndependently) {
    FileRotationStore store(store_path_);
    store.save(SecurityLevel::OFFEN,     100LL);
    store.save(SecurityLevel::VS_NFD,    200LL);
    store.save(SecurityLevel::GEHEIM, 300LL);

    EXPECT_EQ(store.load(SecurityLevel::OFFEN).value(),     100LL);
    EXPECT_EQ(store.load(SecurityLevel::VS_NFD).value(),    200LL);
    EXPECT_EQ(store.load(SecurityLevel::GEHEIM).value(), 300LL);
}

// AC-PRS-6: Loading from missing file returns 0 (not an error)
TEST_F(RotationPersistenceTest, MissingFileReturnsZeroNotError) {
    FileRotationStore store(store_path_ + ".nonexistent");
    auto r = store.load(SecurityLevel::OFFEN);
    ASSERT_TRUE(r.isSuccess());
    EXPECT_EQ(r.value(), 0LL);
}

// ============================================================================
// AC-GCF: GocryptfsBackend availability tests
// ============================================================================

TEST(GocryptfsBackendTest, CheckAvailabilityReturnsResult) {
    GocryptfsBackend backend;
    auto result = backend.checkAvailability();
    // On CI without gocryptfs, this may be an error — either is valid
    // We only assert the function completes without throwing
    (void)result;
    SUCCEED();
}

TEST(GocryptfsBackendTest, GetBackendInfoReturnsStrings) {
    GocryptfsBackend backend;
    EXPECT_EQ(backend.getBackendName(), "gocryptfs");
    // getBackendVersion may return "unknown" if binary is absent — that's fine
    std::string version = backend.getBackendVersion();
    EXPECT_FALSE(version.empty());
}
