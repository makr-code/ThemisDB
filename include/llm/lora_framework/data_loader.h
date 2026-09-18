/**
 * @file data_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct TrainingBatch {
    /**
     * @brief Training Batch.
     * @return Return value.
     */
    virtual ~TrainingBatch() = default;
    std::vector<std::vector<int>> input_ids;      // [batch_size, seq_len]
    std::vector<std::vector<int>> label_ids;      // [batch_size, seq_len]
    std::vector<size_t> sequence_lengths;         // Actual lengths before padding
    int batch_size = 0;
    int max_sequence_length = 0;
    
    bool empty() const { return input_ids.empty(); }
    size_t size() const { return input_ids.size(); }
};

enum class DatasetFormat {
    JSONL,          // One JSON object per line
    ALPACA,         // Stanford Alpaca format
    SHAREGPT,       // ShareGPT conversation format
    PLAIN_TEXT,     // Plain text files (for language modeling)
    CUSTOM          // Custom format with user-provided parser
};

class ITokenizer {
public:
    /**
     * @brief ITokenizer.
     * @return Return value.
     */
    virtual ~ITokenizer() = default;
    
    [[nodiscard]] virtual std::vector<int> encode(const std::string& text, 
                                    bool add_bos = true, 
                                    bool add_eos = false) = 0;
    
    [[nodiscard]] virtual std::string decode(const std::vector<int>& tokens) = 0;
    
    [[nodiscard]] virtual int vocab_size() const = 0;
    
    [[nodiscard]] virtual int bos_token_id() const = 0;
    
    [[nodiscard]] virtual int eos_token_id() const = 0;
    
    [[nodiscard]] virtual int pad_token_id() const = 0;
};

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
    /**
     * @brief Simple character-level tokenization for testing
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<int> char_to_token(const std::string& text);
    /**
     * @brief Token to char.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
    std::string token_to_char(const std::vector<int>& tokens);
};

struct DataLoaderConfig {
    /**
     * @brief Data Loader Config.
     * @return Return value.
     */
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

class DataLoader {
public:
    explicit DataLoader(std::shared_ptr<ITokenizer> tokenizer,
                       const DataLoaderConfig& config = DataLoaderConfig{});
    ~DataLoader();
    
    /**
     * @brief Load From File.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromFile(const std::string& filepath);
    
    /**
     * @brief Load From JSON.
     * @param[in] json_data Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromJSON(const std::string& json_data);
    
    /**
     * @brief Load From Samples.
     * @param[in] samples Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromSamples(const std::vector<InstructionDataSample>& samples);
    
    size_t size() const { return samples_.size(); }
    
    /**
     * @brief Num batches.
     * @return Return value.
     */
    size_t num_batches() const;
    
    /**
     * @brief Get Next Batch.
     * @return Return value.
     */
    TrainingBatch getNextBatch();
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    /**
     * @brief Has Next.
     * @return True when the operation succeeds.
     */
    bool hasNext() const;
    
    /**
     * @brief Shuffle.
     */
    void shuffle();
    
    /**
     * @brief Get Sample.
     * @param[in] idx Input parameter.
     * @return Return value.
     */
    std::optional<InstructionDataSample> getSample(size_t idx) const;
    
    void setFormatter(std::function<std::string(const InstructionDataSample&)> formatter) {
        custom_formatter_ = formatter;
    }
    
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
    /**
     * @brief Parse JSONL.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseJSONL(const std::string& filepath);
    /**
     * @brief Parse Alpaca.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseAlpaca(const std::string& filepath);
    /**
     * @brief Parse Share GPT.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseShareGPT(const std::string& filepath);
    /**
     * @brief Parse Plain Text.
     * @param[in] filepath Input parameter.
     * @return True when the operation succeeds.
     */
    bool parsePlainText(const std::string& filepath);
    
    /**
     * @brief Format Sample.
     * @param[in] sample Input parameter.
     * @return Return value.
     */
    std::string formatSample(const InstructionDataSample& sample) const;
    /**
     * @brief Tokenize Sample.
     * @param[in,out] sample Input/output parameter.
     */
    void tokenizeSample(InstructionDataSample& sample);
    /**
     * @brief Create Batch.
     * @param[in] batch_indices Input parameter.
     * @return Return value.
     */
    TrainingBatch createBatch(const std::vector<size_t>& batch_indices);
    /**
     * @brief Pad Batch.
     * @param[in,out] batch Input/output parameter.
     */
    void padBatch(TrainingBatch& batch);
};

namespace data_utils {
    /**
     * @brief Load Alpaca Format.
     * @param[in] json_data Input parameter.
     * @return Return value.
     */
    std::vector<InstructionDataSample> loadAlpacaFormat(const std::string& json_data);
    
    /**
     * @brief Load Share GPTFormat.
     * @param[in] json_data Input parameter.
     * @return Return value.
     */
    std::vector<InstructionDataSample> loadShareGPTFormat(const std::string& json_data);
    
    /**
     * @brief Create Toy Dataset.
     * @param[in] num_samples Input parameter.
     * @return Return value.
     */
    std::vector<InstructionDataSample> createToyDataset(size_t num_samples);
    
    std::pair<std::vector<InstructionDataSample>, std::vector<InstructionDataSample>>
    trainValSplit(const std::vector<InstructionDataSample>& samples, float validation_split = 0.1f);
} // namespace data_utils

} // namespace lora
} // namespace llm
} // namespace themis

