/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_orchestrator.cpp                              ║
  Version:         1.8.0                                              ║
  Last Modified:   2026-03-16                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "transaction/saga_orchestrator.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <future>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

SAGAOrchestrator::SAGAOrchestrator(Config config)
    : config_(std::move(config))
{}

// ─────────────────────────────────────────────────────────────────────────────
// validate()
// ─────────────────────────────────────────────────────────────────────────────

SagaOrchestratorStatus SAGAOrchestrator::validate(const SAGADefinition& saga) const {
    if (saga.id.empty()) {
        return SagaOrchestratorStatus::Error("saga id must not be empty");
    }
    if (saga.steps.empty()) {
        return SagaOrchestratorStatus::Error("saga must have at least one step");
    }

    // Build name set and check for duplicates
    std::unordered_set<std::string> names;
    for (const auto& step : saga.steps) {
        if (step.name.empty()) {
            return SagaOrchestratorStatus::Error("all steps must have non-empty names");
        }
        if (!names.insert(step.name).second) {
            return SagaOrchestratorStatus::Error("duplicate step name: " + step.name);
        }
    }

    // Validate depends_on references
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            if (names.find(dep) == names.end()) {
                return SagaOrchestratorStatus::Error(
                    "step '" + step.name + "' depends on unknown step '" + dep + "'");
            }
        }
    }

    // Cycle detection via topological sort
    auto order = topologicalSort(saga);
    if (order.empty() && saga.steps.size() > 1) {
        return SagaOrchestratorStatus::Error("dependency cycle detected");
    }

    return SagaOrchestratorStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// execute()
// ─────────────────────────────────────────────────────────────────────────────

SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
    // Validate first
    auto val = validate(saga);
    if (!val.ok) {
        return val;
    }

    const auto start_time = std::chrono::steady_clock::now();

    // Initialise per-instance status record
    SAGAExecutionStatus status_rec;
    status_rec.saga_id   = saga.id;
    status_rec.saga_name = saga.name;
    for (const auto& step : saga.steps) {
        status_rec.step_states[step.name] = StepState::PENDING;
    }
    status_rec.pending_steps = saga.steps.size();

    // Increment sagas_started metric
    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
    }

    journalWrite(saga.id, "saga_started", saga.name);

    // Build step map for fast lookup
    StepMap step_map = buildStepMap(saga);

    // Topological sort → wave-based execution
    // Each "wave" contains steps whose dependencies are all completed.
    std::vector<std::string> topo_order = topologicalSort(saga);

    // Track execution order for compensation (LIFO)
    std::vector<std::string> executed_order;
    executed_order.reserve(saga.steps.size());

    // in_degree map for wave computation
    std::unordered_map<std::string, size_t> in_degree;
    // reverse adjacency: step → set of steps that depend on it
    std::unordered_map<std::string, std::vector<std::string>> rev_adj;
    for (const auto& step : saga.steps) {
        in_degree.emplace(step.name, step.depends_on.size());
        for (const auto& dep : step.depends_on) {
            rev_adj[dep].push_back(step.name);
        }
    }

    // BFS-style wave execution
    std::queue<std::string> ready;
    for (const auto& step : saga.steps) {
        if (step.depends_on.empty()) {
            ready.push(step.name);
        }
    }

    // Track which steps are done/skipped to resolve downstream deps
    std::unordered_set<std::string> finished_steps; // completed OR skipped
    bool saga_failed   = false;
    std::string fail_reason;

    const bool use_parallel = saga.enable_parallel && config_.enable_parallel;

    while (!ready.empty() && !saga_failed) {
        // Collect current wave
        std::vector<std::string> wave;
        while (!ready.empty()) {
            wave.push_back(ready.front());
            ready.pop();
        }

        if (use_parallel && wave.size() > 1) {
            // ── Parallel wave execution ──────────────────────────────────────
            std::vector<std::future<bool>> futures;
            futures.reserve(wave.size());

            for (const auto& step_name : wave) {
                const SAGAStep* step_ptr = step_map.at(step_name);
                futures.push_back(std::async(std::launch::async,
                    [this, step_ptr, &status_rec]() -> bool {
                        return executeStep(*step_ptr, status_rec, config_);
                    }
                ));
            }

            for (size_t i = 0; i < wave.size(); ++i) {
                bool ok = futures[i].get();
                const std::string& sname = wave[i];
                StepState s = status_rec.step_states.at(sname);
                if (!ok) {
                    saga_failed = true;
                    fail_reason = "step '" + sname + "' failed";
                } else {
                    if (s == StepState::COMPLETED) {
                        executed_order.push_back(sname);
                    }
                    finished_steps.insert(sname);
                }
            }
        } else {
            // ── Sequential wave execution ────────────────────────────────────
            for (const auto& step_name : wave) {
                const SAGAStep* step_ptr = step_map.at(step_name);
                bool ok = executeStep(*step_ptr, status_rec, config_);
                StepState s = status_rec.step_states.at(step_name);
                if (!ok) {
                    saga_failed = true;
                    fail_reason = "step '" + step_name + "' failed";
                    break;
                }
                if (s == StepState::COMPLETED) {
                    executed_order.push_back(step_name);
                }
                finished_steps.insert(step_name);
            }
        }

        if (!saga_failed) {
            // Advance in-degree counters and enqueue newly ready steps
            for (const auto& sname : wave) {
                for (const auto& dependent : rev_adj[sname]) {
                    auto& deg = in_degree[dependent];
                    if (deg > 0) {
                        --deg;
                        if (deg == 0) {
                            ready.push(dependent);
                        }
                    }
                }
            }
        }
    }

    // Finalise timing
    const auto end_time = std::chrono::steady_clock::now();
    status_rec.total_duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    if (saga_failed) {
        status_rec.failure_reason = fail_reason;
        journalWrite(saga.id, "saga_compensating", fail_reason);

        // Compensate all successfully completed steps in reverse order
        compensateAll(saga, step_map, executed_order, status_rec);

        // Recount states
        status_rec.completed_steps  = 0;
        status_rec.failed_steps     = 0;
        status_rec.pending_steps    = 0;
        status_rec.skipped_steps    = 0;
        for (const auto& [n, st] : status_rec.step_states) {
            switch (st) {
                case StepState::COMPLETED:   ++status_rec.completed_steps;  break;
                case StepState::FAILED:      ++status_rec.failed_steps;     break;
                case StepState::SKIPPED:     ++status_rec.skipped_steps;    break;
                case StepState::PENDING:     ++status_rec.pending_steps;    break;
                default: break;
            }
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_compensated;
        }

        journalWrite(saga.id, "saga_compensated");
    } else {
        // Count final states
        status_rec.completed_steps = 0;
        status_rec.failed_steps    = 0;
        status_rec.pending_steps   = 0;
        status_rec.skipped_steps   = 0;
        for (const auto& [n, st] : status_rec.step_states) {
            switch (st) {
                case StepState::COMPLETED: ++status_rec.completed_steps; break;
                case StepState::FAILED:    ++status_rec.failed_steps;    break;
                case StepState::SKIPPED:   ++status_rec.skipped_steps;   break;
                case StepState::PENDING:   ++status_rec.pending_steps;   break;
                default: break;
            }
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_completed;
        }

        journalWrite(saga.id, "saga_completed");
    }

    // Store status for getStatus()
    {
        std::lock_guard<std::mutex> lk(status_mutex_);
        statuses_[saga.id] = status_rec;
    }

    if (saga_failed) {
        return SagaOrchestratorStatus::Error(fail_reason);
    }
    return SagaOrchestratorStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// getStatus()
// ─────────────────────────────────────────────────────────────────────────────

std::optional<SAGAExecutionStatus>
SAGAOrchestrator::getStatus(const std::string& saga_id) const {
    std::lock_guard<std::mutex> lk(status_mutex_);
    auto it = statuses_.find(saga_id);
    if (it == statuses_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// getMetrics()
// ─────────────────────────────────────────────────────────────────────────────

SAGAOrchestrator::Metrics SAGAOrchestrator::getMetrics() const {
    std::lock_guard<std::mutex> lk(metrics_mutex_);
    return metrics_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Template support
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::registerTemplate(const std::string& template_name,
                                         SAGADefinition tmpl) {
    std::lock_guard<std::mutex> lk(templates_mutex_);
    templates_[template_name] = std::move(tmpl);
    THEMIS_DEBUG("SAGAOrchestrator: registered template '{}'", template_name);
}

SAGADefinition SAGAOrchestrator::instantiateTemplate(
    const std::string& template_name,
    const std::string& instance_id,
    std::map<std::string, std::string> context_overrides) const
{
    SAGADefinition tmpl_copy;
    {
        std::lock_guard<std::mutex> lk(templates_mutex_);
        auto it = templates_.find(template_name);
        if (it == templates_.end()) {
            throw std::out_of_range("SAGAOrchestrator: template '" + template_name
                                    + "' is not registered");
        }
        tmpl_copy = it->second;
    }
    tmpl_copy.id = instance_id;
    for (auto& [k, v] : context_overrides) {
        tmpl_copy.context[k] = std::move(v);
    }
    return tmpl_copy;
}

// ─────────────────────────────────────────────────────────────────────────────
// renderWorkflow()
// ─────────────────────────────────────────────────────────────────────────────

std::string SAGAOrchestrator::renderWorkflow(const SAGADefinition& saga) const {
    // Build forward adjacency: step → direct dependents
    std::unordered_map<std::string, std::vector<std::string>> fwd_adj;
    for (const auto& step : saga.steps) {
        if (fwd_adj.find(step.name) == fwd_adj.end()) {
            fwd_adj[step.name] = {};
        }
        for (const auto& dep : step.depends_on) {
            fwd_adj[dep].push_back(step.name);
        }
    }

    // Determine column width for alignment
    size_t max_name_len = 0;
    for (const auto& step : saga.steps) {
        max_name_len = std::max(max_name_len, step.name.size());
    }

    // Render
    std::ostringstream oss;
    const std::string separator(45, '-');
    oss << "SAGA: " << saga.name << "\n";
    oss << separator << "\n";

    // Use topological order for deterministic output
    std::vector<std::string> order = topologicalSort(saga);
    if (order.empty()) {
        for (const auto& step : saga.steps) {
            order.push_back(step.name);
        }
    }

    for (const auto& name : order) {
        const auto& deps = fwd_adj.at(name);
        // Pad name for alignment
        std::string padded = name;
        padded.resize(max_name_len, ' ');

        if (deps.empty()) {
            oss << padded << "  (terminal)\n";
        } else {
            for (const auto& dep : deps) {
                oss << padded << " -\u2192 " << dep << "\n";
            }
        }
    }

    oss << separator << "\n";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: topologicalSort()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string>
SAGAOrchestrator::topologicalSort(const SAGADefinition& saga) const {
    // Kahn's algorithm
    std::unordered_map<std::string, size_t> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> adj;

    for (const auto& step : saga.steps) {
        if (in_degree.find(step.name) == in_degree.end()) {
            in_degree[step.name] = 0;
        }
        for (const auto& dep : step.depends_on) {
            adj[dep].push_back(step.name);
            ++in_degree[step.name];
        }
    }

    std::queue<std::string> q;
    for (const auto& [name, deg] : in_degree) {
        if (deg == 0) q.push(name);
    }

    std::vector<std::string> sorted;
    sorted.reserve(saga.steps.size());

    while (!q.empty()) {
        std::string cur = q.front();
        q.pop();
        sorted.push_back(cur);
        for (const auto& dep : adj[cur]) {
            if (--in_degree[dep] == 0) {
                q.push(dep);
            }
        }
    }

    if (sorted.size() != saga.steps.size()) {
        // Cycle detected
        return {};
    }
    return sorted;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: buildStepMap()
// ─────────────────────────────────────────────────────────────────────────────

SAGAOrchestrator::StepMap
SAGAOrchestrator::buildStepMap(const SAGADefinition& saga) {
    StepMap m;
    m.reserve(saga.steps.size());
    for (const auto& step : saga.steps) {
        m[step.name] = &step;
    }
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: executeStep()
// ─────────────────────────────────────────────────────────────────────────────

bool SAGAOrchestrator::executeStep(const SAGAStep& step,
                                    SAGAExecutionStatus& status_rec,
                                    const Config& cfg) {
    // Evaluate condition (conditional branching)
    if (step.condition) {
        bool cond_result = false;
        try {
            cond_result = step.condition();
        } catch (const std::exception& e) {
            THEMIS_WARN("SAGAOrchestrator: condition for step '{}' threw: {}",
                        step.name, e.what());
            cond_result = false;
        }
        if (!cond_result) {
            THEMIS_DEBUG("SAGAOrchestrator: step '{}' SKIPPED (condition=false)", step.name);
            {
                std::lock_guard<std::mutex> lk(metrics_mutex_);
                ++metrics_.total_steps_skipped;
            }
            status_rec.step_states[step.name] = StepState::SKIPPED;
            return true; // Skipped counts as success for dependency resolution
        }
    }

    status_rec.step_states[step.name] = StepState::RUNNING;
    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.total_step_executions;
    }

    const std::chrono::milliseconds timeout =
        (step.timeout.count() > 0) ? step.timeout : cfg.default_timeout;
    const std::chrono::milliseconds retry_delay =
        (step.retry_delay.count() > 0) ? step.retry_delay : cfg.default_retry_delay;
    const size_t max_retries = step.max_retries;

    THEMIS_DEBUG("SAGAOrchestrator: executing step '{}' (max_retries={}, timeout={}ms)",
                 step.name, max_retries, timeout.count());

    std::chrono::milliseconds backoff = retry_delay;
    std::string last_error;

    for (size_t attempt = 0; attempt <= max_retries; ++attempt) {
        if (attempt > 0) {
            THEMIS_DEBUG("SAGAOrchestrator: retrying step '{}' attempt {}/{}",
                         step.name, attempt, max_retries);
            {
                std::lock_guard<std::mutex> lk(metrics_mutex_);
                ++metrics_.total_step_retries;
            }
            std::this_thread::sleep_for(backoff);
            // Exponential backoff, capped at 30 s
            backoff = std::min(backoff * 2, std::chrono::milliseconds(30000));
        }

        try {
            if (timeout.count() > 0) {
                // Execute via std::async to enforce timeout
                auto future = std::async(std::launch::async, step.forward);
                if (future.wait_for(timeout) == std::future_status::timeout) {
                    last_error = "step '" + step.name + "' timed out after "
                                 + std::to_string(timeout.count()) + "ms";
                    THEMIS_WARN("SAGAOrchestrator: {}", last_error);
                    // Don't retry on timeout by default; treat as terminal
                    break;
                }
                future.get(); // re-throws any stored exception
            } else {
                step.forward();
            }

            // Success
            status_rec.step_states[step.name] = StepState::COMPLETED;
            THEMIS_DEBUG("SAGAOrchestrator: step '{}' COMPLETED", step.name);
            return true;

        } catch (const std::exception& e) {
            last_error = e.what();
            THEMIS_WARN("SAGAOrchestrator: step '{}' threw on attempt {}: {}",
                        step.name, attempt + 1, last_error);
        } catch (...) {
            last_error = "unknown exception";
            THEMIS_WARN("SAGAOrchestrator: step '{}' threw unknown exception on attempt {}",
                        step.name, attempt + 1);
        }
    }

    // All attempts exhausted
    status_rec.step_states[step.name] = StepState::FAILED;
    THEMIS_ERROR("SAGAOrchestrator: step '{}' FAILED: {}", step.name, last_error);
    journalWrite(status_rec.saga_id, "step_failed",
                 step.name + ": " + last_error);
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: compensateAll()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::compensateAll(
    const SAGADefinition& saga,
    const StepMap& step_map,
    const std::vector<std::string>& executed_order,
    SAGAExecutionStatus& status_rec)
{
    // Compensate in reverse execution order
    for (auto it = executed_order.rbegin(); it != executed_order.rend(); ++it) {
        const SAGAStep* step_ptr = step_map.at(*it);
        compensateStep(*step_ptr, status_rec);
    }
    (void)saga; // suppress unused warning
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: compensateStep()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::compensateStep(const SAGAStep& step,
                                       SAGAExecutionStatus& status_rec) {
    if (!step.compensate) {
        THEMIS_DEBUG("SAGAOrchestrator: step '{}' has no compensating action (skipping)",
                     step.name);
        status_rec.step_states[step.name] = StepState::COMPENSATED;
        return;
    }

    status_rec.step_states[step.name] = StepState::COMPENSATING;
    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.total_compensations;
    }

    THEMIS_DEBUG("SAGAOrchestrator: compensating step '{}'", step.name);

    try {
        step.compensate();
        status_rec.step_states[step.name] = StepState::COMPENSATED;
        THEMIS_DEBUG("SAGAOrchestrator: compensation for '{}' succeeded", step.name);
    } catch (const std::exception& e) {
        THEMIS_ERROR("SAGAOrchestrator: compensation for '{}' threw: {}", step.name, e.what());
        // Compensation errors are logged but do not abort the compensation loop
        status_rec.step_states[step.name] = StepState::COMPENSATED; // best-effort
        journalWrite(status_rec.saga_id, "compensation_error",
                     step.name + ": " + e.what());
    } catch (...) {
        THEMIS_ERROR("SAGAOrchestrator: compensation for '{}' threw unknown exception",
                     step.name);
        status_rec.step_states[step.name] = StepState::COMPENSATED;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: journalWrite()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::journalWrite(const std::string& saga_id,
                                     const std::string& event,
                                     const std::string& detail) {
    if (config_.journal_path.empty()) return;

    try {
        std::ofstream ofs(config_.journal_path, std::ios::app);
        if (!ofs.is_open()) return;

        // Minimal JSON line
        ofs << "{\"saga_id\":\"" << saga_id << "\""
            << ",\"event\":\"" << event << "\"";
        if (!detail.empty()) {
            ofs << ",\"detail\":\"" << detail << "\"";
        }
        ofs << "}\n";
    } catch (...) {
        // Journal errors are non-fatal
    }
}

} // namespace themis
