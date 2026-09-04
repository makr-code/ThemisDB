/**
 * @file saga.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/saga.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {

Saga::~Saga() {
    if (!compensated_ && !steps_.empty()) {
        THEMIS_WARN("Saga destructed without compensation - auto-compensating {} steps",static_cast<int>(steps_.size()));
        compensate();
    }
}

void Saga::addStep(std::string operation_name, CompensatingAction compensate) {
    steps_.emplace_back(std::move(operation_name), std::move(compensate));
    THEMIS_DEBUG("SAGA: Added step '{}' (total steps: {})", steps_.back().operation_name,static_cast<int>(steps_.size()));
}

void Saga::compensate() {
    if (compensated_) {
        THEMIS_WARN("SAGA: Already compensated, skipping");
        return;
    }
    
    THEMIS_INFO("SAGA: Compensating {} steps in reverse order",static_cast<int>(steps_.size()));
    
    // Execute compensating actions in reverse order
    for (auto it = steps_.rbegin(); it != steps_.rend(); ++it) {
        if (it->compensated) {
            THEMIS_DEBUG("SAGA: Step '{}' already compensated, skipping", it->operation_name);
            continue;
        }
        
        try {
            THEMIS_DEBUG("SAGA: Compensating step '{}'", it->operation_name);
            it->compensate();
            it->compensated = true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("SAGA: Compensation failed for '{}': {}", it->operation_name, e.what());
            // Continue with other compensations
        } catch (const std::string& e) {
            THEMIS_ERROR("SAGA: Compensation failed for '{}': {}", it->operation_name, e);
        } catch (const char* e) {
            THEMIS_ERROR("SAGA: Compensation failed for '{}': {}",
                         it->operation_name,
                         (e ? e : "<null>"));
        }
    }
    
    compensated_ = true;
    THEMIS_INFO("SAGA: Compensation complete ({}/{} steps)", compensatedCount(),static_cast<int>(steps_.size()));
}

void Saga::clear() {
    THEMIS_DEBUG("SAGA: Clearing {} steps",static_cast<int>(steps_.size()));
    steps_.clear();
    compensated_ = false;
    // Note: metrics_failed_ and metrics_retried_ are intentionally preserved across
    // clear() so that getMetrics() reflects the cumulative lifetime counts.
    // They are only reset by the default constructor (i.e., when a new Saga is created).
}

void Saga::trimToSize(size_t n) {
    if (n >= static_cast<int>(steps_.size())) {
      return;
    }
    THEMIS_DEBUG("SAGA: Trimming from {} to {} steps (discarding {} steps)",
                 steps_.size(), n, static_cast<int>(steps_.size()) - n);
    steps_.erase(steps_.begin() + static_cast<ptrdiff_t>(n), steps_.end());
}

size_t Saga::compensatedCount() const {
    return std::count_if(steps_.begin(), steps_.end(), 
                        [](const Step& s) { return s.compensated; });
}

bool Saga::isFullyCompensated() const {
    return compensated_ && compensatedCount() == static_cast<int>(steps_.size());
}

std::vector<std::string> Saga::getStepHistory() const {
    std::vector<std::string> history = {};

    history.reserve(steps_.size());
    for (const auto& step : steps_) {
        std::string status = step.compensated ? "[COMPENSATED]" : "[ACTIVE]";
        history.push_back(status + " " + step.operation_name);
    }
    return history;
}

int64_t Saga::getDurationMs() const {
    if (steps_.empty()) {
      return 0;
    }
    auto now = std::chrono::system_clock::now();
    auto first_step_time = steps_[0].executed_at;
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - first_step_time).count();
}

void Saga::compensateWithRetry(int max_retries,
                               std::chrono::milliseconds backoff_ms) {
    if (compensated_) {
        THEMIS_WARN("SAGA: Already compensated, skipping");
        return;
    }

    static constexpr std::chrono::milliseconds MAX_BACKOFF{30000}; // 30 s cap

    // Clamp input backoff to avoid overflow during doubling
    backoff_ms = std::min(backoff_ms, MAX_BACKOFF);

    THEMIS_INFO("SAGA: Compensating {} steps (max_retries={}, backoff={}ms)",
                steps_.size(), max_retries, backoff_ms.count());

    for (auto it = steps_.rbegin(); it != steps_.rend(); ++it) {
        if (it->compensated) {
          continue;
        }

        bool success = false;
        std::chrono::milliseconds current_backoff = backoff_ms;

        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            try {
                if (attempt > 0) {
                    THEMIS_DEBUG("SAGA: Retrying '{}' (attempt {}/{})",
                                 it->operation_name, attempt, max_retries);
                    std::this_thread::sleep_for(current_backoff);
                    // Double backoff but cap at MAX_BACKOFF
                    current_backoff = std::min(current_backoff * 2, MAX_BACKOFF);
                }
                it->compensate();
                it->compensated = true;
                success = true;
                if (attempt > 0) {
                    ++metrics_retried_;
                }
                break;
            } catch (const std::exception& e) {
                THEMIS_WARN("SAGA: Compensation failed for '{}' (attempt {}): {}",
                            it->operation_name, attempt + 1, e.what());
            } catch (const std::string& e) {
                THEMIS_WARN("SAGA: Compensation failed for '{}' (attempt {}): {}",
                            it->operation_name, attempt + 1, e);
            } catch (const char* e) {
                THEMIS_WARN("SAGA: Compensation failed for '{}' (attempt {}): {}",
                            it->operation_name,
                            attempt + 1,
                            (e ? e : "<null>"));
            }
        }

        if (!success) {
            ++metrics_failed_;
            THEMIS_ERROR("SAGA: Compensation permanently failed for '{}' after {} attempts",
                         it->operation_name, max_retries + 1);
        }
    }

    compensated_ = true;
    THEMIS_INFO("SAGA: Retry-compensation complete – {}/{} steps, {} retried, {} failed",
                compensatedCount(),static_cast<int>(steps_.size()), metrics_retried_, metrics_failed_);
}

Saga::Metrics Saga::getMetrics() const {
    Metrics m;
    m.total_steps            = steps_.size();
    m.compensated_steps      = compensatedCount();
    m.failed_compensations   = metrics_failed_;
    m.retried_compensations  = metrics_retried_;
    m.duration_ms            = getDurationMs();
    return m;
}

// ========== SAGA Operations ==========

void SagaOperation::putEntityWithCompensation(
    RocksDBWrapper& db,
    const std::string& key,
    const std::vector<uint8_t>& value,
    Saga& saga
) {
    // Check if key exists (for idempotency)
    auto existing = db.get(key);
    
    // Compensating action: restore old value or delete
    if (existing.has_value()) {
        // Update case: restore old value
        std::vector<uint8_t> old_value = std::move(*existing);
        saga.addStep("putEntity:" + key, [&db, key, old_value]() {
            db.put(key, old_value);
            THEMIS_DEBUG("SAGA: Restored old value for key '{}'", key);
        });
    } else {
        // Insert case: delete on rollback
        saga.addStep("putEntity:" + key, [&db, key]() {
            db.del(key);
            THEMIS_DEBUG("SAGA: Deleted key '{}' (compensating insert)", key);
        });
    }
}

void SagaOperation::deleteEntityWithCompensation(
    RocksDBWrapper& db,
    const std::string& key,
    Saga& saga
) {
    // Save current value for restore
    auto existing = db.get(key);
    
    if (!existing.has_value()) {
        THEMIS_WARN("SAGA: Delete of non-existent key '{}' - no compensation needed", key);
        return;
    }
    
    std::vector<uint8_t> old_value = std::move(*existing);
    
    // Compensating action: restore deleted value
    saga.addStep("deleteEntity:" + key, [&db, key, old_value]() {
        db.put(key, old_value);
        THEMIS_DEBUG("SAGA: Restored deleted key '{}'", key);
    });
}

void SagaOperation::indexPutWithCompensation(
    SecondaryIndexManager& idx,
    const std::string& table,
    const BaseEntity& entity,
    RocksDBWrapper::WriteBatchWrapper& batch,
    Saga& saga
) {
    const std::string& pk = entity.getPrimaryKey();
    
    // Compensating action: remove from secondary index
    // Note: We don't need old values since we're just removing index entries
    saga.addStep("indexPut:" + table + ":" + pk, [&idx, table, pk]() {
        auto st = idx.erase(table, pk);
        if (!st.ok) {
            THEMIS_WARN("SAGA: Index compensation failed for '{}'::'{}': {}", table, pk, st.message);
        } else {
            THEMIS_DEBUG("SAGA: Removed secondary index entries for '{}'::'{}' (compensating indexPut)", table, pk);
        }
    });
}
void SagaOperation::graphAddWithCompensation(
    GraphIndexManager& graph,
    const BaseEntity& edge,
    RocksDBWrapper::WriteBatchWrapper& batch,
    Saga& saga
) {
    std::string edge_id = edge.getPrimaryKey();
    
    // Compensating action: delete graph edge
    saga.addStep("graphAdd:" + edge_id, [&graph, edge_id]() {
        auto st = graph.deleteEdge(edge_id);
        if (!st.ok) {
            THEMIS_WARN("SAGA: Graph compensation failed for edge '{}': {}", edge_id, st.message);
        } else {
            THEMIS_DEBUG("SAGA: Deleted graph edge '{}' (compensating graphAdd)", edge_id);
        }
    });
}

void SagaOperation::vectorAddWithCompensation(
    VectorIndexManager& vec,
    const BaseEntity& entity,
    RocksDBWrapper::WriteBatchWrapper& batch,
    const std::string& vectorField,
    Saga& saga
) {
    const std::string& pk = entity.getPrimaryKey();
    
    // Compensating action: remove from vector cache and HNSW
    saga.addStep("vectorAdd:" + pk, [&vec, pk]() {
        auto status = vec.removeByPk(pk);
        if (!status.ok) {
            THEMIS_WARN("SAGA: Vector compensation failed for '{}': {}", pk, status.message);
        } else {
            THEMIS_DEBUG("SAGA: Removed vector '{}' from cache", pk);
        }
    });
}

} // namespace themis

