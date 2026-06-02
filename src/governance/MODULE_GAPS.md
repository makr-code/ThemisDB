# governance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: governance
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 249
- Actionable Findings (Critical + High): 1
- Affected Files: 22

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 1 |
| Medium | 246 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 210 |
| container | 171 |
| exception_safety | 54 |
| determinism | 34 |
| memory | 23 |
| performance | 16 |
| platform | 14 |
| reliability | 12 |
| raii | 8 |
| concurrency | 3 |
| legacy_duplication | 3 |
| audit_logging | 2 |
| observability | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/governance/compliance_reporting.cpp | 52 | 0 | 0 | 52 | 0 |
| src/governance/policy_validation.cpp | 51 | 0 | 0 | 51 | 0 |
| src/governance/compliance_reporter.cpp | 32 | 0 | 0 | 30 | 2 |
| src/governance/policy_manager.cpp | 19 | 0 | 0 | 19 | 0 |
| src/governance/policy_review.cpp | 17 | 0 | 0 | 17 | 0 |
| src/governance/policy_validator.cpp | 14 | 0 | 0 | 14 | 0 |
| src/governance/iso27001_rules.cpp | 8 | 0 | 1 | 7 | 0 |
| src/governance/policy_version_history.cpp | 8 | 0 | 0 | 8 | 0 |
| src/governance/soc2_controls.cpp | 7 | 0 | 0 | 7 | 0 |
| src/governance/review_scheduler.cpp | 6 | 0 | 0 | 6 | 0 |
| src/governance/ccpa_rules.cpp | 5 | 0 | 0 | 5 | 0 |
| src/governance/gdpr_subject_rights.cpp | 5 | 0 | 0 | 5 | 0 |
| src/governance/cross_tenant_policy_inheritance.cpp | 4 | 0 | 0 | 4 | 0 |
| src/governance/data_lineage.cpp | 4 | 0 | 0 | 4 | 0 |
| src/governance/data_masker.cpp | 4 | 0 | 0 | 4 | 0 |
| src/governance/policy_template.cpp | 4 | 0 | 0 | 4 | 0 |
| src/governance/policy_engine.cpp | 3 | 0 | 0 | 3 | 0 |
| src/governance/policy_manager_versioned.cpp | 3 | 0 | 0 | 3 | 0 |
| src/governance/hipaa_rules.cpp | 1 | 0 | 0 | 1 | 0 |
| src/governance/opa_adapter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/governance/pci_dss_rules.cpp | 1 | 0 | 0 | 1 | 0 |
| src/governance/policy_file_watcher.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/governance/compliance_reporting.cpp
Total findings: 52

- Line 29: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> collect_action_candidates(const PolicyManager& policy_mgr) {
  Confidence: band=medium; score=0.66
- Line 30: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> action_candidates = {"*"};
  Confidence: band=medium; score=0.66
- Line 49: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& action_candidates
  Confidence: band=medium; score=0.66
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_rule_ids;
  Confidence: band=medium; score=0.66
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: applicable_rules.push_back(rule);
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: applicable_rules.push_back(rule);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.uncovered_resource_list.push_back(resource);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.uncovered_resource_list.push_back(resource);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> pattern_map;
  Confidence: band=medium; score=0.66
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_map[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_map[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_map[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overlaps.push_back(overlap);
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps.push_back(resource);
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_json.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gap.affected_resources.push_back(resource);
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gap.affected_resources.push_back(resource);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_controls.push_back("encryption");
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) s += ", ";
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) s += ", ";
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gap.affected_resources.push_back(resource);
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered_reqs.push_back(req);
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: requirements_.push_back(ComplianceRequirement::fromJson(req_json));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reqs_json.push_back(req.toJson());
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_json.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_json.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 621: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries_json.push_back(entry_json);
  Confidence: band=high; score=0.74
- Line 692: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: risks_json.push_back(risk_json);
  Confidence: band=high; score=0.74
- Line 781: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: changes_json.push_back(change_json);
  Confidence: band=high; score=0.74
- Line 916: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.compliant_controls.push_back(req.name);
  Confidence: band=high; score=0.74
- Line 916: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.compliant_controls.push_back(req.name);
  Confidence: band=high; score=0.74
- Line 951: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix.entries.push_back(entry);
  Confidence: band=high; score=0.74
- Line 951: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix.entries.push_back(entry);
  Confidence: band=high; score=0.74
- Line 951: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix.entries.push_back(entry);
  Confidence: band=high; score=0.74
- Line 991: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.risks.push_back(risk);
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.risks.push_back(risk);
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.risks.push_back(risk);
  Confidence: band=high; score=0.74
- Line 1037: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.risks.push_back(risk);
  Confidence: band=high; score=0.74
- Line 1078: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.changes.push_back(version);
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '(')       result += "\\(";
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '(')       result += "\\(";
  Confidence: band=high; score=0.74
- Line 1099: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '(')       result += "\\(";
  Confidence: band=high; score=0.74
- Line 1234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(full_key + ":");
  Confidence: band=high; score=0.74
- Line 1245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(idx_prefix + ":");
  Confidence: band=high; score=0.74
- Line 1296: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: page_streams.back() += "ET\n";
  Confidence: band=high; score=0.74
- Line 1297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: page_streams.push_back("");
  Confidence: band=high; score=0.74
- Line 1361: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: kids += std::to_string(base_page + i) + " 0 R ";
  Confidence: band=high; score=0.74
- Line 1397: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "xref\n";
  Confidence: band=high; score=0.74
- Line 1397: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "xref\n";
  Confidence: band=high; score=0.74
- Line 1409: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "trailer\n";
  Confidence: band=high; score=0.74
- Line 1547: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> categories_seen;
  Confidence: band=medium; score=0.66
- Line 1559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.third_party_disclosure_rule_ids.push_back(rule.id);
  Confidence: band=high; score=0.74

### src/governance/policy_validation.cpp
Total findings: 51

- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_arr.push_back(conflict.toJson());
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: security_arr.push_back(check.toJson());
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> pattern_rules;
  Confidence: band=medium; score=0.66
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_rules[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_rules[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pattern_rules[key].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(conflict);
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> conflict_graph;
  Confidence: band=medium; score=0.66
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflict_graph[r1.id].push_back(r2.id);
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: component.push_back(cur);
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(std::move(conflict));
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, PolicyValidator::EffectivenessMetrics>
  Confidence: band=medium; score=0.66
- Line 400: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, int> &hit_counts) const {
  Confidence: band=medium; score=0.66
- Line 401: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, EffectivenessMetrics> metrics_map;
  Confidence: band=medium; score=0.66
- Line 459: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, int> &hit_counts,
  Confidence: band=medium; score=0.66
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unused_rules.push_back(rule_id);
  Confidence: band=high; score=0.74
- Line 543: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks.push_back(check);
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks.push_back(check);
  Confidence: band=high; score=0.74
- Line 647: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks.push_back(check);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks.push_back(check);
  Confidence: band=high; score=0.74
- Line 717: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, int> &hit_counts) const {
  Confidence: band=medium; score=0.66
- Line 741: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.recommendations.push_back("Address " + std::to_string(report.conflicts_found) + " rule conflicts");
  Confidence: band=high; score=0.74
- Line 761: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PolicyValidator::validateSingleRule(const PolicyRule &rule)
  Context: std::vector<PolicyValidator::SecurityCheckResult> PolicyValidator::validateSingleRule(const PolicyRule &rule) const {
  Confidence: band=medium; score=0.56
- Line 791: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks.push_back(check);
  Confidence: band=high; score=0.74
- Line 901: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> PolicyMetricsCollector::getAllMetrics() const {
  Confidence: band=medium; score=0.66
- Line 952: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slow_rules.push_back(rule_id);
  Confidence: band=high; score=0.74
- Line 965: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(metrics.toJson());
  Confidence: band=high; score=0.74
- Line 1055: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recs_arr.push_back(rec.toJson());
  Confidence: band=high; score=0.74
- Line 1067: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
  Confidence: band=medium; score=0.66
- Line 1085: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> hit_counts;
  Confidence: band=medium; score=0.66
- Line 1105: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> similar_groups;
  Confidence: band=medium; score=0.66
- Line 1114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: signature += resource + ";";
  Confidence: band=high; score=0.74
- Line 1114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: signature += resource + ";";
  Confidence: band=high; score=0.74
- Line 1114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: signature += resource + ";";
  Confidence: band=high; score=0.74
- Line 1118: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: signature += action + ";";
  Confidence: band=high; score=0.74
- Line 1121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar_groups[signature].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 1121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar_groups[signature].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 1121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar_groups[signature].push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 1136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 1136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 1194: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
  Confidence: band=medium; score=0.66
- Line 1226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule_stats.push_back(stats);
  Confidence: band=high; score=0.74
- Line 1241: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 1253: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, int> &hit_counts) const {
  Confidence: band=medium; score=0.66
- Line 1305: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
  Confidence: band=medium; score=0.66
- Line 1326: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> type_counts;
  Confidence: band=medium; score=0.66
- Line 1376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(std::move(result));
  Confidence: band=high; score=0.74

### src/governance/compliance_reporter.cpp
Total findings: 32

- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back("[" + gap.severity + "] " + gap.gap_type + ": " + gap.description);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: page_streams.back() += "BT\n";
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: page_streams.back() += "ET\n";
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: kids += std::to_string(base_page + i) + " 0 R ";
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "xref\n";
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "xref\n";
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pdf += "trailer\n";
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fs_arr.push_back(fs.toJson());
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fs_arr.push_back(fs.toJson());
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_array.push_back(gap.toJson());
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: analysis.uncovered_resources.push_back(resource);
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: analysis.uncovered_resources.push_back(resource);
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: analysis.overlapping_rules.push_back(rule1 + " <-> " + rule2);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overlaps.push_back({rule1.id, rule2.id});
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overlaps.push_back({rule1.id, rule2.id});
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: no_encryption.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 610: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> categories_seen;
  Confidence: band=medium; score=0.66
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: third_party_disclosure_rule_ids.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 637: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_by_check[eval.ccpa_check_id].push_back(eval.rule_id);
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: uncovered.push_back(res);
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: uncovered.push_back(res);
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix[resource][action].push_back(role);
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix[resource][action].push_back(role);
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix[resource][action].push_back(role);
  Confidence: band=high; score=0.74
- Line 739: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matrix[resource][action].push_back(role);
  Confidence: band=high; score=0.74
- Line 853: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::unordered_map<std::string, size_t>> &raw_field_stats) const {
  Confidence: band=medium; score=0.66
- Line 910: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.recommendations.push_back(
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 1012: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gaps_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 900: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: actual_entropy -= p * std::log(p);
  Confidence: band=medium; score=0.6
- Line 903: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double max_entropy    = (group_map.size() > 1) ? std::log(static_cast<double>(group_map.size())) : 1.0;
  Confidence: band=medium; score=0.6

### src/governance/policy_manager.cpp
Total findings: 19

- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rule);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto search = [&](const std::unordered_map<std::string, PolicyRule> &rules) {
  Confidence: band=medium; score=0.66
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: applicable.push_back(rule);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: applicable.push_back(rule);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decision.applied_rules.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> id_counts;
  Confidence: band=medium; score=0.66
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Duplicate rule ID: " + id);
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Duplicate rule ID: " + id);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("Potential conflict between rules " + id1 + " and " + id2);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("Potential conflict between rules " + id1 + " and " + id2);
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["rules"].push_back(rule.toJson());
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["rules"].push_back(rule.toJson());
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rule_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 716: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(v);
  Confidence: band=high; score=0.74
- Line 716: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(v);
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: concat += '|';
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: concat += '|';
  Confidence: band=high; score=0.74
- Line 783: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: concat += '|';
  Confidence: band=high; score=0.74

### src/governance/policy_review.cpp
Total findings: 17

- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: due_rules.push_back(schedule.rule_id);
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overdue_rules.push_back(schedule.rule_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schedules_arr.push_back(pair.second.toJson());
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reviews_arr.push_back(pair.second.toJson());
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 603: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expired_rules.push_back(config.rule_id);
  Confidence: band=high; score=0.74
- Line 647: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings.push_back(warning);
  Confidence: band=high; score=0.74
- Line 686: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: disabled_rules.push_back(config.rule_id);
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: expirations_arr.push_back(pair.second.toJson());
  Confidence: band=high; score=0.74
- Line 934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 960: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74

### src/governance/policy_validator.cpp
Total findings: 14

- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts_array.push_back(c.toJson());
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: violations_array.push_back(v.toJson());
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metrics_array.push_back(m.toJson());
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(conflict);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflict_graph[r1.id].push_back(r2.id);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflict_graph[r1.id].push_back(r2.id);
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: component.push_back(cur);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: circular.push_back(conflict);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unused.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overly_permissive.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hipaa_ccpa_conflict_rules.push_back(rule.id);
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PolicyValidator::validateSingleRule(const PolicyRule &rule)
  Context: std::vector<std::string> PolicyValidator::validateSingleRule(const PolicyRule &rule) const {
  Confidence: band=medium; score=0.56
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back("Rule ID is required");
  Confidence: band=high; score=0.74

### src/governance/iso27001_rules.cpp
Total findings: 8

- Line 43: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (res.find(kw) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ev_arr.push_back(ev.toJson());
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res_arr.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ev_arr.push_back(ev.toJson());
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(ctrl->evaluate(rule));
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74

### src/governance/policy_version_history.cpp
Total findings: 8

- Line 67: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: AuditLogEntry::toJson()
  Context: nlohmann::json AuditLogEntry::toJson() const {
  Confidence: band=medium; score=0.56
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: version_array.push_back(v.toJson());
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: version_array.push_back(v.toJson());
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit_array.push_back(entry.toJson());
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(PolicyRuleVersion::fromJson(v_json));
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(PolicyRuleVersion::fromJson(v_json));
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit_log_.push_back(AuditLogEntry::fromJson(entry_json));
  Confidence: band=high; score=0.74

### src/governance/soc2_controls.cpp
Total findings: 7

- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ev_arr.push_back(ev.toJson());
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results_arr.push_back(r.toJson());
  Confidence: band=high; score=0.74
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ev_arr.push_back(ev.toJson());
  Confidence: band=high; score=0.74
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(ctrl->evaluate(rule));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.evidence_items.push_back(ev);
  Confidence: band=high; score=0.74

### src/governance/review_scheduler.cpp
Total findings: 6

- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pending.push_back(review);
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overdue.push_back(review);
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history.push_back(review);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: due_rules.push_back(rule_id);
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schedules_array.push_back(schedule.toJson());
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reviews_array.push_back(review.toJson());
  Confidence: band=high; score=0.74

### src/governance/ccpa_rules.cpp
Total findings: 5

- Line 141: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: void CcpaRuleSet::setOptOutRegistry(const std::unordered_set<std::string> &subjects) {
  Confidence: band=medium; score=0.66
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(res));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back("Rule '" + rule.id
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(req);
  Confidence: band=high; score=0.74

### src/governance/gdpr_subject_rights.cpp
Total findings: 5

- Line 24: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> ErasureReport::toSummaryMap() const {
  Confidence: band=medium; score=0.66
- Line 25: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> m;
  Confidence: band=medium; score=0.66
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.store_results.push_back(std::move(res));
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined += ",";
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined += '\n';
  Confidence: band=high; score=0.74

### src/governance/cross_tenant_policy_inheritance.cpp
Total findings: 4

- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tid);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: managers.push_back((it != tenants_.end()) ? it->second.policy_manager : nullptr);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: managers.push_back((it != tenants_.end()) ? it->second.policy_manager : nullptr);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto rules = managers[i]->listRules();
  Confidence: band=high; score=0.74

### src/governance/data_lineage.cpp
Total findings: 4

- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chain.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ev);
  Confidence: band=high; score=0.74

### src/governance/data_masker.cpp
Total findings: 4

- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, rule_index));
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(elem, key, rule_index));
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(doc, "", rule_index));
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(maskNode(doc, "", rule_index));
  Confidence: band=high; score=0.74

### src/governance/policy_template.cpp
Total findings: 4

- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params_json.push_back(param.toJson());
  Confidence: band=high; score=0.74
- Line 484: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 523: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(tmpl->toJson());
  Confidence: band=high; score=0.74

### src/governance/policy_engine.cpp
Total findings: 3

- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ClassificationProfile> new_profiles;
  Confidence: band=medium; score=0.66
- Line 53: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> new_mapping;
  Confidence: band=medium; score=0.66
- Line 641: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: PolicyEngine::checkInferencePermission(const std::unordered_map<std::string, std::string> &headers) const {
  Confidence: band=medium; score=0.66

### src/governance/policy_manager_versioned.cpp
Total findings: 3

- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back(makeContradictoryConflict(new_rule, existing));
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_conflicts.push_back(makeContradictoryConflict(rules[i], rules[j]));
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_conflicts.push_back(makeContradictoryConflict(rules[i], rules[j]));
  Confidence: band=high; score=0.74

### src/governance/hipaa_rules.cpp
Total findings: 1

- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(res));
  Confidence: band=high; score=0.74

### src/governance/opa_adapter.cpp
Total findings: 1

- Line 78: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::string OpaAdapter::buildRequestBody(const std::unordered_map<std::string, std::string> &headers,
  Confidence: band=medium; score=0.66

### src/governance/pci_dss_rules.cpp
Total findings: 1

- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(res));
  Confidence: band=high; score=0.74

### src/governance/policy_file_watcher.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
