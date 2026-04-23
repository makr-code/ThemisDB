/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            project_versioning.cpp                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-15 18:50:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     321                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 600ad1dcea  2026-04-15  feat(projects): implement ProjectVersioning, ProjectDiff,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    if (project_id.empty())
        return Status::Error("project_id must not be empty");

    std::unique_lock lock(mutex_);

    const auto snap_uuid = generateUuid();
    const SnapshotId snap_id = "snap:" + snap_uuid;

    // Collect current document metadata for this project
    auto doc_keys = collectProjectDocKeys(project_id);

    json content_array = json::array();
    for (const auto& key : doc_keys) {
        std::string val;
        if (storage_->get(key, val)) {
            try { content_array.push_back(json::parse(val)); }
            catch (...) { content_array.push_back(val); }
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
    const std::vector<uint8_t> meta_bytes(meta_str.begin(), meta_str.end());
    const std::vector<uint8_t> content_bytes(content_str.begin(), content_str.end());

    // Write metadata record
    if (!storage_->put(snap_id, meta_str))
        return Status::Error("Failed to write snapshot metadata");

    // Write snapshot content
    const std::string content_key = "snap_data:" + snap_uuid;
    if (!storage_->put(content_key, content_str)) {
        storage_->del(snap_id);
        return Status::Error("Failed to write snapshot content");
    }

    // Write secondary index for listSnapshots
    const std::string idx_key =
        "snap_idx:" + project_id + ":" + snap_uuid;
    if (!storage_->put(idx_key, "")) {
        storage_->del(snap_id);
        storage_->del(content_key);
        return Status::Error("Failed to write snapshot index entry");
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
    try {
        return SnapshotMeta::fromJson(json::parse(val));
    } catch (...) {
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
        if (pos == std::string_view::npos) return true;
        const std::string snap_id = "snap:" + std::string(key.substr(pos + 1));
        std::string val;
        if (storage_->get(snap_id, val)) {
            try { result.push_back(SnapshotMeta::fromJson(json::parse(val))); }
            catch (...) {}
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
    std::unique_lock lock(mutex_);

    std::string val;
    if (!storage_->get(snap_id, val))
        return Status::Error("Snapshot not found: " + snap_id);

    SnapshotMeta meta;
    try {
        meta = SnapshotMeta::fromJson(json::parse(val));
    } catch (...) {
        return Status::Error("Failed to parse snapshot metadata");
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
    std::unique_lock lock(mutex_);

    std::string meta_str;
    if (!storage_->get(snap_id, meta_str))
        return Status::Error("Snapshot not found: " + snap_id);

    SnapshotMeta meta;
    try {
        meta = SnapshotMeta::fromJson(json::parse(meta_str));
    } catch (...) {
        return Status::Error("Failed to parse snapshot metadata");
    }

    const auto snap_uuid = snap_id.substr(5);
    std::string content_str;
    if (!storage_->get("snap_data:" + snap_uuid, content_str))
        return Status::Error("Snapshot content missing for: " + snap_id);

    // Verify checksum before writing anything
    const Sha256Digest actual = computeChecksum(content_str);
    if (actual != meta.checksum)
        return Status::Error(
            "Snapshot checksum mismatch — data may be corrupt");

    // Restore: write each document metadata record under the target project
    try {
        auto docs = json::parse(content_str);
        if (!docs.is_array())
            return Status::Error("Snapshot content is not a JSON array");

        for (const auto& doc_json : docs) {
            const std::string doc_id =
                doc_json.value("id", std::string{});
            if (doc_id.empty()) continue;
            const std::string key =
                "doc_proj:" + target_project_id + ":" + doc_id;
            const std::string doc_str = doc_json.dump();
            storage_->put(key, doc_str);
        }
    } catch (const std::exception& e) {
        return Status::Error(
            std::string("Failed to restore snapshot content: ") + e.what());
    }

    return Status::OK();
}

bool ProjectVersioning::verifySnapshot(const SnapshotId& snap_id) const {
    std::shared_lock lock(mutex_);

    std::string meta_str;
    if (!storage_->get(snap_id, meta_str)) return false;

    SnapshotMeta meta;
    try {
        meta = SnapshotMeta::fromJson(json::parse(meta_str));
    } catch (...) {
        return false;
    }

    const auto snap_uuid = snap_id.substr(5);
    std::string content_str;
    if (!storage_->get("snap_data:" + snap_uuid, content_str)) return false;

    return computeChecksum(content_str) == meta.checksum;
}

} // namespace projects
} // namespace themis
