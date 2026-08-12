/**
 * @file distributed_saga.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=2, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=4, M=21, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/distributed_saga.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaCoordinator::DistributedSagaCoordinator(Config config)
    : config_(std::move(config))
{}

// ─────────────────────────────────────────────────────────────────────────────
// validate()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaStatus DistributedSagaCoordinator::validate(
    const DistributedSagaDefinition& saga
) const {
    if (saga.saga_id.empty()) {
        return DistributedSagaStatus::Error("saga_id must not be empty");
    }
    if (saga.steps.empty()) {
        return DistributedSagaStatus::Error("SAGA must contain at least one step");
    }

    // Build name set and check for duplicates
    std::unordered_set<std::string> names;
    for (const auto& step : saga.steps) {
        if (step.name.empty()) {
            return DistributedSagaStatus::Error("step name must not be empty");
        }
        if (!names.insert(step.name).second) {
            return DistributedSagaStatus::Error("duplicate step name: " + step.name);
        }
        if (!step.forward) {
            return DistributedSagaStatus::Error(
                "step '" + step.name + "' has no forward action");
        }
    }

    // Verify all depends_on references exist
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            if (names.find(dep) == names.end()) {
                return DistributedSagaStatus::Error(
                    "step '" + step.name + "' depends on unknown step '" + dep + "'");
            }
        }
    }

    // Cycle detection via DFS
    // Build adjacency list (step → its dependencies)
    std::unordered_map<std::string, std::vector<std::string>> adj;
    for (const auto& step : saga.steps) {
        adj[step.name];
        for (const auto& dep : step.depends_on) {
            adj[dep].push_back(step.name);
        }
    }

    enum class Color { WHITE, GRAY, BLACK };
    std::unordered_map<std::string, Color> color;
    for (const auto& name : names) color[name] = Color::WHITE;

    std::function<bool(const std::string&)> dfs = [&](const std::string& u) -> bool {
        color[u] = Color::GRAY;
        for (const auto& v : adj[u]) {
            if (color[v] == Color::GRAY) return true; // back-edge → cycle
            if (color[v] == Color::WHITE && dfs(v)) return true;
        }
        color[u] = Color::BLACK;
        return false;
    };

    for (const auto& name : names) {
        if (color[name] == Color::WHITE && dfs(name)) {
            return DistributedSagaStatus::Error(
                "dependency cycle detected in SAGA '" + saga.saga_id + "'");
        }
    }

    return DistributedSagaStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// execute()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaReport DistributedSagaCoordinator::execute(
    const DistributedSagaDefinition& saga
) {
    DistributedSagaReport report;
    report.saga_id = saga.saga_id;
    report.state   = SagaExecutionState::RUNNING;

    bool duplicate_saga = false;
    {
        std::lock_guard<std::mutex> lk(reports_mutex_);
        duplicate_saga = reports_.find(saga.saga_id) != reports_.end();
    }
    if (duplicate_saga) {
        report.state = SagaExecutionState::FAILED;
        report.failure_reason =
            "Duplicate saga_id execution rejected: " + saga.saga_id;
        THEMIS_ERROR("DSAGA[{}]: duplicate saga_id rejected", saga.saga_id);
        journalWrite(saga.saga_id, "REJECTED_DUPLICATE", report.failure_reason);
        {
            std::lock_guard<std::mutex> mk(metrics_mutex_);
            ++metrics_.sagas_failed;
        }
        return report;
    }

    auto wall_start = std::chrono::system_clock::now();
    const auto deadline = (config_.saga_timeout.count() > 0)
                              ? std::optional<std::chrono::steady_clock::time_point>(
                                  std::chrono::steady_clock::now() + config_.saga_timeout)
                              : std::nullopt;

    // -- Validate -------------------------------------------------------
    auto vst = validate(saga);
    if (!vst.ok) {
        report.state          = SagaExecutionState::FAILED;
        report.failure_reason = "Validation failed: " + vst.message;
        THEMIS_ERROR("DSAGA[{}]: validation failed: {}", saga.saga_id, vst.message);
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_started;
            ++metrics_.sagas_failed;
        }
        {
            std::lock_guard<std::mutex> lk(reports_mutex_);
            reports_[saga.saga_id] = report;
        }
        return report;
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
    }

    journalWrite(saga.saga_id, "STARTED");
    THEMIS_INFO("DSAGA[{}]: starting ({} steps)", saga.saga_id, saga.steps.size());

    // -- Build step map and initialise records --------------------------
    std::map<std::string, DistributedSagaStep> step_map;
    // Build a name→record-pointer index for O(1) lookup throughout execution
    std::unordered_map<std::string, StepRecord*> record_index;
    for (const auto& step : saga.steps) {
        step_map.emplace(step.name, step);
        StepRecord rec;
        rec.name  = step.name;
        rec.phase = StepRecord::Phase::PENDING;
        report.step_records.push_back(rec);
    }
    // Populate index after all push_backs (to avoid iterator invalidation)
    for (auto& rec : report.step_records) {
        record_index[rec.name] = &rec;
    }

    // Helper: find record by name (O(1) via index)
    auto findRecord = [&](const std::string& name) -> StepRecord& {
        auto it = record_index.find(name);
        if (it == record_index.end()) {
            throw std::logic_error("Internal error: step record not found: " + name);
        }
        return *it->second;
    };

    // -- Topological sort into execution waves --------------------------
    // waves[i] contains steps that can run in parallel at wave i
    std::vector<std::string> topo = topologicalSort(saga);
    // topo is a flat order; group into waves by "level" (max dependency depth)
    std::unordered_map<std::string, int> level;
    for (const auto& name : topo) {
        int lvl = 0;
        for (const auto& dep : step_map.at(name).depends_on) {
            lvl = std::max(lvl, level[dep] + 1);
        }
        level[name] = lvl;
    }

    int max_level = 0;
    for (const auto& [name, lvl] : level) max_level = std::max(max_level, lvl);

    std::vector<std::vector<std::string>> waves(max_level + 1);
    for (const auto& [name, lvl] : level) waves[lvl].push_back(name);

    // -- Execute waves in order ----------------------------------------
    std::vector<std::string> executed_order; // names in forward-execution order
    bool failed = false;
    std::string failure_reason;

    for (auto& wave : waves) {
        if (failed) break;

        auto wst = executeWave(wave, step_map, record_index, failure_reason, deadline);
        if (!wst.ok) {
            failed = true;
            break;
        }
        // Append wave steps to executed_order
        for (const auto& name : wave) {
            if (findRecord(name).phase == StepRecord::Phase::DONE) {
                executed_order.push_back(name);
            }
        }
    }

    // -- Compensate if needed ------------------------------------------
    if (failed) {
        // Collect steps that completed before failure (O(1) set membership check)
        std::vector<std::string> to_compensate = executed_order;
        std::unordered_set<std::string> listed(
            to_compensate.begin(), to_compensate.end());

        // Also collect any DONE steps in the failed wave
        for (auto& rec : report.step_records) {
            if (rec.phase == StepRecord::Phase::DONE &&
                listed.find(rec.name) == listed.end()) {
                to_compensate.push_back(rec.name);
                listed.insert(rec.name);
            }
        }

        report.state          = SagaExecutionState::COMPENSATING;
        report.failure_reason = failure_reason;
        journalWrite(saga.saga_id, "COMPENSATING", failure_reason);
        THEMIS_WARN("DSAGA[{}]: compensating {} steps after failure: {}",
                    saga.saga_id, to_compensate.size(), failure_reason);

        compensate(step_map, to_compensate, record_index);

        // Determine final state: COMPENSATED if all compensable steps were
        // successfully compensated; FAILED if any compensation step failed.
        bool comp_ok = true;
        for (const auto& name : to_compensate) {
            auto& rec = findRecord(name);
            if (rec.phase == StepRecord::Phase::FAILED) {
                comp_ok = false;
                break;
            }
        }

        report.state = comp_ok ? SagaExecutionState::COMPENSATED
                               : SagaExecutionState::FAILED;

        journalWrite(saga.saga_id,
                     comp_ok ? "COMPENSATED" : "FAILED",
                     failure_reason);

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            if (comp_ok)  ++metrics_.sagas_compensated;
            else          ++metrics_.sagas_failed;
        }
    } else {
        report.state = SagaExecutionState::COMPLETED;
        journalWrite(saga.saga_id, "COMPLETED");
        THEMIS_INFO("DSAGA[{}]: completed successfully", saga.saga_id);
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_completed;
        }
    }

    // -- Finish timing -------------------------------------------------
    auto wall_end = std::chrono::system_clock::now();
    report.total_duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            wall_end - wall_start).count();

    {
        std::lock_guard<std::mutex> lk(reports_mutex_);
        reports_[saga.saga_id] = report;
    }
    return report;
}

// ─────────────────────────────────────────────────────────────────────────────
// topologicalSort()  — Kahn's algorithm
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> DistributedSagaCoordinator::topologicalSort(
    const DistributedSagaDefinition& saga
) const {
    std::unordered_map<std::string, int>                  in_degree;
    std::unordered_map<std::string, std::vector<std::string>> successors;

    for (const auto& step : saga.steps) {
        in_degree[step.name] += 0; // ensure entry exists
        for (const auto& dep : step.depends_on) {
            successors[dep].push_back(step.name);
            in_degree[step.name]++;
        }
    }

    // Use a sorted set as the ready queue for deterministic O(log n) extraction
    std::set<std::string> ready;
    for (const auto& [name, deg] : in_degree) {
        if (deg == 0) ready.insert(name);
    }

    std::vector<std::string> result;
    while (!ready.empty()) {
        // Extract smallest name (deterministic ordering)
        auto it   = ready.begin();
        std::string node = *it;
        ready.erase(it);
        result.push_back(node);
        for (const auto& succ : successors[node]) {
            if (--in_degree[succ] == 0) {
                ready.insert(succ);
            }
        }
    }
    return result; // empty on cycle (validate() already rejects cycles)
}

// ─────────────────────────────────────────────────────────────────────────────
// executeWave()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaStatus DistributedSagaCoordinator::executeWave(
    const std::vector<std::string>&                    wave,
    const std::map<std::string, DistributedSagaStep>&  step_map,
    RecordIndex&                                       index,
    std::string&                                       failure_reason,
    std::optional<std::chrono::steady_clock::time_point> deadline
) {
    auto dependenciesSatisfied = [&](const DistributedSagaStep& step,
                                     std::string& missing_dep) -> bool {
        for (const auto& dep : step.depends_on) {
            const auto dep_it = index.find(dep);
            if (dep_it == index.end()) {
                missing_dep = dep;
                return false;
            }
            if (dep_it->second->phase != StepRecord::Phase::DONE) {
                missing_dep = dep;
                return false;
            }
        }
        return true;
    };

    if (!config_.enable_parallel || wave.size() == 1) {
        // Sequential execution
        for (const auto& name : wave) {
            auto it = index.find(name);
            if (it == index.end()) continue;

            const auto& step = step_map.at(name);
            std::string missing_dep;
            if (!dependenciesSatisfied(step, missing_dep)) {
                failure_reason = "Step '" + name +
                                 "' violated causal dependency: '" +
                                 missing_dep + "' not completed";
                return DistributedSagaStatus::Error(failure_reason);
            }

            auto st = executeStep(step, *it->second, deadline);
            if (!st.ok) {
                failure_reason = "Step '" + name + "' failed: " + st.message;
                return st;
            }
        }
        return DistributedSagaStatus::OK();
    }

    // Parallel execution — launch all steps in this wave concurrently
    std::vector<std::future<DistributedSagaStatus>> futures;
    std::vector<StepRecord*> wave_records;

    for (const auto& name : wave) {
        auto it = index.find(name);
        StepRecord* rec = (it != index.end()) ? it->second : nullptr;
        wave_records.push_back(rec);

        const DistributedSagaStep& step = step_map.at(name);

        std::string missing_dep;
        if (!dependenciesSatisfied(step, missing_dep)) {
            failure_reason = "Step '" + name +
                             "' violated causal dependency: '" +
                             missing_dep + "' not completed";
            return DistributedSagaStatus::Error(failure_reason);
        }

        futures.push_back(
            std::async(std::launch::async,
                [this, &step, rec, deadline]() -> DistributedSagaStatus {
                    if (!rec) return DistributedSagaStatus::Error("record not found");
                    return executeStep(step, *rec, deadline);
                }
            )
        );
    }

    // Collect results
    DistributedSagaStatus wave_status;
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            auto st = futures[i].get();
            if (!st.ok && wave_status.ok) {
                wave_status    = st;
                failure_reason = "Step '" + wave[i] + "' failed: " + st.message;
            }
        } catch (const std::exception& e) {
            if (wave_status.ok) {
                wave_status    = DistributedSagaStatus::Error(e.what());
                failure_reason = "Step '" + wave[i] + "' threw: " + e.what();
            }
        } catch (...) {
            if (wave_status.ok) {
                wave_status    = DistributedSagaStatus::Error("unknown exception in step");
                failure_reason = "Step '" + wave[i] + "' threw unknown exception";
            }
        }
    }
    return wave_status;
}

// ─────────────────────────────────────────────────────────────────────────────
// executeStep()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaStatus DistributedSagaCoordinator::executeStep(
    const DistributedSagaStep& step,
    StepRecord&                record,
    std::optional<std::chrono::steady_clock::time_point> deadline
) {
    static constexpr std::chrono::milliseconds MAX_BACKOFF{30000};

    record.phase      = StepRecord::Phase::EXECUTING;
    record.started_at = std::chrono::system_clock::now();

    auto timeout = step.forward_timeout.count() > 0
                       ? step.forward_timeout
                       : config_.default_forward_timeout;

    std::chrono::milliseconds backoff = step.retry_backoff;
    backoff = std::min(backoff, MAX_BACKOFF);

    DistributedSagaStatus last_status;

    for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
        ++record.attempts;
        if (attempt > 0) {
            std::this_thread::sleep_for(backoff);
            backoff = std::min(backoff * 2, MAX_BACKOFF);
            THEMIS_DEBUG("DSAGA: retrying step '{}' (attempt {})", step.name, attempt + 1);
        }

        auto effective_timeout = timeout;
        if (deadline.has_value()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= *deadline) {
                last_status = DistributedSagaStatus::Error("saga timeout budget exhausted before step execution");
                {
                    std::lock_guard<std::mutex> lk(metrics_mutex_);
                    ++metrics_.total_timeout_aborts;
                }
                break;
            }

            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - now);
            effective_timeout = std::min(timeout, remaining);
        }

        // Run forward action inside an async task so we can enforce a timeout
        auto fut = std::async(std::launch::async, step.forward);
        auto wait_result = fut.wait_for(effective_timeout);

        if (wait_result == std::future_status::timeout) {
            last_status = DistributedSagaStatus::Error("timeout after " +
                std::to_string(effective_timeout.count()) + "ms");
            THEMIS_WARN("DSAGA: step '{}' timed out (attempt {})", step.name, attempt + 1);
            // Count timeouts in metrics
            {
                std::lock_guard<std::mutex> lk(metrics_mutex_);
                ++metrics_.total_timeout_aborts;
            }
            // No retry after timeout — treat as permanent failure
            break;
        }

        try {
            last_status = fut.get();
        } catch (const std::exception& e) {
            last_status = DistributedSagaStatus::Error(
                std::string("exception: ") + e.what());
        } catch (const std::string& e) {
            last_status = DistributedSagaStatus::Error(
                std::string("exception: ") + e);
        } catch (const char* e) {
            last_status = DistributedSagaStatus::Error(
                std::string("exception: ") + (e ? e : "<null>"));
        } catch (...) {
            last_status = DistributedSagaStatus::Error("unknown exception");
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_step_executions;
            if (attempt > 0) ++metrics_.total_step_retries;
        }

        if (last_status.ok) {
            record.phase       = StepRecord::Phase::DONE;
            record.finished_at = std::chrono::system_clock::now();
            THEMIS_DEBUG("DSAGA: step '{}' succeeded (attempt {})", step.name, attempt + 1);
            
            // QW-39: Verify distributed consensus for write durability
            // After local step succeeds, check that write was replicated to quorum
            if (config_.enable_consensus_verification) {
                std::string consensus_failure_detail;
                bool consensus_ok = verifyStepConsensus(
                    step.name,
                    step.node_id,
                    record,
                    &consensus_failure_detail);
                record.consensus_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                if (!consensus_ok) {
                    // Consensus verification failed: treat as step failure for retry
                    last_status = DistributedSagaStatus::Error(consensus_failure_detail.empty()
                        ? ("consensus verification failed for step '" + step.name + "'")
                        : consensus_failure_detail);
                    THEMIS_WARN("DSAGA: step '{}' failed consensus verification (attempt {})",
                                step.name, attempt + 1);
                    // Continue to retry loop (attempt will increment)
                    continue;
                }
                THEMIS_DEBUG("DSAGA: step '{}' consensus verified with {} acks",
                             step.name, record.ack_count);
            }
            
            return DistributedSagaStatus::OK();
        }

        THEMIS_WARN("DSAGA: step '{}' failed (attempt {}): {}",
                    step.name, attempt + 1, last_status.message);
    }

    record.phase         = StepRecord::Phase::FAILED;
    record.error_message = last_status.message;
    record.finished_at   = std::chrono::system_clock::now();
    THEMIS_ERROR("DSAGA: step '{}' permanently failed: {}", step.name, last_status.message);
    return last_status;
}

// ─────────────────────────────────────────────────────────────────────────────
// compensate()
// ─────────────────────────────────────────────────────────────────────────────

void DistributedSagaCoordinator::compensate(
    const std::map<std::string, DistributedSagaStep>& step_map,
    const std::vector<std::string>&                   executed_order,
    RecordIndex&                                      index
) {
    // Execute compensations in reverse execution order (LIFO)
    for (auto it = executed_order.rbegin(); it != executed_order.rend(); ++it) {
        const std::string& name = *it;

        auto rit = index.find(name);
        if (rit == index.end()) continue;
        StepRecord* rec = rit->second;

        // Only compensate steps that completed their forward action
        if (rec->phase != StepRecord::Phase::DONE) continue;

        const auto& step = step_map.at(name);
        if (!step.compensate) {
            // No compensating action defined — skip (treat as no-op)
            rec->phase = StepRecord::Phase::COMPENSATED;
            THEMIS_DEBUG("DSAGA: step '{}' has no compensation — skipping", name);
            continue;
        }

        rec->phase = StepRecord::Phase::COMPENSATING;
        auto st    = compensateStep(step, *rec);

        if (!st.ok) {
            THEMIS_ERROR("DSAGA: compensation of step '{}' failed: {}", name, st.message);
            rec->phase         = StepRecord::Phase::FAILED;
            rec->error_message = "compensation failed: " + st.message;
        } else {
            rec->phase = StepRecord::Phase::COMPENSATED;
            THEMIS_DEBUG("DSAGA: step '{}' compensated", name);
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_compensations;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// compensateStep()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaStatus DistributedSagaCoordinator::compensateStep(
    const DistributedSagaStep& step,
    StepRecord&                record
) {
    static constexpr std::chrono::milliseconds MAX_BACKOFF{30000};

    auto timeout = step.compensate_timeout.count() > 0
                       ? step.compensate_timeout
                       : config_.default_compensate_timeout;

    std::chrono::milliseconds backoff = step.retry_backoff;
    backoff = std::min(backoff, MAX_BACKOFF);

    DistributedSagaStatus last_status;

    for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
        ++record.comp_attempts;
        if (attempt > 0) {
            std::this_thread::sleep_for(backoff);
            backoff = std::min(backoff * 2, MAX_BACKOFF);
        }

        auto fut         = std::async(std::launch::async, step.compensate);
        auto wait_result = fut.wait_for(timeout);

        if (wait_result == std::future_status::timeout) {
            last_status = DistributedSagaStatus::Error("compensation timeout");
            {
                std::lock_guard<std::mutex> lk(metrics_mutex_);
                ++metrics_.total_timeout_aborts;
            }
            break;
        }

        try {
            last_status = fut.get();
        } catch (const std::exception& e) {
            last_status = DistributedSagaStatus::Error(
                std::string("compensation exception: ") + e.what());
        } catch (const std::string& e) {
            last_status = DistributedSagaStatus::Error(
                std::string("compensation exception: ") + e);
        } catch (const char* e) {
            last_status = DistributedSagaStatus::Error(
                std::string("compensation exception: ") + (e ? e : "<null>"));
        } catch (...) {
            last_status = DistributedSagaStatus::Error("unknown compensation exception");
        }

        if (last_status.ok) return DistributedSagaStatus::OK();

        THEMIS_WARN("DSAGA: compensation of '{}' failed (attempt {}): {}",
                    step.name, attempt + 1, last_status.message);
    }

    return last_status;
}

// ─────────────────────────────────────────────────────────────────────────────
// verifyStepConsensus() — QW-39: Distributed consensus verification
// ─────────────────────────────────────────────────────────────────────────────

bool DistributedSagaCoordinator::verifyStepConsensus(
    const std::string& step_name,
    const std::string& node_id,
    StepRecord& record,
    std::string* failure_detail)
{
    if (!config_.enable_consensus_verification) {
        record.consensus_reached = false;
        record.quorum_size = 0;
        record.ack_count = 0;
        return true;
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.consensus_checks_total;
    }

    THEMIS_DEBUG("DSAGA: verifying consensus for step '{}' on node '{}'",
                 step_name, node_id);

    if (config_.consensus_verifier) {
        ConsensusVerificationResult result;
        try {
            result = config_.consensus_verifier(step_name, node_id);
        } catch (const std::exception& e) {
            result.verified = false;
            result.detail = std::string("consensus verifier exception: ") + e.what();
        } catch (...) {
            result.verified = false;
            result.detail = "consensus verifier unknown exception";
        }

        record.quorum_size = std::max(0, result.quorum_size);
        record.ack_count = std::max(0, result.ack_count);
        if (record.quorum_size > 0 && record.ack_count > record.quorum_size) {
            record.ack_count = record.quorum_size;
        }

        const bool quorum_reached =
            (record.quorum_size > 0 && record.ack_count >= record.quorum_size);
        record.consensus_reached = result.verified && quorum_reached;

        if (!record.consensus_reached) {
            {
                std::lock_guard<std::mutex> lk(metrics_mutex_);
                ++metrics_.consensus_checks_failed;
            }
            if (failure_detail) {
                if (!result.detail.empty()) {
                    *failure_detail = result.detail;
                } else {
                    *failure_detail = "consensus verification failed for step '" +
                                      step_name + "' (acks=" + std::to_string(record.ack_count) +
                                      ", quorum=" + std::to_string(record.quorum_size) + ")";
                }
            }
        }

        return record.consensus_reached;
    }

    // Backward-compatible single-node fallback: if no external verifier is
    // configured, treat local durability as quorum (1/1).
    record.quorum_size = 1;
    record.ack_count = 1;
    record.consensus_reached = true;
    if (failure_detail) {
        failure_detail->clear();
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// getReport()
// ─────────────────────────────────────────────────────────────────────────────

std::optional<DistributedSagaReport> DistributedSagaCoordinator::getReport(
    const std::string& saga_id
) const {
    std::lock_guard<std::mutex> lk(reports_mutex_);
    auto it = reports_.find(saga_id);
    if (it == reports_.end()) return std::nullopt;
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// getMetrics()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaCoordinator::Metrics DistributedSagaCoordinator::getMetrics() const {
    std::lock_guard<std::mutex> lk(metrics_mutex_);
    return metrics_;
}

// ─────────────────────────────────────────────────────────────────────────────
// journalWrite()
// ─────────────────────────────────────────────────────────────────────────────

void DistributedSagaCoordinator::journalWrite(
    const std::string& saga_id,
    const std::string& event,
    const std::string& detail
) {
    if (config_.journal_path.empty()) return;

    try {
        std::ofstream f(config_.journal_path, std::ios::app);
        if (!f.is_open()) return;

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Simple JSON-lines format
        f << "{\"ts\":" << now_ms
          << ",\"saga_id\":\"" << saga_id << "\""
          << ",\"event\":\"" << event << "\"";
        if (!detail.empty()) {
            // Minimal escaping of double quotes in detail
            std::string escaped;
            escaped.reserve(detail.size());
            for (char c : detail) {
                if (c == '"')  escaped += "\\\"";
                else if (c == '\\') escaped += "\\\\";
                else escaped += c;
            }
            f << ",\"detail\":\"" << escaped << "\"";
        }
        f << "}\n";
    } catch (const std::exception& e) {
        // Journal write failures are non-fatal.
        THEMIS_WARN("DSAGA[{}]: journal write failed: {}", saga_id, e.what());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// remoteStepToLocal()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaStep DistributedSagaCoordinator::remoteStepToLocal(
    const RemoteStep& remote
) const {
    DistributedSagaStep local;
    local.name              = remote.name;
    local.node_id           = remote.service_endpoint;
    local.depends_on        = remote.depends_on;
    local.forward_timeout   = remote.forward_timeout;
    local.compensate_timeout = remote.compensate_timeout;
    local.max_retries       = remote.max_retries;
    local.retry_backoff     = remote.retry_backoff;

    // Capture by value so the lambdas remain valid after this stack frame
    auto executor      = config_.remote_executor;
    auto endpoint      = remote.service_endpoint;
    auto operation     = remote.operation;
    auto params        = remote.params;
    auto comp_op       = remote.compensate_operation;
    auto comp_params   = remote.compensate_params;

    if (executor) {
        local.forward = [executor, endpoint, operation, params]() {
            return executor(endpoint, operation, params);
        };
        if (!comp_op.empty()) {
            local.compensate = [executor, endpoint, comp_op, comp_params]() {
                return executor(endpoint, comp_op, comp_params);
            };
        }
    } else {
        // Defensive fallback: executeDistributed() rejects missing executor.
        local.forward = []() {
            return DistributedSagaStatus::Error("remote_executor_not_configured");
        };
        if (!comp_op.empty()) {
            local.compensate = []() {
                return DistributedSagaStatus::Error("remote_executor_not_configured");
            };
        }
    }

    return local;
}

// ─────────────────────────────────────────────────────────────────────────────
// executeDistributed()
// ─────────────────────────────────────────────────────────────────────────────

DistributedSagaReport DistributedSagaCoordinator::executeDistributed(
    const DistributedSAGADefinition& remote_saga
) {
    auto rejectDistributed = [&](const std::string& reason,
                                 const std::string& event) -> DistributedSagaReport {
        DistributedSagaReport report;
        report.saga_id = remote_saga.saga_id;
        report.state = SagaExecutionState::FAILED;
        report.failure_reason = reason;

        if (!remote_saga.saga_id.empty()) {
            journalWrite(remote_saga.saga_id,
                         event,
                         report.failure_reason);
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_failed;
        }

        if (!report.saga_id.empty()) {
            std::lock_guard<std::mutex> lk(reports_mutex_);
            reports_[report.saga_id] = report;
        }

        THEMIS_ERROR("DSAGA[{}]: executeDistributed rejected: {}",
                     remote_saga.saga_id,
                     report.failure_reason);
        return report;
    };

    if (!config_.remote_executor) {
        return rejectDistributed("remote_executor_not_configured",
                                 "REJECTED_NO_REMOTE_EXECUTOR");
    }

    if (remote_saga.saga_id.empty()) {
        return rejectDistributed("invalid_remote_saga: empty saga_id",
                                 "REJECTED_INVALID_REMOTE_SAGA");
    }

    if (remote_saga.steps.empty()) {
        return rejectDistributed("invalid_remote_saga: no remote steps defined",
                                 "REJECTED_INVALID_REMOTE_SAGA");
    }

    for (size_t i = 0; i < remote_saga.steps.size(); ++i) {
        const auto& step = remote_saga.steps[i];
        if (step.name.empty()) {
            return rejectDistributed(
                "invalid_remote_step: empty name at index " + std::to_string(i),
                "REJECTED_INVALID_REMOTE_STEP");
        }
        if (step.service_endpoint.empty()) {
            return rejectDistributed(
                "invalid_remote_step: empty service_endpoint for step '" + step.name + "'",
                "REJECTED_INVALID_REMOTE_STEP");
        }
        if (step.operation.empty()) {
            return rejectDistributed(
                "invalid_remote_step: empty operation for step '" + step.name + "'",
                "REJECTED_INVALID_REMOTE_STEP");
        }
    }

    // Convert to the canonical DistributedSagaDefinition
    DistributedSagaDefinition local_def;
    local_def.saga_id = remote_saga.saga_id;
    local_def.context = remote_saga.context;

    for (const auto& remote_step : remote_saga.steps) {
        local_def.steps.push_back(remoteStepToLocal(remote_step));
    }

    return execute(local_def);
}

// ─────────────────────────────────────────────────────────────────────────────
// getDistributedStatus()
// ─────────────────────────────────────────────────────────────────────────────

std::optional<DistributedSagaReport> DistributedSagaCoordinator::getDistributedStatus(
    const std::string& saga_id
) const {
    return getReport(saga_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// recoverInProgressSAGAs()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> DistributedSagaCoordinator::recoverInProgressSAGAs() {
    std::vector<std::string> recovered;

    if (config_.journal_path.empty()) return recovered;

    std::ifstream f(config_.journal_path);
    if (!f.is_open()) return recovered;

    // Track terminal states seen per saga_id
    std::map<std::string, std::string> latest_event; // saga_id → most recent event

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        try {
            auto entry = nlohmann::json::parse(line);
            std::string sid   = entry.value("saga_id", std::string{});
            std::string event = entry.value("event",   std::string{});
            if (!sid.empty() && !event.empty()) {
                latest_event[sid] = event;
            }
        } catch (const nlohmann::json::exception&) {
            // Malformed lines are skipped.
        }
    }

    // SAGAs whose last event is STARTED or COMPENSATING are orphaned
    for (const auto& [sid, event] : latest_event) {
        if (event == "STARTED" || event == "COMPENSATING") {
            // Record a synthetic recovery report so callers can inspect it
            DistributedSagaReport report;
            report.saga_id        = sid;
            report.state          = SagaExecutionState::FAILED;
            report.failure_reason = "recovered from incomplete state (" + event + ") after coordinator restart";

            {
                std::lock_guard<std::mutex> lk(reports_mutex_);
                // Only insert if not already present (a concurrent execute() may have
                // completed it between our journal read and now)
                reports_.emplace(sid, report);
            }

            journalWrite(sid, "RECOVERED", event);
            THEMIS_WARN("DSAGA[{}]: recovered orphaned SAGA (last state: {})", sid, event);
            recovered.push_back(sid);
        }
    }

    return recovered;
}

// ─────────────────────────────────────────────────────────────────────────────
// visualize()
// ─────────────────────────────────────────────────────────────────────────────

SagaVisualization DistributedSagaCoordinator::visualize(
    const DistributedSagaDefinition& saga
) const {
    // Retrieve any existing execution report for annotation
    std::optional<DistributedSagaReport> report_opt = getReport(saga.saga_id);

    // Build phase lookup
    std::unordered_map<std::string, std::string> phase_label;
    if (report_opt) {
        for (const auto& rec : report_opt->step_records) {
            switch (rec.phase) {
                case StepRecord::Phase::PENDING:     phase_label[rec.name] = "PENDING";     break;
                case StepRecord::Phase::EXECUTING:   phase_label[rec.name] = "EXECUTING";   break;
                case StepRecord::Phase::DONE:        phase_label[rec.name] = "DONE";        break;
                case StepRecord::Phase::COMPENSATING:phase_label[rec.name] = "COMPENSATING";break;
                case StepRecord::Phase::COMPENSATED: phase_label[rec.name] = "COMPENSATED"; break;
                case StepRecord::Phase::FAILED:      phase_label[rec.name] = "FAILED";      break;
            }
        }
    }

    // ── DOT graph ──────────────────────────────────────────────────────────
    std::ostringstream dot;
    dot << "digraph \"" << saga.saga_id << "\" {\n";
    dot << "  rankdir=LR;\n";
    dot << "  node [shape=box, style=filled, fillcolor=lightgrey];\n";

    for (const auto& step : saga.steps) {
        std::string fill = "lightgrey";
        auto it = phase_label.find(step.name);
        if (it != phase_label.end()) {
            const std::string& ph = it->second;
            if (ph == "DONE")        fill = "lightgreen";
            else if (ph == "FAILED") fill = "salmon";
            else if (ph == "COMPENSATED") fill = "lightyellow";
            else if (ph == "EXECUTING" || ph == "COMPENSATING") fill = "lightblue";
        }
        dot << "  \"" << step.name << "\" [fillcolor=" << fill;
        if (it != phase_label.end()) {
            dot << ", label=\"" << step.name << "\\n(" << it->second << ")\"";
        }
        dot << "];\n";
    }

    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            dot << "  \"" << dep << "\" -> \"" << step.name << "\";\n";
        }
    }
    dot << "}\n";

    // ── Text summary ───────────────────────────────────────────────────────
    std::ostringstream txt;
    txt << "SAGA: " << saga.saga_id << "\n";
    txt << "Steps: " << saga.steps.size() << "\n";
    if (report_opt) {
        txt << "State: ";
        switch (report_opt->state) {
            case SagaExecutionState::PENDING:     txt << "PENDING";     break;
            case SagaExecutionState::RUNNING:     txt << "RUNNING";     break;
            case SagaExecutionState::COMPLETED:   txt << "COMPLETED";   break;
            case SagaExecutionState::COMPENSATING:txt << "COMPENSATING";break;
            case SagaExecutionState::COMPENSATED: txt << "COMPENSATED"; break;
            case SagaExecutionState::FAILED:      txt << "FAILED";      break;
        }
        txt << "\n";
        txt << "Duration: " << report_opt->total_duration_ms << " ms\n";
        txt << "\nStep Details:\n";
        for (const auto& rec : report_opt->step_records) {
            txt << "  [";
            switch (rec.phase) {
                case StepRecord::Phase::DONE:        txt << "✓"; break;
                case StepRecord::Phase::FAILED:      txt << "✗"; break;
                case StepRecord::Phase::COMPENSATED: txt << "↩"; break;
                default:                             txt << "·"; break;
            }
            txt << "] " << rec.name;
            if (rec.attempts > 0) txt << " (attempts: " << rec.attempts << ")";
            if (!rec.error_message.empty()) txt << " — " << rec.error_message;
            txt << "\n";
        }
    } else {
        txt << "(Not yet executed)\n";
        txt << "\nStep Dependency Order:\n";
        for (const auto& step : saga.steps) {
            txt << "  " << step.name;
            if (!step.depends_on.empty()) {
                txt << " (after: ";
                bool first = true;
                for (const auto& d : step.depends_on) {
                    if (!first) txt << ", ";
                    txt << d;
                    first = false;
                }
                txt << ")";
            }
            txt << "\n";
        }
    }

    return SagaVisualization{dot.str(), txt.str()};
}

// ─────────────────────────────────────────────────────────────────────────────
// forceCompensate() / forceComplete()
// ─────────────────────────────────────────────────────────────────────────────

bool DistributedSagaCoordinator::forceCompensate(const std::string& saga_id) {
    std::lock_guard<std::mutex> lk(reports_mutex_);
    auto it = reports_.find(saga_id);
    if (it == reports_.end()) return false;
    it->second.state          = SagaExecutionState::COMPENSATED;
    it->second.failure_reason = "forced compensation via manual intervention";
    journalWrite(saga_id, "FORCE_COMPENSATED");
    THEMIS_WARN("DSAGA[{}]: force-compensated via manual intervention", saga_id);
    return true;
}

bool DistributedSagaCoordinator::forceComplete(const std::string& saga_id) {
    std::lock_guard<std::mutex> lk(reports_mutex_);
    auto it = reports_.find(saga_id);
    if (it == reports_.end()) return false;
    it->second.state          = SagaExecutionState::COMPLETED;
    it->second.failure_reason.clear();
    journalWrite(saga_id, "FORCE_COMPLETED");
    THEMIS_WARN("DSAGA[{}]: force-completed via manual intervention", saga_id);
    return true;
}

} // namespace themis


