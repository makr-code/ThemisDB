# AD/LDAP Integration Test Playbook

**Version:** 1.0  
**Purpose:** Validate Active Directory/LDAP integration with ThemisDB and Apache Ranger  
**Last Updated:** April 2026

---

## Overview

This playbook provides step-by-step test procedures to validate the AD/LDAP integration, JWT authentication, Ranger policy enforcement, and ownership graph ingestion.

---

## Test Environment Setup

### Prerequisites

- ThemisDB server running (port 8765)
- LDAP/AD server accessible
- Identity Provider configured (Keycloak/Azure AD/Auth0)
- Apache Ranger configured (optional)
- Test users and groups created in AD
- Python 3.8+ with required packages installed

### Test Users and Groups

Create the following test objects in Active Directory:

**Users:**
- `test-engineer` (member of `Engineering-Team`)
- `test-hr-admin` (member of `HR-Admins`)
- `test-readonly` (member of `All-Employees` only)

**Groups:**
- `Engineering-Team`
- `HR-Admins`
- `HR-Readers`
- `Finance-Admins`
- `Data-Admins`
- `All-Employees`

**Test Tables in PostgreSQL:**
- `engineering.projects`
- `engineering.tasks`
- `hr.employees`
- `hr.salaries`
- `finance.invoices`
- `public.reference_data`

---

## Test Suite 1: LDAP Export

### Test 1.1: Basic LDAP Connection

**Objective:** Verify connectivity to LDAP server

**Steps:**
```bash
# Test with minimal config
python3 tools/ldap_export.py \
  --server ldap://dc.example.com \
  --base-dn "DC=example,DC=com" \
  --bind-dn "CN=ldap-reader,DC=example,DC=com" \
  --bind-password "password" \
  --max-entries 10 \
  --output test_ldap_connection.jsonl
```

**Expected Result:**
- Connection succeeds
- At least 10 entries exported
- No connection errors in log

**Validation:**
```bash
# Check log for success
grep "Successfully connected" ldap_export.log

# Verify entries exported
wc -l test_ldap_connection.jsonl
# Should show at least 10 lines

# Cleanup
rm test_ldap_connection.jsonl
```

### Test 1.2: User Export

**Objective:** Export AD users with all required attributes

**Steps:**
```bash
python3 tools/ldap_export.py \
  --config ldap_export_config.yaml \
  --output test_users.jsonl

# Filter only user nodes
grep '"type": "ad_user"' test_users.jsonl > test_users_only.jsonl
```

**Expected Result:**
- User nodes contain required attributes:
  - `sAMAccountName`
  - `objectGUID`
  - `mail`
  - `displayName`
  - `distinguishedName`

**Validation:**
```bash
# Check first user
head -1 test_users_only.jsonl | jq '.'

# Verify attributes
head -1 test_users_only.jsonl | jq '.attributes | keys'
# Should include: sAMAccountName, mail, displayName, etc.

# Count users
wc -l test_users_only.jsonl
```

### Test 1.3: Group Export

**Objective:** Export AD groups with member relationships

**Steps:**
```bash
# Filter only group nodes
grep '"type": "ad_group"' test_users.jsonl > test_groups_only.jsonl

# Filter MEMBER_OF edges
grep '"type": "MEMBER_OF"' test_users.jsonl > test_memberships.jsonl
```

**Expected Result:**
- Group nodes created for each AD group
- MEMBER_OF edges link users to groups

**Validation:**
```bash
# Verify Engineering-Team exists
grep "Engineering-Team" test_groups_only.jsonl | jq '.attributes.sAMAccountName'

# Check memberships
cat test_memberships.jsonl | jq -r '"\(.source) -> \(.target)"'

# Count groups and edges
echo "Groups: $(wc -l < test_groups_only.jsonl)"
echo "Memberships: $(wc -l < test_memberships.jsonl)"
```

### Test 1.4: OU Export and Hierarchy

**Objective:** Export organizational units with parent-child relationships

**Steps:**
```bash
# Filter OU nodes
grep '"type": "ad_ou"' test_users.jsonl > test_ous.jsonl

# Filter CHILD_OF edges
grep '"type": "CHILD_OF"' test_users.jsonl > test_ou_hierarchy.jsonl
```

**Expected Result:**
- OU nodes created
- CHILD_OF edges represent OU hierarchy

**Validation:**
```bash
# List OUs
cat test_ous.jsonl | jq '.attributes.ou'

# Verify hierarchy
cat test_ou_hierarchy.jsonl | jq -r '"\(.source) is child of \(.target)"'
```

### Test 1.5: Performance with Large Directory

**Objective:** Verify paging works for large directories

**Steps:**
```bash
# Export with page size
python3 tools/ldap_export.py \
  --config ldap_export_config.yaml \
  --output test_large_export.jsonl

# Monitor progress
tail -f ldap_export.log
```

**Expected Result:**
- Export completes without timeout
- Paging occurs (check log for multiple LDAP queries)
- All entries exported

**Validation:**
```bash
# Check export summary
tail -20 ldap_export.log | grep -A 5 "Export Summary"

# Verify total count matches AD
# Compare with: (Get-ADUser -Filter *).Count in PowerShell
```

---

## Test Suite 2: JWT Token Acquisition and Validation

### Test 2.1: Obtain JWT Token (Keycloak)

**Objective:** Get valid JWT token from Keycloak

**Steps:**
```bash
# Request token
curl -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "client_secret=your-client-secret" \
  -d "username=test-engineer" \
  -d "password=engineer-password" \
  -d "scope=openid email profile" \
  -o token_response.json

# Extract access token
export JWT_TOKEN=$(cat token_response.json | jq -r '.access_token')

# Verify token obtained
echo $JWT_TOKEN | cut -c1-50
```

**Expected Result:**
- HTTP 200 response
- Access token returned
- Token is a valid JWT (three base64 sections separated by dots)

**Validation:**
```bash
# Check response
cat token_response.json | jq 'keys'
# Should include: access_token, expires_in, token_type

# Verify token format
echo $JWT_TOKEN | grep -oE '^[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+$'
# Should match (no output = invalid format)
```

### Test 2.2: Decode and Verify JWT Claims

**Objective:** Verify JWT contains required claims

**Steps:**
```bash
# Decode JWT (requires jwt-cli or use jwt.io)
# Install: cargo install jwt-cli
echo $JWT_TOKEN | jwt decode -

# Or use Python
python3 << 'EOF'
import json
import base64
import sys

token = sys.stdin.read().strip()
parts = token.split('.')
payload = parts[1]

# Add padding if needed
payload += '=' * (4 - len(payload) % 4)

decoded = base64.urlsafe_b64decode(payload)
claims = json.loads(decoded)

print(json.dumps(claims, indent=2))
EOF
```

**Input:** (paste JWT_TOKEN)

**Expected Result:**
- Claims include:
  - `sub`: User identifier
  - `preferred_username`: "test-engineer"
  - `email`: User email
  - `roles` or `groups`: ["Engineering-Team", ...]
  - `iss`: Keycloak issuer URL
  - `aud`: "themis-app"
  - `exp`: Future timestamp

**Validation:**
```bash
# Check specific claims
echo $JWT_TOKEN | jwt decode - | jq '.roles'
# Should include "Engineering-Team"

echo $JWT_TOKEN | jwt decode - | jq '.preferred_username'
# Should be "test-engineer"
```

### Test 2.3: JWT Token with Azure AD

**Objective:** Verify Azure AD token format

**Steps:**
```bash
# Get token from Azure AD
curl -X POST \
  https://login.microsoftonline.com/{tenant-id}/oauth2/v2.0/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=client_credentials" \
  -d "client_id={client-id}" \
  -d "client_secret={client-secret}" \
  -d "scope=https://graph.microsoft.com/.default"

export JWT_TOKEN_AZURE="<access_token>"
```

**Expected Result:**
- Token obtained
- Claims include `groups` or `roles`

**Validation:**
```bash
echo $JWT_TOKEN_AZURE | jwt decode - | jq '.groups'
```

### Test 2.4: Expired Token Handling

**Objective:** Verify ThemisDB rejects expired tokens

**Steps:**
```bash
# Create expired token (or wait for current token to expire)
# For testing, modify exp claim or wait until token expires

# Test with expired token
curl -H "Authorization: Bearer $EXPIRED_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects
```

**Expected Result:**
- HTTP 401 Unauthorized
- Error message: "Token expired"

**Validation:**
```bash
# Check response
curl -v -H "Authorization: Bearer $EXPIRED_TOKEN" \
  http://localhost:8765/entities/postgres_table:engineering.projects 2>&1 \
  | grep "HTTP/1.1"
# Should show "401"
```

---

## Test Suite 3: Ranger Policy Configuration and Import

### Test 3.1: Load Policies from YAML

**Objective:** Verify policies can be loaded from configuration file

**Steps:**
```bash
# Create test policies
cat <<'EOF' > test_policies.yaml
- id: test-engineering-read
  name: Engineering Team read access
  subjects: ["Engineering-Team"]
  actions: ["read", "data:read"]
  resources: ["/entities/postgres_table:engineering.*"]
  effect: allow

- id: test-hr-write
  name: HR Admins full access
  subjects: ["HR-Admins"]
  actions: ["read", "write", "data:read", "data:write"]
  resources: ["/entities/postgres_table:hr.*"]
  effect: allow

- id: test-deny-external
  name: Deny external access to salaries
  subjects: ["*"]
  actions: ["read", "write"]
  resources: ["/entities/postgres_table:hr.salaries"]
  effect: deny
  allowed_ip_prefixes: ["10.0.", "192.168."]
EOF

# Copy to config directory
cp test_policies.yaml config/policies.yaml

# Restart ThemisDB to load policies
# (or use hot reload if supported)
```

**Expected Result:**
- Policies loaded successfully
- No errors in log

**Validation:**
```bash
# Check ThemisDB logs
grep "Loaded.*policies" themisdb.log

# Query policies via API
curl -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://localhost:8765/policies/export/ranger | jq 'length'
# Should show 3 policies
```

### Test 3.2: Ranger Policy Export

**Objective:** Export policies in Ranger format

**Steps:**
```bash
# Export policies
curl -H "Authorization: Bearer $ADMIN_TOKEN" \
  http://localhost:8765/policies/export/ranger \
  -o exported_policies.json

# Review export
cat exported_policies.json | jq '.'
```

**Expected Result:**
- JSON array of policies
- Each policy has required fields: id, name, subjects, actions, resources, effect

**Validation:**
```bash
# Count policies
cat exported_policies.json | jq 'length'

# Check policy structure
cat exported_policies.json | jq '.[0] | keys'
# Should include: id, name, subjects, actions, resources, effect
```

### Test 3.3: Ranger Policy Import

**Objective:** Import policies from external Ranger service

**Prerequisites:** Ranger service running and accessible

**Steps:**
```bash
# Configure Ranger connection
export THEMIS_RANGER_BASE_URL=https://ranger.example.com
export THEMIS_RANGER_SERVICE=themisdb
export THEMIS_RANGER_BEARER=ranger-admin-token
export THEMIS_TOKEN_ADMIN=themisdb-admin-token

# Import policies
curl -X POST \
  -H "Authorization: Bearer $THEMIS_TOKEN_ADMIN" \
  http://localhost:8765/policies/import/ranger \
  -o import_result.json

# Check result
cat import_result.json | jq '.'
```

**Expected Result:**
- Policies imported successfully
- Response includes count of imported policies

**Validation:**
```bash
# Check import result
cat import_result.json | jq '.imported_policies'

# Verify policies loaded
curl -H "Authorization: Bearer $THEMIS_TOKEN_ADMIN" \
  http://localhost:8765/policies/export/ranger | jq 'length'
```

---

## Test Suite 4: Ownership Linkage

### Test 4.1: Create Ownership Edges from Mapping

**Objective:** Generate ownership edges using mapping file

**Steps:**
```bash
# Create test mapping
cat <<'EOF' > test_ownership_mapping.yaml
mappings:
  - entity_pattern: "postgres_table:engineering.*"
    owner_group: "Engineering-Team"
    visible_to_groups: ["Engineering-Team", "Data-Admins"]
    entity_type: "postgres_table"
    
  - entity_pattern: "postgres_table:hr.*"
    owner_group: "HR-Admins"
    visible_to_groups: ["HR-Readers", "HR-Admins", "Data-Admins"]
    entity_type: "postgres_table"
EOF

# Generate edges
python3 tools/link_ownership.py \
  --mapping test_ownership_mapping.yaml \
  --output test_ownership_edges.jsonl
```

**Expected Result:**
- OWNED_BY and VISIBLE_TO edges created
- Summary shows count of edges created

**Validation:**
```bash
# Check output
cat test_ownership_edges.jsonl | jq -r '.type' | sort | uniq -c

# Verify edge format
head -1 test_ownership_edges.jsonl | jq '.'

# Count OWNED_BY edges
grep '"type": "OWNED_BY"' test_ownership_edges.jsonl | wc -l

# Count VISIBLE_TO edges
grep '"type": "VISIBLE_TO"' test_ownership_edges.jsonl | wc -l
```

### Test 4.2: Convention-Based Ownership Mapping

**Objective:** Auto-generate mappings using naming conventions

**Steps:**
```bash
python3 tools/link_ownership.py \
  --convention "postgres_table:hr_*" \
  --output test_convention_edges.jsonl
```

**Expected Result:**
- Edges created for all tables matching pattern
- Owner group derived from pattern

**Validation:**
```bash
cat test_convention_edges.jsonl | jq -r '"\(.source) -> \(.target)"'
```

### Test 4.3: Ingest Ownership Edges

**Objective:** Load ownership edges into ThemisDB

**Steps:**
```bash
# Ingest edges
python3 tools/ingest.py \
  --source test_ownership_edges.jsonl \
  --config ingest_config.yaml
```

**Expected Result:**
- Edges ingested successfully
- No errors

**Validation:**
```bash
# Query edges via API
curl -H "Authorization: Bearer $JWT_TOKEN" \
  "http://localhost:8765/query/aql" \
  -d "MATCH (t:postgres_table)-[:OWNED_BY]->(g:ad_group) RETURN t.id, g.id LIMIT 5"
```

---

## Test Suite 5: End-to-End Access Control

### Test 5.1: Authenticated Access to Allowed Resource

**Objective:** User can access resource they have permission for

**Steps:**
```bash
# Get token for test-engineer (member of Engineering-Team)
export JWT_TOKEN_ENG=$(curl -s -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "username=test-engineer" \
  -d "password=password" \
  | jq -r '.access_token')

# Access engineering table
curl -v -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:engineering.projects
```

**Expected Result:**
- HTTP 200 OK
- Entity data returned

**Validation:**
```bash
# Check status code
curl -s -o /dev/null -w "%{http_code}" \
  -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:engineering.projects
# Should output: 200
```

### Test 5.2: Denied Access to Unauthorized Resource

**Objective:** User cannot access resource they don't have permission for

**Steps:**
```bash
# test-engineer tries to access HR data
curl -v -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:hr.salaries
```

**Expected Result:**
- HTTP 403 Forbidden
- Error message explaining denial

**Validation:**
```bash
# Check status code
curl -s -o /dev/null -w "%{http_code}" \
  -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:hr.salaries
# Should output: 403

# Check error message
curl -s -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:hr.salaries \
  | jq '.error'
```

### Test 5.3: Admin Access to All Resources

**Objective:** Admin user has full access

**Steps:**
```bash
# Get token for admin
export JWT_TOKEN_ADMIN=$(curl -s -X POST \
  https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
  -d "grant_type=password" \
  -d "client_id=themis-app" \
  -d "username=admin" \
  -d "password=admin-password" \
  | jq -r '.access_token')

# Access various resources
curl -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  http://localhost:8765/entities/postgres_table:engineering.projects

curl -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  http://localhost:8765/entities/postgres_table:hr.salaries

curl -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  http://localhost:8765/entities/postgres_table:finance.invoices
```

**Expected Result:**
- All requests return HTTP 200 OK
- Admin can access all resources

**Validation:**
```bash
# Test multiple resources
for table in engineering.projects hr.salaries finance.invoices; do
  status=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
    http://localhost:8765/entities/postgres_table:$table)
  echo "$table: $status"
done
# All should show 200
```

### Test 5.4: Policy Evaluation API

**Objective:** Test direct policy evaluation

**Steps:**
```bash
# Test policy evaluation
curl -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "subject": "Engineering-Team",
    "action": "read",
    "resource": "/entities/postgres_table:engineering.projects"
  }' \
  http://localhost:8765/policies/evaluate
```

**Expected Result:**
- Response shows `"allowed": true`
- Matched policy ID returned

**Validation:**
```bash
# Check response
curl -s -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "subject": "Engineering-Team",
    "action": "read",
    "resource": "/entities/postgres_table:engineering.projects"
  }' \
  http://localhost:8765/policies/evaluate \
  | jq '.allowed'
# Should output: true
```

### Test 5.5: IP-Based Access Restriction

**Objective:** Verify IP filtering in policies

**Prerequisites:** Deny policy with `allowed_ip_prefixes` configured

**Steps:**
```bash
# Access from allowed IP (internal network)
curl -H "Authorization: Bearer $JWT_TOKEN_ENG" \
  http://localhost:8765/entities/postgres_table:hr.salaries

# Simulate access from denied IP
# (In real test, make request from external IP or use X-Forwarded-For header)
curl -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "X-Forwarded-For: 203.0.113.1" \
  http://localhost:8765/entities/postgres_table:hr.salaries
```

**Expected Result:**
- Internal IP: 403 (denied by policy, but IP check passes)
- External IP: 403 (denied by IP check)

**Validation:**
```bash
# Check logs for IP-based denial
grep "IP.*denied" themisdb.log
```

---

## Test Suite 6: Graph Queries and Ownership

### Test 6.1: Query User Group Memberships

**Objective:** Verify user-group relationships in graph

**Steps:**
```bash
curl -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (u:ad_user {sAMAccountName: \"test-engineer\"})-[:MEMBER_OF]->(g:ad_group) RETURN g.attributes.cn AS group_name"
  }' \
  http://localhost:8765/query/aql
```

**Expected Result:**
- Returns list of groups: ["Engineering-Team", ...]

**Validation:**
```bash
# Parse results
curl -s -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (u:ad_user {sAMAccountName: \"test-engineer\"})-[:MEMBER_OF]->(g:ad_group) RETURN g.attributes.cn"
  }' \
  http://localhost:8765/query/aql \
  | jq '.results[].group_name'
```

### Test 6.2: Query Table Ownership

**Objective:** Find owner of a specific table

**Steps:**
```bash
curl -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (t:postgres_table {id: \"postgres_table:engineering.projects\"})-[:OWNED_BY]->(g:ad_group) RETURN g.attributes.cn AS owner"
  }' \
  http://localhost:8765/query/aql
```

**Expected Result:**
- Returns owner group: "Engineering-Team"

**Validation:**
```bash
curl -s -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (t:postgres_table {id: \"postgres_table:engineering.projects\"})-[:OWNED_BY]->(g:ad_group) RETURN g.attributes.cn"
  }' \
  http://localhost:8765/query/aql \
  | jq '.results[0].owner'
# Should output: "Engineering-Team"
```

### Test 6.3: Find All Tables Visible to Group

**Objective:** List all tables a group can access

**Steps:**
```bash
curl -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (t:postgres_table)-[:VISIBLE_TO]->(g:ad_group {cn: \"Engineering-Team\"}) RETURN t.id AS table_id"
  }' \
  http://localhost:8765/query/aql
```

**Expected Result:**
- Returns list of tables visible to Engineering-Team

**Validation:**
```bash
curl -s -X POST \
  -H "Authorization: Bearer $JWT_TOKEN_ADMIN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "MATCH (t:postgres_table)-[:VISIBLE_TO]->(g:ad_group {cn: \"Engineering-Team\"}) RETURN t.id"
  }' \
  http://localhost:8765/query/aql \
  | jq '.results[].table_id'
```

---

## Test Results Summary

Create a summary report after running all tests:

```bash
cat <<'EOF' > test_results_summary.md
# AD/LDAP Integration Test Results

**Date:** $(date)
**Tester:** [Your Name]
**Environment:** [Test/Staging/Production]

## Test Suite Results

| Test Suite | Total Tests | Passed | Failed | Notes |
|------------|-------------|--------|--------|-------|
| LDAP Export | 5 | 5 | 0 | All exports successful |
| JWT Token | 4 | 4 | 0 | Token validation working |
| Ranger Policies | 3 | 3 | 0 | Policy import/export OK |
| Ownership Linkage | 3 | 3 | 0 | Edges created correctly |
| Access Control | 5 | 5 | 0 | Authorization working |
| Graph Queries | 3 | 3 | 0 | Ownership queries OK |

## Overall Status

✅ **PASS** - All 23 tests passed

## Issues Found

None

## Recommendations

- Monitor JWT token expiry in production
- Set up automated daily AD sync
- Review Ranger policies quarterly

EOF
```

---

## Troubleshooting Common Test Failures

### LDAP Export Fails

**Check:**
- Network connectivity: `telnet dc.example.com 389`
- Bind credentials are correct
- Base DN exists
- Service account has read permissions

### JWT Token Invalid

**Check:**
- JWKS URL is accessible
- Token not expired (`exp` claim)
- Issuer and audience match configuration
- Clock skew tolerance set (5 minutes recommended)

### Policy Not Matching

**Check:**
- Group name in JWT exactly matches policy subject
- Resource pattern uses correct syntax (wildcards)
- Policy effect is "allow" (not "deny")
- No conflicting deny policies

### Ownership Edges Not Found

**Check:**
- Edges were ingested successfully
- Entity IDs match exactly (case-sensitive)
- AD groups exist in ThemisDB before creating edges

---

**End of Test Playbook**
