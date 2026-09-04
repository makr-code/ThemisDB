/**
 * @file test_stt_diarization.cpp
 * @brief Unit tests for STTProcessor::diarizeSegments (multi-speaker diarization)
 *
 * Tests exercise the public static diarizeSegments() API which exposes the
 * k-means++ acoustic-feature clustering algorithm used internally by
 * performSpeakerDiarization().
 */

#include <gtest/gtest.h>
#include "content/stt_processor.h"

#include <cmath>
#include <nlohmann/json.hpp>
#include <numbers>
#include <set>
#include <vector>

using themis::content::STTProcessor;
using themis::content::TranscriptionSegment;

namespace {

// ---------------------------------------------------------------------------
// PCM helpers
// ---------------------------------------------------------------------------

// Generate a mono 16 kHz PCM float buffer (duration_ms milliseconds) filled
// with a sine wave at the given frequency and amplitude.
std::vector<float> makePcmFloat(int duration_ms,
                                float freq_hz,
                                float amplitude = 0.4f,
                                int sample_rate = 16000)
{
    const int n = (sample_rate * duration_ms) / 1000;
    std::vector<float> out(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        out[i] = amplitude * std::sin(2.0f * std::numbers::pi_v<float>
                                      * freq_hz
                                      * static_cast<float>(i)
                                      / static_cast<float>(sample_rate));
    }
    return out;
}

// Build a TranscriptionSegment with the given time span and no speaker ID.
TranscriptionSegment makeSegment(const std::string& text,
                                  int64_t start_ms,
                                  int64_t end_ms)
{
    TranscriptionSegment seg;
    seg.text       = text;
    seg.start_ms   = start_ms;
    seg.end_ms     = end_ms;
    seg.confidence = 1.0f;
    seg.speaker_id = -1;
    return seg;
}

// Concatenate two PCM float buffers.
std::vector<float> concat(const std::vector<float>& a, const std::vector<float>& b)
{
    std::vector<float> out = {};

    out.reserve(a.size() + b.size());
    out.insert(out.end(), a.begin(), a.end());
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

}  // namespace

// ===========================================================================
// Edge-case tests (no PCM / too few segments)
// ===========================================================================

TEST(SpeakerDiarization, EmptySegmentsReturnedUnchanged) {
    std::vector<TranscriptionSegment> segs;
    auto result = STTProcessor::diarizeSegments(segs, {});
    EXPECT_TRUE(result.empty());
}

TEST(SpeakerDiarization, EmptyPCMReturnedUnchanged) {
    std::vector<TranscriptionSegment> segs = {
        makeSegment("hello", 0, 1000),
        makeSegment("world", 1000, 2000)
    };
    auto result = STTProcessor::diarizeSegments(segs, {});
    ASSERT_EQ(result.size(), segs.size());
    // speaker_id must remain -1 (unchanged)
    for (const auto& s : result) {
        EXPECT_EQ(s.speaker_id, -1);
    }
}

TEST(SpeakerDiarization, SingleSegmentReturnedUnchanged) {
    auto pcm = makePcmFloat(2000, 440.0f);
    std::vector<TranscriptionSegment> segs = {makeSegment("only one", 0, 2000)};

    auto result = STTProcessor::diarizeSegments(segs, pcm);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].speaker_id, -1) << "single segment should not be modified";
}

// ===========================================================================
// Basic correctness tests
// ===========================================================================

// Two segments backed by identical PCM → same speaker cluster.
TEST(SpeakerDiarization, TwoIdenticalSegmentsGetSameSpeakerId) {
    // Both segments use the same 440 Hz sine wave.
    auto pcm = concat(makePcmFloat(1000, 440.0f), makePcmFloat(1000, 440.0f));
    std::vector<TranscriptionSegment> segs = {
        makeSegment("seg0", 0, 1000),
        makeSegment("seg1", 1000, 2000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, /*max_speakers=*/2);
    ASSERT_EQ(result.size(), 2u);

    // Both speaker_ids should be valid (>= 0).
    EXPECT_GE(result[0].speaker_id, 0);
    EXPECT_GE(result[1].speaker_id, 0);

    // Because the two windows are acoustically identical, k-means should
    // converge to the same cluster for both.
    EXPECT_EQ(result[0].speaker_id, result[1].speaker_id);
}

// Two segments with very different acoustic content should be assigned to
// different speaker clusters.
TEST(SpeakerDiarization, TwoDifferentSegmentsGetDifferentSpeakerIds) {
    // Speaker A: low-frequency tone (100 Hz); Speaker B: high-frequency (2000 Hz).
    auto pcmA = makePcmFloat(2000, 100.0f);
    auto pcmB = makePcmFloat(2000, 2000.0f);
    auto pcm  = concat(pcmA, pcmB);

    std::vector<TranscriptionSegment> segs = {
        makeSegment("speaker A", 0,    2000),
        makeSegment("speaker B", 2000, 4000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, /*max_speakers=*/2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_GE(result[0].speaker_id, 0);
    EXPECT_GE(result[1].speaker_id, 0);
    EXPECT_NE(result[0].speaker_id, result[1].speaker_id)
        << "acoustically very different segments should be assigned to different clusters";
}

// All speaker_ids must be in [0, k-1].
TEST(SpeakerDiarization, SpeakerIdsInValidRange) {
    const int k = 3;
    // Three segments with moderately different tones.
    auto pcm = concat(concat(makePcmFloat(1000, 200.0f),
                             makePcmFloat(1000, 800.0f)),
                      makePcmFloat(1000, 1600.0f));

    std::vector<TranscriptionSegment> segs = {
        makeSegment("seg0", 0,    1000),
        makeSegment("seg1", 1000, 2000),
        makeSegment("seg2", 2000, 3000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, k);
    ASSERT_EQ(result.size(), 3u);
    for (const auto& seg : result) {
        EXPECT_GE(seg.speaker_id, 0);
        EXPECT_LT(seg.speaker_id, k);
    }
}

// Segment count equals k → each segment should get a unique speaker ID.
TEST(SpeakerDiarization, SegmentCountEqualsKAssignsUniqueIds) {
    const int k = 4;
    std::vector<float> pcm;
    std::vector<TranscriptionSegment> segs = {};

    float freqs[] = {150.0f, 600.0f, 1200.0f, 2400.0f};
    int64_t t = 0;
    for (int i = 0; i < k; ++i) {
        auto chunk = makePcmFloat(1000, freqs[i]);
        pcm.insert(pcm.end(), chunk.begin(), chunk.end());
        segs.push_back(makeSegment("seg" + std::to_string(i), t, t + 1000));
        t += 1000;
    }

    auto result = STTProcessor::diarizeSegments(segs, pcm, k);
    ASSERT_EQ(result.size(), static_cast<size_t>(k));

    std::set<int> ids = {};

    for (const auto& seg : result) {
        EXPECT_GE(seg.speaker_id, 0);
        EXPECT_LT(seg.speaker_id, k);
        ids.insert(seg.speaker_id);
    }
    // All k speaker IDs should be represented (each segment is its own cluster).
    EXPECT_EQ(static_cast<int>(ids.size()), k);
}

// Text content of segments must be preserved after diarization.
TEST(SpeakerDiarization, SegmentTextPreserved) {
    auto pcm = concat(makePcmFloat(1000, 300.0f), makePcmFloat(1000, 1500.0f));
    std::vector<TranscriptionSegment> segs = {
        makeSegment("hello world",    0,    1000),
        makeSegment("another phrase", 1000, 2000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, 2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].text, "hello world");
    EXPECT_EQ(result[1].text, "another phrase");
}

// Timestamps must be preserved after diarization.
TEST(SpeakerDiarization, SegmentTimestampsPreserved) {
    auto pcm = concat(makePcmFloat(1500, 300.0f), makePcmFloat(1500, 1500.0f));
    std::vector<TranscriptionSegment> segs = {
        makeSegment("first",  0,    1500),
        makeSegment("second", 1500, 3000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, 2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].start_ms, 0);
    EXPECT_EQ(result[0].end_ms,   1500);
    EXPECT_EQ(result[1].start_ms, 1500);
    EXPECT_EQ(result[1].end_ms,   3000);
}

// max_speakers=1 forces all segments into cluster 0.
TEST(SpeakerDiarization, MaxSpeakersOneAllGetSpeaker0) {
    auto pcm = concat(makePcmFloat(1000, 440.0f), makePcmFloat(1000, 880.0f));
    std::vector<TranscriptionSegment> segs = {
        makeSegment("a", 0, 1000), makeSegment("b", 1000, 2000)
    };

    auto result = STTProcessor::diarizeSegments(segs, pcm, /*max_speakers=*/1);
    ASSERT_EQ(result.size(), 2u);
    for (const auto& seg : result) {
        EXPECT_EQ(seg.speaker_id, 0);
    }
}

// Segments with zero-length audio windows (start_ms == end_ms) should not crash.
TEST(SpeakerDiarization, ZeroLengthSegmentWindowDoesNotCrash) {
    auto pcm = makePcmFloat(2000, 440.0f);
    std::vector<TranscriptionSegment> segs = {
        makeSegment("empty",  500,  500),   // zero-length window
        makeSegment("normal", 1000, 2000)
    };

    EXPECT_NO_THROW({
        auto result = STTProcessor::diarizeSegments(segs, pcm, 2);
        EXPECT_EQ(result.size(), 2u);
    });
}

// Four alternating-speaker segments are assigned to exactly 2 speaker clusters.
TEST(SpeakerDiarization, FourSegmentsTwoAlternatingSpeakers) {
    // Alternate between 200 Hz (speaker A) and 1800 Hz (speaker B).
    std::vector<float> pcm;
    std::vector<TranscriptionSegment> segs = {};

    float freqs[] = {200.0f, 1800.0f, 200.0f, 1800.0f};
    int64_t t = 0;
    for (int i = 0; i < 4; ++i) {
        auto chunk = makePcmFloat(1000, freqs[i]);
        pcm.insert(pcm.end(), chunk.begin(), chunk.end());
        segs.push_back(makeSegment("seg" + std::to_string(i), t, t + 1000));
        t += 1000;
    }

    auto result = STTProcessor::diarizeSegments(segs, pcm, /*max_speakers=*/2);
    ASSERT_EQ(result.size(), 4u);

    // Segments 0 and 2 (same frequency) should have the same speaker_id.
    EXPECT_EQ(result[0].speaker_id, result[2].speaker_id)
        << "segments with identical 200 Hz tone should be the same speaker";

    // Segments 1 and 3 (same frequency) should have the same speaker_id.
    EXPECT_EQ(result[1].speaker_id, result[3].speaker_id)
        << "segments with identical 1800 Hz tone should be the same speaker";

    // The two groups should differ.
    EXPECT_NE(result[0].speaker_id, result[1].speaker_id)
        << "the two acoustically distinct speakers should get different IDs";
}

// ===========================================================================
// Integration: transcribe() with speaker_diarization option
// ===========================================================================

// Build a minimal valid 16-bit PCM WAV buffer at 16 kHz mono.
static std::vector<uint8_t> makeDiarizationTestWav(int duration_seconds,
                                                    int sample_rate = 16000)
{
    const int num_samples = duration_seconds * sample_rate;
    const int data_size   = num_samples * 2;

    std::vector<uint8_t> wav;
    wav.reserve(44 + static_cast<size_t>(data_size));

    auto pushU32 = [&](uint32_t v) {
        wav.push_back(static_cast<uint8_t>(v & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    };
    auto pushU16 = [&](uint16_t v) {
        wav.push_back(static_cast<uint8_t>(v & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    };

    wav.push_back('R'); wav.push_back('I'); wav.push_back('F'); wav.push_back('F');
    pushU32(static_cast<uint32_t>(36 + data_size));
    wav.push_back('W'); wav.push_back('A'); wav.push_back('V'); wav.push_back('E');
    wav.push_back('f'); wav.push_back('m'); wav.push_back('t'); wav.push_back(' ');
    pushU32(16);
    pushU16(1);
    pushU16(1);
    pushU32(static_cast<uint32_t>(sample_rate));
    pushU32(static_cast<uint32_t>(sample_rate * 2));
    pushU16(2);
    pushU16(16);
    wav.push_back('d'); wav.push_back('a'); wav.push_back('t'); wav.push_back('a');
    pushU32(static_cast<uint32_t>(data_size));

    for (int i = 0; i < num_samples; ++i) {
        float val = 0.3f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sample_rate);
        auto s = static_cast<int16_t>(val * 32767.0f);
        wav.push_back(static_cast<uint8_t>(s & 0xFF));
        wav.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    }
    return wav;
}

// Calling transcribe() with speaker_diarization=true must not crash and must
// return a valid (possibly placeholder) TranscriptionResult.
TEST(SpeakerDiarizationIntegration, TranscribeWithDiarizationOptionDoesNotCrash) {
    STTProcessor stt;
    themis::content::PluginConfig cfg;
    bool init_ok = stt.initialize(cfg);
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    auto wav = makeDiarizationTestWav(3);
    nlohmann::json opts;
    opts["speaker_diarization"] = true;

    EXPECT_NO_THROW({
        auto result = stt.transcribe(wav, opts);
        EXPECT_TRUE(result.success);
    });
}

// Without speaker_diarization the default (placeholder-path) segment keeps
// speaker_id == -1 (unchanged).
TEST(SpeakerDiarizationIntegration, SingleSegmentSpeakerIdUnchangedWithoutDiarization) {
    STTProcessor stt;
    themis::content::PluginConfig cfg;
    bool init_ok = stt.initialize(cfg);
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    auto wav    = makeDiarizationTestWav(2);
    auto result = stt.transcribe(wav);   // no speaker_diarization option
    ASSERT_TRUE(result.success);
    for (const auto& seg : result.segments) {
        EXPECT_EQ(seg.speaker_id, -1)
            << "diarization must not be applied without the option";
    }
}

// With speaker_diarization=true on a single-segment result (non-Whisper mode),
// the condition segments.size() >= 2 is false so diarization is skipped and
// speaker_id remains -1.
TEST(SpeakerDiarizationIntegration, SingleSegmentSpeakerIdUnchangedWithDiarization) {
    STTProcessor stt;
    themis::content::PluginConfig cfg;
    bool init_ok = stt.initialize(cfg);
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    auto wav = makeDiarizationTestWav(2);
    nlohmann::json opts;
    opts["speaker_diarization"] = true;

    auto result = stt.transcribe(wav, opts);
    ASSERT_TRUE(result.success);
    // In non-Whisper mode there is exactly one placeholder segment; diarization
    // requires at least two segments, so speaker_id must stay at -1.
    for (const auto& seg : result.segments) {
        EXPECT_EQ(seg.speaker_id, -1)
            << "diarization must be skipped when only one segment is present";
    }
}
