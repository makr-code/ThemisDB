# GAP-004: Configuration Guide - PolicyManager Setup

**Version:** 1.0  
**Datum:** 5. Februar 2026

---

## 📚 Inhaltsverzeichnis

1. [Quick Start](#quick-start)
2. [Configuration File Format](#configuration-file-format)
3. [Policy Rule Design](#policy-rule-design)
4. [Integration Examples](#integration-examples)
5. [Common Patterns](#common-patterns)
6. [Migration Guide](#migration-guide)

---

## 🚀 Quick Start

### Step 1: Create Configuration File

Create `config/policies/rules.yaml`:

```yaml
rules:
  - id: "default_policy"
    name: "Default Access Policy"
    description: "Default policy for all resources"
    resources: ["*"]
    actions: ["*"]
    required_roles: []
    allow_export: true
    enabled: true
    priority: 0
```

### Step 2: Load Configuration in Code

```cpp
#include "governance/policy_manager.h"

int main() {
    auto manager = std::make_shared<PolicyManager>();
    
    // Load policies from YAML
    if (!manager->loadRules("config/policies/rules.yaml")) {
        std::cerr << "Failed to load policies" << std::endl;
        return 1;
    }
    
    // Evaluate policy
    auto decision = manager->evaluatePolicy(
        "data/users",
        "read",
        {"operator"}
    );
    
    if (decision.allowed) {
        std::cout << "Access granted" << std::endl;
    }
    
    return 0;
}
```

### Step 3: Start HTTP API (Optional)

```cpp
#include "server/policy_manager_api_handler.h"

auto api_handler = std::make_unique<PolicyManagerApiHandler>(
    manager,
    auth_middleware
);

// Register endpoints with HTTP server
// GET /policies/rules
// POST /policies/rules
// etc.
```

---

## 📝 Configuration File Format

### YAML Format (Recommended)

**File extension:** `.yaml` or `.yml`

**Structure:**
```yaml
rules:
  - id: string              # Required: Unique identifier
    name: string            # Required: Display name
    description: string     # Optional: Description
    resources: [string]     # Required: Resource patterns
    actions: [string]       # Required: Action patterns
    required_roles: [string] # Optional: Required user roles
    require_encryption: bool # Optional: Default false
    require_signature: bool  # Optional: Default false
    allow_export: bool      # Optional: Default true
    allow_cache: bool       # Optional: Default true
    retention_days: int     # Optional: Default -1 (no limit)
    redaction_level: string # Optional: none|standard|strict
    audit_access: bool      # Optional: Default false
    audit_changes: bool     # Optional: Default false
    enabled: bool           # Optional: Default true
    priority: int           # Optional: Default 0
```

### JSON Format (Alternative)

**File extension:** `.json`

```json
{
  "rules": [
    {
      "id": "policy_001",
      "name": "Example Policy",
      "resources": ["data/*"],
      "actions": ["read"],
      "enabled": true
    }
  ]
}
```

---

## 🎨 Policy Rule Design

### Resource Patterns

**Exact Match:**
```yaml
resources:
  - "data/users"           # Matches exactly "data/users"
  - "keys/encryption-key"  # Matches exactly "keys/encryption-key"
```

**Wildcard Match:**
```yaml
resources:
  - "data/*"               # Matches data/users, data/logs, data/anything
  - "data/sensitive/*"     # Matches data/sensitive/users, data/sensitive/pii
  - "*"                    # Matches everything
```

**Multiple Patterns:**
```yaml
resources:
  - "data/sensitive/*"
  - "data/personal/*"
  - "data/confidential/*"  # Applies to any of these patterns
```

### Action Patterns

**Specific Actions:**
```yaml
actions:
  - "read"                 # Only read
  - "write"                # Only write
  - "delete"               # Only delete
```

**Multiple Actions:**
```yaml
actions:
  - "read"
  - "write"                # Applies to both read and write
```

**Wildcard:**
```yaml
actions:
  - "*"                    # Applies to all actions
```

### Role Requirements

**No Role Required (Public Access):**
```yaml
required_roles: []         # Anyone can access
```

**Single Role:**
```yaml
required_roles:
  - "operator"             # Must have operator role
```

**Multiple Roles (OR Logic):**
```yaml
required_roles:
  - "operator"
  - "admin"                # Must have operator OR admin role
```

### Priority System

**How It Works:**
- Higher priority rules are evaluated first
- If multiple rules match, all are applied with "most restrictive wins" logic

**Example:**
```yaml
rules:
  - id: "high_priority"
    priority: 200          # Evaluated first
    resources: ["data/sensitive/*"]
    
  - id: "medium_priority"
    priority: 100          # Evaluated second
    resources: ["data/*"]
    
  - id: "low_priority"
    priority: 50           # Evaluated last
    resources: ["*"]
```

---

## 🔗 Integration Examples

### Example 1: Basic Data Protection

```yaml
rules:
  # Protect sensitive data
  - id: "sensitive_data"
    name: "Sensitive Data Protection"
    resources:
      - "data/sensitive/*"
      - "data/personal/*"
    actions: ["*"]
    required_roles: ["operator", "admin"]
    require_encryption: true
    allow_export: false
    retention_days: 90
    audit_access: true
    enabled: true
    priority: 100
    
  # Allow public data access
  - id: "public_data"
    name: "Public Data Access"
    resources: ["data/public/*"]
    actions: ["read"]
    required_roles: []
    allow_export: true
    allow_cache: true
    enabled: true
    priority: 50
```

### Example 2: Role-Based Access Control

```yaml
rules:
  # Admin-only operations
  - id: "admin_operations"
    name: "Admin Operations"
    resources:
      - "admin/*"
      - "config/*"
      - "system/*"
    actions: ["*"]
    required_roles: ["admin"]
    require_encryption: true
    require_signature: true
    audit_changes: true
    enabled: true
    priority: 200
    
  # Operator read access
  - id: "operator_read"
    name: "Operator Read Access"
    resources: ["data/*"]
    actions: ["read"]
    required_roles: ["operator"]
    audit_access: true
    enabled: true
    priority: 100
    
  # Analyst limited access
  - id: "analyst_access"
    name: "Analyst Access"
    resources: ["data/analytics/*"]
    actions: ["read"]
    required_roles: ["analyst"]
    redaction_level: "standard"
    enabled: true
    priority: 80
```

### Example 3: Compliance Requirements

```yaml
rules:
  # GDPR compliance for personal data
  - id: "gdpr_personal_data"
    name: "GDPR Personal Data"
    description: "GDPR compliance for personal identifiable information"
    resources:
      - "data/personal/*"
      - "data/pii/*"
    actions: ["*"]
    required_roles: ["data_protection_officer", "admin"]
    require_encryption: true
    require_signature: true
    allow_export: false
    retention_days: 90
    redaction_level: "strict"
    audit_access: true
    audit_changes: true
    enabled: true
    priority: 150
    
  # Financial data retention
  - id: "financial_retention"
    name: "Financial Data Retention"
    description: "7-year retention for financial records"
    resources: ["data/financial/*"]
    actions: ["*"]
    required_roles: ["accountant", "admin"]
    require_encryption: true
    retention_days: 2555  # 7 years
    audit_changes: true
    enabled: true
    priority: 140
```

### Example 4: Development vs Production

**Development (dev_policies.yaml):**
```yaml
rules:
  - id: "dev_access"
    name: "Development Access"
    resources: ["*"]
    actions: ["*"]
    required_roles: ["developer"]
    allow_export: true
    allow_cache: true
    audit_access: false  # Less strict in dev
    enabled: true
```

**Production (prod_policies.yaml):**
```yaml
rules:
  - id: "prod_access"
    name: "Production Access"
    resources: ["*"]
    actions: ["read"]
    required_roles: ["operator"]
    require_encryption: true
    allow_export: false
    audit_access: true   # Strict in production
    audit_changes: true
    enabled: true
```

---

## 🎯 Common Patterns

### Pattern 1: Layered Security

```yaml
rules:
  # Layer 1: Highest security
  - id: "layer1_critical"
    priority: 200
    resources: ["data/critical/*"]
    required_roles: ["admin"]
    require_encryption: true
    require_signature: true
    allow_export: false
    
  # Layer 2: High security
  - id: "layer2_sensitive"
    priority: 150
    resources: ["data/sensitive/*"]
    required_roles: ["operator", "admin"]
    require_encryption: true
    allow_export: false
    
  # Layer 3: Normal security
  - id: "layer3_normal"
    priority: 100
    resources: ["data/normal/*"]
    required_roles: ["user"]
    allow_cache: true
    
  # Layer 4: Public
  - id: "layer4_public"
    priority: 50
    resources: ["data/public/*"]
    required_roles: []
    allow_export: true
```

### Pattern 2: Time-Based Access

```yaml
rules:
  # Short retention for temporary data
  - id: "temp_data"
    resources: ["data/temp/*"]
    retention_days: 7
    allow_cache: false
    
  # Medium retention for operational data
  - id: "operational_data"
    resources: ["data/ops/*"]
    retention_days: 90
    
  # Long retention for archival data
  - id: "archive_data"
    resources: ["data/archive/*"]
    retention_days: 3650  # 10 years
    require_encryption: true
```

### Pattern 3: Read/Write Separation

```yaml
rules:
  # Write operations - strict
  - id: "write_operations"
    resources: ["data/*"]
    actions: ["write", "update", "delete"]
    required_roles: ["admin"]
    require_signature: true
    audit_changes: true
    priority: 100
    
  # Read operations - lenient
  - id: "read_operations"
    resources: ["data/*"]
    actions: ["read"]
    required_roles: ["operator", "analyst"]
    audit_access: false
    allow_cache: true
    priority: 50
```

---

## 🔄 Migration Guide

### From PolicyEngine to PolicyCoordinator

**Before (PolicyEngine only):**
```cpp
auto policy_engine = std::make_shared<PolicyEngine>();
policy_engine->loadFromYAML("config/governance.yaml");

auto decision = policy_engine->evaluate(headers, endpoint);
```

**After (PolicyCoordinator with both systems):**
```cpp
// Keep existing PolicyEngine
auto policy_engine = std::make_shared<PolicyEngine>();
policy_engine->loadFromYAML("config/governance.yaml");

// Add PolicyManager
auto policy_manager = std::make_shared<PolicyManager>();
policy_manager->loadRules("config/policies/rules.yaml");

// Use PolicyCoordinator for unified evaluation
auto coordinator = std::make_unique<PolicyCoordinator>(
    policy_engine,
    policy_manager
);

auto decision = coordinator->evaluate(
    headers,
    endpoint,
    resource,
    action,
    user_roles
);
```

### From JSON to YAML

**Step 1: Save existing rules to JSON**
```cpp
policy_manager->saveRules("backup_rules.json");
```

**Step 2: Convert to YAML manually or use converter**
```yaml
# Converted from JSON
rules:
  - id: "policy_001"
    name: "Example"
    # ... rest of fields
```

**Step 3: Load YAML**
```cpp
policy_manager->loadRules("new_rules.yaml");
```

---

## ✅ Validation Checklist

Before deploying policies:

- [ ] All rules have unique IDs
- [ ] Resource patterns are specific enough
- [ ] Required roles are defined correctly
- [ ] Priorities are set appropriately
- [ ] Retention periods comply with regulations
- [ ] Encryption requirements are set for sensitive data
- [ ] Audit flags are enabled where needed
- [ ] Test in development environment first
- [ ] Document policy decisions and rationale
- [ ] Set up monitoring for policy violations

---

## 🔧 Troubleshooting

### Issue: Rules not applying

**Check:**
1. Rule is enabled (`enabled: true`)
2. Resource pattern matches the resource being accessed
3. Action pattern matches the action being performed
4. User has required role (if specified)

**Debug:**
```cpp
auto stats = manager->getStats();
std::cout << "Total rules: " << stats.total_rules << std::endl;
std::cout << "Enabled rules: " << stats.enabled_rules << std::endl;

auto decision = manager->evaluatePolicy(resource, action, roles);
std::cout << "Applied rules: " << decision.applied_rules.size() << std::endl;
for (const auto& rule_id : decision.applied_rules) {
    std::cout << "  - " << rule_id << std::endl;
}
```

### Issue: Conflicting rules

**Understand:** When multiple rules match, "most restrictive wins"
- `require_encryption`: true if ANY rule requires (OR logic)
- `allow_export`: false if ANY rule denies (AND logic)
- `retention_days`: shortest period (MIN logic)

**Solution:** Use priorities to control evaluation order and adjust rule specificity.

---

## 📖 See Also

- [GAP-004 API Reference](GAP_004_API_REFERENCE.md)
- [GAP-004 Implementation Overview](GAP_004_SECURITY_GOVERNANCE.md)
- [Example Configurations](../../config/policies/example_rules.yaml)
