/**
 * @file voice_accessibility.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
/**
 * @brief TBD: Describe captionFormatToString.
 * @param[in] fmt Input parameter.
 * @return Return value.
 */
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

    /**
     * @brief Generate captions from JSON transcript (as produced by STT processor)
     * @param[in] transcript_json Input parameter.
     * @return Return value.
     */
    std::vector<CaptionCue> generateCaptionsFromJSON(const json& transcript_json);

    // Export captions to a specific format string
    TranscriptExportResult exportTranscript(
        const std::vector<CaptionCue>& cues,
        const TranscriptExportOptions& options = {}
    );

    /**
     * @brief Format helpers (public for testing)
     * @param[in] cues Input parameter.
     * @param[in] opts Input parameter.
     * @return Return value.
     */
    std::string formatAsVTT(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    /**
     * @brief TBD: Describe formatAsSRT.
     * @param[in] cues Input parameter.
     * @param[in] opts Input parameter.
     * @return Return value.
     */
    std::string formatAsSRT(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    /**
     * @brief TBD: Describe formatAsPlainText.
     * @param[in] cues Input parameter.
     * @param[in] opts Input parameter.
     * @return Return value.
     */
    std::string formatAsPlainText(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    /**
     * @brief TBD: Describe formatAsHTML.
     * @param[in] cues Input parameter.
     * @param[in] opts Input parameter.
     * @return Return value.
     */
    std::string formatAsHTML(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;
    /**
     * @brief TBD: Describe formatAsJSONString.
     * @param[in] cues Input parameter.
     * @param[in] opts Input parameter.
     * @return Return value.
     */
    std::string formatAsJSONString(const std::vector<CaptionCue>& cues, const TranscriptExportOptions& opts) const;

    /**
     * @brief Caption cue helpers
     * @param[in] cues Input parameter.
     * @return Return value.
     */
    std::vector<CaptionCue> mergeSortCues(const std::vector<CaptionCue>& cues) const;
    std::string formatTimestamp(int64_t ms, bool vtt_style = true) const; // HH:MM:SS.mmm or HH:MM:SS,mmm
    /**
     * @brief TBD: Describe splitLongCues.
     * @param[in] cues Input parameter.
     * @return Return value.
     */
    std::vector<CaptionCue> splitLongCues(const std::vector<CaptionCue>& cues) const;
    /**
     * @brief TBD: Describe mergeSilentGaps.
     * @param[in] cues Input parameter.
     * @return Return value.
     */
    std::vector<CaptionCue> mergeSilentGaps(const std::vector<CaptionCue>& cues) const;

    /**
     * @brief Statistics
     * @return Return value.
     */
    json getStatistics() const;

private:
    CaptionStyle style_;
    uint64_t exports_completed_ = 0;
    uint64_t total_cues_generated_ = 0;

    /**
     * @brief TBD: Describe wrapText.
     * @param[in] text Input parameter.
     * @param[in] max_chars Input parameter.
     * @return Return value.
     */
    std::string wrapText(const std::string& text, int max_chars) const;
};

}} // namespace themis::voice
