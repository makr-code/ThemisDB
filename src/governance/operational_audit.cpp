/**
 * @file operational_audit.cpp
 * @brief In-memory operational event logging, correlation, and evidence collection.
 */
#include "governance/operational_audit.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis::governance {
namespace {

int64_t currentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string randomUuid() {
    unsigned char bytes[16];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        throw std::runtime_error("Unable to generate an operational audit identifier");
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) result << '-';
        const auto byte = bytes[i];
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

std::string randomId(const char* prefix) {
    return std::string(prefix) + randomUuid();
}

std::string sha256Hex(const std::string& value) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (EVP_Digest(value.data(), value.size(), digest, &length, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("Unable to fingerprint compliance evidence");
    }
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < length; ++i) {
        result << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return result.str();
}

int64_t percentile(std::vector<int64_t> values, double fraction) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const auto index = static_cast<size_t>(
        fraction * static_cast<double>(values.size() - 1));
    return values[index];
}

std::shared_ptr<OperationalAuditLogger> globalLoggerPtr() {
    static auto logger = std::make_shared<OperationalAuditLogger>();
    return logger;
}

} // namespace

nlohmann::json OperationalEvent::toJson() const {
    return nlohmann::json{
        {"event_id", event_id},
        {"event_code", static_cast<int32_t>(event_type)},
        {"correlation_id", correlation_id},
        {"causality_parent_id", causality_parent_id},
        {"timestamp_ms", timestamp_ms},
        {"sequence_number", sequence_number},
        {"actor_id", actor_id},
        {"actor_type", actor_type},
        {"module_name", module_name},
        {"operation_name", operation_name},
        {"resource_id", resource_id},
        {"resource_type", resource_type},
        {"action", action},
        {"result", result},
        {"classification", classification},
        {"compliance_tags", compliance_tags},
        {"operation_duration_us", operation_duration_us},
        {"logging_duration_us", logging_duration_us},
        {"context", context},
        {"error_message", error_message},
        {"event_payload", event_payload},
        {"evidence_ids", evidence_ids}
    };
}

OperationalEvent OperationalEvent::fromJson(const nlohmann::json& j) {
    OperationalEvent event;
    if (j.contains("event_id")) event.event_id = j.at("event_id").get<std::string>();
    if (j.contains("event_code")) event.event_type = static_cast<OperationalEventType>(j.at("event_code").get<int32_t>());
    if (j.contains("correlation_id")) event.correlation_id = j.at("correlation_id").get<std::string>();
    if (j.contains("causality_parent_id")) event.causality_parent_id = j.at("causality_parent_id").get<std::string>();
    if (j.contains("timestamp_ms")) event.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
    if (j.contains("sequence_number")) event.sequence_number = j.at("sequence_number").get<int64_t>();
    if (j.contains("actor_id")) event.actor_id = j.at("actor_id").get<std::string>();
    if (j.contains("actor_type")) event.actor_type = j.at("actor_type").get<std::string>();
    if (j.contains("module_name")) event.module_name = j.at("module_name").get<std::string>();
    if (j.contains("operation_name")) event.operation_name = j.at("operation_name").get<std::string>();
    if (j.contains("resource_id")) event.resource_id = j.at("resource_id").get<std::string>();
    if (j.contains("resource_type")) event.resource_type = j.at("resource_type").get<std::string>();
    if (j.contains("action")) event.action = j.at("action").get<std::string>();
    if (j.contains("result")) event.result = j.at("result").get<std::string>();
    if (j.contains("classification")) event.classification = j.at("classification").get<std::string>();
    if (j.contains("compliance_tags")) event.compliance_tags = j.at("compliance_tags").get<std::vector<std::string>>();
    if (j.contains("operation_duration_us")) event.operation_duration_us = j.at("operation_duration_us").get<int64_t>();
    if (j.contains("logging_duration_us")) event.logging_duration_us = j.at("logging_duration_us").get<int64_t>();
    if (j.contains("context")) event.context = j.at("context").get<std::unordered_map<std::string, std::string>>();
    if (j.contains("error_message")) event.error_message = j.at("error_message").get<std::string>();
    if (j.contains("event_payload")) event.event_payload = j.at("event_payload").get<std::string>();
    if (j.contains("evidence_ids")) event.evidence_ids = j.at("evidence_ids").get<std::vector<std::string>>();
    return event;
}

nlohmann::json ComplianceEvidence::toJson() const {
    return nlohmann::json{
        {"evidence_id", evidence_id},
        {"requirement_id", requirement_id},
        {"requirement_type", requirement_type},
        {"collected_at_ms", collected_at_ms},
        {"evidence_type", evidence_type},
        {"description", description},
        {"source_event_id", source_event_id},
        {"fingerprint", fingerprint},
        {"data_summary", data_summary},
        {"retention_until_ms", retention_until_ms},
        {"audit_classification", audit_classification},
        {"metadata", metadata}
    };
}

ComplianceEvidence ComplianceEvidence::fromJson(const nlohmann::json& j) {
    ComplianceEvidence evidence;
    if (j.contains("evidence_id")) evidence.evidence_id = j.at("evidence_id").get<std::string>();
    if (j.contains("requirement_id")) evidence.requirement_id = j.at("requirement_id").get<std::string>();
    if (j.contains("requirement_type")) evidence.requirement_type = j.at("requirement_type").get<std::string>();
    if (j.contains("collected_at_ms")) evidence.collected_at_ms = j.at("collected_at_ms").get<int64_t>();
    if (j.contains("evidence_type")) evidence.evidence_type = j.at("evidence_type").get<std::string>();
    if (j.contains("description")) evidence.description = j.at("description").get<std::string>();
    if (j.contains("source_event_id")) evidence.source_event_id = j.at("source_event_id").get<std::string>();
    if (j.contains("fingerprint")) evidence.fingerprint = j.at("fingerprint").get<std::string>();
    if (j.contains("data_summary")) evidence.data_summary = j.at("data_summary").get<std::string>();
    if (j.contains("retention_until_ms")) evidence.retention_until_ms = j.at("retention_until_ms").get<int64_t>();
    if (j.contains("audit_classification")) evidence.audit_classification = j.at("audit_classification").get<std::string>();
    if (j.contains("metadata")) evidence.metadata = j.at("metadata");
    evidence.event_id = evidence.source_event_id;
    evidence.compliance_requirement = evidence.requirement_id;
    evidence.evidence_timestamp_ms = evidence.collected_at_ms;
    evidence.evidence_description = evidence.description;
    evidence.evidence_content = evidence.data_summary;
    return evidence;
}

nlohmann::json CorrelationGroup::toJson() const {
    return nlohmann::json{
        {"correlation_id", correlation_id},
        {"created_at_ms", created_at_ms},
        {"last_updated_ms", last_updated_ms},
        {"event_ids", event_ids},
        {"causality_chain", causality_chain}
    };
}

OperationalAuditLogger::OperationalAuditLogger(size_t max_events)
    : max_events_(max_events) {}

OperationalAuditLogger::~OperationalAuditLogger() = default;

std::string OperationalAuditLogger::generateEventId() const {
    return randomUuid();
}

int64_t OperationalAuditLogger::getNextSequenceNumber() const {
    return metrics_.total_operations + 1;
}

OperationalEvent OperationalAuditLogger::logEvent(const OperationalEvent& input) {
    const auto started = std::chrono::steady_clock::now();
    OperationalEvent event = input;
    if (event.event_id.empty()) event.event_id = generateEventId();
    if (event.timestamp_ms == 0) event.timestamp_ms = currentTimeMs();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event.sequence_number == 0) event.sequence_number = getNextSequenceNumber();
        if (event.correlation_id.empty()) event.correlation_id = event.event_id;
        if (!event.causality_parent_id.empty()) {
            causality_map_[event.causality_parent_id].push_back(event.event_id);
        }
        events_.push_back(event);
        if (max_events_ > 0 && events_.size() > max_events_) {
            events_.erase(events_.begin(), events_.begin() + (events_.size() - max_events_));
        }
        ++metrics_.total_operations;
    }

    event.logging_duration_us = std::max<int64_t>(1, std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_.logging_times_us.push_back(event.logging_duration_us);
        if (metrics_.logging_times_us.size() > 10000) {
            metrics_.logging_times_us.erase(metrics_.logging_times_us.begin());
        }
        auto found = std::find_if(events_.rbegin(), events_.rend(), [&](const auto& saved) {
            return saved.event_id == event.event_id;
        });
        if (found != events_.rend()) found->logging_duration_us = event.logging_duration_us;
    }
    return event;
}

OperationalEvent OperationalAuditLogger::logPolicyEvaluation(
    const std::string& correlation_id, const std::string& policy_id,
    const std::string& decision, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& context) {
    OperationalEvent event;
    event.event_type = decision == "permit" || decision == "allow"
        ? OperationalEventType::POLICY_EVALUATION_PERMIT
        : decision == "deny" ? OperationalEventType::POLICY_EVALUATION_DENY
                              : OperationalEventType::POLICY_EVALUATION_ERROR;
    event.correlation_id = correlation_id;
    event.actor_id = actor_id;
    event.actor_type = "service";
    event.module_name = "policy_engine";
    event.operation_name = "evaluate";
    event.resource_id = policy_id;
    event.resource_type = "policy";
    event.action = "evaluate";
    event.result = decision;
    event.classification = "POLICY_DECISION";
    event.context = context;
    return logEvent(event);
}

OperationalEvent OperationalAuditLogger::logComplianceCheck(
    const std::string& correlation_id, const std::string& check_name,
    const std::string& result, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& context) {
    OperationalEvent event;
    event.event_type = result == "pass" ? OperationalEventType::COMPLIANCE_CHECK_PASS
        : result == "fail" ? OperationalEventType::COMPLIANCE_CHECK_FAIL
                           : OperationalEventType::COMPLIANCE_CHECK_ERROR;
    event.correlation_id = correlation_id;
    event.actor_id = actor_id;
    event.actor_type = "service";
    event.module_name = "compliance_engine";
    event.operation_name = "check";
    event.resource_id = check_name;
    event.resource_type = "compliance_check";
    event.action = "check";
    event.result = result;
    event.classification = "COMPLIANCE";
    event.context = context;
    for (const auto& item : context) event.compliance_tags.push_back(item.second);
    return logEvent(event);
}

OperationalEvent OperationalAuditLogger::logDataGovernanceOp(
    const std::string& correlation_id, const std::string& operation_name,
    const std::string& resource_id, bool success, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& context) {
    OperationalEvent event;
    event.event_type = success ? OperationalEventType::DATA_LINEAGE_RECORDED
                               : OperationalEventType::DATA_LINEAGE_ERROR;
    event.correlation_id = correlation_id;
    event.actor_id = actor_id;
    event.actor_type = "service";
    event.module_name = "data_governance";
    event.operation_name = operation_name;
    event.resource_id = resource_id;
    event.resource_type = "data";
    event.action = operation_name;
    event.result = success ? "success" : "failure";
    event.classification = "DATA_GOVERNANCE";
    event.context = context;
    return logEvent(event);
}

OperationalEvent OperationalAuditLogger::logPolicyLifecycle(
    const std::string& correlation_id, const std::string& policy_id,
    const std::string& lifecycle_event, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& context) {
    OperationalEvent event;
    if (lifecycle_event == "create") event.event_type = OperationalEventType::POLICY_CREATED;
    else if (lifecycle_event == "delete") event.event_type = OperationalEventType::POLICY_DELETED;
    else if (lifecycle_event == "activate") event.event_type = OperationalEventType::POLICY_ACTIVATED;
    else if (lifecycle_event == "deactivate" || lifecycle_event == "deprecate") event.event_type = OperationalEventType::POLICY_DEPRECATED;
    else event.event_type = OperationalEventType::POLICY_UPDATED;
    event.correlation_id = correlation_id;
    event.actor_id = actor_id;
    event.actor_type = "user";
    event.module_name = "policy_management";
    event.operation_name = lifecycle_event;
    event.resource_id = policy_id;
    event.resource_type = "policy";
    event.action = lifecycle_event;
    event.result = "success";
    event.classification = "POLICY_LIFECYCLE";
    event.context = context;
    return logEvent(event);
}

void OperationalAuditLogger::linkCausalityRelationship(
    const std::string& parent_event_id, const std::string& child_event_id) {
    if (parent_event_id.empty() || child_event_id.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    causality_map_[parent_event_id].push_back(child_event_id);
}

std::optional<OperationalEvent> OperationalAuditLogger::getEventById(const std::string& event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = std::find_if(events_.begin(), events_.end(), [&](const auto& event) {
        return event.event_id == event_id;
    });
    if (found == events_.end()) return std::nullopt;
    return *found;
}

std::vector<OperationalEvent> OperationalAuditLogger::queryEventsByCorrelationId(const std::string& id) const {
    std::vector<OperationalEvent> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& event : events_) if (event.correlation_id == id) result.push_back(event);
    return result;
}

std::vector<OperationalEvent> OperationalAuditLogger::queryEventsByTimeRange(
    int64_t start_ms, int64_t end_ms, const std::optional<OperationalEventType>& event_type) const {
    const auto started = std::chrono::steady_clock::now();
    std::vector<OperationalEvent> result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& event : events_) {
            if (start_ms != 0 && event.timestamp_ms < start_ms) continue;
            if (end_ms != 0 && event.timestamp_ms > end_ms) continue;
            if (event_type && event.event_type != *event_type) continue;
            result.push_back(event);
        }
        ++metrics_.total_query_operations;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_.query_times_us.push_back(elapsed);
        if (metrics_.query_times_us.size() > 10000) metrics_.query_times_us.erase(metrics_.query_times_us.begin());
    }
    return result;
}

std::vector<OperationalEvent> OperationalAuditLogger::queryEventsByActor(const std::string& id) const {
    std::vector<OperationalEvent> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& event : events_) if (event.actor_id == id) result.push_back(event);
    return result;
}

std::vector<OperationalEvent> OperationalAuditLogger::queryEventsByModule(const std::string& name) const {
    std::vector<OperationalEvent> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& event : events_) if (event.module_name == name) result.push_back(event);
    return result;
}

std::vector<OperationalEvent> OperationalAuditLogger::queryEventsByResource(const std::string& id) const {
    std::vector<OperationalEvent> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& event : events_) if (event.resource_id == id) result.push_back(event);
    return result;
}

std::vector<OperationalEvent> OperationalAuditLogger::getCausalityChain(const std::string& id) const {
    auto starting = getEventById(id);
    if (!starting) {
        auto correlated = queryEventsByCorrelationId(id);
        if (!correlated.empty()) {
            std::sort(correlated.begin(), correlated.end(), [](const auto& left, const auto& right) {
                return left.sequence_number < right.sequence_number;
            });
            return correlated;
        }
    }
    std::vector<OperationalEvent> chain;
    std::unordered_set<std::string> seen;
    while (starting && seen.insert(starting->event_id).second) {
        chain.push_back(*starting);
        if (starting->causality_parent_id.empty()) break;
        starting = getEventById(starting->causality_parent_id);
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

std::vector<OperationalEvent> OperationalAuditLogger::getTriggeredEvents(const std::string& parent_id) const {
    std::vector<std::string> child_ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = causality_map_.find(parent_id);
        if (found != causality_map_.end()) child_ids = found->second;
    }
    std::vector<OperationalEvent> result;
    for (const auto& id : child_ids) {
        auto event = getEventById(id);
        if (event) result.push_back(*event);
    }
    return result;
}

nlohmann::json OperationalAuditLogger::exportEvents(
    int64_t start_ms, int64_t end_ms,
    const std::optional<OperationalEventType>& event_type, bool compress) const {
    (void)compress;
    const auto events = queryEventsByTimeRange(start_ms, end_ms, event_type);
    nlohmann::json output;
    output["events"] = nlohmann::json::array();
    for (const auto& event : events) output["events"].push_back(event.toJson());
    output["event_count"] = events.size();
    return output;
}

nlohmann::json OperationalAuditLogger::getEventStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<int32_t, size_t> by_type;
    for (const auto& event : events_) ++by_type[static_cast<int32_t>(event.event_type)];
    nlohmann::json types = nlohmann::json::object();
    for (const auto& item : by_type) types[std::to_string(item.first)] = item.second;
    return {
        {"total_events", events_.size()},
        {"event_types", types},
        {"logging_latency_p50_us", percentile(metrics_.logging_times_us, 0.50)},
        {"logging_latency_p95_us", percentile(metrics_.logging_times_us, 0.95)},
        {"logging_latency_p99_us", percentile(metrics_.logging_times_us, 0.99)}
    };
}

nlohmann::json OperationalAuditLogger::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {
        {"total_operations", metrics_.total_operations},
        {"total_query_operations", metrics_.total_query_operations},
        {"logging_latency_p50_us", percentile(metrics_.logging_times_us, 0.50)},
        {"logging_latency_p95_us", percentile(metrics_.logging_times_us, 0.95)},
        {"logging_latency_p99_us", percentile(metrics_.logging_times_us, 0.99)},
        {"query_latency_p50_us", percentile(metrics_.query_times_us, 0.50)}
    };
}

size_t OperationalAuditLogger::getTotalEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}

void OperationalAuditLogger::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
    causality_map_.clear();
    metrics_ = {};
}

EventCorrelationEngine::EventCorrelationEngine(std::shared_ptr<OperationalAuditLogger> logger)
    : audit_logger_(std::move(logger)) {}

EventCorrelationEngine::~EventCorrelationEngine() = default;

std::string EventCorrelationEngine::generateCorrelationId() const {
    return randomId("corr_");
}

std::string EventCorrelationEngine::createCorrelation(
    const std::string& operation_name, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& initial_context) {
    const auto id = generateCorrelationId();
    const auto now = currentTimeMs();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        correlations_[id] = CorrelationGroup{id, now, now, {}, {}};
        if (!actor_id.empty()) actor_correlations_[actor_id].push_back(id);
        timeline_.emplace_back(now, id);
    }
    if (audit_logger_) {
        OperationalEvent event;
        event.event_type = OperationalEventType::CORRELATION_CREATED;
        event.correlation_id = id;
        event.actor_id = actor_id;
        event.actor_type = "service";
        event.module_name = "governance";
        event.operation_name = operation_name;
        event.action = "create_correlation";
        event.result = "success";
        event.context = initial_context;
        audit_logger_->logEvent(event);
    }
    return id;
}

std::optional<CorrelationGroup> EventCorrelationEngine::getCorrelationGroup(const std::string& id) const {
    CorrelationGroup group;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = correlations_.find(id);
        if (found == correlations_.end()) return std::nullopt;
        group = found->second;
    }
    if (audit_logger_) {
        const auto events = audit_logger_->queryEventsByCorrelationId(id);
        group.event_ids.clear();
        group.causality_chain.clear();
        for (const auto& event : events) {
            group.event_ids.push_back(event.event_id);
            group.causality_chain.push_back(event.event_id);
            group.last_updated_ms = std::max(group.last_updated_ms, event.timestamp_ms);
        }
    }
    return group;
}

std::vector<CorrelationGroup> EventCorrelationEngine::queryCorrelationsByTimeRange(int64_t start, int64_t end) const {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : timeline_) if (item.first >= start && item.first <= end) ids.push_back(item.second);
    }
    std::vector<CorrelationGroup> result;
    for (const auto& id : ids) {
        auto group = getCorrelationGroup(id);
        if (group) result.push_back(*group);
    }
    return result;
}

std::vector<CorrelationGroup> EventCorrelationEngine::queryCorrelationsByActor(const std::string& actor_id) const {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ids.reserve(correlations_.size());
        for (const auto& item : correlations_) ids.push_back(item.first);
    }
    std::vector<CorrelationGroup> result;
    for (const auto& id : ids) {
        auto group = getCorrelationGroup(id);
        if (!group) continue;
        if (!audit_logger_) {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found = actor_correlations_.find(actor_id);
            if (found != actor_correlations_.end() &&
                std::find(found->second.begin(), found->second.end(), id) != found->second.end()) {
                result.push_back(*group);
            }
            continue;
        }
        const auto events = audit_logger_->queryEventsByCorrelationId(id);
        if (std::any_of(events.begin(), events.end(), [&](const auto& event) {
                return event.actor_id == actor_id;
            })) {
            result.push_back(*group);
        }
    }
    return result;
}

int64_t EventCorrelationEngine::getCorrelationLatency(const std::string& id) const {
    const auto group = getCorrelationGroup(id);
    return group ? std::max<int64_t>(0, group->last_updated_ms - group->created_at_ms) : 0;
}

nlohmann::json EventCorrelationEngine::getCorrelationLatencyStats() const {
    std::vector<int64_t> latencies;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        latencies.reserve(correlations_.size());
        for (const auto& item : correlations_) {
            latencies.push_back(std::max<int64_t>(0, item.second.last_updated_ms - item.second.created_at_ms));
        }
    }
    return {{"correlation_count", latencies.size()}, {"latency_p50_ms", percentile(latencies, 0.50)},
            {"latency_p95_ms", percentile(latencies, 0.95)}, {"latency_p99_ms", percentile(latencies, 0.99)}};
}

nlohmann::json EventCorrelationEngine::exportCorrelations(const std::optional<std::string>& id) const {
    nlohmann::json output;
    output["correlations"] = nlohmann::json::array();
    if (id) {
        auto group = getCorrelationGroup(*id);
        if (group) output["correlations"].push_back(group->toJson());
    } else {
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& item : correlations_) ids.push_back(item.first);
        }
        for (const auto& key : ids) {
            auto group = getCorrelationGroup(key);
            if (group) output["correlations"].push_back(group->toJson());
        }
    }
    output["correlation_count"] = output["correlations"].size();
    return output;
}

ComplianceEvidenceCollector::ComplianceEvidenceCollector(std::shared_ptr<OperationalAuditLogger> logger)
    : audit_logger_(std::move(logger)) {}

ComplianceEvidenceCollector::~ComplianceEvidenceCollector() = default;

std::string ComplianceEvidenceCollector::generateEvidenceId() const {
    return randomId("evi_");
}

std::string ComplianceEvidenceCollector::computeEvidenceFingerprint(const std::string& content) const {
    return sha256Hex(content);
}

std::vector<ComplianceEvidence> ComplianceEvidenceCollector::collectEvidence(
    const std::string& requirement, const std::string& actor_id,
    const std::unordered_map<std::string, std::string>& context) {
    ComplianceEvidence evidence;
    evidence.requirement_id = requirement;
    evidence.requirement_type = requirement;
    evidence.evidence_type = "operational_audit";
    evidence.description = "Operational audit events collected for compliance requirement";
    evidence.audit_classification = "REGULATORY";
    evidence.collected_at_ms = currentTimeMs();
    evidence.metadata = context;
    if (audit_logger_) {
        const auto events = audit_logger_->queryEventsByTimeRange(0, 0);
        nlohmann::json event_data = nlohmann::json::array();
        for (const auto& event : events) {
            if (std::find(event.compliance_tags.begin(), event.compliance_tags.end(), requirement) != event.compliance_tags.end()) {
                event_data.push_back(event.toJson());
            }
        }
        evidence.data_summary = event_data.dump();
    } else {
        evidence.data_summary = "[]";
    }
    evidence.metadata["collected_by"] = actor_id;
    auto recorded = recordEvidence(evidence);
    return {recorded};
}

ComplianceEvidence ComplianceEvidenceCollector::recordEvidence(const ComplianceEvidence& input) {
    ComplianceEvidence evidence = input;
    if (evidence.evidence_id.empty()) evidence.evidence_id = generateEvidenceId();
    if (evidence.collected_at_ms == 0) evidence.collected_at_ms = currentTimeMs();
    if (evidence.requirement_id.empty()) evidence.requirement_id = evidence.compliance_requirement;
    if (evidence.requirement_type.empty()) evidence.requirement_type = evidence.requirement_id;
    if (evidence.description.empty()) evidence.description = evidence.evidence_description;
    if (evidence.data_summary.empty()) evidence.data_summary = evidence.evidence_content;
    if (evidence.source_event_id.empty()) evidence.source_event_id = evidence.event_id;
    if (evidence.fingerprint.empty()) evidence.fingerprint = computeEvidenceFingerprint(evidence.data_summary);
    evidence.event_id = evidence.source_event_id;
    evidence.compliance_requirement = evidence.requirement_id;
    evidence.evidence_timestamp_ms = evidence.collected_at_ms;
    evidence.evidence_description = evidence.description;
    evidence.evidence_content = evidence.data_summary;
    std::lock_guard<std::mutex> lock(mutex_);
    evidence_.push_back(evidence);
    requirement_evidence_map_[evidence.requirement_id].push_back(evidence.evidence_id);
    if (!evidence.source_event_id.empty()) {
        evidence_event_links_[evidence.evidence_id].push_back(evidence.source_event_id);
    }
    return evidence;
}

void ComplianceEvidenceCollector::linkEvidenceToEvent(const std::string& evidence_id, const std::string& event_id) {
    linkEvidenceToEvents(evidence_id, {event_id});
}

void ComplianceEvidenceCollector::linkEvidenceToEvents(
    const std::string& evidence_id, const std::vector<std::string>& event_ids) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = std::find_if(evidence_.begin(), evidence_.end(), [&](const auto& item) {
        return item.evidence_id == evidence_id;
    });
    if (found == evidence_.end()) return;
    for (const auto& event_id : event_ids) {
        if (event_id.empty()) continue;
        evidence_event_links_[evidence_id].push_back(event_id);
        if (found->source_event_id.empty()) found->source_event_id = event_id;
        found->related_events.push_back(event_id);
    }
    found->event_id = found->source_event_id;
}

std::vector<ComplianceEvidence> ComplianceEvidenceCollector::getEvidenceByRequirement(const std::string& requirement) const {
    std::vector<ComplianceEvidence> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& evidence : evidence_) {
        if (evidence.requirement_id == requirement || evidence.requirement_type == requirement) result.push_back(evidence);
    }
    return result;
}

std::vector<ComplianceEvidence> ComplianceEvidenceCollector::getEvidenceByEvent(const std::string& event_id) const {
    std::vector<ComplianceEvidence> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& evidence : evidence_) {
        const auto links = evidence_event_links_.find(evidence.evidence_id);
        if (evidence.source_event_id == event_id ||
            (links != evidence_event_links_.end() &&
             std::find(links->second.begin(), links->second.end(), event_id) != links->second.end())) {
            result.push_back(evidence);
        }
    }
    return result;
}

std::vector<ComplianceEvidence> ComplianceEvidenceCollector::getEvidenceByTimeRange(int64_t start, int64_t end) const {
    std::vector<ComplianceEvidence> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& evidence : evidence_) {
        if (start != 0 && evidence.collected_at_ms < start) continue;
        if (end != 0 && evidence.collected_at_ms > end) continue;
        result.push_back(evidence);
    }
    return result;
}

nlohmann::json ComplianceEvidenceCollector::exportEvidenceForAudit(
    const std::optional<std::string>& requirement, int64_t start, int64_t end) const {
    auto evidence = getEvidenceByTimeRange(start, end);
    if (requirement) {
        evidence.erase(std::remove_if(evidence.begin(), evidence.end(), [&](const auto& item) {
            return item.requirement_type != *requirement && item.requirement_id != *requirement;
        }), evidence.end());
    }
    nlohmann::json output;
    output["evidence"] = nlohmann::json::array();
    for (const auto& item : evidence) output["evidence"].push_back(item.toJson());
    output["evidence_count"] = evidence.size();
    output["requirement_type"] = requirement.value_or("");
    return output;
}

nlohmann::json ComplianceEvidenceCollector::generateEvidenceReport(const std::vector<std::string>& requirements) const {
    nlohmann::json output;
    output["requirements"] = nlohmann::json::array();
    for (const auto& requirement : requirements) {
        auto items = getEvidenceByRequirement(requirement);
        output["requirements"].push_back({
            {"requirement", requirement},
            {"evidence_count", items.size()},
            {"evidence", exportEvidenceForAudit(requirement)["evidence"]}
        });
    }
    return output;
}

nlohmann::json ComplianceEvidenceCollector::getEvidenceStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, size_t> by_requirement;
    for (const auto& evidence : evidence_) ++by_requirement[evidence.requirement_id];
    nlohmann::json output;
    output["total_evidence"] = evidence_.size();
    output["by_requirement"] = nlohmann::json::object();
    for (const auto& item : by_requirement) output["by_requirement"][item.first] = item.second;
    return output;
}

size_t ComplianceEvidenceCollector::getTotalEvidenceCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return evidence_.size();
}

void ComplianceEvidenceCollector::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    evidence_.clear();
    requirement_evidence_map_.clear();
    evidence_event_links_.clear();
}

OperationalAuditLogger& getGlobalAuditLogger() {
    return *globalLoggerPtr();
}

EventCorrelationEngine& getGlobalCorrelationEngine() {
    static EventCorrelationEngine engine(globalLoggerPtr());
    return engine;
}

ComplianceEvidenceCollector& getGlobalEvidenceCollector() {
    static ComplianceEvidenceCollector collector(globalLoggerPtr());
    return collector;
}

} // namespace themis::governance
