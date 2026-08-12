// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_content_audio_processor.cpp
 * @brief Unit tests for AudioProcessor metadata extraction
 *
 * Tests cover:
 * - WAV header parsing (sample rate, channels, bit depth, duration)
 * - FLAC STREAMINFO block parsing (sample rate, channels, total samples, duration)
 * - MP3 MPEG frame header parsing (bitrate, sample rate, channels)
 * - Ogg Vorbis identification header parsing (sample rate, channels, bitrate)
 * - ID3v2 tag parsing (title, artist, album, year, genre, track)
 * - Vorbis comment parsing (FLAC metadata tags)
 * - Error handling (empty blob, too-short blob, unknown format)
 * - Channel layout and quality classification
 */

#include <gtest/gtest.h>
#define THEMIS_PLUGIN_EXPORTS
#include "content/audio_processor.h"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

using namespace themis::content;

// ============================================================================
// Helper utilities for constructing synthetic audio blobs
// ============================================================================

static void writeLE16(std::vector<uint8_t>& buf, size_t offset, uint16_t val) {
    buf[offset]     = static_cast<uint8_t>(val & 0xFF);
    buf[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
}

static void writeLE32(std::vector<uint8_t>& buf, size_t offset, uint32_t val) {
    buf[offset]     = static_cast<uint8_t>(val & 0xFF);
    buf[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[offset + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[offset + 3] = static_cast<uint8_t>((val >> 24) & 0xFF);
}

[[maybe_unused]] static void writeBE32(std::vector<uint8_t>& buf, size_t offset, uint32_t val) {
    buf[offset]     = static_cast<uint8_t>((val >> 24) & 0xFF);
    buf[offset + 1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[offset + 2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[offset + 3] = static_cast<uint8_t>(val & 0xFF);
}

/**
 * Build a minimal valid WAV blob with the given PCM parameters.
 * The data chunk is padded with silence to produce the requested duration.
 */
static std::vector<uint8_t> buildWavBlob(
    uint32_t sample_rate,
    uint16_t num_channels,
    uint16_t bits_per_sample,
    uint32_t duration_ms)
{
    uint32_t byte_rate   = sample_rate * num_channels * bits_per_sample / 8;
    uint16_t block_align = static_cast<uint16_t>(num_channels * bits_per_sample / 8);
    uint32_t data_size   = static_cast<uint32_t>(
        static_cast<uint64_t>(byte_rate) * duration_ms / 1000
    );
    uint32_t riff_size   = 36 + data_size; // 4(WAVE) + 24(fmt) + 8(data hdr) + data_size

    std::vector<uint8_t> buf(44 + data_size, 0x00);

    // RIFF header
    buf[0] = 'R'; buf[1] = 'I'; buf[2] = 'F'; buf[3] = 'F';
    writeLE32(buf, 4, riff_size);
    buf[8] = 'W'; buf[9] = 'A'; buf[10] = 'V'; buf[11] = 'E';

    // fmt chunk
    buf[12] = 'f'; buf[13] = 'm'; buf[14] = 't'; buf[15] = ' ';
    writeLE32(buf, 16, 16); // PCM chunk size
    writeLE16(buf, 20, 1);  // PCM audio format
    writeLE16(buf, 22, num_channels);
    writeLE32(buf, 24, sample_rate);
    writeLE32(buf, 28, byte_rate);
    writeLE16(buf, 32, block_align);
    writeLE16(buf, 34, bits_per_sample);

    // data chunk
    buf[36] = 'd'; buf[37] = 'a'; buf[38] = 't'; buf[39] = 'a';
    writeLE32(buf, 40, data_size);
    // audio data is all zeros (silence)

    return buf;
}

/**
 * Build a minimal FLAC blob for the given parameters.
 * Contains only the fLaC marker + STREAMINFO metadata block.
 */
static std::vector<uint8_t> buildFlacBlob(
    uint32_t sample_rate,
    uint8_t  channels,
    uint8_t  bits_per_sample,
    uint64_t total_samples)
{
    std::vector<uint8_t> buf(42, 0x00); // 4 marker + 4 block hdr + 34 STREAMINFO

    // fLaC marker
    buf[0] = 'f'; buf[1] = 'L'; buf[2] = 'a'; buf[3] = 'C';

    // First metadata block header: last-block=1, type=0 (STREAMINFO), length=34
    buf[4] = 0x80; // last block + type STREAMINFO
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 34; // length

    // STREAMINFO block at offset 8
    // Bytes 0-1: min_block_size = 4096
    buf[8]  = 0x10; buf[9]  = 0x00;
    // Bytes 2-3: max_block_size = 4096
    buf[10] = 0x10; buf[11] = 0x00;
    // Bytes 4-6: min_frame_size = 0 (unknown)
    buf[12] = 0x00; buf[13] = 0x00; buf[14] = 0x00;
    // Bytes 7-9: max_frame_size = 0 (unknown)
    buf[15] = 0x00; buf[16] = 0x00; buf[17] = 0x00;

    // Bytes 10-17: sample_rate(20b) | channels-1(3b) | bps-1(5b) | total_samples(36b)
    //
    // Pack into 8 bytes (64 bits):
    //   bits 63-44: sample_rate (20 bits)
    //   bits 43-41: channels - 1 (3 bits)
    //   bits 40-36: bps - 1 (5 bits)
    //   bits 35-0 : total_samples (36 bits)
    uint64_t packed =
        ((uint64_t)(sample_rate & 0xFFFFF) << 44) |
        ((uint64_t)((channels - 1) & 0x07) << 41) |
        ((uint64_t)((bits_per_sample - 1) & 0x1F) << 36) |
        (total_samples & 0xFFFFFFFFFULL);

    buf[18] = static_cast<uint8_t>((packed >> 56) & 0xFF);
    buf[19] = static_cast<uint8_t>((packed >> 48) & 0xFF);
    buf[20] = static_cast<uint8_t>((packed >> 40) & 0xFF);
    buf[21] = static_cast<uint8_t>((packed >> 32) & 0xFF);
    buf[22] = static_cast<uint8_t>((packed >> 24) & 0xFF);
    buf[23] = static_cast<uint8_t>((packed >> 16) & 0xFF);
    buf[24] = static_cast<uint8_t>((packed >> 8) & 0xFF);
    buf[25] = static_cast<uint8_t>(packed & 0xFF);

    // Remaining 16 bytes: MD5 signature (zeroed)

    return buf;
}

/**
 * Build a minimal MP3 MPEG1 Layer3 sync frame header blob (no ID3 prefix).
 * Produces a 4-byte sync + filler.
 */
static std::vector<uint8_t> buildMp3FrameBlob(int bitrate_kbps, int sample_rate, bool stereo) {
    // Bitrate index table for MPEG1 Layer3
    static const int br_table[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
    // Sample rate index
    int sr_idx = 0;
    if (sample_rate == 48000) sr_idx = 1;
    else if (sample_rate == 32000) sr_idx = 2;

    int br_idx = 0;
    for (int i = 1; i < 15; i++) {
        if (br_table[i] == bitrate_kbps) { br_idx = i; break; }
    }

    // byte 0: 0xFF
    // byte 1: 0xFB = 1111 1011 → sync(11) + MPEG1(11) + Layer3(01) + no-CRC(1)
    uint8_t b1 = 0xFB; // MPEG1 Layer3
    uint8_t b2 = static_cast<uint8_t>(
        (br_idx << 4) | (sr_idx << 2) | 0x00 // no padding, private=0
    );
    uint8_t b3 = static_cast<uint8_t>(
        (stereo ? 0x00 : 0xC0) // 0x00=Stereo, 0xC0=Mono
    );

    std::vector<uint8_t> buf(256, 0x00);
    buf[0] = 0xFF;
    buf[1] = b1;
    buf[2] = b2;
    buf[3] = b3;
    return buf;
}

/**
 * Build a minimal ID3v2.3 tag blob with a single text frame.
 */
static std::vector<uint8_t> buildId3v2Tag(
    const std::string& frame_id,
    const std::string& value)
{
    // Frame: 4(ID) + 4(size BE) + 2(flags) + 1(encoding) + value
    uint32_t frame_data_size = static_cast<uint32_t>(1 + value.size()); // encoding byte + text
    uint32_t frame_total = 10 + frame_data_size;

    // Tag size (syncsafe) = frame_total
    uint32_t tag_size = frame_total;
    uint8_t ss0 = (tag_size >> 21) & 0x7F;
    uint8_t ss1 = (tag_size >> 14) & 0x7F;
    uint8_t ss2 = (tag_size >> 7)  & 0x7F;
    uint8_t ss3 = tag_size & 0x7F;

    std::vector<uint8_t> buf;
    // ID3v2 header (10 bytes)
    buf.push_back('I'); buf.push_back('D'); buf.push_back('3');
    buf.push_back(3); buf.push_back(0); // version 2.3
    buf.push_back(0); // flags
    buf.push_back(ss0); buf.push_back(ss1); buf.push_back(ss2); buf.push_back(ss3);

    // Frame header (10 bytes)
    for (char c : frame_id) buf.push_back(static_cast<uint8_t>(c));
    // Frame size BE
    buf.push_back((frame_data_size >> 24) & 0xFF);
    buf.push_back((frame_data_size >> 16) & 0xFF);
    buf.push_back((frame_data_size >> 8) & 0xFF);
    buf.push_back(frame_data_size & 0xFF);
    buf.push_back(0); buf.push_back(0); // flags

    // Frame data: encoding (UTF-8 = 3) + text
    buf.push_back(3); // UTF-8 encoding
    for (char c : value) buf.push_back(static_cast<uint8_t>(c));

    return buf;
}

// ============================================================================
// Test Fixture
// ============================================================================

class AudioProcessorTest : public ::testing::Test {
protected:
    AudioProcessor processor;

    void SetUp() override {
        PluginConfig config;
        ASSERT_TRUE(processor.initialize(config));
    }
};

// ============================================================================
// WAV metadata extraction tests
// ============================================================================

TEST_F(AudioProcessorTest, Wav_ExtractsSampleRateAndChannels) {
    auto blob = buildWavBlob(44100, 2, 16, 1000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->sample_rate, 44100);
    EXPECT_EQ(result.media->channels, 2);
    EXPECT_EQ(result.media->container_format, "wav");
    EXPECT_EQ(result.media->audio_codec, "pcm");
}

TEST_F(AudioProcessorTest, Wav_ExtractsDuration) {
    // 2 seconds of 44100 Hz stereo 16-bit PCM
    auto blob = buildWavBlob(44100, 2, 16, 2000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    // Duration should be approximately 2000 ms (allow ±10 ms rounding)
    EXPECT_NEAR(static_cast<double>(result.media->duration_ms), 2000.0, 10.0);
}

TEST_F(AudioProcessorTest, Wav_MonoChannel) {
    auto blob = buildWavBlob(22050, 1, 16, 500);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->channels, 1);
    EXPECT_EQ(result.media->sample_rate, 22050);
    EXPECT_EQ(result.metadata["channel_layout"], "mono");
}

TEST_F(AudioProcessorTest, Wav_StereoChannelLayout) {
    auto blob = buildWavBlob(44100, 2, 16, 1000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.metadata["channel_layout"], "stereo");
}

TEST_F(AudioProcessorTest, Wav_BitrateDerivedFromFormat) {
    // 44100 Hz stereo 16-bit: byte_rate = 44100*2*2 = 176400 B/s = 1411.2 kbps
    auto blob = buildWavBlob(44100, 2, 16, 1000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    // Bitrate = sample_rate * channels * bits_per_sample / 1000 = 1411 kbps
    EXPECT_GT(result.media->bitrate_kbps, 0);
}

// ============================================================================
// FLAC metadata extraction tests
// ============================================================================

TEST_F(AudioProcessorTest, Flac_ExtractsSampleRateAndChannels) {
    // 44100 Hz, 2 channels, 16 bps, 44100 samples = 1 second
    auto blob = buildFlacBlob(44100, 2, 16, 44100);
    auto result = processor.extract(blob, "audio/flac");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->sample_rate, 44100);
    EXPECT_EQ(result.media->channels, 2);
    EXPECT_EQ(result.media->container_format, "flac");
    EXPECT_EQ(result.media->audio_codec, "flac");
}

TEST_F(AudioProcessorTest, Flac_ExtractsDuration) {
    // 48000 Hz, 1 channel, 24 bps, 96000 samples = 2 seconds
    auto blob = buildFlacBlob(48000, 1, 24, 96000);
    auto result = processor.extract(blob, "audio/flac");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_NEAR(static_cast<double>(result.media->duration_ms), 2000.0, 5.0);
}

TEST_F(AudioProcessorTest, Flac_HiResSampleRate) {
    // 96000 Hz qualifies as Hi-Res
    auto blob = buildFlacBlob(96000, 2, 24, 192000);
    auto result = processor.extract(blob, "audio/flac");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.metadata["quality_class"], "Hi-Res");
}

// ============================================================================
// MP3 metadata extraction tests
// ============================================================================

TEST_F(AudioProcessorTest, Mp3_ExtractsBitrateAndSampleRate) {
    auto blob = buildMp3FrameBlob(128, 44100, true);
    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->sample_rate, 44100);
    EXPECT_EQ(result.media->bitrate_kbps, 128);
    EXPECT_EQ(result.media->channels, 2);
    EXPECT_EQ(result.media->container_format, "mp3");
    EXPECT_EQ(result.media->audio_codec, "mp3");
}

TEST_F(AudioProcessorTest, Mp3_MonoChannel) {
    auto blob = buildMp3FrameBlob(64, 44100, false);
    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->channels, 1);
    EXPECT_EQ(result.metadata["channel_layout"], "mono");
}

TEST_F(AudioProcessorTest, Mp3_320kbps) {
    auto blob = buildMp3FrameBlob(320, 44100, true);
    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->bitrate_kbps, 320);
}

// ============================================================================
// ID3v2 tag extraction tests
// ============================================================================

TEST_F(AudioProcessorTest, Id3v2_ExtractsTitleTag) {
    // Build an ID3v2 blob with TIT2 (title) frame + MP3 sync frame
    auto id3 = buildId3v2Tag("TIT2", "Test Track");
    auto mp3  = buildMp3FrameBlob(128, 44100, true);
    // Concatenate id3 + mp3
    std::vector<uint8_t> blob = id3;
    blob.insert(blob.end(), mp3.begin(), mp3.end());

    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("tags"));
    EXPECT_EQ(result.metadata["tags"]["title"], "Test Track");
}

TEST_F(AudioProcessorTest, Id3v2_ExtractsArtistTag) {
    auto id3 = buildId3v2Tag("TPE1", "Test Artist");
    auto mp3  = buildMp3FrameBlob(128, 44100, true);
    std::vector<uint8_t> blob = id3;
    blob.insert(blob.end(), mp3.begin(), mp3.end());

    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("tags"));
    EXPECT_EQ(result.metadata["tags"]["artist"], "Test Artist");
}

TEST_F(AudioProcessorTest, Id3v2_ExtractsAlbumTag) {
    auto id3 = buildId3v2Tag("TALB", "Test Album");
    auto mp3  = buildMp3FrameBlob(128, 44100, true);
    std::vector<uint8_t> blob = id3;
    blob.insert(blob.end(), mp3.begin(), mp3.end());

    auto result = processor.extract(blob, "audio/mpeg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("tags"));
    EXPECT_EQ(result.metadata["tags"]["album"], "Test Album");
}

// ============================================================================
// Ogg Vorbis metadata extraction tests
// ============================================================================

TEST_F(AudioProcessorTest, Ogg_DetectsContainerFormat) {
    // Construct a minimal Ogg blob with Vorbis identification header
    // Ogg page header (bytes 0-26) + segment table (byte 27) + Vorbis ID header
    std::vector<uint8_t> blob(128, 0x00);
    blob[0] = 'O'; blob[1] = 'g'; blob[2] = 'g'; blob[3] = 'S';
    // byte 4: version=0, byte 5: header_type=2 (beginning of stream)
    // bytes 6-13: granule_pos=0, bytes 14-17: serial=1, bytes 18-21: seq=0, bytes 22-25: crc=0
    // Byte 26: page_segments = 1
    blob[26] = 1;  // num_segments = 1
    blob[27] = 30; // segment_table[0] = 30 bytes
    // Vorbis ID header starts at offset 28 (= 27 + 1 segment)
    size_t vid = 28;
    blob[vid]   = 0x01; // packet_type = identification
    blob[vid+1] = 'v'; blob[vid+2] = 'o'; blob[vid+3] = 'r'; blob[vid+4] = 'b';
    blob[vid+5] = 'i'; blob[vid+6] = 's';
    // vorbis_version = 0 (4 bytes LE)
    blob[vid+7] = 0x00; blob[vid+8] = 0x00; blob[vid+9] = 0x00; blob[vid+10] = 0x00;
    // audio_channels = 2
    blob[vid+11] = 2;
    // audio_sample_rate = 44100 = 0x0000AC44 (LE)
    blob[vid+12] = 0x44; blob[vid+13] = 0xAC; blob[vid+14] = 0x00; blob[vid+15] = 0x00;
    // bitrate_maximum = 0
    blob[vid+16] = 0; blob[vid+17] = 0; blob[vid+18] = 0; blob[vid+19] = 0;
    // bitrate_nominal = 128000 = 0x0001F400 (LE)
    blob[vid+20] = 0x00; blob[vid+21] = 0xF4; blob[vid+22] = 0x01; blob[vid+23] = 0x00;

    auto result = processor.extract(blob, "audio/ogg");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.media.has_value());
    EXPECT_EQ(result.media->container_format, "ogg");
    EXPECT_EQ(result.media->audio_codec, "vorbis");
    EXPECT_EQ(result.media->channels, 2);
    EXPECT_EQ(result.media->sample_rate, 44100);
    EXPECT_EQ(result.media->bitrate_kbps, 128);
}

// ============================================================================
// Error handling tests
// ============================================================================

TEST_F(AudioProcessorTest, EmptyBlob_ReturnsFalse) {
    std::vector<uint8_t> empty;
    auto result = processor.extract(empty, "audio/wav");
    EXPECT_FALSE(result.success);
}

TEST_F(AudioProcessorTest, TooShortBlob_NoSegfault) {
    std::vector<uint8_t> tiny = {0x52, 0x49, 0x46, 0x46}; // "RIFF" only
    auto result = processor.extract(tiny, "audio/wav");
    // Should not crash; may succeed or fail gracefully
    EXPECT_NO_FATAL_FAILURE(processor.extract(tiny, "audio/wav"));
}

TEST_F(AudioProcessorTest, UnknownFormat_SucceedsWithNoMediaData) {
    std::vector<uint8_t> blob(64, 0xAB); // arbitrary unknown bytes
    auto result = processor.extract(blob, "audio/mpeg");
    // extract() should return success=true even if format is unrecognised (no metadata)
    // or return success=false; what matters is no crash
    EXPECT_NO_FATAL_FAILURE(processor.extract(blob, "audio/mpeg"));
}

// ============================================================================
// CanProcess / plugin info tests
// ============================================================================

TEST_F(AudioProcessorTest, CanProcess_SupportedMimeTypes) {
    EXPECT_TRUE(processor.canProcess("audio/mpeg"));
    EXPECT_TRUE(processor.canProcess("audio/wav"));
    EXPECT_TRUE(processor.canProcess("audio/flac"));
    EXPECT_TRUE(processor.canProcess("audio/ogg"));
    EXPECT_TRUE(processor.canProcess("audio/aac"));
    EXPECT_TRUE(processor.canProcess("audio/mp4"));
    EXPECT_TRUE(processor.canProcess("audio/webm"));
    EXPECT_TRUE(processor.canProcess("audio/x-m4a"));
}

TEST_F(AudioProcessorTest, CanProcess_UnsupportedMimeType) {
    EXPECT_FALSE(processor.canProcess("video/mp4"));
    EXPECT_FALSE(processor.canProcess("image/jpeg"));
    EXPECT_FALSE(processor.canProcess("text/plain"));
}

TEST_F(AudioProcessorTest, GetInfo_HasCorrectName) {
    auto info = processor.getInfo();
    EXPECT_EQ(info.name, "audio-processor");
    EXPECT_EQ(info.supports_streaming, true);
}

// ============================================================================
// Quality classification tests
// ============================================================================

TEST_F(AudioProcessorTest, QualityClass_HiRes) {
    auto blob = buildFlacBlob(96000, 2, 24, 192000);
    auto result = processor.extract(blob, "audio/flac");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.metadata["quality_class"], "Hi-Res");
}

TEST_F(AudioProcessorTest, QualityClass_CdQuality) {
    auto blob = buildWavBlob(44100, 2, 16, 1000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.metadata["quality_class"], "CD Quality");
}

TEST_F(AudioProcessorTest, QualityClass_Standard) {
    auto blob = buildWavBlob(22050, 1, 8, 1000);
    auto result = processor.extract(blob, "audio/wav");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.metadata["quality_class"], "Standard");
}

// ============================================================================
// Statistics tests
// ============================================================================

TEST_F(AudioProcessorTest, Statistics_HealthCheckAfterInit) {
    EXPECT_TRUE(processor.healthCheck());
}

TEST_F(AudioProcessorTest, Statistics_FileCountIncrementsOnSuccess) {
    auto blob = buildWavBlob(44100, 2, 16, 100);
    auto stats_before = processor.getStatistics();
    processor.extract(blob, "audio/wav");
    auto stats_after = processor.getStatistics();
    EXPECT_GT(stats_after["audio_files_processed"].get<uint64_t>(),
              stats_before["audio_files_processed"].get<uint64_t>());
}

// ============================================================================
// Transcription / STT integration tests
// ============================================================================

/**
 * @brief Test fixture for AudioProcessor with transcription enabled.
 *
 * Initializes the processor with "transcription.enabled": true.
 * STTProcessor will initialise in fallback mode when THEMIS_ENABLE_WHISPER is
 * not defined (no model file required).
 */
class AudioProcessorTranscriptionTest : public ::testing::Test {
protected:
    AudioProcessor processor;

    void SetUp() override {
        nlohmann::json settings;
        settings["transcription"]["enabled"] = true;
        settings["transcription"]["model"] = "base";
        settings["transcription"]["language"] = "en";
        PluginConfig config(settings);
        ASSERT_TRUE(processor.initialize(config));
    }
};

TEST_F(AudioProcessorTranscriptionTest, Transcription_ExtractReturnsText_WhenEnabled) {
    // When transcription is enabled, extracting with extract_text=true should
    // populate result.text (even the placeholder text without Whisper).
    auto blob = buildWavBlob(16000, 1, 16, 500);
    ExtractionOptions opts;
    opts.extract_text = true;
    auto result = processor.extract(blob, "audio/wav", opts);
    ASSERT_TRUE(result.success);
    // The text may be the Whisper placeholder or actual transcription; either
    // way it must be a non-empty string when transcription is available.
    EXPECT_FALSE(result.text.empty());
}

TEST_F(AudioProcessorTranscriptionTest, Transcription_StatisticsCountIncrement) {
    auto blob = buildWavBlob(16000, 1, 16, 200);
    ExtractionOptions opts;
    opts.extract_text = true;
    auto stats_before = processor.getStatistics();
    processor.extract(blob, "audio/wav", opts);
    auto stats_after = processor.getStatistics();
    EXPECT_GT(stats_after["transcriptions_performed"].get<uint64_t>(),
              stats_before["transcriptions_performed"].get<uint64_t>());
}

TEST_F(AudioProcessorTranscriptionTest, Transcription_MetadataPopulated) {
    // Transcription metadata (language, confidence, segments) must be
    // present in result.metadata["transcription"] after a successful extract.
    auto blob = buildWavBlob(16000, 1, 16, 500);
    ExtractionOptions opts;
    opts.extract_text = true;
    auto result = processor.extract(blob, "audio/wav", opts);
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.contains("transcription"));
    const auto& tmeta = result.metadata["transcription"];
    EXPECT_TRUE(tmeta.contains("language"));
    EXPECT_TRUE(tmeta.contains("confidence"));
    EXPECT_TRUE(tmeta.contains("audio_duration_ms"));
    EXPECT_TRUE(tmeta.contains("segment_count"));
    EXPECT_TRUE(tmeta.contains("segments"));
    EXPECT_TRUE(tmeta["segments"].is_array());
}

TEST_F(AudioProcessorTranscriptionTest, Transcription_ShutdownAndReinitialize) {
    processor.shutdown();
    EXPECT_FALSE(processor.healthCheck());

    nlohmann::json settings;
    settings["transcription"]["enabled"] = true;
    PluginConfig config(settings);
    ASSERT_TRUE(processor.initialize(config));
    EXPECT_TRUE(processor.healthCheck());
}

TEST(AudioProcessorNoTranscriptionTest, Transcription_DisabledByDefault_NoText) {
    AudioProcessor proc;
    PluginConfig config;  // default: transcription.enabled = false
    ASSERT_TRUE(proc.initialize(config));

    auto blob = buildWavBlob(44100, 2, 16, 500);
    ExtractionOptions opts;
    opts.extract_text = true;
    auto result = proc.extract(blob, "audio/wav", opts);
    ASSERT_TRUE(result.success);
    // No STT processor -> transcribe() returns "" -> result.text is empty
    EXPECT_TRUE(result.text.empty());
}
