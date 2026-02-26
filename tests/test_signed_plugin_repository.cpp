/*
 * test_signed_plugin_repository.cpp
 *
 * Unit tests for SignedPluginRepository (signed plugin catalog with key pinning).
 *
 * Tests cover:
 *   - Key management: addPinnedKey, removePinnedKey, hasPinnedKey, getPinnedKeys
 *   - computeKeyFingerprint correctness
 *   - canonicalManifestJson determinism
 *   - verifyEntry: accept valid signature, reject tampered payload, reject
 *     unknown key, reject inactive key, reject malformed Base64
 *   - addEntry: acceptance / rejection policy
 *   - findEntry / findByName / listEntries
 *   - clear
 *   - Thread-safety smoke test
 */

#include <gtest/gtest.h>
#include "plugins/signed_plugin_repository.h"
#include "plugins/plugin_interface.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace themis::plugins;

// ============================================================================
// Helpers
// ============================================================================

namespace {

// Generate a fresh Ed25519 key pair.
// Returns {private_key_bytes (64 bytes), public_key_bytes (32 bytes)}.
// Throws std::runtime_error on OpenSSL failure.
struct Ed25519KeyPair {
    std::vector<uint8_t> private_key; // 64-byte seed+public (OpenSSL raw format)
    std::vector<uint8_t> public_key;  // 32 bytes
};

Ed25519KeyPair generateKeyPair() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed");

    if (EVP_PKEY_keygen_init(ctx) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen_init failed");
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen failed");
    }
    EVP_PKEY_CTX_free(ctx);

    Ed25519KeyPair kp;
    kp.private_key.resize(64);
    kp.public_key.resize(32);

    size_t priv_len = kp.private_key.size();
    size_t pub_len  = kp.public_key.size();

    if (EVP_PKEY_get_raw_private_key(pkey, kp.private_key.data(), &priv_len) != 1 ||
        EVP_PKEY_get_raw_public_key (pkey, kp.public_key.data(),  &pub_len)  != 1) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_PKEY_get_raw_*_key failed");
    }
    EVP_PKEY_free(pkey);
    kp.private_key.resize(priv_len);
    kp.public_key.resize(pub_len);
    return kp;
}

// Sign a message with a raw Ed25519 private key (32-byte seed).
// Returns 64-byte raw signature.
std::vector<uint8_t> signMessage(const std::vector<uint8_t>& private_key,
                                 const std::string& message) {
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, private_key.data(), private_key.size());
    if (!pkey) throw std::runtime_error("EVP_PKEY_new_raw_private_key failed");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); throw std::runtime_error("EVP_MD_CTX_new failed"); }

    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSignInit failed");
    }

    size_t sig_len = 64;
    std::vector<uint8_t> sig(sig_len);
    if (EVP_DigestSign(ctx,
                       sig.data(), &sig_len,
                       reinterpret_cast<const uint8_t*>(message.data()),
                       message.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSign failed");
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    sig.resize(sig_len);
    return sig;
}

// Base64-encode raw bytes (standard alphabet, no line wrapping).
std::string base64Encode(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.data(), static_cast<int>(data.size()));
    BIO_flush(b64);

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

// Build a minimal MarketplaceManifest for testing.
MarketplaceManifest makeManifest(const std::string& name,
                                 const std::string& version = "1.0.0") {
    MarketplaceManifest m;
    m.name          = name;
    m.version       = version;
    m.description   = "Test plugin";
    m.type          = PluginType::CUSTOM;
    m.author        = "TestAuthor";
    m.license       = "MIT";
    m.binary_linux  = name + ".so";
    m.binary_windows = name + ".dll";
    m.binary_macos  = name + ".dylib";
    m.verified_publisher = false;
    return m;
}

// Build a signed RepositoryEntry.
RepositoryEntry makeEntry(const Ed25519KeyPair& kp,
                          const std::string& fingerprint,
                          const MarketplaceManifest& manifest) {
    const std::string payload = SignedPluginRepository::canonicalManifestJson(manifest);
    auto sig_raw = signMessage(kp.private_key, payload);

    RepositoryEntry entry;
    entry.manifest        = manifest;
    entry.signature_b64   = base64Encode(sig_raw);
    entry.key_fingerprint = fingerprint;
    return entry;
}

} // anonymous namespace

// ============================================================================
// Fixture
// ============================================================================

class SignedPluginRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        kp_ = generateKeyPair();
        fp_ = SignedPluginRepository::computeKeyFingerprint(kp_.public_key);

        PinnedKey pk;
        pk.fingerprint = fp_;
        pk.public_key  = kp_.public_key;
        pk.label       = "Test Key";
        pk.active      = true;
        repo_.addPinnedKey(pk);
    }

    Ed25519KeyPair          kp_;
    std::string             fp_;
    SignedPluginRepository  repo_;
};

// ============================================================================
// computeKeyFingerprint
// ============================================================================

TEST(KeyFingerprintTest, EmptyKeyReturnsEmpty) {
    EXPECT_EQ("", SignedPluginRepository::computeKeyFingerprint({}));
}

TEST(KeyFingerprintTest, DeterministicFor32ZeroBytes) {
    std::vector<uint8_t> zeros(32, 0);
    auto fp1 = SignedPluginRepository::computeKeyFingerprint(zeros);
    auto fp2 = SignedPluginRepository::computeKeyFingerprint(zeros);
    EXPECT_EQ(fp1, fp2);
    // SHA-256 hex is always 64 lowercase hex chars
    EXPECT_EQ(64u, fp1.size());
    for (char c : fp1) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex char: " << c;
    }
}

TEST(KeyFingerprintTest, DifferentKeysGiveDifferentFingerprints) {
    std::vector<uint8_t> key1(32, 0xAB);
    std::vector<uint8_t> key2(32, 0xCD);
    EXPECT_NE(SignedPluginRepository::computeKeyFingerprint(key1),
              SignedPluginRepository::computeKeyFingerprint(key2));
}

// ============================================================================
// canonicalManifestJson
// ============================================================================

TEST(CanonicalJsonTest, Deterministic) {
    auto m = makeManifest("my_plugin", "2.3.4");
    std::string j1 = SignedPluginRepository::canonicalManifestJson(m);
    std::string j2 = SignedPluginRepository::canonicalManifestJson(m);
    EXPECT_EQ(j1, j2);
    EXPECT_FALSE(j1.empty());
}

TEST(CanonicalJsonTest, ChangeInNameChangesSerialization) {
    auto m1 = makeManifest("plugin_a");
    auto m2 = makeManifest("plugin_b");
    EXPECT_NE(SignedPluginRepository::canonicalManifestJson(m1),
              SignedPluginRepository::canonicalManifestJson(m2));
}

TEST(CanonicalJsonTest, ChangeInVersionChangesSerialization) {
    auto m1 = makeManifest("plugin_x", "1.0.0");
    auto m2 = makeManifest("plugin_x", "2.0.0");
    EXPECT_NE(SignedPluginRepository::canonicalManifestJson(m1),
              SignedPluginRepository::canonicalManifestJson(m2));
}

TEST(CanonicalJsonTest, ContainsExpectedFields) {
    auto m = makeManifest("my_plugin", "1.2.3");
    m.author  = "Alice";
    m.license = "Apache-2.0";
    std::string j = SignedPluginRepository::canonicalManifestJson(m);
    EXPECT_NE(std::string::npos, j.find("\"name\""));
    EXPECT_NE(std::string::npos, j.find("\"version\""));
    EXPECT_NE(std::string::npos, j.find("\"author\""));
    EXPECT_NE(std::string::npos, j.find("\"license\""));
}

// ============================================================================
// Key management
// ============================================================================

TEST_F(SignedPluginRepositoryTest, HasPinnedKeyReturnsTrueForActiveKey) {
    EXPECT_TRUE(repo_.hasPinnedKey(fp_));
}

TEST_F(SignedPluginRepositoryTest, HasPinnedKeyReturnsFalseForUnknown) {
    EXPECT_FALSE(repo_.hasPinnedKey("0000000000000000000000000000000000000000000000000000000000000000"));
}

TEST_F(SignedPluginRepositoryTest, RemovePinnedKeyReturnsTrueOnSuccess) {
    EXPECT_TRUE(repo_.removePinnedKey(fp_));
    EXPECT_FALSE(repo_.hasPinnedKey(fp_));
}

TEST_F(SignedPluginRepositoryTest, RemovePinnedKeyReturnsFalseWhenNotFound) {
    EXPECT_FALSE(repo_.removePinnedKey("nonexistent"));
}

TEST_F(SignedPluginRepositoryTest, GetPinnedKeysReturnsAddedKey) {
    auto keys = repo_.getPinnedKeys();
    ASSERT_EQ(1u, keys.size());
    EXPECT_EQ(fp_, keys[0].fingerprint);
    EXPECT_EQ("Test Key", keys[0].label);
    EXPECT_TRUE(keys[0].active);
}

TEST_F(SignedPluginRepositoryTest, InactiveKeyIsNotConsideredPinned) {
    // Add a second key that is inactive
    Ed25519KeyPair kp2 = generateKeyPair();
    std::string fp2 = SignedPluginRepository::computeKeyFingerprint(kp2.public_key);
    PinnedKey pk2;
    pk2.fingerprint = fp2;
    pk2.public_key  = kp2.public_key;
    pk2.active      = false;
    repo_.addPinnedKey(pk2);

    EXPECT_FALSE(repo_.hasPinnedKey(fp2));
}

TEST_F(SignedPluginRepositoryTest, AddPinnedKeyReplacesExisting) {
    PinnedKey updated;
    updated.fingerprint = fp_;
    updated.public_key  = kp_.public_key;
    updated.label       = "Updated Label";
    updated.active      = true;
    repo_.addPinnedKey(updated);

    auto keys = repo_.getPinnedKeys();
    ASSERT_EQ(1u, keys.size());
    EXPECT_EQ("Updated Label", keys[0].label);
}

// ============================================================================
// verifyEntry / addEntry
// ============================================================================

TEST_F(SignedPluginRepositoryTest, VerifyEntryAcceptsValidSignature) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    EXPECT_TRUE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsUnknownFingerprint) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    entry.key_fingerprint = "badfingerprint"; // key not pinned
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsInactiveKey) {
    // Deactivate the key and check that verification fails
    PinnedKey inactive;
    inactive.fingerprint = fp_;
    inactive.public_key  = kp_.public_key;
    inactive.active      = false;
    repo_.addPinnedKey(inactive);  // replaces active key

    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsTamperedName) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    entry.manifest.name = "malicious_plugin"; // tamper after signing
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsTamperedVersion) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    entry.manifest.version = "9.9.9"; // tamper after signing
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsMalformedBase64) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    entry.signature_b64 = "!!! not valid base64 !!!";
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, VerifyEntryRejectsEmptySignature) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);
    entry.signature_b64 = "";
    EXPECT_FALSE(repo_.verifyEntry(entry));
}

TEST_F(SignedPluginRepositoryTest, AddEntrySucceedsForValidEntry) {
    auto entry = makeEntry(kp_, fp_, makeManifest("blob_s3"));
    EXPECT_TRUE(repo_.addEntry(entry));
    EXPECT_EQ(1u, repo_.listEntries().size());
}

TEST_F(SignedPluginRepositoryTest, AddEntryFailsForInvalidSignature) {
    auto entry = makeEntry(kp_, fp_, makeManifest("blob_s3"));
    entry.manifest.name = "tampered";
    EXPECT_FALSE(repo_.addEntry(entry));
    EXPECT_EQ(0u, repo_.listEntries().size());
}

TEST_F(SignedPluginRepositoryTest, AddEntryReplacesExistingWithSameNameVersion) {
    auto manifest = makeManifest("blob_s3", "1.0.0");
    auto entry1   = makeEntry(kp_, fp_, manifest);
    manifest.description = "Updated description";
    auto entry2   = makeEntry(kp_, fp_, manifest);

    EXPECT_TRUE(repo_.addEntry(entry1));
    EXPECT_TRUE(repo_.addEntry(entry2));
    EXPECT_EQ(1u, repo_.listEntries().size());
    EXPECT_EQ("Updated description", repo_.listEntries()[0].manifest.description);
}

// ============================================================================
// findEntry / findByName
// ============================================================================

TEST_F(SignedPluginRepositoryTest, FindEntryByNameAndVersion) {
    auto e10 = makeEntry(kp_, fp_, makeManifest("plug", "1.0.0"));
    auto e20 = makeEntry(kp_, fp_, makeManifest("plug", "2.0.0"));
    repo_.addEntry(e10);
    repo_.addEntry(e20);

    auto found = repo_.findEntry("plug", "1.0.0");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ("1.0.0", found->manifest.version);
}

TEST_F(SignedPluginRepositoryTest, FindEntryLatestVersionWhenVersionOmitted) {
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("plug", "1.0.0")));
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("plug", "2.0.0")));
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("plug", "1.5.0")));

    auto found = repo_.findEntry("plug");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ("2.0.0", found->manifest.version);
}

TEST_F(SignedPluginRepositoryTest, FindEntryReturnsNulloptWhenNotFound) {
    auto result = repo_.findEntry("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SignedPluginRepositoryTest, FindByNameReturnsAllVersions) {
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("plug", "1.0.0")));
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("plug", "2.0.0")));
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("other", "1.0.0")));

    auto results = repo_.findByName("plug");
    ASSERT_EQ(2u, results.size());
    for (const auto& r : results) {
        EXPECT_EQ("plug", r.manifest.name);
    }
}

TEST_F(SignedPluginRepositoryTest, FindByNameReturnsEmptyWhenNotFound) {
    EXPECT_TRUE(repo_.findByName("ghost").empty());
}

// ============================================================================
// listEntries / clear
// ============================================================================

TEST_F(SignedPluginRepositoryTest, ListEntriesReturnsAllEntries) {
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("p1")));
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("p2")));
    EXPECT_EQ(2u, repo_.listEntries().size());
}

TEST_F(SignedPluginRepositoryTest, ClearRemovesAllEntries) {
    repo_.addEntry(makeEntry(kp_, fp_, makeManifest("p1")));
    repo_.clear();
    EXPECT_TRUE(repo_.listEntries().empty());
    // Pinned keys must be retained after clear()
    EXPECT_TRUE(repo_.hasPinnedKey(fp_));
}

// ============================================================================
// Multi-key scenarios
// ============================================================================

TEST_F(SignedPluginRepositoryTest, MultipleKeysIndependentlyVerify) {
    Ed25519KeyPair kp2  = generateKeyPair();
    std::string    fp2  = SignedPluginRepository::computeKeyFingerprint(kp2.public_key);
    PinnedKey pk2;
    pk2.fingerprint = fp2;
    pk2.public_key  = kp2.public_key;
    pk2.active      = true;
    repo_.addPinnedKey(pk2);

    auto entry_kp1 = makeEntry(kp_,  fp_,  makeManifest("plugin_a"));
    auto entry_kp2 = makeEntry(kp2,  fp2,  makeManifest("plugin_b"));

    EXPECT_TRUE(repo_.verifyEntry(entry_kp1));
    EXPECT_TRUE(repo_.verifyEntry(entry_kp2));

    // Cross-key verification must fail
    RepositoryEntry tampered = entry_kp1;
    tampered.key_fingerprint = fp2; // wrong key fingerprint
    EXPECT_FALSE(repo_.verifyEntry(tampered));
}

TEST_F(SignedPluginRepositoryTest, RemovingOneKeyDoesNotAffectOthers) {
    Ed25519KeyPair kp2  = generateKeyPair();
    std::string    fp2  = SignedPluginRepository::computeKeyFingerprint(kp2.public_key);
    PinnedKey pk2;
    pk2.fingerprint = fp2;
    pk2.public_key  = kp2.public_key;
    pk2.active      = true;
    repo_.addPinnedKey(pk2);

    repo_.removePinnedKey(fp2);

    auto entry_kp1 = makeEntry(kp_, fp_, makeManifest("plugin_a"));
    EXPECT_TRUE(repo_.verifyEntry(entry_kp1));
}

// ============================================================================
// Performance benchmark
// ============================================================================

// Validates the design target: Ed25519 signature verification < 1 ms per entry.
TEST_F(SignedPluginRepositoryTest, VerifyEntryMeetsPerformanceTarget) {
    auto manifest = makeManifest("perf_plugin", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);

    // Warm-up: one verification pass to allow JIT / cache effects to settle.
    repo_.verifyEntry(entry);

    const int ITERATIONS = 100;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        ASSERT_TRUE(repo_.verifyEntry(entry));
    }
    auto end = std::chrono::high_resolution_clock::now();

    double avg_us = std::chrono::duration<double, std::micro>(end - start).count()
                    / static_cast<double>(ITERATIONS);

    // Design target: < 1 ms (1000 µs) per verification.
    EXPECT_LT(avg_us, 1000.0)
        << "Ed25519 verification exceeded 1 ms target: " << avg_us << " µs average";
}

// Validates that deactivating a key after entry creation prevents insertion
// (no TOCTOU window in addEntry).
TEST_F(SignedPluginRepositoryTest, AddEntryRejectsEntryWhenKeyDeactivatedBeforeInsert) {
    auto manifest = makeManifest("deact_plugin", "1.0.0");
    auto entry    = makeEntry(kp_, fp_, manifest);

    // Deactivate the key so that addEntry's atomic verify+insert fails.
    PinnedKey inactive;
    inactive.fingerprint = fp_;
    inactive.public_key  = kp_.public_key;
    inactive.active      = false;
    repo_.addPinnedKey(inactive);

    EXPECT_FALSE(repo_.addEntry(entry));
    EXPECT_EQ(0u, repo_.listEntries().size());
}

// ============================================================================
// Thread-safety smoke test
// ============================================================================

TEST_F(SignedPluginRepositoryTest, ConcurrentAddEntriesAreThreadSafe) {
    const int N = 20;
    std::vector<RepositoryEntry> entries;
    entries.reserve(N);
    for (int i = 0; i < N; ++i) {
        entries.push_back(makeEntry(kp_, fp_,
            makeManifest("plug_" + std::to_string(i), "1.0.0")));
    }

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, &entries, i]() {
            repo_.addEntry(entries[static_cast<size_t>(i)]);
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(static_cast<size_t>(N), repo_.listEntries().size());
}
