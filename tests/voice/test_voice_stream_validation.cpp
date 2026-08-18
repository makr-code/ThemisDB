/**
 * @file test_voice_stream_validation.cpp
 * @brief VoiceStreamValidator behavior tests for fail-closed stream validation.
 */

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "voice/voice_stream_validator.h"

namespace themis {
namespace voice {
namespace test {

TEST(VoiceStreamValidation, ValidChunkAcceptedAndStateUpdated)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    const std::array<uint8_t, 8> chunk = {1, 2, 3, 4, 5, 6, 7, 8};

    auto validated = validator.validate_chunk(chunk.data(), chunk.size(), 1, 100);

    EXPECT_EQ(validated.sequence_num, 1u);
    EXPECT_EQ(validated.sample_rate, 16000u);
    EXPECT_EQ(validated.num_channels, 1u);
    EXPECT_EQ(validated.bit_depth, 16u);
    EXPECT_EQ(validator.chunks_validated(), 1u);
    EXPECT_EQ(validator.total_bytes_validated(), chunk.size());
}

TEST(VoiceStreamValidation, EmptyChunkRejected)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);

    EXPECT_THROW(
        validator.validate_chunk(nullptr, 0, 1, 0),
        StreamValidationError);
}

TEST(VoiceStreamValidation, OversizedChunkRejected)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    std::vector<uint8_t> oversized(
        StreamValidationPolicy::MAX_CHUNK_SIZE_BYTES + 1, 0x01);

    EXPECT_THROW(
        validator.validate_chunk(oversized.data(), oversized.size(), 1, 0),
        StreamValidationError);
}

TEST(VoiceStreamValidation, NonSequentialChunkRejected)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    const std::array<uint8_t, 8> chunk = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_NO_THROW(validator.validate_chunk(chunk.data(), chunk.size(), 1, 100));

    EXPECT_THROW(
        validator.validate_chunk(chunk.data(), chunk.size(), 3, 200),
        StreamValidationError);
}

TEST(VoiceStreamValidation, MalformedChunkRejected)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    std::vector<uint8_t> all_zero_chunk(256, 0x00);

    EXPECT_THROW(
        validator.validate_chunk(all_zero_chunk.data(), all_zero_chunk.size(), 1, 100),
        StreamValidationError);
}

TEST(VoiceStreamValidation, ExceededDurationRejected)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    const std::array<uint8_t, 8> chunk = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint64_t over_limit_ms =
        static_cast<uint64_t>(StreamValidationPolicy::MAX_STREAM_DURATION_SECONDS + 1) * 1000ULL;

    EXPECT_THROW(
        validator.validate_chunk(chunk.data(), chunk.size(), 1, over_limit_ms),
        StreamValidationError);
}

TEST(VoiceStreamValidation, FinalChunkMarksStreamComplete)
{
    VoiceStreamValidator validator("session-001", 16000, 1, 16);
    const std::array<uint8_t, 8> chunk = {1, 2, 3, 4, 5, 6, 7, 8};

    ASSERT_NO_THROW(validator.validate_chunk(chunk.data(), chunk.size(), 1, 100, true));
    EXPECT_TRUE(validator.is_complete());
    EXPECT_THROW(
        validator.validate_chunk(chunk.data(), chunk.size(), 2, 200),
        StreamValidationError);
}

TEST(VoiceStreamValidation, ConstructorRejectsInvalidConfig)
{
    EXPECT_THROW(VoiceStreamValidator("", 16000, 1, 16), std::invalid_argument);
    EXPECT_THROW(VoiceStreamValidator("session-001", 12345, 1, 16), std::invalid_argument);
    EXPECT_THROW(VoiceStreamValidator("session-001", 16000, 0, 16), std::invalid_argument);
    EXPECT_THROW(VoiceStreamValidator("session-001", 16000, 1, 7), std::invalid_argument);
}

}  // namespace test
}  // namespace voice
}  // namespace themis
