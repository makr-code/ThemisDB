# Access Control Framework

## Overview

ThemisDB's Access Control Framework provides a comprehensive security layer that integrates:
- **Role-Based Access Control (RBAC)** for permission management
- **Authentication** via JWT, API tokens, Kerberos, and USB admin auth
- **Authorization** with fine-grained resource and action controls
- **Audit Logging** for security event tracking

## Architecture

### Components

1. **AccessControlManager** - Central coordinator for authentication and authorization
2. **RBAC** - Role and permission management system
3. **AuthMiddleware** - Token validation and authentication
4. **UserRoleStore** - User-to-role mappings
5. **SecurityContext** - Per-request security information

### Flow

```
Request → AuthMiddleware (authenticate) → AccessControlManager → RBAC → Decision
```

## Quick Start

### 1. Configuration

Create RBAC roles configuration (`config/rbac_roles.json`):

```json
{
  "roles": [
    {
      "name": "admin",
      "description": "Full access administrator",
      "permissions": [
        {"resource": "*", "action": "*"}
      ],
      "inherits": []
    },
    {
      "name": "developer",
      "description": "Developer with schema access",
      "permissions": [
        {"resource": "schema", "action": "read"},
        {"resource": "schema", "action": "write"},
        {"resource": "data", "action": "read"}
      ],
      "inherits": []
    }
  ]
}
```

Create user-role mappings (`config/user_roles.json`):

```json
{
  "users": [
    {
      "user_id": "alice@example.com",
      "roles": ["admin"],
      "attributes": {"department": "IT"}
    },
    {
      "user_id": "bob@example.com",
      "roles": ["developer"],
      "attributes": {"department": "Engineering"}
    }
  ]
}
```

### 2. Initialization

```cpp
#include "security/access_control_manager.h"
#include "server/auth_middleware.h"

// Configure access control
themis::security::AccessControlConfig config;
config.rbac_config_path = "config/rbac_roles.json";
config.user_role_store_path = "config/user_roles.json";
config.enable_audit_logging = true;
config.fail_closed = true;  // Deny access on errors

// Create manager
auto acm = std::make_shared<themis::security::AccessControlManager>(config);

// Set up auth middleware
auto auth = std::make_shared<themis::AuthMiddleware>();
// Configure JWT or tokens...
acm->setAuthMiddleware(auth);

// Initialize
if (!acm->initialize()) {
    // Handle initialization failure
}
```

### 3. Usage

#### Check Access (Combined Auth + Authz)

```cpp
// Check if bearer token has permission
auto decision = acm->checkAccess(
    bearer_token,      // Authentication token
    "data",           // Resource
    "write",          // Action
    client_ip         // Source IP (optional)
);

if (decision.granted) {
    // Proceed with operation
    std::cout << "Access granted: " << decision.reason << std::endl;
} else {
    // Deny operation
    std::cout << "Access denied: " << decision.reason << std::endl;
}
```

#### Separate Authentication and Authorization

```cpp
// Step 1: Authenticate
auto context = acm->authenticate(bearer_token, client_ip);
if (!context) {
    // Authentication failed
    return;
}

// Step 2: Authorize
auto decision = acm->authorize(*context, "keys", "rotate");
if (decision.granted) {
    // User has permission to rotate keys
}
```

## Permission Model

### Resources

Resources represent system entities:
- `data` - Data operations (tables, documents)
- `schema` - Schema management
- `keys` - Encryption key management
- `config` - System configuration
- `audit` - Audit log access
- `metrics` - Metrics and monitoring
- `health` - Health check endpoints
- `*` - Wildcard (all resources)

### Actions

Actions represent operations:
- `read` - Read/view access
- `write` - Create/update access
- `delete` - Delete access
- `execute` - Execute operations (queries, functions)
- `rotate` - Rotate keys
- `*` - Wildcard (all actions)

### Permission Syntax

Permissions are expressed as `resource:action` pairs:
- `data:read` - Read data
- `data:write` - Write data
- `keys:rotate` - Rotate encryption keys
- `*:*` - Full access to everything

## Built-in Roles

### admin
- **Permissions**: Full access (`*:*`)
- **Use Case**: System administrators

### operator
- **Permissions**: Data operations, key management, audit read
- **Inherits**: analyst
- **Use Case**: Operations team

### analyst
- **Permissions**: Read-only data and audit access
- **Inherits**: readonly
- **Use Case**: Data analysts

### developer
- **Permissions**: Schema management, query execution
- **Use Case**: Application developers

### readonly
- **Permissions**: Metrics and health checks only
- **Use Case**: Monitoring systems, external viewers

## Role Inheritance

Roles can inherit permissions from other roles:

```json
{
  "name": "operator",
  "permissions": [
    {"resource": "keys", "action": "rotate"}
  ],
  "inherits": ["analyst"]
}
```

In this example, `operator` gets:
- Direct: `keys:rotate`
- Inherited from `analyst`: `data:read`, `audit:read`, `metrics:read`
- Inherited from `readonly` (via analyst): `health:read`

## Custom Authorization

Add custom authorization logic:

```cpp
config.custom_authorizer = [](
    const themis::security::SecurityContext& ctx,
    const std::string& resource,
    const std::string& action
) -> themis::security::AccessDecision {
    // Allow operations team access during business hours
    if (ctx.hasGroup("operations")) {
        auto hour = getCurrentHour();
        if (hour >= 9 && hour <= 17) {
            return themis::security::AccessDecision::Allow(
                "Operations team access during business hours"
            );
        }
    }
    
    // Fall through to RBAC
    return themis::security::AccessDecision::Deny("");
};
```

## Security Context

The `SecurityContext` contains per-request security information:

```cpp
struct SecurityContext {
    std::string user_id;                // e.g., "alice@example.com"
    std::vector<std::string> roles;     // e.g., ["admin", "developer"]
    std::vector<std::string> groups;    // e.g., ["engineering", "ops"]
    std::string session_id;             // Session identifier
    std::string source_ip;              // Request source IP
    std::unordered_map<std::string, std::string> attributes;
    
    bool hasRole(const std::string& role) const;
    bool hasGroup(const std::string& group) const;
};
```

## Audit Logging

All access decisions are automatically logged when `enable_audit_logging = true`:

```json
{
  "event_type": "access_control",
  "timestamp": 1674392400000,
  "user_id": "alice@example.com",
  "roles": ["admin"],
  "source_ip": "192.168.1.100",
  "resource": "keys",
  "action": "rotate",
  "decision": "allow",
  "reason": "Permission granted via RBAC",
  "applied_permissions": ["*:*"]
}
```

## Management Operations

### Assign Role to User

```cpp
acm->assignRole("charlie@example.com", "developer");
```

### Revoke Role from User

```cpp
acm->revokeRole("charlie@example.com", "developer");
```

### Get User Roles

```cpp
auto roles = acm->getUserRoles("charlie@example.com");
for (const auto& role : roles) {
    std::cout << "Role: " << role << std::endl;
}
```

### Get User Permissions

```cpp
auto perms = acm->getUserPermissions("charlie@example.com");
for (const auto& perm : perms) {
    std::cout << "Permission: " << perm.toString() << std::endl;
}
```

### Reload Configuration

```cpp
// After modifying config files
if (acm->reloadConfiguration()) {
    std::cout << "Configuration reloaded" << std::endl;
}
```

## Metrics

Monitor access control metrics:

```cpp
const auto& metrics = acm->getMetrics();
std::cout << "Auth success: " << metrics.authentication_success << std::endl;
std::cout << "Auth failure: " << metrics.authentication_failure << std::endl;
std::cout << "Authz success: " << metrics.authorization_success << std::endl;
std::cout << "Access denied: " << metrics.access_denied << std::endl;
```

## Best Practices

### Security

1. **Fail Closed**: Always set `fail_closed = true` in production
2. **Least Privilege**: Assign minimal required roles to users
3. **Audit Everything**: Enable audit logging for compliance
4. **Regular Reviews**: Periodically review user-role assignments
5. **Strong Authentication**: Use JWT with proper validation, not static tokens

### Performance

1. **Cache Decisions**: Results are not cached by default - implement caching if needed
2. **Batch Operations**: Use `getUserPermissions()` once instead of repeated `authorize()` calls
3. **Efficient Configs**: Keep role hierarchies simple (max 3-4 levels)

### Operations

1. **Backup Configs**: Version control RBAC and user-role files
2. **Test Changes**: Test role changes in non-production first
3. **Monitor Metrics**: Set up alerts on high denial rates
4. **Gradual Rollout**: Use custom authorizer for gradual permission changes

## Integration Examples

### HTTP Server

```cpp
// In HTTP request handler
std::string auth_header = request.getHeader("Authorization");
auto token = themis::AuthMiddleware::extractBearerToken(auth_header);

if (!token) {
    return response.status(401).send("Missing authorization");
}

auto decision = acm->checkAccess(*token, "data", "read", request.getIP());
if (!decision.granted) {
    return response.status(403).send("Access denied: " + decision.reason);
}

// Proceed with operation
```

### gRPC Service

```cpp
// In gRPC interceptor
grpc::Status CheckAuthorization(
    grpc::ServerContext* context,
    const std::string& resource,
    const std::string& action
) {
    auto metadata = context->client_metadata();
    auto auth_it = metadata.find("authorization");
    
    if (auth_it == metadata.end()) {
        return grpc::Status(grpc::UNAUTHENTICATED, "No auth token");
    }
    
    std::string token = std::string(auth_it->second.data(), auth_it->second.size());
    auto bearer_token = themis::AuthMiddleware::extractBearerToken(token);
    
    if (!bearer_token) {
        return grpc::Status(grpc::UNAUTHENTICATED, "Invalid auth token");
    }
    
    auto decision = acm->checkAccess(*bearer_token, resource, action);
    if (!decision.granted) {
        return grpc::Status(grpc::PERMISSION_DENIED, decision.reason);
    }
    
    return grpc::Status::OK;
}
```

## Troubleshooting

### Access Denied Unexpectedly

1. Check user's assigned roles: `acm->getUserRoles(user_id)`
2. Check role's permissions: `acm->getUserPermissions(user_id)`
3. Verify RBAC config is loaded correctly
4. Check audit logs for decision details
5. Enable DEBUG logging to see permission checks

### Configuration Not Loading

1. Verify file paths are correct
2. Check JSON syntax with a validator
3. Ensure files are readable by the process
4. Check logs for specific error messages

### Performance Issues

1. Reduce role inheritance depth
2. Implement decision caching layer
3. Use custom authorizer for frequent checks
4. Consider moving static permissions to compile-time

## See Also

- [RBAC Documentation](./rbac.md)
- [Authentication Middleware](./auth_middleware.md)
- [Security Overview](../../SECURITY.md)
- [Audit Logging](../features/features_audit_logging.md)
