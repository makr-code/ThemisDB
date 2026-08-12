#include <gtest/gtest.h>
#include "acceleration/ai_hardware_dispatcher.h"
#include "acceleration/compute_backend.h"

using namespace themis::acceleration;

// =============================================================================
// AH-1 … AH-8: Enum completeness
// =============================================================================

TEST(AiHardwareDispatcherFocusedTests, AH_1_BackendType_HasNpuApple) {
    EXPECT_EQ(static_cast<int>(BackendType::NPU_APPLE),
              static_cast<int>(BackendType::NPU_APPLE));  // enum value exists
}

TEST(AiHardwareDispatcherFocusedTests, AH_2_BackendType_HasNpuIntel) {
    BackendType bt = BackendType::NPU_INTEL;
    EXPECT_NE(bt, BackendType::CPU);
}

TEST(AiHardwareDispatcherFocusedTests, AH_3_BackendType_HasNpuQualcomm) {
    BackendType bt = BackendType::NPU_QUALCOMM;
    EXPECT_NE(bt, BackendType::CPU);
    EXPECT_NE(bt, BackendType::NPU_INTEL);
}

TEST(AiHardwareDispatcherFocusedTests, AH_4_BackendType_HasNpuArm) {
    BackendType bt = BackendType::NPU_ARM;
    EXPECT_NE(bt, BackendType::NPU_QUALCOMM);
}

TEST(AiHardwareDispatcherFocusedTests, AH_5_BackendType_HasNNAPI) {
    BackendType bt = BackendType::NNAPI;
    EXPECT_NE(bt, BackendType::CPU);
}

TEST(AiHardwareDispatcherFocusedTests, AH_6_BackendType_HasOnnxRuntime) {
    BackendType bt = BackendType::ONNX_RUNTIME;
    EXPECT_NE(bt, BackendType::CPU);
    EXPECT_NE(bt, BackendType::NNAPI);
}

TEST(AiHardwareDispatcherFocusedTests, AH_7_PrecisionMode_HasInt4) {
    auto p = PrecisionMode::INT4;
    EXPECT_TRUE(hasPrecision(p, PrecisionMode::INT4));
    EXPECT_FALSE(hasPrecision(p, PrecisionMode::FP32));
}

TEST(AiHardwareDispatcherFocusedTests, AH_8_PrecisionMode_HasW4A8) {
    auto combo = PrecisionMode::FP32 | PrecisionMode::W4A8 | PrecisionMode::INT4;
    EXPECT_TRUE(hasPrecision(combo, PrecisionMode::FP32));
    EXPECT_TRUE(hasPrecision(combo, PrecisionMode::W4A8));
    EXPECT_TRUE(hasPrecision(combo, PrecisionMode::INT4));
    EXPECT_FALSE(hasPrecision(combo, PrecisionMode::BF16));
}

// =============================================================================
// AH-9 … AH-14: probeCapabilities
// =============================================================================

TEST(AiHardwareDispatcherFocusedTests, AH_9_Initialize_NoThrow) {
    EXPECT_NO_THROW(AiHardwareDispatcher::instance().initialize());
}

TEST(AiHardwareDispatcherFocusedTests, AH_10_ProbeCapabilities_NotEmpty) {
    AiHardwareDispatcher::instance().initialize();
    auto caps = AiHardwareDispatcher::instance().probeCapabilities();
    EXPECT_FALSE(caps.empty());
}

TEST(AiHardwareDispatcherFocusedTests, AH_11_ProbeCapabilities_CpuAlwaysPresent) {
    auto caps = AiHardwareDispatcher::instance().probeCapabilities();
    bool has_cpu = false;
    for (const auto& c : caps) {
        if (c.type == BackendType::CPU && c.available) {
            has_cpu = true;
            break;
        }
    }
    EXPECT_TRUE(has_cpu) << "CPU fallback must always be available";
}

TEST(AiHardwareDispatcherFocusedTests, AH_12_ProbeCapabilities_CpuSupportsFP32) {
    auto caps = AiHardwareDispatcher::instance().probeCapabilities();
    for (const auto& c : caps) {
        if (c.type == BackendType::CPU && c.available) {
            EXPECT_TRUE(hasPrecision(c.supported_precisions, PrecisionMode::FP32));
            break;
        }
    }
}

TEST(AiHardwareDispatcherFocusedTests, AH_13_BestBackend_IsAvailable) {
    AiHardwareDispatcher::instance().initialize();
    BackendType best = AiHardwareDispatcher::instance().bestBackend();
    // Any backend type is valid; just ensure the call doesn't throw
    EXPECT_TRUE(best == BackendType::CPU ||
                best == BackendType::NPU_APPLE   ||
                best == BackendType::NPU_INTEL    ||
                best == BackendType::NPU_QUALCOMM ||
                best == BackendType::NPU_ARM      ||
                best == BackendType::NNAPI        ||
                best == BackendType::ONNX_RUNTIME ||
                best == BackendType::CUDA         ||
                best == BackendType::HIP          ||
                best == BackendType::VULKAN       ||
                best == BackendType::METAL        ||
                best == BackendType::OPENCL       ||
                best == BackendType::DIRECTX      ||
                best == BackendType::ONEAPI);
}

TEST(AiHardwareDispatcherFocusedTests, AH_14_BestOnnxEP_NotEmpty) {
    AiHardwareDispatcher::instance().initialize();
    std::string ep = AiHardwareDispatcher::instance().bestOnnxEP();
    EXPECT_FALSE(ep.empty());
    // Must be a valid ONNX EP name
    EXPECT_TRUE(ep.find("ExecutionProvider") != std::string::npos)
        << "Expected ORT EP name, got: " << ep;
}

// =============================================================================
// AH-15 … AH-20: run() fallback chain
// =============================================================================

TEST(AiHardwareDispatcherFocusedTests, AH_15_Run_NullInput_ReturnsError) {
    AiInferenceRequest req;
    req.input_data     = nullptr;
    req.input_elements = 0;
    req.input_shape    = {1, 4};
    req.task_tag       = "embedding";
    req.model_path     = "/nonexistent/model.onnx";

    AiInferenceResult res = AiHardwareDispatcher::instance().run(req);
    // All backends should reject null input
    EXPECT_FALSE(res.success);
}

TEST(AiHardwareDispatcherFocusedTests, AH_16_Run_ValidInput_CpuFallback) {
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    AiInferenceRequest req;
    req.input_data        = data.data();
    req.input_elements    = data.size();
    req.input_shape       = {1, 4};
    req.task_tag          = "embedding";
    req.model_path        = "";              // no model — forces CPU identity fallback
    req.preferred_precision = PrecisionMode::FP32;

    AiInferenceResult res = AiHardwareDispatcher::instance().run(req);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.backend_used, BackendType::CPU);
    EXPECT_EQ(res.output.size(), data.size());
}

TEST(AiHardwareDispatcherFocusedTests, AH_17_Run_OutputMatchesInput_CpuFallback) {
    std::vector<float> data = {0.1f, 0.2f, 0.3f};
    AiInferenceRequest req;
    req.input_data        = data.data();
    req.input_elements    = data.size();
    req.input_shape       = {1, 3};
    req.task_tag          = "embedding";
    req.model_path        = "";
    req.preferred_precision = PrecisionMode::FP32;

    AiInferenceResult res = AiHardwareDispatcher::instance().run(req);
    ASSERT_TRUE(res.success);
    ASSERT_EQ(res.output.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_FLOAT_EQ(res.output[i], data[i]);
    }
}

TEST(AiHardwareDispatcherFocusedTests, AH_18_Run_LatencyPositive) {
    std::vector<float> data(16, 1.0f);
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 16};
    req.task_tag       = "classify";
    req.model_path     = "";
    req.preferred_precision = PrecisionMode::FP32;

    AiInferenceResult res = AiHardwareDispatcher::instance().run(req);
    EXPECT_TRUE(res.success);
    EXPECT_GE(res.latency_ms, 0.0);
}

TEST(AiHardwareDispatcherFocusedTests, AH_19_Run_ChosenBackendFilledIn) {
    std::vector<float> data(8, 0.5f);
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 8};
    req.task_tag       = "rerank";
    req.model_path     = "";
    req.preferred_precision = PrecisionMode::FP32;

    AiHardwareDispatcher::instance().run(req);
    // chosen_backend must be set by the dispatcher
    EXPECT_NE(req.chosen_backend, static_cast<BackendType>(-1));
}

TEST(AiHardwareDispatcherFocusedTests, AH_20_Run_IdempotentSingletonState) {
    // Multiple initialize() calls must not corrupt state
    AiHardwareDispatcher::instance().initialize();
    AiHardwareDispatcher::instance().initialize();
    AiHardwareDispatcher::instance().initialize(/*force=*/true);
    auto caps = AiHardwareDispatcher::instance().probeCapabilities();
    EXPECT_FALSE(caps.empty());
}

// =============================================================================
// AH-21 … AH-25: runOn() per-backend graceful errors
// =============================================================================

TEST(AiHardwareDispatcherFocusedTests, AH_21_RunOn_NpuApple_EmptyInput_Error) {
    AiInferenceRequest req;
    req.input_data     = nullptr;
    req.input_elements = 0;
    req.input_shape    = {};
    req.model_path     = "";
    auto res = AiHardwareDispatcher::instance().runOn(BackendType::NPU_APPLE, req);
    // Either unavailable error or invalid-input error — never a crash
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error.empty());
}

TEST(AiHardwareDispatcherFocusedTests, AH_22_RunOn_NpuIntel_Graceful) {
    std::vector<float> data(4, 1.0f);
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 4};
    req.model_path     = "/nonexistent/model.onnx";
    auto res = AiHardwareDispatcher::instance().runOn(BackendType::NPU_INTEL, req);
    // On CI without OpenVINO this should return a graceful error, never throw
    EXPECT_FALSE(res.error.empty());
}

TEST(AiHardwareDispatcherFocusedTests, AH_22b_RunOn_NpuApple_UsesInjectedBridge) {
    std::vector<float> data = {9.0f, 8.0f};
    AiHardwareDispatcher::setAppleANEDispatchFn([](AiInferenceRequest& req) {
        AiInferenceResult result;
        result.success = true;
        result.backend_used = BackendType::NPU_APPLE;
        result.output.assign(req.input_data, req.input_data + req.input_elements);
        result.output_shape = req.input_shape;
        result.ep_used = "InjectedAppleANE";
        return result;
    });

    AiInferenceRequest req;
    req.input_data = data.data();
    req.input_elements = data.size();
    req.input_shape = {1, 2};
    req.model_path = "";

    auto res = AiHardwareDispatcher::instance().runOn(BackendType::NPU_APPLE, req);
    AiHardwareDispatcher::setAppleANEDispatchFn(nullptr);

    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.backend_used, BackendType::NPU_APPLE);
    ASSERT_EQ(res.output.size(), data.size());
    EXPECT_FLOAT_EQ(res.output[0], 9.0f);
    EXPECT_EQ(res.ep_used, "InjectedAppleANE");
}

TEST(AiHardwareDispatcherFocusedTests, AH_23_RunOn_OnnxRuntime_NoModel_Error) {
    std::vector<float> data(4, 0.0f);
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 4};
    req.model_path     = "";   // empty path → must fail gracefully
    auto res = AiHardwareDispatcher::instance().runOn(BackendType::ONNX_RUNTIME, req);
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error.empty());
}

TEST(AiHardwareDispatcherFocusedTests, AH_24_RunOn_Cpu_Success) {
    std::vector<float> data = {7.0f, 8.0f};
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 2};
    req.model_path     = "";
    req.preferred_precision = PrecisionMode::FP32;
    auto res = AiHardwareDispatcher::instance().runOn(BackendType::CPU, req);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.backend_used, BackendType::CPU);
}

TEST(AiHardwareDispatcherFocusedTests, AH_25_RunOn_Nnapi_NonAndroid_Error) {
    std::vector<float> data(4, 1.0f);
    AiInferenceRequest req;
    req.input_data     = data.data();
    req.input_elements = data.size();
    req.input_shape    = {1, 4};
    req.model_path     = "";
    auto res = AiHardwareDispatcher::instance().runOn(BackendType::NNAPI, req);
#if defined(__ANDROID__)
    // On Android this may succeed (NNAPI delegate available)
    EXPECT_FALSE(res.error.empty() && res.success);
#else
    // Not Android: must return graceful error
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error.empty());
#endif
}

// =============================================================================
// AH-26 … AH-30: DeviceCapabilityInfo / BackendCapabilities new fields
// =============================================================================

TEST(AiHardwareDispatcherFocusedTests, AH_26_DeviceCapabilityInfo_NpuFields) {
    DeviceCapabilityInfo info;
    info.is_npu    = true;
    info.npu_tops  = 38;
    info.supports_int4 = true;
    info.supports_w4a8 = false;
    info.onnx_ep   = "CoreMLExecutionProvider";

    EXPECT_TRUE(info.is_npu);
    EXPECT_EQ(info.npu_tops, 38u);
    EXPECT_TRUE(info.supports_int4);
    EXPECT_FALSE(info.supports_w4a8);
    EXPECT_EQ(info.onnx_ep, "CoreMLExecutionProvider");
}

TEST(AiHardwareDispatcherFocusedTests, AH_27_BackendCapabilities_AiInferenceField) {
    BackendCapabilities caps;
    caps.supportsAiInference = true;
    caps.npuTops             = 45;
    caps.preferredOnnxEP     = "QNNExecutionProvider";
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::INT4 |
                               PrecisionMode::W4A8;

    EXPECT_TRUE(caps.supportsAiInference);
    EXPECT_EQ(caps.npuTops, 45u);
    EXPECT_EQ(caps.preferredOnnxEP, "QNNExecutionProvider");
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::W4A8));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::BF16));
}

TEST(AiHardwareDispatcherFocusedTests, AH_28_PrecisionMode_Bitmask_Orthogonal) {
    // Verify no two modes share the same bit
    const PrecisionMode modes[] = {
        PrecisionMode::FP32, PrecisionMode::FP16, PrecisionMode::BF16,
        PrecisionMode::INT8, PrecisionMode::INT4, PrecisionMode::FP4,
        PrecisionMode::W4A8, PrecisionMode::W8A8
    };
    for (size_t i = 0; i < std::size(modes); ++i) {
        for (size_t j = i + 1; j < std::size(modes); ++j) {
            EXPECT_EQ(static_cast<uint32_t>(modes[i]) &
                      static_cast<uint32_t>(modes[j]), 0u)
                << "Precision mode bits " << i << " and " << j << " overlap";
        }
    }
}

TEST(AiHardwareDispatcherFocusedTests, AH_29_HasAccelerator_ConsistentWithCapabilities) {
    AiHardwareDispatcher::instance().initialize(/*force=*/true);
    bool has_acc = AiHardwareDispatcher::instance().hasAccelerator();
    auto caps    = AiHardwareDispatcher::instance().probeCapabilities();

    bool any_non_cpu = false;
    for (const auto& c : caps) {
        if (c.available && c.type != BackendType::CPU) {
            any_non_cpu = true;
            break;
        }
    }
    // hasAccelerator() must agree with what probeCapabilities() reports
    EXPECT_EQ(has_acc, any_non_cpu)
        << "hasAccelerator() disagreed with probeCapabilities(): "
        << "hasAccelerator=" << has_acc << ", any_non_cpu=" << any_non_cpu;
}

TEST(AiHardwareDispatcherFocusedTests, AH_30_HasNPU_ConsistentWithCapabilities) {
    AiHardwareDispatcher::instance().initialize(/*force=*/true);
    bool has_npu = AiHardwareDispatcher::instance().hasNPU();
    auto caps    = AiHardwareDispatcher::instance().probeCapabilities();

    bool any_npu_cap = false;
    for (const auto& c : caps) {
        if (!c.available) continue;
        if (c.type == BackendType::NPU_APPLE   ||
            c.type == BackendType::NPU_INTEL    ||
            c.type == BackendType::NPU_QUALCOMM ||
            c.type == BackendType::NPU_ARM      ||
            c.type == BackendType::NNAPI) {
            any_npu_cap = true;
            break;
        }
    }
    EXPECT_EQ(has_npu, any_npu_cap)
        << "hasNPU() disagreed with probeCapabilities(): "
        << "hasNPU=" << has_npu << ", any_npu_cap=" << any_npu_cap;
}

// AH-31: logCapabilities() must not throw or crash after initialization.
// It is a startup logging helper — the test verifies that calling it on a
// fully initialized dispatcher completes without any exception.
TEST(AiHardwareDispatcherFocusedTests, AH_31_LogCapabilities_NoThrow) {
    AiHardwareDispatcher::instance().initialize(/*force=*/true);
    EXPECT_NO_THROW(AiHardwareDispatcher::instance().logCapabilities());
}

TEST(AiHardwareDispatcherFocusedTests, AH_32_RunOnCuda_VectorSimilarity_L2) {
    std::vector<float> queries = {1.0f, 0.0f};  // 1 query, dim=2
    std::vector<float> vectors = {
        1.0f, 0.0f,  // idx 0
        0.0f, 1.0f,  // idx 1
        2.0f, 0.0f   // idx 2
    };

    AiInferenceRequest req;
    req.task_tag               = "vector_similarity_l2";
    req.input_data             = queries.data();
    req.input_elements         = queries.size();
    req.similarity_corpus      = vectors.data();
    req.similarity_num_queries = 1;
    req.similarity_num_vectors = 3;
    req.similarity_dim         = 2;
    req.similarity_top_k       = 2;

    auto res = AiHardwareDispatcher::instance().runOn(BackendType::CUDA, req);
    ASSERT_TRUE(res.success);
    ASSERT_EQ(res.topk_indices.size(), 2u);
    ASSERT_EQ(res.topk_distances.size(), 2u);
    EXPECT_EQ(res.topk_indices[0], 0u);
    EXPECT_NEAR(res.topk_distances[0], 0.0f, 1e-6f);
}

TEST(AiHardwareDispatcherFocusedTests, AH_33_RunOnCuda_VectorSimilarity_Cosine) {
    std::vector<float> queries = {1.0f, 0.0f};  // 1 query, dim=2
    std::vector<float> vectors = {
        1.0f, 0.0f,  // idx 0 cosine distance 0
        0.0f, 1.0f,  // idx 1 cosine distance 1
        0.5f, 0.0f   // idx 2 cosine distance 0
    };

    AiInferenceRequest req;
    req.task_tag               = "vector_similarity_cosine";
    req.input_data             = queries.data();
    req.input_elements         = queries.size();
    req.similarity_corpus      = vectors.data();
    req.similarity_num_queries = 1;
    req.similarity_num_vectors = 3;
    req.similarity_dim         = 2;
    req.similarity_top_k       = 1;

    auto res = AiHardwareDispatcher::instance().runOn(BackendType::CUDA, req);
    ASSERT_TRUE(res.success);
    ASSERT_EQ(res.topk_indices.size(), 1u);
    EXPECT_TRUE(res.topk_indices[0] == 0u || res.topk_indices[0] == 2u);
    EXPECT_NEAR(res.topk_distances[0], 0.0f, 1e-5f);
}

TEST(AiHardwareDispatcherFocusedTests, AH_34_VectorSimilarity_InvalidShape_ReturnsError) {
    std::vector<float> queries = {1.0f, 0.0f, 1.0f};  // invalid: expected 2 elements
    std::vector<float> vectors = {
        1.0f, 0.0f,  // idx 0
        0.0f, 1.0f   // idx 1
    };

    AiInferenceRequest req;
    req.task_tag               = "vector_similarity_l2";
    req.input_data             = queries.data();
    req.input_elements         = queries.size();
    req.similarity_corpus      = vectors.data();
    req.similarity_num_queries = 1;
    req.similarity_num_vectors = 2;
    req.similarity_dim         = 2;
    req.similarity_top_k       = 1;

    auto res = AiHardwareDispatcher::instance().runOn(BackendType::CUDA, req);
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error.empty());
}
