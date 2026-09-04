#include <gtest/gtest.h>
#include "config/config_encrypted_store.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace config {
namespace test {

// ============================================================================
// Fixture
// ============================================================================

class ConfigEncryptedStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<ConfigEncryptedStore>();
    }

    std::unique_ptr<ConfigEncryptedStore> store_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, NewStoreIsEmpty) {
    EXPECT_EQ(store_->size(), 0u);
    EXPECT_TRUE(store_->keys().empty());
}

TEST_F(ConfigEncryptedStoreTest, InitialKeyVersionIsOne) {
    EXPECT_EQ(store_->currentKeyVersion(), 1u);
}

// ============================================================================
// Basic set / get
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, SetAndGetRoundtrip) {
    store_->set("db_password", "hunter2");
    EXPECT_EQ(store_->get("db_password"), "hunter2");
}

TEST_F(ConfigEncryptedStoreTest, StoredValueIsNotPlaintext) {
    // The value must be stored in encrypted form.
    // We verify indirectly: the serialised JSON must NOT contain the raw password.
    store_->set("secret", "my_super_secret");
    const std::string snapshot = store_->serialize();
    EXPECT_EQ(snapshot.find("my_super_secret"), std::string::npos)
        << "Plaintext password found in serialised store – encryption is broken";
}

TEST_F(ConfigEncryptedStoreTest, MultipleValuesRoundtrip) {
    store_->set("key_a", "value_a");
    store_->set("key_b", "value_b");
    store_->set("key_c", "value_c");

    EXPECT_EQ(store_->get("key_a"), "value_a");
    EXPECT_EQ(store_->get("key_b"), "value_b");
    EXPECT_EQ(store_->get("key_c"), "value_c");
}

TEST_F(ConfigEncryptedStoreTest, OverwriteExistingKey) {
    store_->set("token", "old_token");
    store_->set("token", "new_token");
    EXPECT_EQ(store_->get("token"), "new_token");
    EXPECT_EQ(store_->size(), 1u);
}

TEST_F(ConfigEncryptedStoreTest, EmptyValueRoundtrip) {
    store_->set("empty_key", "");
    EXPECT_EQ(store_->get("empty_key"), "");
}

TEST_F(ConfigEncryptedStoreTest, LargeValueRoundtrip) {
    std::string large_value(64 * 1024, 'X'); // 64 KB
    store_->set("large", large_value);
    EXPECT_EQ(store_->get("large"), large_value);
}

TEST_F(ConfigEncryptedStoreTest, UnicodeValueRoundtrip) {
    // UTF-8 encoded string: "Ünïcödé vàlùé: hello-world key"
    std::string unicode;
    unicode += '\xC3'; unicode += '\x9C'; // Ü
    unicode += 'n';
    unicode += '\xC3'; unicode += '\xAF'; // ï
    unicode += 'c';
    unicode += '\xC3'; unicode += '\xB6'; // ö
    unicode += 'd';
    unicode += '\xC3'; unicode += '\xA9'; // é
    unicode += " value key";
    store_->set("unicode_key", unicode);
    EXPECT_EQ(store_->get("unicode_key"), unicode);
}

TEST_F(ConfigEncryptedStoreTest, BinaryLikeValueRoundtrip) {
    // Build a value containing null bytes and non-printable characters.
    std::string binary_val("before\x00after", 13); // includes the embedded null byte
    binary_val.push_back('\x01');
    binary_val.push_back('\xFF');
    store_->set("binary_key", binary_val);
    EXPECT_EQ(store_->get("binary_key"), binary_val);
}

// ============================================================================
// tryGet
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, TryGetReturnsValueWhenPresent) {
    store_->set("present", "val");
    auto result = store_->tryGet("present");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "val");
}

TEST_F(ConfigEncryptedStoreTest, TryGetReturnsNulloptWhenAbsent) {
    auto result = store_->tryGet("missing");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Error cases
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, GetMissingKeyThrows) {
    EXPECT_THROW(store_->get("nonexistent"), ConfigKeyNotFoundException);
}

TEST_F(ConfigEncryptedStoreTest, SetEmptyKeyThrows) {
    EXPECT_THROW(store_->set("", "value"), std::invalid_argument);
}

// ============================================================================
// contains / remove / clear / keys
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, ContainsReturnsFalseForMissingKey) {
    EXPECT_FALSE(store_->contains("missing"));
}

TEST_F(ConfigEncryptedStoreTest, ContainsReturnsTrueAfterSet) {
    store_->set("k", "v");
    EXPECT_TRUE(store_->contains("k"));
}

TEST_F(ConfigEncryptedStoreTest, RemoveReturnsTrueWhenKeyExists) {
    store_->set("removable", "val");
    EXPECT_TRUE(store_->remove("removable"));
    EXPECT_FALSE(store_->contains("removable"));
}

TEST_F(ConfigEncryptedStoreTest, RemoveReturnsFalseWhenKeyMissing) {
    EXPECT_FALSE(store_->remove("absent"));
}

TEST_F(ConfigEncryptedStoreTest, ClearRemovesAllEntries) {
    store_->set("a", "1");
    store_->set("b", "2");
    store_->clear();
    EXPECT_EQ(store_->size(), 0u);
    EXPECT_FALSE(store_->contains("a"));
    EXPECT_FALSE(store_->contains("b"));
}

TEST_F(ConfigEncryptedStoreTest, KeysReturnsAllStoredKeys) {
    store_->set("x", "1");
    store_->set("y", "2");
    store_->set("z", "3");

    auto k = store_->keys();
    ASSERT_EQ(k.size(), 3u);
    std::sort(k.begin(), k.end());
    EXPECT_EQ(k[0], "x");
    EXPECT_EQ(k[1], "y");
    EXPECT_EQ(k[2], "z");
}

// ============================================================================
// Key rotation
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, RotateKeyIncrementsVersion) {
    EXPECT_EQ(store_->currentKeyVersion(), 1u);
    const uint32_t new_version = store_->rotateKey();
    EXPECT_EQ(new_version, 2u);
    EXPECT_EQ(store_->currentKeyVersion(), 2u);
}

TEST_F(ConfigEncryptedStoreTest, ValuesAccessibleAfterKeyRotation) {
    store_->set("cred_a", "password1");
    store_->set("cred_b", "secret2");

    store_->rotateKey();

    EXPECT_EQ(store_->get("cred_a"), "password1");
    EXPECT_EQ(store_->get("cred_b"), "secret2");
}

TEST_F(ConfigEncryptedStoreTest, MultipleRotationsPreserveValues) {
    store_->set("persistent", "stays_alive");

    for (int i = 0; i < 5; ++i) {
        store_->rotateKey();
    }

    EXPECT_EQ(store_->currentKeyVersion(), 6u);
    EXPECT_EQ(store_->get("persistent"), "stays_alive");
}

TEST_F(ConfigEncryptedStoreTest, RotateKeyOnEmptyStoreSucceeds) {
    EXPECT_NO_THROW(store_->rotateKey());
    EXPECT_EQ(store_->currentKeyVersion(), 2u);
}

TEST_F(ConfigEncryptedStoreTest, NewValuesEncryptedWithLatestKeyAfterRotation) {
    store_->set("before", "before_value");
    store_->rotateKey();
    store_->set("after", "after_value");

    EXPECT_EQ(store_->get("before"), "before_value");
    EXPECT_EQ(store_->get("after"),  "after_value");
}

// ============================================================================
// Serialisation / deserialisation
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, SerializeDeserializeRoundtrip) {
    store_->set("host", "localhost");
    store_->set("port", "5432");
    store_->set("pass", "s3cr3t!");

    const std::string snapshot = store_->serialize();

    ConfigEncryptedStore restored;
    restored.deserialize(snapshot);

    EXPECT_EQ(restored.get("host"), "localhost");
    EXPECT_EQ(restored.get("port"), "5432");
    EXPECT_EQ(restored.get("pass"), "s3cr3t!");
    EXPECT_EQ(restored.size(),       3u);
}

TEST_F(ConfigEncryptedStoreTest, DeserializeRestoresKeyVersion) {
    store_->rotateKey(); // version 2
    store_->set("val", "data");

    ConfigEncryptedStore restored;
    restored.deserialize(store_->serialize());

    EXPECT_EQ(restored.currentKeyVersion(), 2u);
    EXPECT_EQ(restored.get("val"), "data");
}

TEST_F(ConfigEncryptedStoreTest, DeserializeEmptyStoreSnapshot) {
    const std::string snapshot = store_->serialize();

    ConfigEncryptedStore restored;
    EXPECT_NO_THROW(restored.deserialize(snapshot));
    EXPECT_EQ(restored.size(), 0u);
}

TEST_F(ConfigEncryptedStoreTest, DeserializeMalformedJsonThrows) {
    EXPECT_THROW(store_->deserialize("not json at all"),
                 ConfigEncryptionException);
}

TEST_F(ConfigEncryptedStoreTest, DeserializeTruncatedJsonThrows) {
    EXPECT_THROW(store_->deserialize("{\"key_version\":1"), 
                 ConfigEncryptionException);
}

// ============================================================================
// Encryption correctness
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, TwoEncryptionsOfSamePlaintextDiffer) {
    // Identical plaintexts must produce different ciphertexts due to random IV.
    store_->set("k1", "same_value");
    store_->set("k2", "same_value");

    const std::string snap1 = store_->serialize();
    // Serialise again after re-encrypting k1 with a fresh set.
    store_->set("k1", "same_value");
    const std::string snap2 = store_->serialize();

    // The two JSON snapshots must differ (different IVs).
    EXPECT_NE(snap1, snap2)
        << "Two encryptions of the same plaintext produced identical ciphertexts";
}

// ============================================================================
// Thread safety
// ============================================================================

TEST_F(ConfigEncryptedStoreTest, ConcurrentSetGetIsThreadSafe) {
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                const std::string key = "thread_" + std::to_string(t) +
                                        "_key_" + std::to_string(i);
                const std::string val = "value_" + std::to_string(t * 1000 + i);
                store_->set(key, val);
                EXPECT_EQ(store_->get(key), val);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(store_->size(), static_cast<std::size_t>(kThreads * kOpsPerThread));
}

TEST_F(ConfigEncryptedStoreTest, ConcurrentRotationIsThreadSafe) {
    // Pre-populate a few keys.
    for (int i = 0; i < 10; ++i) {
        store_->set("key_" + std::to_string(i), "val_" + std::to_string(i));
    }

    // Launch threads: some rotate, some read, some write.
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 4; ++t) {
        if (t % 2 == 0) {
            threads.emplace_back([&]() {
                for (int i = 0; i < 3; ++i) {
                    store_->rotateKey();
                }
            });
        } else {
            threads.emplace_back([&]() {
                for (int i = 0; i < 5; ++i) {
                    store_->set("extra_" + std::to_string(i),
                                "v" + std::to_string(i));
                }
            });
        }
    }

    for (auto& th : threads) {
        th.join();
    }

    // After all threads finish, the store must be self-consistent.
    EXPECT_NO_THROW({
        for (const auto& k : store_->keys()) {
            store_->get(k); // must not throw
        }
    });
}

TEST_F(ConfigEncryptedStoreTest, ConcurrentReadersDoNotBlockEachOther) {
    // Populate a key to read.
    store_->set("shared_key", "shared_value");

    constexpr int kReaders = 16;
    std::vector<std::thread> threads;
    threads.reserve(kReaders);
    std::atomic<bool> start_flag{false};
    std::atomic<int> readers_done{0};

    for (int t = 0; t < kReaders; ++t) {
        threads.emplace_back([&]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            EXPECT_EQ(store_->get("shared_key"), "shared_value");
            EXPECT_TRUE(store_->contains("shared_key"));
            EXPECT_GE(store_->size(), 1u);
            readers_done.fetch_add(1, std::memory_order_relaxed);
        });
    }

    start_flag.store(true, std::memory_order_release);

    for (auto& th : threads) {
        th.join();
    }

    // All readers must have completed successfully.
    EXPECT_EQ(readers_done.load(), kReaders);
    EXPECT_EQ(store_->get("shared_key"), "shared_value");
}

} // namespace test
} // namespace config
} // namespace themis
