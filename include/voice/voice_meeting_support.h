/**
 * @file voice_meeting_support.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Phone call & meeting support – Phase 4 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
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

    friend class RealtimeMeetingSession;
};

// Callback invoked for each new action item extracted during a real-time session.
using ActionItemCallback = std::function<void(const ActionItem&)>;

/**
 * @brief Real-time meeting transcription session with incremental action-item extraction.
 *
 * Accepts transcript segments one-by-one as they arrive from a streaming STT
 * source and extracts action items on-the-fly.  The accumulated MeetingProtocol
 * is available at any point via getCurrentProtocol(), and finalize() returns the
 * completed protocol once the meeting has ended.
 *
 * Thread-safety: addSegment() and finalize() are protected by an internal mutex
 * and may be called from separate threads (e.g. an STT callback thread).
 */
class RealtimeMeetingSession {
public:
    /**
     * @param meeting_id   Unique identifier for this meeting session.
     * @param participants Known participant names for assignee extraction.
     * @param config       Shared MeetingSupportConfig (triggers, limits, etc.).
     * @param on_action_item  Optional callback invoked for each new ActionItem.
     */
    explicit RealtimeMeetingSession(
        const std::string& meeting_id = "",
        const std::vector<std::string>& participants = {},
        const MeetingSupportConfig& config = {},
        ActionItemCallback on_action_item = nullptr
    );
    ~RealtimeMeetingSession() = default;

    // Non-copyable, movable
    RealtimeMeetingSession(const RealtimeMeetingSession&) = delete;
    RealtimeMeetingSession& operator=(const RealtimeMeetingSession&) = delete;
    RealtimeMeetingSession(RealtimeMeetingSession&&) = default;
    RealtimeMeetingSession& operator=(RealtimeMeetingSession&&) = default;

    /**
     * @brief Push a new transcript segment into the session.
     *
     * The segment text is classified and, if it matches action-item or decision
     * triggers, the result is immediately extracted and appended to the protocol.
     * The optional ActionItemCallback is fired for each newly extracted item.
     *
     * @param text        Transcript text for this segment.
     * @param speaker     Speaker label (may be empty if diarization is unavailable).
     * @param start_ms    Segment start time in milliseconds (0 if unknown).
     * @param end_ms      Segment end time in milliseconds (0 if unknown).
     */
    void addSegment(
        const std::string& text,
        const std::string& speaker = "",
        int64_t start_ms = 0,
        int64_t end_ms = 0
    );

    /**
     * @brief Return the current (in-progress) meeting protocol snapshot.
     *
     * Thread-safe: may be called concurrently with addSegment().
     */
    MeetingProtocol getCurrentProtocol() const;

    /**
     * @brief Mark the session as complete and return the final MeetingProtocol.
     *
     * Runs extractKeyPoints() on the accumulated full_transcript and freezes
     * the session.  Subsequent calls to addSegment() after finalize() are
     * silently ignored.
     *
     * @return The completed MeetingProtocol.
     */
    MeetingProtocol finalize();

    /// Returns true once finalize() has been called.
    bool isFinalized() const;

    /// Total number of segments received so far.
    size_t segmentCount() const;

private:
    mutable std::mutex mutex_;
    MeetingSupportConfig config_;
    MeetingProtocol protocol_;
    bool finalized_ = false;

    // Reuse VoiceMeetingSupport helpers via composition.
    VoiceMeetingSupport support_;
    ActionItemCallback on_action_item_;
};

}} // namespace themis::voice
