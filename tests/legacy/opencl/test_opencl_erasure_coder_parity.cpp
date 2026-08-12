/**
 * ThemisDB OpenCL Erasure Coder – CPU/GPU Parity Tests
 *
 * Tests the GpuErasureCoderOpenCL implementation (Issue #105, v1.8.0).
 *
 * Coverage:
 *  - Encode / decode round-trip with 1, 2, and 3 erasures
 *  - CPU/GPU parity: OpenCL impl results match ReedSolomonCoder (CPU)
 *  - batchEncode correctness
 *  - isAvailable() / initialize() contract
 *
 * When THEMIS_ENABLE_OPENCL is defined (and a pocl or native OpenCL ICD is
 * present) the GPU-assisted paths are exercised and compared against the
 * CPU reference.  When OpenCL is absent all OpenCL-specific tests are skipped
 * via GTEST_SKIP(), ensuring clean CI runs without GPU hardware.
 */

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_OPENCL

#include "sharding/gpu_erasure_coder.h"
#include "sharding/redundancy_strategy.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::sharding;

namespace themis { namespace sharding { 
std::unique_ptr<GPUErasureCoderImpl> createOpenCLErasureCoder(
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm);
} } // namespace themis::sharding
// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace {

/// Deterministic data generator.
std::vector<uint8_t> makeData(size_t size, uint8_t seed = 0xA5) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i)
        data[i] = static_cast<uint8_t>((seed + i) & 0xFF);
    return data;
}

/// Build an available-chunks map from a full chunks vector, removing the
/// shard indices listed in `erase_idx`.
std::map<uint32_t, std::vector<uint8_t>>
makeAvailable(const std::vector<std::vector<uint8_t>>& chunks,
              const std::vector<uint32_t>& erase_idx) {
    std::set<uint32_t> erased(erase_idx.begin(), erase_idx.end());
    std::map<uint32_t, std::vector<uint8_t>> avail;
    for (uint32_t i = 0; i < static_cast<uint32_t>(chunks.size()); ++i)
        if (!erased.count(i))
            avail[i] = chunks[i];
    return avail;
}

} // namespace

// ─── Test Fixture ─────────────────────────────────────────────────────────────

class OpenCLErasureCoderParityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Build the OpenCL coder under test
        opencl_impl_ = createOpenCLErasureCoder(
            GPUConfig{}, ErasureCodingAlgorithm::REED_SOLOMON);
        ASSERT_NE(opencl_impl_, nullptr);

        gpu_available_ = opencl_impl_->initialize(GPUConfig{});

        // CPU reference coder
        cpu_coder_ = ErasureCoder::create(ErasureCodingAlgorithm::REED_SOLOMON);
        ASSERT_NE(cpu_coder_, nullptr);
    }

    void TearDown() override {
        opencl_impl_.reset();
        cpu_coder_.reset();
    }

    // Encode with the OpenCL impl (using its internal CPU fallback when no
    // device is available).
    std::vector<std::vector<uint8_t>> openclEncode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards)
    {
        return opencl_impl_->encode(data, data_shards, parity_shards);
    }

    // Decode with the OpenCL impl.
    std::vector<uint8_t> openclDecode(
        const std::map<uint32_t, std::vector<uint8_t>>& avail,
        const std::vector<uint32_t>& missing,
        uint32_t data_shards,
        uint32_t parity_shards)
    {
        return opencl_impl_->decode(avail, missing, data_shards, parity_shards);
    }

    std::unique_ptr<GPUErasureCoderImpl> opencl_impl_;
    std::unique_ptr<ErasureCoder>        cpu_coder_;
    bool gpu_available_ = false;
};

// ─── Encode / Decode round-trip tests ─────────────────────────────────────────

/// Helper: encode, erase `erase_idx` shards, decode and verify recovery.
static void roundTrip(GPUErasureCoderImpl& coder,
                      const std::vector<uint8_t>& original,
                      uint32_t data_shards,
                      uint32_t parity_shards,
                      const std::vector<uint32_t>& erase_idx)
{
    auto chunks = coder.encode(original, data_shards, parity_shards);
    ASSERT_EQ(chunks.size(), static_cast<size_t>(data_shards + parity_shards));

    auto avail = makeAvailable(chunks, erase_idx);
    auto recovered = coder.decode(avail, erase_idx, data_shards, parity_shards);

    // Recovered flat data may be padded to a multiple of data_shards – trim.
    recovered.resize(original.size());
    ASSERT_EQ(recovered, original)
        << "Round-trip failed with " << erase_idx.size() << " erasure(s)";
}

// ── 1 erasure ─────────────────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_1Erasure_DataShard) {
    auto data = makeData(1024);
    // Erase the first data shard (index 0)
    roundTrip(*opencl_impl_, data, /*data_shards=*/4, /*parity_shards=*/2,
              /*erase_idx=*/{0});
}

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_1Erasure_ParityShard) {
    auto data = makeData(1024);
    // Erase the first parity shard (index == data_shards)
    roundTrip(*opencl_impl_, data, 4, 2, {4});
}

// ── 2 erasures ────────────────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_2Erasures_BothData) {
    auto data = makeData(1024, 0x3C);
    roundTrip(*opencl_impl_, data, 4, 2, {0, 1});
}

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_2Erasures_MixedDataParity) {
    auto data = makeData(1024, 0x7F);
    roundTrip(*opencl_impl_, data, 4, 2, {2, 4});
}

// ── 3 erasures ────────────────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_3Erasures) {
    // RS(7,3): 7 data + 3 parity → can recover up to 3 erasures
    auto data = makeData(2048, 0xBC);
    roundTrip(*opencl_impl_, data, 7, 3, {0, 3, 6});
}

TEST_F(OpenCLErasureCoderParityTest, RoundTrip_3Erasures_AllParity) {
    auto data = makeData(2048, 0xDE);
    roundTrip(*opencl_impl_, data, 7, 3, {7, 8, 9});
}

// ─── CPU / GPU parity tests ──────────────────────────────────────────────────
// When an OpenCL device is available, encode results must be byte-identical
// to the CPU (ReedSolomonCoder) reference.  Skipped when no device present.

TEST_F(OpenCLErasureCoderParityTest, Parity_Encode_1Erasure) {
    if (!gpu_available_) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_device_for_gpu_parity";
    }
    auto data = makeData(4096, 0x11);
    const uint32_t ds = 4, ps = 2;

    auto cpu_chunks    = cpu_coder_->encode(data, ds, ps);
    auto opencl_chunks = openclEncode(data, ds, ps);

    ASSERT_EQ(cpu_chunks.size(), opencl_chunks.size());
    for (size_t i = 0; i < cpu_chunks.size(); ++i)
        EXPECT_EQ(cpu_chunks[i], opencl_chunks[i])
            << "Chunk " << i << " mismatch between CPU and OpenCL";
}

TEST_F(OpenCLErasureCoderParityTest, Parity_EncodeDecodeRoundTrip_1Erasure) {
    if (!gpu_available_) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_device_for_gpu_parity";
    }
    auto data = makeData(4096, 0x22);
    const uint32_t ds = 4, ps = 2;

    // Encode with OpenCL
    auto opencl_chunks = openclEncode(data, ds, ps);
    auto avail = makeAvailable(opencl_chunks, {1});
    auto recovered = openclDecode(avail, {1}, ds, ps);
    recovered.resize(data.size());
    EXPECT_EQ(data, recovered);
}

TEST_F(OpenCLErasureCoderParityTest, Parity_EncodeDecodeRoundTrip_2Erasures) {
    if (!gpu_available_) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_device_for_gpu_parity";
    }
    auto data = makeData(4096, 0x33);
    const uint32_t ds = 4, ps = 2;

    auto opencl_chunks = openclEncode(data, ds, ps);
    auto avail = makeAvailable(opencl_chunks, {0, 3});
    auto recovered = openclDecode(avail, {0, 3}, ds, ps);
    recovered.resize(data.size());
    EXPECT_EQ(data, recovered);
}

TEST_F(OpenCLErasureCoderParityTest, Parity_EncodeDecodeRoundTrip_3Erasures) {
    if (!gpu_available_) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_device_for_gpu_parity";
    }
    auto data = makeData(4096, 0x44);
    const uint32_t ds = 7, ps = 3;

    auto opencl_chunks = openclEncode(data, ds, ps);
    auto avail = makeAvailable(opencl_chunks, {1, 4, 6});
    auto recovered = openclDecode(avail, {1, 4, 6}, ds, ps);
    recovered.resize(data.size());
    EXPECT_EQ(data, recovered);
}

// ─── batchEncode tests ────────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, BatchEncode_ResultCount) {
    const uint32_t ds = 3, ps = 2;
    std::vector<std::vector<uint8_t>> blocks;
    for (int i = 0; i < 4; ++i)
        blocks.push_back(makeData(512, static_cast<uint8_t>(i)));

    auto results = opencl_impl_->batchEncode(blocks, ds, ps);
    ASSERT_EQ(results.size(), blocks.size());
    for (const auto& r : results)
        EXPECT_EQ(r.size(), static_cast<size_t>(ds + ps));
}

TEST_F(OpenCLErasureCoderParityTest, BatchEncode_MatchesSingleEncode) {
    const uint32_t ds = 3, ps = 2;
    std::vector<std::vector<uint8_t>> blocks;
    for (int i = 0; i < 3; ++i)
        blocks.push_back(makeData(512, static_cast<uint8_t>(0x10 + i)));

    auto batch   = opencl_impl_->batchEncode(blocks, ds, ps);
    for (size_t b = 0; b < blocks.size(); ++b) {
        auto single = opencl_impl_->encode(blocks[b], ds, ps);
        ASSERT_EQ(single.size(), batch[b].size()) << "block " << b;
        for (size_t c = 0; c < single.size(); ++c)
            EXPECT_EQ(single[c], batch[b][c])
                << "block " << b << " chunk " << c;
    }
}

// ─── isAvailable contract ────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, IsAvailable_ReflectsInit) {
    // A freshly built, not-yet-initialized impl must not be available
    auto fresh = createOpenCLErasureCoder(
        GPUConfig{}, ErasureCodingAlgorithm::REED_SOLOMON);
    ASSERT_NE(fresh, nullptr);
    // Do NOT call initialize() — should report unavailable
    EXPECT_FALSE(fresh->isAvailable());
}

// ─── Edge cases ───────────────────────────────────────────────────────────────

TEST_F(OpenCLErasureCoderParityTest, Encode_EmptyData) {
    auto chunks = openclEncode({}, 2, 1);
    EXPECT_EQ(chunks.size(), 3u);
}

TEST_F(OpenCLErasureCoderParityTest, Encode_SingleByte) {
    auto chunks = openclEncode({0x42}, 2, 1);
    EXPECT_EQ(chunks.size(), 3u);
}

#else  // THEMIS_ENABLE_OPENCL not defined

// When OpenCL is not compiled in, provide a single placeholder test so the
// test binary is valid and clearly communicates why it was skipped.
TEST(OpenCLErasureCoderParityTest, SkippedNoBuildFlag) {
    GTEST_SKIP() << "capability:opencl_compiled=false;reason=themis_enable_opencl_not_defined";
}

#endif // THEMIS_ENABLE_OPENCL
