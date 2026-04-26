# Active Directory/LDAP Integration with Apache Ranger

**Version:** 1.5.0  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Overview

This guide describes how to integrate Active Directory/LDAP directory services with ThemisDB for ownership tracking, lineage, and RBAC-based authorization using Apache Ranger. The integration enables:

- **Directory Graph Ingestion**: Import AD users, groups, and OUs as graph entities
- **Ownership Tracking**: Link data assets to AD groups for governance
- **JWT-based Authentication**: Validate user identity via external IdP (Keycloak/Azure AD/Auth0)
- **Ranger-based Authorization**: Map IdP claims to Ranger policies for fine-grained access control

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    End-to-End Flow                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────┐      ┌──────────┐      ┌──────────────┐          │
│  │   User   │─────▶│   IdP    │─────▶│  JWT Token   │          │
│  │          │ Auth │(Keycloak)│ Issue│  + Claims    │          │
│  └──────────┘      └──────────┘      └──────┬───────┘          │
│                                              │                   │
│                                              ▼                   │
│                                    ┌──────────────────┐          │
│  ┌──────────┐                     │   ThemisDB API   │          │
│  │ Active   │                     │  JWT Validation  │          │
│  │Directory │                     └────────┬─────────┘          │
│  │          │                              │                   │
│  │ Users    │                              │                   │
│  │ Groups   │◀─── Export (LDAP)            │                   │
│  │ OUs      │                              │                   │
│  └────┬─────┘                              │                   │
│       │                                    │                   │
│       │ JSONL                              │                   │
│       ▼                                    ▼                   │
│  ┌──────────────┐           ┌────────────────────────┐         │
│  │  ThemisDB    │           │   Apache Ranger        │         │
│  │  Graph Store │           │   Policy Engine        │         │
│  │              │           │                        │         │
│  │  • AD Nodes  │           │  • Subject matching    │         │
│  │  • Ownership │◀──────────│  • Group-based rules   │         │
│  │  • Lineage   │  Consult  │  • ABAC evaluation     │         │
│  └──────────────┘           └────────────────────────┘         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Prerequisites

### Software Requirements

- **ThemisDB**: Version 1.4.0 or later
- **Python**: 3.8 or later with packages:
  - `ldap3` - LDAP client library
  - `pyyaml` - YAML configuration parser
  - `requests` (optional) - For testing JWT flows
- **Apache Ranger**: Configured and accessible (optional but recommended)
- **Identity Provider**: One of:
  - Keycloak
  - Azure Active Directory
  - Auth0
  - Any OIDC-compliant IdP

### Access Requirements

- **AD/LDAP**: Read-only service account with permissions to query users, groups, and OUs
- **Ranger**: Admin access to create/modify policies
- **IdP**: Configuration access to set up JWT token claims

### Network Requirements

- LDAP/LDAPS connectivity to Active Directory (ports 389/636)
- HTTPS connectivity to IdP for JWKS endpoint
- Network access to Ranger API (if using policy import/export)

---

## Step 1: Export Active Directory Data

### 1.1 Create LDAP Export Configuration

Create `ldap_export_config.yaml`:

```yaml
# LDAP Connection
server: "ldap://dc.example.com"
port: 389
use_ssl: false
bind_dn: "CN=themisdb-reader,OU=Service Accounts,DC=example,DC=com"
bind_password: "${LDAP_BIND_PASSWORD}"
base_dn: "DC=example,DC=com"

# Search Configuration
user_filter: "(objectClass=user)"
group_filter: "(objectClass=group)"
ou_filter: "(objectClass=organizationalUnit)"
page_size: 1000

# Attributes to Export
user_attributes:
  - sAMAccountName
  - objectGUID
  - distinguishedName
  - mail
  - displayName
  - memberOf
  - userPrincipalName
  - cn
  - description

group_attributes:
  - sAMAccountName
  - objectGUID
  - distinguishedName
  - cn
  - description
  - member
  - memberOf

ou_attributes:
  - objectGUID
  - distinguishedName
  - ou
  - description

# Output Configuration
output_file: "ad_export.jsonl"
export_users: true
export_groups: true
export_ous: true
```

### 1.2 Run LDAP Export

```bash
# Set bind password via environment variable
export LDAP_BIND_PASSWORD="your-secure-password"

# Run export
python3 tools/ldap_export.py \
  --config ldap_export_config.yaml \
  --output ad_export.jsonl

# Verify output
head -5 ad_export.jsonl
wc -l ad_export.jsonl
```

### 1.3 Exported Data Schema

The export generates JSONL with three node types and three edge types:

**Node Types:**
- `ad_user`: Active Directory user accounts
- `ad_group`: Security and distribution groups
- `ad_ou`: Organizational Units

**Edge Types:**
- `MEMBER_OF`: User/group membership in groups
- `IN_OU`: Entity containment in OUs
- `CHILD_OF`: OU hierarchy relationships

**Example Node (User):**
```json
{
  "id": "ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "type": "ad_user",
  "attributes": {
    "sAMAccountName": "jdoe",
    "mail": "john.doe@example.com",
    "displayName": "John Doe",
    "distinguishedName": "CN=John Doe,OU=Engineering,DC=example,DC=com",
    "ou_path": ["Engineering", "Employees"]
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000",
    "guid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
  }
}
```

**Example Edge (Group Membership):**
```json
{
  "id": "edge:a1b2c3d4e5f6a7b8",
  "type": "MEMBER_OF",
  "source": "ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "target": "ad_group:c3d4e5f6-a7b8-9012-cdef-123456789012",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000"
  }
}
```

---

## Step 2: Configure Identity Provider (IdP) JWT Claims

### 2.1 JWT Claims Structure

Configure your IdP to include relevant claims in JWT tokens:

**Required Claims:**
- `sub` or `preferred_username`: User identifier
- `email`: User email address
- `roles` or `groups`: Group memberships (for Ranger matching)

**Example JWT Payload (Keycloak):**
```json
{
  "sub": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "preferred_username": "jdoe",
  "email": "john.doe@example.com",
  "roles": [
    "Engineering-Team",
    "Data-Analysts",
    "All-Employees"
  ],
  "groups": [
    "/Engineering/Engineering-Team",
    "/Company/Data-Analysts"
  ],
  "iss": "https://keycloak.example.com/realms/themis",
  "aud": "themis-app",
  "iat": 1708286400,
  "exp": 1708290000
}
```

### 2.2 IdP Configuration by Provider

#### Keycloak

1. **Create Realm Roles/Groups** matching AD group names:
   ```
   Engineering-Team
   Finance-Admins
   HR-Readers
   Data-Admins
   ```

2. **Configure Client Mappers** for the ThemisDB client:
   - Mapper Type: `User Realm Role`
   - Token Claim Name: `roles`
   - Claim JSON Type: `String` (array)

3. **Group Mapper** (optional):
   - Mapper Type: `Group Membership`
   - Token Claim Name: `groups`
   - Full group path: `true`

#### Azure Active Directory

1. **App Registration**:
   - Create app registration for ThemisDB
   - Configure redirect URIs

2. **Token Configuration**:
   - Add optional claims: `email`, `preferred_username`
   - Add groups claim: Select "Security groups" or "Group IDs"

3. **API Permissions**:
   - Microsoft Graph: `User.Read`, `GroupMember.Read.All`

4. **Map group IDs to names** (Azure uses GUIDs):
   - Create mapping table: `group_id -> group_name`
   - Or use group display names in claims

#### Auth0

1. **Create Auth0 Application** for ThemisDB

2. **Add Rule** to include roles in token:
```javascript
function addRolesToToken(user, context, callback) {
  const namespace = 'https://themisdb.example.com';
  const assignedRoles = (context.authorization || {}).roles;
  
  context.accessToken[`${namespace}/roles`] = assignedRoles;
  context.idToken[`${namespace}/roles`] = assignedRoles;
  
  callback(null, user, context);
}
```

3. **Configure Authorization Extension**:
   - Define roles matching AD groups
   - Assign users to roles

### 2.3 ThemisDB JWT Configuration

Configure ThemisDB to validate and extract claims:

```cpp
// C++ API Configuration
AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs",
    .expected_issuer = "https://keycloak.example.com/realms/themis",
    .expected_audience = "themis-app",
    .scope_claim = "roles"  // or "groups" depending on your IdP
};
auth->enableJWT(jwt_config);
```

Or via configuration file (`config/auth.yaml`):

```yaml
jwt:
  jwks_url: "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs"
  expected_issuer: "https://keycloak.example.com/realms/themis"
  expected_audience: "themis-app"
  scope_claim: "roles"
  validate_expiry: true
  clock_skew_seconds: 300
```

---

## Step 3: Map IdP Claims to Ranger Policies

### 3.1 Claims to Subjects Mapping

Ranger policies use **subjects** to identify users/groups. Map IdP claims to Ranger subjects:

**Mapping Strategy:**

| IdP Claim Value | Ranger Subject | Policy Match |
|----------------|----------------|--------------|
| `Engineering-Team` | `Engineering-Team` | Direct match |
| `/Engineering/Engineering-Team` | `Engineering-Team` | Strip path prefix |
| `Engineering-Team@example.com` | `Engineering-Team` | Strip domain suffix |
| User email `jdoe@example.com` | `jdoe@example.com` | User-level match |

**Recommendation:** Use clean group names (e.g., `Engineering-Team`) consistently across:
- Active Directory group names (sAMAccountName)
- IdP realm roles/groups
- Ranger policy subjects

### 3.2 Create Ranger Policies

Create policies in `config/policies.yaml`:

```yaml
# Read access for Engineering Team
- id: allow-engineering-read
  name: Engineering Team can read engineering tables
  subjects: ["Engineering-Team"]
  actions: ["read", "data:read"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow

# Write access for HR Admins
- id: allow-hr-admins-write
  name: HR Admins can write to HR tables
  subjects: ["HR-Admins"]
  actions: ["read", "write", "data:read", "data:write"]
  resources: ["/entities/postgres_table:hr.*"]
  effect: allow

# Admin access for Data Admins
- id: allow-data-admins-all
  name: Data Admins have full access
  subjects: ["Data-Admins"]
  actions: ["*"]
  resources: ["*"]
  effect: allow

# Deny external access to sensitive data
- id: deny-hr-external
  name: Deny external access to HR data
  subjects: ["*"]
  actions: ["read", "write"]
  resources: ["/entities/postgres_table:hr.salaries"]
  effect: deny
  allowed_ip_prefixes: ["10.0.", "192.168."]
```

### 3.3 Import Policies from Ranger Service

If you have an existing Ranger service, import policies:

```bash
# Set Ranger credentials
export THEMIS_RANGER_BASE_URL=https://ranger.example.com
export THEMIS_RANGER_SERVICE=themisdb
export THEMIS_RANGER_BEARER=your-ranger-token
export THEMIS_TOKEN_ADMIN=your-themis-admin-token

# Import policies
curl -X POST \
  -H "Authorization: Bearer $THEMIS_TOKEN_ADMIN" \
  http://localhost:8765/policies/import/ranger
```

### 3.4 Export Policies to Ranger

Export ThemisDB policies to Ranger format:

```bash
# Export current policies
curl -H "Authorization: Bearer $THEMIS_TOKEN_ADMIN" \
  http://localhost:8765/policies/export/ranger > policies_export.json

# Review and upload to Ranger
cat policies_export.json
```

---

## Step 4: Create Ownership Linkages

### 4.1 Create Ownership Mapping Configuration

Create `ownership_mapping.yaml`:

```yaml
mappings:
  # HR Department
  - entity_pattern: "postgres_table:hr.*"
    owner_group: "HR-Admins"
    visible_to_groups:
      - "HR-Readers"
      - "HR-Analysts"
      - "Data-Admins"
    entity_type: "postgres_table"
    attributes:
      department: "hr"
      sensitivity: "high"

  # Finance Department
  - entity_pattern: "postgres_table:finance.*"
    owner_group: "Finance-Admins"
    visible_to_groups:
      - "Finance-Team"
      - "Data-Admins"
    entity_type: "postgres_table"
    attributes:
      department: "finance"
      sensitivity: "high"

  # Public/Shared Tables
  - entity_pattern: "postgres_table:public.*"
    owner_group: "Data-Admins"
    visible_to_groups:
      - "All-Employees"
    entity_type: "postgres_table"
    attributes:
      sensitivity: "low"
```

### 4.2 Generate Ownership Edges

```bash
# Generate ownership edges
python3 tools/link_ownership.py \
  --mapping ownership_mapping.yaml \
  --output ownership_edges.jsonl

# Review output
head -10 ownership_edges.jsonl
```

**Example Output:**
```json
{
  "id": "edge:a1b2c3d4e5f6a7b8",
  "type": "OWNED_BY",
  "source": "postgres_table:hr.employees",
  "target": "ad_group:HR-Admins",
  "attributes": {
    "department": "hr",
    "sensitivity": "high"
  }
}
```

---

## Step 5: Ingest Data into ThemisDB

### 5.1 Import PostgreSQL Data (Existing)

```bash
# Export PostgreSQL schema
pg_dump -h localhost -U postgres -d mydb -s > schema.sql

# Import into ThemisDB
python3 tools/ingest.py \
  --source schema.sql \
  --type postgres \
  --output pg_entities.jsonl
```

### 5.2 Ingest AD Data

```bash
# Ingest AD users, groups, OUs
python3 tools/ingest.py \
  --source ad_export.jsonl \
  --config ingest_config.yaml
```

### 5.3 Ingest Ownership Edges

```bash
# Ingest ownership relationships
python3 tools/ingest.py \
  --source ownership_edges.jsonl \
  --config ingest_config.yaml
```

---

## Step 6: Testing and Validation

### 6.1 Test JWT Token Acquisition

```bash
# Keycloak - Get token via password grant
curl -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "client_secret=your-client-secret" \
  -d "username=jdoe" \
  -d "password=user-password" \
  -d "scope=openid email profile"

# Extract access token
export JWT_TOKEN="<access_token from response>"
```

### 6.2 Verify JWT Claims

```bash
# Decode JWT (using jwt.io or jwt-cli)
echo $JWT_TOKEN | jwt decode -

# Expected output:
# {
#   "sub": "...",
#   "preferred_username": "jdoe",
#   "email": "john.doe@example.com",
#   "roles": ["Engineering-Team", "Data-Analysts"],
#   ...
# }
```

### 6.3 Test ThemisDB API Access

```bash
# Test authenticated request
curl -H "Authorization: Bearer $JWT_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects

# Expected: 200 OK (if user has Engineering-Team role)
# Expected: 403 Forbidden (if user lacks required role)
```

### 6.4 Test Ranger Policy Evaluation

```bash
# Test policy evaluation endpoint
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
# {
#   "allowed": true,
#   "matched_policies": ["allow-engineering-read"]
# }
```

### 6.5 Verify Ownership Graph

```bash
# Query ownership edges
curl -H "Authorization: Bearer $JWT_TOKEN" \
  "http://localhost:8765/query/aql" \
  -d "MATCH (t:postgres_table)-[:OWNED_BY]->(g:ad_group) RETURN t.id, g.attributes.cn LIMIT 10"

# Expected: List of tables with their owner groups
```

### 6.6 End-to-End Access Check

**Scenario:** User `jdoe` (member of `Engineering-Team`) tries to access HR data

```bash
# 1. Get JWT token for jdoe
export JWT_TOKEN="<token with Engineering-Team role>"

# 2. Try to access HR table (should be denied)
curl -H "Authorization: Bearer $JWT_TOKEN" \
  http://localhost:8765/entities/postgres_table:hr.salaries

# Expected: 403 Forbidden
# Reason: User only has Engineering-Team role, not HR-Admins or HR-Readers

# 3. Try to access Engineering table (should be allowed)
curl -H "Authorization: Bearer $JWT_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects

# Expected: 200 OK
# Reason: User has Engineering-Team role which matches policy
```

---

## Troubleshooting

### Issue: LDAP Export Fails

**Symptoms:** Connection timeout or authentication error

**Solutions:**
- Verify LDAP server URL and port
- Check network connectivity: `telnet dc.example.com 389`
- Verify bind DN and password
- Try anonymous bind first to test connectivity
- Check firewall rules

### Issue: JWT Validation Fails

**Symptoms:** 401 Unauthorized even with valid token

**Solutions:**
- Verify JWKS URL is accessible from ThemisDB server
- Check issuer and audience claims match configuration
- Verify token is not expired (`exp` claim)
- Check clock skew settings (allow 5-minute tolerance)
- Inspect logs: `grep JWT themisdb.log`

### Issue: Ranger Policy Not Matching

**Symptoms:** 403 Forbidden despite having correct group membership

**Solutions:**
- Verify group name in JWT matches Ranger policy subject exactly
  - Check for case sensitivity
  - Check for path prefixes (e.g., `/Engineering/Team` vs `Team`)
  - Check for domain suffixes (e.g., `Team@example.com` vs `Team`)
- Review policy evaluation logs
- Test with wildcard subject `"*"` temporarily to verify policy syntax
- Use policy evaluation API to debug matching

### Issue: Ownership Edges Not Created

**Symptoms:** `link_ownership.py` creates no edges

**Solutions:**
- Verify entity IDs in PostgreSQL import match patterns in ownership mapping
- Check pattern syntax (wildcards must use `.*` not just `*`)
- Test pattern matching with sample entities
- Review logs: `cat ownership_linkage.log`
- Verify AD groups exist in ThemisDB before creating edges

### Issue: No Groups in JWT Claims

**Symptoms:** JWT token missing `roles` or `groups` claim

**Solutions:**
- **Keycloak**: Add realm role mapper to client
- **Azure AD**: Enable groups claim in token configuration
- **Auth0**: Add rule to include roles in token
- Verify user has group memberships assigned
- Test with IdP's token inspector/debugger

---

## Security Considerations

### 1. LDAP Credentials

- **Never commit** bind passwords to version control
- Use environment variables: `bind_password: "${LDAP_BIND_PASSWORD}"`
- Or use secrets manager (HashiCorp Vault, AWS Secrets Manager)
- Use dedicated read-only service account with minimal permissions
- Rotate credentials regularly

### 2. JWT Token Security

- **Always use HTTPS** for token transmission
- Configure short token lifetimes (15-60 minutes)
- Implement token refresh mechanism
- Validate token signatures using JWKS
- Check token expiry with clock skew tolerance
- Never log token values

### 3. Network Security

- Use LDAPS (LDAP over SSL/TLS) in production
- Restrict ThemisDB API access to internal network
- Use VPN or private network for Ranger communication
- Enable mutual TLS for sensitive endpoints

### 4. Access Control

- Follow principle of least privilege
- Define groups with minimal required permissions
- Use deny policies for sensitive data
- Implement IP-based restrictions for critical resources
- Review Ranger policies regularly

### 5. Data Protection

- Encrypt LDAP export files at rest
- Restrict access to ownership mapping files
- Audit all policy changes
- Monitor authentication failures
- Set up alerts for anomalous access patterns

---

## Maintenance

### Regular Tasks

**Daily:**
- Monitor authentication metrics (`/metrics`)
- Review authorization failures in logs

**Weekly:**
- Sync AD exports with ThemisDB (if directory changes frequently)
- Review Ranger policy effectiveness
- Audit access patterns

**Monthly:**
- Update ownership mappings for new tables/schemas
- Review and prune stale AD groups from ThemisDB
- Rotate LDAP service account credentials
- Test disaster recovery procedures

**Quarterly:**
- Full audit of policies and group memberships
- Review and update security configurations
- Performance tuning for policy evaluation

### Automation

Consider automating common tasks:

```bash
#!/bin/bash
# Daily AD sync script

set -e

# Export AD data
python3 /opt/themisdb/tools/ldap_export.py \
  --config /etc/themisdb/ldap_export_config.yaml \
  --output /tmp/ad_export.jsonl

# Update ownership edges
python3 /opt/themisdb/tools/link_ownership.py \
  --mapping /etc/themisdb/ownership_mapping.yaml \
  --output /tmp/ownership_edges.jsonl

# Ingest into ThemisDB
python3 /opt/themisdb/tools/ingest.py --source /tmp/ad_export.jsonl
python3 /opt/themisdb/tools/ingest.py --source /tmp/ownership_edges.jsonl

# Cleanup
rm /tmp/ad_export.jsonl /tmp/ownership_edges.jsonl

# Log completion
echo "AD sync completed at $(date)" >> /var/log/themisdb/ad_sync.log
```

---

## References

- [ThemisDB RBAC Authorization Guide](guides_rbac_authorization.md)
- [API Authentication and Authorization](../security/api_authentication_authorization.md)
- [Apache Ranger Documentation](https://ranger.apache.org/quick_start_guide.html)
- [Keycloak Documentation](https://www.keycloak.org/documentation)
- [LDAP3 Python Library](https://ldap3.readthedocs.io/)

---

## Appendix: Complete Example Workflow

```bash
# ===== STEP 1: Export Active Directory =====

export LDAP_BIND_PASSWORD="secure-password"

python3 tools/ldap_export.py \
  --config ldap_export_config.yaml \
  --output ad_export.jsonl

# ===== STEP 2: Import PostgreSQL Data =====

pg_dump -h localhost -U postgres -d mydb > mydb_dump.sql

# (ThemisDB PostgreSQL importer ingests this automatically)

# ===== STEP 3: Create Ownership Mappings =====

python3 tools/link_ownership.py \
  --mapping ownership_mapping.yaml \
  --output ownership_edges.jsonl

# ===== STEP 4: Ingest into ThemisDB =====

# Ingest AD data
python3 tools/ingest.py --source ad_export.jsonl

# Ingest ownership edges
python3 tools/ingest.py --source ownership_edges.jsonl

# ===== STEP 5: Configure Ranger Policies =====

# Create policies in config/policies.yaml
cat <<EOF > config/policies.yaml
- id: allow-engineering-read
  name: Engineering Team read access
  subjects: ["Engineering-Team"]
  actions: ["read"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow

- id: allow-hr-admins-write
  name: HR Admins full access
  subjects: ["HR-Admins"]
  actions: ["read", "write", "delete"]
  resources: ["/entities/postgres_table:hr.*"]
  effect: allow
EOF

# ===== STEP 6: Test Access =====

# Get JWT token
export JWT_TOKEN=$(curl -s -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "username=jdoe" \
  -d "password=password" \
  | jq -r '.access_token')

# Test access to engineering table
curl -H "Authorization: Bearer $JWT_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects

# Should return 200 OK if jdoe is in Engineering-Team

echo "Setup complete!"
```

---

**End of Guide**
