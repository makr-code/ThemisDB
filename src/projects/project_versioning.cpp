/**
 * @file project_versioning.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "projects/project_versioning.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// SHA-256 via OpenSSL EVP
#include <openssl/evp.h>

namespace themis {
namespace projects {

// ─── Diagnostic helpers ───────────────────────────────────────────────────────

/**
 * @brief Safely parse JSON with unified error reporting for versioning faults.
 * @param data String to parse
 * @param context Diagnostic context label for error messages
 * @return Parsed JSON on success, null JSON on failure
 */
static json safeJsonParse(const std::string& data, const char* context) noexcept {
    (void)context;
    try {
        return json::parse(data);
    } catch (const nlohmann::json::exception &) {
        // JSON parsing failure — corrupt data or invalid format
        return json();
    } catch (const std::exception &) {
        // Unexpected exception during parsing
        return json();
    } catch (const std::string &) {
        // Fallback for string exceptions
        return json();
    } catch (const char *) {
        // Fallback for C-string exceptions
        return json();
    }
}

// ─── SnapshotMeta serialisation ───────────────────────────────────────────────

json SnapshotMeta::toJson() const {
    return json{
        {"id",             id},
        {"project_id",     project_id},
        {"description",    description},
        {"created_at",     created_at},
        {"checksum",       checksum},
        {"document_count", document_count},
        {"metadata",       metadata},
    };
}

SnapshotMeta SnapshotMeta::fromJson(const json& j) {
    SnapshotMeta m;
    m.id             = j.value("id", std::string{});
    m.project_id     = j.value("project_id", std::string{});
    m.description    = j.value("description", std::string{});
    m.created_at     = j.value("created_at", int64_t{0});
    m.checksum       = j.value("checksum", std::string{});
    m.document_count = j.value("document_count", size_t{0});
    m.metadata       = j.value("metadata", json::object());
    return m;
}

// ─── ProjectVersioning ────────────────────────────────────────────────────────

ProjectVersioning::ProjectVersioning(std::shared_ptr<RocksDBWrapper> storage)
    : storage_(std::move(storage)) {}

// ── Private helpers ──────────────────────────────────────────────────────────

std::string ProjectVersioning::generateUuid() const {
    static thread_local std::mt19937_64 rng{
        static_cast<std::mt19937_64::result_type>(
            std::chrono::steady_clock::now().time_since_epoch().count())
    };
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(rng)
        << std::setw(16) << dist(rng);
    auto s = oss.str();
    // Insert dashes: 8-4-4-4-12
    return s.substr(0,8) + "-" + s.substr(8,4) + "-" + s.substr(12,4)
         + "-" + s.substr(16,4) + "-" + s.substr(20,12);
}

Sha256Digest ProjectVersioning::computeChecksum(const std::string& data) const {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>
        ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx)
        return {};

    EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx.get(), data.data(), data.size());
    EVP_DigestFinal_ex(ctx.get(), digest, &len);

    std::ostringstream oss;
    for (unsigned int i = 0; i < len; ++i)
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)digest[i];
    return oss.str();
}

std::vector<std::string> ProjectVersioning::collectProjectDocKeys(
    const std::string& project_id) const
{
    std::vector<std::string> keys;
    const std::string prefix = "doc_proj:" + project_id + ":";
    storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view) {
        keys.emplace_back(key);
        return true;
    });
    return keys;
}

// ── Public API ───────────────────────────────────────────────────────────────

std::variant<SnapshotId, Status> ProjectVersioning::createSnapshot(
    const std::string& project_id,
    const std::string& description,
    const json&        metadata)
{
    // ── Entry validation: enforce bounded runtime contract ─────────────────────
    if (project_id.empty())
        return Status::Error("createSnapshot: project_id must not be empty");

    std::unique_lock lock(mutex_);

    const auto snap_uuid = generateUuid();
    const SnapshotId snap_id = "snap:" + snap_uuid;

    // Collect current document metadata for this project
    auto doc_keys = collectProjectDocKeys(project_id);

    json content_array = json::array();
    for (const auto& key : doc_keys) {
        std::string val;
        if (storage_->get(key, val)) {
            const json parsed = safeJsonParse(val, "createSnapshot_doc");
            if (!parsed.is_null()) {
                content_array.push_back(parsed);
            } else {
                // Fallback: store raw string as JSON string value
                content_array.push_back(val);
            }
        }
    }
    
    const std::string content_str = content_array.dump();
    const Sha256Digest checksum = computeChecksum(content_str);

    SnapshotMeta meta;
    meta.id             = snap_id;
    meta.project_id     = project_id;
    meta.description    = description;
    meta.created_at     = static_cast<int64_t>(
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::seconds(1));
    meta.checksum       = checksum;
    meta.document_count = doc_keys.size();
    meta.metadata       = metadata;

    const std::string meta_str = meta.toJson().dump();

    // Write metadata record
    if (!storage_->put(snap_id, meta_str))
        return Status::Error("createSnapshot: failed to write snapshot metadata");

    // Write snapshot content
    const std::string content_key = "snap_data:" + snap_uuid;
    if (!storage_->put(content_key, content_str)) {
        storage_->del(snap_id);
        return Status::Error("createSnapshot: failed to write snapshot content");
    }

    // Write secondary index for listSnapshots
    const std::string idx_key =
        "snap_idx:" + project_id + ":" + snap_uuid;
    if (!storage_->put(idx_key, "")) {
        storage_->del(snap_id);
        storage_->del(content_key);
        return Status::Error("createSnapshot: failed to write snapshot index entry");
    }

    return snap_id;
}

std::optional<SnapshotMeta> ProjectVersioning::getSnapshot(
    const SnapshotId& snap_id) const
{
    std::shared_lock lock(mutex_);
    std::string val;
    if (!storage_->get(snap_id, val))
        return std::nullopt;
    const json parsed = safeJsonParse(val, "getSnapshot");
    if (parsed.is_null() || !parsed.is_object())
        return std::nullopt;
    try {
        return SnapshotMeta::fromJson(parsed);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::vector<SnapshotMeta> ProjectVersioning::listSnapshots(
    const std::string& project_id) const
{
    std::shared_lock lock(mutex_);
    std::vector<SnapshotMeta> result;

    const std::string prefix = "snap_idx:" + project_id + ":";
    storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view) {
        // Extract snap_uuid from key  "snap_idx:<proj>:<uuid>"
        const auto pos = key.rfind(':');
        if (pos == std::string_view::npos) {
          return true;
        }
        const std::string snap_id = "snap:" + std::string(key.substr(pos + 1));
        std::string val;
        if (storage_->get(snap_id, val)) {
            const json parsed = safeJsonParse(val, "listSnapshots");
            if (!parsed.is_null() && parsed.is_object()) {
                try {
                    result.push_back(SnapshotMeta::fromJson(parsed));
                } catch (const std::exception &) {
                    // Skip corrupted snapshot entries
                }
            }
        }
        return true;
    });

    // Sort newest first
    std::sort(result.begin(), result.end(),
              [](const SnapshotMeta& a, const SnapshotMeta& b) {
                  return a.created_at > b.created_at;
              });
    return result;
}

Status ProjectVersioning::deleteSnapshot(const SnapshotId& snap_id) {
    // ── Entry validation: enforce bounded runtime contract ─────────────────────
    if (snap_id.empty())
        return Status::Error("deleteSnapshot: snapshot_id must not be empty");
    if (!snap_id.starts_with("snap:"))
        return Status::Error("deleteSnapshot: invalid snapshot_id format (must start with 'snap:')");

    std::unique_lock lock(mutex_);

    std::string val;
    if (!storage_->get(snap_id, val))
        return Status::Error("deleteSnapshot: snapshot not found: " + snap_id);

    SnapshotMeta meta;
    const json parsed = safeJsonParse(val, "deleteSnapshot");
    if (parsed.is_null() || !parsed.is_object()) {
        // Even if metadata is corrupted, attempt to clean up all associated data
        const auto snap_uuid = snap_id.substr(5);
        storage_->del(snap_id);
        storage_->del("snap_data:" + snap_uuid);
        return Status::Error("deleteSnapshot: snapshot metadata corrupted but cleanup attempted");
    }
    
    try {
        meta = SnapshotMeta::fromJson(parsed);
    } catch (const std::exception &e) {
        const auto snap_uuid = snap_id.substr(5);
        storage_->del(snap_id);
        storage_->del("snap_data:" + snap_uuid);
        return Status::Error(
            std::string("deleteSnapshot: failed to deserialize snapshot metadata but cleanup attempted: ") + e.what());
    }

    const auto snap_uuid = snap_id.substr(5); // strip "snap:"
    storage_->del(snap_id);
    storage_->del("snap_data:" + snap_uuid);
    storage_->del("snap_idx:" + meta.project_id + ":" + snap_uuid);

    return Status::OK();
}

Status ProjectVersioning::restoreSnapshot(
    const SnapshotId& snap_id,
    const std::string& target_project_id)
{
    // ── Entry validation: enforce bounded runtime contract ─────────────────────
    if (snap_id.empty())
        return Status::Error("restoreSnapshot: snapshot_id must not be empty");
    if (target_project_id.empty())
        return Status::Error("restoreSnapshot: target_project_id must not be empty");
    if (!snap_id.starts_with("snap:"))
        return Status::Error("restoreSnapshot: invalid snapshot_id format (must start with 'snap:')");

    std::unique_lock lock(mutex_);

    std::string meta_str;
    if (!storage_->get(snap_id, meta_str))
        return Status::Error("restoreSnapshot: snapshot not found: " + snap_id);

    SnapshotMeta meta;
    const json meta_parsed = safeJsonParse(meta_str, "restoreSnapshot_metadata");
    if (meta_parsed.is_null() || !meta_parsed.is_object())
        return Status::Error("restoreSnapshot: snapshot metadata corrupted");
    
    try {
        meta = SnapshotMeta::fromJson(meta_parsed);
    } catch (const std::exception &e) {
        return Status::Error(
            std::string("restoreSnapshot: failed to deserialize snapshot metadata: ") + e.what());
    }

    const auto snap_uuid = snap_id.substr(5);
    std::string content_str;
    if (!storage_->get("snap_data:" + snap_uuid, content_str))
        return Status::Error("restoreSnapshot: snapshot content missing for: " + snap_id);

    // ── Integrity check: verify snapshot content ──────────────────────────────
    const Sha256Digest actual = computeChecksum(content_str);
    if (actual != meta.checksum)
        return Status::Error(
            "restoreSnapshot: checksum mismatch — snapshot data may be corrupt");

    // ── Content validation: verify structure before restore ───────────────────
    const json content_parsed = safeJsonParse(content_str, "restoreSnapshot_content");
    if (content_parsed.is_null())
        return Status::Error("restoreSnapshot: snapshot content is not valid JSON");
    if (!content_parsed.is_array())
        return Status::Error("restoreSnapshot: snapshot content is not a JSON array");
    
    // ── Restore: write each document metadata record under the target project ─
    try {
        for (const auto& doc_json : content_parsed) {
            if (!doc_json.is_object())
                continue;  // Skip non-objects
            const std::string doc_id = doc_json.value("id", std::string{});
            if (doc_id.empty())
                continue;  // Skip entries with empty IDs
            const std::string key = "doc_proj:" + target_project_id + ":" + doc_id;
            const std::string doc_str = doc_json.dump();
            if (!storage_->put(key, doc_str)) {
                return Status::Error(
                    "restoreSnapshot: failed to write document '" + doc_id + "' to target project");
            }
        }
    } catch (const std::exception& e) {
        return Status::Error(
            std::string("restoreSnapshot: exception during content restoration: ") + e.what());
    }

    return Status::OK();
}

bool ProjectVersioning::verifySnapshot(const SnapshotId& snap_id) const {
    std::shared_lock lock(mutex_);

    if (snap_id.empty() || !snap_id.starts_with("snap:"))
        return false;

    std::string meta_str;
    if (!storage_->get(snap_id, meta_str)) 
        return false;

    SnapshotMeta meta;
    const json parsed = safeJsonParse(meta_str, "verifySnapshot");
    if (parsed.is_null() || !parsed.is_object())
        return false;
    
    try {
        meta = SnapshotMeta::fromJson(parsed);
    } catch (const std::exception &) {
        return false;
    }

    const auto snap_uuid = snap_id.substr(5);
    std::string content_str;
    if (!storage_->get("snap_data:" + snap_uuid, content_str))
        return false;

    const Sha256Digest actual = computeChecksum(content_str);
    return actual == meta.checksum;
}

} // namespace projects
} // namespace themis
