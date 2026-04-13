/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_tokenizer.h                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     122                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "data_loader.h"
#include <string>
#include <vector>
#include <memory>

// Forward declaration for llama.cpp type
struct llama_model;

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Tokenizer backed by llama.cpp native tokenization
 * 
 * Integrates llama.cpp's tokenizer to ensure training/inference consistency.
 * Loads model in vocab-only mode (lightweight - only tokenizer, no weights).
 * 
 * Features:
 * - Native subword tokenization (BPE/SentencePiece)
 * - BOS/EOS token support
 * - Compatible with llama.cpp inference
 * - Supports multiple model types (Llama, Mistral, CodeLlama, etc.)
 */
class LlamaTokenizer : public ITokenizer {
public:
    /**
     * @brief Construct LlamaTokenizer from model file
     * @param model_path Path to GGUF model file
     * @throws std::runtime_error if model cannot be loaded
     */
    explicit LlamaTokenizer(const std::string& model_path);
    
    /**
     * @brief Destructor - cleanup llama.cpp resources
     */
    ~LlamaTokenizer() override;
    
    // Disable copy (llama.cpp resources are non-copyable)
    LlamaTokenizer(const LlamaTokenizer&) = delete;
    LlamaTokenizer& operator=(const LlamaTokenizer&) = delete;
    
    // Enable move
    LlamaTokenizer(LlamaTokenizer&& other) noexcept;
    LlamaTokenizer& operator=(LlamaTokenizer&& other) noexcept;
    
    /**
     * @brief Encode text to token IDs using llama.cpp tokenizer
     * @param text Input text
     * @param add_bos Add beginning-of-sequence token
     * @param add_eos Add end-of-sequence token
     * @return Vector of token IDs
     */
    std::vector<int> encode(const std::string& text, 
                           bool add_bos = true, 
                           bool add_eos = false) override;
    
    /**
     * @brief Decode token IDs to text using llama.cpp
     * @param tokens Token IDs
     * @return Decoded text
     */
    std::string decode(const std::vector<int>& tokens) override;
    
    /**
     * @brief Get vocabulary size
     * @return Number of tokens in vocabulary
     */
    int vocab_size() const override;
    
    /**
     * @brief Get BOS (beginning of sequence) token ID
     * @return BOS token ID
     */
    int bos_token_id() const override;
    
    /**
     * @brief Get EOS (end of sequence) token ID
     * @return EOS token ID
     */
    int eos_token_id() const override;
    
    /**
     * @brief Get PAD (padding) token ID
     * @return PAD token ID (typically same as EOS for llama models)
     */
    int pad_token_id() const override;
    
private:
    llama_model* model_ = nullptr;
    std::string model_path_;
    
    void cleanup();
};

} // namespace lora
} // namespace llm
} // namespace themis
