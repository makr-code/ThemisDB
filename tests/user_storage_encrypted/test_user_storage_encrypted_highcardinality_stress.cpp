// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_user_storage_encrypted_highcardinality_stress.cpp
 * @brief Wave D — User Storage Encrypted High-Cardinality Stress Tests.
 *
 * High-cardinality stress tests for the user_storage_encrypted module:
 * - 100 000-record encrypted write under 8-thread concurrency
 * - Concurrent key rotation stress
 * - Enc/dec under load stress
 *
 * Labels: wave_d;stress;not_release_critical
 *
 * SIMULATION NOTE: All storage and cryptographic operations use in-process
 * stubs that model the production hot paths without requiring gocryptfs,
 * FUSE, or external key stores.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_USER_STORAGE_ENCRYPTED.md
 * @see src/user_storage_encrypted/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — StubEncryptedStoreStress
// ---------------------------------------------------------------------------
class StubEncryptedStoreStress {
public:
    void write(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = value;
        write_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void rotateKey() {
        std::lock_guard<std::mutex> lk(mu_);
        rotation_count_.fetch_add(1, std::memory_order_relaxed);
    }

    std::string encrypt(const std::string& plaintext) {
        enc_count_.fetch_add(1, std::memory_order_relaxed);
        return "ENC[" + plaintext + "]";
    }

    std::string decrypt(const std::string& ciphertext) {
        dec_count_.fetch_add(1, std::memory_order_relaxed);
        const std::string prefix = "ENC[";
        const std::string suffix = "]";
        if (ciphertext.size() > prefix.size() + suffix.size() &&
            ciphertext.substr(0, prefix.size()) == prefix &&
            ciphertext.substr(ciphertext.size() - suffix.size()) == suffix) {
            return ciphertext.substr(prefix.size(),
                                     ciphertext.size() - prefix.size() - suffix.size());
        }
        return ciphertext;
    }

    uint64_t writeCount()    const noexcept { return write_count_.load(std::memory_order_relaxed); }
    uint64_t rotationCount() const noexcept { return rotation_count_.load(std::memory_order_relaxed); }
    uint64_t encCount()      const noexcept { return enc_count_.load(std::memory_order_relaxed); }
    uint64_t decCount()      const noexcept { return dec_count_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
    std::atomic<uint64_t> write_count_{0};
    std::atomic<uint64_t> rotation_count_{0};
    std::atomic<uint64_t> enc_count_{0};
    std::atomic<uint64_t> dec_count_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test HighCardinalityEncryptedWrite
 * Writes 100 000 records across 8 concurrent threads and verifies that all
 * writes complete successfully (no exceptions, correct count).
 */
TEST(HighCardinalityEncryptedWrite, ConcurrentWrite) {
    static constexpr uint64_t kTotalRecords = 100'000;
    static constexpr int kThreads = 8;
    static constexpr uint64_t kPerThread = kTotalRecords / kThreads;

    StubEncryptedStoreStress store;
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kPerThread; ++i) {
                store.write("key_" + std::to_string(t) + "_" + std::to_string(i),
                            "value_" + std::to_string(i));
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(kTotalRecords, store.writeCount())
        << "[ENC_STORAGE] HighCardinalityEncryptedWrite: write count mismatch";
}

/**
 * @test ConcurrentKeyRotationStress
 * Runs 8 concurrent key rotation threads issuing 1 000 rotations each and
 * verifies that all 8 000 rotations complete without error.
 */
TEST(ConcurrentKeyRotationStress, MultiThreadedRotation) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kRotationsPerThread = 1'000;

    StubEncryptedStoreStress store;
    std::atomic<uint64_t> errors{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (uint64_t i = 0; i < kRotationsPerThread; ++i) {
                try { store.rotateKey(); }
                catch (...) { errors.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, errors.load())
        << "[ENC_STORAGE:KeyRotationFailed] Concurrent key rotation stress errors";
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kRotationsPerThread, store.rotationCount());
}

/**
 * @test EncDecUnderLoadStress
 * Runs 8 concurrent enc/dec threads each performing 10 000 round-trips and
 * verifies that every plaintext is recovered correctly.
 */
TEST(EncDecUnderLoadStress, RoundTripIntegrity) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 10'000;

    StubEncryptedStoreStress store;
    std::atomic<uint64_t> mismatch_count{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                const std::string plain = "msg_" + std::to_string(t) + "_" + std::to_string(i);
                const std::string cipher = store.encrypt(plain);
                const std::string recovered = store.decrypt(cipher);
                if (recovered != plain) {
                    mismatch_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, mismatch_count.load())
        << "[ENC_STORAGE:DecryptionOOM] Enc/dec round-trip failures under load";
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kOpsPerThread, store.encCount());
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kOpsPerThread, store.decCount());
}
