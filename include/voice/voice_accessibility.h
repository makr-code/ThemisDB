/**
 * @file voice_accessibility.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Closed-captions generation and accessible transcript export – Phase 9
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Caption format
enum class CaptionFormat {
    VTT,        // WebVTT (.vtt) – browser standard
    SRT,        // SubRip (.srt) – universal subtitle
    PLAIN_TEXT, // Flat text with speaker labels
    HTML,       // HTML with accessibility attributes
    JSON        // Structured JSON
};
std::string captionFormatToString(CaptionFormat fmt);

// A single timed caption cue
struct CaptionCue {
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    std::string text;
    std::string speaker;   // empty if not known
    int sequence = 0;
    float confidence = 1.0f;
    json metadata;
};

// Caption style options
struct CaptionStyle {
    int max_chars_per_line = 42;     // Broadcast standard
    int max_lines = 2;
    bool word_wrap = true;
    bool include_speaker_labels = true;
    bool include_confidence = false; // annotations
    int min_duration_ms = 1000;      // Merge cues shorter than this
    int max_duration_ms = 7000;      // Split cues longer than this
};

// Accessible transcript options
struct TranscriptExportOptions {
    CaptionFormat format = CaptionFormat::PLAIN_TEXT;
    CaptionStyle style;
    std::string language = "en";
    bool include_timestamps = true;
    bool include_speaker_info = true;
    bool include_confidence_scores = false;
    std::string title;
    std::string description;
    json custom_metadata;
};

// Export result
struct TranscriptExportResult {
    bool success = false;
    std::string error_message;
    std::string content;        // Serialized output
    CaptionFormat format;
    size_t cue_count = 0;
    int64_t total_duration_ms = 0;
    std::string mime_type;
};

// VoiceAccessibility: Phase 9 production component
/** @brief VoiceAccessibility: Phase 9 production component. */
class VoiceAccessibility {
public:
    explicit VoiceAccessibility(const CaptionStyle& style = {});
    ~VoiceAccessibility() = default;

    // Generate closed captions from segment list
    std::vector<CaptionCue> generateCaptions(
        const std::vector<std::pair<int64_t, std::string>>& timed_segments,
        const std::string& speaker = ""
    );

    // Generate captions from JSON transcript (as produced by STT processor)
    std::vector<CaptionCue> generateCaptionsFromJSON(const json& transcript_json);

    // Export captions to a specific format string
    TranscriptExportResult exportTranscript(
        const std::vector<CaptionCue>& cues,
        const TranscriptExportOptions& options = {}
    );

    // Format helpers (public for testing)
    std::string formatAsVTT(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    std::string formatAsSRT(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    std::string formatAsPlainText(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    std::string formatAsHTML(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    std::string formatAsJSONString(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;

    // Caption cue helpers
    std::vector<CaptionCue> mergeSortCues(const std::vector<CaptionCue>& cues) const;
    std::string formatTimestamp(int64_t ms, bool vtt_style = true) const; // HH:MM:SS.mmm or HH:MM:SS,mmm
    std::vector<CaptionCue> splitLongCues(const std::vector<CaptionCue>& cues) const;
    std::vector<CaptionCue> mergeSilentGaps(const std::vector<CaptionCue>& cues) const;

    // Statistics
    json getStatistics() const;

private:
    CaptionStyle style_;
    uint64_t exports_completed_ = 0;
    uint64_t total_cues_generated_ = 0;

    std::string wrapText(const std::string& text, int max_chars) const;
};

}} // namespace themis::voice
