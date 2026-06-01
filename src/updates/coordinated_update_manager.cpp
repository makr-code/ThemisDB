/*
 * ThemisDB | File: coordinated_update_manager.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 388
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=8, M=3, L=0
 * PR History (last 5): #4347 docs(updates): reality-chec... (2026-03-20) | #3661 feat(updates): build system... (2026-03-12) | #3422 Implement CoordinatedUpdate... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "updates/coordinated_update_manager.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CoordinatedUpdateManager::CoordinatedUpdateManager(
    std::shared_ptr<HotReloadEngine> engine,
    const CoordinatedUpdateConfig& config)
    : engine_(std::move(engine))
    , config_(config)
{
    if (!engine_) {
        throw std::invalid_argument(
            "CoordinatedUpdateManager: engine must not be null");
    }
    if (config_.nodes.empty()) {
        throw std::invalid_argument(
            "CoordinatedUpdateManager: node list must not be empty");
    }
    if (config_.local_node_id.empty()) {
        throw std::invalid_argument(
            "CoordinatedUpdateManager: local_node_id must not be empty");
    }

    // Copy nodes and normalise sequence numbers:
    // When leader_last is requested, ensure the leader node receives the
    // highest sequence number regardless of what the caller supplied.
    sorted_nodes_ = config_.nodes;

    if (config_.leader_last) {
        // Find the leader (if any is explicitly marked).
        bool has_explicit_leader = false;
        for (const auto& n : sorted_nodes_) {
            if (n.is_leader) {
                has_explicit_leader = true;
                break;
            }
        }
        if (has_explicit_leader) {
            // Assign sequence numbers so that non-leaders come first (0 …
            // N-2) and the leader is last (N-1).
            uint32_t seq = 0;
            for (auto& n : sorted_nodes_) {
                if (!n.is_leader) {
                    n.sequence_number = seq++;
                }
            }
            for (auto& n : sorted_nodes_) {
                if (n.is_leader) {
                    n.sequence_number = seq++;
                }
            }
        }
    }

    // Sort by sequence_number.
    std::sort(sorted_nodes_.begin(), sorted_nodes_.end(),
              [](const NodeDescriptor& a, const NodeDescriptor& b) {
                  return a.sequence_number < b.sequence_number;
              });

    // Validate that local_node_id is present.
    bool found = false;
    for (const auto& n : sorted_nodes_) {
        if (n.node_id == config_.local_node_id) {
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::invalid_argument(
            "CoordinatedUpdateManager: local_node_id '" +
            config_.local_node_id + "' not found in node list");
    }

    // Build initial per-node status entries.
    node_statuses_.reserve(sorted_nodes_.size());
    for (const auto& n : sorted_nodes_) {
        NodeUpdateStatus s;
        s.node_id         = n.node_id;
        s.sequence_number = n.sequence_number;
        s.is_local        = (n.node_id == config_.local_node_id);
        s.state           = NodeUpdateState::PENDING;
        node_statuses_.push_back(std::move(s));
    }

    LOG_INFO("CoordinatedUpdateManager initialised: local_node={}, "
             "sequence={}/{}, version={}",
             config_.local_node_id,
             localSequenceNumber(),
             static_cast<uint32_t>(sorted_nodes_.size()),
             config_.version);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

NodeUpdateStatus* CoordinatedUpdateManager::localStatus() {
    for (auto& s : node_statuses_) {
        if (s.is_local) return &s;
    }
    return nullptr; // unreachable after construction
}

const NodeUpdateStatus* CoordinatedUpdateManager::localStatus() const {
    for (const auto& s : node_statuses_) {
        if (s.is_local) return &s;
    }
    return nullptr;
}

const NodeDescriptor* CoordinatedUpdateManager::localDescriptor() const {
    for (const auto& n : sorted_nodes_) {
        if (n.node_id == config_.local_node_id) return &n;
    }
    return nullptr;
}

const NodeDescriptor* CoordinatedUpdateManager::predecessorDescriptor() const {
    const NodeDescriptor* local = localDescriptor();
    if (!local || local->sequence_number == 0) return nullptr;

    for (const auto& n : sorted_nodes_) {
        if (n.sequence_number == local->sequence_number - 1) return &n;
    }
    return nullptr;
}

void CoordinatedUpdateManager::reportProgress(const std::string& message) {
    ProgressCallback cb;
    uint32_t done = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cb = progress_cb_;
        for (const auto& s : node_statuses_) {
            if (s.state == NodeUpdateState::COMPLETED ||
                s.state == NodeUpdateState::FAILED    ||
                s.state == NodeUpdateState::ROLLED_BACK) {
                ++done;
            }
        }
    }
    if (cb) {
        try {
            cb(done, static_cast<uint32_t>(sorted_nodes_.size()), message);
        } catch (const std::exception& e) {
            LOG_WARN("CoordinatedUpdateManager: progress callback threw: {}",
                     e.what());
        }
    }
}

// ---------------------------------------------------------------------------
// Core update operation
// ---------------------------------------------------------------------------

CoordinatedUpdateResult CoordinatedUpdateManager::applyLocalUpdate() {
    CoordinatedUpdateResult result;
    result.success = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_rolled_back_) {
            result.error_message =
                "CoordinatedUpdateManager: cannot apply after rollback";
            result.node_statuses = node_statuses_;
            LOG_WARN("CoordinatedUpdateManager: applyLocalUpdate rejected – "
                     "already rolled back");
            return result;
        }
        NodeUpdateStatus* ls = localStatus();
        if (ls->state == NodeUpdateState::COMPLETED) {
            result.error_message =
                "CoordinatedUpdateManager: local update already applied";
            result.node_statuses = node_statuses_;
            result.nodes_updated = 1;
            result.success = true;
            return result;
        }
        ls->state = NodeUpdateState::IN_PROGRESS;
    }

    const NodeDescriptor* pred = predecessorDescriptor();

    // Step 1: Wait for the preceding node to complete (if there is one).
    if (pred) {
        WaitForPreviousFunc wait_fn;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            wait_fn = wait_for_previous_fn_;
        }

        LOG_INFO("CoordinatedUpdateManager: waiting for predecessor node={}",
                 pred->node_id);
        reportProgress("Waiting for node " + pred->node_id);

        bool predecessor_ready = true;
        if (wait_fn) {
            predecessor_ready = wait_fn(pred->node_id, config_.wait_timeout);
        }

        if (!predecessor_ready) {
            std::lock_guard<std::mutex> lock(mutex_);
            localStatus()->state = NodeUpdateState::FAILED;
            localStatus()->error_message =
                "Timed out waiting for predecessor node " + pred->node_id;
            result.error_message = localStatus()->error_message;
            result.node_statuses = node_statuses_;
            result.nodes_failed  = 1;
            LOG_ERROR("CoordinatedUpdateManager: {}", result.error_message);
            return result;
        }
    }

    // Step 2: Apply the update via HotReloadEngine.
    LOG_INFO("CoordinatedUpdateManager: applying update version={} on node={}",
             config_.version, config_.local_node_id);
    reportProgress("Applying update on node " + config_.local_node_id);

    ReloadResult reload = engine_->applyHotReload(config_.version);

    // Step 3: Record result and optionally rollback on failure.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        NodeUpdateStatus* ls = localStatus();

        if (reload.success) {
            ls->state       = NodeUpdateState::COMPLETED;
            ls->rollback_id = reload.rollback_id;
            local_rollback_id_ = reload.rollback_id;
            result.nodes_updated = 1;
            result.success       = true;

            LOG_INFO("CoordinatedUpdateManager: update applied on node={}, "
                     "rollback_id={}",
                     config_.local_node_id, reload.rollback_id);
        } else {
            ls->state         = NodeUpdateState::FAILED;
            ls->error_message = reload.error_message;
            result.error_message = reload.error_message;
            result.nodes_failed  = 1;

            LOG_ERROR("CoordinatedUpdateManager: update failed on node={}: {}",
                      config_.local_node_id, reload.error_message);
        }

        result.node_statuses = node_statuses_;
    }

    // Step 4: Signal the next node (regardless of success/failure so that
    // the rest of the cluster can react appropriately).
    SignalReadyFunc signal_fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        signal_fn = signal_ready_fn_;
    }
    if (signal_fn) {
        try {
            signal_fn(config_.local_node_id, reload.success);
        } catch (const std::exception& e) {
            LOG_WARN("CoordinatedUpdateManager: signalReady callback threw: {}",
                     e.what());
        }
    }

    // Step 5: Auto-rollback on failure if configured.
    if (!reload.success && config_.rollback_on_failure) {
        rollback("automatic rollback after update failure");
        std::lock_guard<std::mutex> lock(mutex_);
        result.node_statuses  = node_statuses_;
        result.nodes_rolled_back = 1;
        result.nodes_failed      = 0;
    }

    reportProgress(result.success
                   ? "Update completed on node " + config_.local_node_id
                   : "Update failed on node " + config_.local_node_id);

    return result;
}

// ---------------------------------------------------------------------------
// Rollback
// ---------------------------------------------------------------------------

bool CoordinatedUpdateManager::rollback(const std::string& reason) {
    std::string rid;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_rolled_back_) {
            LOG_WARN("CoordinatedUpdateManager: already rolled back");
            return false;
        }

        rid             = local_rollback_id_;
        is_rolled_back_ = true;

        NodeUpdateStatus* ls = localStatus();
        if (ls) {
            if (ls->state == NodeUpdateState::COMPLETED ||
                ls->state == NodeUpdateState::FAILED) {
                ls->state = NodeUpdateState::ROLLED_BACK;
            }
        }
    }

    const std::string log_reason = reason.empty() ? "manual rollback" : reason;
    LOG_WARN("CoordinatedUpdateManager: rolling back node={} – reason: {}",
             config_.local_node_id, log_reason);

    if (!rid.empty()) {
        return engine_->rollback(rid);
    }

    LOG_WARN("CoordinatedUpdateManager: no rollback_id available; "
             "nothing to undo at the engine level");
    return true;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

uint32_t CoordinatedUpdateManager::localSequenceNumber() const {
    const NodeDescriptor* d = localDescriptor();
    return d ? d->sequence_number : 0;
}

uint32_t CoordinatedUpdateManager::totalNodes() const {
    return static_cast<uint32_t>(sorted_nodes_.size());
}

bool CoordinatedUpdateManager::isLeader() const {
    const NodeDescriptor* d = localDescriptor();
    return d && d->is_leader;
}

std::vector<NodeUpdateStatus> CoordinatedUpdateManager::nodeStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return node_statuses_;
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void CoordinatedUpdateManager::setWaitForPreviousFunc(WaitForPreviousFunc fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    wait_for_previous_fn_ = std::move(fn);
}

void CoordinatedUpdateManager::setSignalReadyFunc(SignalReadyFunc fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    signal_ready_fn_ = std::move(fn);
}

void CoordinatedUpdateManager::setProgressCallback(ProgressCallback fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_cb_ = std::move(fn);
}

} // namespace updates
} // namespace themis
