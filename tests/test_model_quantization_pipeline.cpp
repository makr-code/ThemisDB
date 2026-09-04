#include <gtest/gtest.h>
#include "llm/model_quantization_pipeline.h"
#include "llm/lora_framework/quantization.h"
#include "llm/lora_framework/quantized_model.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>

namespace fs = std::filesystem;

using namespace themis::llm;
using namespace themis::llm::lora;

// ---------------------------------------------------------------------------
// Helpers for building synthetic SafeTensors files on the fly
// ---------------------------------------------------------------------------

namespace {

// Convert float → FP16 (uint16_t)
uint16_t fp32_to_fp16(float v)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &v, 4);
    const uint16_t sign = (bits >> 16) & 0x8000;
    const int exp  = static_cast<int>((bits >> 23) & 0xFF) - 127 + 15;
    const uint32_t mant = (bits >> 13) & 0x3FF;
    if (exp <= 0) {
      return sign;
    }
    if (exp >= 31) {
      return static_cast<uint16_t>(sign | 0x7C00);
    }
    return static_cast<uint16_t>(sign | (static_cast<uint16_t>(exp) << 10) | mant);
}

// Helper: make a vector of n FP16 values all equal to v
std::vector<uint16_t> make_fp16_scales(size_t n, float v)
{
    return std::vector<uint16_t>(n, fp32_to_fp16(v));
}

// Build a minimal SafeTensors binary blob (no __metadata__).
// tensors: list of (name, dtype, shape, raw_data)
std::vector<uint8_t> make_safetensors(
    const std::vector<std::tuple<std::string,
                                 std::string,
                                 std::vector<int64_t>,
                                 std::vector<uint8_t>>>& tensors)
{
    // Build data region and header simultaneously
    std::string hdr_json = "{";
    std::vector<uint8_t> data;

    uint64_t offset = 0;
    for (size_t ti = 0; ti < tensors.size(); ++ti) {
        const auto& [name, dtype, shape, raw] = tensors[ti];

        uint64_t begin = offset;
        uint64_t end   = offset + raw.size();
        data.insert(data.end(), raw.begin(), raw.end());
        offset = end;

        if (ti > 0) {
          hdr_json += ",";
        }
        hdr_json += "\"" + name + "\":{\"dtype\":\"" + dtype + "\",\"shape\":[";
        for (size_t si = 0; si < shape.size(); ++si) {
            if (si) {
              hdr_json += ",";
            }
            hdr_json += std::to_string(shape[si]);
        }
        hdr_json += "],\"data_offsets\":[" +
                    std::to_string(begin) + "," +
                    std::to_string(end) + "]}";
    }
    hdr_json += "}";

    // Assemble: [uint64 header_len][header bytes][data bytes]
    std::vector<uint8_t> blob;
    const uint64_t hlen = hdr_json.size();
    blob.resize(8 + hlen + data.size());
    std::memcpy(blob.data(), &hlen, 8);
    std::memcpy(blob.data() + 8, hdr_json.data(), hlen);
    std::memcpy(blob.data() + 8 + hlen, data.data(), data.size());
    return blob;
}

// Write a file from a vector of bytes
void write_file(const std::string& path, const std::vector<uint8_t>& data)
{
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
}

// Write a minimal config.json for AWQ or GPTQ
void write_config(const std::string& dir,
                  const std::string& quant_type,
                  int bits, int group_size)
{
    std::ofstream f(fs::path(dir) / "config.json");
    f << "{\"quantization_config\":{\"quant_type\":\"" << quant_type
      << "\",\"w_bit\":" << bits
      << ",\"bits\":" << bits
      << ",\"group_size\":" << group_size << "}}";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Tests: format detection
// ---------------------------------------------------------------------------

class ModelQuantizationPipelineTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_mqp_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(ModelQuantizationPipelineTest, DetectGGUF_FileExtension)
{
    const std::string p = (tmp_dir_ / "model.gguf").string();
    EXPECT_EQ(ModelQuantizationPipeline::detect_format(p), ModelFormat::GGUF);
}

TEST_F(ModelQuantizationPipelineTest, DetectAWQ_FromConfigJson)
{
    const std::string dir = (tmp_dir_ / "awq_model").string();
    fs::create_directories(dir);
    write_config(dir, "awq", 4, 128);

    EXPECT_EQ(ModelQuantizationPipeline::detect_format(dir), ModelFormat::AWQ);
}

TEST_F(ModelQuantizationPipelineTest, DetectGPTQ_FromConfigJson)
{
    const std::string dir = (tmp_dir_ / "gptq_model").string();
    fs::create_directories(dir);
    write_config(dir, "gptq", 4, 128);

    EXPECT_EQ(ModelQuantizationPipeline::detect_format(dir), ModelFormat::GPTQ);
}

TEST_F(ModelQuantizationPipelineTest, FormatName)
{
    EXPECT_STREQ(ModelQuantizationPipeline::format_name(ModelFormat::GGUF), "GGUF");
    EXPECT_STREQ(ModelQuantizationPipeline::format_name(ModelFormat::AWQ),  "AWQ");
    EXPECT_STREQ(ModelQuantizationPipeline::format_name(ModelFormat::GPTQ), "GPTQ");
}

// ---------------------------------------------------------------------------
// Tests: GPTQ SafeTensors loading (synthetic model)
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, LoadGPTQ_SyntheticLayer)
{
    // Build a tiny synthetic GPTQ model:
    //   in_features=8, out_features=4, group_size=8, bits=4
    //
    //   qweight shape: [8/8, 4] = [1, 4]  → one INT32 per column (8 4-bit weights)
    //   qzeros  shape: [1, 4/8] = [1, 1]  → one INT32 (zeros for col 0..3 packed)
    //   scales  shape: [1, 4]             → four FP16 scales

    const int bits       = 4;
    const int vpw        = 32 / bits;   // 8
    const int in_f       = 8;
    const int out_f      = 4;
    const int group_size = 8;

    // qweight: [in_f/vpw, out_f] = [1, 4]
    // Each column holds 8 4-bit weights packed into one INT32
    // Col 0: weights [2,2,2,2,2,2,2,2], Col 1: [3,...], Col 2: [1,...], Col 3: [4,...]
    auto make_qw_col = [&](int val) -> uint32_t {
        uint32_t w = 0;
        for (int b = 0; b < vpw; ++b) {
          w |= (static_cast<uint32_t>(val & 0xF) << (b * 4));
        }
        return w;
    };
    std::vector<uint32_t> qweight_raw = {
        make_qw_col(2), make_qw_col(3), make_qw_col(1), make_qw_col(4)
    };

    // qzeros: [n_groups=1, out_f/vpw=1] → one INT32 holding zeros for all 4 cols
    // zero[col] = 1 for all cols (packed into one INT32, cols 0..3 in bits 0..15)
    uint32_t qzeros_raw = 0;
    for (int c = 0; c < out_f; ++c)
        qzeros_raw |= (1u << (c * bits));
    std::vector<uint32_t> qzeros_vec = { qzeros_raw };

    // scales: [n_groups=1, out_f=4] → four FP16 values = 0.5f each
    auto scales_raw = make_fp16_scales(static_cast<size_t>(out_f), 0.5f);

    // Build raw byte vectors
    auto to_bytes = [](const auto& v) -> std::vector<uint8_t> {
        std::vector<uint8_t> b(v.size() * sizeof(v[0]));
        std::memcpy(b.data(), v.data(), b.size());
        return b;
    };

    std::vector<uint8_t> qw_bytes = to_bytes(qweight_raw);
    std::vector<uint8_t> qz_bytes = to_bytes(qzeros_vec);
    std::vector<uint8_t> sc_bytes = to_bytes(scales_raw);

    // Create SafeTensors file
    const std::string model_dir = (tmp_dir_ / "gptq_synthetic").string();
    fs::create_directories(model_dir);
    write_config(model_dir, "gptq", bits, group_size);

    auto blob = make_safetensors({
        {"model.layers.0.attn.qweight",
            "I32", {in_f / vpw, out_f}, qw_bytes},
        {"model.layers.0.attn.qzeros",
            "I32", {1, out_f / vpw}, qz_bytes},
        {"model.layers.0.attn.scales",
            "F16", {1, out_f},       sc_bytes},
    });
    write_file((fs::path(model_dir) / "model.safetensors").string(), blob);

    QuantizationPipelineConfig cfg;
    cfg.bits       = bits;
    cfg.group_size = group_size;
    cfg.max_tensors = 1;

    auto model = ModelQuantizationPipeline::load(
        model_dir, ModelFormat::GPTQ, cfg);

    EXPECT_EQ(model.num_layers(), 1u);
    EXPECT_NE(model.get_layer("model.layers.0.attn"), nullptr);

    // Dequantize and verify approximate values
    // Expected: weight[i][c] = (q[c] - zero) * scale = (q[c] - 1) * 0.5
    //   col 0: (2-1)*0.5 = 0.5, col 1: (3-1)*0.5 = 1.0,
    //   col 2: (1-1)*0.5 = 0.0, col 3: (4-1)*0.5 = 1.5
    auto deq = model.dequantize_layer("model.layers.0.attn");
    ASSERT_EQ(deq.data().size(), static_cast<size_t>(in_f * out_f));

    // Allow quantization round-trip tolerance
    for (int i = 0; i < in_f; ++i) {
        EXPECT_NEAR(deq.data()[static_cast<size_t>(i * out_f + 0)], 0.5f,  0.15f);
        EXPECT_NEAR(deq.data()[static_cast<size_t>(i * out_f + 1)], 1.0f,  0.15f);
        EXPECT_NEAR(deq.data()[static_cast<size_t>(i * out_f + 2)], 0.0f,  0.15f);
        EXPECT_NEAR(deq.data()[static_cast<size_t>(i * out_f + 3)], 1.5f,  0.15f);
    }
}

// ---------------------------------------------------------------------------
// Tests: AWQ SafeTensors loading (synthetic model)
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, LoadAWQ_SyntheticLayer)
{
    // AWQ layout (AutoAWQ format):
    //   weight shape: [in_f, out_f/vpw]  (packs along OUT dimension)
    //   zeros  shape: [n_groups, out_f/vpw]
    //   scales shape: [n_groups, out_f]  FP16
    //
    // weight[i, j] = (qweight[i, j//vpw] >> ((j%vpw)*bits)) & mask
    // zero[g, j]   = (qzeros[g,  j//vpw] >> ((j%vpw)*bits)) & mask
    // fp = (w - z) * scale

    const int bits       = 4;
    const int vpw        = 32 / bits;   // 8
    const int in_f       = 8;
    const int out_f      = 4;
    const int group_size = 8;
    // Out features packed into ceil(4/8)=1 INT32 per row
    const int out_packed = (out_f + vpw - 1) / vpw;  // 1

    // Build qweight: [in_f=8, out_packed=1]
    // Each row packs 4 output features (bits 0..15) with value 5
    // qweight[i, 0] = 5 | (5<<4) | (5<<8) | (5<<12) = 0x5555
    const uint32_t w_word = (5u) | (5u << 4) | (5u << 8) | (5u << 12);
    std::vector<uint32_t> weight_raw(static_cast<size_t>(in_f * out_packed), w_word);

    // Build qzeros: [n_groups=1, out_packed=1]
    // zero = 3 for all cols: qzeros[0, 0] = 3 | (3<<4) | (3<<8) | (3<<12)
    const uint32_t z_word = (3u) | (3u << 4) | (3u << 8) | (3u << 12);
    std::vector<uint32_t> zeros_raw = { z_word };

    // scales: [n_groups=1, out_f=4] = 0.25f
    auto scales_raw = make_fp16_scales(static_cast<size_t>(out_f), 0.25f);

    auto to_bytes = [](const auto& v) -> std::vector<uint8_t> {
        std::vector<uint8_t> b(v.size() * sizeof(v[0]));
        std::memcpy(b.data(), v.data(), b.size());
        return b;
    };

    const std::string model_dir = (tmp_dir_ / "awq_synthetic").string();
    fs::create_directories(model_dir);
    write_config(model_dir, "awq", bits, group_size);

    auto blob = make_safetensors({
        {"model.layers.0.mlp.weight",
            "I32", {in_f, out_packed},       to_bytes(weight_raw)},
        {"model.layers.0.mlp.zeros",
            "I32", {1, out_packed},           to_bytes(zeros_raw)},
        {"model.layers.0.mlp.scales",
            "F16", {1, out_f},               to_bytes(scales_raw)},
    });
    write_file((fs::path(model_dir) / "model.safetensors").string(), blob);

    QuantizationPipelineConfig cfg;
    cfg.bits       = bits;
    cfg.group_size = group_size;
    cfg.max_tensors = 1;

    auto model = ModelQuantizationPipeline::load(
        model_dir, ModelFormat::AWQ, cfg);

    EXPECT_EQ(model.num_layers(), 1u);
    EXPECT_NE(model.get_layer("model.layers.0.mlp"), nullptr);

    // Expected: (5 - 3) * 0.25 = 0.5 for all entries
    auto deq = model.dequantize_layer("model.layers.0.mlp");
    ASSERT_EQ(deq.data().size(), static_cast<size_t>(in_f * out_f));
    for (float v : deq.data()) {
        EXPECT_NEAR(v, 0.5f, 0.15f);
    }
}

// ---------------------------------------------------------------------------
// Tests: Missing directory / empty directory
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, LoadGPTQ_MissingDirectory_Throws)
{
    EXPECT_THROW(
        ModelQuantizationPipeline::load("/nonexistent/path", ModelFormat::GPTQ),
        std::runtime_error);
}

TEST_F(ModelQuantizationPipelineTest, LoadAWQ_NoShards_Throws)
{
    const std::string dir = (tmp_dir_ / "awq_empty").string();
    fs::create_directories(dir);
    write_config(dir, "awq", 4, 128);

    EXPECT_THROW(
        ModelQuantizationPipeline::load(dir, ModelFormat::AWQ),
        std::runtime_error);
}

TEST_F(ModelQuantizationPipelineTest, LoadGPTQ_NoShards_Throws)
{
    const std::string dir = (tmp_dir_ / "gptq_empty").string();
    fs::create_directories(dir);
    write_config(dir, "gptq", 4, 128);

    EXPECT_THROW(
        ModelQuantizationPipeline::load(dir, ModelFormat::GPTQ),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// Tests: Auto-detection via shard header heuristic
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, DetectGPTQ_FromShardHeader)
{
    const std::string dir = (tmp_dir_ / "gptq_heuristic").string();
    fs::create_directories(dir);
    // No config.json → rely on header scan

    // Build a minimal safetensors with a "qweight" tensor
    const std::vector<uint8_t> dummy_data(4, 0);
    auto blob = make_safetensors({
        {"model.layer.0.qweight", "I32", {1, 1}, dummy_data},
    });
    write_file((fs::path(dir) / "model.safetensors").string(), blob);

    EXPECT_EQ(ModelQuantizationPipeline::detect_format(dir), ModelFormat::GPTQ);
}

// ---------------------------------------------------------------------------
// Tests: Security – malicious SafeTensors file (out-of-bounds offsets)
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, ParseSafetensors_OOBOffsets_Throws)
{
    // Craft a safetensors file whose data_offsets claim a range exceeding
    // the actual data region → parse_safetensors must reject it.
    const std::string dir = (tmp_dir_ / "oob_shard").string();
    fs::create_directories(dir);
    // No config.json needed – the malicious safetensors shard is rejected
    // during parsing, before any config is read.

    // Manually build a blob with a malicious offset
    // Header claims data_end = 999999999, but actual data is only 4 bytes.
    const std::string hdr =
        R"({"tensor":{"dtype":"I32","shape":[1,1],"data_offsets":[0,999999999]}})";
    const uint64_t hlen = hdr.size();
    const std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};  // 4 bytes

    std::vector<uint8_t> blob(8 + hlen + payload.size());
    std::memcpy(blob.data(), &hlen, 8);
    std::memcpy(blob.data() + 8, hdr.data(), hlen);
    std::memcpy(blob.data() + 8 + hlen, payload.data(), payload.size());

    write_file((fs::path(dir) / "model.safetensors").string(), blob);

    EXPECT_THROW(
        ModelQuantizationPipeline::load(dir, ModelFormat::GPTQ),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// Tests: Security – invalid bits / group_size guard
// ---------------------------------------------------------------------------

TEST_F(ModelQuantizationPipelineTest, LoadGPTQ_InvalidBits_Throws)
{
    // A config.json with bits=0 must be rejected before any division.
    const std::string dir = (tmp_dir_ / "gptq_bad_bits").string();
    fs::create_directories(dir);

    {
        std::ofstream f(fs::path(dir) / "config.json");
        f << R"({"quantization_config":{"bits":0,"group_size":128}})";
    }

    // Build a minimal valid-looking shard (will be rejected at dequant time).
    const std::vector<uint8_t> dummy(4, 0);
    auto blob = make_safetensors({
        {"layer.qweight", "I32", {1, 1}, dummy},
        {"layer.qzeros",  "I32", {1, 1}, dummy},
        {"layer.scales",  "F16", {1, 1}, dummy},
    });
    write_file((fs::path(dir) / "model.safetensors").string(), blob);

    QuantizationPipelineConfig cfg;
    cfg.bits = 0;
    EXPECT_THROW(
        ModelQuantizationPipeline::load(dir, ModelFormat::GPTQ, cfg),
        std::runtime_error);
}

TEST_F(ModelQuantizationPipelineTest, LoadGPTQ_InvalidGroupSize_Throws)
{
    // A config.json with group_size=0 must be rejected before any division.
    const std::string dir = (tmp_dir_ / "gptq_bad_gs").string();
    fs::create_directories(dir);

    {
        std::ofstream f(fs::path(dir) / "config.json");
        f << R"({"quantization_config":{"bits":4,"group_size":0}})";
    }

    const std::vector<uint8_t> dummy(4, 0);
    auto blob = make_safetensors({
        {"layer.qweight", "I32", {1, 1}, dummy},
        {"layer.qzeros",  "I32", {1, 1}, dummy},
        {"layer.scales",  "F16", {1, 1}, dummy},
    });
    write_file((fs::path(dir) / "model.safetensors").string(), blob);

    QuantizationPipelineConfig cfg;
    cfg.bits       = 4;
    cfg.group_size = 0;
    EXPECT_THROW(
        ModelQuantizationPipeline::load(dir, ModelFormat::GPTQ, cfg),
        std::runtime_error);
}
