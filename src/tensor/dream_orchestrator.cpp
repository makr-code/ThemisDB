#include "tensor/dream_orchestrator.h"

#include <chrono>
#include <cmath>
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

DreamGateOutcome outcome(DreamGateStatus status, std::string reason) {
    return DreamGateOutcome{status, std::move(reason)};
}

nlohmann::json optionalMetric(const std::optional<double>& value) {
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

bool allRequiredGatesPass(const DreamGateReport& report) {
    return report.isolation.status == DreamGateStatus::pass &&
           report.label_integrity.status == DreamGateStatus::pass &&
           report.utility.status == DreamGateStatus::pass &&
           report.nonregression.status == DreamGateStatus::pass &&
           report.ethics_boundary.status == DreamGateStatus::pass &&
           report.security_leakage.status == DreamGateStatus::pass;
}

DreamGateReport buildGateReport(const std::string& run_id,
                                const DreamValidationEvidence& evidence) {
    DreamGateReport report;
    report.run_id = run_id;
    report.ethics_veto_triggered = evidence.ethics_veto;
    report.security_veto_triggered = evidence.security_veto;
    report.security_leakage_detected = evidence.security_leakage_detected;
    report.utility_delta_recall_at_10 = evidence.utility_delta_recall_at_10;
    report.utility_delta_ndcg_at_10 = evidence.utility_delta_ndcg_at_10;
    report.online_hallucination_delta = evidence.online_hallucination_delta;
    report.online_error_delta = evidence.online_error_delta;

    if (evidence.security_leakage_detected) {
        report.security_leakage = outcome(
            DreamGateStatus::fail,
            evidence.security_reason.empty() ? "security leakage was detected"
                                             : evidence.security_reason);
    } else if (evidence.security_veto) {
        report.security_leakage = outcome(
            DreamGateStatus::hold,
            evidence.security_reason.empty() ? "security veto requires review"
                                             : evidence.security_reason);
    } else if (!evidence.security_check_completed) {
        report.security_leakage = outcome(
            DreamGateStatus::hold, "security leakage check has not completed");
    } else {
        report.security_leakage =
            outcome(DreamGateStatus::pass, "security leakage check passed");
    }

    if (evidence.isolation_check_completed && !evidence.truth_path_firewall_passed) {
        report.isolation = outcome(DreamGateStatus::fail,
                                   "truth-path firewall check failed");
    } else if (report.security_leakage.status == DreamGateStatus::fail) {
        report.isolation = outcome(DreamGateStatus::fail,
                                   report.security_leakage.reason);
    } else if (!evidence.isolation_check_completed) {
        report.isolation = outcome(DreamGateStatus::hold,
                                   "truth-path isolation check has not completed");
    } else if (report.security_leakage.status == DreamGateStatus::hold) {
        report.isolation = outcome(DreamGateStatus::hold,
                                   report.security_leakage.reason);
    } else {
        report.isolation = outcome(DreamGateStatus::pass,
                                   "truth-path firewall check passed");
    }

    if (!evidence.label_integrity_check_completed) {
        report.label_integrity =
            outcome(DreamGateStatus::hold, "provenance label check has not completed");
    } else if (!evidence.provenance_labels_valid) {
        report.label_integrity =
            outcome(DreamGateStatus::fail, "synthetic provenance labels are invalid");
    } else {
        report.label_integrity =
            outcome(DreamGateStatus::pass, "synthetic provenance labels are intact");
    }

    if (!evidence.utility_delta_recall_at_10 ||
        !evidence.utility_delta_ndcg_at_10) {
        report.utility =
            outcome(DreamGateStatus::hold, "utility metrics are incomplete");
    } else if (!std::isfinite(*evidence.utility_delta_recall_at_10) ||
               !std::isfinite(*evidence.utility_delta_ndcg_at_10)) {
        report.utility =
            outcome(DreamGateStatus::fail, "utility metrics must be finite");
    } else if (*evidence.utility_delta_recall_at_10 < 0.0 ||
               *evidence.utility_delta_ndcg_at_10 < 0.0) {
        report.utility =
            outcome(DreamGateStatus::fail, "utility regressed against baseline");
    } else if (*evidence.utility_delta_recall_at_10 > 0.0 ||
               *evidence.utility_delta_ndcg_at_10 > 0.0) {
        report.utility =
            outcome(DreamGateStatus::pass, "measurable utility gain was validated");
    } else {
        report.utility =
            outcome(DreamGateStatus::hold, "utility metrics show no measurable gain");
    }

    if (!evidence.online_hallucination_delta || !evidence.online_error_delta) {
        report.nonregression =
            outcome(DreamGateStatus::hold, "online nonregression metrics are incomplete");
    } else if (!std::isfinite(*evidence.online_hallucination_delta)) {
        report.nonregression =
            outcome(DreamGateStatus::fail, "online nonregression metric must be finite");
    } else if (!std::isfinite(*evidence.online_error_delta)) {
        report.nonregression =
            outcome(DreamGateStatus::fail, "online nonregression metric must be finite");
    } else if (*evidence.online_hallucination_delta > 0.0 ||
               *evidence.online_error_delta > 0.0) {
        report.nonregression = outcome(
            DreamGateStatus::fail, "online hallucination or error rate regressed against baseline");
    } else {
        report.nonregression =
            outcome(DreamGateStatus::pass, "online hallucination rate did not regress");
    }

    if (evidence.ethics_veto) {
        report.ethics_boundary = outcome(
            DreamGateStatus::fail,
            evidence.ethics_reason.empty() ? "ethics veto was triggered"
                                           : evidence.ethics_reason);
    } else if (!evidence.ethics_check_completed) {
        report.ethics_boundary =
            outcome(DreamGateStatus::hold, "ethics boundary check has not completed");
    } else {
        report.ethics_boundary =
            outcome(DreamGateStatus::pass, "ethics boundary check passed");
    }

    if (!allRequiredGatesPass(report)) {
        const auto has_failure =
            report.isolation.status == DreamGateStatus::fail ||
            report.label_integrity.status == DreamGateStatus::fail ||
            report.utility.status == DreamGateStatus::fail ||
            report.nonregression.status == DreamGateStatus::fail ||
            report.ethics_boundary.status == DreamGateStatus::fail ||
            report.security_leakage.status == DreamGateStatus::fail;
        report.status = has_failure ? DreamGateStatus::fail : DreamGateStatus::hold;
        report.recommendation = has_failure ? "no_go" : "research_only";
        const DreamGateOutcome* reason = nullptr;
        for (const auto* gate : {&report.ethics_boundary,
                                 &report.security_leakage,
                                 &report.nonregression,
                                 &report.isolation,
                                 &report.label_integrity,
                                 &report.utility}) {
            if (gate->status == DreamGateStatus::fail ||
                gate->status == DreamGateStatus::hold) {
                reason = gate;
                break;
            }
        }
        report.rationale = reason ? reason->reason : "validation evidence is incomplete";
    } else {
        report.status = DreamGateStatus::pass;
        report.recommendation = "promotion_candidate";
        report.rationale = "all Dream-Mode validation and security gates passed";
    }
    return report;
}

} // namespace

nlohmann::json DreamGateReport::toJson() const {
    nlohmann::json gates = {
        {"BRAIN-DREAM-ISOLATION",
         DreamOrchestrator::gateStatusToString(isolation.status)},
        {"BRAIN-DREAM-LABEL-INTEGRITY",
         DreamOrchestrator::gateStatusToString(label_integrity.status)},
        {"BRAIN-DREAM-UTILITY", DreamOrchestrator::gateStatusToString(utility.status)},
        {"BRAIN-DREAM-NONREGRESSION",
         DreamOrchestrator::gateStatusToString(nonregression.status)},
        {"BRAIN-DREAM-ETHICS-BOUNDARY",
         DreamOrchestrator::gateStatusToString(ethics_boundary.status)},
    };
    nlohmann::json reasons = {
        {"BRAIN-DREAM-ISOLATION", isolation.reason},
        {"BRAIN-DREAM-LABEL-INTEGRITY", label_integrity.reason},
        {"BRAIN-DREAM-UTILITY", utility.reason},
        {"BRAIN-DREAM-NONREGRESSION", nonregression.reason},
        {"BRAIN-DREAM-ETHICS-BOUNDARY", ethics_boundary.reason},
    };
    return nlohmann::json{
        {"report_version", "1.0"},
        {"run_id", run_id},
        {"status", DreamOrchestrator::gateStatusToString(status)},
        {"gates", std::move(gates)},
        {"reasons", std::move(reasons)},
        {"security",
         {{"status", DreamOrchestrator::gateStatusToString(security_leakage.status)},
          {"veto", security_veto_triggered},
          {"leakage_detected", security_leakage_detected},
          {"reason", security_leakage.reason}}},
        {"results",
         {{"utility_delta_recall_at_10",
           optionalMetric(utility_delta_recall_at_10)},
          {"utility_delta_ndcg_at_10", optionalMetric(utility_delta_ndcg_at_10)},
          {"online_hallucination_delta",
           optionalMetric(online_hallucination_delta)},
          {"online_error_delta", optionalMetric(online_error_delta)}}},
        {"vetoes",
         {{"ethics",
           ethics_veto_triggered ? nlohmann::json(ethics_boundary.reason)
                                 : nlohmann::json(nullptr)},
          {"security",
           security_veto_triggered ? nlohmann::json(security_leakage.reason)
                                   : nlohmann::json(nullptr)}}},
        {"decision", {{"recommendation", recommendation}, {"rationale", rationale}}},
    };
}

bool DreamGateReport::permitsPromotionCandidate() const noexcept {
    return allRequiredGatesPass(*this);
}

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
        case DreamRunAuditReasonClass::gate_rejected:
            return "GATE_REJECTED";
        case DreamRunAuditReasonClass::generic_failure:
            return "GENERIC_FAILURE";
        case DreamRunAuditReasonClass::abort:
            return "ABORT";
    }
    return "UNKNOWN";
}

std::string DreamOrchestrator::gateStatusToString(DreamGateStatus status) noexcept {
    switch (status) {
        case DreamGateStatus::pass:
            return "pass";
        case DreamGateStatus::hold:
            return "hold";
        case DreamGateStatus::fail:
            return "fail";
    }
    return "fail";
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

    const auto report_it = gate_reports_.find(run_id);
    const bool validation_passed =
        report_it != gate_reports_.end() && report_it->second.permitsPromotionCandidate();
    const bool requires_passing_gates =
        next_state == DreamRunState::R1 || next_state == DreamRunState::R2 ||
        next_state == DreamRunState::R3;
    if (requires_passing_gates && !validation_passed) {
        const std::string error =
            next_state == DreamRunState::R1
                ? "Transition requires utility, nonregression, and safety gates to pass"
                : "Promotion-candidate transition requires all Dream-Mode gates to pass";
        if (detail) {
            *detail = error;
        }
        if (reason_class) {
            *reason_class = DreamRunAuditReasonClass::gate_rejected;
        }
        recordAuditLocked(run_id, current_state, "transition", error,
                          DreamRunAuditReasonClass::gate_rejected);
        return false;
    }

    it->second.state = next_state;
    it->second.updated_at_utc = formatUtcTimestampNow();
    recordAuditLocked(run_id, next_state, "transition", "State transition accepted",
                      DreamRunAuditReasonClass::none);
    return true;
}

std::optional<DreamGateReport> DreamOrchestrator::validateRun(
    const std::string& run_id,
    const DreamValidationEvidence& evidence) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto run_it = runs_.find(run_id);
    if (run_it == runs_.end()) {
        return std::nullopt;
    }

    auto report = buildGateReport(run_id, evidence);
    gate_reports_[run_id] = report;
    run_it->second.updated_at_utc = formatUtcTimestampNow();
    const auto reason_class =
        report.status == DreamGateStatus::pass ? DreamRunAuditReasonClass::none
                                               : DreamRunAuditReasonClass::gate_rejected;
    recordAuditLocked(run_id, run_it->second.state, "validation", report.rationale,
                      reason_class);
    return report;
}

std::optional<DreamGateReport> DreamOrchestrator::getGateReportByRunId(
    const std::string& run_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = gate_reports_.find(run_id);
    if (it == gate_reports_.end()) {
        return std::nullopt;
    }
    return it->second;
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
