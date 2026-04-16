/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_compressor.h                                ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// Prompt compression / summarization for context reduction
#include <string>
#include <vector>

namespace themis { namespace prompt_engineering {

enum class CompressionStrategy {
    TRUNCATE_HEAD,
    TRUNCATE_TAIL,
    SELECTIVE_TRIM,
    SUMMARY,
    EMBEDDING_PRUNE,
};

struct PromptCompressionConfig {
    CompressionStrategy strategy = CompressionStrategy::SELECTIVE_TRIM;
    int target_token_budget = 2048;
    float max_compression_ratio = 0.5f;
    bool preserve_system_prompt = true;
    bool preserve_last_n_turns = 3;
    std::string summary_model_id;
};

struct CompressionResult {
    std::string compressed_prompt;
    int original_token_count = 0;
    int compressed_token_count = 0;
    float compression_ratio = 0.0f;
    CompressionStrategy strategy_used{CompressionStrategy::SELECTIVE_TRIM};
    double compression_ms = 0.0;
};

class IPromptCompressor {
public:
    virtual ~IPromptCompressor() = default;
    virtual CompressionResult compress(
        const std::string& prompt,
        const PromptCompressionConfig& config) = 0;
    virtual int estimateTokenCount(const std::string& text) = 0;
    virtual std::vector<CompressionStrategy> supportedStrategies() const = 0;
};

}} // namespace themis::prompt_engineering
