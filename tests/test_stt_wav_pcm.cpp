/**
 * @file test_stt_wav_pcm.cpp
 * @brief Unit tests for WAV/PCM extraction edge cases in STT Processor
 * 
 * Tests various WAV file formats, bit depths, channel configurations,
 * and error conditions to ensure robust audio processing.
 */

#include <gtest/gtest.h>
#include "content/stt_processor.h"
#include <vector>
#include <cstring>
#include <cmath>

using namespace themis::content;

class STTWavPcmTest : public ::testing::Test {
protected:
    // WAV format constants
    static constexpr size_t DATA_CHUNK_HEADER_SIZE = 8;  // 'data' identifier (4 bytes) + size field (4 bytes)
    
    void SetUp() override {
        processor = std::make_unique<STTProcessor>();
    }
    
    /**
     * @brief Create a valid WAV file header with specified parameters
     */
    std::vector<uint8_t> createWavHeader(
        uint32_t sample_rate,
        uint16_t num_channels,
        uint16_t bits_per_sample,
        uint32_t num_samples,
        uint16_t audio_format = 1  // 1 = PCM, 3 = IEEE float
    ) {
        std::vector<uint8_t> header;
        
        uint32_t data_size = num_samples * num_channels * (bits_per_sample / 8);
        uint32_t file_size = 36 + data_size;  // Total size minus 8 bytes
        
        // RIFF header
        header.push_back('R');
        header.push_back('I');
        header.push_back('F');
        header.push_back('F');
        
        // File size
        writeUInt32LE(header, file_size);
        
        // WAVE identifier
        header.push_back('W');
        header.push_back('A');
        header.push_back('V');
        header.push_back('E');
        
        // fmt chunk
        header.push_back('f');
        header.push_back('m');
        header.push_back('t');
        header.push_back(' ');
        
        // fmt chunk size
        writeUInt32LE(header, 16);
        
        // Audio format
        writeUInt16LE(header, audio_format);
        
        // Number of channels
        writeUInt16LE(header, num_channels);
        
        // Sample rate
        writeUInt32LE(header, sample_rate);
        
        // Byte rate
        uint32_t byte_rate = sample_rate * num_channels * (bits_per_sample / 8);
        writeUInt32LE(header, byte_rate);
        
        // Block align
        uint16_t block_align = num_channels * (bits_per_sample / 8);
        writeUInt16LE(header, block_align);
        
        // Bits per sample
        writeUInt16LE(header, bits_per_sample);
        
        // data chunk
        header.push_back('d');
        header.push_back('a');
        header.push_back('t');
        header.push_back('a');
        
        // Data size
        writeUInt32LE(header, data_size);
        
        return header;
    }
    
    void writeUInt32LE(std::vector<uint8_t>& vec, uint32_t value) {
        vec.push_back(value & 0xFF);
        vec.push_back((value >> 8) & 0xFF);
        vec.push_back((value >> 16) & 0xFF);
        vec.push_back((value >> 24) & 0xFF);
    }
    
    void writeUInt16LE(std::vector<uint8_t>& vec, uint16_t value) {
        vec.push_back(value & 0xFF);
        vec.push_back((value >> 8) & 0xFF);
    }
    
    void writeInt16LE(std::vector<uint8_t>& vec, int16_t value) {
        vec.push_back(value & 0xFF);
        vec.push_back((value >> 8) & 0xFF);
    }
    
    void writeFloat32LE(std::vector<uint8_t>& vec, float value) {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(float));
        writeUInt32LE(vec, bits);
    }
    
    std::unique_ptr<STTProcessor> processor;
};

// ============================================================================
// Valid WAV File Tests
// ============================================================================

TEST_F(STTWavPcmTest, Extract16BitMonoPCM) {
    // Create a 16-bit mono WAV with 4 samples
    auto wav = createWavHeader(16000, 1, 16, 4);
    
    // Add sample data: [0, 16384, -16384, 32767]
    writeInt16LE(wav, 0);
    writeInt16LE(wav, 16384);
    writeInt16LE(wav, -16384);
    writeInt16LE(wav, 32767);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 4);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_NEAR(pcm[1], 0.5f, 0.01f);
    EXPECT_NEAR(pcm[2], -0.5f, 0.01f);
    EXPECT_NEAR(pcm[3], 1.0f, 0.01f);
}

TEST_F(STTWavPcmTest, Extract16BitStereoPCM) {
    // Create a 16-bit stereo WAV with 2 samples (4 channel samples total)
    auto wav = createWavHeader(16000, 2, 16, 2);
    
    // Sample 1: Left=16384, Right=-16384 -> Average=0
    writeInt16LE(wav, 16384);
    writeInt16LE(wav, -16384);
    
    // Sample 2: Left=32767, Right=32767 -> Average=32767
    writeInt16LE(wav, 32767);
    writeInt16LE(wav, 32767);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 2);
    EXPECT_NEAR(pcm[0], 0.0f, 0.01f);  // Average of 0.5 and -0.5
    EXPECT_NEAR(pcm[1], 1.0f, 0.01f);  // Average of 1.0 and 1.0
}

TEST_F(STTWavPcmTest, Extract8BitMonoPCM) {
    // Create an 8-bit mono WAV with 4 samples
    auto wav = createWavHeader(16000, 1, 8, 4);
    
    // 8-bit PCM is unsigned: 0=min, 128=center, 255=max
    wav.push_back(0);    // -128 / 128 = -1.0
    wav.push_back(128);  // 0 / 128 = 0.0
    wav.push_back(192);  // 64 / 128 = 0.5
    wav.push_back(255);  // 127 / 128 ≈ 1.0
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 4);
    EXPECT_FLOAT_EQ(pcm[0], -1.0f);
    EXPECT_FLOAT_EQ(pcm[1], 0.0f);
    EXPECT_NEAR(pcm[2], 0.5f, 0.01f);
    EXPECT_NEAR(pcm[3], 0.99f, 0.01f);
}

TEST_F(STTWavPcmTest, Extract24BitMonoPCM) {
    // Create a 24-bit mono WAV with 3 samples
    auto wav = createWavHeader(16000, 1, 24, 3);
    
    // 24-bit samples (little-endian, signed)
    // Sample 1: 0
    wav.push_back(0x00);
    wav.push_back(0x00);
    wav.push_back(0x00);
    
    // Sample 2: 4194304 (0x400000) -> 0.5
    wav.push_back(0x00);
    wav.push_back(0x00);
    wav.push_back(0x40);
    
    // Sample 3: -4194304 (0xC00000 in 24-bit) -> -0.5
    wav.push_back(0x00);
    wav.push_back(0x00);
    wav.push_back(0xC0);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 3);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_NEAR(pcm[1], 0.5f, 0.01f);
    EXPECT_NEAR(pcm[2], -0.5f, 0.01f);
}

TEST_F(STTWavPcmTest, Extract32BitMonoPCM) {
    // Create a 32-bit mono WAV with 3 samples
    auto wav = createWavHeader(16000, 1, 32, 3);
    
    // 32-bit samples (little-endian, signed)
    writeUInt32LE(wav, 0);                    // 0
    writeUInt32LE(wav, 1073741824);          // 0.5
    writeUInt32LE(wav, 0xC0000000);          // -0.5 (two's complement)
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 3);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_NEAR(pcm[1], 0.5f, 0.01f);
    EXPECT_NEAR(pcm[2], -0.5f, 0.01f);
}

TEST_F(STTWavPcmTest, Extract32BitFloatMono) {
    // Create a 32-bit float WAV with 4 samples
    auto wav = createWavHeader(16000, 1, 32, 4, 3);  // format 3 = IEEE float
    
    writeFloat32LE(wav, 0.0f);
    writeFloat32LE(wav, 0.5f);
    writeFloat32LE(wav, -0.75f);
    writeFloat32LE(wav, 1.0f);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 4);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_FLOAT_EQ(pcm[1], 0.5f);
    EXPECT_FLOAT_EQ(pcm[2], -0.75f);
    EXPECT_FLOAT_EQ(pcm[3], 1.0f);
}

TEST_F(STTWavPcmTest, ExtractMultichannelToMono) {
    // Create a 4-channel WAV with 1 sample
    auto wav = createWavHeader(16000, 4, 16, 1);
    
    // Channels: 8192, 16384, -8192, -16384
    // Average: (8192 + 16384 - 8192 - 16384) / 4 = 0
    writeInt16LE(wav, 8192);
    writeInt16LE(wav, 16384);
    writeInt16LE(wav, -8192);
    writeInt16LE(wav, -16384);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 1);
    EXPECT_NEAR(pcm[0], 0.0f, 0.01f);
}

// ============================================================================
// Edge Case Tests - Different Sample Rates
// ============================================================================

TEST_F(STTWavPcmTest, AcceptDifferentSampleRates) {
    // Test that different sample rates are accepted
    // (resampling should be done by convertToWav16kHz, but extractPCMData should accept any rate)
    
    for (uint32_t sample_rate : {8000u, 11025u, 22050u, 44100u, 48000u}) {
        auto wav = createWavHeader(sample_rate, 1, 16, 1);
        writeInt16LE(wav, 16384);
        
        auto pcm = processor->extractPCMData(wav);
        ASSERT_EQ(pcm.size(), 1) << "Failed for sample rate " << sample_rate;
        EXPECT_NEAR(pcm[0], 0.5f, 0.01f) << "Failed for sample rate " << sample_rate;
    }
}

// ============================================================================
// WAV Header with Extra Chunks
// ============================================================================

TEST_F(STTWavPcmTest, HandleExtraChunksBeforeData) {
    // Create header with fmt chunk
    auto wav = createWavHeader(16000, 1, 16, 2);
    
    // But before adding data, insert the LIST chunk at position before data
    // Find data chunk position (should be at end of header)
    auto data_pos = wav.size() - DATA_CHUNK_HEADER_SIZE;  // Position before "data" identifier
    
    // Insert a LIST chunk before data
    std::vector<uint8_t> list_chunk;
    list_chunk.push_back('L');
    list_chunk.push_back('I');
    list_chunk.push_back('S');
    list_chunk.push_back('T');
    writeUInt32LE(list_chunk, 4);  // Chunk size
    list_chunk.push_back('I');
    list_chunk.push_back('N');
    list_chunk.push_back('F');
    list_chunk.push_back('O');
    
    // Insert LIST chunk before data chunk
    wav.insert(wav.begin() + data_pos, list_chunk.begin(), list_chunk.end());
    
    // Now add actual data samples
    writeInt16LE(wav, 16384);
    writeInt16LE(wav, -16384);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 2);
    EXPECT_NEAR(pcm[0], 0.5f, 0.01f);
    EXPECT_NEAR(pcm[1], -0.5f, 0.01f);
}

// ============================================================================
// Error Cases - Invalid Headers
// ============================================================================

TEST_F(STTWavPcmTest, RejectTooSmallFile) {
    std::vector<uint8_t> tiny_file = {0x52, 0x49, 0x46, 0x46};  // Just "RIFF"
    
    EXPECT_THROW({
        processor->extractPCMData(tiny_file);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectEmptyFile) {
    std::vector<uint8_t> empty_file;
    
    EXPECT_THROW({
        processor->extractPCMData(empty_file);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectInvalidRiffHeader) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Corrupt RIFF header
    wav[0] = 'X';
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectInvalidWaveIdentifier) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Corrupt WAVE identifier
    wav[8] = 'X';
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectMissingFmtChunk) {
    std::vector<uint8_t> wav;
    
    // Valid RIFF/WAVE header
    wav.push_back('R');
    wav.push_back('I');
    wav.push_back('F');
    wav.push_back('F');
    writeUInt32LE(wav, 36);
    wav.push_back('W');
    wav.push_back('A');
    wav.push_back('V');
    wav.push_back('E');
    
    // Add data chunk without fmt chunk
    wav.push_back('d');
    wav.push_back('a');
    wav.push_back('t');
    wav.push_back('a');
    writeUInt32LE(wav, 4);
    writeInt16LE(wav, 0);
    writeInt16LE(wav, 0);
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectMissingDataChunk) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Remove data chunk identifier (last 8 bytes are "data" + size)
    // Keep only up to the fmt chunk
    wav.resize(36);  // RIFF header + fmt chunk = 36 bytes
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectUnsupportedAudioFormat) {
    auto wav = createWavHeader(16000, 1, 16, 1, 2);  // Format 2 = Microsoft ADPCM
    writeInt16LE(wav, 0);
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectZeroChannels) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Set num_channels to 0 (byte offset 22-23)
    wav[22] = 0;
    wav[23] = 0;
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectZeroSampleRate) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Set sample_rate to 0 (byte offset 24-27)
    wav[24] = 0;
    wav[25] = 0;
    wav[26] = 0;
    wav[27] = 0;
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectZeroBitsPerSample) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Set bits_per_sample to 0 (byte offset 34-35)
    wav[34] = 0;
    wav[35] = 0;
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectInvalidBitsPerSample) {
    auto wav = createWavHeader(16000, 1, 16, 1);
    
    // Set bits_per_sample to 7 (not byte-aligned)
    wav[34] = 7;
    wav[35] = 0;
    
    // Should not throw for non-standard bit depth, but will throw when trying to parse
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectTruncatedDataChunk) {
    auto wav = createWavHeader(16000, 1, 16, 10);  // Claims 10 samples
    
    // But only add 1 sample
    writeInt16LE(wav, 0);
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

TEST_F(STTWavPcmTest, RejectTruncatedFmtChunk) {
    std::vector<uint8_t> wav;
    
    // Valid RIFF/WAVE header
    wav.push_back('R');
    wav.push_back('I');
    wav.push_back('F');
    wav.push_back('F');
    writeUInt32LE(wav, 36);
    wav.push_back('W');
    wav.push_back('A');
    wav.push_back('V');
    wav.push_back('E');
    
    // Add truncated fmt chunk
    wav.push_back('f');
    wav.push_back('m');
    wav.push_back('t');
    wav.push_back(' ');
    writeUInt32LE(wav, 16);  // Claims 16 bytes
    // But only add 8 bytes
    writeUInt16LE(wav, 1);
    writeUInt16LE(wav, 1);
    writeUInt32LE(wav, 16000);
    
    EXPECT_THROW({
        processor->extractPCMData(wav);
    }, std::runtime_error);
}

// ============================================================================
// Boundary Conditions
// ============================================================================

TEST_F(STTWavPcmTest, HandleMinimumValidWavFile) {
    // Smallest valid WAV: 16kHz mono 16-bit with 1 sample
    auto wav = createWavHeader(16000, 1, 16, 1);
    writeInt16LE(wav, 0);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 1);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
}

TEST_F(STTWavPcmTest, HandleLargeNumberOfSamples) {
    // Create WAV with 10000 samples
    const size_t num_samples = 10000;
    auto wav = createWavHeader(16000, 1, 16, num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        writeInt16LE(wav, 0);
    }
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), num_samples);
}

TEST_F(STTWavPcmTest, ClampOutOfRangeFloats) {
    // Create a 32-bit float WAV with out-of-range values
    auto wav = createWavHeader(16000, 1, 32, 3, 3);  // IEEE float
    
    writeFloat32LE(wav, 0.0f);
    writeFloat32LE(wav, 2.0f);   // Out of range (> 1.0)
    writeFloat32LE(wav, -2.0f);  // Out of range (< -1.0)
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 3);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_FLOAT_EQ(pcm[1], 1.0f);   // Clamped to 1.0
    EXPECT_FLOAT_EQ(pcm[2], -1.0f);  // Clamped to -1.0
}

// ============================================================================
// Real-world Format Tests
// ============================================================================

TEST_F(STTWavPcmTest, CDQualityAudio) {
    // CD quality: 44.1kHz, stereo, 16-bit
    auto wav = createWavHeader(44100, 2, 16, 2);
    
    // Left/Right pairs
    writeInt16LE(wav, 10000);
    writeInt16LE(wav, -10000);
    writeInt16LE(wav, 20000);
    writeInt16LE(wav, -20000);
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 2);
    // Each sample is average of left and right
    EXPECT_NEAR(pcm[0], 0.0f, 0.01f);
    EXPECT_NEAR(pcm[1], 0.0f, 0.01f);
}

TEST_F(STTWavPcmTest, TelephoneQuality) {
    // Telephone quality: 8kHz, mono, 8-bit
    auto wav = createWavHeader(8000, 1, 8, 4);
    
    wav.push_back(0);    // Min
    wav.push_back(64);   // Quarter
    wav.push_back(128);  // Center
    wav.push_back(255);  // Max
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 4);
    EXPECT_FLOAT_EQ(pcm[0], -1.0f);
    EXPECT_NEAR(pcm[1], -0.5f, 0.01f);
    EXPECT_FLOAT_EQ(pcm[2], 0.0f);
    EXPECT_NEAR(pcm[3], 0.99f, 0.01f);
}

TEST_F(STTWavPcmTest, StudioQuality) {
    // Studio quality: 96kHz, mono, 24-bit
    auto wav = createWavHeader(96000, 1, 24, 2);
    
    // 24-bit samples
    wav.push_back(0x00);
    wav.push_back(0x00);
    wav.push_back(0x00);
    
    wav.push_back(0xFF);
    wav.push_back(0xFF);
    wav.push_back(0x7F);  // Max positive value
    
    auto pcm = processor->extractPCMData(wav);
    
    ASSERT_EQ(pcm.size(), 2);
    EXPECT_FLOAT_EQ(pcm[0], 0.0f);
    EXPECT_NEAR(pcm[1], 1.0f, 0.01f);
}
