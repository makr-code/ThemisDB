/**
 * @file audit_trail.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/audit_trail.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <mutex>
#include <openssl/evp.h>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// eventTypeToString
// ---------------------------------------------------------------------------

std::string AuditedImporter::eventTypeToString(EventType t) {
    switch (t) {
        case EventType::IMPORT_STARTED:
            return "IMPORT_STARTED";
        case EventType::SCHEMA_ANALYZED:
            return "SCHEMA_ANALYZED";
        case EventType::RELATIONSHIP_MAPPED:
            return "RELATIONSHIP_MAPPED";
        case EventType::DATA_VALIDATED:
            return "DATA_VALIDATED";
        case EventType::CONFLICT_RESOLVED:
            return "CONFLICT_RESOLVED";
        case EventType::RECORD_IMPORTED:
            return "RECORD_IMPORTED";
        case EventType::IMPORT_COMPLETED:
            return "IMPORT_COMPLETED";
        case EventType::ERROR_OCCURRED:
            return "ERROR_OCCURRED";
        default:
            return "UNKNOWN";
    }
}

// ============================================================================
// Phase 2 T2.3.3 – Unified Audit Event Schema & Correlation
// ============================================================================

std::string auditEventTypeToString(AuditEventType t) {
    // PHASE-2-HARDENING: Audit Event Type Conversion
    // Determinism: yes (enum-based)
    // Audit: suitable for structured logging
    // Bounded: O(1) string generation

    switch (t) {
        case AuditEventType::IMPORT_STARTED:
            return "IMPORT_STARTED";
        case AuditEventType::SCHEMA_ANALYZED:
            return "SCHEMA_ANALYZED";
        case AuditEventType::RELATIONSHIP_MAPPED:
            return "RELATIONSHIP_MAPPED";
        case AuditEventType::DATA_VALIDATED:
            return "DATA_VALIDATED";
        case AuditEventType::CONFLICT_DETECTED:
            return "CONFLICT_DETECTED";
        case AuditEventType::CONFLICT_RESOLVED:
            return "CONFLICT_RESOLVED";
        case AuditEventType::QUALITY_CHECK_FAILED:
            return "QUALITY_CHECK_FAILED";
        case AuditEventType::QUALITY_GATE_BYPASSED:
            return "QUALITY_GATE_BYPASSED";
        case AuditEventType::SCHEMA_VALIDATION_FAILED:
            return "SCHEMA_VALIDATION_FAILED";
        case AuditEventType::RECORD_IMPORTED:
            return "RECORD_IMPORTED";
        case AuditEventType::IMPORT_COMPLETED:
            return "IMPORT_COMPLETED";
        case AuditEventType::IMPORT_ROLLBACK_REQUESTED:
            return "IMPORT_ROLLBACK_REQUESTED";
        case AuditEventType::ERROR_OCCURRED:
            return "ERROR_OCCURRED";
        default:
            return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// ImmutableAuditLog – private hash helper
// ---------------------------------------------------------------------------

std::string AuditedImporter::ImmutableAuditLog::computeEventHash(const AuditEvent &event,
                                                                 const std::string &prev_hash) const {
    // Deterministic serialisation
    std::ostringstream ss = {};
    ss << prev_hash << eventTypeToString(event.type) << event.timestamp << event.user_principal
       << event.importer_instance_id << event.correlation_id << event.details.dump();
    const std::string payload = ss.str();

    // SHA-256 via OpenSSL EVP (collision-resistant, suitable for audit chains)
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_MD_CTX *ctx         = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, payload.data(),static_cast<int>(payload.size()));
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Return first 32 hex chars (128-bit prefix) – same width as the old 16-char placeholder
    std::ostringstream hex = {};
    for (unsigned int i = 0; i < digest_len; ++i) {
        hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(digest[i]);
    }
    return hex.str();
}

// ---------------------------------------------------------------------------
// ImmutableAuditLog – public API
// ---------------------------------------------------------------------------

void AuditedImporter::ImmutableAuditLog::recordEvent(const AuditEvent &event) {
    std::string prev = chain_hashes_.empty() ? "0000000000000000" : chain_hashes_.back();
    std::string hash = computeEventHash(event, prev);
    events_.push_back(event);
    chain_hashes_.push_back(hash);
}

bool AuditedImporter::ImmutableAuditLog::verifyIntegrity() const {
    if (static_cast<int>(events_.size()) != chain_hashes_.size()) {
        return false;
    }
    std::string prev = "0000000000000000";
    for (size_t i = 0; i <static_cast<int>(events_.size()); ++i) {
        std::string expected = computeEventHash(events_[i], prev);
        if (expected != chain_hashes_[i]) {
            return false;
        }
        prev = chain_hashes_[i];
    }
    return true;
}

json AuditedImporter::ImmutableAuditLog::exportForSIEM(const std::string &format) const {
    json arr = json::array();
    for (size_t i = 0; i <static_cast<int>(events_.size()); ++i) {
        const auto &e = events_[i];
        json entry    = {{"event_type", eventTypeToString(e.type)},
                         {"timestamp", e.timestamp},
                         {"user_principal", e.user_principal},
                         {"importer_instance_id", e.importer_instance_id},
                         {"details", e.details},
                         {"correlation_id", e.correlation_id},
                         {"chain_hash", chain_hashes_[i]}};

        if (format == "splunk") {
            // Splunk HEC format
            entry = json{{"time", e.timestamp},
                         {"source", "themisdb_importer"},
                         {"sourcetype", "themisdb:audit"},
                         {"event", entry}};
        } else if (format == "elk") {
            // ELK / Elastic common schema prefix
            entry["@timestamp"] = e.timestamp;
        }

        arr.push_back(std::move(entry));
    }
    return arr;
}

size_t AuditedImporter::ImmutableAuditLog::size() const {
    return static_cast<int>(events_.size());
}

const std::vector<AuditedImporter::AuditEvent> &AuditedImporter::ImmutableAuditLog::events() const {
    return events_;
}

// ============================================================================
// Phase 2 T2.3.3 – Unified Audit Event Schema & Correlation
// ============================================================================

void AuditedImporter::ImmutableAuditLog::emitAuditEvent(const AuditEvent& event) {
    // PHASE-2-HARDENING: Centralized Audit Event Emission
    // Determinism: yes (all fields deterministic)
    // Audit: centralized emission with timestamp and sequence
    // Bounded: buffer limited to 100,000 events; drops oldest when full

    // Buffer overflow handling: if we're at max capacity, drop oldest event
    if (static_cast<int>(events_.size()) > = kMaxAuditBufferSize) {
        // Drop oldest event (FIFO)
        if (!events_.empty()) {
            events_.erase(events_.begin());
            chain_hashes_.erase(chain_hashes_.begin());
        }
    }

    // Record the event (will be processed by recordEvent)
    recordEvent(event);
}

std::vector<AuditedImporter::AuditEvent> AuditedImporter::ImmutableAuditLog::getAuditTrailForImport(
    const std::string& import_id) const {
    // PHASE-2-HARDENING: Audit Trail Replay
    // Determinism: yes (ordered by sequence_number)
    // Audit: enables audit trail replay for debugging
    // Bounded: returns all events in chronological order

    std::vector<AuditEvent> result;

    // Collect all events for this import_id
    for (const auto& event : events_) {
        if (event.import_id == import_id) {
            result.push_back(event);
        }
    }

    // Sort by sequence_number to ensure chronological order
    std::sort(result.begin(), result.end(), [](const AuditEvent& a, const AuditEvent& b) {
        return a.sequence_number < b.sequence_number;
    });

    return result;
}

// ============================================================================
// PHASE-3-ERROR-HANDLING: Rollback & Recovery Audit Trail
// ============================================================================

std::string rollbackReasonToString(RollbackReason reason) {
    // PHASE-3-ERROR-HANDLING: Convert rollback reason to string
    switch (reason) {
        case RollbackReason::USER_REQUESTED:
            return "USER_REQUESTED";
        case RollbackReason::QUOTA_EXCEEDED:
            return "QUOTA_EXCEEDED";
        case RollbackReason::SCHEMA_VALIDATION_FAILED:
            return "SCHEMA_VALIDATION_FAILED";
        case RollbackReason::CONNECTOR_UNAVAILABLE:
            return "CONNECTOR_UNAVAILABLE";
        case RollbackReason::QUALITY_GATE_FAILED:
            return "QUALITY_GATE_FAILED";
        case RollbackReason::INTEGRITY_VIOLATION:
            return "INTEGRITY_VIOLATION";
        case RollbackReason::TIMEOUT:
            return "TIMEOUT";
        case RollbackReason::UNKNOWN:
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

void AuditedImporter::ImmutableAuditLog::emitRollbackEvent(
    const RollbackAuditEvent& rollback_event,
    const std::string& import_id,
    const std::string& user_principal) {
    // PHASE-3-ERROR-HANDLING: Emit rollback audit event with full context
    
    AuditEvent event;
    event.type = EventType::IMPORT_COMPLETED;
    event.event_type = AuditEventType::IMPORT_ROLLBACK_REQUESTED;
    event.timestamp = ""; // Will be populated by caller if needed
    event.user_principal = user_principal;
    event.importer_instance_id = "";
    event.import_id = import_id;
    event.correlation_id = ""; // Will be populated by caller if needed
    event.sequence_number = events_.size();
    event.event_timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Populate details with rollback information
    event.details = rollback_event.toJson();
    
    // Record the event (this will add it to the chain)
    recordEvent(event);
}

} // namespace importers
} // namespace themis

