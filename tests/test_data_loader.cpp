#include <gtest/gtest.h>
#include "llm/lora_framework/data_loader.h"
#include <memory>
#include <fstream>
#include <filesystem>

using namespace themis::llm::lora;
namespace fs = std::filesystem;

/**
 * @file test_data_loader.cpp
 * @brief Comprehensive tests for data loading and tokenization
 * 
 * Test Coverage:
 * - Tokenizer functionality
 * - Dataset loading (multiple formats)
 * - Batch creation and padding
 * - Iteration and shuffling
 */

class DataLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        tokenizer_ = std::make_shared<SimpleTokenizer>();
        temp_dir_ = fs::temp_directory_path() / "themis_test_data";
        fs::create_directories(temp_dir_);
    }
    
    void TearDown() override {
        // Clean up temp files
        if (fs::exists(temp_dir_)) {
            fs::remove_all(temp_dir_);
        }
    }
    
    std::shared_ptr<SimpleTokenizer> tokenizer_;
    fs::path temp_dir_;
    
    // Helper to create test file
    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(temp_dir_ / filename);
        file << content;
        file.close();
    }
};

// ===== SimpleTokenizer Tests =====

TEST_F(DataLoaderTest, SimpleTokenizer_VocabSize) {
    EXPECT_GT(tokenizer_->vocab_size(), 0);
}

TEST_F(DataLoaderTest, SimpleTokenizer_SpecialTokens) {
    EXPECT_EQ(tokenizer_->bos_token_id(), 1);
    EXPECT_EQ(tokenizer_->eos_token_id(), 2);
    EXPECT_EQ(tokenizer_->pad_token_id(), 0);
}

TEST_F(DataLoaderTest, SimpleTokenizer_EncodeBasic) {
    std::string text = "Hello";
    auto tokens = tokenizer_->encode(text, false, false);
    
    EXPECT_GT(tokens.size(), 0);
}

TEST_F(DataLoaderTest, SimpleTokenizer_EncodeWithSpecialTokens) {
    std::string text = "Hello";
    auto tokens = tokenizer_->encode(text, true, true);
    
    EXPECT_GE(tokens.size(), 3);  // At least BOS + content + EOS
    EXPECT_EQ(tokens.front(), tokenizer_->bos_token_id());
    EXPECT_EQ(tokens.back(), tokenizer_->eos_token_id());
}

TEST_F(DataLoaderTest, SimpleTokenizer_DecodeBasic) {
    std::vector<int> tokens = {100, 101, 102};
    std::string decoded = tokenizer_->decode(tokens);
    
    EXPECT_FALSE(decoded.empty());
}

TEST_F(DataLoaderTest, SimpleTokenizer_RoundTrip) {
    std::string original = "Test message 123";
    auto encoded = tokenizer_->encode(original, false, false);
    std::string decoded = tokenizer_->decode(encoded);
    
    EXPECT_EQ(original, decoded);
}

// ===== DataLoader Construction Tests =====

TEST_F(DataLoaderTest, DataLoader_Construction) {
    DataLoaderConfig config;
    DataLoader loader(tokenizer_, config);
    
    EXPECT_EQ(loader.size(), 0);
    EXPECT_FALSE(loader.hasNext());
}

TEST_F(DataLoaderTest, DataLoader_Config) {
    DataLoaderConfig config;
    config.batch_size = 8;
    config.max_sequence_length = 512;
    config.shuffle = true;
    
    DataLoader loader(tokenizer_, config);
    
    const auto& retrieved_config = loader.getConfig();
    EXPECT_EQ(retrieved_config.batch_size, 8);
    EXPECT_EQ(retrieved_config.max_sequence_length, 512);
    EXPECT_TRUE(retrieved_config.shuffle);
}

// ===== Sample Loading Tests =====

TEST_F(DataLoaderTest, DataLoader_LoadFromSamples) {
    DataLoader loader(tokenizer_);
    
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 5; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        samples.push_back(sample);
    }
    
    bool loaded = loader.loadFromSamples(samples);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(loader.size(), 5);
}

TEST_F(DataLoaderTest, DataLoader_GetSample) {
    DataLoader loader(tokenizer_);
    
    std::vector<InstructionDataSample> samples;
    InstructionDataSample sample1;
    sample1.instruction = "Test instruction";
    sample1.output = "Test output";
    samples.push_back(sample1);
    
    loader.loadFromSamples(samples);
    
    auto retrieved = loader.getSample(0);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->instruction, "Test instruction");
    EXPECT_EQ(retrieved->output, "Test output");
    
    // Out of bounds
    auto invalid = loader.getSample(999);
    EXPECT_FALSE(invalid.has_value());
}

// ===== JSON Loading Tests =====

TEST_F(DataLoaderTest, DataLoader_LoadFromJSON_Array) {
    DataLoader loader(tokenizer_);
    
    std::string json_data = R"([
        {
            "instruction": "Add 1 and 2",
            "input": "",
            "output": "3"
        },
        {
            "instruction": "Multiply 3 and 4",
            "input": "",
            "output": "12"
        }
    ])";
    
    bool loaded = loader.loadFromJSON(json_data);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(loader.size(), 2);
    
    auto sample = loader.getSample(0);
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->instruction, "Add 1 and 2");
}

TEST_F(DataLoaderTest, DataLoader_LoadFromJSON_SingleObject) {
    DataLoader loader(tokenizer_);
    
    std::string json_data = R"({
        "instruction": "What is AI?",
        "input": "",
        "output": "Artificial Intelligence"
    })";
    
    bool loaded = loader.loadFromJSON(json_data);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(loader.size(), 1);
}

TEST_F(DataLoaderTest, DataLoader_LoadFromFile_JSONL) {
    // Create JSONL test file
    std::string jsonl_content = 
        R"({"instruction": "Q1", "input": "", "output": "A1"})" "\n"
        R"({"instruction": "Q2", "input": "", "output": "A2"})" "\n"
        R"({"instruction": "Q3", "input": "", "output": "A3"})" "\n";
    
    createTestFile("test.jsonl", jsonl_content);
    
    DataLoaderConfig config;
    config.format = DatasetFormat::JSONL;
    DataLoader loader(tokenizer_, config);
    
    bool loaded = loader.loadFromFile((temp_dir_ / "test.jsonl").string());
    EXPECT_TRUE(loaded);
    EXPECT_EQ(loader.size(), 3);
}

// ===== Batch Creation Tests =====

TEST_F(DataLoaderTest, DataLoader_GetBatch_Basic) {
    DataLoaderConfig config;
    config.batch_size = 2;
    config.max_sequence_length = 64;
    config.shuffle = false;
    
    DataLoader loader(tokenizer_, config);
    
    auto samples = data_utils::createToyDataset(5);
    loader.loadFromSamples(samples);
    
    auto batch = loader.getNextBatch();
    
    EXPECT_FALSE(batch.empty());
    EXPECT_EQ(batch.batch_size, 2);
    EXPECT_EQ(batch.input_ids.size(), 2);
    EXPECT_EQ(batch.label_ids.size(), 2);
}

TEST_F(DataLoaderTest, DataLoader_GetBatch_Padding) {
    DataLoaderConfig config;
    config.batch_size = 3;
    config.max_sequence_length = 128;
    config.pad_to_max_length = true;
    config.shuffle = false;
    
    DataLoader loader(tokenizer_, config);
    
    auto samples = data_utils::createToyDataset(3);
    loader.loadFromSamples(samples);
    
    auto batch = loader.getNextBatch();
    
    // All sequences should be padded to max_sequence_length
    for (const auto& seq : batch.input_ids) {
        EXPECT_EQ(seq.size(), static_cast<size_t>(config.max_sequence_length));
    }
    
    for (const auto& seq : batch.label_ids) {
        EXPECT_EQ(seq.size(), static_cast<size_t>(config.max_sequence_length));
    }
}

TEST_F(DataLoaderTest, DataLoader_BatchIteration) {
    DataLoaderConfig config;
    config.batch_size = 2;
    config.shuffle = false;
    
    DataLoader loader(tokenizer_, config);
    
    auto samples = data_utils::createToyDataset(7);
    loader.loadFromSamples(samples);
    
    EXPECT_EQ(loader.num_batches(), 4);  // 7 samples / 2 per batch = 4 batches
    
    int batch_count = 0;
    int total_samples = 0;
    
    while (loader.hasNext()) {
        auto batch = loader.getNextBatch();
        EXPECT_FALSE(batch.empty());
        batch_count++;
        total_samples += batch.batch_size;
    }
    
    EXPECT_EQ(batch_count, 4);
    EXPECT_EQ(total_samples, 7);
}

TEST_F(DataLoaderTest, DataLoader_Reset) {
    DataLoaderConfig config;
    config.batch_size = 2;
    config.shuffle = false;
    
    DataLoader loader(tokenizer_, config);
    
    auto samples = data_utils::createToyDataset(4);
    loader.loadFromSamples(samples);
    
    // Exhaust the loader
    while (loader.hasNext()) {
        loader.getNextBatch();
    }
    
    EXPECT_FALSE(loader.hasNext());
    
    // Reset and iterate again
    loader.reset();
    EXPECT_TRUE(loader.hasNext());
    
    int batch_count = 0;
    while (loader.hasNext()) {
        loader.getNextBatch();
        batch_count++;
    }
    
    EXPECT_EQ(batch_count, 2);
}

TEST_F(DataLoaderTest, DataLoader_Shuffle) {
    DataLoaderConfig config;
    config.batch_size = 1;
    config.shuffle = false;
    
    DataLoader loader(tokenizer_, config);
    
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 10; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        samples.push_back(sample);
    }
    
    loader.loadFromSamples(samples);
    
    // Get first sample before shuffle
    auto first_before = loader.getSample(0);
    
    // Shuffle
    loader.shuffle();
    
    // After shuffling internal indices, samples themselves don't change
    // (getSample accesses by original index)
    auto first_after = loader.getSample(0);
    EXPECT_EQ(first_before->instruction, first_after->instruction);
}

// ===== Data Utilities Tests =====

TEST_F(DataLoaderTest, DataUtils_CreateToyDataset) {
    auto samples = data_utils::createToyDataset(10);
    
    EXPECT_EQ(samples.size(), 10);
    
    for (const auto& sample : samples) {
        EXPECT_FALSE(sample.instruction.empty());
        EXPECT_FALSE(sample.output.empty());
    }
}

TEST_F(DataLoaderTest, DataUtils_TrainValSplit) {
    auto samples = data_utils::createToyDataset(100);
    
    auto [train, val] = data_utils::trainValSplit(samples, 0.2f);
    
    EXPECT_EQ(train.size(), 80);
    EXPECT_EQ(val.size(), 20);
    EXPECT_EQ(train.size() + val.size(), samples.size());
}

TEST_F(DataLoaderTest, DataUtils_TrainValSplit_EdgeCases) {
    auto samples = data_utils::createToyDataset(10);
    
    // No validation split
    auto [train1, val1] = data_utils::trainValSplit(samples, 0.0f);
    EXPECT_EQ(train1.size(), 10);
    EXPECT_EQ(val1.size(), 0);
    
    // Invalid split (>= 1.0)
    auto [train2, val2] = data_utils::trainValSplit(samples, 1.0f);
    EXPECT_EQ(train2.size(), 10);
    EXPECT_EQ(val2.size(), 0);
}

TEST_F(DataLoaderTest, DataUtils_LoadAlpacaFormat) {
    std::string json_data = R"([
        {
            "instruction": "What is the capital of France?",
            "input": "",
            "output": "Paris"
        },
        {
            "instruction": "Translate to Spanish",
            "input": "Hello",
            "output": "Hola"
        }
    ])";
    
    auto samples = data_utils::loadAlpacaFormat(json_data);
    
    EXPECT_EQ(samples.size(), 2);
    EXPECT_EQ(samples[0].instruction, "What is the capital of France?");
    EXPECT_EQ(samples[0].output, "Paris");
    EXPECT_EQ(samples[1].input, "Hello");
}

TEST_F(DataLoaderTest, DataUtils_LoadShareGPTFormat) {
    std::string json_data = R"([
        {
            "conversations": [
                {"from": "human", "value": "What is 2+2?"},
                {"from": "gpt", "value": "4"}
            ]
        },
        {
            "conversations": [
                {"from": "user", "value": "Hello"},
                {"from": "assistant", "value": "Hi there!"}
            ]
        }
    ])";
    
    auto samples = data_utils::loadShareGPTFormat(json_data);
    
    EXPECT_GE(samples.size(), 1);  // At least one conversation pair
    
    if (samples.size() > 0) {
        EXPECT_FALSE(samples[0].instruction.empty());
        EXPECT_FALSE(samples[0].output.empty());
    }
}

// ===== InstructionDataSample Tests =====

TEST_F(DataLoaderTest, InstructionDataSample_GetFullPrompt) {
    InstructionDataSample sample;
    sample.instruction = "Calculate";
    sample.input = "";
    sample.output = "42";
    
    EXPECT_EQ(sample.getFullPrompt(), "Calculate");
    
    sample.input = "2 + 2";
    EXPECT_EQ(sample.getFullPrompt(), "Calculate\n2 + 2");
}

// ===== Custom Formatter Test =====

TEST_F(DataLoaderTest, DataLoader_CustomFormatter) {
    DataLoader loader(tokenizer_);
    
    // Set custom formatter
    loader.setFormatter([](const InstructionDataSample& sample) {
        return "CUSTOM: " + sample.instruction + " -> " + sample.output;
    });
    
    InstructionDataSample sample;
    sample.instruction = "Test";
    sample.output = "Result";
    
    loader.loadFromSamples({sample});
    
    // The formatter affects tokenization (internal)
    // We can't directly test the formatted text, but can verify it loads
    EXPECT_EQ(loader.size(), 1);
}


