/**
 * @file updates_operator_diagnostics.cpp
 * @brief Implementation of operator-facing diagnostics for update incidents
 * @version 1.0.0
 * @since 2.4.0 (Q4 2026)
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 ThemisDB Contributors
 */

#include "updates/updates_operator_diagnostics.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <mutex>

namespace themis {
namespace updates {

// ============================================================================
// AlertingRule JSON Serialization
// ============================================================================

json AlertingRule::to_json() const {
    json j;
    j["rule_id"] = rule_id;
    j["rule_name"] = rule_name;
    j["severity"] = severity;
    j["condition"] = condition;
    j["message_template"] = message_template;
    j["runbook_url"] = runbook_url;
    j["recommended_action"] = static_cast<int>(recommended_action);
    return j;
}

// ============================================================================
// RecoveryProcedure JSON Serialization
// ============================================================================

json RecoveryProcedure::to_json() const {
    json j;
    j["scenario"] = static_cast<int>(scenario);
    j["symptoms"] = symptoms;
    j["root_cause_analysis"] = root_cause_analysis;
    j["recovery_steps"] = recovery_steps;
    j["prevention_tips"] = prevention_tips;
    j["expected_outcome"] = expected_outcome;
    return j;
}

// ============================================================================
// OperatorDiagnostics Implementation
// ============================================================================

OperatorDiagnostics::OperatorDiagnostics() {
    initializeRecoveryProcedures();
    initializeAlertingRules();
}

// ============================================================================
// Failure Scenario Detection
// ============================================================================

FailureScenario OperatorDiagnostics::detectScenario(const ErrorContext& context) const {
    // Pattern matching based on error codes and operation context
    
    // Check for coordinator-related issues
    if (context.error_code == DiagnosticErrorCode::COORDINATION_TIMEOUT ||
        context.error_code == DiagnosticErrorCode::COORDINATION_PEER_FAILED ||
        context.error_code == DiagnosticErrorCode::COORDINATION_QUORUM_LOST) {
        return detectCoordinatorScenario(context);
    }

    // Check for migration failures
    if (context.operation.find("migrate") != std::string::npos ||
        context.operation.find("migration") != std::string::npos) {
        return detectMigrationScenario(context);
    }

    // Check for canary-related issues
    if (context.operation.find("canary") != std::string::npos) {
        return detectCanaryScenario(context);
    }

    // Check for rollback issues
    if (context.operation.find("rollback") != std::string::npos ||
        context.error_code == DiagnosticErrorCode::ROLLBACK_FAILED) {
        return detectRollbackScenario(context);
    }

    // Check for resource exhaustion
    if (context.error_code == DiagnosticErrorCode::RESOURCE_EXHAUSTED) {
        return detectResourceScenario(context);
    }

    // Check for manifest issues
    if (context.error_code == DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH ||
        context.error_code == DiagnosticErrorCode::STATE_HISTORY_CORRUPT) {
        return detectManifestScenario(context);
    }

    // Check for network partition
    if (context.error_code == DiagnosticErrorCode::NETWORK_PARTITION) {
        return detectPartitionScenario(context);
    }

    // Default: treat as deadlock/race condition
    return FailureScenario::DEADLOCK_RACE_CONDITION;
}

FailureScenario OperatorDiagnostics::detectCoordinatorScenario(const ErrorContext& context) const {
    // Repeated timeouts indicate coordinator unreachability
    if (context.error_code == DiagnosticErrorCode::COORDINATION_TIMEOUT) {
        // Check for canary timeout cycle patterns
        if (context.operation.find("canary") != std::string::npos) {
            return FailureScenario::CANARY_TIMEOUT_CYCLE;
        }
        return FailureScenario::COORDINATOR_UNREACHABLE;
    }
    return FailureScenario::COORDINATOR_UNREACHABLE;
}

FailureScenario OperatorDiagnostics::detectMigrationScenario(const ErrorContext& context) const {
    // Partial failures during migration are indicated by specific operations
    if (context.error_code == DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS ||
        context.phase.find("migrat") != std::string::npos) {
        return FailureScenario::PARTIAL_MIGRATION_FAILURE;
    }
    return FailureScenario::PARTIAL_MIGRATION_FAILURE;
}

FailureScenario OperatorDiagnostics::detectCanaryScenario(const ErrorContext& context) const {
    return FailureScenario::CANARY_TIMEOUT_CYCLE;
}

FailureScenario OperatorDiagnostics::detectRollbackScenario(const ErrorContext& context) const {
    // Blue-green rollback failures during node failures
    if (context.operation.find("blue_green") != std::string::npos ||
        context.operation.find("rollback") != std::string::npos) {
        if (!context.node_id.empty() && context.error_code == DiagnosticErrorCode::ROLLBACK_FAILED) {
            return FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE;
        }
    }
    return FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE;
}

FailureScenario OperatorDiagnostics::detectResourceScenario(const ErrorContext& context) const {
    return FailureScenario::RESOURCE_EXHAUSTION;
}

FailureScenario OperatorDiagnostics::detectManifestScenario(const ErrorContext& context) const {
    return FailureScenario::MANIFEST_CORRUPTION;
}

FailureScenario OperatorDiagnostics::detectPartitionScenario(const ErrorContext& context) const {
    return FailureScenario::CLUSTER_PARTITION;
}

FailureScenario OperatorDiagnostics::detectDeadlockScenario(const ErrorContext& context) const {
    return FailureScenario::DEADLOCK_RACE_CONDITION;
}

std::string OperatorDiagnostics::getScenarioName(FailureScenario scenario) const {
    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE:
            return "Coordinator Unreachable";
        case FailureScenario::PARTIAL_MIGRATION_FAILURE:
            return "Partial Migration Failure";
        case FailureScenario::CANARY_TIMEOUT_CYCLE:
            return "Canary Timeout Cycle";
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE:
            return "Blue-Green Rollback Failure";
        case FailureScenario::RESOURCE_EXHAUSTION:
            return "Resource Exhaustion";
        case FailureScenario::MANIFEST_CORRUPTION:
            return "Manifest Corruption";
        case FailureScenario::CLUSTER_PARTITION:
            return "Cluster Partition";
        case FailureScenario::DEADLOCK_RACE_CONDITION:
            return "Deadlock/Race Condition";
        default:
            return "Unknown";
    }
}

bool OperatorDiagnostics::matchesScenario(FailureScenario scenario, const ErrorContext& context) const {
    return detectScenario(context) == scenario;
}

// ============================================================================
// Recovery Procedures
// ============================================================================

RecoveryProcedure OperatorDiagnostics::getRecoveryProcedure(FailureScenario scenario) const {
    RecoveryProcedure proc;
    proc.scenario = scenario;

    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE: {
            proc.symptoms = "Coordinator timeout, peer unreachable, quorum lost";
            proc.root_cause_analysis = {
                "1. Check coordinator service status: systemctl status themis_coordinator",
                "2. Verify network connectivity: ping coordinator_host",
                "3. Check firewall rules for port 9042 (coordination)",
                "4. Review coordinator logs for startup errors"
            };
            proc.recovery_steps = {
                "1. Verify at least 3/5 nodes in coordination quorum are healthy",
                "2. If coordinator crashed, restart: systemctl restart themis_coordinator",
                "3. Wait 30 seconds for quorum establishment",
                "4. Trigger coordinator health check: themis_cli health check --coordinator",
                "5. Retry the failed operation"
            };
            proc.prevention_tips = {
                "Use heartbeat monitoring (30s interval) to detect coordinator failures early",
                "Implement coordinator redundancy with automatic failover",
                "Set reasonable timeouts (default 5s) for coordination operations",
                "Monitor network latency and packet loss to coordinators"
            };
            proc.expected_outcome = "Coordinator restored to healthy state; operations resume normally";
            break;
        }
        case FailureScenario::PARTIAL_MIGRATION_FAILURE: {
            proc.symptoms = "Migration completes on some nodes but fails on others; inconsistent schema state";
            proc.root_cause_analysis = {
                "1. Identify failed nodes: themis_cli updates status",
                "2. Check node-specific logs for migration errors",
                "3. Verify schema compatibility on each node",
                "4. Check disk space and temporary file availability"
            };
            proc.recovery_steps = {
                "1. Abort migration on all nodes: themis_cli updates abort",
                "2. Rollback to pre-migration state on all nodes",
                "3. Fix root cause (e.g., disk space, schema incompatibility)",
                "4. Retry migration with increased timeout if needed"
            };
            proc.prevention_tips = {
                "Run preflight schema validation before migration",
                "Ensure all nodes have sufficient disk space (min 2x schema size)",
                "Test migrations in staging environment first",
                "Implement per-node progress monitoring during migration"
            };
            proc.expected_outcome = "All nodes rolled back to consistent state; migration can be retried";
            break;
        }
        case FailureScenario::CANARY_TIMEOUT_CYCLE: {
            proc.symptoms = "Canary update repeatedly times out; update progress stalls";
            proc.root_cause_analysis = {
                "1. Check canary node resource usage (CPU, memory, disk I/O)",
                "2. Review network latency to coordinator",
                "3. Analyze patch application time on canary node",
                "4. Check for hung processes: ps aux | grep themis"
            };
            proc.recovery_steps = {
                "1. Increase canary timeout: themis_cli updates config --canary-timeout 30s",
                "2. If timeout persists, abort canary: themis_cli updates abort",
                "3. Restart canary node: systemctl restart themis_server",
                "4. Reduce patch size or pipeline parallelism if disk I/O is bottleneck",
                "5. Retry canary with adjusted timeout"
            };
            proc.prevention_tips = {
                "Monitor canary node resource usage during deployments",
                "Set appropriate timeout based on patch size (1s per 100MB typical)",
                "Implement circuit breaker: abort after 3 timeout cycles",
                "Pre-warm canary node disk cache before applying patches"
            };
            proc.expected_outcome = "Canary timeout resolved; update progression resumes";
            break;
        }
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE: {
            proc.symptoms = "Rollback fails on some nodes during blue-green deployment; systems in inconsistent state";
            proc.root_cause_analysis = {
                "1. Identify which nodes failed rollback: themis_cli updates status",
                "2. Check for node crashes during rollback",
                "3. Verify previous version binaries and state are intact",
                "4. Check isolation mechanisms are active"
            };
            proc.recovery_steps = {
                "1. Verify failed nodes are isolated (no traffic)",
                "2. On isolated nodes, manually rollback: themis_cli updates rollback --force",
                "3. Verify previous version is operational",
                "4. Restart service on recovered nodes",
                "5. Re-add to load balancer pool once verified healthy"
            };
            proc.prevention_tips = {
                "Use node isolation during rollback (fail-closed)",
                "Implement atomic rollback with checkpoints",
                "Test rollback path regularly in staging",
                "Ensure previous version binaries are always available"
            };
            proc.expected_outcome = "All nodes rolled back to previous version; system operational";
            break;
        }
        case FailureScenario::RESOURCE_EXHAUSTION: {
            proc.symptoms = "Operations fail with out-of-memory, disk full, or file descriptor limit errors";
            proc.root_cause_analysis = {
                "1. Check available memory: free -h",
                "2. Check disk space: df -h",
                "3. Check open file descriptors: lsof -p $(pidof themis_server) | wc -l",
                "4. Review memory usage of running updates"
            };
            proc.recovery_steps = {
                "1. Abort current update operation: themis_cli updates abort",
                "2. Free disk space: remove old update artifacts, logs",
                "3. Increase ulimit for file descriptors if needed",
                "4. Reduce update parallelism to lower memory usage",
                "5. Increase system resources if bottleneck persists"
            };
            proc.prevention_tips = {
                "Monitor resource usage (memory, disk, FDs) during deployments",
                "Set aside minimum 20% free disk for temporary update files",
                "Use resource limits (cgroups) to prevent memory exhaustion",
                "Implement progressive rollout to limit concurrent resource usage"
            };
            proc.expected_outcome = "Resources freed; updates resume with reduced parallelism";
            break;
        }
        case FailureScenario::MANIFEST_CORRUPTION: {
            proc.symptoms = "Checksum mismatch on update files, corrupted state history, invalid manifest";
            proc.root_cause_analysis = {
                "1. Verify download integrity: sha256sum manifest.json",
                "2. Check network during download (packet loss, truncation)",
                "3. Review local storage for corruption (bad sector, fs damage)",
                "4. Check state history log for corruption markers"
            };
            proc.recovery_steps = {
                "1. Abort current update: themis_cli updates abort",
                "2. Re-download manifest and artifacts from authoritative source",
                "3. Verify checksums before applying",
                "4. If state history corrupted, rollback to last known good state",
                "5. Retry update with fresh artifacts"
            };
            proc.prevention_tips = {
                "Always verify checksums after download",
                "Use checksums stored in authoritative manifest server",
                "Monitor network health during artifact downloads",
                "Run periodic manifest integrity checks"
            };
            proc.expected_outcome = "Corrupted artifacts replaced; system in consistent state";
            break;
        }
        case FailureScenario::CLUSTER_PARTITION: {
            proc.symptoms = "Cluster split into isolated partitions; quorum or consensus impossible";
            proc.root_cause_analysis = {
                "1. Check network connectivity between all nodes: ping all_nodes",
                "2. Identify isolated partition (minority partition is cut off)",
                "3. Review firewall/network ACL changes",
                "4. Check for DNS resolution failures"
            };
            proc.recovery_steps = {
                "1. Identify majority partition (>50% nodes)",
                "2. Continue operations only on majority partition",
                "3. Abort operations on minority partition",
                "4. Restore network connectivity to isolate partition",
                "5. Rejoin isolated nodes to cluster once healed"
            };
            proc.prevention_tips = {
                "Use split-brain prevention (quorum check before commit)",
                "Monitor inter-node network latency and packet loss",
                "Implement network partition detection (ping heartbeat)",
                "Practice partition recovery procedures regularly"
            };
            proc.expected_outcome = "Cluster partitions healed; nodes rejoin majority partition";
            break;
        }
        case FailureScenario::DEADLOCK_RACE_CONDITION: {
            proc.symptoms = "Operations hang indefinitely, circular dependencies detected, simultaneous modifications";
            proc.root_cause_analysis = {
                "1. Collect stack traces: gdb attach $(pidof themis_server)",
                "2. Check for lock ordering violations in code",
                "3. Review concurrent operation sequences",
                "4. Check for unguarded shared state modifications"
            };
            proc.recovery_steps = {
                "1. Forcefully abort hanging operation: kill -9 pid (last resort)",
                "2. Restart affected service: systemctl restart themis_server",
                "3. Disable conflicting operations if possible",
                "4. Serialize operations to avoid concurrency issues",
                "5. File issue with reproduction steps"
            };
            proc.prevention_tips = {
                "Use lock-free data structures or explicit lock ordering",
                "Run thread sanitizer and deadlock detector in CI",
                "Add timeout on all lock acquisitions",
                "Implement operation serialization where concurrency is problematic"
            };
            proc.expected_outcome = "Service restarted; operation can be retried sequentially";
            break;
        }
    }

    return proc;
}

RecoveryAction OperatorDiagnostics::recommendRecoveryAction(const ErrorContext& context) const {
    auto scenario = detectScenario(context);

    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE:
            return RecoveryAction::RESTART_COORDINATOR;
        case FailureScenario::PARTIAL_MIGRATION_FAILURE:
            return RecoveryAction::FULL_CLUSTER_ROLLBACK;
        case FailureScenario::CANARY_TIMEOUT_CYCLE:
            return RecoveryAction::RESOURCE_CLEANUP;
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE:
            return RecoveryAction::ISOLATED_ROLLBACK;
        case FailureScenario::RESOURCE_EXHAUSTION:
            return RecoveryAction::RESOURCE_CLEANUP;
        case FailureScenario::MANIFEST_CORRUPTION:
            return RecoveryAction::GRACEFUL_DEGRADE;
        case FailureScenario::CLUSTER_PARTITION:
            return RecoveryAction::MANUAL_INTERVENTION;
        case FailureScenario::DEADLOCK_RACE_CONDITION:
            return RecoveryAction::MANUAL_INTERVENTION;
        default:
            return RecoveryAction::MANUAL_INTERVENTION;
    }
}

// ============================================================================
// Alerting Rules
// ============================================================================

AlertingRule OperatorDiagnostics::getAlertingRule(FailureScenario scenario) const {
    AlertingRule rule;
    rule.scenario = scenario;
    rule.runbook_url = "https://themisdb.dev/docs/RUNBOOK_UPDATES_Q4.md";

    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE: {
            rule.rule_id = "UPD_OD_001";
            rule.rule_name = "Updates Coordinator Unreachable";
            rule.severity = "CRITICAL";
            rule.condition = "increase(updates_coordination_timeout_total[5m]) > 3";
            rule.message_template = "Coordinator unreachable for 5+ minutes; {{lost_quorum}} lost from quorum";
            rule.recommended_action = RecoveryAction::RESTART_COORDINATOR;
            break;
        }
        case FailureScenario::PARTIAL_MIGRATION_FAILURE: {
            rule.rule_id = "UPD_OD_002";
            rule.rule_name = "Updates Partial Migration Failure";
            rule.severity = "ERROR";
            rule.condition = "updates_migration_partial_failure_total > 0";
            rule.message_template = "Migration failed on {{failed_nodes}} nodes; inconsistent state";
            rule.recommended_action = RecoveryAction::FULL_CLUSTER_ROLLBACK;
            break;
        }
        case FailureScenario::CANARY_TIMEOUT_CYCLE: {
            rule.rule_id = "UPD_OD_003";
            rule.rule_name = "Updates Canary Timeout Cycle";
            rule.severity = "ERROR";
            rule.condition = "increase(updates_canary_timeout_total[10m]) > 3";
            rule.message_template = "Canary timeout cycles detected; {{timeout_count}} timeouts in 10m";
            rule.recommended_action = RecoveryAction::RESOURCE_CLEANUP;
            break;
        }
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE: {
            rule.rule_id = "UPD_OD_004";
            rule.rule_name = "Updates Blue-Green Rollback Failure";
            rule.severity = "CRITICAL";
            rule.condition = "updates_rollback_failed_total > 0";
            rule.message_template = "Rollback failed on node {{node_id}}; manual intervention needed";
            rule.recommended_action = RecoveryAction::MANUAL_INTERVENTION;
            break;
        }
        case FailureScenario::RESOURCE_EXHAUSTION: {
            rule.rule_id = "UPD_OD_005";
            rule.rule_name = "Updates Resource Exhaustion";
            rule.severity = "CRITICAL";
            rule.condition = "node_memory_MemAvailable_bytes < 1073741824 OR node_filesystem_avail_bytes < 1073741824";
            rule.message_template = "Resources exhausted: memory={{mem_avail}}, disk={{disk_avail}}";
            rule.recommended_action = RecoveryAction::RESOURCE_CLEANUP;
            break;
        }
        case FailureScenario::MANIFEST_CORRUPTION: {
            rule.rule_id = "UPD_OD_006";
            rule.rule_name = "Updates Manifest Corruption";
            rule.severity = "CRITICAL";
            rule.condition = "updates_manifest_checksum_mismatch_total > 0 OR updates_state_corruption_total > 0";
            rule.message_template = "Manifest or state corruption detected; graceful degrade active";
            rule.recommended_action = RecoveryAction::GRACEFUL_DEGRADE;
            break;
        }
        case FailureScenario::CLUSTER_PARTITION: {
            rule.rule_id = "UPD_OD_007";
            rule.rule_name = "Updates Cluster Partition";
            rule.severity = "CRITICAL";
            rule.condition = "updates_cluster_partition_detected == 1";
            rule.message_template = "Cluster partition detected; {{isolated_nodes}} nodes isolated";
            rule.recommended_action = RecoveryAction::MANUAL_INTERVENTION;
            break;
        }
        case FailureScenario::DEADLOCK_RACE_CONDITION: {
            rule.rule_id = "UPD_OD_008";
            rule.rule_name = "Updates Deadlock/Race Condition";
            rule.severity = "CRITICAL";
            rule.condition = "increase(updates_operation_timeout_total[5m]) > 5";
            rule.message_template = "Potential deadlock detected; {{timeout_count}} operation timeouts";
            rule.recommended_action = RecoveryAction::MANUAL_INTERVENTION;
            break;
        }
    }

    return rule;
}

std::vector<AlertingRule> OperatorDiagnostics::getAllAlertingRules() const {
    std::vector<AlertingRule> rules;
    for (int i = 0; i <= static_cast<int>(FailureScenario::DEADLOCK_RACE_CONDITION); ++i) {
        rules.push_back(getAlertingRule(static_cast<FailureScenario>(i)));
    }
    return rules;
}

// ============================================================================
// Log Patterns and Metrics
// ============================================================================

std::vector<std::string> OperatorDiagnostics::getLogPatterns(FailureScenario scenario) const {
    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE:
            return {
                "coordination timeout",
                "quorum lost",
                "peer unreachable",
                "coordinator heartbeat failed"
            };
        case FailureScenario::PARTIAL_MIGRATION_FAILURE:
            return {
                "migration.*failed",
                "schema.*mismatch",
                "partial.*success"
            };
        case FailureScenario::CANARY_TIMEOUT_CYCLE:
            return {
                "canary.*timeout",
                "update.*stalled",
                "progress.*stalled"
            };
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE:
            return {
                "rollback.*failed",
                "blue_green.*failure",
                "version.*mismatch"
            };
        case FailureScenario::RESOURCE_EXHAUSTION:
            return {
                "out.of.memory",
                "disk full",
                "too many open files"
            };
        case FailureScenario::MANIFEST_CORRUPTION:
            return {
                "checksum.*mismatch",
                "state.*corrupt",
                "artifact.*invalid"
            };
        case FailureScenario::CLUSTER_PARTITION:
            return {
                "partition.*detected",
                "quorum.*lost",
                "network.*split"
            };
        case FailureScenario::DEADLOCK_RACE_CONDITION:
            return {
                "deadlock",
                "race condition",
                "operation.*hang",
                "circular.*dependency"
            };
        default:
            return {};
    }
}

std::vector<std::string> OperatorDiagnostics::getMetricsToTrack(FailureScenario scenario) const {
    switch (scenario) {
        case FailureScenario::COORDINATOR_UNREACHABLE:
            return {
                "updates_coordination_timeout_total",
                "updates_coordinator_quorum_healthy",
                "updates_coordinator_heartbeat_latency_ms"
            };
        case FailureScenario::PARTIAL_MIGRATION_FAILURE:
            return {
                "updates_migration_partial_failure_total",
                "updates_migration_nodes_failed",
                "updates_migration_duration_ms"
            };
        case FailureScenario::CANARY_TIMEOUT_CYCLE:
            return {
                "updates_canary_timeout_total",
                "updates_canary_apply_duration_ms",
                "updates_canary_node_cpu_usage_percent"
            };
        case FailureScenario::BLUE_GREEN_ROLLBACK_FAILURE:
            return {
                "updates_rollback_failed_total",
                "updates_rollback_nodes_recovered",
                "updates_blue_green_switch_latency_ms"
            };
        case FailureScenario::RESOURCE_EXHAUSTION:
            return {
                "node_memory_MemAvailable_bytes",
                "node_filesystem_avail_bytes",
                "process_open_fds"
            };
        case FailureScenario::MANIFEST_CORRUPTION:
            return {
                "updates_manifest_checksum_mismatch_total",
                "updates_state_corruption_total",
                "updates_artifact_download_errors_total"
            };
        case FailureScenario::CLUSTER_PARTITION:
            return {
                "updates_cluster_partition_detected",
                "updates_cluster_isolated_nodes",
                "updates_quorum_healthy_nodes"
            };
        case FailureScenario::DEADLOCK_RACE_CONDITION:
            return {
                "updates_operation_timeout_total",
                "updates_lock_contention_duration_ms",
                "updates_concurrent_operations_active"
            };
        default:
            return {};
    }
}

// ============================================================================
// Error Context Enrichment
// ============================================================================

bool OperatorDiagnostics::enrichErrorContext(ErrorContext& context) const {
    try {
        auto scenario = detectScenario(context);
        auto action = recommendRecoveryAction(context);
        auto rule = getAlertingRule(scenario);

        // Add scenario and recovery info to extra_context
        context.extra_context["scenario"] = getScenarioName(scenario);
        context.extra_context["recommended_action"] = static_cast<int>(action);
        context.extra_context["alert_rule_id"] = rule.rule_id;
        context.extra_context["runbook_url"] = rule.runbook_url;

        return true;
    } catch (const std::exception& e) {
        spdlog::warn("Failed to enrich error context: {}", e.what());
        return false;
    }
}

// ============================================================================
// JSON Export
// ============================================================================

json OperatorDiagnostics::exportScenariosAsJson() const {
    json scenarios = json::array();
    for (int i = 0; i <= static_cast<int>(FailureScenario::DEADLOCK_RACE_CONDITION); ++i) {
        auto scenario = static_cast<FailureScenario>(i);
        json s;
        s["id"] = i;
        s["name"] = getScenarioName(scenario);
        scenarios.push_back(s);
    }
    return scenarios;
}

json OperatorDiagnostics::exportProceduresAsJson() const {
    json procedures = json::array();
    for (int i = 0; i <= static_cast<int>(FailureScenario::DEADLOCK_RACE_CONDITION); ++i) {
        auto proc = getRecoveryProcedure(static_cast<FailureScenario>(i));
        procedures.push_back(proc.to_json());
    }
    return procedures;
}

json OperatorDiagnostics::exportAlertingRulesAsJson() const {
    json rules = json::array();
    for (const auto& rule : getAllAlertingRules()) {
        rules.push_back(rule.to_json());
    }
    return rules;
}

// ============================================================================
// Initialization
// ============================================================================

void OperatorDiagnostics::initializeRecoveryProcedures() {
    // Procedures are lazily generated on request by getRecoveryProcedure()
    // No persistent state needed for initialization
}

void OperatorDiagnostics::initializeAlertingRules() {
    // Rules are lazily generated on request by getAlertingRule()
    // No persistent state needed for initialization
}

}  // namespace updates
}  // namespace themis
