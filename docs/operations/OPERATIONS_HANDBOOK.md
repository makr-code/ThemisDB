# ThemisDB Operations Handbook

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Site Reliability Engineers, Database Administrators, Security Operations, Compliance Officers

---

## Table of Contents

1. [Overview](#overview)
2. [Access Management](#access-management)
3. [Incident Response](#incident-response)
4. [Disaster Recovery](#disaster-recovery)
5. [Logging & Monitoring](#logging--monitoring)
6. [Compliance & Audit](#compliance--audit)
7. [Automation & CI/CD](#automation--cicd)
3. [Schema Management](#schema-management)
4. [Incident Response](#incident-response)
5. [Disaster Recovery](#disaster-recovery)
6. [Logging & Monitoring](#logging--monitoring)
7. [Compliance & Audit](#compliance--audit)
8. [Automation & CI/CD](#automation--cicd)

---

## Overview

This Operations Handbook provides comprehensive operational procedures for ThemisDB deployments. It covers access management, incident response, disaster recovery, logging configuration, and compliance requirements aligned with ISO 27001 and BSI C5 standards.

### Operational Objectives

- **Availability:** 99.95% uptime (RTO: 1 hour, RPO: 15 minutes)
- **Security:** Zero-trust access model with automated reviews
- **Compliance:** ISO 27001, BSI C5, GDPR, SOC 2
- **Automation:** 90% of operational tasks automated

---

## Access Management

### Overview

Access management procedures ensure proper user access controls throughout the user lifecycle, from onboarding to offboarding.

### Access Review Procedures

**Frequency:** Quarterly (automated monthly reports)

**Automated Access Review:**
```bash
# Run access review report
./scripts/operations/access-review.sh --report

# Generate compliance report
./scripts/operations/access-review.sh --compliance-report
```

**Manual Review Process:**
1. Review automated access report
2. Validate user roles against current job functions
3. Identify and flag excessive permissions
4. Document review findings
5. Submit remediation plan

**See also:** [Access Review Automation](access-management/ACCESS_REVIEW_AUTOMATION.md)

### User Onboarding

**Process:**
1. Submit access request via ServiceNow/JIRA
2. Manager approval required
3. Security team validation
4. Role-based access provisioning
5. MFA enrollment mandatory
6. Security training completion required

**Standard Roles:**
- **Admin:** Full system access (requires MFA)
- **Operator:** Operational access (requires MFA)
- **Developer:** Read/write access to dev environments
- **Auditor:** Read-only access to logs and audit trails
- **User:** Basic authenticated access

### User Offboarding

**Process:**
1. HR initiates offboarding
2. Automated rights revocation triggered
3. Access keys rotated
4. Audit trail preserved
5. Knowledge transfer documented

**Automated Rights Revocation:**
```bash
# Revoke user access
./scripts/operations/revoke-access.sh --user <username>

# Bulk revocation for terminated users
./scripts/operations/revoke-access.sh --batch users.csv
```

**See also:** [Rights Revocation Automation](access-management/RIGHTS_REVOCATION.md)

### Access Documentation

All access changes must be documented in:
- **Change Log:** `logs/access-changes.log`
- **Audit Trail:** `logs/audit.log`
- **Compliance Dashboard:** Grafana metrics

---

## Schema Management

### Overview

ThemisDB provides comprehensive schema introspection and management capabilities. The Schema Manager enables operators and developers to:
- Discover existing table schemas automatically
- Define custom schemas for validation and documentation
- Update schemas with partial modifications
- Query schema information via REST API

**Version:** 1.5.0+  
**Feature Tag:** Schema Registry (GAP #001)

### REST API Endpoints

#### GET /api/v1/schema

**Description:** Retrieve complete database schema including all tables and relationships.

**Response:**
```json
{
  "status": "success",
  "metadata": {
    "version": "1.5.0",
    "table_count": 15,
    "total_rows": 125000,
    "capabilities": ["graph", "vector_search", "timeseries", ...],
    "last_refresh": "2026-02-03 15:30:45"
  },
  "tables": [
    {
      "name": "users",
      "type": "relational",
      "properties": [...],
      "indexes": [...],
      "estimated_row_count": 10000
    }
  ],
  "relationships": [...]
}
```

**Usage:**
```bash
curl http://localhost:8080/api/v1/schema
```

#### GET /api/v1/schema/tables

**Description:** List all tables with summary information.

**Response:**
```json
{
  "status": "success",
  "tables": [
    {
      "name": "users",
      "type": "relational",
      "estimated_row_count": 10000,
      "property_count": 12,
      "index_count": 3
    }
  ]
}
```

**Usage:**
```bash
curl http://localhost:8080/api/v1/schema/tables
```

#### GET /api/v1/schema/tables/:name

**Description:** Get detailed schema for a specific table.

**Response:**
```json
{
  "status": "success",
  "table": {
    "name": "users",
    "type": "relational",
    "properties": [
      {
        "name": "id",
        "type": "integer",
        "indexed": true,
        "nullable": false,
        "index_type": "regular"
      },
      {
        "name": "email",
        "type": "string",
        "indexed": true,
        "nullable": false
      },
      {
        "name": "created_at",
        "type": "integer",
        "indexed": false,
        "nullable": true
      }
    ],
    "indexes": [
      {
        "name": "id",
        "type": "regular",
        "unique": true,
        "columns": ["id"]
      },
      {
        "name": "email",
        "type": "regular",
        "unique": true,
        "columns": ["email"]
      }
    ],
    "estimated_row_count": 10000
  }
}
```

**Usage:**
```bash
curl http://localhost:8080/api/v1/schema/tables/users
```

#### PUT /api/v1/schema/:tablename

**Description:** Create or update a custom schema definition.

**Request Body:**
```json
{
  "name": "products",
  "type": "relational",
  "properties": [
    {
      "name": "id",
      "type": "integer",
      "indexed": true,
      "nullable": false
    },
    {
      "name": "name",
      "type": "string",
      "nullable": false
    },
    {
      "name": "price",
      "type": "double",
      "nullable": false
    },
    {
      "name": "description",
      "type": "string",
      "nullable": true
    }
  ],
  "indexes": [
    {
      "name": "id",
      "type": "regular",
      "unique": true,
      "columns": ["id"]
    }
  ]
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Schema stored successfully",
  "table_name": "products"
}
```

**Usage:**
```bash
curl -X PUT http://localhost:8080/api/v1/schema/products \
  -H "Content-Type: application/json" \
  -d @schema.json
```

**Validation:**
- Table name: alphanumeric, underscore, hyphen only
- Table type: `relational`, `document`, `graph_node`, `graph_edge`, `vector`
- Property types: `string`, `integer`, `double`, `boolean`, `vector`, `binary`, `null`
- Index types: `regular`, `range`, `sparse`, `geo`, `ttl`, `fulltext`, `composite`
- Index columns must reference existing properties
- No duplicate property or index names

#### PATCH /api/v1/schema/:tablename

**Description:** Partially update an existing schema. Only specified fields are modified.

**Request Body:**
```json
{
  "properties": [
    {
      "name": "category",
      "type": "string",
      "indexed": true,
      "nullable": true
    }
  ],
  "indexes": [
    {
      "name": "category",
      "type": "regular",
      "unique": false,
      "columns": ["category"]
    }
  ]
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Schema updated successfully",
  "table_name": "products"
}
```

**Usage:**
```bash
curl -X PATCH http://localhost:8080/api/v1/schema/products \
  -H "Content-Type: application/json" \
  -d '{"properties":[{"name":"category","type":"string"}]}'
```

**Behavior:**
- Creates table if it doesn't exist (promotes discovered schema to custom)
- Merges properties: new properties added, existing properties updated
- Merges indexes: new indexes added, existing indexes updated
- Does not remove existing properties/indexes (use PUT for full replacement)

#### GET /api/v1/capabilities

**Description:** Get database capabilities and enabled features.

**Response:**
```json
{
  "status": "success",
  "version": "1.5.0",
  "capabilities": [
    "multi-model",
    "transactions",
    "secondary-indexes",
    "fulltext-search",
    "graph",
    "vector_search",
    "geo",
    "mcp",
    "grpc"
  ]
}
```

**Usage:**
```bash
curl http://localhost:8080/api/v1/capabilities
```

### Operational Procedures

#### Schema Discovery

**Automatic Discovery:**
- Schema Manager automatically scans RocksDB keys to discover tables
- Properties detected by sampling up to 100 entities per table
- Index metadata retrieved from SecondaryIndexManager
- Row counts estimated via key iteration
- Results cached for 60 seconds (configurable)

**Manual Cache Refresh:**
```bash
# Force refresh via API (requires admin access)
curl -X POST http://localhost:8080/api/v1/admin/schema/refresh
```

#### Schema Persistence

**Storage Location:**
- Custom schemas: `config:schema:{table_name}` in RocksDB
- Loaded automatically on SchemaManager initialization
- Persisted immediately on PUT/PATCH operations

**Backup:**
```bash
# Backup all custom schemas
curl http://localhost:8080/api/v1/schema > schemas_backup.json

# Restore specific schema
curl -X PUT http://localhost:8080/api/v1/schema/users \
  -H "Content-Type: application/json" \
  -d @schemas_backup.json
```

#### Schema Validation Best Practices

**Pre-Deployment Validation:**
1. Export schemas from development: `GET /api/v1/schema`
2. Validate in staging environment
3. Review for consistency and correctness
4. Deploy to production using PUT endpoints

**Schema Version Control:**
```bash
# Save schema to version control
curl http://localhost:8080/api/v1/schema/users > schemas/users.json

# Commit to git
git add schemas/users.json
git commit -m "Add users table schema"
```

### Troubleshooting

#### Schema Not Found

**Symptom:** `GET /api/v1/schema/tables/mytable` returns `"status": "error"`, `"message": "Table not found"`

**Possible Causes:**
1. Table doesn't exist in database
2. No data has been inserted yet
3. Cache hasn't refreshed since table creation

**Resolution:**
```bash
# Check if table has data
curl http://localhost:8080/entities?table=mytable

# Force cache refresh
curl -X POST http://localhost:8080/api/v1/admin/schema/refresh

# Create custom schema explicitly
curl -X PUT http://localhost:8080/api/v1/schema/mytable \
  -H "Content-Type: application/json" \
  -d '{"name":"mytable","type":"relational","properties":[]}'
```

#### Validation Errors

**Symptom:** `PUT /api/v1/schema` returns validation error

**Common Errors:**
- `"Table name contains invalid characters"` - Use only alphanumeric, `_`, `-`
- `"Invalid table type"` - Must be one of: relational, document, graph_node, graph_edge, vector
- `"Duplicate property name"` - Property names must be unique
- `"Index references non-existent property"` - Index columns must exist in properties list
- `"Invalid property type"` - Must be: string, integer, double, boolean, vector, binary, null

**Resolution:**
Review schema definition against validation rules and correct errors.

#### Performance Issues

**Symptom:** Schema discovery takes >1 second

**Possible Causes:**
1. Large number of tables (>100)
2. Tables with many entities (>100K rows)
3. Complex property structures

**Mitigation:**
```bash
# Increase cache TTL to reduce refresh frequency
# Configure in themis.conf:
schema_cache_ttl_seconds: 300  # 5 minutes instead of 60 seconds

# Reduce sample size for property discovery
# Configure in themis.conf:
schema_property_sample_size: 50  # Default: 100
```

### Monitoring & Metrics

**Key Metrics:**
- `schema_cache_hits` - Cache hit rate (target: >90%)
- `schema_cache_miss` - Cache misses triggering rebuild
- `schema_discovery_duration_ms` - Discovery time (target: <100ms)
- `schema_validation_errors` - Failed PUT/PATCH attempts
- `schema_custom_count` - Number of custom schemas

**Grafana Dashboard:**
```
Dashboard: ThemisDB Schema Management
URL: https://grafana.example.com/d/schema-mgmt
```

**Alerts:**
- Discovery time >500ms for 5 consecutive requests
- Cache hit rate <80% for 10 minutes
- Validation error rate >10% of PUT/PATCH requests

### Security Considerations

**Access Control:**
- GET endpoints: Requires authenticated user (any role)
- PUT/PATCH endpoints: Requires `schema:write` permission
- Recommended: Limit schema write access to DBA and DevOps roles

**Audit Logging:**
All schema operations logged to audit trail:
```
[2026-02-03 15:30:45] USER=admin ACTION=schema_put TABLE=products STATUS=success
[2026-02-03 15:31:12] USER=developer ACTION=schema_patch TABLE=orders STATUS=validation_error
```

**Rate Limiting:**
- GET requests: 1000 req/min per user
- PUT/PATCH requests: 100 req/min per user (to prevent abuse)

---

## Incident Response

### Overview

Incident response procedures follow ISO 27001 (A.16) and BSI C5 (OIS-01 to OIS-04) requirements.

### Incident Classification

| Severity | Response Time | Examples |
|----------|---------------|----------|
| **P0 - Critical** | 15 minutes | Data breach, service outage |
| **P1 - High** | 1 hour | Security vulnerability, significant degradation |
| **P2 - Medium** | 4 hours | Minor service issues, non-critical bugs |
| **P3 - Low** | 24 hours | Documentation issues, feature requests |

### Incident Response Process

**Detection → Triage → Containment → Eradication → Recovery → Lessons Learned**

**See also:** [Incident Response Playbook](incident-response/INCIDENT_RESPONSE_PLAYBOOK.md)

### Automated Incident Response Drills

**Frequency:** Monthly

```bash
# Run incident response drill
./scripts/operations/incident-drill.sh --scenario data-breach

# Generate drill report
./scripts/operations/incident-drill.sh --report
```

**Drill Scenarios:**
1. Data breach response
2. Ransomware attack
3. DDoS attack
4. Insider threat
5. Service outage recovery

**See also:** [Incident Response Testing](incident-response/INCIDENT_RESPONSE_TESTING.md)

---

## Disaster Recovery

### Overview

Disaster Recovery (DR) procedures ensure business continuity with defined Recovery Time Objective (RTO) and Recovery Point Objective (RPO).

**RTO Target:** 1 hour  
**RPO Target:** 15 minutes

### Backup Procedures

**Backup Schedule:**
- **Full Backup:** Daily at 02:00 UTC
- **Incremental Backup:** Every 4 hours
- **Transaction Log Backup:** Every 15 minutes

**Backup Retention:**
- Daily backups: 30 days
- Weekly backups: 90 days
- Monthly backups: 1 year

### Automated DR Testing

**Frequency:** Weekly

```bash
# Run automated DR test
./scripts/operations/dr-test.sh --full

# Test backup restore
./scripts/operations/dr-test.sh --test-restore

# Verify RTO/RPO metrics
./scripts/operations/dr-test.sh --verify-metrics
```

**See also:** [DR Testing Automation](disaster-recovery/DR_TESTING.md)

### DR Checklists

**Pre-Disaster:**
- [ ] Backup verification completed
- [ ] DR site connectivity verified
- [ ] Runbooks up to date
- [ ] Contact lists current

**During Disaster:**
- [ ] Incident declared
- [ ] Stakeholders notified
- [ ] DR site activated
- [ ] Data restoration initiated

**Post-Disaster:**
- [ ] Service restored
- [ ] Data integrity verified
- [ ] Incident report completed
- [ ] Lessons learned documented

**See also:** [DR Checklists](disaster-recovery/DR_CHECKLISTS.md)

---

## Logging & Monitoring

### Logging Configuration

**Centralized Logging:** All logs forwarded to centralized logging system (ELK/Splunk)

**Log Categories:**
- **Application Logs:** `/var/log/themisdb/app.log`
- **Audit Logs:** `/var/log/themisdb/audit.log`
- **Security Logs:** `/var/log/themisdb/security.log`
- **Performance Logs:** `/var/log/themisdb/performance.log`

**Log Retention:**
- Application logs: 90 days
- Audit logs: 7 years (compliance requirement)
- Security logs: 2 years

**See also:** [Logging Configuration Guide](logging/LOGGING_CONFIGURATION.md)

### Monitoring Configuration

**Metrics Collection:**
- System metrics (CPU, memory, disk, network)
- Application metrics (transactions, queries, errors)
- Security metrics (authentication failures, access violations)
- Compliance metrics (audit events, policy violations)

**Alerting Thresholds:**
- CPU utilization > 80% for 5 minutes
- Memory utilization > 85%
- Disk space < 15% free
- Error rate > 1% of requests
- Authentication failure rate > 10 per minute

**See also:** [Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)

---

## Compliance & Audit

### Compliance Standards

ThemisDB operations comply with:
- **ISO 27001:2013** - Information Security Management
- **BSI C5:2020** - Cloud Computing Compliance Controls Catalogue
- **GDPR** - General Data Protection Regulation
- **SOC 2 Type II** - Service Organization Controls

### Audit Requirements

**Audit Log Requirements:**
- All administrative actions logged
- All access attempts logged (success and failure)
- All data modifications logged
- All configuration changes logged

**Audit Trail Integrity:**
- Logs cryptographically signed
- Tamper-proof storage
- Immutable audit records
- Regular integrity verification

### Compliance Dashboard

**Metrics Dashboard:** Grafana dashboards track compliance indicators

**Key Metrics:**
- Access review completion rate
- Incident response time (average)
- Backup success rate
- DR test success rate
- Security event rate
- Compliance policy violations

**Dashboard URL:** `https://grafana.example.com/d/compliance`

**See also:** [Metrics Dashboard Configuration](../observability/METRICS_DASHBOARD.md)

---

## Automation & CI/CD

### Operational Automation

**CI/CD Integration:**
- Access reviews: Monthly automated reports
- DR testing: Weekly automated tests
- Incident drills: Monthly automated scenarios
- Compliance reporting: Real-time dashboard updates

**GitHub Actions Workflows:**
- `.github/workflows/access-review.yml` - Automated access reviews
- `.github/workflows/dr-testing.yml` - Automated DR testing
- `.github/workflows/incident-drill.yml` - Incident response drills

### Operational Scripts

**Location:** `scripts/operations/`

**Available Scripts:**
- `access-review.sh` - Access review automation
- `revoke-access.sh` - Automated rights revocation
- `incident-drill.sh` - Incident response testing
- `dr-test.sh` - Disaster recovery testing
- `logging-config.sh` - Logging configuration management
- `compliance-report.sh` - Compliance reporting

### Script Usage

```bash
# Access review
./scripts/operations/access-review.sh --report

# Rights revocation
./scripts/operations/revoke-access.sh --user john.doe

# Incident drill
./scripts/operations/incident-drill.sh --scenario data-breach

# DR test
./scripts/operations/dr-test.sh --full

# Compliance report
./scripts/operations/compliance-report.sh --monthly
```

---

## Related Documentation

- [Operational Procedures](OPERATIONAL_PROCEDURES.md)
- [Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)
- [Troubleshooting Guide](TROUBLESHOOTING_GUIDE.md)
- [Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Audit Findings Report](../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

## Change History

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.5.0 | 2026-02-03 | Initial comprehensive handbook created | Operations Team |

---

**Document Version:** 1.5.0  
**ThemisDB Compatibility:** 1.5.0+  
**Last Reviewed:** 2026-02-03  
**Next Review:** 2026-05-03 (Quarterly)
