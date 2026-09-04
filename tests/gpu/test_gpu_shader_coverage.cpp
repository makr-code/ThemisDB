#include <gtest/gtest.h>
#include "acceleration/kernel_registry.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace themis::acceleration;

// =============================================================================
// Helpers
// =============================================================================

/// Return the repository root by walking up from the current working directory.
/// Looks for a directory that contains both "src" and "include" subdirectories.
static fs::path repoRoot() {
    for (fs::path p = fs::current_path();
         p.has_parent_path() && p != p.parent_path();
         p = p.parent_path()) {
        if (fs::exists(p / "src") && fs::exists(p / "include")) {
            return p;
        }
    }
    return fs::current_path();
}

// =============================================================================
// Fixture
// =============================================================================

class ShaderCoverageTest : public ::testing::Test {
protected:
    void SetUp() override { root_ = repoRoot(); }
    fs::path root_;
};

// =============================================================================
// Part 1 — Shader file existence
//
// These tests verify that every required shader file for each GPU backend
// is present in the source tree.  A missing file is a CI blocker because
// it means the backend will fail at runtime when it tries to load its shader.
// =============================================================================

/// Vulkan GLSL compute shaders (core ANN + Geo operations).
static const std::vector<const char*> kVulkanCoreShaders = {
    "src/acceleration/vulkan/shaders/l2_distance.comp",
    "src/acceleration/vulkan/shaders/cosine_distance.comp",
    "src/acceleration/vulkan/shaders/inner_product_distance.comp",
    "src/acceleration/vulkan/shaders/topk_selection.comp",
    "src/acceleration/vulkan/shaders/batch_search.comp",
    "src/acceleration/vulkan/shaders/haversine_distance.comp",
    "src/acceleration/vulkan/shaders/point_in_polygon.comp",
};

/// Vulkan GLSL compute shaders (LoRA fine-tuning).
static const std::vector<const char*> kVulkanLoRAShaders = {
    "src/acceleration/vulkan/shaders/lora/matmul.comp",
    "src/acceleration/vulkan/shaders/lora/gradient.comp",
    "src/acceleration/vulkan/shaders/lora/elementwise.comp",
    "src/acceleration/vulkan/shaders/lora/embedding_lookup.comp",
    "src/acceleration/vulkan/shaders/lora/quantization_nf4.comp",
    "src/acceleration/vulkan/shaders/lora/dequantization_nf4.comp",
    "src/acceleration/vulkan/shaders/lora/sequence_mean.comp",
};

/// DirectX HLSL compute shaders (core ANN + Geo operations).
static const std::vector<const char*> kDirectXCoreShaders = {
    "src/acceleration/directx/shaders/l2_distance.hlsl",
    "src/acceleration/directx/shaders/cosine_distance.hlsl",
    "src/acceleration/directx/shaders/inner_product_distance.hlsl",
    "src/acceleration/directx/shaders/topk_selection.hlsl",
    "src/acceleration/directx/shaders/batch_search.hlsl",
    "src/acceleration/directx/shaders/haversine_distance.hlsl",
    "src/acceleration/directx/shaders/point_in_polygon.hlsl",
};

/// DirectX HLSL compute shaders (LoRA fine-tuning).
static const std::vector<const char*> kDirectXLoRAShaders = {
    "src/acceleration/directx/shaders/lora/matmul.hlsl",
    "src/acceleration/directx/shaders/lora/gradient.hlsl",
    "src/acceleration/directx/shaders/lora/elementwise.hlsl",
    "src/acceleration/directx/shaders/lora/embedding_lookup.hlsl",
    "src/acceleration/directx/shaders/lora/quantization_nf4.hlsl",
    "src/acceleration/directx/shaders/lora/dequantization_nf4.hlsl",
    "src/acceleration/directx/shaders/lora/sequence_mean.hlsl",
};

/// CUDA kernel source files.
static const std::vector<const char*> kCUDAKernels = {
    "src/acceleration/cuda/vector_kernels.cu",
    "src/acceleration/cuda/ann_kernels.cu",
    "src/acceleration/cuda/graph_kernels.cu",
    "src/acceleration/cuda/geo_kernels.cu",
    "src/acceleration/cuda/tensor_core_matmul.cu",
};

/// HIP kernel source files (AMD ROCm).
static const std::vector<const char*> kHIPKernels = {
    "src/acceleration/hip/ann_kernels.hip",
    "src/acceleration/hip/geo_kernels.hip",
    "src/acceleration/hip/graph_kernels.hip",
    "src/acceleration/hip/hnsw_kernels.hip",
};

// ---------------------------------------------------------------------------
// Vulkan shader completeness
// ---------------------------------------------------------------------------

TEST_F(ShaderCoverageTest, VulkanCoreShaders_AllPresent) {
    for (const char* rel : kVulkanCoreShaders) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing Vulkan core shader: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

TEST_F(ShaderCoverageTest, VulkanLoRAShaders_AllPresent) {
    for (const char* rel : kVulkanLoRAShaders) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing Vulkan LoRA shader: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

// ---------------------------------------------------------------------------
// DirectX shader completeness
// ---------------------------------------------------------------------------

TEST_F(ShaderCoverageTest, DirectXCoreShaders_AllPresent) {
    for (const char* rel : kDirectXCoreShaders) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing DirectX core shader: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

TEST_F(ShaderCoverageTest, DirectXLoRAShaders_AllPresent) {
    for (const char* rel : kDirectXLoRAShaders) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing DirectX LoRA shader: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

// ---------------------------------------------------------------------------
// CUDA kernel completeness
// ---------------------------------------------------------------------------

TEST_F(ShaderCoverageTest, CUDAKernels_AllPresent) {
    for (const char* rel : kCUDAKernels) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing CUDA kernel: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

// ---------------------------------------------------------------------------
// HIP kernel completeness
// ---------------------------------------------------------------------------

TEST_F(ShaderCoverageTest, HIPKernels_AllPresent) {
    for (const char* rel : kHIPKernels) {
        const fs::path p = root_ / rel;
        EXPECT_TRUE(fs::exists(p))
            << "Missing HIP kernel: " << rel
            << "\n  (resolved path: " << p.string() << ")";
    }
}

// ---------------------------------------------------------------------------
// Parity: DirectX and Vulkan must have the same number of core/LoRA shaders
// ---------------------------------------------------------------------------

TEST_F(ShaderCoverageTest, DirectXVulkanParity_CoreShaderCount) {
    EXPECT_EQ(kVulkanCoreShaders.size(), kDirectXCoreShaders.size())
        << "Vulkan and DirectX core shader file counts diverge — parity broken.";
}

TEST_F(ShaderCoverageTest, DirectXVulkanParity_LoRAShaderCount) {
    EXPECT_EQ(kVulkanLoRAShaders.size(), kDirectXLoRAShaders.size())
        << "Vulkan and DirectX LoRA shader file counts diverge — parity broken.";
}

// =============================================================================
// Part 2 — KernelRegistry validation API
//
// These tests exercise the KernelRegistry::validate() API.  A local
// KernelRegistry instance is used so tests do not pollute the BackendRegistry
// singleton state.
// =============================================================================

// Minimal stub implementations matching the frozen kernel function typedefs.
static int stubL2(const float*, const float*, float*, int, int, int, void*) { return 0; }
static int stubCosine(const float*, const float*, float*, int, int, int, void*) { return 0; }
static int stubIP(const float*, const float*, float*, int, int, int, void*) { return 0; }
static int stubTopK(const float*, uint32_t*, float*, int, int, int, void*) { return 0; }
static int stubGeoDist(const double*, const double*, const double*, const double*,
                       float*, int, GeoDistanceFormula, void*) { return 0; }
static int stubGeoContain(const double*, const double*, int,
                          const double*, int, uint8_t*, void*) { return 0; }
static int stubMatMul(const MatrixKernelParams&, void*) { return 0; }

// ---------------------------------------------------------------------------
// Empty registry
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, EmptyRegistry_AllComplete_IsTrue) {
    KernelRegistry reg;
    // No backends registered: nothing to fail on.
    auto r = reg.validate();
    EXPECT_TRUE(r.allComplete());
    EXPECT_TRUE(r.entries.empty());
}

// ---------------------------------------------------------------------------
// Complete ANN registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, CompleteANN_PassesValidation) {
    KernelRegistry reg;

    ANNKernelDispatch d;
    d.launchL2Distance   = stubL2;
    d.launchCosine       = stubCosine;
    d.launchInnerProduct = stubIP;
    d.launchTopK         = stubTopK;
    reg.registerANNDispatch(BackendType::CUDA, d);

    auto r = reg.validate();
    ASSERT_EQ(r.entries.size(), 1u);
    EXPECT_TRUE(r.entries[0].annComplete);
    EXPECT_TRUE(r.entries[0].missingSlots.empty());
    EXPECT_TRUE(r.allComplete());
}

// ---------------------------------------------------------------------------
// Incomplete ANN registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, IncompleteANN_ReportsMissingSlots) {
    KernelRegistry reg;

    ANNKernelDispatch d;
    d.launchL2Distance   = stubL2;
    d.launchCosine       = nullptr;  // Missing
    d.launchInnerProduct = stubIP;
    d.launchTopK         = nullptr;  // Missing
    reg.registerANNDispatch(BackendType::VULKAN, d);

    auto r = reg.validate();
    ASSERT_EQ(r.entries.size(), 1u);
    EXPECT_FALSE(r.entries[0].annComplete);
    EXPECT_FALSE(r.allComplete());

    const auto& missing = r.entries[0].missingSlots;
    EXPECT_EQ(missing.size(), 2u);

    bool foundCosine = false, foundTopK = false;
    for (const auto& s : missing) {
        if (s.find("launchCosine") != std::string::npos) {
          foundCosine = true;
        }
        if (s.find("launchTopK")   != std::string::npos) {
          foundTopK   = true;
        }
    }
    EXPECT_TRUE(foundCosine) << "Expected 'launchCosine' in missing slots";
    EXPECT_TRUE(foundTopK)   << "Expected 'launchTopK' in missing slots";
}

// ---------------------------------------------------------------------------
// Complete Geo registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, CompleteGeo_PassesValidation) {
    KernelRegistry reg;

    GeoKernelDispatch d;
    d.launchDistance    = stubGeoDist;
    d.launchContainment = stubGeoContain;
    reg.registerGeoDispatch(BackendType::HIP, d);

    auto r = reg.validate();
    ASSERT_EQ(r.entries.size(), 1u);
    EXPECT_TRUE(r.entries[0].geoComplete);
    EXPECT_TRUE(r.allComplete());
}

// ---------------------------------------------------------------------------
// Incomplete Geo registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, IncompleteGeo_ReportsMissingContainment) {
    KernelRegistry reg;

    GeoKernelDispatch d;
    d.launchDistance    = stubGeoDist;
    d.launchContainment = nullptr;  // Missing
    reg.registerGeoDispatch(BackendType::DIRECTX, d);

    auto r = reg.validate();
    ASSERT_EQ(r.entries.size(), 1u);
    EXPECT_FALSE(r.entries[0].geoComplete);
    EXPECT_FALSE(r.allComplete());

    const auto& missing = r.entries[0].missingSlots;
    ASSERT_EQ(missing.size(), 1u);
    EXPECT_NE(missing[0].find("launchContainment"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Complete Matrix registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, CompleteMatrix_PassesValidation) {
    KernelRegistry reg;

    MatrixKernelDispatch d;
    d.launchMatmul = stubMatMul;
    reg.registerMatrixDispatch(BackendType::CUDA, d);

    auto r = reg.validate();
    ASSERT_EQ(r.entries.size(), 1u);
    EXPECT_TRUE(r.entries[0].matrixComplete);
    EXPECT_TRUE(r.allComplete());
}

// ---------------------------------------------------------------------------
// Multiple backends — partial coverage fails allComplete()
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, MultiBackend_PartialCoverage_FailsAllComplete) {
    KernelRegistry reg;

    ANNKernelDispatch cudaDisp;
    cudaDisp.launchL2Distance   = stubL2;
    cudaDisp.launchCosine       = stubCosine;
    cudaDisp.launchInnerProduct = stubIP;
    cudaDisp.launchTopK         = stubTopK;
    reg.registerANNDispatch(BackendType::CUDA, cudaDisp);

    ANNKernelDispatch hipDisp;
    hipDisp.launchL2Distance   = stubL2;
    hipDisp.launchCosine       = stubCosine;
    hipDisp.launchInnerProduct = stubIP;
    hipDisp.launchTopK         = nullptr;  // Missing
    reg.registerANNDispatch(BackendType::HIP, hipDisp);

    auto r = reg.validate();
    EXPECT_EQ(r.entries.size(), 2u);
    EXPECT_FALSE(r.allComplete());

    // The summary must mention the missing slot.
    const std::string s = r.summary();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("launchTopK"), std::string::npos);
}

// ---------------------------------------------------------------------------
// lookupANNWithFallback — fills null slots from CPU registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, LookupANNWithFallback_FillsNullSlotsFromCPU) {
    KernelRegistry reg;

    ANNKernelDispatch cpu;
    cpu.launchL2Distance   = stubL2;
    cpu.launchCosine       = stubCosine;
    cpu.launchInnerProduct = stubIP;
    cpu.launchTopK         = stubTopK;
    reg.registerANNDispatch(BackendType::CPU, cpu);

    ANNKernelDispatch gpu;
    gpu.launchL2Distance = stubL2;  // Only L2 registered on GPU
    reg.registerANNDispatch(BackendType::CUDA, gpu);

    ANNKernelDispatch resolved = reg.lookupANNWithFallback(BackendType::CUDA);
    EXPECT_EQ(resolved.launchL2Distance,   stubL2)     << "GPU slot preferred for L2";
    EXPECT_EQ(resolved.launchCosine,       stubCosine) << "Fallback CPU cosine slot";
    EXPECT_EQ(resolved.launchInnerProduct, stubIP)     << "Fallback CPU IP slot";
    EXPECT_EQ(resolved.launchTopK,         stubTopK)   << "Fallback CPU topK slot";
}

// ---------------------------------------------------------------------------
// lookupGeoWithFallback — fills null slots from CPU registration
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, LookupGeoWithFallback_FillsNullSlotsFromCPU) {
    KernelRegistry reg;

    GeoKernelDispatch cpu;
    cpu.launchDistance    = stubGeoDist;
    cpu.launchContainment = stubGeoContain;
    reg.registerGeoDispatch(BackendType::CPU, cpu);

    GeoKernelDispatch gpu;  // No slots registered for VULKAN
    reg.registerGeoDispatch(BackendType::VULKAN, gpu);

    GeoKernelDispatch resolved = reg.lookupGeoWithFallback(BackendType::VULKAN);
    EXPECT_EQ(resolved.launchDistance,    stubGeoDist)    << "Fallback CPU distance";
    EXPECT_EQ(resolved.launchContainment, stubGeoContain) << "Fallback CPU containment";
}

// ---------------------------------------------------------------------------
// registeredBackends() — deduplication
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, RegisteredBackends_NoDuplicates) {
    KernelRegistry reg;
    reg.registerANNDispatch(BackendType::CUDA, ANNKernelDispatch{});
    reg.registerGeoDispatch(BackendType::CUDA, GeoKernelDispatch{});
    reg.registerMatrixDispatch(BackendType::CUDA, MatrixKernelDispatch{});

    auto backends = reg.registeredBackends();
    EXPECT_EQ(backends.size(), 1u)
        << "Three registrations for CUDA must deduplicate to one entry";
    EXPECT_EQ(backends[0], BackendType::CUDA);
}

// ---------------------------------------------------------------------------
// clear() restores empty state
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, Clear_RestoresEmptyState) {
    KernelRegistry reg;
    reg.registerANNDispatch(BackendType::CUDA, ANNKernelDispatch{});
    EXPECT_TRUE(reg.hasANNDispatch(BackendType::CUDA));

    reg.clear();
    EXPECT_FALSE(reg.hasANNDispatch(BackendType::CUDA));
    EXPECT_TRUE(reg.registeredBackends().empty());
}

// ---------------------------------------------------------------------------
// BackendRegistry::validateKernels() — convenience delegate
// ---------------------------------------------------------------------------

TEST(KernelRegistryValidateTest, BackendRegistry_ValidateKernels_ReturnsReport) {
    // BackendRegistry::instance() registers CPU backends at construction time
    // (via CPUVectorBackend::populateANNDispatch etc.).  validateKernels()
    // should return a non-crashing ValidationReport regardless of what is
    // registered at test time.
    const ValidationReport r = BackendRegistry::instance().validateKernels();
    // We only assert that the call completes without throwing and returns a
    // coherent report.
    EXPECT_NO_THROW({
        const bool complete = r.allComplete();
        (void)complete;
        const std::string s = r.summary();
        (void)s;
    });
}
