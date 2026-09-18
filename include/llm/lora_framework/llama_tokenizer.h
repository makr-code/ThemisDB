/**
 * @file llama_tokenizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class LlamaTokenizer : public ITokenizer {
public:
    /**
     * @brief Llama Tokenizer.
     * @param[in] model_path Path to the model.
     * @return Return value.
     */
    explicit LlamaTokenizer(const std::string& model_path);
    
    ~LlamaTokenizer() noexcept override;
    
    // Disable copy (llama.cpp resources are non-copyable)
    LlamaTokenizer(const LlamaTokenizer&) = delete;
    LlamaTokenizer& operator=(const LlamaTokenizer&) = delete;
    
    // Enable move
    LlamaTokenizer(LlamaTokenizer&& other) noexcept;
    LlamaTokenizer& operator=(LlamaTokenizer&& other) noexcept;
    
    std::vector<int> encode(const std::string& text, 
                           bool add_bos = true, 
                           bool add_eos = false) override;
    
    std::string decode(const std::vector<int>& tokens) override;
    
    int vocab_size() const override;
    
    int bos_token_id() const override;
    
    int eos_token_id() const override;
    
    int pad_token_id() const override;
    
private:
    llama_model* model_ = nullptr;
    std::string model_path_;
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
};

} // namespace lora
} // namespace llm
} // namespace themis

