/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_orchestrator.cpp                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-17                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~430                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "transaction/saga_orchestrator.h"
#include "utils/logger.h"

#include <algorithm>
#include <future>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// validate()
// ─────────────────────────────────────────────────────────────────────────────

SAGAStatus SAGAOrchestrator::validate(const SAGADefinition& saga) const {
    if (saga.name.empty()) {
        return SAGAStatus::Error("SAGA name must not be empty");
    }
    if (saga.steps.empty()) {
        return SAGAStatus::Error("SAGA must contain at least one step");
    }

    // Check for duplicate step names and missing forward actions
    std::unordered_set<std::string> names;
    for (const auto& step : saga.steps) {
        if (step.name.empty()) {
            return SAGAStatus::Error("step name must not be empty");
        }
        if (!names.insert(step.name).second) {
            return SAGAStatus::Error("duplicate step name: " + step.name);
        }
        if (!step.forward) {
            return SAGAStatus::Error(
                "step '" + step.name + "' has no forward action");
        }
    }

    // Verify all depends_on references exist
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            if (names.find(dep) == names.end()) {
                return SAGAStatus::Error(
                    "step '" + step.name + "' depends on unknown step '" + dep + "'");
            }
        }
    }

    // Cycle detection via DFS on the dependency graph.
    // Build adjacency list: dep → steps that depend on it
    std::unordered_map<std::string, std::vector<std::string>> adj;
    for (const auto& step : saga.steps) {
        adj[step.name]; // ensure node exists even with no outgoing edges
        for (const auto& dep : step.depends_on) {
            adj[dep].push_back(step.name);
        }
    }

    enum class Color { WHITE, GRAY, BLACK };
    std::unordered_map<std::string, Color> color;
    for (const auto& name : names) { color[name] = Color::WHITE; }

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
            return SAGAStatus::Error(
                "dependency cycle detected in SAGA '" + saga.name + "'");
        }
    }

    return SAGAStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildWaves()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::vector<std::string>>
SAGAOrchestrator::buildWaves(const SAGADefinition& saga) const {
    // Assign each step a "level" equal to (1 + max level of its dependencies).
    // Steps with no dependencies are at level 0.
    std::unordered_map<std::string, int> level;

    // Build a map for dependency lookup
    std::unordered_map<std::string, const Step*> step_ptr;
    for (const auto& step : saga.steps) {
        step_ptr[step.name] = &step;
        level[step.name]    = -1; // unprocessed sentinel
    }

    // Topological order via Kahn's algorithm
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> adj;
    for (const auto& step : saga.steps) {
        in_degree[step.name]; // ensure key exists
        adj[step.name];
        for (const auto& dep : step.depends_on) {
            adj[dep].push_back(step.name);
            ++in_degree[step.name];
        }
    }

    std::vector<std::string> queue;
    for (const auto& step : saga.steps) {
        if (in_degree[step.name] == 0) {
            queue.push_back(step.name);
            level[step.name] = 0;
        }
    }

    std::vector<std::string> topo;
    size_t head = 0;
    while (head < queue.size()) {
        const std::string& u = queue[head++];
        topo.push_back(u);
        for (const auto& v : adj[u]) {
            level[v] = std::max(level[v], level[u] + 1);
            if (--in_degree[v] == 0) {
                queue.push_back(v);
            }
        }
    }

    if (topo.size() != saga.steps.size()) {
        // Cycle detected — return empty to signal error
        return {};
    }

    // Group steps by level into waves
    int max_level = 0;
    for (const auto& [name, lvl] : level) {
        max_level = std::max(max_level, lvl);
    }

    std::vector<std::vector<std::string>> waves(max_level + 1);
    for (const auto& [name, lvl] : level) {
        waves[lvl].push_back(name);
    }
    return waves;
}

// ─────────────────────────────────────────────────────────────────────────────
// executeStep() — single step with retry and timeout
// ─────────────────────────────────────────────────────────────────────────────

SAGAStatus SAGAOrchestrator::executeStep(const Step& step, size_t& retry_count) {
    auto deadline = std::chrono::steady_clock::now() + step.timeout;
    std::chrono::milliseconds backoff = step.retry_delay;
    constexpr std::chrono::milliseconds kMaxBackoff{30'000};

    size_t attempt = 0;
    while (true) {
        // Check wall-clock timeout before each attempt
        if (std::chrono::steady_clock::now() >= deadline) {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_timeout_aborts;
            return SAGAStatus::Error(
                "step '" + step.name + "' timed out after " +
                std::to_string(step.timeout.count()) + " ms");
        }

        // Launch the forward action in a std::async task so we can enforce
        // the remaining deadline with wait_for.
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_timeout_aborts;
            return SAGAStatus::Error(
                "step '" + step.name + "' timed out (no remaining budget)");
        }

        std::exception_ptr exc;
        auto fut = std::async(std::launch::async, [&step, &exc]() {
            try {
                step.forward();
            } catch (...) {
                exc = std::current_exception();
            }
        });

        auto wait_status = fut.wait_for(remaining);
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_step_executions;
        }

        if (wait_status == std::future_status::timeout) {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_timeout_aborts;
            return SAGAStatus::Error(
                "step '" + step.name + "' timed out after " +
                std::to_string(step.timeout.count()) + " ms");
        }

        // Forward completed within time budget — check for exception
        if (!exc) {
            return SAGAStatus::OK();
        }

        // Step threw — decide whether to retry
        ++attempt;
        if (attempt > step.max_retries) {
            std::string err_msg;
            try { std::rethrow_exception(exc); }
            catch (const std::exception& e) { err_msg = e.what(); }
            catch (...) { err_msg = "(unknown exception)"; }
            return SAGAStatus::Error(
                "step '" + step.name + "' failed after " +
                std::to_string(attempt) + " attempt(s): " + err_msg);
        }

        ++retry_count;
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_retries;
        }
        THEMIS_WARN("SAGAOrchestrator: step '{}' attempt {}/{} failed, retrying in {} ms",
                    step.name, attempt, step.max_retries + 1, backoff.count());
        std::this_thread::sleep_for(backoff);
        backoff = std::min(backoff * 2, kMaxBackoff);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// compensateSteps()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::compensateSteps(
    const std::map<std::string, const Step*>& step_map,
    const std::vector<std::string>&           executed_order,
    ExecutionStatus&                          status)
{
    // Compensate in reverse execution order
    for (auto it = executed_order.rbegin(); it != executed_order.rend(); ++it) {
        const std::string& name = *it;
        auto map_it = step_map.find(name);
        if (map_it == step_map.end()) continue;

        const Step* step = map_it->second;
        updateStepState(status.saga_name, name, StepState::COMPENSATING);
        status.step_states[name] = StepState::COMPENSATING;

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_compensations;
        }

        if (step->compensate) {
            try {
                step->compensate();
            } catch (const std::exception& e) {
                THEMIS_WARN("SAGAOrchestrator: compensation for step '{}' threw: {}",
                            name, e.what());
            } catch (...) {
                THEMIS_WARN("SAGAOrchestrator: compensation for step '{}' threw unknown exception",
                            name);
            }
        }

        updateStepState(status.saga_name, name, StepState::COMPENSATED);
        status.step_states[name] = StepState::COMPENSATED;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// updateStepState()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::updateStepState(const std::string& saga_id,
                                       const std::string& step_name,
                                       StepState          state)
{
    std::lock_guard<std::mutex> lk(status_mutex_);
    auto& s = statuses_[saga_id];
    s.step_states[step_name] = state;

    // Recompute aggregate counters
    size_t completed = 0, failed = 0, pending = 0;
    for (const auto& [n, st] : s.step_states) {
        if (st == StepState::COMPLETED) ++completed;
        else if (st == StepState::FAILED) ++failed;
        else if (st == StepState::PENDING) ++pending;
    }
    s.completed_steps = completed;
    s.failed_steps    = failed;
    s.pending_steps   = pending;
}

// ─────────────────────────────────────────────────────────────────────────────
// execute()
// ─────────────────────────────────────────────────────────────────────────────

SAGAStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
    // -- Validate -------------------------------------------------------
    auto vst = validate(saga);
    if (!vst.ok) {
        THEMIS_ERROR("SAGAOrchestrator[{}]: validation failed: {}", saga.name, vst.message);
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
        ++metrics_.sagas_failed;
        return vst;
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
    }

    THEMIS_INFO("SAGAOrchestrator[{}]: starting ({} steps, parallel={})",
                saga.name, saga.steps.size(), saga.enable_parallel);

    // -- Initialise execution status -----------------------------------
    ExecutionStatus status;
    status.saga_name    = saga.name;
    status.pending_steps = saga.steps.size();
    for (const auto& step : saga.steps) {
        status.step_states[step.name] = StepState::PENDING;
    }
    {
        std::lock_guard<std::mutex> lk(status_mutex_);
        statuses_[saga.name] = status;
    }

    // -- Build step pointer map for compensation -----------------------
    std::map<std::string, const Step*> step_map;
    for (const auto& step : saga.steps) {
        step_map[step.name] = &step;
    }

    // -- Build waves (topological sort) --------------------------------
    auto waves = buildWaves(saga);
    if (waves.empty()) {
        // Should not happen after validate(), but guard defensively
        auto err = SAGAStatus::Error("internal: dependency cycle in SAGA '" + saga.name + "'");
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_failed;
        return err;
    }

    // -- Execute waves in order ----------------------------------------
    std::vector<std::string> executed_order;
    bool        failed = false;
    std::string failure_reason;

    for (const auto& wave : waves) {
        if (failed) break;

        if (saga.enable_parallel && wave.size() > 1) {
            // ── Parallel wave execution ───────────────────────────────
            struct WaveResult {
                std::string name;
                SAGAStatus  status;
                size_t      retries{0};
            };

            std::vector<std::future<WaveResult>> futures;
            futures.reserve(wave.size());

            for (const std::string& name : wave) {
                updateStepState(saga.name, name, StepState::RUNNING);

                futures.push_back(std::async(std::launch::async,
                    [this, &step_map, &name]() -> WaveResult {
                        WaveResult result;
                        result.name = name;
                        result.retries = 0;
                        result.status  = executeStep(*step_map.at(name), result.retries);
                        return result;
                    }));
            }

            for (auto& fut : futures) {
                auto result = fut.get();
                if (result.status.ok) {
                    updateStepState(saga.name, result.name, StepState::COMPLETED);
                    executed_order.push_back(result.name);
                } else {
                    updateStepState(saga.name, result.name, StepState::FAILED);
                    if (!failed) {
                        failed         = true;
                        failure_reason = result.status.message;
                    }
                }
            }
        } else {
            // ── Sequential wave execution ─────────────────────────────
            for (const std::string& name : wave) {
                if (failed) break;

                updateStepState(saga.name, name, StepState::RUNNING);
                size_t retries = 0;
                auto   st      = executeStep(*step_map.at(name), retries);

                if (st.ok) {
                    updateStepState(saga.name, name, StepState::COMPLETED);
                    executed_order.push_back(name);
                } else {
                    updateStepState(saga.name, name, StepState::FAILED);
                    failed         = true;
                    failure_reason = st.message;
                }
            }
        }
    }

    // -- Compensation on failure ----------------------------------------
    if (failed) {
        THEMIS_WARN("SAGAOrchestrator[{}]: compensating {} steps: {}",
                    saga.name, executed_order.size(), failure_reason);

        {
            // Refresh local status snapshot before compensation
            std::lock_guard<std::mutex> lk(status_mutex_);
            status = statuses_[saga.name];
        }

        compensateSteps(step_map, executed_order, status);

        {
            std::lock_guard<std::mutex> lk(status_mutex_);
            statuses_[saga.name] = status;
        }
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_compensated;
            ++metrics_.sagas_failed;
        }

        THEMIS_INFO("SAGAOrchestrator[{}]: compensation complete", saga.name);
        return SAGAStatus::Error(failure_reason);
    }

    THEMIS_INFO("SAGAOrchestrator[{}]: completed successfully ({} steps)",
                saga.name, executed_order.size());

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_completed;
    }

    return SAGAStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// getStatus()
// ─────────────────────────────────────────────────────────────────────────────

SAGAOrchestrator::ExecutionStatus
SAGAOrchestrator::getStatus(const std::string& saga_id) const {
    std::lock_guard<std::mutex> lk(status_mutex_);
    auto it = statuses_.find(saga_id);
    if (it == statuses_.end()) {
        return {};
    }
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// registerTemplate() / getTemplate()
// ─────────────────────────────────────────────────────────────────────────────

void SAGAOrchestrator::registerTemplate(SAGADefinition templ) {
    std::lock_guard<std::mutex> lk(templates_mutex_);
    templates_[templ.name] = std::move(templ);
}

std::optional<SAGAOrchestrator::SAGADefinition>
SAGAOrchestrator::getTemplate(const std::string& name) const {
    std::lock_guard<std::mutex> lk(templates_mutex_);
    auto it = templates_.find(name);
    if (it == templates_.end()) return std::nullopt;
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// getMetrics()
// ─────────────────────────────────────────────────────────────────────────────

SAGAOrchestrator::Metrics SAGAOrchestrator::getMetrics() const {
    std::lock_guard<std::mutex> lk(metrics_mutex_);
    return metrics_;
}

} // namespace themis
