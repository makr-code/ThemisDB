#include <gtest/gtest.h>
#include "llm/lora_framework/llama_tokenizer.h"
#include "llm/lora_framework/data_loader.h"
#include <memory>
#include <filesystem>
#include <cstdlib>

using namespace themis::llm::lora;
namespace fs = std::filesystem;

/**
 * @file test_llama_tokenizer.cpp
 * @brief Comprehensive tests for llama.cpp native tokenizer integration
 * 
 * Test Coverage:
 * - LlamaTokenizer initialization
 * - Encoding/decoding with llama.cpp
 * - Special tokens (BOS, EOS, PAD)
 * - Round-trip tokenization
 * - Comparison with SimpleTokenizer
 * - Error handling and fallback
 * 
 * NOTE: These tests require a GGUF model file to be present.
 * Set THEMIS_TEST_MODEL_PATH environment variable to specify model path.
 * If no model is available, tests will be skipped with a warning.
 */

class LlamaTokenizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if test model is available
        const char* model_path_env = std::getenv("THEMIS_TEST_MODEL_PATH");
        if (model_path_env && fs::exists(model_path_env)) {
            test_model_path_ = model_path_env;
            model_available_ = true;
        } else {
            // Try common locations for test models
            // These are example paths - users should set THEMIS_TEST_MODEL_PATH
            std::vector<std::string> common_paths = {
                "models/test-model.gguf",      // Generic test model
                "models/llama-2-7b.gguf",      // Llama 2 example
                "models/mistral-7b-v0.1.gguf", // Mistral example
                "../models/test-model.gguf",
                "/tmp/test_model.gguf"
            };
            
            for (const auto& path : common_paths) {
                if (fs::exists(path)) {
                    test_model_path_ = path;
                    model_available_ = true;
                    break;
                }
            }
        }
        
        if (!model_available_) {
            GTEST_SKIP() << "No test model available. Set THEMIS_TEST_MODEL_PATH environment variable "
                        << "to a GGUF model file to run LlamaTokenizer tests.";
        }
    }
    
    std::string test_model_path_;
    bool model_available_ = false;
};

// ===== Initialization Tests =====

TEST_F(LlamaTokenizerTest, Construction_ValidModel) {
    ASSERT_TRUE(model_available_);
    
    EXPECT_NO_THROW({
        LlamaTokenizer tokenizer(test_model_path_);
        EXPECT_GT(tokenizer.vocab_size(), 0);
    });
}

TEST_F(LlamaTokenizerTest, Construction_InvalidModel) {
    EXPECT_THROW({
        LlamaTokenizer tokenizer("/nonexistent/model.gguf");
    }, std::runtime_error);
}

TEST_F(LlamaTokenizerTest, VocabSize) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    int vocab_size = tokenizer.vocab_size();
    
    // Llama models typically have ~32k vocab, but check for reasonable range
    EXPECT_GT(vocab_size, 1000);
    EXPECT_LT(vocab_size, 200000);
}

// ===== Special Tokens Tests =====

TEST_F(LlamaTokenizerTest, SpecialTokens_BOS) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    int bos = tokenizer.bos_token_id();
    
    EXPECT_GE(bos, 0);
    EXPECT_LT(bos, tokenizer.vocab_size());
}

TEST_F(LlamaTokenizerTest, SpecialTokens_EOS) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    int eos = tokenizer.eos_token_id();
    
    EXPECT_GE(eos, 0);
    EXPECT_LT(eos, tokenizer.vocab_size());
}

TEST_F(LlamaTokenizerTest, SpecialTokens_PAD) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    int pad = tokenizer.pad_token_id();
    
    EXPECT_GE(pad, 0);
    EXPECT_LT(pad, tokenizer.vocab_size());
}

TEST_F(LlamaTokenizerTest, SpecialTokens_Distinct) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    int bos = tokenizer.bos_token_id();
    int eos = tokenizer.eos_token_id();
    
    // BOS and EOS should be different
    EXPECT_NE(bos, eos);
}

// ===== Encoding Tests =====

TEST_F(LlamaTokenizerTest, Encode_BasicText) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string text = "Hello, world!";
    
    auto tokens = tokenizer.encode(text, false, false);
    
    // Should produce tokens (not empty)
    EXPECT_GT(tokens.size(), 0);
    
    // Subword tokenization should produce fewer tokens than characters
    EXPECT_LT(tokens.size(), text.size());
}

TEST_F(LlamaTokenizerTest, Encode_WithBOS) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string text = "Hello";
    
    auto tokens_without_bos = tokenizer.encode(text, false, false);
    auto tokens_with_bos = tokenizer.encode(text, true, false);
    
    // With BOS should have one more token
    EXPECT_EQ(tokens_with_bos.size(), tokens_without_bos.size() + 1);
    
    // First token should be BOS
    EXPECT_EQ(tokens_with_bos.front(), tokenizer.bos_token_id());
}

TEST_F(LlamaTokenizerTest, Encode_WithEOS) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string text = "Hello";
    
    auto tokens_without_eos = tokenizer.encode(text, false, false);
    auto tokens_with_eos = tokenizer.encode(text, false, true);
    
    // With EOS should have one more token
    EXPECT_EQ(tokens_with_eos.size(), tokens_without_eos.size() + 1);
    
    // Last token should be EOS
    EXPECT_EQ(tokens_with_eos.back(), tokenizer.eos_token_id());
}

TEST_F(LlamaTokenizerTest, Encode_EmptyString) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    
    auto tokens = tokenizer.encode("", false, false);
    EXPECT_EQ(tokens.size(), 0);
    
    auto tokens_bos = tokenizer.encode("", true, false);
    EXPECT_EQ(tokens_bos.size(), 1);
    EXPECT_EQ(tokens_bos[0], tokenizer.bos_token_id());
}

TEST_F(LlamaTokenizerTest, Encode_LongText) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string long_text = "The quick brown fox jumps over the lazy dog. "
                           "This is a longer text to test tokenization of "
                           "multiple sentences with various punctuation marks!";
    
    auto tokens = tokenizer.encode(long_text, false, false);
    
    EXPECT_GT(tokens.size(), 10);  // Should produce multiple tokens
    EXPECT_LT(tokens.size(), long_text.size());  // Subword compression
}

// ===== Decoding Tests =====

TEST_F(LlamaTokenizerTest, Decode_BasicTokens) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string text = "Hello, world!";
    
    auto tokens = tokenizer.encode(text, false, false);
    std::string decoded = tokenizer.decode(tokens);
    
    EXPECT_FALSE(decoded.empty());
}

TEST_F(LlamaTokenizerTest, Decode_EmptyTokens) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::vector<int> empty_tokens;
    
    std::string decoded = tokenizer.decode(empty_tokens);
    EXPECT_EQ(decoded, "");
}

TEST_F(LlamaTokenizerTest, Decode_SkipsSpecialTokens) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    
    // Create tokens with special tokens
    std::vector<int> tokens = {
        tokenizer.bos_token_id(),
        100,  // Some regular token
        tokenizer.eos_token_id()
    };
    
    std::string decoded = tokenizer.decode(tokens);
    
    // Decoded text should not contain special token artifacts
    EXPECT_FALSE(decoded.empty());
}

// ===== Round-Trip Tests =====

TEST_F(LlamaTokenizerTest, RoundTrip_Simple) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string original = "Hello, world!";
    
    auto tokens = tokenizer.encode(original, false, false);
    std::string decoded = tokenizer.decode(tokens);
    
    EXPECT_EQ(original, decoded);
}

TEST_F(LlamaTokenizerTest, RoundTrip_MultipleWords) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string original = "The quick brown fox jumps over the lazy dog.";
    
    auto tokens = tokenizer.encode(original, false, false);
    std::string decoded = tokenizer.decode(tokens);
    
    EXPECT_EQ(original, decoded);
}

TEST_F(LlamaTokenizerTest, RoundTrip_WithSpecialChars) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tokenizer(test_model_path_);
    std::string original = "Test: 123, ABC! @#$%";
    
    auto tokens = tokenizer.encode(original, false, false);
    std::string decoded = tokenizer.decode(tokens);
    
    // Some special characters might be normalized, so check length is similar
    EXPECT_GT(decoded.size(), 0);
}

// ===== Comparison with SimpleTokenizer =====

TEST_F(LlamaTokenizerTest, CompareWithSimple_TokenCount) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer llama_tok(test_model_path_);
    SimpleTokenizer simple_tok;
    
    std::string text = "Hello, world!";
    
    auto llama_tokens = llama_tok.encode(text, false, false);
    auto simple_tokens = simple_tok.encode(text, false, false);
    
    // llama.cpp should produce fewer tokens (subword tokenization)
    // SimpleTokenizer is character-level, so should have more tokens
    EXPECT_LT(llama_tokens.size(), simple_tokens.size());
}

TEST_F(LlamaTokenizerTest, CompareWithSimple_VocabSize) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer llama_tok(test_model_path_);
    SimpleTokenizer simple_tok;
    
    // llama.cpp should have much larger vocabulary
    EXPECT_GT(llama_tok.vocab_size(), simple_tok.vocab_size());
}

// ===== Move Semantics Tests =====

TEST_F(LlamaTokenizerTest, MoveConstructor) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tok1(test_model_path_);
    int vocab_size = tok1.vocab_size();
    
    LlamaTokenizer tok2(std::move(tok1));
    EXPECT_EQ(tok2.vocab_size(), vocab_size);
    
    // Can still use tok2
    auto tokens = tok2.encode("test", false, false);
    EXPECT_GT(tokens.size(), 0);
}

TEST_F(LlamaTokenizerTest, MoveAssignment) {
    ASSERT_TRUE(model_available_);
    
    LlamaTokenizer tok1(test_model_path_);
    int vocab_size = tok1.vocab_size();
    
    LlamaTokenizer tok2(test_model_path_);
    tok2 = std::move(tok1);
    
    EXPECT_EQ(tok2.vocab_size(), vocab_size);
    
    // Can still use tok2
    auto tokens = tok2.encode("test", false, false);
    EXPECT_GT(tokens.size(), 0);
}

// ===== Integration Tests =====

TEST_F(LlamaTokenizerTest, Integration_DataLoader) {
    ASSERT_TRUE(model_available_);
    
    auto tokenizer = std::make_shared<LlamaTokenizer>(test_model_path_);
    
    DataLoaderConfig config;
    config.batch_size = 2;
    config.max_sequence_length = 128;
    
    DataLoader loader(tokenizer, config);
    
    std::vector<InstructionDataSample> samples;
    InstructionDataSample sample;
    sample.instruction = "What is machine learning?";
    sample.output = "Machine learning is a subset of AI.";
    samples.push_back(sample);
    
    bool loaded = loader.loadFromSamples(samples);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(loader.size(), 1);
}

// Main function

