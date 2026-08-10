/**
 * @file test_cuda_plugin_standalone_focused.cpp
 * @brief Focused unit tests for the standalone CUDAAccelerationPlugin.
 *
 * All tests are GPU-agnostic: they exercise plugin metadata, factory method
 * return values, and configuration wiring without requiring a real NVIDIA GPU.
 * Paths that would invoke CUDA kernels are guarded by isAvailable() checks
 * inside the plugin itself and fall back gracefully.
 *
 * @version 1.0.0
 */

#include <gtest/gtest.h>

// Pull in the plugin implementation directly (no .so load required for unit tests)
#include "cuda_plugin_config.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/multi_gpu_backend.h"
#include "acceleration/faiss_gpu_backend.h"
#include "acceleration/plugin_loader.h"

// Include plugin source directly so we can access the class without dlopen
// (the CMakeLists links this test against the plugin .cpp via the focused-test
// glob pattern).
#include "../../plugins/cuda/cuda_plugin.cpp"

using namespace themis::acceleration;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a plugin instance with a given config
// ─────────────────────────────────────────────────────────────────────────────

static std::unique_ptr<CUDAAccelerationPlugin> makePlugin(
    CudaPluginVectorMode mode      = CudaPluginVectorMode::SINGLE_GPU,
    CudaPluginPrecision  precision = CudaPluginPrecision::FP32)
{
    CudaPluginConfig cfg;
    cfg.vectorMode = mode;
    cfg.precision  = precision;
    return std::make_unique<CUDAAccelerationPlugin>(cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

/// @test Default-constructed plugin reports correct name and version.
TEST(CUDAPluginInstantiation, DefaultConstructor)
{
    CUDAAccelerationPlugin plugin;
    EXPECT_STREQ(plugin.pluginName(),    "ThemisDB CUDA Acceleration Plugin");
    EXPECT_STREQ(plugin.pluginVersion(), "2.0.0");
    // Default mode is SINGLE_GPU → CUDA type
    EXPECT_EQ(plugin.backendType(), BackendType::CUDA);
}

/// @test Config-constructed plugin with MULTI_GPU mode advertises MULTI_GPU type.
TEST(CUDAPluginInstantiation, MultiGPUBackendType)
{
    auto plugin = makePlugin(CudaPluginVectorMode::MULTI_GPU);
    EXPECT_EQ(plugin->backendType(), BackendType::MULTI_GPU);
}

/// @test Config-constructed plugin with FAISS_GPU mode advertises CUDA type.
TEST(CUDAPluginInstantiation, FaissGPUBackendType)
{
    // FAISS_GPU reports BackendType::CUDA (FaissGPUVectorBackend::type())
    auto plugin = makePlugin(CudaPluginVectorMode::FAISS_GPU);
    EXPECT_EQ(plugin->backendType(), BackendType::CUDA);
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test createVectorBackend() returns a non-null pointer in default mode.
TEST(CUDAPluginCreateBackends, VectorBackendNonNull_SingleGPU_FP32)
{
    CUDAAccelerationPlugin plugin;
    auto backend = plugin.createVectorBackend();
    ASSERT_NE(backend, nullptr);
}

/// @test createGraphBackend() returns a non-null pointer.
TEST(CUDAPluginCreateBackends, GraphBackendNonNull)
{
    CUDAAccelerationPlugin plugin;
    auto backend = plugin.createGraphBackend();
    ASSERT_NE(backend, nullptr);
}

/// @test createGeoBackend() returns a non-null pointer.
TEST(CUDAPluginCreateBackends, GeoBackendNonNull)
{
    CUDAAccelerationPlugin plugin;
    auto backend = plugin.createGeoBackend();
    ASSERT_NE(backend, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test FP16 config produces a backend whose name() is "CUDA FP16".
TEST(CUDAPluginPrecisionConfig, FP16BackendName)
{
    auto plugin = makePlugin(CudaPluginVectorMode::SINGLE_GPU,
                              CudaPluginPrecision::FP16);
    auto backend = plugin->createVectorBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_STREQ(backend->name(), "CUDA FP16");
}

/// @test BF16 config produces a backend whose name() is "CUDA BF16".
TEST(CUDAPluginPrecisionConfig, BF16BackendName)
{
    auto plugin = makePlugin(CudaPluginVectorMode::SINGLE_GPU,
                              CudaPluginPrecision::BF16);
    auto backend = plugin->createVectorBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_STREQ(backend->name(), "CUDA BF16");
}

/// @test FP32 config produces a plain CUDAVectorBackend (not the FP16 wrapper).
TEST(CUDAPluginPrecisionConfig, FP32BackendName)
{
    auto plugin = makePlugin(CudaPluginVectorMode::SINGLE_GPU,
                              CudaPluginPrecision::FP32);
    auto backend = plugin->createVectorBackend();
    ASSERT_NE(backend, nullptr);
    // CUDAVectorBackend::name() should NOT be "CUDA FP16" or "CUDA BF16"
    EXPECT_STRNE(backend->name(), "CUDA FP16");
    EXPECT_STRNE(backend->name(), "CUDA BF16");
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test Multi-GPU config produces a backend whose type() is MULTI_GPU.
TEST(CUDAPluginMultiGPUConfig, BackendTypeIsMultiGPU)
{
    auto plugin = makePlugin(CudaPluginVectorMode::MULTI_GPU);
    auto backend = plugin->createVectorBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->type(), BackendType::MULTI_GPU);
}

/// @test Multi-GPU backend name is "MultiGPU".
TEST(CUDAPluginMultiGPUConfig, BackendName)
{
    auto plugin = makePlugin(CudaPluginVectorMode::MULTI_GPU);
    auto backend = plugin->createVectorBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_STREQ(backend->name(), "MultiGPU");
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test FAISS config produces a backend whose name() contains "Faiss".
/// If CUDA is unavailable at runtime, initializeIndex() may fail; we guard
/// with isAvailable() to avoid a hard failure in CI without a GPU.
TEST(CUDAPluginFaissConfig, BackendNameContainsFaiss)
{
#ifdef THEMIS_ENABLE_CUDA
    CudaPluginConfig cfg;
    cfg.vectorMode = CudaPluginVectorMode::FAISS_GPU;
    cfg.faissConfig.indexType = FaissGPUVectorBackend::IndexType::FLAT_L2;
    cfg.faissConfig.dimension = 64;

    CUDAAccelerationPlugin plugin(cfg);

    // createVectorBackend() throws if initializeIndex fails on a no-GPU machine.
    // Catch and skip when GPU is not available.
    std::unique_ptr<IVectorBackend> backend;
    try {
        backend = plugin.createVectorBackend();
    } catch (const std::exception& ex) {
        GTEST_SKIP() << "FAISS GPU unavailable in this environment: " << ex.what();
    }

    ASSERT_NE(backend, nullptr);
    EXPECT_NE(std::string(backend->name()).find("Faiss"), std::string::npos);
#else
    GTEST_SKIP() << "THEMIS_ENABLE_CUDA not defined — FAISS_GPU test skipped";
#endif
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test When isAvailable() returns false, backends degrade without crashing.
TEST(CUDAPluginGracefulUnavailable, DegradeWithoutCrash)
{
    CUDAAccelerationPlugin plugin;
    auto backend = plugin.createVectorBackend();
    ASSERT_NE(backend, nullptr);

    // If CUDA is not available, isAvailable() should be false and all
    // operations should return graceful (empty/zero) results without crashing.
    if (!backend->isAvailable()) {
        // computeDistances on an unavailable backend should not throw
        EXPECT_NO_THROW({
            constexpr size_t numQ = 2;
            constexpr size_t numV = 3;
            constexpr size_t dim  = 4;
            std::vector<float> q(numQ * dim, 1.0f);
            std::vector<float> v(numV * dim, 0.5f);
            auto result = backend->computeDistances(
                q.data(), numQ, dim, v.data(), numV, true);
            // Result may be empty or populated; no crash is the requirement.
            (void)result;
        });
    } else {
        SUCCEED() << "CUDA available — graceful-unavailable path not exercised";
    }
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test The C entry-point CreateBackendPlugin() returns a non-null pointer.
TEST(CUDAPluginEntryPoint, CreateBackendPluginReturnsNonNull)
{
    // CreateBackendPlugin is generated by THEMIS_DEFINE_PLUGIN macro in the
    // included cuda_plugin.cpp.  Call it directly (no dlopen required).
    BackendPlugin* raw = CreateBackendPlugin();
    ASSERT_NE(raw, nullptr);
    EXPECT_STREQ(raw->pluginName(),    "ThemisDB CUDA Acceleration Plugin");
    EXPECT_STREQ(raw->pluginVersion(), "2.0.0");
    delete raw;  // PluginLoader takes ownership in production; we own it here.
}

// ─────────────────────────────────────────────────────────────────────────────

/// @test MixedPrecisionVectorBackend: FP16 wrapper reports "CUDA FP16".
TEST(MixedPrecisionBackendName, FP16ReportsCUDAFP16)
{
    MixedPrecisionVectorBackend backend(CudaPluginPrecision::FP16);
    EXPECT_STREQ(backend.name(), "CUDA FP16");
    EXPECT_EQ(backend.type(), BackendType::CUDA);
}

/// @test MixedPrecisionVectorBackend: BF16 wrapper reports "CUDA BF16".
TEST(MixedPrecisionBackendName, BF16ReportsCUDABF16)
{
    MixedPrecisionVectorBackend backend(CudaPluginPrecision::BF16);
    EXPECT_STREQ(backend.name(), "CUDA BF16");
    EXPECT_EQ(backend.type(), BackendType::CUDA);
}

/// @test MixedPrecisionVectorBackend: FP32 precision throws invalid_argument.
TEST(MixedPrecisionBackendName, FP32ThrowsInvalidArgument)
{
    EXPECT_THROW(
        { MixedPrecisionVectorBackend bad(CudaPluginPrecision::FP32); },
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
