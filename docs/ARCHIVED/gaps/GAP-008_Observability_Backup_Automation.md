# GAP-008: Observability/Backup Automation

**Status:** ✅ Base Structure Implemented  
**Version:** 1.4.1-dev  
**Last Updated:** 2026-02-04

## Overview

GAP-008 implements the foundational structure for advanced observability and backup automation in ThemisDB. This implementation provides stub interfaces and placeholders for future cloud integration and Kubernetes-native operations.

## Implemented Features

### 1. Backup Automation Module

#### Cloud Backup Interface (Stub)
Located in `include/storage/backup_manager.h` and `src/storage/backup_manager.cpp`

**New Methods:**
- `scheduleBackup()` - Schedule automatic backups (K8s CronJob placeholder)
- `cancelScheduledBackup()` - Cancel scheduled backups
- `listScheduledBackups()` - List all scheduled backups
- `uploadBackupToCloud()` - Upload backups to S3/Azure/GCS (stub)
- `restoreFromCloud()` - Restore from cloud backups (stub)
- `createSnapshot()` - Create K8s VolumeSnapshots (stub)
- `restoreFromSnapshot()` - Restore from K8s VolumeSnapshots (stub)

**Example Usage:**
```cpp
#include "storage/backup_manager.h"

// Schedule daily backup at 2 AM
BackupOptions options;
options.storage = StorageBackend::S3;
options.compression = CompressionType::ZSTD;

auto result = backup_mgr->scheduleBackup(
    "0 2 * * *",  // Cron expression
    "incremental",
    options
);

// Upload to cloud (stub)
auto cloud_result = backup_mgr->uploadBackupToCloud(
    "/backups/full_20260204_020000",
    "s3://my-bucket/themisdb/backups",
    options
);

// Create Kubernetes snapshot (stub)
auto snapshot_result = backup_mgr->createSnapshot(
    "themisdb-snapshot-001",
    "fast-ssd"
);
```

### 2. Observability Module

#### Existing Health Check Systems (No Duplicates)

ThemisDB already has comprehensive health check systems in place:

1. **`sharding::HealthCheckSystem`** - Located in `include/sharding/health_check.h`
   - Shard and cluster health monitoring
   - Certificate validity checks
   - Storage capacity monitoring
   - Network connectivity checks
   - Response time tracking

2. **`sharding::HealthMonitor`** - Located in `include/sharding/health_monitor.h`
   - Node health monitoring with auto-failover
   - State machine: HEALTHY → SUSPECT → DOWN → RECOVERING
   - Auto-promotion of standby nodes
   - Event logging for audit trail

3. **`server::HealthErrorService`** - Located in `include/server/health_error_service.h`
   - HTTP health check endpoint (port 9090)
   - Error introspection API
   - Minimal dependencies for reliability

**Example Usage:**
```cpp
#include "sharding/health_check.h"
#include "server/health_error_service.h"

// Cluster health monitoring
sharding::HealthCheckSystem::Config config;
sharding::HealthCheckSystem health_system(config);
auto cluster_health = health_system.checkClusterHealth(shard_endpoints);

// HTTP health endpoint
server::HealthErrorService::Config http_config;
http_config.port = 9090;
server::HealthErrorService health_service(http_config);
health_service.start();

// Access via HTTP: curl http://localhost:9090/health
```

#### Alertmanager Integration (Stub) - NEW
Located in `include/observability/alertmanager.h` and `src/observability/alertmanager.cpp`

**Features:**
- Alert creation and management
- Alert severity levels (INFO, WARNING, ERROR, CRITICAL)
- Alert status tracking (FIRING, RESOLVED, SILENCED)
- Prometheus Alertmanager compatibility (stub)

**Example Usage:**
```cpp
#include "observability/alertmanager.h"

AlertmanagerConfig config;
config.endpoint_url = "http://alertmanager:9093";
config.enabled = true;
config.receivers = {"email", "slack", "pagerduty"};

DefaultAlertmanager alertmanager(config);

// Send alert
Alert alert;
alert.alert_name = "HighMemoryUsage";
alert.severity = AlertSeverity::WARNING;
alert.status = AlertStatus::FIRING;
alert.message = "Memory usage above 80%";
alert.labels["component"] = "database";
alert.labels["instance"] = "themisdb-0";

alertmanager.sendAlert(alert);

// Resolve alert
alertmanager.resolveAlert(alert.alert_id);

// Silence alert for 60 minutes
alertmanager.silenceAlert(alert.alert_id, 60);
```

## Testing

### Test Files
1. `tests/test_gap008_backup_automation.cpp` - Backup automation tests (11 test cases)
2. `tests/test_gap008_observability.cpp` - Alertmanager integration tests (10 test cases)

**Note:** Health check functionality is tested via existing test suites for the health check systems mentioned above.

### Running Tests
```bash
# Build and run tests
cd /home/runner/work/ThemisDB/ThemisDB/build
cmake --build . --target test_gap008_backup_automation
cmake --build . --target test_gap008_observability

# Run tests
./tests/test_gap008_backup_automation
./tests/test_gap008_observability
```

## Kubernetes Integration

### Backup CronJob Example
Located in `scripts/k8s/backup-cronjob.yaml`

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: themisdb-backup
spec:
  schedule: "0 2 * * *"  # Daily at 2 AM
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: themisdb/themisdb:latest
            command: ["/usr/local/bin/themisdb-backup"]
            args:
              - "--type=incremental"
              - "--destination=s3://my-bucket/backups"
            volumeMounts:
            - name: data
              mountPath: /data
```

### Health Check Probes (Using Existing Infrastructure)

ThemisDB provides health endpoints via `server::HealthErrorService` on port 9090:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: themisdb
spec:
  containers:
  - name: themisdb
    image: themisdb/themisdb:latest
    livenessProbe:
      httpGet:
        path: /health
        port: 9090
      initialDelaySeconds: 30
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /health
        port: 9090
      initialDelaySeconds: 10
      periodSeconds: 5
```

For cluster-wide health monitoring, use `sharding::HealthCheckSystem` programmatically.

## Architecture

### Backup Automation Flow
```
┌─────────────────┐
│ K8s CronJob     │
│ (Schedule)      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌──────────────┐
│ BackupManager   │────▶│ Local Backup │
│ scheduleBackup()│     └──────┬───────┘
└─────────────────┘            │
         │                     │
         ▼                     ▼
┌─────────────────┐     ┌──────────────┐
│ Cloud Storage   │     │ VolumeSnapshot│
│ (S3/Azure/GCS)  │     │ (K8s CSI)    │
└─────────────────┘     └──────────────┘
```

### Observability Flow
```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│ Components  │────▶│ HealthCheck  │────▶│ /health API │
│ (DB/Network)│     │ Module       │     └──────┬──────┘
└─────────────┘     └──────────────┘            │
                                                 │
                    ┌──────────────┐            │
                    │ Alertmanager │◀───────────┘
                    │ Integration  │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │ Prometheus   │
                    │ Grafana      │
                    └──────────────┘
```

## Missing Features (To Be Implemented)

### Cloud Backup Policy
**Priority:** High  
**Estimated Effort:** 2-3 weeks

**Requirements:**
- AWS S3 SDK integration (boto3/AWS SDK for C++)
- Azure Blob Storage SDK integration
- Google Cloud Storage SDK integration
- Multi-region backup replication
- Backup lifecycle management
- Cost optimization (tiering to Glacier/Archive)

**Implementation Notes:**
- Use cloud provider SDKs for native integration
- Implement retry logic with exponential backoff
- Add progress tracking for large backups
- Support IAM role-based authentication
- Implement backup verification after upload

### Alertmanager Integration
**Priority:** Medium  
**Estimated Effort:** 1-2 weeks

**Requirements:**
- HTTP client for Alertmanager API v2
- Alert rule evaluation engine
- Integration with MetricsCollector
- Webhook support for custom receivers
- Alert templating and formatting
- Silence management

**Implementation Notes:**
- Use libcurl or Boost.Beast for HTTP
- Implement alert batching for efficiency
- Add exponential backoff for failed sends
- Support custom alert labels and annotations
- Integrate with existing logging infrastructure

### Extended Metric Exports
**Priority:** Medium  
**Estimated Effort:** 1 week

**Requirements:**
- OpenTelemetry exporter
- Additional metric types (histograms, summaries)
- Custom metric definitions
- Metric cardinality management
- Remote write protocol support

**Implementation Notes:**
- Extend existing MetricsCollector
- Add metric filtering and aggregation
- Implement push-based metrics
- Support multiple export formats
- Add metric retention policies

## Security Considerations

1. **Cloud Credentials:** Never hardcode credentials. Use K8s secrets or IAM roles.
2. **Backup Encryption:** Always encrypt backups at rest and in transit.
3. **Alert Authentication:** Use tokens or mTLS for Alertmanager.
4. **Health Endpoint:** Expose only on internal network or with authentication.

## Performance Impact

- **HealthCheck:** < 10ms overhead per check (stub implementation)
- **Alertmanager:** Async alert sending, minimal blocking
- **Backup Scheduling:** No runtime overhead (K8s-managed)

## Compatibility

- **Kubernetes:** 1.19+ (VolumeSnapshot API)
- **Cloud Providers:** AWS, Azure, GCP
- **Monitoring:** Prometheus 2.x+, Alertmanager 0.21+
- **Storage Classes:** CSI-compatible storage drivers

## Roadmap

### Phase 1: Current (GAP-008) ✅
- Base structure and interfaces
- Stub implementations
- Example tests

### Phase 2: Cloud Integration (Q2 2026)
- S3 backup implementation
- Azure Blob Storage integration
- GCS integration
- Backup lifecycle management

### Phase 3: Alerting (Q2 2026)
- Alertmanager HTTP client
- Alert rule engine
- Webhook integrations
- PagerDuty/Slack/Email

### Phase 4: Advanced Observability (Q3 2026)
- OpenTelemetry integration
- Distributed tracing
- Extended metrics
- Custom dashboards

## References

- [Kubernetes VolumeSnapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)
- [Prometheus Alertmanager](https://prometheus.io/docs/alerting/latest/alertmanager/)
- [AWS S3 API](https://docs.aws.amazon.com/s3/index.html)
- [Azure Blob Storage](https://docs.microsoft.com/en-us/azure/storage/blobs/)
- [Google Cloud Storage](https://cloud.google.com/storage/docs)

## Contributors

- Implementation: GitHub Copilot
- Review: ThemisDB Team
- Testing: Automated CI/CD

## License

MIT License - See LICENSE file for details
