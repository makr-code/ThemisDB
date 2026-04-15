/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hardware_migration_manager.cpp                     ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:50:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/hardware_migration_manager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/** ISO-8601 timestamp for the current UTC wall-clock time (thread-safe). */
std::string utcNowIso8601() {
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// NodeIdentity
// ─────────────────────────────────────────────────────────────────────────────

std::string NodeIdentity::toJson() const {
    nlohmann::json j = {
        {"shard_id",         shard_id},
        {"cluster_name",     cluster_name},
        {"token_start",      token_start},
        {"token_end",        token_end},
        {"created_at",       created_at},
        {"identity_version", identity_version}
    };
    return j.dump(2);
}

std::optional<NodeIdentity> NodeIdentity::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        NodeIdentity id;
        id.shard_id         = j.value("shard_id",         "");
        id.cluster_name     = j.value("cluster_name",     "");
        id.token_start      = j.value("token_start",      uint64_t{0});
        id.token_end        = j.value("token_end",        uint64_t{0});
        id.created_at       = j.value("created_at",       "");
        id.identity_version = j.value("identity_version", "");

        if (id.shard_id.empty()) {
            return std::nullopt;
        }
        return id;
    } catch (const std::exception& e) {
        std::cerr << "NodeIdentity::fromJson: parse error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool NodeIdentity::saveTo(const std::string& path) const {
    try {
        namespace fs = std::filesystem;
        fs::path p(path);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }
        std::ofstream ofs(p);
        if (!ofs.is_open()) {
            std::cerr << "NodeIdentity::saveTo: cannot open file for writing: " << path << std::endl;
            return false;
        }
        ofs << toJson();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "NodeIdentity::saveTo: " << e.what() << std::endl;
        return false;
    }
}

std::optional<NodeIdentity> NodeIdentity::loadFrom(const std::string& path) {
    try {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            return std::nullopt; // File does not exist yet — not an error.
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        return fromJson(content);
    } catch (const std::exception& e) {
        std::cerr << "NodeIdentity::loadFrom: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HardwareMigrationManager — construction
// ─────────────────────────────────────────────────────────────────────────────

HardwareMigrationManager::HardwareMigrationManager(
    const HardwareMigrationConfig& config,
    std::shared_ptr<ShardTopology>       topology,
    std::shared_ptr<ConsistentHashRing>  ring
) : config_(config), topology_(topology), ring_(ring) {}

// ─────────────────────────────────────────────────────────────────────────────
// Identity management
// ─────────────────────────────────────────────────────────────────────────────

std::optional<NodeIdentity>
HardwareMigrationManager::loadIdentity() const {
    return NodeIdentity::loadFrom(config_.identity_file_path);
}

std::optional<NodeIdentity>
HardwareMigrationManager::createAndSaveIdentity(
    const std::string& shard_id,
    const std::string& cluster_name,
    uint64_t           token_start,
    uint64_t           token_end
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Refuse to overwrite an existing identity.
    if (std::filesystem::exists(config_.identity_file_path)) {
        std::cerr << "HardwareMigrationManager::createAndSaveIdentity: "
                     "identity file already exists at "
                  << config_.identity_file_path
                  << " — load it with loadIdentity() instead." << std::endl;
        return std::nullopt;
    }

    NodeIdentity identity;
    identity.shard_id         = shard_id;
    identity.cluster_name     = cluster_name;
    identity.token_start      = token_start;
    identity.token_end        = token_end;
    identity.created_at       = utcNowIso8601();
    identity.identity_version = "1";

    if (!identity.saveTo(config_.identity_file_path)) {
        return std::nullopt;
    }
    return identity;
}

// ─────────────────────────────────────────────────────────────────────────────
// Endpoint replacement
// ─────────────────────────────────────────────────────────────────────────────

HardwareMigrationResult
HardwareMigrationManager::replaceEndpoint(
    const std::string& shard_id,
    const std::string& new_endpoint
) {
    HardwareMigrationResult result;
    result.shard_id      = shard_id;
    result.new_endpoint  = new_endpoint;

    if (shard_id.empty()) {
        result.message = "shard_id must not be empty";
        return result;
    }
    if (new_endpoint.empty()) {
        result.message = "new_endpoint must not be empty";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // ── 1. Validate shard exists in topology ─────────────────────────────────
    auto maybe_shard = topology_->getShard(shard_id);
    if (!maybe_shard) {
        result.message = "shard_id '" + shard_id + "' not found in topology";
        return result;
    }
    result.old_endpoint = maybe_shard->primary_endpoint;

    // ── 2. Optional ring-stability pre-check ─────────────────────────────────
    std::map<std::string, size_t> before_vnodes;
    if (config_.verify_ring_stability && ring_) {
        before_vnodes = captureRingSnapshotLocked();
    }

    // ── 3. Update topology (only the endpoint; ring positions are unaffected) ─
    ShardInfo updated = *maybe_shard;
    updated.primary_endpoint = new_endpoint;
    topology_->addShard(updated); // addShard is an upsert

    // ── 4. Ring-stability post-check ─────────────────────────────────────────
    if (config_.verify_ring_stability && ring_) {
        auto after_vnodes = captureRingSnapshotLocked();
        bool stable = true;
        for (const auto& [sid, count] : before_vnodes) {
            auto it = after_vnodes.find(sid);
            if (it == after_vnodes.end() || it->second != count) {
                stable = false;
                result.message = "Ring stability check FAILED: virtual-node count changed "
                                 "for shard '" + sid + "'";
                // Roll back topology update.
                topology_->addShard(*maybe_shard);
                return result;
            }
        }
        result.ring_stability_verified = stable;
    }

    result.success = true;
    result.message = "Endpoint for shard '" + shard_id + "' updated from '" +
                     result.old_endpoint + "' to '" + new_endpoint + "'";
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Ring-stability helpers
// ─────────────────────────────────────────────────────────────────────────────

std::map<std::string, size_t>
HardwareMigrationManager::captureRingSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return captureRingSnapshotLocked();
}

std::map<std::string, size_t>
HardwareMigrationManager::captureRingSnapshotLocked() const {
    // mutex_ must already be held by caller when invoked from replaceEndpoint().
    std::map<std::string, size_t> snapshot;
    if (!ring_) {
        return snapshot;
    }
    // NOTE: ConsistentHashRing does not expose individual per-shard virtual-node
    // counts; only the aggregate total via getVirtualNodeCount() is available.
    // This snapshot therefore stores the same sentinel value (total vnode count)
    // for every shard. The purpose is purely change-detection for hardware
    // migration: because replaceEndpoint() never calls ring_.addShard() or
    // ring_.removeShard(), neither the shard set nor the total vnode count can
    // change, making the comparison trivially stable. The snapshot would detect
    // accidental ring mutations introduced by future refactoring.
    auto shards = ring_->getAllShards();
    size_t total = ring_->getVirtualNodeCount();
    for (const auto& sid : shards) {
        snapshot[sid] = total; // sentinel: total vnode count shared by all shards
    }
    return snapshot;
}

bool HardwareMigrationManager::validateRingStability(
    const std::vector<std::string>&       shard_ids,
    const std::map<std::string, size_t>&  before_vnode_counts
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ring_) {
        return true; // No ring — nothing to validate.
    }
    auto current = captureRingSnapshotLocked();
    for (const auto& sid : shard_ids) {
        auto before_it  = before_vnode_counts.find(sid);
        auto current_it = current.find(sid);

        if (before_it == before_vnode_counts.end()) {
            continue; // Shard was not in the ring before — ignore.
        }
        if (current_it == current.end()) {
            std::cerr << "validateRingStability: shard '" << sid
                      << "' disappeared from the ring after migration" << std::endl;
            return false;
        }
        if (current_it->second != before_it->second) {
            std::cerr << "validateRingStability: virtual-node count changed for shard '"
                      << sid << "': was " << before_it->second
                      << ", now " << current_it->second << std::endl;
            return false;
        }
    }
    return true;
}

} // namespace themis::sharding
