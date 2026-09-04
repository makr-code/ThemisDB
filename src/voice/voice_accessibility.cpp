/**
 * @file voice_accessibility.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_accessibility.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cctype>

namespace themis { namespace voice {

// ---- Free functions ----

std::string captionFormatToString(CaptionFormat fmt) {
    switch (fmt) {
        case CaptionFormat::VTT:        return "vtt";
        case CaptionFormat::SRT:        return "srt";
        case CaptionFormat::PLAIN_TEXT: return "plain_text";
        case CaptionFormat::HTML:       return "html";
        case CaptionFormat::JSON:       return "json";
        default:                        return "unknown";
    }
}

// ---- VoiceAccessibility ----

VoiceAccessibility::VoiceAccessibility(const CaptionStyle& style)
    : style_(style) {}

std::vector<CaptionCue> VoiceAccessibility::generateCaptions(
    const std::vector<std::pair<int64_t, std::string>>& timed_segments,
    const std::string& speaker)
{
    std::vector<CaptionCue> cues = {};

    cues.reserve(timed_segments.size());

    for (size_t i = 0; i < timed_segments.size(); ++i) {
        CaptionCue cue;
        cue.start_ms = timed_segments[i].first;
        // End time: next segment start or start + 3000ms
        if (i + 1 < timed_segments.size()) {
            cue.end_ms = timed_segments[i + 1].first;
        } else {
            cue.end_ms = timed_segments[i].first + 3000;
        }
        cue.text = timed_segments[i].second;
        cue.speaker = speaker;
        cue.sequence = static_cast<int>(i + 1);
        cue.confidence = 1.0f;
        cues.push_back(std::move(cue));
    }

    total_cues_generated_ += cues.size();
    return cues;
}

std::vector<CaptionCue> VoiceAccessibility::generateCaptionsFromJSON(const json& transcript_json) {
    std::vector<CaptionCue> cues;

    if (!transcript_json.is_array()) {
        return cues;
    }

    int seq = 1;
    for (const auto& item : transcript_json) {
        CaptionCue cue;
        cue.sequence = seq++;
        if (item.contains("text") && item["text"].is_string())
            cue.text = item["text"].get<std::string>();
        if (item.contains("start_ms") && item["start_ms"].is_number())
            cue.start_ms = item["start_ms"].get<int64_t>();
        if (item.contains("end_ms") && item["end_ms"].is_number())
            cue.end_ms = item["end_ms"].get<int64_t>();
        if (item.contains("speaker") && item["speaker"].is_string())
            cue.speaker = item["speaker"].get<std::string>();
        if (item.contains("confidence") && item["confidence"].is_number())
            cue.confidence = item["confidence"].get<float>();
        cues.push_back(std::move(cue));
    }

    total_cues_generated_ += cues.size();
    return cues;
}

TranscriptExportResult VoiceAccessibility::exportTranscript(
    const std::vector<CaptionCue>& cues,
    const TranscriptExportOptions& options)
{
    TranscriptExportResult result;
    result.format = options.format;
    result.cue_count = cues.size();

    if (!cues.empty()) {
        auto sorted = mergeSortCues(cues);
        result.total_duration_ms = sorted.back().end_ms;
    }

    try {
        switch (options.format) {
            case CaptionFormat::VTT:
                result.content = formatAsVTT(cues, options);
                result.mime_type = "text/vtt";
                break;
            case CaptionFormat::SRT:
                result.content = formatAsSRT(cues, options);
                result.mime_type = "application/x-subrip";
                break;
            case CaptionFormat::PLAIN_TEXT:
                result.content = formatAsPlainText(cues, options);
                result.mime_type = "text/plain";
                break;
            case CaptionFormat::HTML:
                result.content = formatAsHTML(cues, options);
                result.mime_type = "text/html";
                break;
            case CaptionFormat::JSON:
                result.content = formatAsJSONString(cues, options);
                result.mime_type = "application/json";
                break;
        }
        result.success = true;
        ++exports_completed_;
    } catch (const std::exception& ex) {
        result.success = false;
        result.error_message = ex.what();
    }

    return result;
}

std::string VoiceAccessibility::formatAsVTT(
    const std::vector<CaptionCue>& cues,
    const TranscriptExportOptions& opts) const
{
    std::ostringstream ss = {};
    ss << "WEBVTT\n\n";

    auto sorted = mergeSortCues(cues);
    int seq = 1;
    for (const auto& cue : sorted) {
        ss << seq++ << "\n";
        ss << formatTimestamp(cue.start_ms, true) << " --> " << formatTimestamp(cue.end_ms, true) << "\n";
        if (opts.include_speaker_info && !cue.speaker.empty()) {
            ss << "<v " << cue.speaker << ">" << cue.text << "\n";
        } else {
            ss << cue.text << "\n";
        }
        ss << "\n";
    }
    return ss.str();
}

std::string VoiceAccessibility::formatAsSRT(
    const std::vector<CaptionCue>& cues,
    [[maybe_unused]] const TranscriptExportOptions& opts) const
{
    std::ostringstream ss = {};

    auto sorted = mergeSortCues(cues);
    int seq = 1;
    for (const auto& cue : sorted) {
        ss << seq++ << "\n";
        ss << formatTimestamp(cue.start_ms, false) << " --> " << formatTimestamp(cue.end_ms, false) << "\n";
        ss << cue.text << "\n";
        ss << "\n";
    }
    return ss.str();
}

std::string VoiceAccessibility::formatAsPlainText(
    const std::vector<CaptionCue>& cues,
    const TranscriptExportOptions& opts) const
{
    std::ostringstream ss = {};

    if (!opts.title.empty()) {
        ss << opts.title << "\n";
        ss << std::string(opts.title.size(), '=') << "\n\n";
    }

    auto sorted = mergeSortCues(cues);
    for (const auto& cue : sorted) {
        if (opts.include_timestamps) {
            // Format as [HH:MM:SS]
            int64_t total_s = cue.start_ms / 1000;
            int hh = static_cast<int>(total_s / 3600);
            int mm = static_cast<int>((total_s % 3600) / 60);
            int ss_val = static_cast<int>(total_s % 60);
            ss << "[";
            ss << std::setfill('0') << std::setw(2) << hh << ":"
               << std::setw(2) << mm << ":"
               << std::setw(2) << ss_val;
            ss << "] ";
        }
        if (opts.include_speaker_info && !cue.speaker.empty()) {
            ss << cue.speaker << ": ";
        }
        ss << cue.text << "\n";
    }
    return ss.str();
}

std::string VoiceAccessibility::formatAsHTML(
    const std::vector<CaptionCue>& cues,
    const TranscriptExportOptions& opts) const
{
    std::ostringstream ss = {};
    ss << "<!DOCTYPE html>\n";
    ss << "<html lang=\"" << opts.language << "\">\n";
    ss << "<head><meta charset=\"UTF-8\">";
    if (!opts.title.empty()) {
        ss << "<title>" << opts.title << "</title>";
    }
    ss << "</head>\n<body>\n";
    ss << "<article lang=\"" << opts.language << "\">\n";

    auto sorted = mergeSortCues(cues);
    for (const auto& cue : sorted) {
        ss << "<p>";
        if (opts.include_timestamps) {
            ss << "<time datetime=\"" << formatTimestamp(cue.start_ms, true) << "\">"
               << formatTimestamp(cue.start_ms, true) << "</time> ";
        }
        if (opts.include_speaker_info && !cue.speaker.empty()) {
            ss << "<b>" << cue.speaker << "</b>: ";
        }
        ss << cue.text << "</p>\n";
    }

    ss << "</article>\n</body>\n</html>\n";
    return ss.str();
}

std::string VoiceAccessibility::formatAsJSONString(
    const std::vector<CaptionCue>& cues,
    const TranscriptExportOptions& opts) const
{
    auto sorted = mergeSortCues(cues);
    json arr = json::array();
    for (const auto& cue : sorted) {
        json obj;
        obj["sequence"] = cue.sequence;
        obj["start_ms"] = cue.start_ms;
        obj["end_ms"] = cue.end_ms;
        obj["text"] = cue.text;
        if (!cue.speaker.empty()) {
            obj["speaker"] = cue.speaker;
        }
        if (opts.include_confidence_scores) {
            obj["confidence"] = cue.confidence;
        }
        arr.push_back(obj);
    }

    json result = {};
    if (!opts.title.empty()) {
      result["title"] = opts.title;
    }
    result["language"] = opts.language;
    result["cues"] = arr;
    return result.dump(2);
}

std::vector<CaptionCue> VoiceAccessibility::mergeSortCues(const std::vector<CaptionCue>& cues) const {
    std::vector<CaptionCue> sorted = cues;
    std::sort(sorted.begin(), sorted.end(), [](const CaptionCue& a, const CaptionCue& b) {
        return a.start_ms < b.start_ms;
    });

    // Fix overlapping timestamps: ensure end_ms >= start_ms + 1 and no overlap with next
    for (size_t i = 0; i < sorted.size(); ++i) {
        static constexpr int64_t DEFAULT_MIN_CUE_DURATION_MS = 1000;
        if (sorted[i].end_ms <= sorted[i].start_ms) {
            sorted[i].end_ms = sorted[i].start_ms + DEFAULT_MIN_CUE_DURATION_MS;
        }
        if (i + 1 < sorted.size() && sorted[i].end_ms > sorted[i + 1].start_ms) {
            sorted[i].end_ms = sorted[i + 1].start_ms;
        }
        sorted[i].sequence = static_cast<int>(i + 1);
    }
    return sorted;
}

std::string VoiceAccessibility::formatTimestamp(int64_t ms, bool vtt_style) const {
    int64_t total_ms = ms < 0 ? 0 : ms;
    int hh = static_cast<int>(total_ms / 3600000);
    int mm = static_cast<int>((total_ms % 3600000) / 60000);
    int ss = static_cast<int>((total_ms % 60000) / 1000);
    int frac = static_cast<int>(total_ms % 1000);

    char sep = vtt_style ? '.' : ',';
    std::ostringstream out = {};
    out << std::setfill('0')
        << std::setw(2) << hh << ":"
        << std::setw(2) << mm << ":"
        << std::setw(2) << ss << sep
        << std::setw(3) << frac;
    return out.str();
}

std::vector<CaptionCue> VoiceAccessibility::splitLongCues(const std::vector<CaptionCue>& cues) const {
    std::vector<CaptionCue> result = {};

    for (const auto& cue : cues) {
        int64_t duration = cue.end_ms - cue.start_ms;
        if (duration <= style_.max_duration_ms) {
            result.push_back(cue);
            continue;
        }

        // Split at word boundaries
        std::istringstream iss(cue.text);
        std::vector<std::string> words;
        std::string word = {};
        while (iss >> word) {
          words.push_back(word);
        }

        if (words.empty()) {
            result.push_back(cue);
            continue;
        }

        int parts = static_cast<int>((duration + style_.max_duration_ms - 1) / style_.max_duration_ms);
        size_t words_per_part = (static_cast<int>(words.size()) + static_cast<size_t>(parts) - 1) / static_cast<size_t>(parts);
        int64_t ms_per_part = duration / parts;

        for (int p = 0; p < parts; ++p) {
            CaptionCue sub;
            sub.start_ms = cue.start_ms + static_cast<int64_t>(p) * ms_per_part;
            sub.end_ms   = (p + 1 == parts) ? cue.end_ms : sub.start_ms + ms_per_part;
            sub.speaker  = cue.speaker;
            sub.confidence = cue.confidence;

            size_t from = static_cast<size_t>(p) * words_per_part;
            size_t to   = std::min(from + words_per_part, words.size());
            std::ostringstream txt = {};
            for (size_t w = from; w < to; ++w) {
                if (w > from) {
                  txt << " ";
                }
                txt << words[w];
            }
            sub.text = txt.str();
            sub.sequence = static_cast<int>(result.size() + 1);
            result.push_back(sub);
        }
    }
    return result;
}

std::vector<CaptionCue> VoiceAccessibility::mergeSilentGaps(const std::vector<CaptionCue>& cues) const {
    if (cues.empty()) {
      return cues;
    }

    auto sorted = mergeSortCues(cues);
    std::vector<CaptionCue> result;
    result.push_back(sorted[0]);

    for (size_t i = 1; i < sorted.size(); ++i) {
        auto& last = result.back();
        int64_t gap = sorted[i].start_ms - last.end_ms;
        // Merge if gap is small and same speaker
        if (gap < style_.min_duration_ms && last.speaker == sorted[i].speaker) {
            last.text += " " + sorted[i].text;
            last.end_ms = sorted[i].end_ms;
        } else {
            result.push_back(sorted[i]);
        }
    }

    // Renumber sequences
    for (size_t i = 0; i < result.size(); ++i) {
        result[i].sequence = static_cast<int>(i + 1);
    }
    return result;
}

std::string VoiceAccessibility::wrapText(const std::string& text, int max_chars) const {
    if (static_cast<int>(text.size()) <= max_chars) {
      return text;
    }

    std::ostringstream wrapped = {};
    size_t start = 0;
    while (static_cast<size_t>(start) < text.size()) {
        size_t end = start + static_cast<size_t>(max_chars);
        if (end >= static_cast<int>(text.size())) {
            wrapped << text.substr(start);
            break;
        }
        // Walk back to last space
        size_t space = text.rfind(' ', end);
        if (space == std::string::npos || space <= start) {
            space = end;
        }
        wrapped << text.substr(start, space - start) << "\n";
        start = space + 1;
    }
    return wrapped.str();
}

json VoiceAccessibility::getStatistics() const {
    json stats;
    stats["exports_completed"] = exports_completed_;
    stats["total_cues_generated"] = total_cues_generated_;
    return stats;
}

}} // namespace themis::voice

