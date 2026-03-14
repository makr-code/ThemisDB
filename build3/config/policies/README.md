# PolicyManager Configuration Files

This directory contains YAML configuration files for ThemisDB PolicyManager RBAC rules.

## Files

- `example_rules.yaml` - Example policy rules demonstrating various access control scenarios

## YAML Schema

```yaml
rules:
  - id: "unique_rule_id"              # Required: Unique identifier
    name: "Rule Name"                  # Required: Human-readable name
    description: "Description"         # Optional: Rule description
    classification_level: "vs-nfd"     # Optional: offen, vs-nfd, geheim, streng-geheim
    enabled: true                      # Optional: Whether rule is active (default: true)
    
    # Conditions
    resources:                         # Optional: Resource patterns (default: [])
      - "data/*"                       # Supports wildcards: data/*, keys/*, *
    actions:                           # Optional: Action patterns (default: [])
      - "read"                         # Supported: read, write, delete, *, etc.
      - "write"
    required_roles:                    # Optional: Required user roles (default: [])
      - "operator"                     # User must have at least one of these roles
      - "admin"
    
    # Effects
    require_encryption: false          # Optional: Require data encryption (default: false)
    require_signature: false           # Optional: Require digital signature (default: false)
    allow_export: true                 # Optional: Allow data export (default: true)
    allow_cache: true                  # Optional: Allow caching (default: true)
    retention_days: 365                # Optional: Data retention period (default: 365)
    redaction_level: "standard"        # Optional: none, standard, strict (default: standard)
    
    # Audit
    audit_access: false                # Optional: Audit access attempts (default: false)
    audit_changes: false               # Optional: Audit data changes (default: false)
    
    # Metadata
    priority: 0                        # Optional: Rule priority, higher = more important (default: 0)
    created_by: "system"               # Optional: Creator identifier
    created_at: 1707134400             # Optional: Unix timestamp
    updated_at: 1707134400             # Optional: Unix timestamp
```

## Usage

Load rules from YAML:

```cpp
#include "governance/policy_manager.h"

PolicyManager manager;
if (manager.loadRules("config/policies/example_rules.yaml")) {
    // Rules loaded successfully
}

// Evaluate policy
auto decision = manager.evaluatePolicy(
    "data/sensitive/users",
    "read",
    {"operator"}
);
```

## Rule Evaluation

When multiple rules apply to a resource/action:

1. **Priority**: Rules are sorted by priority (highest first)
2. **OR logic** for requirements: If any rule requires encryption, it's required
3. **AND logic** for permissions: If any rule denies export, export is denied
4. **MIN logic** for retention: Shortest retention period is used
5. **Most restrictive** redaction level is used

## Examples

See `example_rules.yaml` for common scenarios:
- Sensitive data protection
- Key management access control
- Public data access
- Role-based access (analyst, admin)
- Wildcard resource/action matching
