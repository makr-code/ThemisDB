/**
 * @file cache_replication.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Cache Replication for High-Availability Deployments – Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cache/cache_replication.h"
#include <stdexcept>
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

const char *CacheReplicationManager::healthToString(CacheReplicaHealth h) {
    switch (h) {
        case CacheReplicaHealth::HEALTHY:
            return "HEALTHY";
        case CacheReplicaHealth::DEGRADED:
            return "DEGRADED";
        case CacheReplicaHealth::UNHEALTHY:
            return "UNHEALTHY";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// CacheReplicationManager – constructor
// ---------------------------------------------------------------------------

CacheReplicationManager::CacheReplicationManager(const CacheReplicationConfig &config) : config_(config) {}

// ---------------------------------------------------------------------------
// Replica registration
// ---------------------------------------------------------------------------

void CacheReplicationManager::addReplica(std::shared_ptr<ICacheReplicationListener> listener,
                                         const std::string &snapshot_ndjson) {
    if ([[maybe_unused]] !listener) {
        THEMIS_WARN([[maybe_unused]] "CacheReplicationManager::addReplica: null listener ignored");
        return;
    }

    const std::string id = listener->replicaId();

    {
        std::lock_guard<std::mutex> lock(replicas_mutex_);

        // Remove any existing entry with the same ID so re-registrations are
        // idempotent.
        replicas_.erase(
            std::remove_if(replicas_.begin(), replicas_.end(),
                           [&id]([[maybe_unused]] const CacheReplicaState &s) { return s.listener && s.listener->replicaId() == id; }),
            replicas_.end());

        CacheReplicaState state;
        state.listener     = std::move([[maybe_unused]] listener);
        state.health       = CacheReplicaHealth::HEALTHY;
        state.last_success = std::chrono::steady_clock::now();
        replicas_.push_back(std::move(state));
    }

    THEMIS_INFO("CacheReplicationManager: added replica '{}'", id);

    // Bootstrap with a snapshot if provided.
    if (!snapshot_ndjson.empty()) {
        CacheReplicationEvent ev = makeEvent([[maybe_unused]] CacheReplicationEventType::SNAPSHOT);
        ev.payload               = snapshot_ndjson;
        dispatch(ev);
        stats_.snapshots_sent++;
        THEMIS_INFO("CacheReplicationManager: sent bootstrap snapshot to '{}'", id);
    }
}

void CacheReplicationManager::removeReplica(const std::string &replica_id) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    replicas_.erase(std::remove_if(replicas_.begin(), replicas_.end(),
                                   [&replica_id](const CacheReplicaState &s) {
                                       return s.listener && s.listener->replicaId() == replica_id;
                                   }),
                    replicas_.end());
    THEMIS_INFO("CacheReplicationManager: removed replica '{}'", replica_id);
}

size_t CacheReplicationManager::replicaCount() const {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    return static_cast<int>(replicas_.size());
}

// ---------------------------------------------------------------------------
// Health probing
// ---------------------------------------------------------------------------

void CacheReplicationManager::probeUnhealthyReplicas() {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    uint64_t unhealthy_count = 0;

    for (auto &state : replicas_) {
        if (state.health == CacheReplicaHealth::UNHEALTHY) {
            bool alive = false;
            try {
                alive = state.listener->ping();
            } catch (const std::exception& ex) {
                THEMIS_WARN("CacheReplicationManager: ping exception from replica '{}': {}",
                            state.listener->replicaId(), ex.what());
                alive = false;
            } catch (const std::string& ex) {
                THEMIS_WARN("CacheReplicationManager: ping exception from replica '{}': {}",
                            state.listener->replicaId(), ex);
                alive = false;
            } catch (const char* ex) {
                THEMIS_WARN("CacheReplicationManager: ping exception from replica '{}': {}",
                            state.listener->replicaId(), (ex ? ex : "<null>"));
                alive = false;
            }

            if (alive) {
                state.health               = CacheReplicaHealth::HEALTHY;
                state.consecutive_failures = 0;
                state.last_success         = std::chrono::steady_clock::now();
                THEMIS_INFO("CacheReplicationManager: replica '{}' recovered", state.listener->replicaId());
            } else {
                ++unhealthy_count;
            }
        } else if (state.health == CacheReplicaHealth::DEGRADED) {
            ++unhealthy_count;
        }
    }

    stats_.replicas_unhealthy.store(unhealthy_count);
}

// ---------------------------------------------------------------------------
// ICacheReplicationListener – fan-out implementation
// ---------------------------------------------------------------------------

bool CacheReplicationManager::onReplicationEvent([[maybe_unused]] const CacheReplicationEvent &event) {
    if (!config_.enabled) {
        return true;
    }
    dispatch([[maybe_unused]] event);
    return true;
}

bool CacheReplicationManager::ping() {
    return true;
}

// ---------------------------------------------------------------------------
// Convenience helpers
// ---------------------------------------------------------------------------

void CacheReplicationManager::notifyWrite(const std::string &key, const std::string &payload,
                                          const std::string &tenant_id, int ttl_seconds) {
    if (!config_.enabled) {
        return;
    }

    CacheReplicationEvent ev = makeEvent([[maybe_unused]] CacheReplicationEventType::WRITE);
    ev.key                   = key;
    ev.payload               = payload;
    ev.tenant_id             = tenant_id;
    ev.ttl_seconds           = ttl_seconds;
    dispatch(ev);
}

void CacheReplicationManager::notifyInvalidate(const std::string &pattern) {
    if (!config_.enabled) {
        return;
    }

    CacheReplicationEvent ev = makeEvent([[maybe_unused]] CacheReplicationEventType::INVALIDATE);
    ev.pattern               = pattern;
    dispatch(ev);
}

void CacheReplicationManager::notifyInvalidateTenant(const std::string &tenant_id) {
    if (!config_.enabled) {
        return;
    }

    CacheReplicationEvent ev = makeEvent([[maybe_unused]] CacheReplicationEventType::INVALIDATE_TENANT);
    ev.tenant_id             = tenant_id;
    dispatch(ev);
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

nlohmann::json CacheReplicationManager::getStats() const {
    auto j             = stats_.toJson();
    j["enabled"]       = config_.enabled;
    j["semi_sync"]     = config_.semi_sync;
    j["replica_count"] = replicaCount();
    return j;
}

nlohmann::json CacheReplicationManager::getReplicaHealth() const {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &state : replicas_) {
        nlohmann::json r;
        r["replica_id"]           = state.listener ? state.listener->replicaId() : "(null)";
        r["health"]               = healthToString(state.health);
        r["consecutive_failures"] = state.consecutive_failures;
        r["events_sent"]          = state.events_sent;
        r["events_failed"]        = state.events_failed;
        arr.push_back(std::move(r));
    }
    return arr;
}

// ---------------------------------------------------------------------------
// Internal dispatch
// ---------------------------------------------------------------------------

void CacheReplicationManager::dispatch([[maybe_unused]] const CacheReplicationEvent &event) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);

    bool any_success         = false;
    uint64_t unhealthy_count = 0;

    for (auto &state : replicas_) {
        if (state.health == CacheReplicaHealth::UNHEALTHY) {
            ++unhealthy_count;
            continue; // Skip until probed healthy again
        }

        bool ok = false;
        try {
            ok = state.listener->onReplicationEvent([[maybe_unused]] event);
        } catch (const std::exception &ex) {
            THEMIS_WARN("CacheReplicationManager: exception from replica '{}': {}", state.listener->replicaId(),
                        ex.what());
            ok = false;
        } catch (const std::string &ex) {
            THEMIS_WARN("CacheReplicationManager: exception from replica '{}': {}",
                        state.listener->replicaId(), ex);
            ok = false;
        } catch (const char *ex) {
            THEMIS_WARN("CacheReplicationManager: exception from replica '{}': {}",
                        state.listener->replicaId(), (ex ? ex : "<null>"));
            ok = false;
        }

        if (ok) {
            state.consecutive_failures = 0;
            state.last_success         = std::chrono::steady_clock::now();
            state.health               = CacheReplicaHealth::HEALTHY;
            state.events_sent++;
            any_success = true;
        } else {
            state.consecutive_failures++;
            state.last_failure = std::chrono::steady_clock::now();
            state.events_failed++;
            stats_.events_failed++;

            if (state.consecutive_failures >= config_.max_consecutive_failures) {
                if (state.health != CacheReplicaHealth::UNHEALTHY) {
                    state.health = CacheReplicaHealth::UNHEALTHY;
                    THEMIS_WARN("CacheReplicationManager: replica '{}' marked UNHEALTHY "
                                "after {} consecutive failures",
                                state.listener->replicaId(), state.consecutive_failures);
                }
                ++unhealthy_count;
            } else {
                state.health = CacheReplicaHealth::DEGRADED;
            }
        }
    }

    stats_.events_dispatched++;
    stats_.replicas_unhealthy.store(unhealthy_count);

    // In semi-sync mode, warn if no replica acknowledged.
    if (config_.semi_sync && !replicas_.empty() && !any_success) {
        THEMIS_WARN("CacheReplicationManager: semi-sync: no replica acknowledged event seq={}", event.sequence);
    }
}

CacheReplicationEvent CacheReplicationManager::makeEvent([[maybe_unused]] CacheReplicationEventType type) const {
    CacheReplicationEvent ev;
    ev.type         = type;
    ev.timestamp_ms = nowMs();
    ev.sequence     = ++sequence_;
    return ev;
}

} // namespace cache
} // namespace themis
