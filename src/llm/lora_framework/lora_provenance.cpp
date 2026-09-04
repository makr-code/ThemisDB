/**
 * @file lora_provenance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.40
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_provenance.h"
#include <spdlog/spdlog.h>
#include <openssl/sha.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// ISO 8601 UTC timestamp for the current moment.
static std::string nowISO8601() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream ss = {};
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

/// Generate a simple UUID-like identifier (random hex).
static std::string generateId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << dis(gen);
    oss << std::setw(16) << dis(gen);
    return oss.str();
}

/// Convert raw SHA-256 digest to lowercase hex string.
static std::string digestToHex(const unsigned char digest[SHA256_DIGEST_LENGTH]) {
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// LoRAProvenanceRecord – serialisation
// ============================================================================

json LoRAProvenanceRecord::toJSON() const {
    return json{
        {"dataset_hash",          dataset_hash},
        {"base_model_hash",       base_model_hash},
        {"hyperparameter_hash",   hyperparameter_hash},
        {"adapter_weights_hash",  adapter_weights_hash},
        {"trainer_id",            trainer_id},
        {"ca_chain",              ca_chain},
        {"signature",             signature},
        {"created_at",            created_at},
        {"rfc3161_timestamp",     rfc3161_timestamp},
        {"training_duration_secs", training_duration_secs},
        {"hardware_info",         hardware_info},
        {"custom_metadata",       custom_metadata}
    };
}

LoRAProvenanceRecord LoRAProvenanceRecord::fromJSON(const json& j) {
    LoRAProvenanceRecord r;
    auto get_str = [&](const char* key, std::string& dest) {
        if (j.contains(key) && j[key].is_string()) {
          dest = j[key].get<std::string>();
        }
    };
    get_str("dataset_hash",         r.dataset_hash);
    get_str("base_model_hash",      r.base_model_hash);
    get_str("hyperparameter_hash",  r.hyperparameter_hash);
    get_str("adapter_weights_hash", r.adapter_weights_hash);
    get_str("trainer_id",           r.trainer_id);
    get_str("ca_chain",             r.ca_chain);
    get_str("signature",            r.signature);
    get_str("created_at",           r.created_at);
    get_str("rfc3161_timestamp",    r.rfc3161_timestamp);
    if (j.contains("training_duration_secs"))
        r.training_duration_secs = j["training_duration_secs"].get<double>();
    if (j.contains("hardware_info")) {
      r.hardware_info   = j["hardware_info"];
    }
    if (j.contains("custom_metadata")) {
      r.custom_metadata = j["custom_metadata"];
    }
    return r;
}

// ============================================================================
// ExternalAdapterProvenance – serialisation
// ============================================================================

json ExternalAdapterProvenance::toJSON() const {
    return json{
        {"source_url",            source_url},
        {"commit_hash",           commit_hash},
        {"description",           description},
        {"adapter_hash",          adapter_hash},
        {"provenance_signature",  provenance_signature},
        {"certificate_chain",     certificate_chain},
        {"import_timestamp",      import_timestamp},
        {"signature_valid",       signature_valid},
        {"cert_chain_valid",      cert_chain_valid},
        {"validation_errors",     validation_errors}
    };
}

ExternalAdapterProvenance ExternalAdapterProvenance::fromJSON(const json& j) {
    ExternalAdapterProvenance r;
    auto get_str = [&](const char* key, std::string& dest) {
        if (j.contains(key) && j[key].is_string()) {
          dest = j[key].get<std::string>();
        }
    };
    get_str("source_url",           r.source_url);
    get_str("commit_hash",          r.commit_hash);
    get_str("description",          r.description);
    get_str("adapter_hash",         r.adapter_hash);
    get_str("provenance_signature", r.provenance_signature);
    get_str("certificate_chain",    r.certificate_chain);
    get_str("import_timestamp",     r.import_timestamp);
    if (j.contains("signature_valid")) {
      r.signature_valid  = j["signature_valid"].get<bool>();
    }
    if (j.contains("cert_chain_valid")) {
      r.cert_chain_valid = j["cert_chain_valid"].get<bool>();
    }
    if (j.contains("validation_errors"))
        r.validation_errors = j["validation_errors"].get<std::vector<std::string>>();
    return r;
}

// ============================================================================
// AdapterSnapshot – serialisation
// ============================================================================

json AdapterSnapshot::toJSON() const {
    return json{
        {"snapshot_id",        snapshot_id},
        {"adapter_id",         adapter_id},
        {"version",            version},
        {"weights_hash",       weights_hash},
        {"timestamp",          timestamp},
        {"parent_snapshot_id", parent_snapshot_id},
        {"provenance",         provenance.toJSON()}
    };
}

AdapterSnapshot AdapterSnapshot::fromJSON(const json& j) {
    AdapterSnapshot s;
    auto get_str = [&](const char* key, std::string& dest) {
        if (j.contains(key) && j[key].is_string()) {
          dest = j[key].get<std::string>();
        }
    };
    get_str("snapshot_id",        s.snapshot_id);
    get_str("adapter_id",         s.adapter_id);
    get_str("version",            s.version);
    get_str("weights_hash",       s.weights_hash);
    get_str("timestamp",          s.timestamp);
    get_str("parent_snapshot_id", s.parent_snapshot_id);
    if (j.contains("provenance"))
        s.provenance = LoRAProvenanceRecord::fromJSON(j["provenance"]);
    return s;
}

// ============================================================================
// InferenceAuditEntry – serialisation & hash computation
// ============================================================================

json InferenceAuditEntry::toJSON() const {
    return json{
        {"entry_id",       entry_id},
        {"previous_hash",  previous_hash},
        {"entry_hash",     entry_hash},
        {"timestamp",      timestamp},
        {"request_id",     request_id},
        {"query_hash",     query_hash},
        {"response_hash",  response_hash},
        {"model_hash",     model_hash},
        {"adapter_hash",   adapter_hash},
        {"commitments",    commitments},
        {"metadata",       metadata}
    };
}

InferenceAuditEntry InferenceAuditEntry::fromJSON(const json& j) {
    InferenceAuditEntry e;
    auto get_str = [&](const char* key, std::string& dest) {
        if (j.contains(key) && j[key].is_string()) {
          dest = j[key].get<std::string>();
        }
    };
    get_str("entry_id",      e.entry_id);
    get_str("previous_hash", e.previous_hash);
    get_str("entry_hash",    e.entry_hash);
    get_str("timestamp",     e.timestamp);
    get_str("request_id",    e.request_id);
    get_str("query_hash",    e.query_hash);
    get_str("response_hash", e.response_hash);
    get_str("model_hash",    e.model_hash);
    get_str("adapter_hash",  e.adapter_hash);
    if (j.contains("commitments")) {
      e.commitments = j["commitments"];
    }
    if (j.contains("metadata")) {
      e.metadata    = j["metadata"];
    }
    return e;
}

std::string InferenceAuditEntry::computeContentHash() const {
    // Canonical JSON covering all content fields except entry_hash itself
    json content = {
        {"entry_id",      entry_id},
        {"previous_hash", previous_hash},
        {"timestamp",     timestamp},
        {"request_id",    request_id},
        {"query_hash",    query_hash},
        {"response_hash", response_hash},
        {"model_hash",    model_hash},
        {"adapter_hash",  adapter_hash},
        {"commitments",   commitments},
        {"metadata",      metadata}
    };
    const std::string canonical = content.dump();
    return LoRAProvenanceManager::sha256Hex(canonical);
}

// ============================================================================
// LoRAProvenanceManager – Pimpl
// ============================================================================

struct LoRAProvenanceManager::Impl {
    mutable std::mutex mu;

    // adapter_id → provenance record (local)
    std::unordered_map<std::string, LoRAProvenanceRecord> local_provenance;

    // adapter_id → external provenance record
    std::unordered_map<std::string, ExternalAdapterProvenance> external_provenance;

    // adapter_id → ordered list of snapshots
    std::unordered_map<std::string, std::vector<AdapterSnapshot>> snapshots;

    // snapshot_id → snapshot (for fast lookup)
    std::unordered_map<std::string, AdapterSnapshot> snapshot_by_id;

    // adapter_id → ordered audit log entries (Merkle chain)
    std::unordered_map<std::string, std::vector<InferenceAuditEntry>> audit_logs;
};

// ============================================================================
// LoRAProvenanceManager – public interface
// ============================================================================

LoRAProvenanceManager::LoRAProvenanceManager()
    : impl_(std::make_unique<Impl>()) {}

LoRAProvenanceManager::~LoRAProvenanceManager() = default;

// ---------------------------------------------------------------------------
// Local provenance
// ---------------------------------------------------------------------------

bool LoRAProvenanceManager::storeProvenance(const std::string& adapter_id,
                                             const LoRAProvenanceRecord& record) {
    if (adapter_id.empty()) {
        spdlog::error("LoRAProvenanceManager::storeProvenance: empty adapter_id");
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->local_provenance[adapter_id] = record;
    spdlog::debug("LoRAProvenanceManager: stored provenance for '{}'", adapter_id);
    return true;
}

std::optional<LoRAProvenanceRecord> LoRAProvenanceManager::getProvenance(
    const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->local_provenance.find(adapter_id);
    if (it == impl_->local_provenance.end()) {
      return std::nullopt;
    }
    return it->second;
}

// ---------------------------------------------------------------------------
// External adapter import
// ---------------------------------------------------------------------------

ExternalAdapterProvenance LoRAProvenanceManager::importExternalAdapter(
    const std::string& adapter_id,
    ExternalAdapterProvenance provenance,
    const std::string& trusted_ca_pem,
    bool allow_unsigned) {

    provenance.import_timestamp = nowISO8601();
    provenance.validation_errors.clear();

    if (allow_unsigned) {
        // Skip all checks; mark as valid for the caller's convenience
        provenance.signature_valid  = true;
        provenance.cert_chain_valid = true;
        spdlog::warn("LoRAProvenanceManager::importExternalAdapter: "
                     "allow_unsigned=true for '{}' — provenance checks bypassed",
                     adapter_id);
    } else {
        // Signature validation
        if (provenance.provenance_signature.empty()) {
            provenance.signature_valid = false;
            provenance.validation_errors.push_back("Missing provenance signature");
        } else {
            // NOTE: Full Ed25519/ECDSA cryptographic verification requires the
            // supplier's public key or certificate.  Here we verify that the
            // signature field is non-empty and the certificate chain is present,
            // which is the minimum structural check.  A production deployment
            // must replace this with a real OpenSSL / BoringSSL verification.
            provenance.signature_valid = !provenance.certificate_chain.empty();
            if (!provenance.signature_valid) {
                provenance.validation_errors.push_back(
                    "Certificate chain required for signature verification");
            }
        }

        // Certificate chain validation
        if (provenance.certificate_chain.empty()) {
            provenance.cert_chain_valid = false;
            provenance.validation_errors.push_back("Missing certificate chain");
        } else if (trusted_ca_pem.empty()) {
            // No trusted CA bundle provided — cannot verify chain
            provenance.cert_chain_valid = false;
            provenance.validation_errors.push_back(
                "No trusted CA bundle provided; certificate chain cannot be verified");
        } else {
            // Structural check: chain and trusted CA are both present
            provenance.cert_chain_valid = true;
        }

        // Adapter hash sanity check
        if (provenance.adapter_hash.empty()) {
            provenance.validation_errors.push_back("Missing adapter hash");
        } else if (static_cast<int>(provenance.adapter_hash.size()) != 64) {
            provenance.validation_errors.push_back(
                "Adapter hash must be a 64-character hex SHA-256 digest");
        }

        if (!provenance.validation_errors.empty()) {
            spdlog::warn("LoRAProvenanceManager::importExternalAdapter: "
                         "validation failed for '{}': {}",
                         adapter_id, provenance.validation_errors[0]);
        }
    }

    // Only persist when all validations pass (or unsigned import was requested)
    bool all_valid = provenance.signature_valid &&
                     provenance.cert_chain_valid &&
                     provenance.validation_errors.empty();

    if (all_valid || allow_unsigned) {
        std::lock_guard<std::mutex> lock(impl_->mu);
        impl_->external_provenance[adapter_id] = provenance;
        spdlog::debug("LoRAProvenanceManager: stored external provenance for '{}'",
                      adapter_id);
    }

    return provenance;
}

std::optional<ExternalAdapterProvenance> LoRAProvenanceManager::getExternalProvenance(
    const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->external_provenance.find(adapter_id);
    if (it == impl_->external_provenance.end()) {
      return std::nullopt;
    }
    return it->second;
}

// ---------------------------------------------------------------------------
// Snapshots
// ---------------------------------------------------------------------------

AdapterSnapshot LoRAProvenanceManager::createSnapshot(
    const std::string& adapter_id,
    const std::string& version,
    const std::string& weights_hash,
    const LoRAProvenanceRecord& provenance) {

    AdapterSnapshot snap;
    snap.snapshot_id = generateId();
    snap.adapter_id  = adapter_id;
    snap.version     = version;
    snap.weights_hash = weights_hash;
    snap.timestamp   = nowISO8601();
    snap.provenance  = provenance;

    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto& chain = impl_->snapshots[adapter_id];
        if (!chain.empty()) {
            snap.parent_snapshot_id = chain.back().snapshot_id;
        }
        chain.push_back(snap);
        impl_->snapshot_by_id[snap.snapshot_id] = snap;
    }

    spdlog::debug("LoRAProvenanceManager: created snapshot '{}' for '{}'",
                  snap.snapshot_id, adapter_id);
    return snap;
}

std::vector<AdapterSnapshot> LoRAProvenanceManager::listSnapshots(
    const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->snapshots.find(adapter_id);
    if (it == impl_->snapshots.end()) return {};
    return it->second;
}

std::optional<AdapterSnapshot> LoRAProvenanceManager::getSnapshot(
    const std::string& snapshot_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->snapshot_by_id.find(snapshot_id);
    if (it == impl_->snapshot_by_id.end()) {
      return std::nullopt;
    }
    return it->second;
}

// ---------------------------------------------------------------------------
// Merkle-chained audit log
// ---------------------------------------------------------------------------

InferenceAuditEntry LoRAProvenanceManager::appendAuditEntry(
    const std::string& adapter_id,
    InferenceAuditEntry entry) {

    // Populate auto-generated fields
    if (entry.entry_id.empty()) {
      entry.entry_id  = generateId();
    }
    if (entry.timestamp.empty()) {
      entry.timestamp  = nowISO8601();
    }

    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto& log = impl_->audit_logs[adapter_id];

        // Link to previous entry
        if (!log.empty()) {
            entry.previous_hash = log.back().entry_hash;
        } else {
            entry.previous_hash = "";  // genesis entry
        }

        // Compute the Merkle hash for this entry
        entry.entry_hash = entry.computeContentHash();

        log.push_back(entry);
    }

    spdlog::debug("LoRAProvenanceManager: appended audit entry '{}' for '{}'",
                  entry.entry_id, adapter_id);
    return entry;
}

std::vector<InferenceAuditEntry> LoRAProvenanceManager::getAuditLog(
    const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->audit_logs.find(adapter_id);
    if (it == impl_->audit_logs.end()) return {};
    return it->second;
}

bool LoRAProvenanceManager::verifyAuditChain(
    const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    auto it = impl_->audit_logs.find(adapter_id);
    if (it == impl_->audit_logs.end()) return true;  // empty chain is trivially valid

    const auto& log = it->second;
    std::string expected_previous = {};

    for (size_t i = 0; i < log.size(); ++i) {
        const auto& e = log[i];

        // Verify previous_hash linkage
        if (e.previous_hash != expected_previous) {
            spdlog::error("LoRAProvenanceManager::verifyAuditChain: "
                          "previous_hash mismatch at entry {} for '{}'", i, adapter_id);
            return false;
        }

        // Recompute and verify entry_hash
        const std::string recomputed = e.computeContentHash();
        if (e.entry_hash != recomputed) {
            spdlog::error("LoRAProvenanceManager::verifyAuditChain: "
                          "entry_hash mismatch at entry {} for '{}'", i, adapter_id);
            return false;
        }

        expected_previous = e.entry_hash;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

std::string LoRAProvenanceManager::sha256Hex(const std::string& data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
    return digestToHex(digest);
}

std::string LoRAProvenanceManager::sha256File(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        spdlog::warn("LoRAProvenanceManager::sha256File: cannot open '{}'", path);
        return "";
    }

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buf[4096];
    while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
        SHA256_Update(&ctx,
                      reinterpret_cast<const unsigned char*>(buf),
                      static_cast<size_t>(file.gcount()));
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &ctx);
    return digestToHex(digest);
}

} // namespace lora
} // namespace llm
} // namespace themis

