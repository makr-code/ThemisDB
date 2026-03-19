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
        if (!step.forward) {
            return SagaOrchestratorStatus::Error(
                "step '" + step.name + "' has no forward callable");
        }
    }

    // Validate depends_on references
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            if (names.find(dep) == names.end()) {
                return SagaOrchestratorStatus::Error(
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
            // Each future returns its StepState; status_rec is updated in this
            // (calling) thread only, eliminating any data race on step_states.
            std::vector<std::future<StepState>> futures;
            futures.reserve(wave.size());

            for (const auto& step_name : wave) {
                const SAGAStep* step_ptr = step_map.at(step_name);
                futures.push_back(std::async(std::launch::async,
                    [this, step_ptr, &saga]() -> StepState {
                        return executeStep(*step_ptr, saga.id, config_);
                    }
                ));
            }

            for (size_t i = 0; i < wave.size(); ++i) {
                StepState s = futures[i].get();
                const std::string& sname = wave[i];
                status_rec.step_states[sname] = s;  // safe: calling thread only
                if (s == StepState::FAILED) {
                    saga_failed = true;
                    fail_reason = "step '" + sname + "' failed";
                } else {
                    if (s == StepState::COMPLETED) {
                        executed_order.push_back(sname);
                    }
                }
            }
        } else {
            // ── Sequential wave execution ────────────────────────────────────
            for (const auto& step_name : wave) {
                const SAGAStep* step_ptr = step_map.at(step_name);
                StepState s = executeStep(*step_ptr, saga.id, config_);
                status_rec.step_states[step_name] = s;
                if (s == StepState::FAILED) {
                    saga_failed = true;
                    fail_reason = "step '" + step_name + "' failed";
                    break;
                }
                if (s == StepState::COMPLETED) {
                    executed_order.push_back(step_name);
                }
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
            ++metrics_.sagas_failed;
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

StepState SAGAOrchestrator::executeStep(const SAGAStep& step,
                                         const std::string& saga_id,
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
            return StepState::SKIPPED; // Skipped counts as success for dependency resolution
        }
    }

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
                // Use a detached thread + promise so that timeout does not
                // block in the future destructor (std::launch::async would
                // block on destruction if the task hasn't finished yet).
                // NOTE: timeout is best-effort; the step task continues running
                // detached after a timeout is declared.
                std::promise<void> prom;
                auto fut = prom.get_future();
                std::thread([fwd = step.forward, p = std::move(prom)]() mutable {
                    try {
                        fwd();
                        p.set_value();
                    } catch (...) {
                        try { p.set_exception(std::current_exception()); }
                        catch (...) {} // set_exception() itself can throw if the
                                       // promise was already satisfied (future_error);
                                       // swallow to prevent terminate()
                    }
                }).detach();

                if (fut.wait_for(timeout) != std::future_status::ready) {
                    last_error = "step '" + step.name + "' timed out after "
                                 + std::to_string(timeout.count()) + "ms";
                    THEMIS_WARN("SAGAOrchestrator: {}", last_error);
                    // Don't retry on timeout by default; treat as terminal
                    break;
                }
                fut.get(); // re-throws any stored exception
            } else {
                step.forward();
            }

            // Success
            THEMIS_DEBUG("SAGAOrchestrator: step '{}' COMPLETED", step.name);
            return StepState::COMPLETED;

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
    THEMIS_ERROR("SAGAOrchestrator: step '{}' FAILED: {}", step.name, last_error);
    journalWrite(saga_id, "step_failed", step.name + ": " + last_error);
    return StepState::FAILED;
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

namespace {
/// Escape a string value for inclusion in a JSON object field.
std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    // 6 chars for "\uXXXX" + null terminator = 7 bytes; 8 gives alignment
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}
} // anonymous namespace

void SAGAOrchestrator::journalWrite(const std::string& saga_id,
                                     const std::string& event,
                                     const std::string& detail) {
    if (config_.journal_path.empty()) return;

    // Serialize journal writes: concurrent execute() calls must not interleave
    // output lines or produce partial JSONL records.
    std::lock_guard<std::mutex> lk(journal_mutex_);

    try {
        std::ofstream ofs(config_.journal_path, std::ios::app);
        if (!ofs.is_open()) return;

        // Well-formed JSON line with properly escaped field values
        ofs << "{\"saga_id\":\"" << jsonEscape(saga_id) << "\""
            << ",\"event\":\"" << jsonEscape(event) << "\"";
        if (!detail.empty()) {
            ofs << ",\"detail\":\"" << jsonEscape(detail) << "\"";
        }
        ofs << "}\n";
    } catch (...) {
        // Journal errors are non-fatal
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
