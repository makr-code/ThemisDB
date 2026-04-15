/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audit_trail.cpp                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:16:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/audit_trail.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <functional>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// eventTypeToString
// ---------------------------------------------------------------------------

std::string AuditedImporter::eventTypeToString(EventType t) {
    switch (t) {
        case EventType::IMPORT_STARTED:       return "IMPORT_STARTED";
        case EventType::SCHEMA_ANALYZED:      return "SCHEMA_ANALYZED";
        case EventType::RELATIONSHIP_MAPPED:  return "RELATIONSHIP_MAPPED";
        case EventType::DATA_VALIDATED:       return "DATA_VALIDATED";
        case EventType::CONFLICT_RESOLVED:    return "CONFLICT_RESOLVED";
        case EventType::RECORD_IMPORTED:      return "RECORD_IMPORTED";
        case EventType::IMPORT_COMPLETED:     return "IMPORT_COMPLETED";
        case EventType::ERROR_OCCURRED:       return "ERROR_OCCURRED";
        default:                              return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// ImmutableAuditLog – private hash helper
// ---------------------------------------------------------------------------

std::string AuditedImporter::ImmutableAuditLog::computeEventHash(
    const AuditEvent& event,
    const std::string& prev_hash) const
{
    // Deterministic serialisation → std::hash (portable; NOT cryptographic)
    // Production builds replace this with OpenSSL SHA-256.
    std::ostringstream ss;
    ss << prev_hash
       << eventTypeToString(event.type)
       << event.timestamp
       << event.user_principal
       << event.importer_instance_id
       << event.correlation_id
       << event.details.dump();

    std::size_t h = std::hash<std::string>{}(ss.str());
    // Return as hex string
    std::ostringstream hex;
    hex << std::hex << std::setw(16) << std::setfill('0') << h;
    return hex.str();
}

// ---------------------------------------------------------------------------
// ImmutableAuditLog – public API
// ---------------------------------------------------------------------------

void AuditedImporter::ImmutableAuditLog::recordEvent(const AuditEvent& event) {
    std::string prev = chain_hashes_.empty() ? "0000000000000000" : chain_hashes_.back();
    std::string hash = computeEventHash(event, prev);
    events_.push_back(event);
    chain_hashes_.push_back(hash);
}

bool AuditedImporter::ImmutableAuditLog::verifyIntegrity() const {
    if (events_.size() != chain_hashes_.size()) return false;
    std::string prev = "0000000000000000";
    for (size_t i = 0; i < events_.size(); ++i) {
        std::string expected = computeEventHash(events_[i], prev);
        if (expected != chain_hashes_[i]) return false;
        prev = chain_hashes_[i];
    }
    return true;
}

json AuditedImporter::ImmutableAuditLog::exportForSIEM(
    const std::string& format) const
{
    json arr = json::array();
    for (size_t i = 0; i < events_.size(); ++i) {
        const auto& e = events_[i];
        json entry = {
            {"event_type",           eventTypeToString(e.type)},
            {"timestamp",            e.timestamp},
            {"user_principal",       e.user_principal},
            {"importer_instance_id", e.importer_instance_id},
            {"details",              e.details},
            {"correlation_id",       e.correlation_id},
            {"chain_hash",           chain_hashes_[i]}
        };

        if (format == "splunk") {
            // Splunk HEC format
            entry = json{
                {"time",       e.timestamp},
                {"source",     "themisdb_importer"},
                {"sourcetype", "themisdb:audit"},
                {"event",      entry}
            };
        } else if (format == "elk") {
            // ELK / Elastic common schema prefix
            entry["@timestamp"] = e.timestamp;
        }

        arr.push_back(std::move(entry));
    }
    return arr;
}

size_t AuditedImporter::ImmutableAuditLog::size() const {
    return events_.size();
}

const std::vector<AuditedImporter::AuditEvent>&
AuditedImporter::ImmutableAuditLog::events() const {
    return events_;
}

} // namespace importers
} // namespace themis
