/**
 * @file data_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_config.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Training data sample with instruction-response format
 */
struct InstructionDataSample {
    std::string instruction;     // User instruction/prompt
    std::string input;           // Optional additional context
    std::string output;          // Expected response
    
    // Tokenized versions (populated by tokenizer)
    std::vector<int> input_ids;
    std::vector<int> label_ids;
    
    // Get full prompt (instruction + input if present)
    std::string getFullPrompt() const {
        if (input.empty()) {
            return instruction;
        }
        return instruction + "\n" + input;
    }
};

/**
 * @brief Batch of training samples
 */
struct TrainingBatch {
    virtual ~TrainingBatch() = default;
    std::vector<std::vector<int>> input_ids;      // [batch_size, seq_len]
    std::vector<std::vector<int>> label_ids;      // [batch_size, seq_len]
    std::vector<size_t> sequence_lengths;         // Actual lengths before padding
    int batch_size = 0;
    int max_sequence_length = 0;
    
    bool empty() const { return input_ids.empty(); }
    size_t size() const { return input_ids.size(); }
};

/**
 * @brief Dataset format types
 */
enum class DatasetFormat {
    JSONL,          // One JSON object per line
    ALPACA,         // Stanford Alpaca format
    SHAREGPT,       // ShareGPT conversation format
    PLAIN_TEXT,     // Plain text files (for language modeling)
    CUSTOM          // Custom format with user-provided parser
};

/**
 * @brief Tokenizer interface (abstract - can be backed by llama.cpp or other tokenizers)
 */
class ITokenizer {
public:
    virtual ~ITokenizer() = default;
    
    /**
     * @brief Encode text to token IDs
     * @param text Input text
     * @param add_bos Add beginning-of-sequence token
     * @param add_eos Add end-of-sequence token
     * @return Vector of token IDs
     */
    [[nodiscard]] virtual std::vector<int> encode(const std::string& text, 
                                    bool add_bos = true, 
                                    bool add_eos = false) = 0;
    
    /**
     * @brief Decode token IDs to text
     * @param tokens Token IDs
     * @return Decoded text
     */
    [[nodiscard]] virtual std::string decode(const std::vector<int>& tokens) = 0;
    
    /**
     * @brief Get vocabulary size
     * @return Number of tokens in vocabulary
     */
    [[nodiscard]] virtual int vocab_size() const = 0;
    
    /**
     * @brief Get BOS (beginning of sequence) token ID
     * @return BOS token ID
     */
    [[nodiscard]] virtual int bos_token_id() const = 0;
    
    /**
     * @brief Get EOS (end of sequence) token ID
     * @return EOS token ID
     */
    [[nodiscard]] virtual int eos_token_id() const = 0;
    
    /**
     * @brief Get PAD (padding) token ID
     * @return PAD token ID
     */
    [[nodiscard]] virtual int pad_token_id() const = 0;
};

/**
 * @brief Simple tokenizer implementation (for testing)
 */
class SimpleTokenizer : public ITokenizer {
public:
    SimpleTokenizer(int vocab_size = 32000);
    ~SimpleTokenizer() override = default;
    
    std::vector<int> encode(const std::string& text, 
                           bool add_bos = true, 
                           bool add_eos = false) override;
    
    std::string decode(const std::vector<int>& tokens) override;
    
    int vocab_size() const override { return vocab_size_; }
    int bos_token_id() const override { return 1; }
    int eos_token_id() const override { return 2; }
    int pad_token_id() const override { return 0; }
    
private:
    int vocab_size_ = 0;
    // Simple character-level tokenization for testing
    std::vector<int> char_to_token(const std::string& text);
    std::string token_to_char(const std::vector<int>& tokens);
};

/**
 * @brief Data loader configuration
 */
struct DataLoaderConfig {
    virtual ~DataLoaderConfig() = default;
    DatasetFormat format = DatasetFormat::JSONL;
    int max_sequence_length = 2048;
    int batch_size = 1;
    bool shuffle = true;
    int num_workers = 1;
    
    // Prompt formatting
    std::string instruction_prefix = "### Instruction:\n";
    std::string input_prefix = "\n### Input:\n";
    std::string response_prefix = "\n### Response:\n";
    
    // Padding and truncation
    bool pad_to_max_length = true;
    bool truncate_to_max_length = true;
    int pad_token_id = 0;
    
    // Data augmentation (optional)
    bool enable_augmentation = false;
    float noise_probability = 0.0f;
};

/**
 * @brief Data loader for LoRA training
 * 
 * Loads and preprocesses text data for fine-tuning:
 * - Supports multiple dataset formats
 * - Tokenization via pluggable tokenizer interface
 * - Batching and padding
 * - Prompt formatting
 */
class DataLoader {
public:
    explicit DataLoader(std::shared_ptr<ITokenizer> tokenizer,
                       const DataLoaderConfig& config = DataLoaderConfig{});
    ~DataLoader();
    
    /**
     * @brief Load dataset from file
     * @param filepath Path to dataset file
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& filepath);
    
    /**
     * @brief Load dataset from JSON string
     * @param json_data JSON string containing dataset
     * @return true if loaded successfully
     */
    bool loadFromJSON(const std::string& json_data);
    
    /**
     * @brief Load dataset from vector of samples
     * @param samples Vector of instruction samples
     * @return true if loaded successfully
     */
    bool loadFromSamples(const std::vector<InstructionDataSample>& samples);
    
    /**
     * @brief Get total number of samples
     * @return Sample count
     */
    size_t size() const { return samples_.size(); }
    
    /**
     * @brief Get number of batches
     * @return Batch count
     */
    size_t num_batches() const;
    
    /**
     * @brief Get next batch
     * @return Training batch (empty if no more batches)
     */
    TrainingBatch getNextBatch();
    
    /**
     * @brief Reset iterator to beginning
     */
    void reset();
    
    /**
     * @brief Check if more batches available
     * @return true if has next batch
     */
    bool hasNext() const;
    
    /**
     * @brief Shuffle dataset
     */
    void shuffle();
    
    /**
     * @brief Get sample at index
     * @param idx Sample index
     * @return Sample (empty optional if out of bounds)
     */
    std::optional<InstructionDataSample> getSample(size_t idx) const;
    
    /**
     * @brief Set custom sample formatter
     * @param formatter Function to format sample text
     */
    void setFormatter(std::function<std::string(const InstructionDataSample&)> formatter) {
        custom_formatter_ = formatter;
    }
    
    /**
     * @brief Get configuration
     * @return Current configuration
     */
    const DataLoaderConfig& getConfig() const { return config_; }
    
private:
    std::shared_ptr<ITokenizer> tokenizer_;
    DataLoaderConfig config_;
    
    std::vector<InstructionDataSample> samples_;
    std::vector<size_t> indices_;  // For shuffling
    size_t current_index_ = 0;
    
    // Custom formatter (optional)
    std::function<std::string(const InstructionDataSample&)> custom_formatter_;
    
    // Helper methods
    bool parseJSONL(const std::string& filepath);
    bool parseAlpaca(const std::string& filepath);
    bool parseShareGPT(const std::string& filepath);
    bool parsePlainText(const std::string& filepath);
    
    std::string formatSample(const InstructionDataSample& sample) const;
    void tokenizeSample(InstructionDataSample& sample);
    TrainingBatch createBatch(const std::vector<size_t>& batch_indices);
    void padBatch(TrainingBatch& batch);
};

/**
 * @brief Helper functions for data loading
 */
namespace data_utils {
    /**
     * @brief Load samples from Alpaca JSON format
     * @param json_data JSON string
     * @return Vector of samples
     */
    std::vector<InstructionDataSample> loadAlpacaFormat(const std::string& json_data);
    
    /**
     * @brief Load samples from ShareGPT JSON format
     * @param json_data JSON string
     * @return Vector of samples
     */
    std::vector<InstructionDataSample> loadShareGPTFormat(const std::string& json_data);
    
    /**
     * @brief Create synthetic toy dataset for testing
     * @param num_samples Number of samples to generate
     * @return Vector of samples
     */
    std::vector<InstructionDataSample> createToyDataset(size_t num_samples);
    
    /**
     * @brief Split dataset into train/validation sets
     * @param samples Full dataset
     * @param validation_split Fraction for validation (0.0-1.0)
     * @return Pair of (train_samples, val_samples)
     */
    std::pair<std::vector<InstructionDataSample>, std::vector<InstructionDataSample>>
    trainValSplit(const std::vector<InstructionDataSample>& samples, float validation_split = 0.1f);
} // namespace data_utils

} // namespace lora
} // namespace llm
} // namespace themis

