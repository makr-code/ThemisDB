#include <benchmark/benchmark.h>

#include "ai/ai_plugin_generator.h"
#include "plugins/plugin_interface.h"

#include <string>

using themis::plugins::PluginType;
using themis::plugins::ai::AIPluginGenerator;
using themis::plugins::ai::GeneratedPlugin;
using themis::plugins::ai::PluginGenerationPrompt;
using themis::Result;

namespace {

PluginGenerationPrompt makePrompt() {
    PluginGenerationPrompt prompt;
    prompt.description = "Generate a lightweight importer plugin with metrics support.";
    prompt.type = PluginType::IMPORTER;
    prompt.required_capabilities = {"streaming", "metrics"};
    prompt.dependencies = {"nlohmann_json", "spdlog"};
    prompt.generate_tests = true;
    prompt.generate_docs = true;
    return prompt;
}

AIPluginGenerator::Config makeConfig() {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://benchmark-llm.local/generate";
    cfg.timeout_ms = 50;
    cfg.endpoint_invoke_fn = [](
        const std::string&,
        const std::string&,
        long) -> Result<std::string> {
        return std::string{
            R"({"generated_plugin":{"name":"bench_plugin","implementation_code":"int generated(){return 42;}","header_code":"int generated();","test_code":"TEST(GeneratedPlugin, Smoke) {}","cmake_code":"add_library(bench_plugin SHARED bench.cpp)","build_dependencies":["spdlog"]}})"};
    };
    return cfg;
}

static void BM_AIPluginGeneratorValidatePrompt(benchmark::State& state) {
    const PluginGenerationPrompt prompt = makePrompt();
    AIPluginGenerator generator(makeConfig());

    for (auto _ : state) {
        auto result = generator.validatePrompt(prompt);
        benchmark::DoNotOptimize(result.has_value());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AIPluginGeneratorValidatePrompt);

static void BM_AIPluginGeneratorGeneratePlugin(benchmark::State& state) {
    const PluginGenerationPrompt prompt = makePrompt();
    AIPluginGenerator generator(makeConfig());

    for (auto _ : state) {
        auto result = generator.generatePlugin(prompt);
        benchmark::DoNotOptimize(result.has_value());
        if (result) {
            benchmark::DoNotOptimize(result->implementation_code.size());
            benchmark::DoNotOptimize(result->build_dependencies.size());
        }
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AIPluginGeneratorGeneratePlugin);

static void BM_AIPluginGeneratorErrorPathMalformedJson(benchmark::State& state) {
    AIPluginGenerator::Config cfg = makeConfig();
    cfg.endpoint_invoke_fn = [](
        const std::string&,
        const std::string&,
        long) -> Result<std::string> {
        return std::string{"{\"generated_plugin\":{\"implementation_code\":"};
    };
    AIPluginGenerator generator(cfg);
    const PluginGenerationPrompt prompt = makePrompt();

    for (auto _ : state) {
        auto result = generator.generatePlugin(prompt);
        benchmark::DoNotOptimize(result.has_value());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AIPluginGeneratorErrorPathMalformedJson);

}  // namespace
