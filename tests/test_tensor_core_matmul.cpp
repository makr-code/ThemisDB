// =============================================================================
// ThemisDB - Tests: Tensor Core FP16/BF16 Matrix Operations
//
// Tests the kernel_invocation.h MatrixKernelDispatch types, the
// CPUMatrixBackend FP32 fallback, and the BackendRegistry matrix backend
// selection.  All tests run on CPU without GPU hardware.
// =============================================================================

#include <gtest/gtest.h>
#include "acceleration/kernel_invocation.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/tensor_core_matmul.h"

#include <cstdint>
#include <cmath>
#include <type_traits>
#include <vector>

using namespace themis::acceleration;
using namespace themis::acceleration::tensor_core;

// =============================================================================
// MatrixPrecision enum
// =============================================================================

TEST(TensorCoreMatmul, MatrixPrecisionValues) {
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::FP32), 0u);
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::FP16), 1u);
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::BF16), 2u);
}

TEST(TensorCoreMatmul, MatrixPrecisionUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<MatrixPrecision>::type, uint32_t>::value,
        "MatrixPrecision must use uint32_t as its underlying type"
    );
}

// =============================================================================
// MatrixKernelParams defaults
// =============================================================================

TEST(TensorCoreMatmul, MatrixKernelParamsDefaultValues) {
    MatrixKernelParams p;
    EXPECT_EQ(p.A,         nullptr);
    EXPECT_EQ(p.B,         nullptr);
    EXPECT_EQ(p.C,         nullptr);
    EXPECT_EQ(p.M,         0u);
    EXPECT_EQ(p.K,         0u);
    EXPECT_EQ(p.N,         0u);
    EXPECT_FLOAT_EQ(p.alpha, 1.0f);
    EXPECT_FLOAT_EQ(p.beta,  0.0f);
    EXPECT_EQ(p.precision, MatrixPrecision::FP32);
}

TEST(TensorCoreMatmul, MatrixKernelParamsCanBePopulated) {
    std::vector<float> A(4, 1.0f), B(6, 2.0f), C(6, 0.0f);
    MatrixKernelParams p;
    p.A         = A.data();
    p.B         = B.data();
    p.C         = C.data();
    p.M         = 2;
    p.K         = 2;
    p.N         = 3;
    p.alpha     = 0.5f;
    p.beta      = 0.0f;
    p.precision = MatrixPrecision::FP16;

    EXPECT_EQ(p.M,         2u);
    EXPECT_EQ(p.K,         2u);
    EXPECT_EQ(p.N,         3u);
    EXPECT_FLOAT_EQ(p.alpha, 0.5f);
    EXPECT_EQ(p.precision, MatrixPrecision::FP16);
}

// =============================================================================
// MatrixKernelDispatch defaults
// =============================================================================

TEST(TensorCoreMatmul, MatrixKernelDispatchDefaultIsNull) {
    MatrixKernelDispatch d;
    EXPECT_EQ(d.launchMatmul, nullptr);
}

TEST(TensorCoreMatmul, MatrixKernelDispatchCanBePopulated) {
    auto dummy_fn = [](const MatrixKernelParams&, void*) -> int { return 0; };
    MatrixKernelDispatch d;
    d.launchMatmul = dummy_fn;
    EXPECT_NE(d.launchMatmul, nullptr);
    // Call it to confirm the signature matches
    MatrixKernelParams p;
    EXPECT_EQ(d.launchMatmul(p, nullptr), 0);
}

// =============================================================================
// CPU fallback launcher — correctness
// =============================================================================

// Helper: compare float arrays up to a tolerance
static void expectNear(const float* got, const std::vector<float>& expected,
                        float tol = 1e-5f)
{
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(got[i], expected[i], tol);
    }
}

TEST(TensorCoreMatmul, CPULauncherIdentityMatrix) {
    // A = I_2 (2×2), B = I_2 (2×2), expect C = I_2
    float A[4] = {1, 0,
                  0, 1};
    float B[4] = {1, 0,
                  0, 1};
    float C[4] = {};
    ASSERT_EQ(launchCPUMatmulKernel(A, B, C, 2, 2, 2, 1.0f, 0.0f), 0);
    expectNear(C, {1, 0, 0, 1});
}

TEST(TensorCoreMatmul, CPULauncher2x2Times2x2) {
    // A = [[1,2],[3,4]], B = [[5,6],[7,8]]
    // C = A*B = [[19,22],[43,50]]
    float A[4] = {1, 2, 3, 4};
    float B[4] = {5, 6, 7, 8};
    float C[4] = {};
    ASSERT_EQ(launchCPUMatmulKernel(A, B, C, 2, 2, 2, 1.0f, 0.0f), 0);
    expectNear(C, {19, 22, 43, 50});
}

TEST(TensorCoreMatmul, CPULauncherAlpha) {
    // Same matrices but alpha = 0.5 → C = [[9.5, 11], [21.5, 25]]
    float A[4] = {1, 2, 3, 4};
    float B[4] = {5, 6, 7, 8};
    float C[4] = {};
    ASSERT_EQ(launchCPUMatmulKernel(A, B, C, 2, 2, 2, 0.5f, 0.0f), 0);
    expectNear(C, {9.5f, 11.0f, 21.5f, 25.0f});
}

TEST(TensorCoreMatmul, CPULauncherBeta) {
    // C_init = [[1,1],[1,1]], beta = 2 → C = 2*C_init + A*B
    float A[4] = {1, 2, 3, 4};
    float B[4] = {5, 6, 7, 8};
    float C[4] = {1, 1, 1, 1};
    ASSERT_EQ(launchCPUMatmulKernel(A, B, C, 2, 2, 2, 1.0f, 2.0f), 0);
    // C = beta*C_init + alpha*A*B = 2*[[1,1],[1,1]] + [[19,22],[43,50]]
    //   = [[21,24],[45,52]]
    expectNear(C, {21, 24, 45, 52});
}

TEST(TensorCoreMatmul, CPULauncherNonSquare) {
    // A [2×3], B [3×2] → C [2×2]
    // A = [[1,2,3],[4,5,6]], B = [[7,8],[9,10],[11,12]]
    // C[0,0] = 1*7+2*9+3*11 = 7+18+33 = 58
    // C[0,1] = 1*8+2*10+3*12 = 8+20+36 = 64
    // C[1,0] = 4*7+5*9+6*11 = 28+45+66 = 139
    // C[1,1] = 4*8+5*10+6*12 = 32+50+72 = 154
    float A[6]  = {1, 2, 3, 4, 5, 6};
    float B[6]  = {7, 8, 9, 10, 11, 12};
    float C[4]  = {};
    ASSERT_EQ(launchCPUMatmulKernel(A, B, C, 2, 3, 2, 1.0f, 0.0f), 0);
    expectNear(C, {58, 64, 139, 154});
}

TEST(TensorCoreMatmul, CPULauncherNullInputReturnsError) {
    float C[4] = {};
    EXPECT_NE(launchCPUMatmulKernel(nullptr, nullptr, C, 2, 2, 2, 1.0f, 0.0f), 0);
}

TEST(TensorCoreMatmul, CPULauncherZeroDimensionReturnsError) {
    float A[4] = {}, B[4] = {}, C[4] = {};
    EXPECT_NE(launchCPUMatmulKernel(A, B, C, 0, 2, 2, 1.0f, 0.0f), 0);
    EXPECT_NE(launchCPUMatmulKernel(A, B, C, 2, 0, 2, 1.0f, 0.0f), 0);
    EXPECT_NE(launchCPUMatmulKernel(A, B, C, 2, 2, 0, 1.0f, 0.0f), 0);
}

// =============================================================================
// dispatchMatmul (no-GPU path always uses FP32 CPU fallback)
// =============================================================================

TEST(TensorCoreMatmul, DispatchMatmulFP32NoCuda) {
    float A[4] = {1, 2, 3, 4};
    float B[4] = {1, 0, 0, 1};  // identity
    float C[4] = {};
    MatrixKernelParams p;
    p.A         = A;
    p.B         = B;
    p.C         = C;
    p.M         = 2;
    p.K         = 2;
    p.N         = 2;
    p.precision = MatrixPrecision::FP32;

    // In non-CUDA builds this always succeeds using the CPU path.
    // In CUDA builds it routes to cuBLAS FP32 (requires a device, may fail).
#ifndef THEMIS_ENABLE_CUDA
    ASSERT_EQ(dispatchMatmul(p, nullptr), 0);
    expectNear(C, {1, 2, 3, 4});
#else
    // Just ensure it compiles and does not crash for the FP32 path.
    // Actual correctness on real GPU hardware is validated by the CUDA tests.
    (void)dispatchMatmul(p, nullptr);
#endif
}

TEST(TensorCoreMatmul, DispatchMatmulRejectsNullPointers) {
    MatrixKernelParams p;
    p.M = p.K = p.N = 4;
    // A, B, C are null — expect failure
    int rc = dispatchMatmul(p, nullptr);
    EXPECT_NE(rc, 0);
}

// =============================================================================
// CPUMatrixBackend — capability and lifecycle
// =============================================================================

TEST(TensorCoreMatmul, CPUMatrixBackendIsAlwaysAvailable) {
    CPUMatrixBackend backend;
    EXPECT_TRUE(backend.isAvailable());
    EXPECT_STREQ(backend.name(), "CPU");
    EXPECT_EQ(backend.type(), BackendType::CPU);
}

TEST(TensorCoreMatmul, CPUMatrixBackendInitializeAndShutdown) {
    CPUMatrixBackend backend;
    EXPECT_TRUE(backend.initialize());
    backend.shutdown();
    // After shutdown the backend is still available (CPU always ready)
    EXPECT_TRUE(backend.isAvailable());
}

TEST(TensorCoreMatmul, CPUMatrixBackendCapabilities) {
    CPUMatrixBackend backend;
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsMatrixOps);
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
    // CPU does not advertise FP16/BF16 Tensor Core support
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::BF16));
}

TEST(TensorCoreMatmul, CPUMatrixBackendMatmul) {
    CPUMatrixBackend backend;
    backend.initialize();

    float A[4] = {1, 2, 3, 4};
    float B[4] = {5, 6, 7, 8};
    float C[4] = {};

    MatrixKernelParams p;
    p.A = A; p.B = B; p.C = C;
    p.M = 2; p.K = 2; p.N = 2;
    p.precision = MatrixPrecision::FP32;

    ASSERT_EQ(backend.matmul(p, nullptr), 0);
    expectNear(C, {19, 22, 43, 50});
}

TEST(TensorCoreMatmul, CPUMatrixBackendDispatchTable) {
    CPUMatrixBackend backend;
    auto disp = backend.populateMatrixDispatch();
    ASSERT_NE(disp.launchMatmul, nullptr);

    float A[4] = {1, 0, 0, 1};  // identity
    float B[4] = {3, 7, 2, 5};
    float C[4] = {};
    MatrixKernelParams p;
    p.A = A; p.B = B; p.C = C;
    p.M = 2; p.K = 2; p.N = 2;

    ASSERT_EQ(disp.launchMatmul(p, nullptr), 0);
    // I * B = B
    expectNear(C, {3, 7, 2, 5});
}

// =============================================================================
// BackendCapabilities — supportsMatrixOps
// =============================================================================

TEST(TensorCoreMatmul, BackendCapabilitiesHasMatrixOpsField) {
    BackendCapabilities caps;
    EXPECT_FALSE(caps.supportsMatrixOps);  // default is false

    caps.supportsMatrixOps = true;
    EXPECT_TRUE(caps.supportsMatrixOps);
}

// =============================================================================
// BackendRegistry — matrix backend registration and selection
// =============================================================================

class TensorCoreRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto backend = std::make_unique<CPUMatrixBackend>();
        BackendRegistry::instance().registerBackend(std::move(backend));
    }

    void TearDown() override {
        BackendRegistry::instance().shutdownAll();
    }
};

TEST_F(TensorCoreRegistryTest, GetBestMatrixBackend) {
    auto* backend = BackendRegistry::instance().getBestMatrixBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->getCapabilities().supportsMatrixOps);
}

TEST_F(TensorCoreRegistryTest, SelectMatrixBackendForFP32) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps      = true;
    reqs.requiredPrecisions  = PrecisionMode::FP32;

    auto* backend = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(hasPrecision(backend->getCapabilities().supportedPrecisions,
                              PrecisionMode::FP32));
}

TEST_F(TensorCoreRegistryTest, SatisfiesMatrixOpsRequirement) {
    BackendCapabilities caps;
    caps.supportsMatrixOps   = true;
    caps.supportedPrecisions = PrecisionMode::FP32;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps     = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));

    BackendRegistry::CapabilityRequirements reqs_no_matrix;
    reqs_no_matrix.needsMatrixOps = false;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs_no_matrix));

    BackendCapabilities caps_no_matrix;
    caps_no_matrix.supportsMatrixOps = false;
    EXPECT_FALSE(BackendRegistry::satisfies(caps_no_matrix, reqs));
}

// =============================================================================
// PrecisionMode — FP16 / BF16 bitmask operators (already defined in header)
// =============================================================================

TEST(TensorCoreMatmul, PrecisionModeOrOperator) {
    auto combined = PrecisionMode::FP16 | PrecisionMode::BF16;
    EXPECT_TRUE(hasPrecision(combined, PrecisionMode::FP16));
    EXPECT_TRUE(hasPrecision(combined, PrecisionMode::BF16));
    EXPECT_FALSE(hasPrecision(combined, PrecisionMode::FP32));
}

TEST(TensorCoreMatmul, PrecisionModeHasPrecision) {
    EXPECT_TRUE(hasPrecision(PrecisionMode::FP32, PrecisionMode::FP32));
    EXPECT_FALSE(hasPrecision(PrecisionMode::FP32, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(PrecisionMode::FP32, PrecisionMode::BF16));
}

// =============================================================================
// MatrixPrecision::INT8 enum value
// =============================================================================

TEST(TensorCoreMatmul, MatrixPrecisionINT8IsThree) {
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::INT8), 3u);
}

TEST(TensorCoreMatmul, MatrixPrecisionParamsCanSetINT8) {
    MatrixKernelParams p;
    p.precision = MatrixPrecision::INT8;
    EXPECT_EQ(p.precision, MatrixPrecision::INT8);
}

// =============================================================================
// quantize() — FP32 → INT8 symmetric quantisation
// =============================================================================

TEST(TensorCoreMatmul, QuantizeBasic) {
    // scale = 1.0: values should map directly to int8
    float src[4] = {1.0f, -1.0f, 0.0f, 127.0f};
    int8_t dst[4] = {};
    quantize(src, dst, 4, 1.0f);
    EXPECT_EQ(dst[0],  1);
    EXPECT_EQ(dst[1], -1);
    EXPECT_EQ(dst[2],  0);
    EXPECT_EQ(dst[3],  127);
}

TEST(TensorCoreMatmul, QuantizeWithScale) {
    // scale = 2.0: src / scale = dst → 2.0/2=1, -4.0/2=-2, 0.0/2=0, 254.0/2=127
    float src[4] = {2.0f, -4.0f, 0.0f, 254.0f};
    int8_t dst[4] = {};
    quantize(src, dst, 4, 2.0f);
    EXPECT_EQ(dst[0],   1);
    EXPECT_EQ(dst[1],  -2);
    EXPECT_EQ(dst[2],   0);
    EXPECT_EQ(dst[3],  127);
}

TEST(TensorCoreMatmul, QuantizeClampMax) {
    // Values that exceed INT8 range must be clamped to 127
    float src[2] = {1000.0f, 256.0f};
    int8_t dst[2] = {};
    quantize(src, dst, 2, 1.0f);
    EXPECT_EQ(dst[0], 127);
    EXPECT_EQ(dst[1], 127);
}

TEST(TensorCoreMatmul, QuantizeClampMin) {
    // Values below INT8 range must be clamped to -128
    float src[2] = {-1000.0f, -200.0f};
    int8_t dst[2] = {};
    quantize(src, dst, 2, 1.0f);
    EXPECT_EQ(dst[0], -128);
    EXPECT_EQ(dst[1], -128);
}

TEST(TensorCoreMatmul, QuantizeNullSrcNoOp) {
    int8_t dst[4] = {5, 5, 5, 5};
    quantize(nullptr, dst, 4, 1.0f);
    // dst must be unchanged
    for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(dst[i], 5);
    }
}

TEST(TensorCoreMatmul, QuantizeNullDstNoOp) {
    float src[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    // Must not crash
    quantize(src, nullptr, 4, 1.0f);
}

TEST(TensorCoreMatmul, QuantizeZeroNNoOp) {
    float src[4] = {1.0f, 2.0f};
    int8_t dst[4] = {7, 7};
    quantize(src, dst, 0, 1.0f);
    EXPECT_EQ(dst[0], 7);
}

TEST(TensorCoreMatmul, QuantizeNonPositiveScaleNoOp) {
    float src[2] = {10.0f, 20.0f};
    int8_t dst[2] = {9, 9};
    quantize(src, dst, 2, 0.0f);
    EXPECT_EQ(dst[0], 9);
    quantize(src, dst, 2, -1.0f);
    EXPECT_EQ(dst[0], 9);
}

TEST(TensorCoreMatmul, QuantizeRounding) {
    // 1.5 / 1.0 = 1.5 → rounds to 2 (round half away from zero)
    float src[2] = {1.5f, -1.5f};
    int8_t dst[2] = {};
    quantize(src, dst, 2, 1.0f);
    EXPECT_EQ(dst[0],  2);
    EXPECT_EQ(dst[1], -2);
}

// =============================================================================
// dequantize() — INT8 → FP32 inverse
// =============================================================================

TEST(TensorCoreMatmul, DequantizeBasic) {
    int8_t src[4] = {1, -1, 0, 127};
    float dst[4] = {};
    dequantize(src, dst, 4, 1.0f);
    EXPECT_FLOAT_EQ(dst[0],   1.0f);
    EXPECT_FLOAT_EQ(dst[1],  -1.0f);
    EXPECT_FLOAT_EQ(dst[2],   0.0f);
    EXPECT_FLOAT_EQ(dst[3], 127.0f);
}

TEST(TensorCoreMatmul, DequantizeWithScale) {
    int8_t src[3] = {1, -2, 127};
    float dst[3] = {};
    dequantize(src, dst, 3, 2.0f);
    EXPECT_FLOAT_EQ(dst[0],   2.0f);
    EXPECT_FLOAT_EQ(dst[1],  -4.0f);
    EXPECT_FLOAT_EQ(dst[2], 254.0f);
}

TEST(TensorCoreMatmul, DequantizeNullSrcNoOp) {
    float dst[4] = {9.0f, 9.0f, 9.0f, 9.0f};
    dequantize(nullptr, dst, 4, 1.0f);
    for (int i = 0; i < 4; ++i) {
      EXPECT_FLOAT_EQ(dst[i], 9.0f);
    }
}

TEST(TensorCoreMatmul, DequantizeNullDstNoOp) {
    int8_t src[4] = {1, 2, 3, 4};
    // Must not crash
    dequantize(src, nullptr, 4, 1.0f);
}

TEST(TensorCoreMatmul, DequantizeZeroNNoOp) {
    int8_t src[2] = {5, 5};
    float dst[2] = {3.0f, 3.0f};
    dequantize(src, dst, 0, 1.0f);
    EXPECT_FLOAT_EQ(dst[0], 3.0f);
}

TEST(TensorCoreMatmul, QuantizeDequantizeRoundTrip) {
    // Round-trip: quantize then dequantize should recover approximately original values
    // Using scale = max(|src|) / 127 ensures the full range maps to [-127, 127]
    float src[5] = {0.0f, 25.4f, -25.4f, 12.7f, -12.7f};
    const float scale = 25.4f / 127.0f;  // ≈ 0.2
    int8_t quantized[5] = {};
    float recovered[5] = {};
    quantize(src, quantized, 5, scale);
    dequantize(quantized, recovered, 5, scale);
    for (int i = 0; i < 5; ++i) {
        // Tolerate rounding error of at most 1 quantisation step (scale)
        EXPECT_NEAR(recovered[i], src[i], scale);
    }
}

