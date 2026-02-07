# GAP-004 Phase 5: Enterprise Policy Features - Documentation

## Overview

This document provides comprehensive documentation for the GAP-004 Phase 5 enterprise policy features implemented for ThemisDB PolicyManager. These features transform PolicyManager from a basic RBAC engine into an enterprise-grade governance platform.

## Table of Contents

1. [Feature 1: Policy Versioning & History](#feature-1-policy-versioning--history)
2. [Feature 2: Policy Templates](#feature-2-policy-templates)
3. [Feature 3: Compliance Reporting](#feature-3-compliance-reporting)
4. [Feature 4: Automated Policy Validation](#feature-4-automated-policy-validation)
5. [Feature 5: Scheduled Policy Reviews](#feature-5-scheduled-policy-reviews)
6. [Usage Examples](#usage-examples)
7. [API Reference](#api-reference)
8. [Performance Considerations](#performance-considerations)
9. [Security Considerations](#security-considerations)

---

## Feature 1: Policy Versioning & History

### Purpose
Provides complete audit trail and rollback capability for policy rules, ensuring compliance with regulations like SOX, GDPR, and HIPAA.

### Key Components

#### PolicyRule Extensions
```cpp
std::string version = "1.0.0";        // Semantic versioning
std::string last_modified_by;          // Last modifier
std::string change_description;        // Description of changes
```

#### PolicyVersionHistory
Manages historical versions of policy rules:
- Stores complete snapshots of rule changes
- Tracks modification timestamps and authors
- Supports version comparison and diff generation
- Provides audit trail with time-based filtering

### Usage Example

```cpp
PolicyManager manager;

// Add a rule
PolicyRule rule;
rule.id = "rule_001";
rule.name = "Sensitive Data Access";
rule.require_encryption = false;
manager.addRule(rule);

// Update the rule (creates version 1.0.1)
rule.require_encryption = true;
manager.updateRule("rule_001", rule, "admin", "Added encryption requirement");

// View version history
auto versions = manager.getRuleVersions("rule_001");
for (const auto& version : versions) {
    std::cout << "Version: " << version.version 
              << " Modified by: " << version.modified_by 
              << " At: " << version.timestamp << std::endl;
}

// Rollback to previous version
manager.rollbackToVersion("rule_001", "1.0.0", "admin");

// Preview rollback changes before applying
auto diffs = manager.previewRollback("rule_001", "1.0.0");
for (const auto& diff : diffs) {
    std::cout << diff.field << ": " << diff.old_value << " -> " << diff.new_value << std::endl;
}

// Query audit trail
auto trail = manager.getAuditTrail("rule_001", start_time, end_time);
auto user_trail = manager.getAuditTrailByUser("admin", start_time, end_time);
```

### API Methods
- `updateRule()` - Update rule with version tracking
- `getRuleVersions()` - Get all versions of a rule
- `getRuleVersion()` - Get specific version
- `rollbackToVersion()` - Rollback to any version
- `rollbackToPreviousVersion()` - Quick rollback
- `previewRollback()` - Preview changes before rollback
- `compareRuleVersions()` - Compare two versions
- `getAuditTrail()` - Get audit trail with time filtering
- `getAuditTrailByUser()` - Filter by user

---

## Feature 2: Policy Templates

### Purpose
Accelerates policy deployment and ensures consistency through parameterized policy templates.

### Built-in Templates

1. **Least Privilege** (Security)
   - Minimizes permissions to specific resources and actions
   - Parameters: resource_path, allowed_action, role
   - Enforces: encryption, no export, audit access

2. **Data Lifecycle** (Compliance)
   - Manages data retention and archival
   - Parameters: data_type, retention_days
   - Enforces: audit changes, configurable retention

3. **Compliance Audit** (Compliance)
   - Enforces audit logging and encryption
   - Parameters: resource_category, classification
   - Enforces: encryption, signature, no export, 7-year retention

4. **Separation of Duties** (Security)
   - Ensures role segregation for sensitive operations
   - Parameters: resource, action, authorized_role
   - Enforces: encryption, audit access and changes

5. **Time-based Access** (Security)
   - Temporary access with expiration
   - Parameters: resource, temp_role, duration_days
   - Enforces: audit access, configurable duration

### Usage Example

```cpp
PolicyTemplateManager template_mgr;

// List available templates
auto templates = template_mgr.listTemplates();
auto security_templates = template_mgr.listTemplatesByCategory("security");

// Instantiate a template
std::unordered_map<std::string, std::string> params;
params["resource_path"] = "data/financial/*";
params["allowed_action"] = "read";
params["role"] = "accountant";

auto rule = template_mgr.instantiateTemplate(
    "least_privilege", 
    "rule_financial_read", 
    params, 
    "admin"
);

if (rule) {
    manager.addRule(*rule);
}

// Preview template before instantiation
auto preview = template_mgr.previewTemplate(
    "least_privilege",
    "rule_preview",
    params
);

if (preview.valid) {
    std::cout << "Generated rule: " << preview.rule.name << std::endl;
} else {
    for (const auto& warning : preview.warnings) {
        std::cout << "Warning: " << warning << std::endl;
    }
}

// Create custom template
PolicyTemplate custom;
custom.id = "custom_template";
custom.name = "Custom Security Template";
custom.category = "security";

TemplateParameter param;
param.name = "department";
param.type = "string";
param.required = true;
custom.parameters.push_back(param);

custom.name_template = "Department Access: {{department}}";
custom.resources_template = {"{{department}}/*"};
custom.actions_template = {"read"};

template_mgr.addTemplate(custom);
```

### API Methods
- `addTemplate()` - Add custom template
- `getTemplate()` - Get template by ID
- `listTemplates()` - List all templates
- `listTemplatesByCategory()` - Filter by category
- `instantiateTemplate()` - Create rule from template
- `previewTemplate()` - Preview without creating
- `loadTemplates()` / `saveTemplates()` - File I/O

---

## Feature 3: Compliance Reporting

### Purpose
Provides visibility into policy coverage, compliance gaps, and generates audit reports for stakeholders.

### Key Components

#### PolicyCoverageAnalyzer
Analyzes policy coverage across resources:
- Calculates coverage percentage
- Identifies uncovered resources
- Detects overlapping rules
- Finds policy gaps

#### ComplianceGapDetector
Compares policies against compliance requirements:
- Supports multiple frameworks (GDPR, SOC2, HIPAA, ISO27001, PCI-DSS)
- Identifies missing controls
- Flags non-compliant configurations
- Calculates compliance percentage

#### ComplianceReporter
Generates five types of audit reports:
1. **Policy Summary** - Statistics, classification breakdown
2. **Compliance Status** - Framework compliance, gaps
3. **Access Control Matrix** - Role-resource-action matrix
4. **Risk Assessment** - Security risks by severity
5. **Change History** - Audit trail with user activity

### Usage Example

```cpp
PolicyCoverageAnalyzer coverage_analyzer;
ComplianceGapDetector gap_detector;
ComplianceReporter reporter;

// Analyze coverage
std::vector<std::string> resources = {
    "data/users", "data/orders", "data/payments"
};
auto coverage = coverage_analyzer.analyzeCoverage(manager, resources);
std::cout << "Coverage: " << coverage.coverage_percentage << "%" << std::endl;

// Detect overlaps
auto overlaps = coverage_analyzer.detectOverlaps(manager);
for (const auto& overlap : overlaps) {
    std::cout << "Overlap: " << overlap.resource_pattern 
              << " (" << overlap.overlap_count << " rules)" << std::endl;
}

// Add compliance requirements
ComplianceGapDetector::ComplianceRequirement req;
req.id = "gdpr_encryption";
req.name = "GDPR Data Encryption";
req.framework = "GDPR";
req.required_resources = {"data/personal/*"};
req.requires_encryption = true;
gap_detector.addRequirement(req);

// Detect compliance gaps
auto gaps = gap_detector.detectGaps(manager);
for (const auto& gap : gaps) {
    std::cout << "Gap: " << gap.requirement_name 
              << " - " << gap.description << std::endl;
}

// Get compliance status
auto status = gap_detector.getComplianceStatus(manager, "GDPR");
std::cout << "GDPR Compliance: " << status.compliance_percentage << "%" << std::endl;

// Generate reports
auto summary = reporter.generatePolicySummary(manager);
auto compliance = reporter.generateComplianceStatus(manager, gap_detector, "GDPR");
auto matrix = reporter.generateAccessControlMatrix(manager);
auto risks = reporter.generateRiskAssessment(manager);
auto history = reporter.generateChangeHistory(manager, start_time, end_time);

// Export reports
std::string json_report = reporter.exportReport(summary.toJson(), ReportFormat::JSON);
std::string csv_report = reporter.exportReport(summary.toJson(), ReportFormat::CSV);
std::string html_report = reporter.exportReport(summary.toJson(), ReportFormat::HTML);
```

### Export Formats
- **JSON**: Full structured data
- **CSV**: Spreadsheet-compatible tables
- **HTML**: Styled web reports

---

## Feature 4: Automated Policy Validation

### Purpose
Ensures policy quality through automated validation, conflict detection, and optimization recommendations.

### Key Components

#### PolicyValidator
Performs comprehensive validation:
- **Conflict Detection**: Contradictory rules, overlapping permissions, circular dependencies
- **Effectiveness Metrics**: Usage statistics, unused rule detection, scoring
- **Security Checks**: Overly permissive rules, encryption requirements, audit logging, retention compliance

#### PolicyMetricsCollector
Tracks policy performance:
- Evaluation count and match rate
- Evaluation times (microseconds)
- Performance impact analysis
- Identifies slow rules

#### PolicyOptimizer
Provides intelligent recommendations:
- Merge similar rules
- Simplify complex rules
- Reorder for performance
- Remove unused/redundant rules

### Usage Example

```cpp
PolicyValidator validator;
PolicyMetricsCollector metrics_collector;
PolicyOptimizer optimizer;

// Detect conflicts
auto conflicts = validator.detectConflicts(manager);
for (const auto& conflict : conflicts) {
    std::cout << "Conflict (" << conflict.severity << "): " 
              << conflict.description << std::endl;
    std::cout << "Recommendation: " << conflict.recommendation << std::endl;
}

// Calculate effectiveness
std::unordered_map<std::string, int> hit_counts;
hit_counts["rule_001"] = 1000;
hit_counts["rule_002"] = 0;

auto effectiveness = validator.calculateEffectiveness(manager, hit_counts);
for (const auto& [rule_id, metrics] : effectiveness) {
    std::cout << "Rule: " << rule_id 
              << " Score: " << metrics.effectiveness_score 
              << " Unused: " << metrics.is_unused << std::endl;
}

// Perform security checks
auto security_checks = validator.performSecurityChecks(manager);
for (const auto& check : security_checks) {
    if (!check.passed) {
        std::cout << "Security Issue (" << check.severity << "): " 
                  << check.description << std::endl;
    }
}

// Generate validation report
auto report = validator.generateValidationReport(manager, hit_counts);
std::cout << "Total rules: " << report.total_rules_checked << std::endl;
std::cout << "Conflicts: " << report.conflicts_found << std::endl;
std::cout << "Security issues: " << report.security_issues_found << std::endl;

// Track metrics
metrics_collector.recordEvaluation("rule_001", true, 500); // 500 microseconds
auto rule_metrics = metrics_collector.getRuleMetrics("rule_001");
std::cout << "Average eval time: " << rule_metrics->avg_evaluation_time_us << "us" << std::endl;

// Get optimization recommendations
auto all_metrics = metrics_collector.getAllMetrics();
auto recommendations = optimizer.generateRecommendations(manager, report, all_metrics);
for (const auto& rec : recommendations) {
    std::cout << "Recommendation (" << rec.priority << "): " 
              << rec.description << std::endl;
}
```

---

## Feature 5: Scheduled Policy Reviews

### Purpose
Ensures ongoing policy hygiene through scheduled reviews, expiration management, and automated notifications.

### Key Components

#### ReviewScheduler
Manages review schedules:
- Configurable review periods (30/60/90 days)
- Automatic next review date calculation
- Due and overdue review tracking

#### ReviewWorkflow
Handles review approval process:
- Create, approve, reject reviews
- Track review history
- Status management (pending, approved, rejected)

#### PolicyExpiration
Manages rule expiration:
- Automatic expiration with grace periods
- Multi-level warnings (30, 14, 7 days)
- Automatic rule disabling after grace period

#### NotificationManager
Sends notifications:
- Email and webhook support
- Review due/overdue notifications
- Expiration warnings
- Notification history

### Usage Example

```cpp
ReviewScheduler scheduler;
ReviewWorkflow workflow;
PolicyExpiration expiration;
NotificationManager notification_mgr;

// Configure notifications
NotificationManager::NotificationConfig config;
config.email_enabled = true;
config.smtp_server = "smtp.example.com";
config.from_email = "policy@example.com";
config.webhook_enabled = true;
config.webhook_url = "https://example.com/webhook";
notification_mgr.configure(config);

// Schedule reviews
scheduler.setSchedule("rule_001", 90); // Review every 90 days

// Get due reviews
auto due_reviews = scheduler.getRulesDueForReview();
for (const auto& rule_id : due_reviews) {
    // Create review request
    auto review_id = workflow.createReview(
        rule_id,
        "reviewer@example.com",
        "admin@example.com",
        7 // 7 days to complete
    );
    
    // Send notification
    auto review = workflow.getReview(review_id);
    notification_mgr.notifyReviewDue("reviewer@example.com", *review);
}

// Approve review
workflow.approveReview(review_id, "Policy looks good");
scheduler.markAsReviewed("rule_001");

// Set expiration
int64_t expiration_date = /* 90 days from now */;
expiration.setExpiration("rule_temp_001", expiration_date, 7);

// Check expiring rules
auto warnings = expiration.getRulesExpiringSoon();
for (const auto& warning : warnings) {
    notification_mgr.notifyExpirationWarning(
        "admin@example.com",
        warning
    );
}

// Process expirations (disable expired rules)
auto expired = expiration.processExpirations(manager);
for (const auto& rule_id : expired) {
    notification_mgr.notifyRuleExpired("admin@example.com", rule_id);
}

// Get review history
auto history = workflow.getReviewHistory("rule_001");
for (const auto& review : history) {
    std::cout << "Review: " << review.status 
              << " by " << review.reviewer 
              << " at " << review.completed_at << std::endl;
}
```

---

## Performance Considerations

### Recommended Limits
- **Rules**: Up to 1,000 rules with <5s report generation
- **Validation**: <2s for 1,000 rules
- **Policy Evaluation**: <1ms per evaluation
- **Version History**: Unlimited versions (storage permitting)

### Optimization Tips
1. Use `PolicyMetricsCollector` to identify slow rules
2. Reorder rules by priority (higher priority = evaluated first)
3. Use specific resource patterns instead of wildcards when possible
4. Regularly review and remove unused rules
5. Batch validation and report generation off-peak

---

## Security Considerations

### Best Practices
1. **Encryption**: Always enable `require_encryption` for sensitive data
2. **Audit Logging**: Enable `audit_access` and `audit_changes` for compliance
3. **Least Privilege**: Use templates to enforce principle of least privilege
4. **Regular Reviews**: Schedule reviews every 90 days minimum
5. **Version Control**: Never delete version history
6. **Validation**: Run `PolicyValidator` after any policy changes

### Security Features
- Thread-safe operations with mutex protection
- Complete audit trail with timestamps
- Conflict detection prevents security gaps
- Automated expiration prevents stale policies
- Security checks validate best practices

---

## Migration from Phase 2 to Phase 5

### Backward Compatibility
All Phase 5 features are backward compatible with Phase 2. Existing rules work without modification.

### Migration Steps
1. Enable versioning: Existing rules get version "1.0.0" automatically
2. Load built-in templates: `PolicyTemplateManager` auto-loads on construction
3. Add compliance requirements: Use `ComplianceGapDetector` for your frameworks
4. Schedule reviews: Set review periods for critical rules
5. Configure notifications: Set up email/webhook for alerts

### Example Migration
```cpp
// Phase 2 code (still works)
PolicyManager manager;
manager.loadRules("existing_rules.json");

// Phase 5 enhancements (additive)
PolicyTemplateManager templates;  // Auto-loads built-in templates
ReviewScheduler scheduler;
scheduler.setSchedule("existing_rule", 90);

ComplianceGapDetector compliance;
// Load compliance requirements...
```

---

## API Reference Summary

### PolicyManager (Phase 5 Extensions)
- `updateRule()` - Update with versioning
- `getRuleVersions()` - Get version history
- `rollbackToVersion()` - Rollback to version
- `compareRuleVersions()` - Compare versions
- `getAuditTrail()` - Get audit trail

### PolicyTemplateManager
- `listTemplates()` - List all templates
- `getTemplate()` - Get template by ID
- `instantiateTemplate()` - Create rule from template
- `previewTemplate()` - Preview instantiation

### PolicyCoverageAnalyzer
- `analyzeCoverage()` - Analyze coverage
- `detectOverlaps()` - Find overlapping rules
- `findGaps()` - Identify policy gaps

### ComplianceGapDetector
- `addRequirement()` - Add compliance requirement
- `detectGaps()` - Find compliance gaps
- `getComplianceStatus()` - Get compliance status

### ComplianceReporter
- `generatePolicySummary()` - Generate summary
- `generateComplianceStatus()` - Generate compliance report
- `generateAccessControlMatrix()` - Generate matrix
- `generateRiskAssessment()` - Generate risk report
- `generateChangeHistory()` - Generate history report

### PolicyValidator
- `detectConflicts()` - Find conflicts
- `calculateEffectiveness()` - Calculate metrics
- `performSecurityChecks()` - Run security checks
- `generateValidationReport()` - Generate report

### PolicyMetricsCollector
- `recordEvaluation()` - Record evaluation
- `getRuleMetrics()` - Get metrics
- `analyzePerformanceImpact()` - Analyze performance

### PolicyOptimizer
- `generateRecommendations()` - Get recommendations
- `generateOptimizationReport()` - Generate report

### ReviewScheduler
- `setSchedule()` - Set review schedule
- `getRulesDueForReview()` - Get due reviews
- `markAsReviewed()` - Mark as reviewed

### ReviewWorkflow
- `createReview()` - Create review
- `approveReview()` - Approve review
- `rejectReview()` - Reject review
- `listPendingReviews()` - List pending

### PolicyExpiration
- `setExpiration()` - Set expiration
- `getRulesExpiringSoon()` - Get warnings
- `processExpirations()` - Process expirations

### NotificationManager
- `configure()` - Configure notifications
- `notifyReviewDue()` - Send review notification
- `notifyExpirationWarning()` - Send expiration warning

---

## Testing Summary

### Test Coverage
- **Feature 1**: 18 tests (versioning, rollback, audit trail)
- **Feature 2**: 28 tests (templates, instantiation, validation)
- **Feature 3**: 50 tests (coverage, compliance, reports)
- **Feature 4**: 60 tests (validation, metrics, optimization)
- **Feature 5**: 66 tests (reviews, expiration, notifications)

**Total**: 222+ comprehensive tests with >95% code coverage

### Running Tests
```bash
# Run all policy tests
./build/tests/themis_tests --gtest_filter="Policy*"

# Run specific feature tests
./build/tests/themis_tests --gtest_filter="PolicyManagerTest.*"
./build/tests/themis_tests --gtest_filter="PolicyTemplateTest.*"
./build/tests/themis_tests --gtest_filter="ComplianceReportingTest.*"
./build/tests/themis_tests --gtest_filter="PolicyValidationTest.*"
./build/tests/themis_tests --gtest_filter="PolicyReviewTest.*"
```

---

## Support and Troubleshooting

### Common Issues

1. **Version mismatch on rollback**
   - Ensure version exists in history before rollback
   - Use `getRuleVersions()` to verify available versions

2. **Template instantiation fails**
   - Check required parameters are provided
   - Validate parameter values against allowed_values
   - Use `previewTemplate()` to see validation errors

3. **Compliance gaps detected**
   - Review requirement definitions
   - Ensure policies cover all required resources
   - Check encryption/audit settings

4. **Slow validation**
   - Reduce number of rules being validated
   - Use `PolicyMetricsCollector` to identify slow rules
   - Consider rule reordering

5. **Notifications not sent**
   - Verify NotificationConfig is set
   - Check SMTP/webhook settings
   - Review notification queue with `getPendingNotifications()`

### Logging
All features use ThemisDB logging:
- `THEMIS_DEBUG`: Detailed operation logs
- `THEMIS_INFO`: Important events
- `THEMIS_ERROR`: Errors and failures

Enable debug logging for troubleshooting:
```cpp
// Set log level to DEBUG
themis::utils::Logger::setLevel(themis::utils::LogLevel::DEBUG);
```

---

## License and Attribution

Part of ThemisDB - GAP-004 Security & Governance Implementation
Copyright © 2026 ThemisDB Team

---

## Change Log

### Version 5.0.0 (February 2026)
- ✅ Feature 1: Policy Versioning & History
- ✅ Feature 2: Policy Templates (5 built-in templates)
- ✅ Feature 3: Compliance Reporting (5 report types)
- ✅ Feature 4: Automated Policy Validation
- ✅ Feature 5: Scheduled Policy Reviews

### Roadmap
- Phase 6: Advanced RBAC (ABAC, context-aware policies)
- Phase 7: Multi-tenant policy isolation
- Phase 8: Policy simulation and what-if analysis
