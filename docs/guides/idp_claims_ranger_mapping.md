# IdP Claims to Ranger Policy Mapping Guide

**Version:** 1.0  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Overview

This guide explains how to map JWT claims from Identity Providers (IdP) to Apache Ranger policy subjects and ThemisDB authorization scopes. Proper mapping ensures that user authentication through external IdPs (Keycloak, Azure AD, Auth0) translates correctly into access control decisions.

---

## Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                  Claims Mapping Flow                            │
├────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────┐     JWT      ┌─────────────┐     Scopes         │
│  │   IdP    │─────Token────▶│  ThemisDB   │────Validation────┐ │
│  │(Keycloak)│    +Claims   │  JWT Val.   │                  │ │
│  └──────────┘              └─────────────┘                  │ │
│                                   │                          │ │
│                                   │                          │ │
│                          Extract Claims                      │ │
│                          (roles/groups)                      │ │
│                                   │                          │ │
│                                   ▼                          │ │
│                         ┌──────────────────┐                │ │
│                         │ Scope Mapper     │                │ │
│                         │ roles → scopes   │                │ │
│                         └────────┬─────────┘                │ │
│                                  │                          │ │
│                                  │                          │ │
│        ┌─────────────────────────┴────────────┐             │ │
│        │                                      │             │ │
│        ▼                                      ▼             │ │
│  ┌───────────────┐                    ┌────────────────┐   │ │
│  │ Ranger Engine │                    │ Scope Check    │   │ │
│  │ Policy Eval   │◀───Authorized──────│ (policy:read)  │◀──┘ │
│  │               │    User Context    │                │     │
│  │ Subject Match │                    │ Decision:      │     │
│  │ Resource Match│                    │ Allow/Deny     │     │
│  └───────────────┘                    └────────────────┘     │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

## JWT Token Structure

### Standard Claims

JWT tokens from IdPs contain standard and custom claims:

**Standard OIDC Claims:**
- `sub`: Subject identifier (user ID)
- `iss`: Issuer URL
- `aud`: Audience (intended recipient)
- `exp`: Expiration timestamp
- `iat`: Issued at timestamp
- `email`: User email
- `preferred_username`: Username

**Custom Claims for Authorization:**
- `roles`: Array of role names
- `groups`: Array of group names or paths
- `scope`: Space-separated list of OAuth scopes
- Custom namespace claims (e.g., `https://themisdb.example.com/roles`)

### Example JWT Payload

```json
{
  "sub": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "iss": "https://keycloak.example.com/realms/themis",
  "aud": "themis-app",
  "exp": 1708290000,
  "iat": 1708286400,
  "preferred_username": "jdoe",
  "email": "john.doe@example.com",
  "email_verified": true,
  "name": "John Doe",
  "roles": [
    "Engineering-Team",
    "Data-Analysts",
    "All-Employees"
  ],
  "groups": [
    "/Company/Engineering/Engineering-Team",
    "/Company/Analytics/Data-Analysts",
    "/Company/All-Employees"
  ]
}
```

---

## Mapping Strategy

### 1. Claim Selection

Choose which claim to use for authorization. ThemisDB's `scope_claim` configuration determines this:

**Option A: roles claim** (recommended for Keycloak)
```yaml
jwt:
  scope_claim: "roles"
```

**Option B: groups claim** (common for Azure AD)
```yaml
jwt:
  scope_claim: "groups"
```

**Option C: Custom namespace claim** (for Auth0)
```yaml
jwt:
  scope_claim: "https://themisdb.example.com/roles"
```

### 2. Group Name Normalization

Ensure group names are consistent across:
1. **Active Directory** group names (sAMAccountName)
2. **IdP** realm roles/groups
3. **Ranger** policy subjects

**Naming Convention:**
- Use clean names: `Engineering-Team`, not `/Company/Engineering/Engineering-Team`
- Avoid special characters except hyphens and underscores
- Case-sensitive matching (prefer PascalCase or kebab-case)
- No domain suffixes (e.g., `Team` not `Team@example.com`)

**Example Mapping:**

| Active Directory | IdP Realm Role | Ranger Subject | Match |
|------------------|---------------|----------------|-------|
| `Engineering-Team` | `Engineering-Team` | `Engineering-Team` | ✅ Direct |
| `HR-Admins` | `HR-Admins` | `HR-Admins` | ✅ Direct |
| `CN=Finance,OU=Groups,DC=example,DC=com` | `Finance-Team` | `Finance-Team` | ⚠️ Transform DN |

### 3. Claim Transformation

If your IdP returns group paths or DNs, transform them before matching:

**Strip Path Prefix:**
```javascript
// Auth0 Rule Example
function transformGroups(user, context, callback) {
  const groups = context.authorization.groups || [];
  const cleanGroups = groups.map(g => g.split('/').pop());
  context.accessToken['https://themisdb.example.com/roles'] = cleanGroups;
  callback(null, user, context);
}
```

**Strip Domain Suffix:**
```javascript
function removeDomain(groups) {
  return groups.map(g => g.replace(/@.*$/, ''));
}
```

---

## IdP-Specific Configuration

### Keycloak

#### 1. Create Realm Roles

Create roles matching your AD group names:

```bash
# Via Admin Console
Realm Settings → Roles → Add Role
- Role Name: Engineering-Team
- Description: Engineering department

Repeat for:
- HR-Admins
- HR-Readers
- Finance-Admins
- Data-Admins
- All-Employees
```

#### 2. Configure Client Mappers

Add a mapper to include roles in token:

```yaml
# Client Mappers Configuration
Mapper Type: User Realm Role
Mapper Name: realm-roles
Token Claim Name: roles
Claim JSON Type: String (array)
Add to ID token: ON
Add to access token: ON
Add to userinfo: ON
```

#### 3. Assign Roles to Users

```bash
# Via Admin Console
Users → Select User → Role Mappings
Assign Realm Roles: Engineering-Team, Data-Analysts
```

#### 4. ThemisDB Configuration

```cpp
AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs",
    .expected_issuer = "https://keycloak.example.com/realms/themis",
    .expected_audience = "themis-app",
    .scope_claim = "roles"
};
```

Or in YAML:
```yaml
# config/auth.yaml
jwt:
  jwks_url: "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs"
  expected_issuer: "https://keycloak.example.com/realms/themis"
  expected_audience: "themis-app"
  scope_claim: "roles"
```

---

### Azure Active Directory

#### 1. Configure App Registration

1. Azure Portal → Azure Active Directory → App Registrations
2. Create new registration for ThemisDB
3. Configure Redirect URIs

#### 2. Token Configuration

Add optional claims:
```yaml
Token Configuration → Add optional claim
- ID Token: email, preferred_username
- Access Token: email, preferred_username, groups
```

Enable groups claim:
```yaml
Token Configuration → Groups
Select: Security groups
```

#### 3. Map Group IDs to Names

Azure uses GUIDs for group IDs in tokens. Create a mapping table:

```yaml
# group_id_mapping.yaml
group_mappings:
  "12345678-1234-1234-1234-123456789012": "Engineering-Team"
  "23456789-2345-2345-2345-234567890123": "HR-Admins"
  "34567890-3456-3456-3456-345678901234": "Finance-Admins"
```

Or configure Azure to use group names instead:
```yaml
Manifest → groupMembershipClaims: "SecurityGroup"
Manifest → optionalClaims → accessToken:
  - name: groups
    source: null
    essential: false
    additionalProperties: ["sam_account_name"]
```

#### 4. ThemisDB Configuration

```cpp
AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://login.microsoftonline.com/{tenant-id}/discovery/v2.0/keys",
    .expected_issuer = "https://login.microsoftonline.com/{tenant-id}/v2.0",
    .expected_audience = "{application-id}",
    .scope_claim = "groups"
};
```

---

### Auth0

#### 1. Create Auth0 Application

1. Auth0 Dashboard → Applications → Create Application
2. Choose: Regular Web Application
3. Note Client ID and Client Secret

#### 2. Add Roles to Users

```bash
# Via Auth0 Management API or Dashboard
Users → Select User → Roles
Assign Roles: Engineering-Team, Data-Analysts
```

#### 3. Create Rule to Add Roles to Token

```javascript
function addRolesToToken(user, context, callback) {
  const namespace = 'https://themisdb.example.com';
  const assignedRoles = (context.authorization || {}).roles || [];
  
  // Add roles to access token
  context.accessToken[`${namespace}/roles`] = assignedRoles;
  
  // Add roles to ID token
  context.idToken[`${namespace}/roles`] = assignedRoles;
  
  callback(null, user, context);
}
```

#### 4. ThemisDB Configuration

```cpp
AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://{domain}.auth0.com/.well-known/jwks.json",
    .expected_issuer = "https://{domain}.auth0.com/",
    .expected_audience = "{client-id}",
    .scope_claim = "https://themisdb.example.com/roles"
};
```

---

## Ranger Policy Configuration

### Mapping Roles to Policy Subjects

Ranger policies use **subjects** to identify who can perform actions:

```yaml
# config/policies.yaml
- id: allow-engineering-read
  name: Engineering Team read access
  subjects: ["Engineering-Team"]
  actions: ["read", "data:read"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow
```

**Subject Matching:**
- JWT claim value must **exactly match** policy subject
- Matching is **case-sensitive**
- Use wildcards for flexible matching: `subjects: ["*"]`

### Multi-Group Policies

Users with multiple group memberships match any relevant policy:

**User has roles:** `["Engineering-Team", "Data-Analysts"]`

**Policies:**
```yaml
# Policy 1: Matches via Engineering-Team
- id: policy-1
  subjects: ["Engineering-Team"]
  actions: ["read"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow

# Policy 2: Matches via Data-Analysts
- id: policy-2
  subjects: ["Data-Analysts"]
  actions: ["read"]
  resources: ["/entities/postgres_view:analytics.*"]
  effect: allow
```

Result: User can access both engineering tables and analytics views.

### Admin Policies

Grant broad access to admin groups:

```yaml
- id: allow-data-admins-all
  name: Data Admins have full access
  subjects: ["Data-Admins"]
  actions: ["*"]
  resources: ["*"]
  effect: allow
```

---

## ThemisDB Scope Mapping

### Built-in Scopes

ThemisDB has built-in scopes for API endpoints:

| Scope | Permissions | Typical Roles |
|-------|-------------|---------------|
| `admin` | Full access | Data-Admins, System-Admins |
| `policy:read` | View policies | All authenticated users |
| `policy:write` | Modify policies | Policy-Admins |
| `data:read` | Read entities | All-Employees, Analysts |
| `data:write` | Write entities | Engineers, Data-Admins |
| `cdc:read` | Read changefeed | Monitoring, Analysts |
| `cdc:admin` | Configure CDC | Data-Admins |
| `metrics:read` | View metrics | Monitoring, Operators |
| `audit:read` | View audit logs | Auditors, Admins |
| `pii:reveal` | Decrypt PII | HR-Admins, Compliance |
| `pii:erase` | Delete PII | HR-Admins, DPO |

### Mapping Roles to Scopes

Use `auth_scope_mapper` utilities to map roles to scopes consistently:

```cpp
// C++ Example
#include "server/auth_scope_mapper.h"

// Map role to policy scope
std::string scope = themis::server::auth_scope_mapper::mapPolicyRoleToScope("admin");
// Returns: "policy:write"

// Map role to audit scope
std::string audit_scope = themis::server::auth_scope_mapper::mapAuditRoleToScope("operator");
// Returns: "audit:read"
```

### Custom Scope Mapping

Define custom mappings in configuration:

```yaml
# config/scope_mappings.yaml
role_to_scope:
  Engineering-Team:
    - data:read
    - data:write
    - metrics:read
  
  HR-Admins:
    - data:read
    - data:write
    - pii:reveal
    - pii:erase
    - audit:read
  
  Data-Analysts:
    - data:read
    - metrics:read
    - audit:read
  
  All-Employees:
    - data:read
```

---

## Testing Claims Mapping

### 1. Obtain JWT Token

```bash
# Keycloak
curl -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "client_secret=your-secret" \
  -d "username=jdoe" \
  -d "password=password" \
  | jq -r '.access_token' > token.txt

export JWT_TOKEN=$(cat token.txt)
```

### 2. Decode and Inspect Claims

```bash
# Using jwt-cli (install: cargo install jwt-cli)
echo $JWT_TOKEN | jwt decode -

# Or using Python
python3 << 'EOF'
import base64, json, sys
token = sys.stdin.read().strip()
payload = token.split('.')[1]
payload += '=' * (4 - len(payload) % 4)
claims = json.loads(base64.urlsafe_b64decode(payload))
print(json.dumps(claims, indent=2))
EOF
```

Expected output:
```json
{
  "roles": ["Engineering-Team", "Data-Analysts"],
  "preferred_username": "jdoe",
  "email": "john.doe@example.com",
  ...
}
```

### 3. Test API Access

```bash
# Test with role-based access
curl -v -H "Authorization: Bearer $JWT_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects

# Should return 200 OK if user has Engineering-Team role
```

### 4. Test Policy Evaluation

```bash
# Direct policy evaluation
curl -X POST \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "subject": "Engineering-Team",
    "action": "read",
    "resource": "/entities/postgres_table:engineering.projects"
  }' \
  http://localhost:8765/policies/evaluate

# Expected response:
# {"allowed": true, "matched_policies": ["allow-engineering-read"]}
```

---

## Troubleshooting

### Issue: Roles Not in Token

**Symptoms:** Token missing `roles` or `groups` claim

**Solutions:**
- **Keycloak:** Verify client mapper is configured and enabled
- **Azure AD:** Enable groups claim in token configuration
- **Auth0:** Ensure rule is active and user has roles assigned
- Check user actually has roles/groups assigned
- Verify claim name matches `scope_claim` configuration

### Issue: Role Name Mismatch

**Symptoms:** 403 Forbidden despite having correct role

**Solutions:**
- Check exact role name in JWT (case-sensitive)
- Verify policy subject matches JWT claim exactly
- Check for path prefixes: `/Company/Team` vs `Team`
- Check for domain suffixes: `Team@domain.com` vs `Team`
- Review policy evaluation logs

### Issue: Multiple Groups Not Working

**Symptoms:** User with multiple roles can only access some resources

**Solutions:**
- Verify all groups in token are being extracted
- Check if scope_claim is set to correct claim name
- Ensure policies exist for all relevant groups
- Review policy evaluation order (deny-overrides)

### Issue: JWT Validation Fails

**Symptoms:** 401 Unauthorized with valid-looking token

**Solutions:**
- Verify JWKS URL is accessible
- Check issuer matches exactly (including trailing slash)
- Verify audience matches expected value
- Check token expiry (`exp` claim)
- Review clock skew tolerance (default: 5 minutes)

---

## Best Practices

### 1. Consistent Naming

- Use same group names across AD, IdP, and Ranger
- Document naming convention in runbook
- Use automation to sync groups if possible

### 2. Least Privilege

- Map roles to minimal required scopes
- Use specific resource patterns in policies
- Avoid wildcard policies except for admins

### 3. Regular Audits

- Review role assignments quarterly
- Audit policy effectiveness with metrics
- Check for orphaned or unused roles

### 4. Documentation

- Document role-to-scope mappings
- Maintain list of privileged groups
- Keep IdP configuration in version control

### 5. Testing

- Test new roles before production deployment
- Validate claims in staging environment
- Use policy evaluation API for debugging

---

## Example Complete Setup

### 1. AD Groups

```
Engineering-Team
HR-Admins
HR-Readers
Finance-Admins
Data-Admins
All-Employees
```

### 2. Keycloak Realm Roles

Create roles matching AD groups (1:1)

### 3. JWT Configuration

```yaml
jwt:
  jwks_url: "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs"
  expected_issuer: "https://keycloak.example.com/realms/themis"
  expected_audience: "themis-app"
  scope_claim: "roles"
```

### 4. Ranger Policies

```yaml
- id: allow-engineering
  subjects: ["Engineering-Team"]
  actions: ["read", "write"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow

- id: allow-hr-admins
  subjects: ["HR-Admins"]
  actions: ["read", "write", "delete"]
  resources: ["/entities/postgres_table:hr.*"]
  effect: allow

- id: allow-all-read
  subjects: ["All-Employees"]
  actions: ["read"]
  resources: ["/entities/postgres_table:public.*"]
  effect: allow

- id: allow-admins-all
  subjects: ["Data-Admins"]
  actions: ["*"]
  resources: ["*"]
  effect: allow
```

### 5. Test User Setup

**User:** jdoe  
**Keycloak Roles:** Engineering-Team, All-Employees  
**Expected Access:**
- Can read/write engineering.* tables
- Can read public.* tables
- Cannot access hr.* tables

**Validation:**
```bash
# Get token
export TOKEN=$(curl -s -X POST ... | jq -r '.access_token')

# Test allowed
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects
# → 200 OK

# Test denied
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8765/entities/postgres_table:hr.employees
# → 403 Forbidden
```

---

## References

- [AD/LDAP Integration Guide](ad_ldap_integration_guide.md)
- [RBAC Authorization Guide](guides_rbac_authorization.md)
- [API Authentication Documentation](../security/api_authentication_authorization.md)
- [Ranger Documentation](https://ranger.apache.org/)
- [JWT Standard (RFC 7519)](https://tools.ietf.org/html/rfc7519)

---

**End of Guide**
