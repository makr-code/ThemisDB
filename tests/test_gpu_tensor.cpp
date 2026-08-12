#include "themis/gpu/tensor_buffer.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

using namespace themis::gpu;

// ============================================================================
// Fixture
// ============================================================================

class GPUTensorBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        GPUTensorBuffer::resetGlobalStats();
    }
};

// ============================================================================
// Shape helpers
// ============================================================================

TEST_F(GPUTensorBufferTest, ShapeNumElements) {
    GPUTensorBuffer::Shape s{{2, 3, 4}};
    EXPECT_EQ(s.numElements(), 24u);
}

TEST_F(GPUTensorBufferTest, ShapeNumElementsEmpty) {
    GPUTensorBuffer::Shape s{{}};
    EXPECT_EQ(s.numElements(), 0u);
}

TEST_F(GPUTensorBufferTest, ShapeElementBytes) {
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::FLOAT32),  4u);
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::FLOAT16),  2u);
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::BFLOAT16), 2u);
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::INT32),    4u);
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::INT8),     1u);
    EXPECT_EQ(GPUTensorBuffer::Shape::elementBytes(DType::UINT8),    1u);
}

TEST_F(GPUTensorBufferTest, ShapeTotalBytes) {
    GPUTensorBuffer::Shape s{{4, 4}};
    EXPECT_EQ(s.totalBytes(DType::FLOAT32), 64u);
    EXPECT_EQ(s.totalBytes(DType::INT8),    16u);
}

TEST_F(GPUTensorBufferTest, ShapeEquality) {
    GPUTensorBuffer::Shape a{{2, 3}};
    GPUTensorBuffer::Shape b{{2, 3}};
    GPUTensorBuffer::Shape c{{3, 2}};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

// ============================================================================
// Construction
// ============================================================================

TEST_F(GPUTensorBufferTest, ConstructFloat32) {
    GPUTensorBuffer::Shape s{{2, 4}};
    GPUTensorBuffer buf("weights", s, DType::FLOAT32);
    EXPECT_EQ(buf.name(), "weights");
    EXPECT_EQ(buf.shape(), s);
    EXPECT_EQ(buf.dtype(), DType::FLOAT32);
    EXPECT_EQ(buf.totalBytes(), 32u);  // 8 elements × 4 bytes
    EXPECT_TRUE(buf.isValid());
}

TEST_F(GPUTensorBufferTest, ConstructUpdatesGlobalStats) {
    GPUTensorBuffer::resetGlobalStats();
    GPUTensorBuffer::Shape s{{10}};
    GPUTensorBuffer buf("t", s, DType::INT8);
    auto stats = GPUTensorBuffer::getGlobalStats();
    EXPECT_EQ(stats.total_buffers_created, 1u);
    EXPECT_EQ(stats.current_bytes, 10u);
    EXPECT_EQ(stats.peak_bytes, 10u);
}

TEST_F(GPUTensorBufferTest, DestructorUpdatesStats) {
    GPUTensorBuffer::resetGlobalStats();
    {
        GPUTensorBuffer buf("tmp", GPUTensorBuffer::Shape{{8}}, DType::UINT8);
        EXPECT_EQ(GPUTensorBuffer::getGlobalStats().current_bytes, 8u);
    }
    auto stats = GPUTensorBuffer::getGlobalStats();
    EXPECT_EQ(stats.current_bytes, 0u);
    EXPECT_EQ(stats.total_buffers_freed, 1u);
}

// ============================================================================
// fill
// ============================================================================

TEST_F(GPUTensorBufferTest, FillFloat32) {
    GPUTensorBuffer buf("f", GPUTensorBuffer::Shape{{4}}, DType::FLOAT32);
    buf.fill(3.14);
    std::vector<float> out(4);
    buf.copyToHost(out.data(), 4 * sizeof(float));
    for (float v : out) EXPECT_NEAR(v, 3.14f, 1e-5f);
}

TEST_F(GPUTensorBufferTest, FillInt8) {
    GPUTensorBuffer buf("i", GPUTensorBuffer::Shape{{6}}, DType::INT8);
    buf.fill(7.0);
    std::vector<int8_t> out(6);
    buf.copyToHost(out.data(), 6);
    for (int8_t v : out) EXPECT_EQ(v, 7);
}

TEST_F(GPUTensorBufferTest, FillUint8) {
    GPUTensorBuffer buf("u", GPUTensorBuffer::Shape{{3}}, DType::UINT8);
    buf.fill(255.0);
    std::vector<uint8_t> out(3);
    buf.copyToHost(out.data(), 3);
    for (uint8_t v : out) EXPECT_EQ(v, 255u);
}

TEST_F(GPUTensorBufferTest, FillZero) {
    GPUTensorBuffer buf("z", GPUTensorBuffer::Shape{{8}}, DType::FLOAT32);
    buf.fill(1.0);
    buf.fill(0.0);
    std::vector<float> out(8);
    buf.copyToHost(out.data(), 8 * sizeof(float));
    for (float v : out) EXPECT_EQ(v, 0.0f);
}

TEST_F(GPUTensorBufferTest, FillFloat16_ProperIEEE754Encoding) {
    // Verify that FLOAT16 fill stores proper IEEE 754 half-precision bits.
    // 1.0f in FP16 = 0x3C00; 2.0f in FP16 = 0x4000.
    GPUTensorBuffer buf16("h16", GPUTensorBuffer::Shape{{2}}, DType::FLOAT16);
    buf16.fill(1.0);
    std::vector<uint16_t> raw(2);
    buf16.copyToHost(raw.data(), 2 * sizeof(uint16_t));
    EXPECT_EQ(raw[0], static_cast<uint16_t>(0x3C00u));  // 1.0 in FP16
    EXPECT_EQ(raw[1], static_cast<uint16_t>(0x3C00u));
}

TEST_F(GPUTensorBufferTest, FillBFloat16_ProperEncoding) {
    // 1.0f in BF16 = upper 16 bits of float32 0x3F800000 = 0x3F80.
    GPUTensorBuffer buf_bf16("bf16", GPUTensorBuffer::Shape{{2}}, DType::BFLOAT16);
    buf_bf16.fill(1.0);
    std::vector<uint16_t> raw(2);
    buf_bf16.copyToHost(raw.data(), 2 * sizeof(uint16_t));
    EXPECT_EQ(raw[0], static_cast<uint16_t>(0x3F80u));  // 1.0 in BF16
    EXPECT_EQ(raw[1], static_cast<uint16_t>(0x3F80u));
}

// ============================================================================
// copyFromHost / copyToHost
// ============================================================================

TEST_F(GPUTensorBufferTest, CopyRoundtrip) {
    GPUTensorBuffer buf("rt", GPUTensorBuffer::Shape{{4}}, DType::FLOAT32);
    float src[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    buf.copyFromHost(src, sizeof(src));
    float dst[4] = {};
    buf.copyToHost(dst, sizeof(dst));
    for (int i = 0; i < 4; ++i) EXPECT_FLOAT_EQ(dst[i], src[i]);
}

TEST_F(GPUTensorBufferTest, CopyFromHostOverflowThrows) {
    GPUTensorBuffer buf("ov", GPUTensorBuffer::Shape{{2}}, DType::INT8);
    uint8_t big[100] = {};
    EXPECT_THROW(buf.copyFromHost(big, 100), std::out_of_range);
}

TEST_F(GPUTensorBufferTest, CopyToHostOverflowThrows) {
    GPUTensorBuffer buf("ov", GPUTensorBuffer::Shape{{2}}, DType::INT8);
    uint8_t big[100] = {};
    EXPECT_THROW(buf.copyToHost(big, 100), std::out_of_range);
}

// ============================================================================
// createView
// ============================================================================

TEST_F(GPUTensorBufferTest, CreateView) {
    GPUTensorBuffer buf("parent", GPUTensorBuffer::Shape{{8, 4}}, DType::FLOAT32);
    auto view = buf.createView("head_0", 0, GPUTensorBuffer::Shape{{4}});
    EXPECT_EQ(view.name, "head_0");
    EXPECT_EQ(view.offset_bytes, 0u);
    EXPECT_EQ(view.dtype, DType::FLOAT32);
    EXPECT_EQ(view.shape, (GPUTensorBuffer::Shape{{4}}));
}

TEST_F(GPUTensorBufferTest, CreateViewWithOffset) {
    GPUTensorBuffer buf("p", GPUTensorBuffer::Shape{{16}}, DType::FLOAT32);
    auto view = buf.createView("slice", 4, GPUTensorBuffer::Shape{{4}});
    EXPECT_EQ(view.offset_bytes, 4 * sizeof(float));
}

TEST_F(GPUTensorBufferTest, CreateViewUpdatesStats) {
    GPUTensorBuffer::resetGlobalStats();
    GPUTensorBuffer buf("p", GPUTensorBuffer::Shape{{8}}, DType::FLOAT32);
    buf.createView("v1", 0, GPUTensorBuffer::Shape{{4}});
    buf.createView("v2", 4, GPUTensorBuffer::Shape{{4}});
    EXPECT_EQ(GPUTensorBuffer::getGlobalStats().total_views_created, 2u);
}

// ============================================================================
// Move semantics
// ============================================================================

TEST_F(GPUTensorBufferTest, MoveConstructor) {
    GPUTensorBuffer src("m", GPUTensorBuffer::Shape{{4}}, DType::INT32);
    src.fill(42.0);
    GPUTensorBuffer dst(std::move(src));
    EXPECT_EQ(dst.name(), "m");
    int32_t out[4] = {};
    dst.copyToHost(out, 4 * sizeof(int32_t));
    for (int32_t v : out) EXPECT_EQ(v, 42);
}

// ============================================================================
// Serialisation / deserialisation
// ============================================================================

TEST_F(GPUTensorBufferTest, SerialiseRoundtrip) {
    GPUTensorBuffer::Shape s{{2, 3}};
    GPUTensorBuffer orig("layer_0", s, DType::FLOAT32);
    float src[6] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    orig.copyFromHost(src, sizeof(src));

    auto bytes = orig.serialize();
    EXPECT_FALSE(bytes.empty());

    auto restored = GPUTensorBuffer::deserialize(bytes);
    EXPECT_EQ(restored.name(), "layer_0");
    EXPECT_EQ(restored.shape(), s);
    EXPECT_EQ(restored.dtype(), DType::FLOAT32);

    float dst[6] = {};
    restored.copyToHost(dst, sizeof(dst));
    for (int i = 0; i < 6; ++i) EXPECT_FLOAT_EQ(dst[i], src[i]);
}

TEST_F(GPUTensorBufferTest, DeserialiseCorruptMagicThrows) {
    std::vector<uint8_t> bad = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_THROW(GPUTensorBuffer::deserialize(bad), std::runtime_error);
}

TEST_F(GPUTensorBufferTest, DeserialiseTruncatedThrows) {
    GPUTensorBuffer buf("t", GPUTensorBuffer::Shape{{4}}, DType::INT8);
    auto bytes = buf.serialize();
    bytes.resize(bytes.size() / 2);
    EXPECT_THROW(GPUTensorBuffer::deserialize(bytes), std::runtime_error);
}

// ============================================================================
// Global stats – peak tracking
// ============================================================================

TEST_F(GPUTensorBufferTest, PeakBytesTracked) {
    GPUTensorBuffer::resetGlobalStats();
    {
        GPUTensorBuffer a("a", GPUTensorBuffer::Shape{{100}}, DType::UINT8);
        EXPECT_EQ(GPUTensorBuffer::getGlobalStats().peak_bytes, 100u);
        {
            GPUTensorBuffer b("b", GPUTensorBuffer::Shape{{50}}, DType::UINT8);
            EXPECT_EQ(GPUTensorBuffer::getGlobalStats().peak_bytes, 150u);
        }
        // After b destroyed, current drops but peak stays
        EXPECT_EQ(GPUTensorBuffer::getGlobalStats().peak_bytes, 150u);
        EXPECT_EQ(GPUTensorBuffer::getGlobalStats().current_bytes, 100u);
    }
    EXPECT_EQ(GPUTensorBuffer::getGlobalStats().current_bytes, 0u);
}
