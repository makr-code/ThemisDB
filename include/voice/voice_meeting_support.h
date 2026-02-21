/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_meeting_support.h                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 14:07:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     189                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 6632e4e56  2026-02-21  Add License Portal and Renewal Reminder classes for ThemisDB ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Phone call & meeting support – Phase 4 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Action item with assignment
struct ActionItem {
    std::string id;
    std::string description;
    std::string assignee;           // Extracted or assigned name/email
    std::string due_date;           // Extracted or empty
    std::string status = "pending"; // pending/in_progress/done
    float confidence = 0.0f;        // Extraction confidence
    int64_t source_timestamp_ms = 0;// When in the meeting
    std::string source_speaker;     // Who assigned it
};

// Meeting structure type
enum class MeetingSegmentType {
    AGENDA_ITEM,
    DECISION,
    ACTION_ITEM,
    DISCUSSION,
    INTRODUCTION,
    CLOSING,
    OTHER
};
std::string meetingSegmentTypeToString(MeetingSegmentType t);

// Meeting transcript segment with structure
struct MeetingSegment {
    MeetingSegmentType type = MeetingSegmentType::OTHER;
    std::string text;
    std::string speaker;
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    float confidence = 0.0f;
    json metadata;
};

// Structured meeting protocol
struct MeetingProtocol {
    std::string meeting_id;
    std::string title;
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    std::string organizer;
    std::vector<std::string> participants;
    std::string full_transcript;

    std::vector<MeetingSegment> segments;
    std::vector<std::string> decisions;
    std::vector<std::string> key_points;
    std::vector<ActionItem> action_items;

    std::map<std::string, size_t> speaker_word_counts;
    json metadata;
};

// Call recording compliance record
struct ComplianceRecord {
    std::string call_id;
    std::string recording_type;       // "inbound", "outbound", "conference"
    bool consent_obtained = false;
    std::string consent_method;       // "verbal", "ivr", "pre_agreed"
    bool disclosure_given = false;    // "This call may be recorded..."
    std::string jurisdiction;         // "US", "EU", "UK", etc.
    int64_t record_timestamp_ms = 0;
    json regulatory_flags;
    bool gdpr_compliant = true;
    bool ccpa_compliant = true;
    std::string retention_policy;     // e.g. "90days", "7years"
};

// Meeting action item extractor config
struct MeetingSupportConfig {
    float action_item_confidence_threshold = 0.4f;
    float decision_confidence_threshold = 0.5f;
    bool auto_assign_action_items = true;     // try to extract assignee name
    bool detect_agenda_structure = true;
    size_t max_action_items = 50;
    std::vector<std::string> action_item_triggers = {
        "will", "should", "needs to", "must", "action:", "todo:", "follow up",
        "by next", "by end of", "assigned to", "take care of", "responsible for"
    };
    std::vector<std::string> decision_triggers = {
        "decided", "agreed", "resolved", "approved", "rejected",
        "we will", "we have decided", "the decision is", "consensus"
    };
};

// VoiceMeetingSupport: Phase 4 production component
class VoiceMeetingSupport {
public:
    explicit VoiceMeetingSupport(const MeetingSupportConfig& config = {});
    ~VoiceMeetingSupport() = default;

    // Meeting protocol generation from transcript
    MeetingProtocol analyzeTranscript(
        const std::string& transcript,
        const std::string& meeting_id = "",
        const std::vector<std::string>& known_participants = {}
    );

    // Segment classification (classify a text segment by type)
    MeetingSegmentType classifySegment(const std::string& text) const;

    // Action item extraction
    std::vector<ActionItem> extractActionItems(
        const std::string& transcript,
        const std::vector<std::string>& known_participants = {}
    );

    // Decision extraction
    std::vector<std::string> extractDecisions(const std::string& transcript) const;

    // Key points extraction (simple heuristic)
    std::vector<std::string> extractKeyPoints(const std::string& transcript) const;

    // Speaker statistics
    std::map<std::string, size_t> computeSpeakerWordCounts(
        const std::vector<MeetingSegment>& segments
    ) const;

    // Call recording compliance
    ComplianceRecord createComplianceRecord(
        const std::string& call_id,
        const std::string& call_type,
        const std::string& jurisdiction = "US",
        bool consent_obtained = false,
        const std::string& consent_method = "verbal"
    ) const;

    bool isCompliantForJurisdiction(const ComplianceRecord& record, const std::string& jurisdiction) const;

    // Try to extract assignee from action item text
    std::string extractAssignee(
        const std::string& text,
        const std::vector<std::string>& known_participants
    ) const;

    // Statistics
    json getStatistics() const;

private:
    MeetingSupportConfig config_;
    uint64_t meetings_analyzed_ = 0;
    uint64_t action_items_extracted_ = 0;
    uint64_t decisions_extracted_ = 0;

    std::vector<std::string> tokenizeSentences(const std::string& text) const;
    bool containsTrigger(const std::string& text, const std::vector<std::string>& triggers) const;
    std::string generateActionItemId() const;
    std::string toLower(const std::string& s) const;
};

}} // namespace themis::voice
