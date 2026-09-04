#include <gtest/gtest.h>

#include "stable_diffusion/sd_plugin.h"
#include "utils/checksum_utils.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <thread>
#include <vector>

using namespace themis::imggen;
using json = nlohmann::json;

namespace {

[[nodiscard]] const char* requireEnv(const char* key) {
    const char* value = std::getenv(key);
    if (!value || value[0] == '\0') {
        GTEST_SKIP() << "Set " << key << " to run stable_diffusion real-backend E2E tests";
    }
    return value;
}

[[nodiscard]] bool hasPngSignature(const std::vector<uint8_t>& png) {
    static constexpr uint8_t kSig[8] = {0x89u, 'P', 'N', 'G', '\r', '\n', 0x1Au, '\n'};
    return static_cast<int>(png.size()) >= 8 && std::memcmp(png.data(), kSig, 8) == 0;
}

} // namespace

TEST(SDPluginRealBackendE2ETests, E2E_Text2ImgAndImg2Img_WithModelSha256Gate) {
    const char* model_path = requireEnv("THEMIS_SD_E2E_MODEL_PATH");
    ASSERT_TRUE(std::filesystem::exists(model_path))
        << "THEMIS_SD_E2E_MODEL_PATH does not exist: " << model_path;

    const std::string model_sha256 = themis::utils::calculateSHA256(model_path);
    ASSERT_FALSE(model_sha256.empty()) << "failed to calculate model SHA-256";

    SDPlugin plugin;
    json init_cfg = {
        {"model_sha256", model_sha256},
        {"width", 64},
        {"height", 64},
        {"steps", 2},
        {"cfg_scale", 5.0f},
        {"sampler", "euler_a"}
    };
    ASSERT_TRUE(plugin.initialize(model_path, init_cfg)) << "real-backend init failed";

    SDGenerationConfig t2i_cfg;
    t2i_cfg.width = 64;
    t2i_cfg.height = 64;
    t2i_cfg.steps = 2;
    t2i_cfg.cfg_scale = 5.0f;
    t2i_cfg.sampler = "euler_a";
    t2i_cfg.seed = 42;

    const auto t2i = plugin.generate("a minimal geometric icon", t2i_cfg);
    ASSERT_TRUE(t2i.success) << t2i.error_message;
    ASSERT_GT(t2i.width, 0);
    ASSERT_GT(t2i.height, 0);
    ASSERT_TRUE(hasPngSignature(t2i.png_data));
    ASSERT_FALSE(t2i.prompt_hash.empty());
    ASSERT_EQ(t2i.plugin_version, "2.3.0");

    Img2ImgConfig i2i_cfg;
    i2i_cfg.input_width = 64;
    i2i_cfg.input_height = 64;
    i2i_cfg.input_image_rgb.assign(static_cast<size_t>(64 * 64 * 3), 120);
    i2i_cfg.width = 64;
    i2i_cfg.height = 64;
    i2i_cfg.steps = 2;
    i2i_cfg.cfg_scale = 5.0f;
    i2i_cfg.sampler = "euler_a";
    i2i_cfg.seed = 1337;
    i2i_cfg.strength = 0.5f;

    const auto i2i = plugin.generateImg2Img("refine edges and improve contrast", i2i_cfg);
    ASSERT_TRUE(i2i.success) << i2i.error_message;
    ASSERT_GT(i2i.width, 0);
    ASSERT_GT(i2i.height, 0);
    ASSERT_TRUE(hasPngSignature(i2i.png_data));
    ASSERT_FALSE(i2i.prompt_hash.empty());
}

TEST(SDPluginRealBackendE2ETests, ParallelAudit_RealBackendGenerate) {
    const char* model_path = requireEnv("THEMIS_SD_E2E_MODEL_PATH");
    ASSERT_TRUE(std::filesystem::exists(model_path))
        << "THEMIS_SD_E2E_MODEL_PATH does not exist: " << model_path;

    const std::string model_sha256 = themis::utils::calculateSHA256(model_path);
    ASSERT_FALSE(model_sha256.empty()) << "failed to calculate model SHA-256";

    SDPlugin plugin;
    json init_cfg = {
        {"model_sha256", model_sha256},
        {"width", 32},
        {"height", 32},
        {"steps", 1},
        {"cfg_scale", 4.5f},
        {"sampler", "euler_a"}
    };
    ASSERT_TRUE(plugin.initialize(model_path, init_cfg)) << "real-backend init failed";

    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    workers.reserve(4);

    for (int worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&plugin, &failures, worker]() {
            for (int i = 0; i < 2; ++i) {
                try {
                    SDGenerationConfig cfg;
                    cfg.width = 32;
                    cfg.height = 32;
                    cfg.steps = 1;
                    cfg.cfg_scale = 4.5f;
                    cfg.sampler = "euler_a";
                    cfg.seed = worker * 100 + i;

                    const auto result = plugin.generate("parallel audit prompt", cfg);
                    if (!result.success || result.png_data.empty() || result.width <= 0 || result.height <= 0) {
                        ++failures;
                    }
                } catch (...) {
                    ++failures;
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(failures.load(), 0) << "parallel real-backend generate audit failures";
}
