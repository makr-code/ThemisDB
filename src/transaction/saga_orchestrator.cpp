/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_orchestrator.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-30 04:21:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     442                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b792a6ae  2026-03-20  Refactor saga orchestrator, add compute types ║
    • efdbcc2fc  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 770a20ebf  2026-03-18  Fix all 14 review issues in SAGAOrchestrator: data race, ... ║
    • 1135d6917  2026-03-17  feat(transaction): implement SAGAOrchestrator v1.8.0 with... ║
    • 0e8ba0fbc  2026-03-16  Changes before error encountered         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    std::string out;
    out.reserve(input.size() + 8);
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
    if (saga.id.empty()) {
        return SagaOrchestratorStatus::Error("saga id must not be empty");
    }

    std::set<std::string> names;
    for (const auto& step : saga.steps) {
        if (step.name.empty()) {
            return SagaOrchestratorStatus::Error("saga contains step with empty name");
        }
        if (!names.insert(step.name).second) {
            return SagaOrchestratorStatus::Error("duplicate step name: " + step.name);
        }
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
    if (order.size() != saga.steps.size()) {
        return SagaOrchestratorStatus::Error("dependency cycle detected");
    }

    return SagaOrchestratorStatus::OK();
}

void SAGAOrchestrator::registerTemplate(const std::string& template_name, SAGADefinition tmpl) {
    std::lock_guard<std::mutex> lk(templates_mutex_);
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
    std::ostringstream oss;
    oss << "SAGA: " << saga.name << "\n";
    oss << "----------------------------------------\n";

    std::unordered_map<std::string, std::vector<std::string>> dependents;
    for (const auto& step : saga.steps) {
        dependents.emplace(step.name, std::vector<std::string>{});
    }
    for (const auto& step : saga.steps) {
        for (const auto& dep : step.depends_on) {
            dependents[dep].push_back(step.name);
        }
    }

    for (const auto& step : saga.steps) {
        const auto it = dependents.find(step.name);
        if (it == dependents.end() || it->second.empty()) {
            oss << step.name << " (terminal)\n";
            continue;
        }

        oss << step.name << " -> ";
        for (size_t i = 0; i < it->second.size(); ++i) {
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

    std::queue<std::string> q;
    for (const auto& kv : indegree) {
        if (kv.second == 0) {
            q.push(kv.first);
        }
    }

    std::vector<std::string> order;
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

    if (order.size() != saga.steps.size()) {
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

    const auto timeout = effectiveTimeout(step, cfg);
    auto delay = effectiveDelay(step, cfg);

    for (size_t attempt = 0; attempt <= step.max_retries; ++attempt) {
        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.total_step_executions;
            if (attempt > 0) {
                ++metrics_.total_step_retries;
            }
        }

        try {
            if (timeout.count() > 0) {
                auto fut = std::async(std::launch::async, [&step]() { step.forward(); });
                if (fut.wait_for(timeout) == std::future_status::timeout) {
                    throw std::runtime_error("step timed out");
                }
                fut.get();
            } else {
                step.forward();
            }

            return StepState::COMPLETED;
        } catch (const std::exception& ex) {
            journalWrite(saga_id, "step_exception", step.name + ": " + ex.what());
        } catch (...) {
            journalWrite(saga_id, "step_exception", step.name + ": unknown exception");
        }

        if (attempt < step.max_retries) {
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, std::chrono::milliseconds(30000));
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
    const auto validation = validate(saga);
    if (!validation.ok) {
        return validation;
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_started;
    }

    SAGAExecutionStatus status_rec;
    status_rec.saga_id = saga.id;
    status_rec.saga_name = saga.name;

    for (const auto& step : saga.steps) {
        status_rec.step_states[step.name] = StepState::PENDING;
    }

    const auto start_time = std::chrono::steady_clock::now();
    const auto order = topologicalSort(saga);
    const auto step_map = buildStepMap(saga);

    std::vector<std::string> executed;
    std::string failure_reason;

    for (const auto& name : order) {
        auto it = step_map.find(name);
        if (it == step_map.end()) {
            failure_reason = "unknown step in execution order: " + name;
            break;
        }

        const SAGAStep& step = *it->second;
        status_rec.step_states[name] = StepState::RUNNING;

        const StepState st = executeStep(step, saga.id, config_);
        status_rec.step_states[name] = st;

        if (st == StepState::COMPLETED || st == StepState::SKIPPED) {
            executed.push_back(name);
            continue;
        }

        failure_reason = "step failed: " + name;
        break;
    }

    const auto end_time = std::chrono::steady_clock::now();
    status_rec.total_duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    status_rec.completed_steps = 0;
    status_rec.failed_steps = 0;
    status_rec.pending_steps = 0;
    status_rec.skipped_steps = 0;
    for (const auto& kv : status_rec.step_states) {
        switch (kv.second) {
            case StepState::COMPLETED:
            case StepState::COMPENSATED:
                ++status_rec.completed_steps;
                break;
            case StepState::FAILED:
                ++status_rec.failed_steps;
                break;
            case StepState::SKIPPED:
                ++status_rec.skipped_steps;
                break;
            case StepState::PENDING:
            case StepState::RUNNING:
            case StepState::COMPENSATING:
                ++status_rec.pending_steps;
                break;
        }
    }

    if (!failure_reason.empty()) {
        status_rec.failure_reason = failure_reason;
        journalWrite(saga.id, "saga_compensating", failure_reason);
        compensateAll(saga, step_map, executed, status_rec);

        {
            std::lock_guard<std::mutex> lk(metrics_mutex_);
            ++metrics_.sagas_failed;
            ++metrics_.sagas_compensated;
        }

        {
            std::lock_guard<std::mutex> lk(status_mutex_);
            statuses_[saga.id] = status_rec;
        }

        journalWrite(saga.id, "saga_failed", failure_reason);
        return SagaOrchestratorStatus::Error(failure_reason);
    }

    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        ++metrics_.sagas_completed;
    }

    {
        std::lock_guard<std::mutex> lk(status_mutex_);
        statuses_[saga.id] = status_rec;
    }

    journalWrite(saga.id, "saga_completed");
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

} // namespace themis
