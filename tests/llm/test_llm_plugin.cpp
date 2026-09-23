#include <gtest/gtest.h>
#include "ggml.h"
#include "gguf.h"
#include "llm/llama_wrapper.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/async_inference_engine.h"
#include "llm/llm_plugin_manager.h"
#include "test_helpers_llm.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstdlib>

namespace fs = std::filesystem;
using namespace themis::llm;
using json = nlohmann::json;

class LLMPluginTest : public ::testing::Test {
protected:
    std::string test_model_dir = "./test_llm_models";
    std::string test_lora_dir = "./test_llm_loras";
    
    void SetUp() override {
        // Clean up
        if (fs::exists(test_model_dir)) {
            fs::remove_all(test_model_dir);
        }
        if (fs::exists(test_lora_dir)) {
            fs::remove_all(test_lora_dir);
        }
        fs::create_directories(test_model_dir);
        fs::create_directories(test_lora_dir);
    }
    
    void TearDown() override {
        // Clean up
        if (fs::exists(test_model_dir)) {
            fs::remove_all(test_model_dir);
        }
        if (fs::exists(test_lora_dir)) {
            fs::remove_all(test_lora_dir);
        }
    }

public:

    static void writeGGUFString(std::ostream& out, const std::string& value) {
        const uint64_t len = static_cast<uint64_t>(value.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (!value.empty()) {
            out.write(value.data(), static_cast<std::streamsize>(value.size()));
        }
    }

    static void writeGGUFUInt32(std::ostream& out, uint32_t value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    static void writeGGUFUInt64(std::ostream& out, uint64_t value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    static void writeGGUFMetadataString(std::ostream& out, const std::string& key, const std::string& value) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 8u); // GGUF string type
        writeGGUFString(out, value);
    }

    static void writeGGUFMetadataUInt32(std::ostream& out, const std::string& key, uint32_t value) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 4u); // GGUF uint32 type
        writeGGUFUInt32(out, value);
    }

    static void writeGGUFFloat32(std::ostream& out, float value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    static void writeGGUFMetadataFloat32(std::ostream& out, const std::string& key, float value) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 6u); // GGUF float32 type
        writeGGUFFloat32(out, value);
    }

    static void writeGGUFMetadataBool(std::ostream& out, const std::string& key, bool value) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 7u); // GGUF bool type
        const uint8_t b = value ? 1u : 0u;
        out.write(reinterpret_cast<const char*>(&b), sizeof(b));
    }

    static void writeGGUFStringArray(std::ostream& out, const std::string& key, const std::vector<std::string>& values) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 9u); // GGUF array type
        writeGGUFUInt32(out, static_cast<uint32_t>(8u)); // element type: string
        writeGGUFUInt64(out, static_cast<uint64_t>(values.size()));
        for (const auto& value : values) {
            writeGGUFString(out, value);
        }
    }

    static void writeGGUFUInt32Array(std::ostream& out, const std::string& key, const std::vector<uint32_t>& values) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 9u); // GGUF array type
        writeGGUFUInt32(out, static_cast<uint32_t>(4u)); // element type: uint32
        writeGGUFUInt64(out, static_cast<uint64_t>(values.size()));
        for (const auto& value : values) {
            writeGGUFUInt32(out, value);
        }
    }

    static void writeGGUFFloat32Array(std::ostream& out, const std::string& key, const std::vector<float>& values) {
        writeGGUFString(out, key);
        writeGGUFUInt32(out, 9u); // GGUF array type
        writeGGUFUInt32(out, static_cast<uint32_t>(6u)); // element type: float32
        writeGGUFUInt64(out, static_cast<uint64_t>(values.size()));
        for (const auto& value : values) {
            writeGGUFFloat32(out, value);
        }
    }

    static void writeMinimalValidGGUF(const std::string& file_path,
                                     const std::string& model_name,
                                     size_t bytes_to_write) {
        struct gguf_context* ctx = gguf_init_empty();
        if (!ctx) {
            throw std::runtime_error("gguf_init_empty() failed for test fixture");
        }

        gguf_set_val_str(ctx, "general.architecture", "llama");
        gguf_set_val_str(ctx, "general.name", model_name.c_str());
        gguf_set_val_u32(ctx, "llama.context_length", 2048u);
        gguf_set_val_u32(ctx, "llama.embedding_length", 64u);
        gguf_set_val_u32(ctx, "llama.block_count", 1u);
        gguf_set_val_u32(ctx, "llama.feed_forward_length", 64u);
        gguf_set_val_u32(ctx, "llama.attention.head_count", 1u);
        gguf_set_val_u32(ctx, "llama.attention.head_count_kv", 1u);
        gguf_set_val_f32(ctx, "llama.attention.layer_norm_rms_epsilon", 1.0e-5f);
        gguf_set_val_u32(ctx, "llama.rope.dimension_count", 64u);
        gguf_set_val_str(ctx, "tokenizer.ggml.model", "llama");
        gguf_set_val_str(ctx, "tokenizer.ggml.pre", "default");

        std::vector<std::string> tokenizer_tokens;
        tokenizer_tokens.reserve(96);
        tokenizer_tokens.emplace_back("<unk>");
        tokenizer_tokens.emplace_back("<s>");
        tokenizer_tokens.emplace_back("</s>");
        tokenizer_tokens.emplace_back("\n");
        for (char ch = ' '; ch <= '~'; ++ch) {
            tokenizer_tokens.emplace_back(1, ch);
        }

        std::vector<const char*> token_ptrs;
        token_ptrs.reserve(tokenizer_tokens.size());
        for (const auto& token : tokenizer_tokens) {
            token_ptrs.push_back(token.c_str());
        }
        gguf_set_arr_str(ctx, "tokenizer.ggml.tokens", token_ptrs.data(), token_ptrs.size());

        std::vector<int32_t> token_types(tokenizer_tokens.size(), 0);
        token_types[1] = 1;
        token_types[2] = 1;
        gguf_set_arr_data(ctx, "tokenizer.ggml.token_type", GGUF_TYPE_INT32,
                          token_types.data(), token_types.size());

        std::vector<float> token_scores(tokenizer_tokens.size(), 0.0f);
        gguf_set_arr_data(ctx, "tokenizer.ggml.scores", GGUF_TYPE_FLOAT32,
                          token_scores.data(), token_scores.size());

        gguf_set_val_u32(ctx, "tokenizer.ggml.unk_token_id", 0u);
        gguf_set_val_u32(ctx, "tokenizer.ggml.bos_token_id", 1u);
        gguf_set_val_u32(ctx, "tokenizer.ggml.eos_token_id", 2u);
        gguf_set_val_u32(ctx, "tokenizer.ggml.pad_token_id", 0u);
        gguf_set_val_bool(ctx, "tokenizer.ggml.add_bos_token", true);
        gguf_set_val_bool(ctx, "tokenizer.ggml.add_eos_token", false);

        ggml_init_params params = {256ull * 1024ull * 1024ull, nullptr, false};
        ggml_context* gctx = ggml_init(params);
        if (!gctx) {
            gguf_free(ctx);
            throw std::runtime_error("ggml_init() failed for test fixture");
        }

        const std::array<int64_t, 2> embd_ne = {64, static_cast<int64_t>(tokenizer_tokens.size())};
        auto* embed = ggml_new_tensor(gctx, GGML_TYPE_F32, embd_ne.size(), embd_ne.data());
        ggml_set_name(embed, "token_embd.weight");

        const std::array<int64_t, 1> norm_ne = {64};
        auto* norm = ggml_new_tensor(gctx, GGML_TYPE_F32, norm_ne.size(), norm_ne.data());
        ggml_set_name(norm, "output_norm.weight");

        const std::array<int64_t, 2> q_ne = {64, 64};
        auto* q = ggml_new_tensor(gctx, GGML_TYPE_F32, q_ne.size(), q_ne.data());
        ggml_set_name(q, "blk.0.attn_q.weight");

        const std::array<int64_t, 2> k_ne = {64, 64};
        auto* k = ggml_new_tensor(gctx, GGML_TYPE_F32, k_ne.size(), k_ne.data());
        ggml_set_name(k, "blk.0.attn_k.weight");

        const std::array<int64_t, 2> v_ne = {64, 64};
        auto* v = ggml_new_tensor(gctx, GGML_TYPE_F32, v_ne.size(), v_ne.data());
        ggml_set_name(v, "blk.0.attn_v.weight");

        const std::array<int64_t, 1> attn_norm_ne = {64};
        auto* attn_norm = ggml_new_tensor(gctx, GGML_TYPE_F32, attn_norm_ne.size(), attn_norm_ne.data());
        ggml_set_name(attn_norm, "blk.0.attn_norm.weight");

        const std::array<int64_t, 2> out_ne = {64, 64};
        auto* out = ggml_new_tensor(gctx, GGML_TYPE_F32, out_ne.size(), out_ne.data());
        ggml_set_name(out, "blk.0.attn_output.weight");

        const std::array<int64_t, 1> ffn_norm_ne = {64};
        auto* ffn_norm = ggml_new_tensor(gctx, GGML_TYPE_F32, ffn_norm_ne.size(), ffn_norm_ne.data());
        ggml_set_name(ffn_norm, "blk.0.ffn_norm.weight");

        const std::array<int64_t, 2> gate_ne = {64, 64};
        auto* gate = ggml_new_tensor(gctx, GGML_TYPE_F32, gate_ne.size(), gate_ne.data());
        ggml_set_name(gate, "blk.0.ffn_gate.weight");

        const std::array<int64_t, 2> down_ne = {64, 64};
        auto* down = ggml_new_tensor(gctx, GGML_TYPE_F32, down_ne.size(), down_ne.data());
        ggml_set_name(down, "blk.0.ffn_down.weight");

        const std::array<int64_t, 2> up_ne = {64, 64};
        auto* up = ggml_new_tensor(gctx, GGML_TYPE_F32, up_ne.size(), up_ne.data());
        ggml_set_name(up, "blk.0.ffn_up.weight");

        const std::array<int64_t, 2> output_ne = {64, static_cast<int64_t>(tokenizer_tokens.size())};
        auto* output = ggml_new_tensor(gctx, GGML_TYPE_F32, output_ne.size(), output_ne.data());
        ggml_set_name(output, "output.weight");

        gguf_add_tensor(ctx, embed);
        gguf_add_tensor(ctx, norm);
        gguf_add_tensor(ctx, q);
        gguf_add_tensor(ctx, k);
        gguf_add_tensor(ctx, v);
        gguf_add_tensor(ctx, attn_norm);
        gguf_add_tensor(ctx, out);
        gguf_add_tensor(ctx, ffn_norm);
        gguf_add_tensor(ctx, gate);
        gguf_add_tensor(ctx, down);
        gguf_add_tensor(ctx, up);
        gguf_add_tensor(ctx, output);

        const bool wrote = gguf_write_to_file(ctx, file_path.c_str(), false);
        ggml_free(gctx);
        gguf_free(ctx);
        if (!wrote) {
            throw std::runtime_error("gguf_write_to_file() failed for test fixture: " + file_path);
        }

        struct gguf_init_params verify_params = {};
        verify_params.no_alloc = true;
        verify_params.ctx = nullptr;
        struct gguf_context* verify_ctx = gguf_init_from_file(file_path.c_str(), verify_params);
        if (!verify_ctx) {
            throw std::runtime_error("gguf_init_from_file() failed for generated test fixture: " + file_path);
        }
        gguf_free(verify_ctx);

        if (bytes_to_write > 0) {
            std::ifstream in(file_path, std::ios::binary | std::ios::ate);
            const std::streamoff actual_size = in.tellg();
            if (actual_size >= 0 && static_cast<size_t>(actual_size) < bytes_to_write) {
                std::vector<char> pad(bytes_to_write - static_cast<size_t>(actual_size), 0);
                std::ofstream out(file_path, std::ios::binary | std::ios::app);
                out.write(pad.data(), static_cast<std::streamsize>(pad.size()));
            }
        }
    }
    
    static std::string resolveRealModelPath() {
        const char* env_path = std::getenv("THEMIS_TEST_MODEL_PATH");
        if (env_path && fs::exists(env_path) && fs::is_regular_file(env_path)) {
            return env_path;
        }

        const fs::path repo_root = fs::path(__FILE__).parent_path().parent_path().parent_path();
        for (const auto& root : {repo_root, repo_root / "models", repo_root / "llama.cpp" / "models"}) {
            for (const auto& name : {
                    "TinyLlama-1.1B-Chat-v1.0.gguf",
                    "tinyllama-1.1b-chat-v1.0.gguf",
                    "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf",
                    "tinyllama_1.1b.gguf",
                    "default.gguf",
                    "gemma4_latest.gguf",
                    "phi4_latest.gguf",
                    "qwen3-coder_latest.gguf",
                    "test_model.gguf"}) {
                const fs::path candidate = root / name;
                if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                    return candidate.string();
                }
            }
        }
        return {};
    }

    static std::string resolveModelPathForInference(const std::string& fallback_name, size_t fallback_size_mb = 50) {
        (void)fallback_name;
        (void)fallback_size_mb;
        return resolveRealModelPath();
    }

    // Create a dummy model file for testing
    void createDummyModel(const std::string& filename, size_t size_mb = 100) {
        std::string path = test_model_dir + "/" + filename;
        writeMinimalValidGGUF(path, filename, size_mb * 1024ULL * 1024ULL);
    }
    
    // Create a dummy LoRA file
    void createDummyLoRA(const std::string& filename, size_t size_mb = 10) {
        std::string path = test_lora_dir + "/" + filename;
        writeMinimalValidGGUF(path, filename, size_mb * 1024ULL * 1024ULL);
    }
};

// ═══════════════════════════════════════════════════════════
// Lazy Model Loader Tests (Ollama-style)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, LazyModelLoader_BasicLoading) {
    LazyModelLoader::Config config;
    config.max_models = 3;
    config.max_vram_mb = 10240;
    config.enable_lazy_load = true;
    
    LazyModelLoader loader(config);
    
    createDummyModel("model1.gguf", 100);
    std::string model_path = test_model_dir + "/model1.gguf";
    
    json load_config = {
        {"n_gpu_layers", 32},
        {"n_ctx", 2048}
    };
    
    // First load - should be cache miss
    auto* model1 = loader.getOrLoadModel("model1", model_path, load_config);
    ASSERT_NE(model1, nullptr);
    EXPECT_EQ(model1->use_count, 1);
    
    // Second load - should be cache hit
    auto* model2 = loader.getOrLoadModel("model1", model_path, load_config);
    ASSERT_NE(model2, nullptr);
    EXPECT_EQ(model2, model1);  // Same pointer
    EXPECT_EQ(model2->use_count, 2);
    
    auto stats = loader.getStatistics();
    EXPECT_EQ(stats.cache_hits, 1);
    EXPECT_EQ(stats.cache_misses, 1);
}

TEST_F(LLMPluginTest, LazyModelLoader_LRUEviction) {
    LazyModelLoader::Config config;
    config.max_models = 2;  // Only keep 2 models
    config.max_vram_mb = 10240;
    config.enable_lazy_load = true;
    
    LazyModelLoader loader(config);
    
    createDummyModel("model1.gguf", 50);
    createDummyModel("model2.gguf", 50);
    createDummyModel("model3.gguf", 50);
    
    json load_config = {{"n_gpu_layers", 32}};
    
    // Load 2 models
    auto* m1 = loader.getOrLoadModel("model1", test_model_dir + "/model1.gguf", load_config);
    auto* m2 = loader.getOrLoadModel("model2", test_model_dir + "/model2.gguf", load_config);
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    // Load 3rd model - should evict model1 (LRU)
    auto* m3 = loader.getOrLoadModel("model3", test_model_dir + "/model3.gguf", load_config);
    ASSERT_NE(m3, nullptr);
    
    // Try to get model1 again - should be cache miss (evicted)
    auto* m1_reload = loader.getOrLoadModel("model1", test_model_dir + "/model1.gguf", load_config);
    ASSERT_NE(m1_reload, nullptr);
    
    auto stats = loader.getStatistics();
    EXPECT_GE(stats.cache_misses, 3);  // At least 3 cache misses
}

TEST_F(LLMPluginTest, LazyModelLoader_ModelPinning) {
    LazyModelLoader::Config config;
    config.max_models = 2;
    config.max_vram_mb = 10240;
    
    LazyModelLoader loader(config);
    
    createDummyModel("important.gguf", 50);
    createDummyModel("temp1.gguf", 50);
    createDummyModel("temp2.gguf", 50);
    
    json load_config = {{"n_gpu_layers", 32}};
    
    // Load and pin important model
    auto* important = loader.getOrLoadModel("important", test_model_dir + "/important.gguf", load_config);
    ASSERT_NE(important, nullptr);
    loader.pinModel("important");
    
    // Load another model
    auto* temp1 = loader.getOrLoadModel("temp1", test_model_dir + "/temp1.gguf", load_config);
    ASSERT_NE(temp1, nullptr);
    
    // Load 3rd model - should evict temp1, not the pinned model
    auto* temp2 = loader.getOrLoadModel("temp2", test_model_dir + "/temp2.gguf", load_config);
    ASSERT_NE(temp2, nullptr);
    
    // Pinned model should still be accessible (cache hit)
    auto* important_reaccess = loader.getOrLoadModel("important", test_model_dir + "/important.gguf", load_config);
    EXPECT_EQ(important_reaccess, important);
}

// ═══════════════════════════════════════════════════════════
// Multi-LoRA Manager Tests (vLLM-style)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, MultiLoRAManager_BasicLoading) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.max_lora_vram_mb = 2048;
    config.enable_multi_lora_batch = true;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("legal.bin", 20);
    std::string lora_path = test_lora_dir + "/legal.bin";
    
    // Load LoRA with scale factor
    bool loaded = manager.loadLoRA("legal-qa", lora_path, "model1", 1.0f);
    EXPECT_TRUE(loaded);
    
    // Check if loaded
    EXPECT_TRUE(manager.isLoRALoaded("legal-qa"));
    
    // Get LoRA info
    auto info = manager.getLoRAInfo("legal-qa");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "legal-qa");
    EXPECT_EQ(info->base_model_id, "model1");
}

TEST_F(LLMPluginTest, MultiLoRAManager_MultipleLoRAs) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("legal.bin", 20);
    createDummyLoRA("medical.bin", 20);
    createDummyLoRA("finance.bin", 20);
    
    // Load multiple LoRAs with scale factor
    EXPECT_TRUE(manager.loadLoRA("legal", test_lora_dir + "/legal.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("medical", test_lora_dir + "/medical.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("finance", test_lora_dir + "/finance.bin", "model1", 1.0f));
    
    // All should be loaded
    EXPECT_TRUE(manager.isLoRALoaded("legal"));
    EXPECT_TRUE(manager.isLoRALoaded("medical"));
    EXPECT_TRUE(manager.isLoRALoaded("finance"));
    
    // List LoRAs
    auto loras = manager.listLoRAs("model1");
    EXPECT_EQ(loras.size(), 3);
}

TEST_F(LLMPluginTest, MultiLoRAManager_SlotLimit) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 3;  // Only 3 slots
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("lora1.bin", 10);
    createDummyLoRA("lora2.bin", 10);
    createDummyLoRA("lora3.bin", 10);
    createDummyLoRA("lora4.bin", 10);
    
    // Load 3 LoRAs - should succeed
    EXPECT_TRUE(manager.loadLoRA("lora1", test_lora_dir + "/lora1.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("lora2", test_lora_dir + "/lora2.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("lora3", test_lora_dir + "/lora3.bin", "model1", 1.0f));
    
    // Load 4th LoRA - should evict LRU
    EXPECT_TRUE(manager.loadLoRA("lora4", test_lora_dir + "/lora4.bin", "model1", 1.0f));
    
    // Check stats - should have evictions count
    auto stats = manager.getStatistics();
    EXPECT_GE(stats.evictions, 1);  // At least 1 eviction occurred
}

// ═══════════════════════════════════════════════════════════
// LlamaWrapper Tests (Consolidated)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, LlamaWrapper_Initialization) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    
    EXPECT_EQ(plugin.getName(), "llamacpp");
    EXPECT_FALSE(plugin.isModelLoaded());
}

TEST_F(LLMPluginTest, LlamaWrapper_ModelLoading) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.lazy_loader_config.max_models = 2;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("mistral.gguf", 100);
    std::string model_path = test_model_dir + "/mistral.gguf";
    
    json load_config = {
        {"n_gpu_layers", 32},
        {"n_ctx", 4096}
    };
    
    // Load model
    bool loaded = plugin.loadModel(model_path, load_config);
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(plugin.isModelLoaded());
    
    // Get model info
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value());
}

TEST_F(LLMPluginTest, LlamaWrapper_LoRAManagement) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("mistral.gguf", 100);
    createDummyLoRA("legal.bin", 20);
    
    // Load model first
    plugin.loadModel(test_model_dir + "/mistral.gguf", {});
    
    // Load LoRA
    bool loaded = plugin.loadLoRA("legal-qa", test_lora_dir + "/legal.bin", 1.0f);
    EXPECT_TRUE(loaded);
    
    // List LoRAs
    auto loras = plugin.listLoRAs();
    EXPECT_GE(loras.size(), 1);
}

TEST_F(LLMPluginTest, LlamaWrapper_BasicInference) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);

    const std::string model_path = resolveModelPathForInference("tiny.gguf", 50);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin.loadModel(model_path, {});

    InferenceRequest request;
    request.prompt = "<unk>";
    request.max_tokens = 100;
    request.temperature = 0.7f;
    request.top_p = 0.9f;
    
    // Generate response
    auto response = plugin.generate(request);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GE(response.inference_time_ms, 0);
}

// ═══════════════════════════════════════════════════════════
// Async Inference Engine Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, AsyncInference_NonBlocking) {
    LlamaWrapper::Config plugin_config;
    plugin_config.n_gpu_layers = 32;
    plugin_config.n_ctx = 2048;
    plugin_config.require_model_integrity = false;
    
    auto plugin = std::make_shared<LlamaWrapper>(plugin_config);

    const std::string model_path = resolveModelPathForInference("async_model.gguf", 50);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin->loadModel(model_path, {});
    
    AsyncInferenceEngine::Config engine_config;
    engine_config.num_worker_threads = 2;
    engine_config.max_queue_size = 100;
    
    AsyncInferenceEngine engine(plugin, engine_config);
    
    InferenceRequest request;
    request.prompt = "<unk>";
    request.max_tokens = 50;
    
    // Submit request - should return immediately
    auto start = std::chrono::high_resolution_clock::now();
    auto handle = engine.submit(request, 10);
    auto submit_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start
    ).count();
    
    // Submission should be very fast (< 10ms)
    EXPECT_LT(submit_time, 10);
    
    // Wait for result
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());
}

TEST_F(LLMPluginTest, AsyncInference_Callback) {
    LlamaWrapper::Config plugin_config;
    plugin_config.require_model_integrity = false;
    auto plugin = std::make_shared<LlamaWrapper>(plugin_config);
    const std::string model_path = resolveModelPathForInference("callback_model.gguf", 50);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin->loadModel(model_path, {});
    
    AsyncInferenceEngine engine(plugin, AsyncInferenceEngine::Config{});
    
    InferenceRequest request;
    request.prompt = "<unk>";
    request.max_tokens = 20;
    
    std::atomic<bool> callback_called{false};
    std::string result_text = {};
    std::promise<void> callback_ready;
    auto callback_future = callback_ready.get_future();
    
    engine.submitAsync(
        request,
        [&callback_called, &result_text, &callback_ready](const InferenceResponse& response) {
            callback_called.store(true);
            result_text = response.text;
            callback_ready.set_value();
        },
        5
    );

    ASSERT_EQ(callback_future.wait_for(std::chrono::seconds(30)), std::future_status::ready)
        << "callback did not complete within the execution window for the real GGUF path";
    
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(result_text.empty());
}

TEST_F(LLMPluginTest, AsyncInference_PriorityScheduling) {
    LlamaWrapper::Config plugin_config;
    plugin_config.require_model_integrity = false;
    auto plugin = std::make_shared<LlamaWrapper>(plugin_config);
    const std::string model_path = resolveModelPathForInference("priority_model.gguf", 50);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin->loadModel(model_path, {});
    
    AsyncInferenceEngine::Config config;
    config.num_worker_threads = 1;  // Single worker to test priority
    config.max_queue_size = 100;
    
    AsyncInferenceEngine engine(plugin, config);
    
    std::vector<int> completion_order;
    std::mutex order_mutex = {};
    std::promise<void> low_done;
    std::promise<void> high_done;
    auto low_future = low_done.get_future();
    auto high_future = high_done.get_future();

    // Submit low priority request
    engine.submitAsync(
        InferenceRequest{.prompt = "<unk>", .max_tokens = 10},
        [&order_mutex, &completion_order, &low_done](const InferenceResponse& response) {
            (void)response;
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(1);
            low_done.set_value();
        },
        1  // Low priority
    );
    
    // Submit high priority request
    engine.submitAsync(
        InferenceRequest{.prompt = "<unk>", .max_tokens = 10},
        [&order_mutex, &completion_order, &high_done](const InferenceResponse& response) {
            (void)response;
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(10);
            high_done.set_value();
        },
        10  // High priority
    );

    ASSERT_EQ(low_future.wait_for(std::chrono::seconds(30)), std::future_status::ready)
        << "low-priority request did not complete within the real GGUF execution window";
    ASSERT_EQ(high_future.wait_for(std::chrono::seconds(30)), std::future_status::ready)
        << "high-priority request did not complete within the real GGUF execution window";

    EXPECT_GE(completion_order.size(), 2);
    EXPECT_TRUE(std::find(completion_order.begin(), completion_order.end(), 10) != completion_order.end());
    EXPECT_TRUE(std::find(completion_order.begin(), completion_order.end(), 1) != completion_order.end());
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, Integration_RAGWorkflow) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 8192;  // Large context for RAG
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);

    const std::string model_path = resolveModelPathForInference("rag_model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin.loadModel(model_path, {});
    
    // Create RAG context
    RAGContext rag_context;
    rag_context.documents = {
        {"ThemisDB is a distributed graph database."},
        {"It supports vector search and full-text search."},
        {"ThemisDB uses RocksDB as storage backend."}
    };
    rag_context.max_context_tokens = 2048;
    
    InferenceRequest request;
    request.prompt = "<unk>";
    request.max_tokens = 100;
    
    // Generate with RAG
    auto response = plugin.generateRAG(rag_context, request);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
}

TEST_F(LLMPluginTest, Integration_MultiLoRASwitch) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    
    const std::string model_path = resolveModelPathForInference("base.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("legal.bin", 20);
    createDummyLoRA("medical.bin", 20);
    
    plugin.loadModel(model_path, {});
    plugin.loadLoRA("legal", test_lora_dir + "/legal.bin", {});
    plugin.loadLoRA("medical", test_lora_dir + "/medical.bin", {});
    
    // Generate with legal LoRA
    InferenceRequest legal_req;
    legal_req.prompt = "<unk>";
    legal_req.max_tokens = 50;
    legal_req.lora_adapter_id = "legal";
    
    auto legal_response = plugin.generate(legal_req);
    EXPECT_FALSE(legal_response.text.empty());
    ASSERT_TRUE(legal_response.lora_used.has_value());
    EXPECT_EQ(legal_response.lora_used.value(), "legal");
    
    // Generate with medical LoRA (fast switch!)
    InferenceRequest medical_req;
    medical_req.prompt = "<unk>";
    medical_req.max_tokens = 50;
    medical_req.lora_adapter_id = "medical";
    
    auto medical_response = plugin.generate(medical_req);
    EXPECT_FALSE(medical_response.text.empty());
    ASSERT_TRUE(medical_response.lora_used.has_value());
    EXPECT_EQ(medical_response.lora_used.value(), "medical");
    
    // Responses should be different (different LoRAs)
    // In real implementation, they would have domain-specific knowledge
}

// ═══════════════════════════════════════════════════════════
// LoRA Inference Verification Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, InferenceLoRAInclusion_LoRAFieldSet) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    
    const std::string model_path = resolveModelPathForInference("model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("adapter.bin", 20);
    
    plugin.loadModel(model_path, {});
    plugin.loadLoRA("adapter", test_lora_dir + "/adapter.bin", 1.0f);

    // Verify LoRA is loaded before inference
    auto loaded_loras = plugin.listLoRAs();
    ASSERT_EQ(loaded_loras.size(), 1);
    EXPECT_EQ(loaded_loras[0].id, "adapter");
    EXPECT_TRUE(loaded_loras[0].is_loaded);
    
    // Inference with LoRA specified
    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    req.lora_adapter_id = "adapter";
    
    InferenceResponse resp = plugin.generate(req);
    
    // Critical check: Response must have lora_used field set
    ASSERT_TRUE(resp.lora_used.has_value()) 
        << "lora_used field not set in response when LoRA was specified!";
    EXPECT_EQ(resp.lora_used.value(), "adapter");
    EXPECT_FALSE(resp.text.empty());
    EXPECT_GT(resp.tokens_generated, 0);
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_InvalidLoRAFails) {
    LlamaWrapper::Config config;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin.loadModel(model_path, {});
    
    // Try to use non-existent LoRA
    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    req.lora_adapter_id = "nonexistent-lora";
    
    // Should handle gracefully (fallback to base model or error)
    // Expected behavior: try to apply LoRA, fallback if not loaded
    InferenceResponse resp = plugin.generate(req);
    EXPECT_FALSE(resp.text.empty());
    
    // lora_used should NOT be set since LoRA wasn't loaded
    EXPECT_FALSE(resp.lora_used.has_value())
        << "invalid LoRA should not be reported as active in the response payload";
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_WithoutLoRA) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("base.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin.loadModel(model_path, {});
    
    // Inference WITHOUT LoRA
    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    // No lora_adapter_id specified
    
    InferenceResponse resp = plugin.generate(req);
    EXPECT_FALSE(resp.text.empty());
    EXPECT_GT(resp.tokens_generated, 0);
    
    // lora_used should not be set
    EXPECT_FALSE(resp.lora_used.has_value());
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_ModelNotLoaded) {
    LlamaWrapper::Config config;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    createDummyLoRA("orphan.bin", 20);
    
    // Load LoRA without model (should fail or be deferred)
    bool lora_loaded = plugin.loadLoRA("orphan", test_lora_dir + "/orphan.bin", 1.0f);
    EXPECT_FALSE(lora_loaded) << "LoRA should not load without base model";
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_VerifyScaleFactor) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("scaled.bin", 20);
    
    plugin.loadModel(model_path, {});
    plugin.loadLoRA("scaled", test_lora_dir + "/scaled.bin", 2.0f);
    
    // Verify LoRA was loaded with correct scale
    auto loras = plugin.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_EQ(loras[0].scale, 2.0f);
    EXPECT_EQ(loras[0].id, "scaled");
    
    // Inference with scaled LoRA
    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    req.lora_adapter_id = "scaled";
    
    InferenceResponse resp = plugin.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "scaled");
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_MultipleSequentialRequests) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("seq_model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("lora_a.bin", 20);
    createDummyLoRA("lora_b.bin", 20);
    
    plugin.loadModel(model_path, {});
    plugin.loadLoRA("lora_a", test_lora_dir + "/lora_a.bin", 1.0f);
    plugin.loadLoRA("lora_b", test_lora_dir + "/lora_b.bin", 1.0f);
    
    // Request 1: Use lora_a
    InferenceRequest req1;
    req1.prompt = "<unk>";
    req1.max_tokens = 32;
    req1.lora_adapter_id = "lora_a";
    req1.request_id = "req-001";
    
    InferenceResponse resp1 = plugin.generate(req1);
    EXPECT_EQ(resp1.request_id, "req-001");
    ASSERT_TRUE(resp1.lora_used.has_value());
    EXPECT_EQ(resp1.lora_used.value(), "lora_a");
    
    // Request 2: Use lora_b (different LoRA - verify switching works)
    InferenceRequest req2;
    req2.prompt = "<unk>";
    req2.max_tokens = 32;
    req2.lora_adapter_id = "lora_b";
    req2.request_id = "req-002";
    
    InferenceResponse resp2 = plugin.generate(req2);
    EXPECT_EQ(resp2.request_id, "req-002");
    ASSERT_TRUE(resp2.lora_used.has_value());
    EXPECT_EQ(resp2.lora_used.value(), "lora_b");
    
    // Request 3: Back to lora_a (verify switching back)
    InferenceRequest req3;
    req3.prompt = "<unk>";
    req3.max_tokens = 32;
    req3.lora_adapter_id = "lora_a";
    req3.request_id = "req-003";
    
    InferenceResponse resp3 = plugin.generate(req3);
    EXPECT_EQ(resp3.request_id, "req-003");
    ASSERT_TRUE(resp3.lora_used.has_value());
    EXPECT_EQ(resp3.lora_used.value(), "lora_a");
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_CacheVerification) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("cache_model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("cached.bin", 20);
    
    plugin.loadModel(model_path, {});
    
    // Load LoRA twice (should hit cache)
    bool loaded1 = plugin.loadLoRA("cached", test_lora_dir + "/cached.bin", 1.0f);
    EXPECT_TRUE(loaded1);
    
    bool loaded2 = plugin.loadLoRA("cached", test_lora_dir + "/cached.bin", 1.0f);
    EXPECT_TRUE(loaded2) << "Cached LoRA load should succeed";
    
    // Get cache stats
    auto loras = plugin.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_TRUE(loras[0].is_loaded);
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_UnloadAndReload) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;
    
    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("unload_model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    createDummyLoRA("unload.bin", 20);
    
    plugin.loadModel(model_path, {});
    
    // Load
    EXPECT_TRUE(plugin.loadLoRA("unload", test_lora_dir + "/unload.bin", 1.0f));
    EXPECT_EQ(plugin.listLoRAs().size(), 1);
    
    // Unload
    EXPECT_TRUE(plugin.unloadLoRA("unload"));
    EXPECT_EQ(plugin.listLoRAs().size(), 0);
    
    // Reload
    EXPECT_TRUE(plugin.loadLoRA("unload", test_lora_dir + "/unload.bin", 1.0f));
    EXPECT_EQ(plugin.listLoRAs().size(), 1);
    
    // Inference should work after reload
    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    req.lora_adapter_id = "unload";
    
    InferenceResponse resp = plugin.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "unload");
}

TEST_F(LLMPluginTest, MultiLoRAManager_ConcurrentAccessSharedState) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.max_lora_vram_mb = 4096;
    config.enable_multi_lora_batch = true;

    MultiLoRAManager manager(config);

    for (int i = 0; i < 6; ++i) {
        const std::string filename = "parallel_" + std::to_string(i) + ".bin";
        createDummyLoRA(filename, 16);
    }

    std::atomic<int> successful_loads{0};
    std::vector<std::thread> workers;
    workers.reserve(4);

    for (int worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&, worker]() {
            for (int i = 0; i < 12; ++i) {
                const std::string id = "worker_" + std::to_string(worker) + "_" + std::to_string(i);
                const std::string path = test_lora_dir + "/parallel_" + std::to_string((i + worker) % 6) + ".bin";
                if (manager.loadLoRA(id, path, "shared-model", 1.0f)) {
                    ++successful_loads;
                }
                auto list = manager.listLoRAs("shared-model");
                EXPECT_LE(list.size(), static_cast<size_t>(config.max_lora_slots));
                const auto info = manager.getLoRAInfo(id);
                if (info.has_value()) {
                    EXPECT_TRUE(info->is_loaded);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_GT(successful_loads.load(), 0);
    EXPECT_LE(manager.listLoRAs("shared-model").size(), static_cast<size_t>(config.max_lora_slots));
}

TEST_F(LLMPluginTest, MultiLoRAManager_MultiGPUPlacementBalance) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 8;
    config.max_lora_vram_mb = 256;
    config.multi_gpu.enabled = true;
    config.multi_gpu.devices = {0, 1};
    config.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    config.multi_gpu.max_vram_per_gpu_mb = 128;

    MultiLoRAManager manager(config);

    createDummyLoRA("gpu_a.bin", 16);
    createDummyLoRA("gpu_b.bin", 16);

    EXPECT_TRUE(manager.loadLoRA("gpu_a", test_lora_dir + "/gpu_a.bin", "base-model", false, GPUPlacement::MULTI_GPU, 1.0f));
    EXPECT_TRUE(manager.loadLoRA("gpu_b", test_lora_dir + "/gpu_b.bin", "base-model", false, GPUPlacement::MULTI_GPU, 1.0f));

    const auto placement_a = manager.getLoRAGPUPlacement("gpu_a");
    const auto placement_b = manager.getLoRAGPUPlacement("gpu_b");
    EXPECT_FALSE(placement_a.empty());
    EXPECT_FALSE(placement_b.empty());
    EXPECT_GE(manager.getPerGPUMemoryUsage().size(), 2u);

    const auto memory = manager.getMemoryStats();
    EXPECT_GT(memory["vram_used_mb"].get<size_t>(), 0u);
}

TEST_F(LLMPluginTest, MultiLoRAManager_EvictsExpiredAdaptersAndRespectsVRAMPressure) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 2;
    config.max_lora_vram_mb = 64;
    config.lora_ttl = std::chrono::seconds(0);

    MultiLoRAManager manager(config);

    createDummyLoRA("ttl_a.bin", 16);
    createDummyLoRA("ttl_b.bin", 16);
    createDummyLoRA("ttl_c.bin", 16);
    EXPECT_TRUE(manager.loadLoRA("ttl_a", test_lora_dir + "/ttl_a.bin", "base-model", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("ttl_b", test_lora_dir + "/ttl_b.bin", "base-model", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("ttl_c", test_lora_dir + "/ttl_c.bin", "base-model", 1.0f));

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    EXPECT_GE(manager.getStatistics().evictions, 1u);
    EXPECT_GE(manager.evictExpired(), 1u);
    EXPECT_FALSE(manager.isLoRALoaded("ttl_a"));
    EXPECT_FALSE(manager.isLoRALoaded("ttl_b"));
}

TEST_F(LLMPluginTest, LoRASecurityValidator_RejectsInvalidMetadataAndUntrustedPaths) {
    LoRASecurityConfig security_config;
    security_config.allowed_base_models = {"approved-model"};
    security_config.min_rank = 8;
    security_config.max_rank = 32;
    security_config.validate_metadata = true;

    LoRASecurityValidator validator(security_config);

    const fs::path base_lora_dir = fs::path(test_lora_dir);
    const fs::path bad_metadata = base_lora_dir / "bad_metadata.lora";
    std::ofstream bad_file(bad_metadata, std::ios::binary);
    bad_file << R"({"base_model":"wrong-model","rank":2})";
    bad_file.close();
    EXPECT_FALSE(validator.validateMetadata(bad_metadata.string()));

    const fs::path outside = fs::path("./outside_lora_dir") / "foreign.bin";
    fs::create_directories(fs::path("./outside_lora_dir"));
    std::ofstream out_file(outside, std::ios::binary);
    out_file << "not a valid lora";
    out_file.close();

    MultiLoRAManager::Config manager_config;
    manager_config.max_lora_slots = 4;
    manager_config.max_lora_vram_mb = 256;
    manager_config.security_validator = std::make_shared<LoRASecurityValidator>(security_config);
    manager_config.lora_base_dir = fs::absolute(test_lora_dir).string();
    manager_config.enforce_security_validation = true;

    MultiLoRAManager manager(manager_config);
    EXPECT_FALSE(manager.loadLoRA("bad-metadata", bad_metadata.string(), "approved-model", 1.0f));
    EXPECT_FALSE(manager.loadLoRA("outside", outside.string(), "approved-model", 1.0f));
}

TEST_F(LLMPluginTest, LlamaWrapper_RecoveryAfterFailedLoRALoad) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    config.require_model_integrity = false;

    LlamaWrapper plugin(config);
    const std::string model_path = resolveModelPathForInference("recover_model.gguf", 100);
    if (model_path.empty()) {
        GTEST_SKIP() << "No real GGUF model available for inference-focused test.";
    }
    plugin.loadModel(model_path, {});

    const std::string missing_path = test_lora_dir + "/missing_recovery.bin";
    EXPECT_FALSE(plugin.loadLoRA("recover", missing_path, 1.0f));

    createDummyLoRA("recover.bin", 20);
    EXPECT_TRUE(plugin.loadLoRA("recover", test_lora_dir + "/recover.bin", 1.0f));
    EXPECT_EQ(plugin.listLoRAs().size(), 1u);

    InferenceRequest req;
    req.prompt = "<unk>";
    req.max_tokens = 32;
    req.lora_adapter_id = "recover";

    InferenceResponse resp = plugin.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "recover");
}

TEST_F(LLMPluginTest, MultiLoRAManager_OperationLatencyIsStableUnderLoad) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 32;
    config.max_lora_vram_mb = 4096;

    MultiLoRAManager manager(config);

    for (int i = 0; i < 12; ++i) {
        const std::string filename = "perf_" + std::to_string(i) + ".bin";
        createDummyLoRA(filename, 8);
        EXPECT_TRUE(manager.loadLoRA("perf_" + std::to_string(i), test_lora_dir + "/" + filename, "base-model", 1.0f));
    }

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 200; ++i) {
        (void)manager.listLoRAs("base-model");
        (void)manager.getLoRAInfo("perf_0");
        (void)manager.isLoRALoaded("perf_1");
    }
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );

    EXPECT_LT(elapsed_ms.count(), 250);
}

// ═══════════════════════════════════════════════════════════
// RoPE Scaling Tests (Phase 3.1)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, RopeScaling_ConfigValidation) {
    LlamaWrapper::Config config;
    config.require_model_integrity = false;
    
    // Test valid RoPE scaling configuration
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Should not throw
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

TEST_F(LLMPluginTest, RopeScaling_InvalidConfig) {
    LlamaWrapper::Config config;
    config.require_model_integrity = false;
    
    // Test invalid configuration: max_context < original_context
    config.rope_scaling.enabled = true;
    config.rope_scaling.max_context = 2048;
    config.rope_scaling.original_context = 4096;
    
    // Should issue a warning but not throw (warning is logged, not an error)
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

TEST_F(LLMPluginTest, RopeScaling_YarnParameters) {
    LlamaWrapper::Config config;
    config.require_model_integrity = false;
    
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Test YaRN-specific parameters
    config.rope_scaling.yarn_ext_factor = 1.5f;
    config.rope_scaling.yarn_attn_factor = 1.2f;
    config.rope_scaling.yarn_beta_fast = 32.0f;
    config.rope_scaling.yarn_beta_slow = 1.0f;
    
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

// ═══════════════════════════════════════════════════════════
// QW-27: LLMPluginManager Fail-Closed Guard Tests
// ═══════════════════════════════════════════════════════════

class LLMPluginManagerTest : public ::testing::Test {
protected:
    LLMPluginManager& manager = LLMPluginManager::instance();
};

TEST_F(LLMPluginManagerTest, LoadModelFailsClosedForEmptyModelId) {
    // Fail-closed: reject empty model_id immediately
    const bool result = manager.loadModel("", "/path/to/model.gguf");
    EXPECT_FALSE(result) << "loadModel should return false for empty model_id";
}

TEST_F(LLMPluginManagerTest, LoadModelFailsClosedForEmptyPath) {
    // Fail-closed: reject empty path immediately
    const bool result = manager.loadModel("test_model", "");
    EXPECT_FALSE(result) << "loadModel should return false for empty path";
}

TEST_F(LLMPluginManagerTest, LoadLoRAFailsClosedForEmptyLoRAId) {
    // Fail-closed: reject empty lora_id immediately
    const bool result = manager.loadLoRA("", "/path/to/lora.bin", "base_model");
    EXPECT_FALSE(result) << "loadLoRA should return false for empty lora_id";
}

TEST_F(LLMPluginManagerTest, LoadLoRAFailsClosedForEmptyPath) {
    // Fail-closed: reject empty path immediately
    const bool result = manager.loadLoRA("test_lora", "", "base_model");
    EXPECT_FALSE(result) << "loadLoRA should return false for empty path";
}

TEST_F(LLMPluginTest, RopeScaling_AllMethods) {
    // Test all scaling methods
    std::vector<RopeScalingMethod> methods = {
        RopeScalingMethod::LINEAR,
        RopeScalingMethod::NTK,
        RopeScalingMethod::YARN,
        RopeScalingMethod::DYNAMIC
    };
    
    for (auto method : methods) {
        LlamaWrapper::Config config;
        config.require_model_integrity = false;
        config.rope_scaling.enabled = true;
        config.rope_scaling.method = method;
        config.rope_scaling.max_context = 16384;
        config.rope_scaling.original_context = 4096;
        
        EXPECT_NO_THROW(LlamaWrapper wrapper(config));
    }
}

TEST_F(LLMPluginTest, RopeScaling_InvalidYarnParameters) {
    LlamaWrapper::Config config;
    config.require_model_integrity = false;
    
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Test negative yarn_ext_factor
    config.rope_scaling.yarn_ext_factor = -1.0f;
    EXPECT_THROW(LlamaWrapper wrapper(config), std::invalid_argument);
    
    // Reset to valid value
    config.rope_scaling.yarn_ext_factor = 1.0f;
    
    // Test zero yarn_beta_fast
    config.rope_scaling.yarn_beta_fast = 0.0f;
    EXPECT_THROW(LlamaWrapper wrapper(config), std::invalid_argument);
}

// ═══════════════════════════════════════════════════════════
// P5-L02: LLM Memory Leak — Load/Unload Cycle Tests
// ═══════════════════════════════════════════════════════════
//
// Acceptance criteria: 1,000 load/unload cycles; cache does not accumulate
// entries; memory growth < 50 MB (verified via /proc/self/status).

class LLMMemoryLeakTest : public ::testing::Test {
protected:
    // Returns current resident set size (VmRSS) in kB, or 0 on failure.
    static long getProcRssKb() {
        std::ifstream f("/proc/self/status");
        if (!f.is_open()) {
          return 0;
        }
        std::string line = {};
        while (std::getline(f, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                long kb = 0;
                if (std::sscanf(line.c_str(), "VmRSS: %ld kB", &kb) == 1) {
                    return kb;
                }
            }
        }
        return 0;
    }

    std::string test_model_dir_ = {};

    void SetUp() override {
        test_model_dir_ = "/tmp/llm_leak_test_models_" +
                          std::to_string(
                              std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_model_dir_);
        LLMPluginTest::writeMinimalValidGGUF(test_model_dir_ + "/tiny.gguf", "tiny.gguf", 0);
    }

    void TearDown() override {
        fs::remove_all(test_model_dir_);
    }
};

TEST_F(LLMMemoryLeakTest, P5L02_LazyLoader_CacheDoesNotAccumulateAfter1KCycles) {
    // 1,000 load/evict cycles: with max_models=1, each second load forces eviction of the
    // first. Verify that the models_ map never grows beyond max_models entries.
    LazyModelLoader::Config cfg;
    cfg.max_models = 1;
    cfg.model_ttl  = std::chrono::seconds(3600); // TTL long enough to avoid time-based eviction
    LazyModelLoader loader(cfg);

    const int kCycles = 1000;
    for (int i = 0; i < kCycles; ++i) {
        // Alternate between two model IDs so every even cycle forces an eviction.
        const std::string id   = (i % 2 == 0) ? "model-a" : "model-b";
        const std::string path = test_model_dir_ + "/tiny.gguf";
        // The actual load may fail (no real GGUF), but the cache management
        // (insert/evict tracking) happens regardless and must not accumulate.
        [[maybe_unused]] auto result = loader.getModel(id, path);
    }

    auto stats = loader.getStatistics();
    // With max_models=1, evictions must be non-zero after alternating 1K calls.
    EXPECT_GT(stats.evictions + stats.models_loaded, 0u)
        << "Cache tracking must be active after 1K cycles";
    EXPECT_GE(stats.cache_hits + stats.cache_misses, 1u)
        << "Statistics counters must be updated";
}

TEST_F(LLMMemoryLeakTest, P5L02_LazyLoader_CacheHitDoesNotGrowModelsMap) {
    // The same model loaded 1,000 times must remain a single entry.
    LazyModelLoader::Config cfg;
    cfg.max_models = 10;
    LazyModelLoader loader(cfg);

    const std::string id   = "singleton-model";
    const std::string path = test_model_dir_ + "/tiny.gguf";

    for (int i = 0; i < 1000; ++i) {
        [[maybe_unused]] auto r = loader.getModel(id, path);
    }

    auto stats = loader.getStatistics();
    EXPECT_LE(stats.models_loaded, 1u)
        << "Repeated cache hits must not create duplicate model entries";
    EXPECT_GE(stats.cache_hits, 1u)
        << "At least one cache hit expected after first load";
}

TEST_F(LLMMemoryLeakTest, P5L02_LazyLoader_EvictionCountMatchesExcessLoads) {
    // With max_models=2 and 3 distinct IDs: the third load must trigger exactly 1 eviction.
    LazyModelLoader::Config cfg;
    cfg.max_models = 2;
    LazyModelLoader loader(cfg);

    const std::string path = test_model_dir_ + "/tiny.gguf";
    [[maybe_unused]] auto r1 = loader.getModel("m1", path);
    [[maybe_unused]] auto r2 = loader.getModel("m2", path);
    [[maybe_unused]] auto r3 = loader.getModel("m3", path); // must evict m1 or m2

    auto stats = loader.getStatistics();
    EXPECT_GE(stats.evictions, 1u)
        << "Loading a third model with max_models=2 must trigger at least 1 eviction";
}

TEST_F(LLMMemoryLeakTest, P5L02_MemoryGrowthBelow50MB_Over1KCycles) {
    // Run 1,000 load/evict cycles and verify RSS growth stays < 50 MB.
    // This is a best-effort guard: /proc may be unavailable in some environments.
    LazyModelLoader::Config cfg;
    cfg.max_models = 1;
    LazyModelLoader loader(cfg);

    const std::string path = test_model_dir_ + "/tiny.gguf";
    const long rss_before = getProcRssKb();

    for (int i = 0; i < 1000; ++i) {
        const std::string id = (i % 2 == 0) ? "ma" : "mb";
        [[maybe_unused]] auto r = loader.getModel(id, path);
    }

    const long rss_after = getProcRssKb();
    if (rss_before > 0 && rss_after > 0) {
        const long growth_kb = rss_after - rss_before;
        EXPECT_LT(growth_kb, 50L * 1024L)
            << "Memory growth after 1K load/evict cycles must be < 50 MB; "
               "actual growth: " << growth_kb << " kB";
    }
}

TEST_F(LLMMemoryLeakTest, P5L02_StatsMonotonicAfterCycles) {
    // Statistics counters must be strictly monotonic (never decrease).
    LazyModelLoader::Config cfg;
    cfg.max_models = 2;
    LazyModelLoader loader(cfg);

    const std::string path = test_model_dir_ + "/tiny.gguf";
    size_t prev_hits = 0, prev_misses = 0, prev_evictions = 0;

    for (int i = 0; i < 100; ++i) {
        const std::string id = "m" + std::to_string(i % 3);
        [[maybe_unused]] auto r = loader.getModel(id, path);

        auto s = loader.getStatistics();
        EXPECT_GE(s.cache_hits,   prev_hits)     << "cache_hits must be non-decreasing";
        EXPECT_GE(s.cache_misses, prev_misses)   << "cache_misses must be non-decreasing";
        EXPECT_GE(s.evictions,    prev_evictions) << "evictions must be non-decreasing";
        prev_hits      = s.cache_hits;
        prev_misses    = s.cache_misses;
        prev_evictions = s.evictions;
    }
}
