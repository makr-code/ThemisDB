# Audit Log Retention Policy Implementation

## Overview

This document describes the implementation of automated retention policy enforcement for audit logs in ThemisDB. The implementation ensures compliance with regulatory requirements such as GDPR, BSI IT-Grundschutz, and ISO 27001 by automatically archiving and purging audit logs based on configurable retention policies.

## Background

ThemisDB's audit logging system records security events, access patterns, and compliance-relevant activities. These logs must be retained for specific periods as mandated by:

- **GDPR Article 5(2)**: Accountability principle requiring demonstrable compliance
- **BSI IT-Grundschutz**: Recommends 7-year retention for security audit trails
- **ISO 27001**: Information security management requirements

The retention policy defined in `config/retention_policies.yaml` specifies:
- **Retention Period**: 7 years (2555 days)
- **Archive Period**: 5 years (1825 days) 
- **Auto-Purge**: Enabled after retention period expires

## Architecture

### Components

1. **AuditLogger** (`include/utils/audit_logger.h`, `src/utils/audit_logger.cpp`)
   - Logs security events to `data/logs/audit.jsonl` in JSON Lines format
   - Supports encryption-then-sign pattern for tamper-proof logging
   - Maintains hash chain for integrity verification

2. **RetentionManager** (`include/utils/retention_manager.h`, `src/utils/retention_manager.cpp`)
   - Manages retention policies loaded from YAML configuration
   - Schedules periodic retention checks
   - Executes archive and purge operations based on policy rules

3. **Retention Thread** (in `src/main_server.cpp`)
   - Background thread that runs retention checks at configured intervals (default: daily)
   - Enumerates audit log entries and applies retention policies
   - Logs retention actions to a separate audit trail for compliance tracking

### Data Flow

```
[Audit Events] 
    ↓
[AuditLogger.logEvent()]
    ↓
[data/logs/audit.jsonl]
    ↓
[Retention Thread - Daily Check]
    ↓
[AuditLogger.enumerateEntries()]
    ↓
[Age Evaluation vs Policy]
    ↓
    ├─→ [< 5 years] → Retain in active log
    ├─→ [5-7 years] → Archive to data/logs/audit_archive.jsonl
    └─→ [> 7 years] → Purge (permanent deletion)
```

## Implementation Details

### New Methods in AuditLogger

#### 1. `enumerateEntries()`
```cpp
std::vector<AuditLogEntry> enumerateEntries() const;
```
- Reads all entries from the audit log file
- Extracts timestamps and metadata for each entry
- Returns vector of `AuditLogEntry` structures containing:
  - `entry_number`: Sequential position in log
  - `timestamp`: Entry creation time
  - `record`: Full JSON record

#### 2. `archiveOldEntries()`
```cpp
size_t archiveOldEntries(
    std::chrono::system_clock::time_point older_than,
    const std::string& archive_path
);
```
- Moves entries older than threshold to archive file
- Appends to existing archive (preserves previous archives)
- Atomically updates main log file to remove archived entries
- Returns count of entries archived

#### 3. `purgeOldEntries()`
```cpp
size_t purgeOldEntries(
    std::chrono::system_clock::time_point older_than
);
```
- Permanently deletes entries older than threshold
- Rewrites main log file without purged entries
- Returns count of entries purged

### Integration with Retention System

The retention thread in `main_server.cpp` has been enhanced to handle audit logs specially:

1. **Entity Provider**: When processing the "audit_logs" policy, the entity provider calls `main_audit_logger->enumerateEntries()` instead of querying the database.

2. **Archive Handler**: Detects audit log entries (by "audit_entry_" prefix) and performs bulk archival:
   - Uses atomic flag to ensure single execution per retention run
   - Archives all entries older than 5 years in one operation
   - Logs the archival action to the retention audit trail

3. **Purge Handler**: Detects audit log entries and performs bulk purge:
   - Uses atomic flag to ensure single execution per retention run
   - Purges all entries older than 7 years in one operation  
   - Logs the purge action to the retention audit trail

### Configuration

Retention policies are defined in `config/retention_policies.yaml`:

```yaml
policies:
  - name: "audit_logs"
    description: "Security and compliance audit trails"
    retention_days: 2555  # 7 years (BSI IT-Grundschutz)
    archive_days: 1825    # Archive after 5 years
    auto_purge: true
    legal_basis: "GDPR Art. 5(2) - Accountability principle, BSI IT-Grundschutz"
    categories:
      - "authentication_logs"
      - "access_logs"
      - "governance_events"
      - "pii_detection_events"
    compliance:
      - "GDPR"
      - "BSI"
      - "ISO27001"
```

Server configuration enables the retention system:

```yaml
features:
  retention:
    enabled: true
    interval_hours: 24  # Daily retention checks
    policies_path: "./config/retention_policies.yaml"
```

## Testing

### Unit Tests

Comprehensive test suite in `tests/test_audit_logger.cpp`:

1. **EnumerateEntries**: Verifies correct enumeration of audit log entries
2. **ArchiveOldEntries**: Tests archival of entries older than threshold
3. **PurgeOldEntries**: Tests deletion of entries older than threshold
4. **RetentionWithNoOldEntries**: Ensures no-op when all entries are recent

Run tests with:
```bash
# Build tests
cmake --build build --target test_audit_logger

# Run tests
./build/test_audit_logger
```

## Security Considerations

1. **Audit Trail of Retention Actions**: All archive and purge operations are logged to a separate audit trail (`data/logs/retention_audit.jsonl`) with:
   - Action type (AUDIT_LOG_ARCHIVE / AUDIT_LOG_PURGE)
   - Count of affected entries
   - Timestamp
   - Classification for compliance tracking

2. **Atomic Operations**: Archive and purge operations are protected by file-level locking to prevent concurrent modifications.

3. **Data Integrity**: Unparseable entries are preserved during archive/purge operations to prevent accidental data loss.

4. **Hash Chain Preservation**: The audit log's integrity hash chain is maintained independently and not affected by retention operations.

## File Locations

- **Active Audit Log**: `data/logs/audit.jsonl`
- **Archive Audit Log**: `data/logs/audit_archive.jsonl`
- **Retention Audit Trail**: `data/logs/retention_audit.jsonl`
- **Hash Chain State**: `data/logs/audit_chain.json`

## Compliance Benefits

1. **GDPR Compliance**: Demonstrates accountability by maintaining audit trails for required period and purging after retention expires
2. **BSI IT-Grundschutz**: Meets 7-year retention requirement for security logs
3. **ISO 27001**: Provides evidence of information security management
4. **Storage Optimization**: Reduces storage costs by archiving old logs and purging expired data
5. **Audit Trail**: Complete record of retention actions for compliance audits

## Future Enhancements

Potential improvements for future releases:

1. **Compression**: Compress archived logs with zstd or gzip
2. **Remote Archive**: Upload archived logs to S3/Azure Blob/Google Cloud Storage
3. **Encrypted Archive**: Encrypt archived logs with separate key for long-term storage
4. **Retention Reports**: Generate compliance reports showing retention policy adherence
5. **Archive Rotation**: Rotate archive files by year/month for better organization
6. **Multi-policy Support**: Support different retention policies for different audit event types

## References

- GDPR Article 5(2): https://gdpr-info.eu/art-5-gdpr/
- BSI IT-Grundschutz: https://www.bsi.bund.de/EN/Topics/ITGrundschutz/
- ISO 27001: https://www.iso.org/standard/27001
