#include "tensor/dream_orchestrator.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>

namespace themis {
namespace tensor {

namespace {

std::string formatUtcTimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

bool hasMeaningfulSeedSet(const std::vector<std::int64_t>& seed_set) {
    return !seed_set.empty();
}

} // namespace

nlohmann::json DreamRunDescriptor::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    j["run_id"] = run_id;
    j["created_at_utc"] = created_at_utc;
    j["updated_at_utc"] = updated_at_utc;
    j["mode"] = mode;
    j["state"] = DreamOrchestrator::toString(state);
    j["policy_version"] = policy_version;
    j["dataset_ref"] = dataset_ref;
    j["seed_set"] = seed_set;
    j["synthetic"] = synthetic;
    j["dream_mode"] = dream_mode;
    return j;
}

nlohmann::json DreamRunAuditEvent::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    j["run_id"] = run_id;
    j["timestamp_utc"] = timestamp_utc;
    j["state"] = DreamOrchestrator::toString(state);
    j["reason_class"] = DreamOrchestrator::reasonClassToString(reason_class);
    j["reason"] = reason;
    j["phase"] = phase;
    return j;
}

DreamOrchestrator::DreamOrchestrator() = default;

bool DreamOrchestrator::isAllowedTransition(DreamRunState from_state,
                                           DreamRunState to_state) noexcept {
    switch (from_state) {
        case DreamRunState::R0:
            return to_state == DreamRunState::R1;
        case DreamRunState::R1:
            return to_state == DreamRunState::R2;
        case DreamRunState::R2:
            return to_state == DreamRunState::R3;
        case DreamRunState::R3:
            return false;
    }
    return false;
}

std::string DreamOrchestrator::toString(DreamRunState state) noexcept {
    switch (state) {
        case DreamRunState::R0:
            return "R0";
        case DreamRunState::R1:
            return "R1";
        case DreamRunState::R2:
            return "R2";
        case DreamRunState::R3:
            return "R3";
    }
    return "UNKNOWN";
}

std::string DreamOrchestrator::reasonClassToString(
    DreamRunAuditReasonClass reason_class) noexcept {
    switch (reason_class) {
        case DreamRunAuditReasonClass::none:
            return "NONE";
        case DreamRunAuditReasonClass::invalid_mode:
            return "INVALID_MODE";
        case DreamRunAuditReasonClass::invalid_transition:
            return "INVALID_TRANSITION";
        case DreamRunAuditReasonClass::missing_policy_version:
            return "MISSING_POLICY_VERSION";
        case DreamRunAuditReasonClass::missing_dataset_ref:
            return "MISSING_DATASET_REF";
        case DreamRunAuditReasonClass::missing_seed_set:
            return "MISSING_SEED_SET";
        case DreamRunAuditReasonClass::generic_failure:
            return "GENERIC_FAILURE";
        case DreamRunAuditReasonClass::abort:
            return "ABORT";
    }
    return "UNKNOWN";
}

std::optional<DreamRunDescriptor> DreamOrchestrator::StartDreamRun(
    const std::string& mode,
    const std::string& policy_version,
    const std::string& dataset_ref,
    const std::vector<std::int64_t>& seed_set,
    std::string* error,
    DreamRunAuditReasonClass* reason_class) {
    if (error) {
        *error = "";
    }
    if (reason_class) {
        *reason_class = DreamRunAuditReasonClass::none;
    }

    if (mode != "dream_research") {
        const std::string detail = "Dream run rejected: mode must be dream_research";
        if (error) {
            *error = detail;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::invalid_mode;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        recordAuditLocked("<rejected>", DreamRunState::R0, "start", detail,
                          DreamRunAuditReasonClass::invalid_mode);
        return std::nullopt;
    }
    if (policy_version.empty()) {
        const std::string detail = "Dream run rejected: missing policy_version";
        if (error) {
            *error = detail;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::missing_policy_version;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        recordAuditLocked("<rejected>", DreamRunState::R0, "start", detail,
                          DreamRunAuditReasonClass::missing_policy_version);
        return std::nullopt;
    }
    if (dataset_ref.empty()) {
        const std::string detail = "Dream run rejected: missing dataset_ref";
        if (error) {
            *error = detail;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::missing_dataset_ref;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        recordAuditLocked("<rejected>", DreamRunState::R0, "start", detail,
                          DreamRunAuditReasonClass::missing_dataset_ref);
        return std::nullopt;
    }
    if (!hasMeaningfulSeedSet(seed_set)) {
        const std::string detail = "Dream run rejected: missing seed_set";
        if (error) {
            *error = detail;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::missing_seed_set;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        recordAuditLocked("<rejected>", DreamRunState::R0, "start", detail,
                          DreamRunAuditReasonClass::missing_seed_set);
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const std::string run_id = "dream-run-" + std::to_string(++next_run_number_);
    DreamRunDescriptor descriptor;
    descriptor.run_id = run_id;
    descriptor.mode = mode;
    descriptor.state = DreamRunState::R0;
    descriptor.policy_version = policy_version;
    descriptor.dataset_ref = dataset_ref;
    descriptor.seed_set = seed_set;
    descriptor.synthetic = true;
    descriptor.dream_mode = true;
    const std::string now = formatUtcTimestampNow();
    descriptor.created_at_utc = now;
    descriptor.updated_at_utc = now;

    runs_[run_id] = descriptor;
    recordAuditLocked(run_id, DreamRunState::R0, "start", "Dream run started",
                      DreamRunAuditReasonClass::none);
    return descriptor;
}

bool DreamOrchestrator::transitionRun(const std::string& run_id,
                                     DreamRunState next_state,
                                     std::string* detail,
                                     DreamRunAuditReasonClass* reason_class) {
    if (detail) {
        *detail = "";
    }
    if (reason_class) {
        *reason_class = DreamRunAuditReasonClass::none;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = runs_.find(run_id);
    if (it == runs_.end()) {
        const std::string error = "Dream run not found: " + run_id;
        if (detail) {
            *detail = error;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::generic_failure;
        }
        recordAuditLocked(run_id, DreamRunState::R0, "transition", error,
                          DreamRunAuditReasonClass::generic_failure);
        return false;
    }

    DreamRunState current_state = it->second.state;
    if (!isAllowedTransition(current_state, next_state)) {
        const std::string error = "Invalid transition from " + toString(current_state) +
                                  " to " + toString(next_state);
        if (detail) {
            *detail = error;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::invalid_transition;
        }
        recordAuditLocked(run_id, current_state, "transition", error,
                          DreamRunAuditReasonClass::invalid_transition);
        return false;
    }

    it->second.state = next_state;
    it->second.updated_at_utc = formatUtcTimestampNow();
    recordAuditLocked(run_id, next_state, "transition", "State transition accepted",
                      DreamRunAuditReasonClass::none);
    return true;
}

bool DreamOrchestrator::abortRun(const std::string& run_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = runs_.find(run_id);
    if (it == runs_.end()) {
        return false;
    }
    it->second.state = DreamRunState::R0;
    it->second.updated_at_utc = formatUtcTimestampNow();
    recordAuditLocked(run_id, DreamRunState::R0, "abort", reason,
                      DreamRunAuditReasonClass::abort);
    return true;
}

bool DreamOrchestrator::failRun(const std::string& run_id,
                               const std::string& reason,
                               DreamRunAuditReasonClass reason_class) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = runs_.find(run_id);
    if (it == runs_.end()) {
        return false;
    }
    it->second.state = DreamRunState::R0;
    it->second.updated_at_utc = formatUtcTimestampNow();
    recordAuditLocked(run_id, DreamRunState::R0, "failure", reason, reason_class);
    return true;
}

std::optional<DreamRunDescriptor> DreamOrchestrator::getRunById(
    const std::string& run_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = runs_.find(run_id);
    if (it == runs_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<DreamRunAuditEvent> DreamOrchestrator::auditLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return audit_events_;
}

std::string DreamOrchestrator::serializeRunDescriptor(
    const DreamRunDescriptor& descriptor) const {
    return descriptor.toJson().dump();
}

void DreamOrchestrator::recordAuditLocked(const std::string& run_id,
                                         DreamRunState state,
                                         const std::string& phase,
                                         const std::string& reason,
                                         DreamRunAuditReasonClass reason_class) {
    DreamRunAuditEvent event;
    event.run_id = run_id;
    event.timestamp_utc = formatUtcTimestampNow();
    event.state = state;
    event.reason_class = reason_class;
    event.reason = reason;
    event.phase = phase;
    audit_events_.push_back(event);
}

} // namespace tensor
} // namespace themis
