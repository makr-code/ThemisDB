/**
 * @file saga_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=33, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/saga_orchestrator.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace themis {
namespace {

std::string jsonEscape(const std::string& input) {
    std::string out = {};
    out.reserve(static_cast<int>(input.size()) + 8);
    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::chrono::milliseconds effectiveDelay(const SAGAStep& step,
                                         const SAGAOrchestrator::Config& cfg) {
    return (step.retry_delay.count() > 0) ? step.retry_delay : cfg.default_retry_delay;
}

std::chrono::milliseconds effectiveTimeout(const SAGAStep& step,
                                           const SAGAOrchestrator::Config& cfg) {
    return (step.timeout.count() > 0) ? step.timeout : cfg.default_timeout;
}

} // namespace

SAGAOrchestrator::SAGAOrchestrator(Config config)
    : config_(std::move(config)) {}

SagaOrchestratorStatus SAGAOrchestrator::validate(const SAGADefinition& saga) const {
    if (saga.name.empty()) {
        return SagaOrchestratorStatus::Error("saga name must not be empty");
    }

    if (saga.steps.empty()) {
        return SagaOrchestratorStatus::Error("saga must contain at least one step");
    }

    std::set<std::string> names = {};

    for (const auto& step : saga.steps) {
        if (step.name.empty()) {
            return SagaOrchestratorStatus::Error("saga contains step with empty name");
        }
        if (!step.forward) {
            return SagaOrchestratorStatus::Error(
                "step '" + step.name + "' must define a forward action");
        }
        if (!names.insert(step.name).second) {
            return SagaOrchestratorStatus::Error("duplicate step name: " + step.name);
        }
    }

    if (saga.id.empty()) {
        return SagaOrchestratorStatus::Error("saga id must not be empty");
    }

    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            if (names.find(dep) == names.end()) {
                return SagaOrchestratorStatus::Error(
                    "step '" + step.name + "' depends on unknown step '" + dep + "'");
            }
        }
    }

    const auto order = topologicalSort(saga);
    if (static_cast<int>(order.size()) != static_cast<int>(saga.steps.size())) {
        return SagaOrchestratorStatus::Error("dependency cycle detected");
    }

    return SagaOrchestratorStatus::OK();
}

void SAGAOrchestrator::registerTemplate(const std::string& template_name, SAGADefinition tmpl) {
    std::lock_guard<std::mutex> lk(templates_mutex_);
    // Sprint 8 Phase 1 (GAP A-3): Template is moved to map storage.
    // Subsequent access uses map lookup (instantiateTemplate), never direct reference to moved object.
    // Pattern: Move to map, retrieve by key; never access moved object.
    templates_[template_name] = std::move(tmpl);
}

SAGADefinition SAGAOrchestrator::instantiateTemplate(
    const std::string& template_name,
    const std::string& instance_id,
    std::map<std::string, std::string> context_overrides) const {

    std::lock_guard<std::mutex> lk(templates_mutex_);
    auto it = templates_.find(template_name);
    if (it == templates_.end()) {
        throw std::out_of_range("saga template not found: " + template_name);
    }

    SAGADefinition out = it->second;
    out.id = instance_id;
    for (auto& kv : context_overrides) {
        out.context[kv.first] = std::move(kv.second);
    }
    return out;
}

std::string SAGAOrchestrator::renderWorkflow(const SAGADefinition& saga) const {
    std::ostringstream oss = {};
    oss << "SAGA: " << saga.name << "\n";
    oss << "----------------------------------------\n";

    // Build dependents map: for each step, which steps depend on it
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    for (const auto& step : saga.steps) {
        // Initialize entry for all step names
        dependents.emplace(step.name, std::vector<std::string>{});
    }
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            dependents[dep].push_back(step.name);
        }
    }

    // Render workflow with dependency information
    for (const auto& step : saga.steps) {
        auto it = dependents.find(step.name);
        if (it == dependents.end() || it->second.empty()) {
            oss << step.name << " (terminal)\n";
            continue;
        }

        oss << step.name << " -> ";
        for (size_t i = 0; i < it-> static_cast<int>(second.size()); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << it->second[i];
        }
        oss << "\n";
    }

    oss << "----------------------------------------\n";
    return oss.str();
}

std::vector<std::string> SAGAOrchestrator::topologicalSort(const SAGADefinition& saga) const {
    std::unordered_map<std::string, size_t> indegree;
    std::unordered_map<std::string, std::vector<std::string>> outgoing;

    indegree.reserve(saga.steps.size());
    outgoing.reserve(saga.steps.size());

    for (const auto& step : saga.steps) {
        indegree[step.name] = 0;
        outgoing[step.name] = {};
    }

    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            ++indegree[step.name];
            outgoing[dep].push_back(step.name);
        }
    }

    std::queue<std::string> q = {};

    for (const auto& kv : indegree) {
        if (kv.second == 0) {
            q.push(kv.first);
        }
    }

    std::vector<std::string> order = {};

    order.reserve(saga.steps.size());

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        order.push_back(current);

        for (const auto& next : outgoing[current]) {
            size_t& deg = indegree[next];
            if (--deg == 0) {
                q.push(next);
            }
        }
    }

    if (static_cast<int>(order.size()) != static_cast<int>(saga.steps.size())) {
        return {};
    }
    return order;
}

SAGAOrchestrator::StepMap SAGAOrchestrator::buildStepMap(const SAGADefinition& saga) {
    StepMap map;
    map.reserve(saga.steps.size());
    for (const auto& step : saga.steps) {
        map.emplace(step.name, &step);
    }
    return map;
}

StepState SAGAOrchestrator::executeStep(const SAGAStep& step,
                                        const std::string& saga_id,
                                        const Config& cfg) {
    if (step.condition && !step.condition()) {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.total_steps_skipped;
        return StepState::SKIPPED;
    }

    // AC-9/AC-10: Check circuit breaker before attempting step
    if (isCircuitBreakerOpen(step.name)) {
        journalWrite(saga_id, "circuit_breaker_open", 
                    "Circuit breaker is open for step: " + step.name);
        return StepState::FAILED;
    }

    const auto timeout = effectiveTimeout(step, cfg);
    auto delay = effectiveDelay(step, cfg);

    for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
        // AC-9/AC-10: Check circuit breaker on each retry attempt
        if (attempt > 0 && isCircuitBreakerOpen(step.name)) {
            journalWrite(saga_id, "circuit_breaker_open_on_retry",
                        "Circuit breaker opened during retries for: " + step.name);
            recordCircuitBreakerFailure(step.name);
            return StepState::FAILED;
        }

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_step_executions;
            if (attempt > 0) {
                ++metrics_.total_step_retries;
            }
        }

        try {
            if (timeout.count() > 0) {
                auto fut = std::async(std::launch::async, step.forward);
                if (fut.wait_for(timeout) == std::future_status::timeout) {
                    throw std::runtime_error("step timed out");
                }
                fut.get();
            } else {
                step.forward();
            }

            // Step succeeded: reset circuit breaker
            recordCircuitBreakerSuccess(step.name);
            return StepState::COMPLETED;
        } catch (const std::exception& ex) {
            journalWrite(saga_id, "step_exception", step.name + ": " + ex.what());
            recordCircuitBreakerFailure(step.name);
        } catch (const std::string& ex) {
            journalWrite(saga_id, "step_exception", step.name + ": " + ex);
            recordCircuitBreakerFailure(step.name);
        } catch (const char* ex) {
            journalWrite(saga_id,
                         "step_exception",
                         step.name + ": " + std::string(ex ? ex : "<null>"));
            recordCircuitBreakerFailure(step.name);
        } catch (...) {
            journalWrite(saga_id, "step_exception", step.name + ": unknown exception");
            recordCircuitBreakerFailure(step.name);
        }

        if (attempt < step.max_retries && !isCircuitBreakerOpen(step.name)) {
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, std::chrono::milliseconds(30000));
        } else if (isCircuitBreakerOpen(step.name)) {
            // Circuit breaker opened; stop retrying
            break;
        }
    }

    return StepState::FAILED;
}

void SAGAOrchestrator::compensateAll(const SAGADefinition&,
                                     const StepMap& step_map,
                                     const std::vector<std::string>& executed_order,
                                     SAGAExecutionStatus& status_rec) {
    for (auto it = executed_order.rbegin(); it != executed_order.rend(); ++it) {
        auto step_it = step_map.find(*it);
        if (step_it == step_map.end()) {
            continue;
        }
        compensateStep(*step_it->second, status_rec);
    }
}

void SAGAOrchestrator::compensateStep(const SAGAStep& step,
                                      SAGAExecutionStatus& status_rec) {
    status_rec.step_states[step.name] = StepState::COMPENSATING;

    if (!step.compensate) {
        status_rec.step_states[step.name] = StepState::COMPENSATED;
        return;
    }

    try {
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_compensations;
        }
        step.compensate();
        status_rec.step_states[step.name] = StepState::COMPENSATED;
    } catch (const std::exception& ex) {
        status_rec.failure_reason += " | compensation failed for " + step.name + ": " + ex.what();
        status_rec.step_states[step.name] = StepState::FAILED;
    } catch (const std::string& ex) {
        status_rec.failure_reason += " | compensation failed for " + step.name + ": " + ex;
        status_rec.step_states[step.name] = StepState::FAILED;
    } catch (const char* ex) {
        status_rec.failure_reason +=
            " | compensation failed for " + step.name + ": " + std::string(ex ? ex : "<null>");
        status_rec.step_states[step.name] = StepState::FAILED;
    } catch (...) {
        status_rec.failure_reason += " | compensation failed for " + step.name + ": unknown exception";
        status_rec.step_states[step.name] = StepState::FAILED;
    }
}

void SAGAOrchestrator::journalWrite(const std::string& saga_id,
                                    const std::string& event,
                                    const std::string& detail) {
    if (config_.journal_path.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lk(journal_mutex_);

    std::ofstream out(config_.journal_path, std::ios::app);
    if (!out.is_open()) {
        return;
    }

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    out << "{\"ts_ms\":" << ms
        << ",\"saga_id\":\"" << jsonEscape(saga_id)
        << "\",\"event\":\"" << jsonEscape(event)
        << "\",\"detail\":\"" << jsonEscape(detail)
        << "\"}\n";
}

SagaOrchestratorStatus SAGAOrchestrator::execute(const SAGADefinition& saga) {
    SAGADefinition saga_exec = saga;
    if (saga_exec.id.empty()) {
        static std::atomic<uint64_t> next_id{0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto seq = next_id.fetch_add(1, std::memory_order_relaxed);
        saga_exec.id = "saga-auto-" + std::to_string(now) + "-" + std::to_string(seq);
    }

    const auto validation = validate(saga_exec);
    if (!validation.ok) {
        return validation;
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
    }

    SAGAExecutionStatus status_rec;
    status_rec.saga_id = saga_exec.id;
    status_rec.saga_name = saga_exec.name;

    for (const auto& step : saga_exec.steps) {
        status_rec.step_states[step.name] = StepState::PENDING;
    }

    auto recomputeStatusCounters = [&status_rec]() {
        status_rec.completed_steps = 0;
        status_rec.failed_steps = 0;
        status_rec.pending_steps = 0;
        status_rec.skipped_steps = 0;
        for (const auto& kv : status_rec.step_states) {
            switch (kv.second) {
                case StepState::COMPLETED:
                [[fallthrough]];\n                case StepState::COMPENSATED:
                    ++status_rec.completed_steps;
                    break;
                case StepState::FAILED:
                    ++status_rec.failed_steps;
                    break;
                case StepState::SKIPPED:
                    ++status_rec.skipped_steps;
                    break;
                case StepState::PENDING:
                [[fallthrough]];\n                case StepState::RUNNING:
                [[fallthrough]];\n                case StepState::COMPENSATING:
                    ++status_rec.pending_steps;
                    break;
            }
        }
    };

    const auto start_time = std::chrono::steady_clock::now();
    const auto order = topologicalSort(saga_exec);
    const auto step_map = buildStepMap(saga_exec);

    journalWrite(saga_exec.id, "saga_started", saga_exec.name);

    std::unordered_map<std::string, size_t> remaining_dependencies;
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    remaining_dependencies.reserve(saga_exec.steps.size());
    dependents.reserve(saga_exec.steps.size());
    for (const auto& step : saga_exec.steps) {
        remaining_dependencies[step.name] = step.depends_on.size();
        dependents[step.name] = {};
    }
    for (const auto& step : saga_exec.steps) {
        for (const auto& dep : step.depends_on) {
            dependents[dep].push_back(step.name);
        }
    }

    const bool allow_parallel = config_.enable_parallel && saga_exec.enable_parallel;
    std::vector<std::string> ready = {};

    ready.reserve(saga.steps.size());
    for (const auto& name : order) {
        if (remaining_dependencies[name] == 0) {
            ready.push_back(name);
        }
    }

    std::vector<std::string> executed;
    std::string failure_reason = {};

    while (!ready.empty() && failure_reason.empty()) {
        std::vector<std::string> wave = std::move(ready);

        for (const auto& name : wave) {
            status_rec.step_states[name] = StepState::RUNNING;
        }

        std::vector<std::pair<std::string, StepState>> results;
        results.reserve(wave.size());

        if (allow_parallel && static_cast<int>(wave.size()) > 1) {
            std::vector<std::future<std::pair<std::string, StepState>>> futures;
            futures.reserve(wave.size());
            for (const auto& name : wave) {
                auto it = step_map.find(name);
                if (it == step_map.end()) {
                    failure_reason = "unknown step in execution order: " + name;
                    break;
                }

                const SAGAStep& step = *it->second;
                futures.push_back(std::async(
                    std::launch::async,
                    [this, &step, &saga_exec, name]() {
                        return std::make_pair(name, executeStep(step, saga_exec.id, config_));
                    }));
            }

            for (auto& future : futures) {
                try {
                    results.push_back(future.get());
                } catch (const std::exception& ex) {
                    results.push_back({std::string{}, StepState::FAILED});
                    if (failure_reason.empty()) {
                        failure_reason = std::string("wave step threw: ") + ex.what();
                    }
                } catch (...) {
                    results.push_back({std::string{}, StepState::FAILED});
                    if (failure_reason.empty()) {
                        failure_reason = "wave step threw unknown exception";
                    }
                }
            }
        } else {
            for (const auto& name : wave) {
                auto it = step_map.find(name);
                if (it == step_map.end()) {
                    failure_reason = "unknown step in execution order: " + name;
                    break;
                }

                const SAGAStep& step = *it->second;
                results.emplace_back(name, executeStep(step, saga_exec.id, config_));
            }
        }

        std::vector<std::string> succeeded_in_wave = {};

        succeeded_in_wave.reserve(results.size());
        for (const auto& [name, state] : results) {
            status_rec.step_states[name] = state;

            if (state == StepState::COMPLETED) {
                executed.push_back(name);
                succeeded_in_wave.push_back(name);
                continue;
            }

            if (state == StepState::SKIPPED) {
                succeeded_in_wave.push_back(name);
                continue;
            }

            if (failure_reason.empty()) {
                failure_reason = "step failed: " + name;
            }
        }

        if (!failure_reason.empty()) {
            break;
        }

        for (const auto& name : succeeded_in_wave) {
            for (const auto& dependent : dependents[name]) {
                size_t& count = remaining_dependencies[dependent];
                if (count > 0) {
                    --count;
                    if (count == 0) {
                        ready.push_back(dependent);
                    }
                }
            }
        }
    }

    if (failure_reason.empty()) {
        for (const auto& kv : status_rec.step_states) {
            if (kv.second == StepState::PENDING || kv.second == StepState::RUNNING) {
                failure_reason = "saga execution stalled before all steps completed";
                break;
            }
        }
    }

    const auto end_time = std::chrono::steady_clock::now();
    status_rec.total_duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    recomputeStatusCounters();

    if (!failure_reason.empty()) {
        status_rec.failure_reason = failure_reason;
        journalWrite(saga_exec.id, "saga_compensating", failure_reason);
        compensateAll(saga_exec, step_map, executed, status_rec);
        recomputeStatusCounters();

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_failed;
            ++metrics_.sagas_compensated;
        }

        {
            std::lock_guard<std::mutex> lk(status_mutex_);
            statuses_[saga_exec.id] = status_rec;
        }

        journalWrite(saga_exec.id, "saga_failed", failure_reason);
        return SagaOrchestratorStatus::Error(failure_reason);
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_completed;
    }

    {
        std::lock_guard<std::mutex> lk(status_mutex_);
        statuses_[saga_exec.id] = status_rec;
    }

    journalWrite(saga_exec.id, "saga_completed");
    return SagaOrchestratorStatus::OK();
}

std::optional<SAGAExecutionStatus>
SAGAOrchestrator::getStatus(const std::string& saga_id) const {
    std::lock_guard<std::mutex> lk(status_mutex_);
    auto it = statuses_.find(saga_id);
    if (it == statuses_.end()) {
        return std::nullopt;
    }
    return it->second;
}

SAGAOrchestrator::Metrics SAGAOrchestrator::getMetrics() const {
    std::lock_guard<std::mutex> lk(metrics_mutex_);
    return metrics_;
}

bool SAGAOrchestrator::isCircuitBreakerOpen(const std::string& step_name) const {
    std::lock_guard<std::mutex> lk(circuit_breaker_mutex_);
    
    auto it = consecutive_failures_.find(step_name);
    if (it == consecutive_failures_.end() || it->second < config_.circuit_breaker_threshold) {
        return false;  // Circuit is CLOSED
    }

    // Circuit is OPEN; check if enough time has passed to try HALF_OPEN
    auto time_it = last_failure_time_.find(step_name);
    if (time_it != last_failure_time_.end()) {
        const auto now = std::chrono::system_clock::now();
        const auto elapsed = now - time_it->second;
        if (elapsed >= config_.circuit_breaker_timeout) {
            // Timeout expired; allow one retry (HALF_OPEN state)
            return false;
        }
    }

    return true;  // Circuit remains OPEN
}

void SAGAOrchestrator::recordCircuitBreakerFailure(const std::string& step_name) {
    std::lock_guard<std::mutex> lk(circuit_breaker_mutex_);
    
    auto& count = consecutive_failures_[step_name];
    ++count;
    last_failure_time_[step_name] = std::chrono::system_clock::now();
}

void SAGAOrchestrator::recordCircuitBreakerSuccess(const std::string& step_name) {
    std::lock_guard<std::mutex> lk(circuit_breaker_mutex_);
    
    // Reset failure counter on success
    consecutive_failures_[step_name] = 0;
}

} // namespace themis


