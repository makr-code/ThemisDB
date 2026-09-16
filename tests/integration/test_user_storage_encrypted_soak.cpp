// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_user_storage_encrypted_soak.cpp
 * @brief Wave D — User Storage Encrypted Soak Tests.
 *
 * Long-duration soak tests for the user_storage_encrypted module hot paths:
 * write/read throughput, key-rotation stability, and enc/dec reliability.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Write/read throughput ≥ 1 000 ops/sec (simulated, in-process stubs)
 * - No uncaught exceptions or data-race signals during soak duration
 * - Key rotation completes successfully on every iteration
 * - Enc/dec reliability: 100 % round-trip match over duration
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * SIMULATION NOTE: All storage, key-rotation, and enc/dec operations use
 * in-process stubs that model the production hot paths without requiring
 * gocryptfs, FUSE, or any external backend.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_USER_STORAGE_ENCRYPTED.md
 * @see src/user_storage_encrypted/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Soak duration helper
// ---------------------------------------------------------------------------
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// SIMULATION NOTE — StubEncryptedStore
// ---------------------------------------------------------------------------
class StubEncryptedStore {
public:
    void write(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = value;
        write_count_.fetch_add(1, std::memory_order_relaxed);
    }

    bool read(const std::string& key, std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) { return false; }
        out = it->second;
        read_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    void rotateKey() {
        std::lock_guard<std::mutex> lk(mu_);
        rotation_count_.fetch_add(1, std::memory_order_relaxed);
        // Simulate key rotation — re-encrypt all stored values in place.
        for (auto& [k, v] : store_) { v = "[rotated]" + v; }
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
        return ciphertext; // passthrough on malformed (acceptable in stub)
    }

    uint64_t writeCount()    const noexcept { return write_count_.load(std::memory_order_relaxed); }
    uint64_t readCount()     const noexcept { return read_count_.load(std::memory_order_relaxed); }
    uint64_t rotationCount() const noexcept { return rotation_count_.load(std::memory_order_relaxed); }
    uint64_t encCount()      const noexcept { return enc_count_.load(std::memory_order_relaxed); }
    uint64_t decCount()      const noexcept { return dec_count_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
    std::atomic<uint64_t> write_count_{0};
    std::atomic<uint64_t> read_count_{0};
    std::atomic<uint64_t> rotation_count_{0};
    std::atomic<uint64_t> enc_count_{0};
    std::atomic<uint64_t> dec_count_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test EncryptedStorageSoak_WriteReadThroughput
 * Verifies that write/read throughput stays above 1 000 ops/sec over the
 * configured soak duration using 4 concurrent writer + 4 reader threads.
 */
TEST(EncryptedStorageSoak, WriteReadThroughput) {
    StubEncryptedStore store;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 4;

    std::atomic<bool> stop{false};
    std::vector<std::thread> writers, readers;

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(durationMs);

    for (int t = 0; t < kWorkers; ++t) {
        writers.emplace_back([&, t]() {
            std::mt19937_64 rng(static_cast<uint64_t>(t) * 0x9e3779b97f4a7c15ULL);
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                store.write("key_" + std::to_string(t) + "_" + std::to_string(idx++),
                            "value_" + std::to_string(rng()));
            }
        });
        readers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                std::string out;
                store.read("key_" + std::to_string(t) + "_" + std::to_string(idx++), out);
            }
        });
    }

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_relaxed);

    for (auto& th : writers) th.join();
    for (auto& th : readers) th.join();

    const double elapsed_s = static_cast<double>(durationMs) / 1000.0;
    const double write_ops_per_sec = static_cast<double>(store.writeCount()) / elapsed_s;
    const double read_ops_per_sec  = static_cast<double>(store.readCount())  / elapsed_s;

    EXPECT_GT(write_ops_per_sec, 1000.0)
        << "[ENC_STORAGE] WriteReadThroughput: write_ops_per_sec=" << write_ops_per_sec;
    EXPECT_GT(read_ops_per_sec, 1000.0)
        << "[ENC_STORAGE] WriteReadThroughput: read_ops_per_sec=" << read_ops_per_sec;
}

/**
 * @test EncryptedStorageSoak_KeyRotationStability
 * Runs continuous key rotation while writers are active and verifies that no
 * rotations fail (exception-free) over the soak duration.
 */
TEST(EncryptedStorageSoak, KeyRotationStability) {
    StubEncryptedStore store;
    const uint64_t durationMs = soakDurationMs();

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> rotation_errors{0};

    // Background writer to keep the store non-empty.
    std::thread writer([&]() {
        uint64_t idx = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            store.write("kr_key_" + std::to_string(idx), "v" + std::to_string(idx));
            ++idx;
            std::this_thread::sleep_for(1ms);
        }
    });

    // Rotation thread.
    std::thread rotator([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            try { store.rotateKey(); }
            catch (...) { rotation_errors.fetch_add(1, std::memory_order_relaxed); }
            std::this_thread::sleep_for(10ms);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    writer.join();
    rotator.join();

    EXPECT_EQ(0u, rotation_errors.load())
        << "[ENC_STORAGE:KeyRotationFailed] rotation errors detected during soak";
    EXPECT_GT(store.rotationCount(), 0u)
        << "[ENC_STORAGE] KeyRotationStability: no rotations completed";
}

/**
 * @test EncryptedStorageSoak_EncDecReliability
 * Verifies that enc/dec round-trips produce 100 % matching results over the
 * soak duration using 4 concurrent threads.
 */
TEST(EncryptedStorageSoak, EncDecReliability) {
    StubEncryptedStore store;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 4;

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> mismatch_count{0};

    std::vector<std::thread> workers;
    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                const std::string plain = "msg_" + std::to_string(t) + "_" + std::to_string(idx++);
                const std::string cipher = store.encrypt(plain);
                const std::string recovered = store.decrypt(cipher);
                if (recovered != plain) {
                    mismatch_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, mismatch_count.load())
        << "[ENC_STORAGE:DecryptionOOM] enc/dec round-trip mismatches during soak";
    EXPECT_GT(store.encCount(), 0u);
    EXPECT_GT(store.decCount(), 0u);
}
