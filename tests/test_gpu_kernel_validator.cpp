#include <gtest/gtest.h>
#include "themis/gpu/kernel_validator.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::gpu;

// Reset singleton before each test.
class KernelValidatorTest : public ::testing::Test {
protected:
    void SetUp()    override { GPUKernelValidator::GetInstance().reset(); }
    void TearDown() override { GPUKernelValidator::GetInstance().reset(); }
};

// Convenience blob builder.
static std::vector<uint8_t> makeBlob(const std::string& s) {
    return {s.begin(), s.end()};
}

// ---------------------------------------------------------------------------
// Checksum computation
// ---------------------------------------------------------------------------

TEST(GPUKernelValidatorChecksumTest, EmptyBlob_HasKnownValue) {
    // FNV-1a of empty input is the offset basis.
    const uint64_t cs = GPUKernelValidator::computeChecksum({});
    EXPECT_EQ(cs, 14695981039346656037ULL);
}

TEST(GPUKernelValidatorChecksumTest, SameData_SameChecksum) {
    const auto b1 = makeBlob("hello_kernel");
    const auto b2 = makeBlob("hello_kernel");
    EXPECT_EQ(GPUKernelValidator::computeChecksum(b1),
              GPUKernelValidator::computeChecksum(b2));
}

TEST(GPUKernelValidatorChecksumTest, DifferentData_DifferentChecksum) {
    EXPECT_NE(GPUKernelValidator::computeChecksum(makeBlob("kernel_a")),
              GPUKernelValidator::computeChecksum(makeBlob("kernel_b")));
}

TEST(GPUKernelValidatorChecksumTest, RawPointer_MatchesVector) {
    const auto blob = makeBlob("test");
    const uint64_t v1 = GPUKernelValidator::computeChecksum(blob);
    const uint64_t v2 = GPUKernelValidator::computeChecksum(
        blob.data(), blob.size());
    EXPECT_EQ(v1, v2);
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Register_Explicit_Checksum) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("k1", 12345ULL);
    EXPECT_TRUE(kv.isRegistered("k1"));
    EXPECT_EQ(kv.registeredKernels().size(), 1u);
}

TEST_F(KernelValidatorTest, Register_FromBlob) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto blob = makeBlob("kernel_code");
    kv.registerKernel("k2", blob);
    EXPECT_TRUE(kv.isRegistered("k2"));
}

TEST_F(KernelValidatorTest, Unregister_RemovesKernel) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("k3", 999ULL);
    EXPECT_TRUE(kv.isRegistered("k3"));
    kv.unregisterKernel("k3");
    EXPECT_FALSE(kv.isRegistered("k3"));
}

TEST_F(KernelValidatorTest, Unregister_UnknownKernel_NoOp) {
    auto& kv = GPUKernelValidator::GetInstance();
    EXPECT_NO_THROW(kv.unregisterKernel("does_not_exist"));
}

TEST_F(KernelValidatorTest, RegisteredKernels_ContainsAllIds) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("a", 1ULL);
    kv.registerKernel("b", 2ULL);
    kv.registerKernel("c", 3ULL);
    const auto ids = kv.registeredKernels();
    EXPECT_EQ(ids.size(), 3u);
}

// ---------------------------------------------------------------------------
// Validation — success
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Validate_OK_WhenBlobMatchesRegistered) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto blob = makeBlob("vector_dot_fp32_impl");
    kv.registerKernel("vec_dot", blob);
    const auto r = kv.validate("vec_dot", blob);
    EXPECT_EQ(r.status, GPUKernelValidator::Status::OK);
    EXPECT_EQ(r.message, "OK");
}

TEST_F(KernelValidatorTest, IsValid_ReturnsTrueOnMatch) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto blob = makeBlob("some_kernel");
    kv.registerKernel("my_k", blob);
    EXPECT_TRUE(kv.isValid("my_k", blob));
}

// ---------------------------------------------------------------------------
// Validation — failure: unknown kernel
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Validate_UnknownKernel_Rejected) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto r = kv.validate("unknown", makeBlob("data"));
    EXPECT_EQ(r.status, GPUKernelValidator::Status::UNKNOWN_KERNEL);
    EXPECT_FALSE(r.message.empty());
    EXPECT_NE(r.message.find("whitelist"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Validation — failure: checksum mismatch
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Validate_ChecksumMismatch_Rejected) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("k", makeBlob("original"));
    const auto r = kv.validate("k", makeBlob("tampered!"));
    EXPECT_EQ(r.status, GPUKernelValidator::Status::CHECKSUM_MISMATCH);
    EXPECT_NE(r.computed_checksum, r.expected_checksum);
}

TEST_F(KernelValidatorTest, Validate_Mismatch_ContainsChecksums) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("k", 999ULL);
    const auto r = kv.validate("k", makeBlob("wrong"));
    EXPECT_EQ(r.expected_checksum, 999ULL);
    EXPECT_NE(r.computed_checksum, 999ULL);
}

// ---------------------------------------------------------------------------
// Validation — failure: empty blob
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Validate_EmptyBlob_Rejected) {
    auto& kv = GPUKernelValidator::GetInstance();
    kv.registerKernel("k", 42ULL);
    const auto r = kv.validate("k", {});
    EXPECT_EQ(r.status, GPUKernelValidator::Status::EMPTY_BLOB);
}

TEST_F(KernelValidatorTest, Validate_EmptyBlob_UnknownKernel_Rejected) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto r = kv.validate("missing", {});
    EXPECT_EQ(r.status, GPUKernelValidator::Status::EMPTY_BLOB);
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Stats_CountsCorrectly) {
    auto& kv = GPUKernelValidator::GetInstance();
    const auto blob = makeBlob("impl");
    kv.registerKernel("k", blob);

    kv.validate("k", blob);               // OK
    kv.validate("k", makeBlob("tamper")); // CHECKSUM_MISMATCH
    kv.validate("nope", makeBlob("x"));   // UNKNOWN
    kv.validate("k", {});                 // EMPTY

    const auto s = kv.getStats();
    EXPECT_EQ(s.total_validations, 4u);
    EXPECT_EQ(s.ok_count, 1u);
    EXPECT_EQ(s.checksum_mismatch_count, 1u);
    EXPECT_EQ(s.unknown_kernel_count, 1u);
    EXPECT_EQ(s.empty_blob_count, 1u);
    EXPECT_EQ(s.registered_count, 1u);
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST_F(KernelValidatorTest, Concurrent_RegisterAndValidate_NoDataRace) {
    auto& kv = GPUKernelValidator::GetInstance();
    constexpr int THREADS = 8, OPS_PER_THREAD = 20;
    std::atomic<int> ok_count{0};

    auto worker = [&](int id) {
        const std::string kid = "ker_" + std::to_string(id);
        const auto blob = makeBlob("blob_" + std::to_string(id));
        kv.registerKernel(kid, blob);
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            if (kv.isValid(kid, blob)) {
              ok_count.fetch_add(1);
            }
        }
    };

    std::vector<std::thread> threads = {};

    for (int t = 0; t < THREADS; ++t) {
      threads.emplace_back(worker, t);
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_GT(ok_count.load(), 0);
}
