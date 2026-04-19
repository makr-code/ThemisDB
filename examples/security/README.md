> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Security Examples

This directory contains example code demonstrating ThemisDB's security features.

## Access Control Example

**File**: `access_control_example.cpp`

Demonstrates the complete Access Control Framework including:
- Initialization and configuration
- Authentication setup with API tokens
- Role management (assign, revoke, query)
- Access control checks
- Custom authorization logic
- Metrics tracking
- Configuration management

### Building

```bash
# Add to your CMakeLists.txt or build manually
g++ -std=c++20 \
    -I../../include \
    -I../../vcpkg/installed/x64-linux/include \
    access_control_example.cpp \
    ../../src/security/access_control_manager.cpp \
    ../../src/security/rbac.cpp \
    ../../src/server/auth_middleware.cpp \
    -o access_control_example \
    -lssl -lcrypto
```

### Running

```bash
# Ensure config files exist
mkdir -p ../../config
cp ../../config/rbac_roles.json ../../config/
cp ../../config/user_roles.json ../../config/

# Run the example
./access_control_example
```

### Expected Output

```
Access control initialized successfully

=== Role Management ===
Admin roles: admin 
Admin permissions (1 total):
  - *:*

=== Access Control Checks ===

1. Admin writing data:
   Decision: ALLOW
   Reason: Permission granted via RBAC

2. User writing data:
   Decision: DENY
   Reason: User does not have required permission for data:write

3. User reading data:
   Decision: ALLOW
   Reason: Permission granted via RBAC

4. Admin rotating keys:
   Decision: ALLOW
   Reason: Permission granted via RBAC

5. Anonymous reading public data:
   Decision: ALLOW
   Reason: Public data is accessible to all

=== Metrics ===
Authentication success: 4
Authentication failure: 0
Authorization success: 5
Access denied: 1

=== Configuration Management ===
Configuration saved successfully

Example completed successfully!
```

## Key Concepts Demonstrated

### 1. Configuration-Based Security

The example shows how to configure access control using JSON files for:
- Role definitions with permissions
- User-to-role mappings
- System-wide security policies

### 2. Multiple Authentication Methods

While this example uses API tokens for simplicity, the framework supports:
- JWT tokens with claims validation
- Kerberos/GSSAPI authentication
- USB admin authentication
- Custom authentication providers

### 3. Fine-Grained Authorization

Access control decisions are based on:
- Resource identifiers (what is being accessed)
- Action identifiers (what operation is requested)
- User roles and permissions
- Custom authorization logic

### 4. Audit Trail

All access decisions are automatically logged for:
- Security incident investigation
- Compliance reporting
- Access pattern analysis
- Threat detection

### 5. Operational Metrics

The framework tracks key metrics:
- Authentication success/failure rates
- Authorization decisions
- Access denial patterns
- System health indicators

## Production Considerations

For production deployments, consider:

1. **Use JWT instead of static tokens**
   ```cpp
   AuthMiddleware::JWTConfig jwt_config;
   jwt_config.jwks_url = "https://auth.example.com/.well-known/jwks.json";
   jwt_config.expected_issuer = "https://auth.example.com";
   jwt_config.expected_audience = "themisdb";
   auth->enableJWT(jwt_config);
   ```

2. **Enable comprehensive audit logging**
   ```cpp
   config.enable_audit_logging = true;
   // Configure audit log destination and encryption
   ```

3. **Implement rate limiting**
   - Add rate limiting to prevent brute force attacks
   - Use the metrics to detect anomalous patterns

4. **Regular security reviews**
   - Review user-role assignments periodically
   - Audit access logs for suspicious activity
   - Update security policies as needed

5. **Fail-safe defaults**
   ```cpp
   config.fail_closed = true;  // Always deny on errors
   ```

## See Also

- [Access Control Framework Documentation](../../docs/security/access_control_framework.md)
- [RBAC Guide](../../docs/security/rbac.md)
- [Authentication Guide](../../docs/security/authentication.md)
- [Security Best Practices](../../docs/security/security_hardening.md)
