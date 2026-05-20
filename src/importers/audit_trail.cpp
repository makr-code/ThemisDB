/*
 * ThemisDB | File: audit_trail.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 93/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=21, M=16, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "importers/audit_trail.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <functional>
#include <openssl/evp.h>

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
    // Deterministic serialisation
    std::ostringstream ss;
    ss << prev_hash
       << eventTypeToString(event.type)
       << event.timestamp
       << event.user_principal
       << event.importer_instance_id
       << event.correlation_id
       << event.details.dump();
    const std::string payload = ss.str();

    // SHA-256 via OpenSSL EVP (collision-resistant, suitable for audit chains)
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, payload.data(), payload.size());
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Return first 32 hex chars (128-bit prefix) – same width as the old 16-char placeholder
    std::ostringstream hex;
    for (unsigned int i = 0; i < digest_len; ++i) {
        hex << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(digest[i]);
    }
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
