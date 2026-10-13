#include <gtest/gtest.h>

#include "llama_cpp/llama_cpp_plugin.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace themis::llamacpp;
using namespace themis::llm;

namespace {

std::unique_ptr<LlamaCppPlugin> make_loaded_plugin() {
    auto plugin = std::make_unique<LlamaCppPlugin>();
    EXPECT_TRUE(plugin->loadModel("/stub/model.gguf", {}));
    return plugin;
}

}  // namespace

TEST(LlamaCppErrorHandlingTests, E01_RequestCancelled_ReturnsError) {
    auto plugin = make_loaded_plugin();

    InferenceRequest req;
    req.prompt = "cancel me";
    req.cancellation_token = std::make_shared<std::atomic<bool>>(true);

    auto resp = plugin->generate(req);
    EXPECT_FALSE(resp.success);
    EXPECT_EQ(resp.error_message, "Request cancelled");
}

TEST(LlamaCppErrorHandlingTests, E02_PolicyGateRejectsRequest) {
    auto plugin = make_loaded_plugin();
    plugin->setPolicyFn([](const InferenceRequest&, std::string& reason) {
        reason = "policy denied";
        return false;
    });

    InferenceRequest req;
    req.prompt = "forbidden";
    auto resp = plugin->generate(req);

    EXPECT_FALSE(resp.success);
    EXPECT_NE(resp.error_message.find("Request denied by policy"), std::string::npos);
}

TEST(LlamaCppErrorHandlingTests, E03_ImportLoRA_InvalidMagicReturnsFalse) {
    LlamaCppPlugin plugin;
    std::vector<uint8_t> bad = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    EXPECT_FALSE(plugin.importLoRA("bad", bad));
}

TEST(LlamaCppErrorHandlingTests, E04_ImportLoRA_ValidMagicButStubbedReturnsTrue) {
    LlamaCppPlugin plugin;
    std::vector<uint8_t> valid = {'G', 'G', 'U', 'F', 0x00, 0x00, 0x00, 0x00};

    EXPECT_TRUE(plugin.importLoRA("ok", valid));
}

TEST(LlamaCppErrorHandlingTests, E05_ConcurrentGenerationDoesNotCrash) {
    auto plugin = make_loaded_plugin();

    std::atomic<int> successful{0};
    std::vector<std::thread> threads;
    threads.reserve(4);

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&plugin, &successful]() {
            InferenceRequest req;
            req.prompt = "concurrent";
            req.max_tokens = 16;
            auto resp = plugin->generate(req);
            if (resp.success || !resp.error_message.empty()) {
                ++successful;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successful.load(), 4);
}
