# GAP-004: API Reference - PolicyManager & Integration

**Version:** 1.0  
**Datum:** 5. Februar 2026

---

## 📚 Inhaltsverzeichnis

1. [HTTP REST API Endpoints](#http-rest-api-endpoints)
2. [PolicyManager C++ API](#policymanager-c-api)
3. [PolicyCoordinator C++ API](#policycoordinator-c-api)
4. [Configuration Reference](#configuration-reference)
5. [Error Codes](#error-codes)
6. [Performance Guidelines](#performance-guidelines)

---

## 🌐 HTTP REST API Endpoints

### Base URL
```
http://localhost:8080/policies
```

### Authentication
All endpoints require authentication via `Authorization` header:
```
Authorization: Bearer <token>
```

### Endpoints Overview

| Endpoint | Method | Role Required | Description |
|----------|--------|---------------|-------------|
| `/rules` | GET | operator | List all policy rules |
| `/rules/:id` | GET | operator | Get specific rule by ID |
| `/rules` | POST | admin | Create new rule |
| `/rules/:id` | PUT | admin | Update existing rule |
| `/rules/:id` | DELETE | admin | Delete rule |
| `/evaluate` | POST | operator | Evaluate policy for resource/action/roles |
| `/stats` | GET | operator | Get policy statistics |

---

### GET /rules

**Description:** Retrieve list of all policy rules

**Request:**
```http
GET /policies/rules HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
Content-Type: application/json
```

**Response (200 OK):**
```json
{
  "rules": [
    {
      "id": "sensitive_data_policy",
      "name": "Sensitive Data Protection",
      "description": "Protect sensitive user data",
      "resources": ["data/sensitive/*", "data/personal/*"],
      "actions": ["*"],
      "required_roles": ["operator", "admin"],
      "require_encryption": true,
      "require_signature": true,
      "allow_export": false,
      "allow_cache": false,
      "retention_days": 90,
      "redaction_level": "strict",
      "audit_access": true,
      "audit_changes": true,
      "enabled": true,
      "priority": 100,
      "created_at": 1707091200,
      "updated_at": 1707091200
    }
  ],
  "count": 1
}
```

**Error Responses:**
- `401 Unauthorized` - Missing or invalid authentication
- `500 Internal Server Error` - Server error

---

### GET /rules/:id

**Description:** Retrieve specific policy rule by ID

**Request:**
```http
GET /policies/rules/sensitive_data_policy HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
```

**Response (200 OK):**
```json
{
  "id": "sensitive_data_policy",
  "name": "Sensitive Data Protection",
  "resources": ["data/sensitive/*"],
  "actions": ["*"],
  "required_roles": ["operator"],
  "require_encryption": true,
  "enabled": true,
  "priority": 100
}
```

**Error Responses:**
- `404 Not Found` - Rule does not exist
- `401 Unauthorized` - Missing or invalid authentication

---

### POST /rules

**Description:** Create new policy rule

**Required Role:** admin

**Request:**
```http
POST /policies/rules HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
Content-Type: application/json

{
  "id": "new_policy",
  "name": "New Policy Rule",
  "description": "Policy for new resources",
  "resources": ["data/new/*"],
  "actions": ["read", "write"],
  "required_roles": ["operator"],
  "require_encryption": true,
  "allow_export": false,
  "retention_days": 180,
  "enabled": true,
  "priority": 80
}
```

**Response (201 Created):**
```json
{
  "rule": {
    "id": "new_policy",
    "name": "New Policy Rule",
    "created_at": 1707091200,
    "updated_at": 1707091200
  },
  "message": "Rule created successfully"
}
```

**Error Responses:**
- `400 Bad Request` - Invalid JSON or missing required fields
- `409 Conflict` - Rule with this ID already exists
- `401 Unauthorized` - Not authorized (requires admin role)

---

### PUT /rules/:id

**Description:** Update existing policy rule

**Required Role:** admin

**Request:**
```http
PUT /policies/rules/new_policy HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
Content-Type: application/json

{
  "name": "Updated Policy Rule",
  "resources": ["data/new/*", "data/updated/*"],
  "actions": ["read"],
  "required_roles": ["operator", "admin"],
  "enabled": true
}
```

**Response (200 OK):**
```json
{
  "rule": {
    "id": "new_policy",
    "name": "Updated Policy Rule",
    "updated_at": 1707091300
  },
  "message": "Rule updated successfully"
}
```

**Error Responses:**
- `404 Not Found` - Rule does not exist
- `400 Bad Request` - Invalid JSON
- `401 Unauthorized` - Not authorized (requires admin role)

---

### DELETE /rules/:id

**Description:** Delete policy rule

**Required Role:** admin

**Request:**
```http
DELETE /policies/rules/new_policy HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
```

**Response (200 OK):**
```json
{
  "message": "Rule deleted successfully",
  "rule_id": "new_policy"
}
```

**Error Responses:**
- `404 Not Found` - Rule does not exist
- `401 Unauthorized` - Not authorized (requires admin role)

---

### POST /evaluate

**Description:** Evaluate policy for given resource, action, and user roles

**Request:**
```http
POST /policies/evaluate HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
Content-Type: application/json

{
  "resource": "data/sensitive/users",
  "action": "read",
  "user_roles": ["operator", "analyst"]
}
```

**Response (200 OK):**
```json
{
  "resource": "data/sensitive/users",
  "action": "read",
  "user_roles": ["operator", "analyst"],
  "decision": {
    "allowed": true,
    "require_encryption": true,
    "require_signature": true,
    "allow_export": false,
    "allow_cache": false,
    "retention_days": 90,
    "redaction_level": "strict",
    "audit_access": true,
    "audit_changes": true,
    "applied_rules": ["sensitive_data_policy"]
  }
}
```

**Error Responses:**
- `400 Bad Request` - Missing required fields (resource, action)
- `401 Unauthorized` - Missing or invalid authentication

---

### GET /stats

**Description:** Get policy statistics

**Request:**
```http
GET /policies/stats HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
```

**Response (200 OK):**
```json
{
  "total_rules": 5,
  "enabled_rules": 4,
  "disabled_rules": 1,
  "rules_by_priority": {
    "50": 1,
    "100": 2,
    "200": 2
  }
}
```

---

## 🔧 PolicyManager C++ API

### Class: `PolicyManager`

**Header:** `include/governance/policy_manager.h`

#### Constructor
```cpp
PolicyManager();
```

#### Methods

##### addRule
```cpp
void addRule(const PolicyRule& rule);
```
Add a new policy rule. Throws exception if rule validation fails.

**Example:**
```cpp
PolicyManager manager;
PolicyRule rule;
rule.id = "my_policy";
rule.resources = {"data/*"};
rule.actions = {"read"};
manager.addRule(rule);
```

##### removeRule
```cpp
void removeRule(const std::string& rule_id);
```
Remove a policy rule by ID.

##### getRule
```cpp
std::optional<PolicyRule> getRule(const std::string& rule_id) const;
```
Retrieve a specific rule. Returns `std::nullopt` if not found.

##### listRules
```cpp
std::vector<PolicyRule> listRules() const;
```
Get all policy rules.

##### evaluatePolicy
```cpp
PolicyDecision evaluatePolicy(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const;
```
Evaluate policy for given resource, action, and user roles.

**Returns:** `PolicyDecision` with aggregated policy requirements.

**Example:**
```cpp
auto decision = manager.evaluatePolicy(
    "data/sensitive/users",
    "read",
    {"operator"}
);

if (decision.allowed && decision.require_encryption) {
    // Apply encryption before access
}
```

##### loadRules / saveRules
```cpp
bool loadRules(const std::string& filepath);
bool saveRules(const std::string& filepath) const;
```
Load/save rules from/to YAML or JSON file (auto-detected by extension).

**Supported formats:**
- `.yaml` / `.yml` - YAML format
- `.json` - JSON format

##### getStats
```cpp
PolicyStats getStats() const;
```
Get statistics about policies.

---

### Struct: `PolicyRule`

```cpp
struct PolicyRule {
    std::string id;                          // Unique identifier
    std::string name;                        // Display name
    std::string description;                 // Description
    std::vector<std::string> resources;      // Resource patterns (supports *)
    std::vector<std::string> actions;        // Action patterns (supports *)
    std::vector<std::string> required_roles; // Required user roles (OR logic)
    
    // Policy requirements
    bool require_encryption = false;
    bool require_signature = false;
    bool allow_export = true;
    bool allow_cache = true;
    int retention_days = -1;                 // -1 = no limit
    std::string redaction_level = "none";    // none, standard, strict
    bool audit_access = false;
    bool audit_changes = false;
    
    // Metadata
    bool enabled = true;
    int priority = 0;                        // Higher = evaluated first
    time_t created_at = 0;
    time_t updated_at = 0;
    
    // Serialization
    nlohmann::json toJson() const;
    static PolicyRule fromJson(const nlohmann::json& j);
};
```

---

### Struct: `PolicyDecision`

```cpp
struct PolicyDecision {
    bool allowed = true;                     // Access allowed
    bool require_encryption = false;
    bool require_signature = false;
    bool allow_export = true;
    bool allow_cache = true;
    int retention_days = -1;
    std::string redaction_level = "none";
    bool audit_access = false;
    bool audit_changes = false;
    std::vector<std::string> applied_rules;  // IDs of applied rules
};
```

---

## 🔗 PolicyCoordinator C++ API

### Class: `PolicyCoordinator`

**Header:** `include/governance/policy_coordinator.h`

Combines PolicyEngine (VS classification) and PolicyManager (RBAC) for unified governance.

#### Constructor
```cpp
PolicyCoordinator(
    std::shared_ptr<PolicyEngine> policy_engine,
    std::shared_ptr<PolicyManager> policy_manager
);
```
Both parameters can be `nullptr` for independent use.

#### Methods

##### evaluate
```cpp
UnifiedPolicyDecision evaluate(
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& endpoint,
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const;
```

Evaluate both classification (from headers) and RBAC policies, combining with "most restrictive wins" logic.

**Example:**
```cpp
auto coordinator = std::make_unique<PolicyCoordinator>(
    policy_engine,
    policy_manager
);

std::unordered_map<std::string, std::string> headers = {
    {"X-Classification", "geheim"}
};

auto decision = coordinator->evaluate(
    headers,
    "/api/data",
    "data/sensitive/users",
    "read",
    {"operator"}
);

if (decision.rbac_allowed && decision.require_encryption) {
    // Both systems allow with encryption required
}
```

---

### Struct: `UnifiedPolicyDecision`

```cpp
struct UnifiedPolicyDecision {
    // RBAC decision
    bool rbac_allowed = true;
    
    // Combined requirements (most restrictive wins)
    bool require_encryption = false;        // OR logic
    bool require_signature = false;         // OR logic
    bool allow_export = true;               // AND logic
    bool allow_cache = true;                // AND logic
    int retention_days = -1;                // MIN logic
    std::string redaction_level = "none";   // Most strict
    bool audit_access = false;              // OR logic
    bool audit_changes = false;             // OR logic
    
    // Sources
    std::vector<std::string> rbac_applied_rules;
    std::string classification_level;
};
```

---

## ⚙️ Configuration Reference

### YAML Configuration Format

**File:** `config/policies/rules.yaml`

```yaml
rules:
  - id: "sensitive_data_policy"
    name: "Sensitive Data Protection"
    description: "Protect sensitive and personal data"
    resources:
      - "data/sensitive/*"
      - "data/personal/*"
    actions:
      - "*"
    required_roles:
      - "operator"
      - "admin"
    require_encryption: true
    require_signature: true
    allow_export: false
    allow_cache: false
    retention_days: 90
    redaction_level: "strict"
    audit_access: true
    audit_changes: true
    enabled: true
    priority: 100
    
  - id: "public_data_policy"
    name: "Public Data Access"
    description: "Allow public access to public data"
    resources:
      - "data/public/*"
    actions:
      - "read"
    required_roles: []  # No role required
    require_encryption: false
    allow_export: true
    allow_cache: true
    retention_days: 365
    redaction_level: "none"
    audit_access: false
    enabled: true
    priority: 50
```

### JSON Configuration Format

**File:** `config/policies/rules.json`

```json
{
  "rules": [
    {
      "id": "sensitive_data_policy",
      "name": "Sensitive Data Protection",
      "resources": ["data/sensitive/*"],
      "actions": ["*"],
      "required_roles": ["operator"],
      "require_encryption": true,
      "enabled": true,
      "priority": 100
    }
  ]
}
```

---

## ⚠️ Error Codes

| Code | Description | Resolution |
|------|-------------|------------|
| 400 | Bad Request - Invalid JSON or missing required fields | Check request body format and required fields |
| 401 | Unauthorized - Missing or invalid authentication | Provide valid `Authorization` header |
| 403 | Forbidden - Insufficient permissions | Request requires higher privilege role (e.g., admin) |
| 404 | Not Found - Resource does not exist | Verify resource ID is correct |
| 409 | Conflict - Resource already exists | Use different ID or update existing resource |
| 500 | Internal Server Error | Check server logs for details |

---

## 📊 Performance Guidelines

### Evaluation Performance
- **Target:** < 1ms per policy evaluation
- **Tested:** 1000 evaluations in < 1 second
- **Optimization:** Rules are cached in memory, wildcard matching is optimized

### Scaling Guidelines
- **Small deployments:** < 100 rules - no tuning needed
- **Medium deployments:** 100-1000 rules - consider rule priorities
- **Large deployments:** > 1000 rules - use rule grouping by priority

### Best Practices
1. **Use specific resources:** Prefer `data/users/*` over `data/*` for better performance
2. **Set appropriate priorities:** Higher priority rules evaluated first
3. **Disable unused rules:** Better than deleting for audit trail
4. **Batch updates:** Update multiple rules in single transaction if possible
5. **Monitor statistics:** Use `/stats` endpoint to track rule usage

---

## 🔍 Troubleshooting

### Common Issues

**Issue:** Policy evaluation returns unexpected results
- **Check:** Rule priority ordering
- **Check:** Wildcard pattern matching
- **Check:** Multiple conflicting rules (most restrictive wins)

**Issue:** HTTP API returns 401 Unauthorized
- **Check:** Authorization header is present
- **Check:** Token is valid and not expired
- **Check:** User has required role (operator/admin)

**Issue:** Policy changes not taking effect
- **Check:** Rule is enabled (`enabled: true`)
- **Check:** Rule priority is appropriate
- **Check:** PolicyManager has been reloaded after configuration change

---

## 📖 See Also

- [GAP-004 Implementation Overview](GAP_004_SECURITY_GOVERNANCE.md)
- [GAP-004 Roadmap](GAP_004_ROADMAP.md)
- [Configuration Examples](../../config/policies/README.md)
- [Integration Tests](../../tests/test_policy_integration_e2e.cpp)
