/**
 * @file data_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=9, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/data_loader.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cctype>

using json = nlohmann::json;

namespace themis {
namespace llm {
namespace lora {

// ===== SimpleTokenizer Implementation =====

SimpleTokenizer::SimpleTokenizer([[maybe_unused]] int vocab_size) 
    : vocab_size_(vocab_size) {
}

std::vector<int> SimpleTokenizer::encode(const std::string& text, 
                                         bool add_bos, 
                                         bool add_eos) {
    std::vector<int> tokens;
    
    if (add_bos) {
        tokens.push_back(bos_token_id());
    }
    
    // Simple character-level tokenization for testing
    // Each character maps to token ID based on ASCII value + offset
    for (char c : text) {
        int token_id = static_cast<int>(static_cast<unsigned char>(c)) + 10;
        if (token_id < vocab_size_) {
            tokens.push_back(token_id);
        }
    }
    
    if (add_eos) {
        tokens.push_back(eos_token_id());
    }
    
    return tokens;
}

std::string SimpleTokenizer::decode(const std::vector<int>& tokens) {
    std::string text = {};
    
    for (int token_id : tokens) {
        // Skip special tokens
        if (token_id == bos_token_id() || 
            token_id == eos_token_id() || 
            token_id == pad_token_id()) {
            continue;
        }
        
        // Convert token back to character
        if (token_id >= 10 && token_id < vocab_size_) {
            char c = static_cast<char>(token_id - 10);
            text += c;
        }
    }
    
    return text;
}

// ===== DataLoader Implementation =====

DataLoader::DataLoader(std::shared_ptr<ITokenizer> tokenizer,
                       const DataLoaderConfig& config)
    : tokenizer_(tokenizer)
    , config_(config)
    , current_index_(0) {
}

DataLoader::~DataLoader() {
}

bool DataLoader::loadFromFile(const std::string& filepath) {
    spdlog::info("Loading dataset from: {}", filepath);
    
    bool success = false;
    
    switch (config_.format) {
        case DatasetFormat::JSONL:
            success = parseJSONL(filepath);
            break;
        case DatasetFormat::ALPACA:
            success = parseAlpaca(filepath);
            break;
        case DatasetFormat::SHAREGPT:
            success = parseShareGPT(filepath);
            break;
        case DatasetFormat::PLAIN_TEXT:
            success = parsePlainText(filepath);
            break;
        default:
            spdlog::error("Unsupported dataset format");
            return false;
    }
    
    if (!success) {
        return false;
    }
    
    // Tokenize all samples
    for (auto& sample : samples_) {
        tokenizeSample(sample);
    }
    
    // Initialize indices for iteration
    indices_.resize(samples_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    
    if (config_.shuffle) {
        shuffle();
    }
    
    spdlog::info("Loaded {} samples", samples_.size());
    return true;
}

bool DataLoader::loadFromJSON(const std::string& json_data) {
    try {
        json j = json::parse(json_data);
        
        if (j.is_array()) {
            // Array of samples
            for (const auto& item : j) {
                InstructionDataSample sample;
                sample.instruction = item.value("instruction", "");
                sample.input = item.value("input", "");
                sample.output = item.value("output", "");
                
                samples_.push_back(sample);
            }
        } else if (j.is_object()) {
            // Single sample
            InstructionDataSample sample;
            sample.instruction = j.value("instruction", "");
            sample.input = j.value("input", "");
            sample.output = j.value("output", "");
            
            samples_.push_back(sample);
        }
        
        // Tokenize all samples
        for (auto& sample : samples_) {
            tokenizeSample(sample);
        }
        
        // Initialize indices
        indices_.resize(samples_.size());
        std::iota(indices_.begin(), indices_.end(), 0);
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse JSON: {}", e.what());
        return false;
    }
}

bool DataLoader::loadFromSamples(const std::vector<InstructionDataSample>& samples) {
    samples_ = samples;
    
    // Tokenize all samples
    for (auto& sample : samples_) {
        tokenizeSample(sample);
    }
    
    // Initialize indices
    indices_.resize(samples_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    
    if (config_.shuffle) {
        shuffle();
    }
    
    spdlog::info("Loaded {} samples from memory", samples_.size());
    return true;
}

bool DataLoader::parseJSONL(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filepath);
        return false;
    }
    
    std::string line = {};
    int line_num = 0;
    
    while (std::getline(file, line)) {
        line_num++;
        
        if (line.empty()) {
            continue;
        }
        
        try {
            json j = json::parse(line);
            
            InstructionDataSample sample;
            sample.instruction = j.value("instruction", "");
            sample.input = j.value("input", "");
            sample.output = j.value("output", "");
            
            if (!sample.instruction.empty() && !sample.output.empty()) {
                samples_.push_back(sample);
            }
            
        } catch (const std::exception& e) {
            spdlog::warn("Failed to parse line {}: {}", line_num, e.what());
        }
    }
    
    return !samples_.empty();
}

bool DataLoader::parseAlpaca(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filepath);
        return false;
    }
    
    try {
        json j;
        file >> j;
        
        samples_ = data_utils::loadAlpacaFormat(j.dump());
        
        return !samples_.empty();
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse Alpaca format: {}", e.what());
        return false;
    }
}

bool DataLoader::parseShareGPT(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filepath);
        return false;
    }
    
    try {
        json j;
        file >> j;
        
        samples_ = data_utils::loadShareGPTFormat(j.dump());
        
        return !samples_.empty();
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse ShareGPT format: {}", e.what());
        return false;
    }
}

bool DataLoader::parsePlainText(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filepath);
        return false;
    }
    
    // Read entire file as single text
    std::stringstream buffer = {};
    buffer << file.rdbuf();
    std::string text = buffer.str();
    
    // Split into chunks for language modeling
    // Each chunk becomes an instruction-output pair
    const size_t chunk_size = 512;  // characters per chunk
    
    for (size_t i = 0; i < text.size(); i += chunk_size) {
        size_t end = std::min(i + chunk_size, text.size());
        std::string chunk = text.substr(i, end - i);
        
        InstructionDataSample sample;
        sample.instruction = "Continue the text:";
        sample.input = "";
        sample.output = chunk;
        
        samples_.push_back(sample);
    }
    
    return !samples_.empty();
}

std::string DataLoader::formatSample(const InstructionDataSample& sample) const {
    if (custom_formatter_) {
        return custom_formatter_(sample);
    }
    
    // Default Alpaca-style formatting
    std::string formatted = config_.instruction_prefix + sample.instruction;
    
    if (!sample.input.empty()) {
        formatted += config_.input_prefix + sample.input;
    }
    
    formatted += config_.response_prefix + sample.output;
    
    return formatted;
}

void DataLoader::tokenizeSample(InstructionDataSample& sample) {
    // Format sample text
    std::string formatted_text = formatSample(sample);
    
    // Encode to token IDs
    sample.input_ids = tokenizer_->encode(formatted_text, true, false);
    
    // For causal language modeling, labels are the same as inputs shifted by 1
    // (predict next token at each position)
    sample.label_ids = sample.input_ids;
    
    // Truncate if needed
    if (config_.truncate_to_max_length && 
        sample.input_ids.size() > static_cast<size_t>(config_.max_sequence_length)) {
        sample.input_ids.resize(config_.max_sequence_length);
        sample.label_ids.resize(config_.max_sequence_length);
    }
}

size_t DataLoader::num_batches() const {
    return (samples_.size() + config_.batch_size - 1) / config_.batch_size;
}

TrainingBatch DataLoader::getNextBatch() {
    if (!hasNext()) {
        return TrainingBatch{};  // Empty batch
    }
    
    // Collect indices for this batch
    std::vector<size_t> batch_indices = {};

    size_t end_index = std::min(current_index_ + config_.batch_size, indices_.size());
    
    for (size_t i = current_index_; i < end_index; ++i) {
        batch_indices.push_back(indices_[i]);
    }
    
    current_index_ = end_index;
    
    return createBatch(batch_indices);
}

TrainingBatch DataLoader::createBatch(const std::vector<size_t>& batch_indices) {
    TrainingBatch batch;
    batch.batch_size = static_cast<int>(batch_indices.size());
    batch.max_sequence_length = 0;
    
    // Collect samples and find max length
    for (size_t idx : batch_indices) {
        const auto& sample = samples_[idx];
        batch.input_ids.push_back(sample.input_ids);
        batch.label_ids.push_back(sample.label_ids);
        batch.sequence_lengths.push_back(sample.input_ids.size());
        
        batch.max_sequence_length = std::max(
            batch.max_sequence_length, 
            static_cast<int>(sample.input_ids.size())
        );
    }
    
    // Pad sequences if needed
    if (config_.pad_to_max_length) {
        padBatch(batch);
    }
    
    return batch;
}

void DataLoader::padBatch(TrainingBatch& batch) {
    int target_length = config_.pad_to_max_length ? 
        config_.max_sequence_length : batch.max_sequence_length;
    
    for (size_t i = 0; i < batch.input_ids.size(); ++i) {
        auto& input_seq = batch.input_ids[i];
        auto& label_seq = batch.label_ids[i];
        
        // Pad to target length
        while (input_seq.size() < static_cast<size_t>(target_length)) {
            input_seq.push_back(config_.pad_token_id);
            label_seq.push_back(-100);  // -100 is ignored in loss calculation
        }
    }
    
    batch.max_sequence_length = target_length;
}

void DataLoader::reset() {
    current_index_ = 0;
    
    if (config_.shuffle) {
        shuffle();
    }
}

bool DataLoader::hasNext() const {
    return static_cast<bool>(current_index_  < static_cast<int>(indices_.size()));
}

void DataLoader::shuffle() {
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::shuffle(indices_.begin(), indices_.end(), gen);
    
    spdlog::debug("Dataset shuffled");
}

std::optional<InstructionDataSample> DataLoader::getSample([[maybe_unused]] size_t idx) const {
    if (idx >= static_cast<int>(samples_.size())) {
        return std::nullopt;
    }
    return samples_[idx];
}

// ===== Data Utilities Implementation =====

namespace data_utils {

std::vector<InstructionDataSample> loadAlpacaFormat(const std::string& json_data) {
    std::vector<InstructionDataSample> samples;
    
    try {
        json j = json::parse(json_data);
        
        if (!j.is_array()) {
            spdlog::error("Alpaca format must be a JSON array");
            return samples;
        }
        
        for (const auto& item : j) {
            InstructionDataSample sample;
            sample.instruction = item.value("instruction", "");
            sample.input = item.value("input", "");
            sample.output = item.value("output", "");
            
            if (!sample.instruction.empty() && !sample.output.empty()) {
                samples.push_back(sample);
            }
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse Alpaca format: {}", e.what());
    }
    
    return samples;
}

std::vector<InstructionDataSample> loadShareGPTFormat(const std::string& json_data) {
    std::vector<InstructionDataSample> samples;
    
    try {
        json j = json::parse(json_data);
        
        if (!j.is_array()) {
            spdlog::error("ShareGPT format must be a JSON array");
            return samples;
        }
        
        for (const auto& conversation : j) {
            if (!conversation.contains("conversations")) {
                continue;
            }
            
            const auto& messages = conversation["conversations"];
            
            // Convert conversations to instruction-response pairs
            std::string instruction = {};
            std::string response = {};
            
            for (const auto& msg : messages) {
                std::string role = msg.value("from", "");
                std::string content = msg.value("value", "");
                
                if (role == "human" || role == "user") {
                    instruction = content;
                } else if (role == "gpt" || role == "assistant") {
                    response = content;
                    
                    // Create sample when we have both
                    if (!instruction.empty() && !response.empty()) {
                        InstructionDataSample sample;
                        sample.instruction = instruction;
                        sample.input = "";
                        sample.output = response;
                        samples.push_back(sample);
                        
                        instruction.clear();
                        response.clear();
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse ShareGPT format: {}", e.what());
    }
    
    return samples;
}

std::vector<InstructionDataSample> createToyDataset([[maybe_unused]] size_t num_samples) {
    std::vector<InstructionDataSample> samples;
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    for (size_t i = 0; i < num_samples; ++i) {
        InstructionDataSample sample = {};
        
        if (i % 5 < 2) {
            // Arithmetic
            int a = dis(gen);
            int b = dis(gen);
            
            if (i % 5 == 0) {
                sample.instruction = "What is " + std::to_string(a) + " + " + std::to_string(b) + "?";
                sample.output = "The answer is " + std::to_string(a + b) + ".";
            } else {
                sample.instruction = "Calculate " + std::to_string(a) + " * " + std::to_string(b) + ".";
                sample.output = "The result is " + std::to_string(a * b) + ".";
            }
        } else if (i % 5 == 2) {
            sample.instruction = "Translate 'hello' to Spanish.";
            sample.output = "The translation is 'hola'.";
        } else if (i % 5 == 3) {
            sample.instruction = "What is the capital of France?";
            sample.output = "The capital of France is Paris.";
        } else {
            sample.instruction = "Write a haiku about coding.";
            sample.output = "Code flows like water\nBugs hide in the shadows deep\nDebug and rejoice";
        }
        
        sample.input = "";
        samples.push_back(sample);
    }
    
    return samples;
}

std::pair<std::vector<InstructionDataSample>, std::vector<InstructionDataSample>>
trainValSplit(const std::vector<InstructionDataSample>& samples, float validation_split) {
    if (validation_split <= 0.0f || validation_split >= 1.0f) {
        return {samples, {}};
    }
    
    size_t val_size = static_cast<size_t>(samples.size() * validation_split);
    size_t train_size = samples.size() - val_size;
    
    std::vector<InstructionDataSample> train_samples(
        samples.begin(), samples.begin() + train_size);
    std::vector<InstructionDataSample> val_samples(
        samples.begin() + train_size, samples.end());
    
    return {train_samples, val_samples};
}

} // namespace data_utils

} // namespace lora
} // namespace llm
} // namespace themis

