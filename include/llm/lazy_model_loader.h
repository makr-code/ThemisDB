#pragma once

#include <string>
#include <chrono>

namespace themis {
namespace llm {

struct LazyModelLoaderConfig {
    size_t max_models = 1;
    size_t max_vram_mb = 0;
    std::chrono::seconds model_ttl{0};
};

class LazyModelLoader {
public:
    using Config = LazyModelLoaderConfig;

    explicit LazyModelLoader(const Config& = {}) {}
    bool loadModel(const std::string&, const std::string&) { return true; }
    void unloadModel(const std::string&) {}
};

} // namespace llm
} // namespace themis
