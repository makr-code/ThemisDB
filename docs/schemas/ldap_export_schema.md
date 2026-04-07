# LDAP Export Schema Documentation

**Version:** 1.0  
**Format:** JSONL (JSON Lines)  
**Compatible with:** ThemisDB Ingestion Tools  
**Last Updated:** April 2026

---

## Overview

This document describes the schema and format of JSONL output produced by the `ldap_export.py` tool. The export creates graph entities (nodes and edges) representing Active Directory/LDAP directory structure.

---

## Entity Types

### Node Types

1. **ad_user** - Active Directory user account
2. **ad_group** - Security or distribution group
3. **ad_ou** - Organizational Unit

### Edge Types

1. **MEMBER_OF** - User/group membership in a group
2. **IN_OU** - Entity containment within an OU
3. **CHILD_OF** - OU hierarchy (parent-child relationship)

---

## Node Schemas

### ad_user

Represents an Active Directory user account.

**Schema:**
```json
{
  "id": "ad_user:{objectGUID}",
  "type": "ad_user",
  "attributes": {
    "sAMAccountName": "string",
    "mail": "string",
    "displayName": "string",
    "userPrincipalName": "string",
    "cn": "string",
    "description": "string",
    "distinguishedName": "string",
    "ou_path": ["string"]
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp",
    "guid": "string"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique identifier: `ad_user:{objectGUID}` |
| `type` | string | Yes | Always `"ad_user"` |
| `attributes.sAMAccountName` | string | Yes | Windows logon name (e.g., "jdoe") |
| `attributes.mail` | string | No | Email address |
| `attributes.displayName` | string | No | Full display name |
| `attributes.userPrincipalName` | string | No | UPN (e.g., "jdoe@example.com") |
| `attributes.cn` | string | No | Common name |
| `attributes.description` | string | No | User description |
| `attributes.distinguishedName` | string | Yes | Full LDAP DN path |
| `attributes.ou_path` | array | No | List of parent OUs (closest first) |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |
| `metadata.guid` | string | Yes | objectGUID as UUID string |

**Example:**
```json
{
  "id": "ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "type": "ad_user",
  "attributes": {
    "sAMAccountName": "jdoe",
    "mail": "john.doe@example.com",
    "displayName": "John Doe",
    "userPrincipalName": "jdoe@example.com",
    "cn": "John Doe",
    "description": "Software Engineer",
    "distinguishedName": "CN=John Doe,OU=Engineering,OU=Employees,DC=example,DC=com",
    "ou_path": ["Engineering", "Employees"]
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000",
    "guid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
  }
}
```

---

### ad_group

Represents an Active Directory security or distribution group.

**Schema:**
```json
{
  "id": "ad_group:{objectGUID}",
  "type": "ad_group",
  "attributes": {
    "sAMAccountName": "string",
    "cn": "string",
    "description": "string",
    "distinguishedName": "string",
    "ou_path": ["string"]
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp",
    "guid": "string"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique identifier: `ad_group:{objectGUID}` |
| `type` | string | Yes | Always `"ad_group"` |
| `attributes.sAMAccountName` | string | Yes | Group name (e.g., "Engineering-Team") |
| `attributes.cn` | string | No | Common name |
| `attributes.description` | string | No | Group description |
| `attributes.distinguishedName` | string | Yes | Full LDAP DN path |
| `attributes.ou_path` | array | No | List of parent OUs |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |
| `metadata.guid` | string | Yes | objectGUID as UUID string |

**Example:**
```json
{
  "id": "ad_group:c3d4e5f6-a7b8-9012-cdef-123456789012",
  "type": "ad_group",
  "attributes": {
    "sAMAccountName": "Engineering-Team",
    "cn": "Engineering Team",
    "description": "Engineering department group",
    "distinguishedName": "CN=Engineering Team,OU=Groups,DC=example,DC=com",
    "ou_path": ["Groups"]
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000",
    "guid": "c3d4e5f6-a7b8-9012-cdef-123456789012"
  }
}
```

---

### ad_ou

Represents an Active Directory Organizational Unit.

**Schema:**
```json
{
  "id": "ad_ou:{objectGUID}",
  "type": "ad_ou",
  "attributes": {
    "ou": "string",
    "description": "string",
    "distinguishedName": "string"
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp",
    "guid": "string"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique identifier: `ad_ou:{objectGUID}` |
| `type` | string | Yes | Always `"ad_ou"` |
| `attributes.ou` | string | Yes | OU name (e.g., "Engineering") |
| `attributes.description` | string | No | OU description |
| `attributes.distinguishedName` | string | Yes | Full LDAP DN path |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |
| `metadata.guid` | string | Yes | objectGUID as UUID string |

**Example:**
```json
{
  "id": "ad_ou:e5f6a7b8-c9d0-1234-ef12-345678901234",
  "type": "ad_ou",
  "attributes": {
    "ou": "Engineering",
    "description": "Engineering department",
    "distinguishedName": "OU=Engineering,OU=Employees,DC=example,DC=com"
  },
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000",
    "guid": "e5f6a7b8-c9d0-1234-ef12-345678901234"
  }
}
```

---

## Edge Schemas

### MEMBER_OF

Represents group membership (user in group, or group in group).

**Schema:**
```json
{
  "id": "edge:{hash}",
  "type": "MEMBER_OF",
  "source": "ad_user:{guid} or ad_group:{guid}",
  "target": "ad_group:{guid}",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique edge identifier (SHA256 hash) |
| `type` | string | Yes | Always `"MEMBER_OF"` |
| `source` | string | Yes | User or group ID (member) |
| `target` | string | Yes | Group ID (parent group) |
| `attributes` | object | Yes | Empty object (reserved for future use) |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |

**Example (User to Group):**
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

**Example (Group to Group):**
```json
{
  "id": "edge:b2c3d4e5f6a7b8c9",
  "type": "MEMBER_OF",
  "source": "ad_group:d4e5f6a7-b8c9-0123-def1-234567890123",
  "target": "ad_group:c3d4e5f6-a7b8-9012-cdef-123456789012",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000"
  }
}
```

---

### IN_OU

Represents entity containment within an Organizational Unit.

**Schema:**
```json
{
  "id": "edge:{hash}",
  "type": "IN_OU",
  "source": "ad_user:{guid} or ad_group:{guid}",
  "target": "ad_ou:{guid}",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique edge identifier (SHA256 hash) |
| `type` | string | Yes | Always `"IN_OU"` |
| `source` | string | Yes | User or group ID |
| `target` | string | Yes | OU ID (immediate parent) |
| `attributes` | object | Yes | Empty object (reserved for future use) |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |

**Example:**
```json
{
  "id": "edge:d4e5f6a7b8c9d0e1",
  "type": "IN_OU",
  "source": "ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "target": "ad_ou:e5f6a7b8-c9d0-1234-ef12-345678901234",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000"
  }
}
```

---

### CHILD_OF

Represents OU hierarchy (child OU to parent OU).

**Schema:**
```json
{
  "id": "edge:{hash}",
  "type": "CHILD_OF",
  "source": "ad_ou:{guid}",
  "target": "ad_ou:{guid}",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "ISO8601 timestamp"
  }
}
```

**Field Descriptions:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique edge identifier (SHA256 hash) |
| `type` | string | Yes | Always `"CHILD_OF"` |
| `source` | string | Yes | Child OU ID |
| `target` | string | Yes | Parent OU ID |
| `attributes` | object | Yes | Empty object (reserved for future use) |
| `metadata.source` | string | Yes | Always `"ldap_export"` |
| `metadata.export_time` | string | Yes | ISO8601 timestamp of export |

**Example:**
```json
{
  "id": "edge:f6a7b8c9d0e1f2a3",
  "type": "CHILD_OF",
  "source": "ad_ou:e5f6a7b8-c9d0-1234-ef12-345678901234",
  "target": "ad_ou:f6a7b8c9-d0e1-2345-f123-456789012345",
  "attributes": {},
  "metadata": {
    "source": "ldap_export",
    "export_time": "2026-02-18T20:00:00.000000"
  }
}
```

---

## JSONL Format

The export file uses JSONL (JSON Lines) format:
- Each line is a complete, valid JSON object
- No commas between lines
- No wrapping array brackets
- One entity per line

**Example File:**
```
{"id":"ad_user:...","type":"ad_user",...}
{"id":"ad_user:...","type":"ad_user",...}
{"id":"ad_group:...","type":"ad_group",...}
{"id":"ad_ou:...","type":"ad_ou",...}
{"id":"edge:...","type":"MEMBER_OF",...}
{"id":"edge:...","type":"IN_OU",...}
```

---

## ID Generation

### Node IDs

Node IDs are generated using the format: `{type}:{objectGUID}`

- `type`: Entity type (`ad_user`, `ad_group`, `ad_ou`)
- `objectGUID`: Active Directory objectGUID converted to UUID string format

**Example:**
- AD objectGUID (binary): `\x01\x02\x03...` (16 bytes)
- Converted to UUID: `a1b2c3d4-e5f6-7890-abcd-ef1234567890`
- Node ID: `ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890`

### Edge IDs

Edge IDs are generated using SHA256 hash of the edge signature:
- Signature: `{source_id}-{edge_type}-{target_id}`
- Hash: SHA256 of signature
- ID: `edge:{first_16_chars_of_hash}`

**Example:**
- Source: `ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890`
- Type: `MEMBER_OF`
- Target: `ad_group:c3d4e5f6-a7b8-9012-cdef-123456789012`
- Signature: `ad_user:a1b2c3d4-e5f6-7890-abcd-ef1234567890-MEMBER_OF-ad_group:c3d4e5f6-a7b8-9012-cdef-123456789012`
- Hash: `a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6...`
- Edge ID: `edge:a1b2c3d4e5f6a7b8`

---

## Attribute Mapping

### Standard AD Attributes

The following AD attributes are exported by default:

**Users:**
- `sAMAccountName` → `attributes.sAMAccountName`
- `objectGUID` → `metadata.guid` (also used in ID)
- `distinguishedName` → `attributes.distinguishedName`
- `mail` → `attributes.mail`
- `displayName` → `attributes.displayName`
- `userPrincipalName` → `attributes.userPrincipalName`
- `cn` → `attributes.cn`
- `description` → `attributes.description`

**Groups:**
- `sAMAccountName` → `attributes.sAMAccountName`
- `objectGUID` → `metadata.guid`
- `distinguishedName` → `attributes.distinguishedName`
- `cn` → `attributes.cn`
- `description` → `attributes.description`

**OUs:**
- `objectGUID` → `metadata.guid`
- `distinguishedName` → `attributes.distinguishedName`
- `ou` → `attributes.ou`
- `description` → `attributes.description`

### Custom Attributes

Additional attributes can be configured in `ldap_export_config.yaml`:

```yaml
user_attributes:
  - sAMAccountName
  - objectGUID
  - mail
  - department      # Custom attribute
  - title           # Custom attribute
  - manager         # Custom attribute
```

Custom attributes appear in the `attributes` object with their original names.

---

## Validation Rules

### Required Fields

Every entity must have:
- `id` (non-empty string)
- `type` (valid entity type)
- `metadata.source` (must be "ldap_export")
- `metadata.export_time` (valid ISO8601 timestamp)

Nodes must have:
- `attributes.distinguishedName` (non-empty string)

Edges must have:
- `source` (valid node ID)
- `target` (valid node ID)

### ID Constraints

- Node IDs must follow format: `{type}:{guid}`
- Edge IDs must follow format: `edge:{hash}`
- GUIDs must be valid UUID v4 format
- Edge hashes must be 16 hex characters

### Relationship Constraints

- `MEMBER_OF`: Source must be `ad_user` or `ad_group`, target must be `ad_group`
- `IN_OU`: Source must be `ad_user` or `ad_group`, target must be `ad_ou`
- `CHILD_OF`: Source and target must both be `ad_ou`

---

## Integration with ThemisDB

### Ingestion

Use `tools/ingest.py` to load LDAP export into ThemisDB:

```bash
python3 tools/ingest.py \
  --source ad_export.jsonl \
  --config ingest_config.yaml
```

### Querying

Query AD entities using AQL (Themis Query Language):

```aql
// Find all users in Engineering team
MATCH (u:ad_user)-[:MEMBER_OF]->(g:ad_group {sAMAccountName: "Engineering-Team"})
RETURN u.attributes.displayName, u.attributes.mail

// Find organizational hierarchy
MATCH (ou:ad_ou)-[:CHILD_OF*]->(parent:ad_ou)
RETURN ou.attributes.ou, parent.attributes.ou

// Find all groups a user belongs to (direct and transitive)
MATCH (u:ad_user {sAMAccountName: "jdoe"})-[:MEMBER_OF*1..5]->(g:ad_group)
RETURN DISTINCT g.attributes.cn
```

---

## Version History

- **1.0** (2026-02-18): Initial schema definition
  - Node types: ad_user, ad_group, ad_ou
  - Edge types: MEMBER_OF, IN_OU, CHILD_OF
  - JSONL format specification

---

## See Also

- [LDAP Export Tool README](../../tools/README.md)
- [AD/LDAP Integration Guide](ad_ldap_integration_guide.md)
- [ThemisDB Ingestion Documentation](../../tools/ingest.py)
- [Graph Query Language (AQL) Reference](../aql/README.md)

---

**End of Schema Documentation**
