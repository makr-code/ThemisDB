# Admin Tools - Audit und Compliance

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Admin Tools  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Audit Logging](#audit-logging)
- [Compliance Reports](#compliance-reports)
- [Data Classification](#data-classification)
- [PII Management](#pii-management)
- [Retention Policies](#retention-policies)
- [Praktische Szenarien](#praktische-szenarien)

---

## Übersicht

ThemisDB bietet umfassende Audit- und Compliance-Tools für regulierte Umgebungen.

### Compliance Standards

| Standard | Status | Beschreibung |
|----------|--------|--------------|
| **GDPR** | ✅ Vollständig | EU-Datenschutz-Grundverordnung |
| **HIPAA** | ✅ Vollständig | Health Insurance Portability |
| **SOX** | ✅ Vollständig | Sarbanes-Oxley Act |
| **ISO 27001** | ✅ Zertifiziert | Information Security Management |
| **PCI DSS** | ✅ Level 1 | Payment Card Industry |

---

## Audit Logging

### Konfiguration

```yaml
# themis-config.yaml
audit:
  enabled: true
  log_level: "detailed"  # minimal, standard, detailed
  
  # What to log
  events:
    - authentication
    - data_access
    - data_modification
    - schema_changes
    - admin_operations
  
  # Where to store
  storage:
    type: "database"  # database, file, syslog
    retention_days: 365
    
  # Compliance
  tamper_proof: true
  encryption: true
  
  # Alerting
  alerts:
    - event: "failed_login"
      threshold: 5
      window: "5m"
      action: "notify_admin"
```

### Audit Event Types

```bash
# View all event types
themis-admin audit events list
```

**Event Types:**
```
┌─────────────────────┬────────────────────────────────────┐
│ Event Type          │ Description                        │
├─────────────────────┼────────────────────────────────────┤
│ LOGIN_SUCCESS       │ Successful user login              │
│ LOGIN_FAILED        │ Failed login attempt               │
│ LOGOUT              │ User logout                        │
│ DOCUMENT_READ       │ Document read access               │
│ DOCUMENT_CREATE     │ Document creation                  │
│ DOCUMENT_UPDATE     │ Document modification              │
│ DOCUMENT_DELETE     │ Document deletion                  │
│ QUERY_EXECUTE       │ Query execution                    │
│ COLLECTION_CREATE   │ Collection created                 │
│ COLLECTION_DROP     │ Collection dropped                 │
│ USER_CREATE         │ User account created               │
│ USER_UPDATE         │ User permissions changed           │
│ USER_DELETE         │ User account deleted               │
│ BACKUP_CREATE       │ Backup created                     │
│ BACKUP_RESTORE      │ Backup restored                    │
│ CONFIG_CHANGE       │ Configuration changed              │
└─────────────────────┴────────────────────────────────────┘
```

### Audit Log Query

```bash
# Recent events
themis-admin audit query \
  --since "24h" \
  --limit 100

# Filter by user
themis-admin audit query \
  --user "alice" \
  --event "DOCUMENT_DELETE" \
  --since "7d"

# Export to CSV
themis-admin audit query \
  --since "30d" \
  --format csv \
  --output audit-report.csv
```

**REST API:**
```bash
# Query audit logs
curl "http://localhost:8765/api/v1/admin/audit/query" \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "filters": {
      "user": "alice",
      "event_type": "DOCUMENT_DELETE",
      "since": "2026-01-17T00:00:00Z"
    },
    "limit": 100
  }'
```

**Response:**
```json
{
  "events": [
    {
      "id": "evt-12345",
      "timestamp": "2026-01-24T14:30:15.123Z",
      "event_type": "DOCUMENT_DELETE",
      "user": "alice",
      "user_role": "admin",
      "ip_address": "192.168.1.100",
      "collection": "users",
      "document_key": "user-123",
      "success": true,
      "details": {
        "reason": "user_request",
        "ticket_id": "GDPR-2026-0124"
      }
    }
  ],
  "total": 15,
  "page": 1
}
```

### Audit Trail für Dokument

```bash
# Show complete history of a document
themis-admin audit trail \
  --collection users \
  --key user-123
```

**Output:**
```
Document Audit Trail: users/user-123

┌─────────────────────┬─────────┬──────────────────┬──────────┐
│ Timestamp           │ User    │ Action           │ Changes  │
├─────────────────────┼─────────┼──────────────────┼──────────┤
│ 2026-01-20 10:00:00 │ alice   │ CREATE           │ Initial  │
│ 2026-01-21 14:30:00 │ alice   │ UPDATE           │ age: 30→31│
│ 2026-01-22 09:15:00 │ bob     │ READ             │ -        │
│ 2026-01-24 14:30:15 │ alice   │ DELETE           │ Complete │
└─────────────────────┴─────────┴──────────────────┴──────────┘

Total events: 4
Retention: 365 days
Immutable: Yes
```

---

## Compliance Reports

### GDPR Data Subject Report

```bash
# Generate GDPR report for user
themis-admin compliance gdpr-report \
  --subject-email "alice@example.com" \
  --output alice-gdpr-report.pdf
```

**Report Content:**
```
GDPR Data Subject Access Request Report
Subject: alice@example.com
Generated: 2026-01-24T14:30:00Z

Personal Data Stored:
- Collection: users
  Key: user-123
  Data: {name, email, age, address}
  
- Collection: orders
  Keys: [order-456, order-789]
  Data: {shipping_address, billing_address}

Processing Activities:
- Marketing emails: Consent given 2025-06-15
- Analytics: Legitimate interest
- Order processing: Contract fulfillment

Data Retention:
- User profile: 2 years after last activity
- Order history: 7 years (legal requirement)

Third-party Sharing:
- Payment processor: Stripe (DPA signed)
- Shipping provider: DHL (DPA signed)
```

### HIPAA Access Log

```bash
# HIPAA-compliant access log
themis-admin compliance hipaa-log \
  --patient-id "P12345" \
  --since "2026-01-01" \
  --output hipaa-access-log.pdf
```

**Output:**
```
HIPAA Access Log
Patient ID: P12345
Period: 2026-01-01 to 2026-01-24

┌─────────────────────┬──────────┬─────────┬──────────────┬────────┐
│ Timestamp           │ User     │ Role    │ Action       │ Reason │
├─────────────────────┼──────────┼─────────┼──────────────┼────────┤
│ 2026-01-15 09:30:00 │ dr_jones │ Doctor  │ READ         │ Consult│
│ 2026-01-15 10:00:00 │ dr_jones │ Doctor  │ UPDATE       │ Notes  │
│ 2026-01-18 14:00:00 │ nurse_a  │ Nurse   │ READ         │ Vitals │
│ 2026-01-24 11:30:00 │ dr_smith │ Doctor  │ READ         │ Review │
└─────────────────────┴──────────┴─────────┴──────────────┴────────┘

Break-the-glass Access: None
Unauthorized Attempts: 0
```

### SOX Compliance Report

```bash
# SOX financial data access report
themis-admin compliance sox-report \
  --quarter Q1-2026 \
  --output sox-q1-2026.pdf
```

---

## Data Classification

### Classification Levels

```yaml
# data-classification.yaml
classification_levels:
  - level: PUBLIC
    color: green
    retention: 1_year
    encryption: optional
    
  - level: INTERNAL
    color: yellow
    retention: 3_years
    encryption: required
    
  - level: CONFIDENTIAL
    color: orange
    retention: 7_years
    encryption: required
    audit_all_access: true
    
  - level: RESTRICTED
    color: red
    retention: 10_years
    encryption: required
    audit_all_access: true
    require_justification: true
    mfa_required: true
```

### Auto-Classification

```bash
# Classify collection
themis-admin classification auto \
  --collection users \
  --scan-fields

# Manual classification
themis-admin classification set \
  --collection financial_records \
  --level RESTRICTED \
  --fields "ssn,bank_account,credit_card"
```

**Auto-Classification Rules:**
```python
# classification_rules.py
CLASSIFICATION_RULES = {
    'email': 'INTERNAL',
    'password': 'RESTRICTED',
    'ssn': 'RESTRICTED',
    'credit_card': 'RESTRICTED',
    'bank_account': 'RESTRICTED',
    'medical_record': 'RESTRICTED',
    'address': 'CONFIDENTIAL',
    'phone': 'CONFIDENTIAL',
    'salary': 'CONFIDENTIAL'
}
```

---

## PII Management

### PII Detection

```bash
# Scan for PII
themis-admin pii scan \
  --collection users \
  --output pii-report.json
```

**Report:**
```json
{
  "collection": "users",
  "total_documents": 10000,
  "pii_found": {
    "email": 10000,
    "phone": 9500,
    "address": 8900,
    "ssn": 2300,
    "credit_card": 0
  },
  "recommendations": [
    "Mask SSN in non-production environments",
    "Encrypt email field",
    "Implement field-level access control for address"
  ]
}
```

### PII Anonymization

```bash
# Anonymize PII for testing
themis-admin pii anonymize \
  --collection users \
  --target test_users \
  --fields "email,phone,address"
```

**Example:**
```
Original:
  name: "Alice Smith"
  email: "alice.smith@example.com"
  phone: "+1-555-0123"
  address: "123 Main St, Berlin"

Anonymized:
  name: "User-A1B2C3"
  email: "user.a1b2c3@test.example.com"
  phone: "+1-555-9999"
  address: "Street 1, City"
```

### Right to be Forgotten (GDPR)

```bash
# Delete all user data
themis-admin pii delete \
  --email "alice@example.com" \
  --reason "user_request" \
  --ticket "GDPR-2026-0124"
```

**Process:**
```
Deleting data for: alice@example.com

✅ Found in collection: users (1 document)
✅ Found in collection: orders (3 documents)
✅ Found in collection: reviews (5 documents)
✅ Found in collection: audit_logs (marked for retention)

Actions taken:
1. Deleted user profile (users/user-123)
2. Anonymized order data (kept for legal requirement)
3. Deleted reviews
4. Updated audit log (kept for compliance)

Deletion request logged: GDPR-2026-0124
Verification period: 30 days
```

---

## Retention Policies

### Policy Configuration

```yaml
# retention-policies.yaml
policies:
  - name: "user_profiles"
    collections: ["users"]
    retention_period: "2_years_after_last_activity"
    action: "delete"
    
  - name: "financial_records"
    collections: ["orders", "invoices"]
    retention_period: "7_years"
    action: "archive"
    
  - name: "audit_logs"
    collections: ["audit_logs"]
    retention_period: "10_years"
    action: "immutable"
    
  - name: "temporary_data"
    collections: ["sessions", "cache"]
    retention_period: "24_hours"
    action: "delete"
```

### Apply Retention Policy

```bash
# Create policy
themis-admin retention create \
  --name user_profiles \
  --collection users \
  --retention "2y" \
  --action delete

# List policies
themis-admin retention list

# Execute policy
themis-admin retention execute \
  --policy user_profiles \
  --dry-run  # Test first
```

### Automatic Cleanup

```bash
# Schedule automatic cleanup
themis-admin retention schedule \
  --policy user_profiles \
  --cron "0 2 * * 0"  # Every Sunday at 2 AM
```

---

## Praktische Szenarien

### Szenario 1: GDPR Data Subject Access Request

```python
#!/usr/bin/env python3
# gdpr_dsar.py - Data Subject Access Request

import requests
import json
from datetime import datetime

class GDPRProcessor:
    def __init__(self, server, auth):
        self.server = server
        self.auth = auth
    
    def find_user_data(self, email):
        """Find all data for a user across all collections"""
        collections = self.get_collections()
        user_data = {}
        
        for collection in collections:
            # Search for user email in collection
            query = f"""
                FOR doc IN {collection}
                  FILTER doc.email == @email OR doc.user_email == @email
                  RETURN doc
            """
            
            result = requests.post(
                f'{self.server}/api/v1/query',
                auth=self.auth,
                json={'query': query, 'bind_vars': {'email': email}}
            ).json()
            
            if result['entities']:
                user_data[collection] = result['entities']
        
        return user_data
    
    def generate_dsar_report(self, email):
        """Generate GDPR DSAR report"""
        user_data = self.find_user_data(email)
        
        report = {
            'request_type': 'GDPR_DSAR',
            'subject_email': email,
            'generated_at': datetime.utcnow().isoformat(),
            'data_found': user_data,
            'collections_searched': len(self.get_collections()),
            'records_found': sum(len(v) for v in user_data.values())
        }
        
        # Save report
        filename = f'dsar_{email}_{datetime.now().strftime("%Y%m%d")}.json'
        with open(filename, 'w') as f:
            json.dump(report, f, indent=2)
        
        return report
    
    def get_collections(self):
        response = requests.get(
            f'{self.server}/api/v1/collections',
            auth=self.auth
        )
        return [c['name'] for c in response.json()['collections']]

# Usage
processor = GDPRProcessor('http://localhost:8765', ('admin', 'secret'))
report = processor.generate_dsar_report('alice@example.com')
print(f"Found {report['records_found']} records in {len(report['data_found'])} collections")
```

### Szenario 2: Automated Compliance Auditing

```bash
#!/bin/bash
# compliance-audit.sh

set -e

REPORT_DIR="/reports/compliance"
DATE=$(date +%Y%m%d)

echo "=== Compliance Audit $(date) ==="

# 1. Check audit log integrity
echo "Checking audit log integrity..."
themis-admin audit verify --full

# 2. Generate access reports
echo "Generating access reports..."
for user in $(themis-admin user list --format json | jq -r '.[].username'); do
    themis-admin audit query \
      --user "$user" \
      --since "30d" \
      --output "$REPORT_DIR/access-$user-$DATE.json"
done

# 3. PII compliance check
echo "Scanning for PII compliance..."
themis-admin pii scan --all-collections \
  --output "$REPORT_DIR/pii-scan-$DATE.json"

# 4. Check retention policies
echo "Checking retention policies..."
themis-admin retention check --report \
  --output "$REPORT_DIR/retention-$DATE.json"

# 5. Security audit
echo "Running security audit..."
themis-admin security audit \
  --output "$REPORT_DIR/security-$DATE.json"

# 6. Generate summary report
echo "Generating summary..."
python3 << 'EOF'
import json
import glob

reports = {}
for file in glob.glob('$REPORT_DIR/*-$DATE.json'):
    with open(file) as f:
        report_type = file.split('/')[-1].split('-')[0]
        reports[report_type] = json.load(f)

summary = {
    'audit_date': '$DATE',
    'pii_issues': len(reports.get('pii', {}).get('issues', [])),
    'security_issues': len(reports.get('security', {}).get('issues', [])),
    'retention_expired': reports.get('retention', {}).get('expired_count', 0)
}

with open('$REPORT_DIR/summary-$DATE.json', 'w') as f:
    json.dump(summary, f, indent=2)

print("Summary:", summary)
EOF

# 7. Send report to compliance team
echo "Sending reports..."
tar -czf "$REPORT_DIR/compliance-$DATE.tar.gz" $REPORT_DIR/*-$DATE.*
# Upload to secure storage or send via email

echo "=== Compliance audit completed ==="
```

### Szenario 3: Real-time Compliance Monitoring

```python
#!/usr/bin/env python3
# compliance_monitor.py

import requests
import time
from datetime import datetime

class ComplianceMonitor:
    def __init__(self, server, auth):
        self.server = server
        self.auth = auth
        self.alerts = []
    
    def check_suspicious_activity(self):
        """Check for suspicious access patterns"""
        # Failed login attempts
        response = requests.get(
            f'{self.server}/api/v1/admin/audit/query',
            auth=self.auth,
            params={
                'event_type': 'LOGIN_FAILED',
                'since': '1h'
            }
        )
        
        failed_logins = response.json()['events']
        
        # Group by user
        user_failures = {}
        for event in failed_logins:
            user = event['user']
            user_failures[user] = user_failures.get(user, 0) + 1
        
        # Alert on excessive failures
        for user, count in user_failures.items():
            if count >= 5:
                self.alert(f'Excessive failed logins for user {user}: {count}')
    
    def check_after_hours_access(self):
        """Check for access outside business hours"""
        hour = datetime.now().hour
        
        if hour < 8 or hour > 18:  # Outside 8 AM - 6 PM
            response = requests.get(
                f'{self.server}/api/v1/admin/audit/query',
                auth=self.auth,
                params={
                    'event_type': 'DOCUMENT_READ',
                    'classification': 'RESTRICTED',
                    'since': '1h'
                }
            )
            
            events = response.json()['events']
            if events:
                self.alert(f'After-hours access to restricted data: {len(events)} events')
    
    def check_bulk_exports(self):
        """Detect bulk data exports"""
        response = requests.get(
            f'{self.server}/api/v1/admin/queries/recent',
            auth=self.auth,
            params={'since': '1h'}
        )
        
        for query in response.json()['queries']:
            if 'FOR doc IN' in query['query'] and 'RETURN doc' in query['query']:
                if query['result_count'] > 10000:
                    self.alert(f'Bulk export detected: {query["result_count"]} records by {query["user"]}')
    
    def alert(self, message):
        """Send alert"""
        print(f'🚨 ALERT: {message}')
        self.alerts.append({
            'timestamp': datetime.utcnow().isoformat(),
            'message': message
        })
        
        # Send to monitoring system
        requests.post(
            'https://hooks.slack.com/services/YOUR/WEBHOOK',
            json={'text': f'Compliance Alert: {message}'}
        )
    
    def monitor(self, interval=60):
        """Continuous monitoring"""
        while True:
            print(f'\n=== Compliance Check {datetime.now()} ===')
            
            self.check_suspicious_activity()
            self.check_after_hours_access()
            self.check_bulk_exports()
            
            print(f'Alerts: {len(self.alerts)}')
            
            time.sleep(interval)

if __name__ == '__main__':
    monitor = ComplianceMonitor('http://localhost:8765', ('admin', 'secret'))
    monitor.monitor(interval=60)
```

---

## Siehe auch

- [Admin Tools Management](admin_tools_management.md)
- [Security Guide](../security/SECURITY_GUIDE.md)
- [GDPR Compliance](../compliance/GDPR_COMPLIANCE.md)
- [Audit Configuration](../guides/GUIDE_AUDIT_CONFIGURATION.md)
