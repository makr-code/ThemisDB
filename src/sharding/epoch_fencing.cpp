/**
 * @file epoch_fencing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/epoch_fencing.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// NullStonithProvider
// ─────────────────────────────────────────────────────────────────────────────

bool NullStonithProvider::fence(const NodeId& node_id,
                                const std::string& /*reason*/,
                                std::chrono::steady_clock::time_point /*deadline*/) {
    std::lock_guard<std::mutex> lk(mutex_);
    fenced_.push_back(node_id);
    spdlog::info("[NullStonith] fenced node '{}'", node_id);
    return true;
}

bool NullStonithProvider::isFenced(const NodeId& node_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return std::find(fenced_.begin(), fenced_.end(), node_id) != fenced_.end();
}

std::vector<NodeId> NullStonithProvider::fencedNodes() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return fenced_;
}

void NullStonithProvider::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    fenced_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingConfig
// ─────────────────────────────────────────────────────────────────────────────

bool EpochFencingConfig::validate() const {
    if (shard_id.empty()) {
        spdlog::error("[EpochFencingConfig] shard_id must not be empty");
        return false;
    }
    if (node_id.empty()) {
        spdlog::error("[EpochFencingConfig] node_id must not be empty");
        return false;
    }
    if (stonith_timeout_ms.count() <= 0) {
        spdlog::error("[EpochFencingConfig] stonith_timeout_ms must be > 0");
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

EpochFencingManager::EpochFencingManager(EpochFencingConfig config,
                                         std::shared_ptr<IStonithProvider> stonith)
    : config_(std::move(config)), stonith_(std::move(stonith)) {
    if (!config_.validate()) {
        throw std::invalid_argument("EpochFencingManager: invalid configuration");
    }
    if (!stonith_) {
        stonith_ = std::make_shared<NullStonithProvider>();
    }
    spdlog::info("[EpochFencing] shard='{}' node='{}' initial_epoch=1 stonith='{}'",
                 config_.shard_id, config_.node_id,
                 stonith_->providerName());
}

EpochFencingManager::~EpochFencingManager() = default;

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — epoch management
// ─────────────────────────────────────────────────────────────────────────────

EpochToken EpochFencingManager::bumpEpoch(const std::string& reason) {
    EpochNumber new_epoch = current_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++metrics_.epoch_bumps;
    }

    spdlog::info("[EpochFencing] shard='{}' epoch bumped to {} reason='{}'",
                 config_.shard_id, new_epoch, reason);

    EpochToken tok;
    tok.epoch     = new_epoch;
    tok.issuer    = config_.node_id;
    tok.shard_id  = config_.shard_id;
    tok.issued_at = std::chrono::system_clock::now();
    return tok;
}

EpochNumber EpochFencingManager::currentEpoch() const noexcept {
    return current_epoch_.load(std::memory_order_acquire);
}

EpochToken EpochFencingManager::makeToken() const {
    EpochToken tok;
    tok.epoch     = current_epoch_.load(std::memory_order_acquire);
    tok.issuer    = config_.node_id;
    tok.shard_id  = config_.shard_id;
    tok.issued_at = std::chrono::system_clock::now();
    return tok;
}

// ─────────────────────────────────────────────────────────────────────────────
// EpochFencingManager — fencing check
// ─────────────────────────────────────────────────────────────────────────────

FencingResult EpochFencingManager::checkToken(const EpochToken& token,
                                              const NodeId& source_node) {
    // Structural validity
    if (token.epoch == 0) {
        spdlog::warn("[EpochFencing] INVALID_TOKEN from '{}' (epoch==0)", source_node);
        return FencingResult::INVALID_TOKEN;
    }

    EpochNumber cur = current_epoch_.load(std::memory_order_acquire);

    if (token.epoch >= cur) {
        std::lock_guard<std::mutex> lk(mutex_);
        ++metrics_.allowed_writes;
        return FencingResult::ALLOWED;
    }

    // Stale epoch
    uint64_t delta = cur - token.epoch;
    if (delta >= config_.log_warn_epoch_delta) {
        spdlog::warn("[EpochFencing] STALE_EPOCH from='{}' token_epoch={} current={}",
                     source_node, token.epoch, cur);
    }

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++metrics_.stale_rejections;
    }

    if (config_.auto_stonith) {
        return issueStonith(source_node, "stale_epoch");
    }
    return FencingResult::STALE_EPOCH;
}

FencingResult EpochFencingManager::issueStonith(const NodeId& node,
                                                const std::string& reason) {
    auto deadline = std::chrono::steady_clock::now() + config_.stonith_timeout_ms;
    spdlog::warn("[EpochFencing] issuing STONITH against '{}' reason='{}'",
                 node, reason);

    bool ok = stonith_->fence(node, reason, deadline);
    std::lock_guard<std::mutex> lk(mutex_);
    if (ok) {
        ++metrics_.stonith_issued;
        spdlog::info("[EpochFencing] STONITH confirmed for '{}'", node);
        return FencingResult::STONITH_ISSUED;
    } else {
        ++metrics_.stonith_failed;
        spdlog::error("[EpochFencing] STONITH FAILED for '{}'", node);
        return FencingResult::STONITH_FAILED;
    }
}

EpochFencingManager::Metrics EpochFencingManager::metrics() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return metrics_;
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseConfig
// ─────────────────────────────────────────────────────────────────────────────

bool LeaseConfig::validate() const {
    if (ttl_ms.count() <= 0) {
        spdlog::error("[LeaseConfig] ttl_ms must be > 0");
        return false;
    }
    if (renew_before_ms >= ttl_ms) {
        spdlog::error("[LeaseConfig] renew_before_ms must be < ttl_ms");
        return false;
    }
    if (acquire_wait_ms.count() <= 0) {
        spdlog::error("[LeaseConfig] acquire_wait_ms must be > 0");
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

LeaseManager::LeaseManager(LeaseConfig config,
                           std::shared_ptr<EpochFencingManager> fencing)
    : config_(std::move(config)), fencing_(std::move(fencing)) {
    if (!config_.validate()) {
        throw std::invalid_argument("LeaseManager: invalid configuration");
    }
    if (!fencing_) {
        throw std::invalid_argument("LeaseManager: fencing manager must not be null");
    }
    if (!config_.wal_path.empty()) {
        loadFromWal();
    }
    spdlog::info("[LeaseManager] ttl={}ms renew_before={}ms wal='{}'",
                 config_.ttl_ms.count(),
                 config_.renew_before_ms.count(),
                 config_.wal_path);
}

LeaseManager::~LeaseManager() = default;

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — acquire
// ─────────────────────────────────────────────────────────────────────────────

LeaseAcquireResult LeaseManager::acquire(const LeaseKey& key,
                                         const NodeId&   node_id) {
    auto deadline = std::chrono::steady_clock::now() + config_.acquire_wait_ms;

    while (true) {
        std::unique_lock<std::mutex> lk(mutex_);

        evictExpired();  // Clean up expired leases first

        auto it = leases_.find(key);

        // Available or never seen
        if (it == leases_.end() || it->second.state == LeaseState::AVAILABLE ||
            it->second.state == LeaseState::EXPIRED) {
            // Grant the lease
            LeaseRecord rec;
            rec.key        = key;
            rec.holder     = node_id;
            rec.epoch      = fencing_->currentEpoch();
            rec.acquired_at = std::chrono::system_clock::now();
            rec.expires_at = rec.acquired_at + config_.ttl_ms;
            rec.state      = LeaseState::HELD;

            if (it == leases_.end()) {
                rec.generation = 1;
            } else {
                rec.generation = it->second.generation + 1;
            }

            leases_[key] = rec;
            ++metrics_.acquires;
            persistToWal(rec);

            spdlog::info("[LeaseManager] lease='{}' granted to '{}' epoch={} gen={} ttl={}ms",
                         key, node_id, rec.epoch, rec.generation,
                         config_.ttl_ms.count());

            return LeaseAcquireResult{true, rec, {}};
        }

        // Already held by us — idempotent re-acquire (acts like renew)
        if (it->second.holder == node_id && it->second.isActive()) {
            auto& rec = it->second;
            rec.expires_at = std::chrono::system_clock::now() + config_.ttl_ms;
            ++metrics_.acquires;
            persistToWal(rec);
            return LeaseAcquireResult{true, rec, {}};
        }

        // Held by someone else — wait for it to expire.
        // Capture holder string while lock is held; iterator becomes invalid once lk is released.
        std::string blocked_holder = it->second.holder;
        lk.unlock();

        if (std::chrono::steady_clock::now() >= deadline) {
            ++metrics_.acquire_failures;
            LeaseAcquireResult res;
            res.success       = false;
            res.error_message = "lease held by '" + blocked_holder + "'; timeout";

            // Issue STONITH against the blocking holder so we can proceed
            if (fencing_->config().auto_stonith) {
                auto stonith_deadline =
                    std::chrono::steady_clock::now() + fencing_->config().stonith_timeout_ms;
                bool ok = fencing_->stonithProvider()->fence(
                    blocked_holder, "lease_acquisition_timeout", stonith_deadline);
                if (ok) {
                    ++metrics_.stonith_triggered;
                    // Try one more time after STONITH
                    std::unique_lock<std::mutex> lk2(mutex_);
                    leases_[key].state = LeaseState::AVAILABLE;
                    lk2.unlock();
                    continue;
                }
            }
            return res;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — renew
// ─────────────────────────────────────────────────────────────────────────────

std::optional<LeaseRecord> LeaseManager::renew(const LeaseKey& key,
                                               const NodeId&   node_id) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = leases_.find(key);
    if (it == leases_.end()) {
        ++metrics_.renewal_failures;
        spdlog::warn("[LeaseManager] renew: lease='{}' not found", key);
        return std::nullopt;
    }

    LeaseRecord& rec = it->second;

    if (rec.holder != node_id) {
        ++metrics_.renewal_failures;
        spdlog::warn("[LeaseManager] renew: lease='{}' held by '{}' not '{}'",
                     key, rec.holder, node_id);
        return std::nullopt;
    }

    if (rec.state != LeaseState::HELD) {
        ++metrics_.renewal_failures;
        spdlog::warn("[LeaseManager] renew: lease='{}' not in HELD state ({})",
                     key, toString(rec.state));
        return std::nullopt;
    }

    rec.expires_at = std::chrono::system_clock::now() + config_.ttl_ms;
    rec.epoch      = fencing_->currentEpoch();
    ++metrics_.renewals;
    persistToWal(rec);

    spdlog::debug("[LeaseManager] lease='{}' renewed by '{}' ttl={}ms",
                  key, node_id, config_.ttl_ms.count());
    return rec;
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — release
// ─────────────────────────────────────────────────────────────────────────────

bool LeaseManager::release(const LeaseKey& key, const NodeId& node_id) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto it = leases_.find(key);
    if (it == leases_.end()) {
        return false;
    }

    LeaseRecord& rec = it->second;
    if (rec.holder != node_id) {
        return false;
    }

    rec.state = LeaseState::AVAILABLE;
    ++metrics_.releases;
    persistToWal(rec);

    spdlog::info("[LeaseManager] lease='{}' released by '{}'", key, node_id);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — query
// ─────────────────────────────────────────────────────────────────────────────

std::optional<LeaseRecord> LeaseManager::get(const LeaseKey& key) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = leases_.find(key);
    if (it == leases_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool LeaseManager::isHolder(const LeaseKey& key, const NodeId& node_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = leases_.find(key);
    if (it == leases_.end()) {
        return false;
    }
    return it->second.holder == node_id && it->second.isActive();
}

std::vector<LeaseKey> LeaseManager::listLeases() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<LeaseKey> keys;
    keys.reserve(leases_.size());
    for (const auto& [k, _] : leases_) {
        keys.push_back(k);
    }
    return keys;
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — maintenance
// ─────────────────────────────────────────────────────────────────────────────

void LeaseManager::evictExpired() {
    // Assumes caller holds mutex_
    auto now = std::chrono::system_clock::now();
    for (auto& [key, rec] : leases_) {
        if (rec.state == LeaseState::HELD && now >= rec.expires_at) {
            rec.state = LeaseState::EXPIRED;
            ++metrics_.evictions;
            spdlog::info("[LeaseManager] lease='{}' expired (was held by '{}')",
                         key, rec.holder);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LeaseManager — WAL persistence (simple append-only text WAL)
// ─────────────────────────────────────────────────────────────────────────────

void LeaseManager::persistToWal(const LeaseRecord& rec) {
    if (config_.wal_path.empty()) {
        return;
    }
    try {
        std::ofstream wal(config_.wal_path, std::ios::app);
        if (!wal.is_open()) {
            spdlog::error("[LeaseManager] cannot open WAL '{}'", config_.wal_path);
            return;
        }
        // Format: key|holder|epoch|generation|state|acquired_ms|expires_ms
        auto since_epoch = [](std::chrono::system_clock::time_point tp) -> int64_t {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       tp.time_since_epoch())
                .count();
        };
        wal << rec.key << '|' << rec.holder << '|' << rec.epoch << '|'
            << rec.generation << '|' << static_cast<int>(rec.state) << '|'
            << since_epoch(rec.acquired_at) << '|' << since_epoch(rec.expires_at)
            << '\n';
    } catch (const std::exception& ex) {
        spdlog::error("[LeaseManager] WAL write error: {}", ex.what());
    }
}

void LeaseManager::loadFromWal() {
    if (config_.wal_path.empty()) {
        return;
    }
    std::ifstream wal(config_.wal_path);
    if (!wal.is_open()) {
        return;  // WAL does not exist yet — fresh start
    }
    auto now = std::chrono::system_clock::now();
    std::string line;
    std::unordered_map<LeaseKey, LeaseRecord> latest;

    while (std::getline(wal, line)) {
        if (line.empty()) {
          continue;
        }
        std::istringstream ss(line);
        LeaseRecord rec;
        std::string state_str, acquired_str, expires_str, gen_str, epoch_str;

        std::getline(ss, rec.key, '|');
        std::getline(ss, rec.holder, '|');
        std::getline(ss, epoch_str, '|');
        std::getline(ss, gen_str, '|');
        std::getline(ss, state_str, '|');
        std::getline(ss, acquired_str, '|');
        std::getline(ss, expires_str, '|');

        if (rec.key.empty() || rec.holder.empty()) {
          continue;
        }

        try {
            rec.epoch      = static_cast<EpochNumber>(std::stoull(epoch_str));
            rec.generation = std::stoull(gen_str);
            rec.state      = static_cast<LeaseState>(std::stoi(state_str));
            rec.acquired_at = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{std::stoll(acquired_str)}};
            rec.expires_at  = std::chrono::system_clock::time_point{
                std::chrono::milliseconds{std::stoll(expires_str)}};
        } catch (...) {
            continue;  // Skip malformed lines
        }

        // Discard already-expired records
        if (rec.state == LeaseState::HELD && now >= rec.expires_at) {
            rec.state = LeaseState::EXPIRED;
        }

        latest[rec.key] = rec;
    }

    leases_ = std::move(latest);
    spdlog::info("[LeaseManager] loaded {} lease record(s) from WAL '{}'",
                 leases_.size(), config_.wal_path);
}

bool LeaseManager::waitForExpiry(const LeaseKey& key,
                                 std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            auto it = leases_.find(key);
            if (it == leases_.end() ||
                it->second.state == LeaseState::AVAILABLE ||
                it->second.state == LeaseState::EXPIRED) {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

LeaseManager::Metrics LeaseManager::metrics() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return metrics_;
}

} // namespace sharding
} // namespace themis


