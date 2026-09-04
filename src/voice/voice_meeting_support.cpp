/**
 * @file voice_meeting_support.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "voice/voice_meeting_support.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <sstream>
#include <chrono>
#include <random>
#include <mutex>

namespace themis { namespace voice {

std::string meetingSegmentTypeToString(MeetingSegmentType t) {
    switch (t) {
        case MeetingSegmentType::AGENDA_ITEM:  return "agenda_item";
        case MeetingSegmentType::DECISION:     return "decision";
        case MeetingSegmentType::ACTION_ITEM:  return "action_item";
        case MeetingSegmentType::DISCUSSION:   return "discussion";
        case MeetingSegmentType::INTRODUCTION: return "introduction";
        case MeetingSegmentType::CLOSING:      return "closing";
        default:                               return "other";
    }
}

VoiceMeetingSupport::VoiceMeetingSupport(const MeetingSupportConfig& config)
    : config_(config)
{}

std::string VoiceMeetingSupport::toLower(const std::string& s) const {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
      out += static_cast<char>(std::tolower(c));
    }
    return out;
}

bool VoiceMeetingSupport::containsTrigger(
    const std::string& text,
    const std::vector<std::string>& triggers) const
{
    std::string lower = toLower(text);
    for (const auto& t : triggers) {
        if (lower.find(toLower(t)) != std::string::npos) {
          return true;
        }
    }
    return false;
}

std::vector<std::string> VoiceMeetingSupport::tokenizeSentences(const std::string& text) const {
    std::vector<std::string> sentences;
    std::string current = {};
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        current += c;
        if (c == '.' || c == '!' || c == '?') {
            // Look ahead: if next non-space is uppercase or end, split
            size_t j = i + 1;
            while (j < text.size() && text[j] == ' ') {
              ++j;
            }
            if (j >= text.size() || std::isupper(static_cast<unsigned char>(text[j]))) {
                // trim
                while (!current.empty() && std::isspace(static_cast<unsigned char>(current.front())))
                    current.erase(current.begin());
                if (!current.empty()) {
                  sentences.push_back(current);
                }
                current.clear();
            }
        }
    }
    if (!current.empty()) {
        while (!current.empty() && std::isspace(static_cast<unsigned char>(current.front())))
            current.erase(current.begin());
        if (!current.empty()) {
          sentences.push_back(current);
        }
    }
    return sentences;
}

MeetingSegmentType VoiceMeetingSupport::classifySegment(const std::string& text) const {
    std::string lower = toLower(text);

    // Check action item triggers
    if (containsTrigger(text, config_.action_item_triggers)) {
        return MeetingSegmentType::ACTION_ITEM;
    }
    // Check decision triggers
    if (containsTrigger(text, config_.decision_triggers)) {
        return MeetingSegmentType::DECISION;
    }
    // Agenda keywords
    if (lower.find("agenda:") != std::string::npos ||
        lower.find("topic:") != std::string::npos ||
        lower.find("next item") != std::string::npos ||
        lower.find("agenda item") != std::string::npos) {
        return MeetingSegmentType::AGENDA_ITEM;
    }
    // Closing keywords
    if (lower.find("thank you") != std::string::npos ||
        lower.find("goodbye") != std::string::npos ||
        lower.find("that's all") != std::string::npos ||
        lower.find("thats all") != std::string::npos ||
        lower.find("meeting is adjourned") != std::string::npos ||
        lower.find("see you next") != std::string::npos) {
        return MeetingSegmentType::CLOSING;
    }
    // Introduction keywords
    if (lower.find("welcome") != std::string::npos ||
        lower.find("let me introduce") != std::string::npos ||
        lower.find("first of all") != std::string::npos ||
        lower.find("good morning") != std::string::npos ||
        lower.find("good afternoon") != std::string::npos) {
        return MeetingSegmentType::INTRODUCTION;
    }
    return MeetingSegmentType::DISCUSSION;
}

std::string VoiceMeetingSupport::generateActionItemId() const {
    // Use system_clock for human-readable time component in IDs
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return "ai-" + std::to_string(now) + "-" + std::to_string(++counter);
}

std::string VoiceMeetingSupport::extractAssignee(
    const std::string& text,
    const std::vector<std::string>& known_participants) const
{
    std::string lower = toLower(text);

    // Check known participants
    for (const auto& p : known_participants) {
        std::string lp = toLower(p);
        if (lower.find(lp) != std::string::npos) {
            return p;
        }
    }

    // Simple heuristics: look for patterns like "assigned to X", "X will", "X should"
    static const std::vector<std::string> patterns = {
        "assigned to ", "assign to ", "by ", "will be done by "
    };
    for (const auto& pat : patterns) {
        size_t pos = lower.find(pat);
        if (pos != std::string::npos) {
            size_t start = pos + pat.size();
            // Read a word
            size_t end = start;
            while (end < text.size() && !std::isspace(static_cast<unsigned char>(text[end])) &&
                   text[end] != '.' && text[end] != ',') {
                ++end;
            }
            if (end > start) {
              return text.substr(start, end - start);
            }
        }
    }
    return {};
}

std::vector<ActionItem> VoiceMeetingSupport::extractActionItems(
    const std::string& transcript,
    const std::vector<std::string>& known_participants)
{
    std::vector<ActionItem> items;
    auto sentences = tokenizeSentences(transcript);
    for (const auto& sent : sentences) {
        if (items.size() >= config_.max_action_items) {
          break;
        }
        if (!containsTrigger(sent, config_.action_item_triggers)) {
          continue;
        }

        ActionItem ai;
        ai.id = generateActionItemId();
        ai.description = sent;
        ai.confidence = 0.7f;
        if (config_.auto_assign_action_items) {
            ai.assignee = extractAssignee(sent, known_participants);
        }
        items.push_back(std::move(ai));
    }
    action_items_extracted_ += items.size();
    return items;
}

std::vector<std::string> VoiceMeetingSupport::extractDecisions(
    const std::string& transcript) const
{
    std::vector<std::string> decisions;
    auto sentences = tokenizeSentences(transcript);
    for (const auto& sent : sentences) {
        if (containsTrigger(sent, config_.decision_triggers)) {
            decisions.push_back(sent);
        }
    }
    return decisions;
}

std::vector<std::string> VoiceMeetingSupport::extractKeyPoints(
    const std::string& transcript) const
{
    std::vector<std::string> points;
    auto sentences = tokenizeSentences(transcript);
    int count = 0;
    for (const auto& sent : sentences) {
        if (sent.size() < 20) continue; // Skip trivial sentences
        auto type = classifySegment(sent);
        if (type == MeetingSegmentType::DECISION ||
            type == MeetingSegmentType::AGENDA_ITEM) {
            points.push_back(sent);
            if (++count >= 3) {
              break;
            }
        }
    }
    return points;
}

std::map<std::string, size_t> VoiceMeetingSupport::computeSpeakerWordCounts(
    const std::vector<MeetingSegment>& segments) const
{
    std::map<std::string, size_t> counts = {};

    for (const auto& seg : segments) {
        if (seg.speaker.empty()) {
          continue;
        }
        // Count words by splitting on whitespace
        std::istringstream iss(seg.text);
        std::string word = {};
        size_t wc = 0;
        while (iss >> word) {
          ++wc;
        }
        counts[seg.speaker] += wc;
    }
    return counts;
}

MeetingProtocol VoiceMeetingSupport::analyzeTranscript(
    const std::string& transcript,
    const std::string& meeting_id,
    const std::vector<std::string>& known_participants)
{
    MeetingProtocol protocol;
    protocol.meeting_id = meeting_id.empty() ? generateActionItemId() : meeting_id;
    protocol.full_transcript = transcript;
    protocol.participants = known_participants;

    auto sentences = tokenizeSentences(transcript);
    for (const auto& sent : sentences) {
        MeetingSegment seg;
        seg.text = sent;
        seg.type = classifySegment(sent);
        seg.confidence = 0.7f;
        protocol.segments.push_back(seg);
    }

    protocol.action_items = extractActionItems(transcript, known_participants);
    protocol.decisions = extractDecisions(transcript);
    protocol.key_points = extractKeyPoints(transcript);
    protocol.speaker_word_counts = computeSpeakerWordCounts(protocol.segments);

    ++meetings_analyzed_;
    decisions_extracted_ += protocol.decisions.size();
    return protocol;
}

ComplianceRecord VoiceMeetingSupport::createComplianceRecord(
    const std::string& call_id,
    const std::string& call_type,
    const std::string& jurisdiction,
    bool consent_obtained,
    const std::string& consent_method) const
{
    ComplianceRecord rec;
    rec.call_id = call_id;
    rec.recording_type = call_type;
    rec.consent_obtained = consent_obtained;
    rec.consent_method = consent_method;
    rec.jurisdiction = jurisdiction;
    rec.record_timestamp_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    if (jurisdiction == "EU" || jurisdiction == "EEA") {
        rec.gdpr_compliant = consent_obtained;
        rec.ccpa_compliant = true;
        rec.retention_policy = "90days";
        rec.regulatory_flags = {{"gdpr", true}, {"ccpa", false}};
    } else if (jurisdiction == "CA") {
        // California (CCPA)
        rec.gdpr_compliant = false;
        rec.ccpa_compliant = consent_obtained;
        rec.retention_policy = "365days";
        rec.regulatory_flags = {{"gdpr", false}, {"ccpa", true}};
    } else {
        // Default US: CCPA is technically California-only, but consent is still recorded.
        // ccpa_compliant is set to match consent_obtained for general US record-keeping.
        rec.gdpr_compliant = false;
        rec.ccpa_compliant = consent_obtained;
        rec.retention_policy = "90days";
        rec.regulatory_flags = {{"gdpr", false}, {"ccpa", false}};
    }

    rec.disclosure_given = consent_obtained;
    return rec;
}

bool VoiceMeetingSupport::isCompliantForJurisdiction(
    const ComplianceRecord& record,
    const std::string& jurisdiction) const
{
    if (jurisdiction == "EU" || jurisdiction == "EEA") {
        return record.gdpr_compliant && record.consent_obtained;
    }
    if (jurisdiction == "CA") {
        return record.ccpa_compliant && record.consent_obtained;
    }
    // Default: consent required
    return record.consent_obtained;
}

json VoiceMeetingSupport::getStatistics() const {
    return {
        {"meetings_analyzed",    meetings_analyzed_},
        {"action_items_extracted", action_items_extracted_},
        {"decisions_extracted",  decisions_extracted_}
    };
}

// ============================================================
// RealtimeMeetingSession implementation
// ============================================================

RealtimeMeetingSession::RealtimeMeetingSession(
    const std::string& meeting_id,
    const std::vector<std::string>& participants,
    const MeetingSupportConfig& config,
    ActionItemCallback on_action_item)
    : config_(config)
    , support_(config)
    , on_action_item_(std::move(on_action_item))
{
    protocol_.meeting_id   = meeting_id.empty() ? support_.generateActionItemId() : meeting_id;
    protocol_.participants = participants;
}

void RealtimeMeetingSession::addSegment(
    const std::string& text,
    const std::string& speaker,
    int64_t start_ms,
    int64_t end_ms)
{
    if (text.empty()) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (finalized_) {
      return;
    }

    // Classify and append segment
    MeetingSegment seg;
    seg.text       = text;
    seg.speaker    = speaker;
    seg.start_ms   = start_ms;
    seg.end_ms     = end_ms;
    seg.type       = support_.classifySegment(text);
    seg.confidence = 0.7f;
    protocol_.segments.push_back(seg);

    // Append to full transcript (space-separated)
    if (!protocol_.full_transcript.empty()) {
      protocol_.full_transcript += ' ';
    }
    protocol_.full_transcript += text;

    // Extract action items from this segment on-the-fly
    if (seg.type == MeetingSegmentType::ACTION_ITEM ||
        support_.containsTrigger(text, config_.action_item_triggers))
    {
        if (protocol_.action_items.size() < config_.max_action_items) {
            ActionItem ai;
            ai.id               = support_.generateActionItemId();
            ai.description      = text;
            ai.confidence       = 0.7f;
            ai.source_speaker   = speaker;
            ai.source_timestamp_ms = start_ms;
            if (config_.auto_assign_action_items) {
                ai.assignee = support_.extractAssignee(text, protocol_.participants);
            }
            protocol_.action_items.push_back(ai);
            if (on_action_item_) {
              on_action_item_(protocol_.action_items.back());
            }
        }
    }

    // Extract decisions from this segment on-the-fly
    if (support_.containsTrigger(text, config_.decision_triggers)) {
        protocol_.decisions.push_back(text);
    }

    // Update speaker word counts
    if (!speaker.empty()) {
        std::istringstream iss(text);
        std::string word = {};
        while (iss >> word) {
          ++protocol_.speaker_word_counts[speaker];
        }
    }
}

MeetingProtocol RealtimeMeetingSession::getCurrentProtocol() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return protocol_;
}

MeetingProtocol RealtimeMeetingSession::finalize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!finalized_) {
        // Compute key points from the full transcript accumulated so far
        protocol_.key_points = support_.extractKeyPoints(protocol_.full_transcript);
        finalized_ = true;
    }
    return protocol_;
}

bool RealtimeMeetingSession::isFinalized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return finalized_;
}

size_t RealtimeMeetingSession::segmentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return protocol_.segments.size();
}

}} // namespace themis::voice
