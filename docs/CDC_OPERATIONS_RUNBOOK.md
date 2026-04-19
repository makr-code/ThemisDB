# CDC Operations Runbook

**ThemisDB Change Data Capture Operations Guide**

Version: 1.0  
Last Updated: 2026-02-20

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Quick Reference](#architecture-quick-reference)
3. [Monitoring](#monitoring)
4. [Common Operations](#common-operations)
5. [Troubleshooting](#troubleshooting)
6. [Emergency Procedures](#emergency-procedures)
7. [Maintenance](#maintenance)
8. [Performance Tuning](#performance-tuning)
9. [Capacity Planning](#capacity-planning)
10. [Incident Response](#incident-response)

---

## Overview

### Purpose

This runbook provides operational procedures for managing the ThemisDB CDC (Change Data Capture) module in production environments.

### Scope

- Single-tenant and multi-tenant deployments
- Production, staging, and development environments
- 24/7 operational support

### Prerequisites

- Access to CDC admin API
- Monitoring dashboard access
- RocksDB CLI tools (for advanced operations)
- Log aggregation system access

---

## Architecture Quick Reference

### Components

```
┌─────────────────────────────────────────────┐
│           Application Layer                  │
└──────────────┬──────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────┐
│      TenantBufferManager (Multi-Tenant)     │
│    ┌──────────┬──────────┬──────────┐       │
│    │ Tenant 1 │ Tenant 2 │ Tenant N │       │
│    │  Buffer  │  Buffer  │  Buffer  │       │
│    └──────────┴──────────┴──────────┘       │
└──────────────┬──────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────┐
│            Changefeed                        │
│  ┌──────────────────────────────────────┐   │
│  │  Sequence Generation (Atomic)        │   │
│  │  Event Recording                     │   │
│  │  Retention Policy                    │   │
│  │  Watermark Tracking                  │   │
│  └──────────────────────────────────────┘   │
└──────────────┬──────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────┐
│           RocksDB Storage                    │
└─────────────────────────────────────────────┘
```

### Key Metrics

| Metric | Threshold | Action |
|--------|-----------|--------|
| P95 Latency | > 500ms | Investigate performance |
| Error Rate | > 1% | Check logs, health status |
| Buffer Utilization | > 80% | Increase flush frequency |
| Memory Usage | > 90% | Enable retention, purge old data |
| Throughput | < expected | Check rate limiting, bottlenecks |

---

## Monitoring

### Essential Dashboards

#### 1. Health Dashboard

**Metrics to Display:**
- Overall health status (healthy/degraded/unhealthy)
- Component health (changefeed/buffer/retention)
- Error count and rate
- Buffer utilization percentage

**Alert Rules:**
```
CRITICAL: health_status == "unhealthy"
WARNING: buffer_utilization > 0.8
WARNING: error_rate_per_sec > 10
```

#### 2. Performance Dashboard

**Metrics to Display:**
- P50/P95/P99 latency for recordEvent
- P50/P95/P99 latency for flush
- Throughput (events/sec, bytes/sec)
- Compression ratio

**Alert Rules:**
```
WARNING: p95_latency > 500ms
CRITICAL: p99_latency > 2000ms
WARNING: throughput < 80% of baseline
```

#### 3. Capacity Dashboard

**Metrics to Display:**
- Watermarks (low/high)
- Total events stored
- Storage size
- Buffer size per tenant

**Alert Rules:**
```
WARNING: storage_size > 80% of limit
CRITICAL: storage_size > 95% of limit
WARNING: buffer_count > max_buffered_events * 0.8
```

### Health Check API

**Endpoint:** `CDCAdmin::healthCheck()`

**Usage:**
```cpp
CDCAdmin admin(&changefeed);
HealthStatus health = admin.healthCheck();

if (!health.is_healthy) {
    ALERT("CDC unhealthy: " + health.message);
    
    if (!health.changefeed_healthy) {
        // Changefeed issues
    }
    if (!health.buffer_healthy) {
        // Buffer issues
    }
    if (health.buffer_utilization > 0.9) {
        // Near capacity
    }
}
```

**Frequency:** Every 30 seconds

---

## Common Operations

### 1. Start CDC System

**Procedure:**
```cpp
// Single-tenant
Changefeed::RetentionPolicy retention;
retention.enabled = true;
retention.max_age_hours = std::chrono::hours(168);  // 7 days
retention.max_event_count = 1000000;
retention.cleanup_interval = std::chrono::minutes(60);

Changefeed changefeed(db, nullptr, retention);

ChangefeedBufferConfig config;
config.enable_rate_limiting = true;
config.max_events_per_second = 10000;
config.max_retry_attempts = 3;

ChangefeedBuffer buffer(&changefeed, config);
buffer.start();

// Multi-tenant
TenantBufferManager manager(&changefeed, config);
manager.start();
```

**Verification:**
1. Check health: `admin.healthCheck()` returns healthy
2. Verify watermarks: `changefeed.getWatermarks()` returns valid values
3. Monitor logs for startup messages

### 2. Stop CDC System

**Procedure:**
```cpp
// Graceful shutdown
buffer.stop();  // or manager.stop()

// Flush remaining events
manager.flushAll();  // For multi-tenant

// Verify shutdown
HealthStatus health = admin.healthCheck();
// Should show not running
```

**Verification:**
1. All buffers flushed (check metrics)
2. No pending events
3. Clean shutdown in logs

### 3. Purge Old Data

**When to Use:**
- Storage approaching capacity
- Manual cleanup needed
- Compliance requirements (data deletion)

**Procedure:**
```cpp
CDCAdmin admin(&changefeed);

// Option 1: Purge by timestamp
uint64_t cutoff_ms = /* 7 days ago */;
PurgeResult result = admin.purgeByTimestamp(cutoff_ms);
LOG("Purged {} events in {}ms", result.events_deleted, result.elapsed_time_ms);

// Option 2: Purge by sequence range
auto watermarks = changefeed.getWatermarks();
uint64_t old_watermark = watermarks.low_watermark + 100000;
result = admin.purgeBySequenceRange(watermarks.low_watermark, old_watermark);

// Option 3: Purge specific tenant
result = admin.purgeTenant("tenant_id");
```

**Verification:**
1. Check watermarks after purge
2. Verify storage size reduced
3. Confirm retention policy still active

### 4. Replay Events

**When to Use:**
- Consumer failure recovery
- Reprocessing after bug fix
- Data migration

**Procedure:**
```cpp
CDCAdmin admin(&changefeed);

// Get last processed sequence from consumer
uint64_t last_processed = getLastProcessedSequence();

// Replay from next sequence
auto events = admin.replayFromSequence(last_processed + 1, 1000);

for (const auto& event : events) {
    try {
        reprocessEvent(event);
        updateLastProcessed(event.sequence);
    } catch (const std::exception& e) {
        LOG_ERROR("Replay failed for sequence {}: {}", event.sequence, e.what());
        // Handle failure
    }
}
```

**Verification:**
1. All events replayed successfully
2. Consumer state updated
3. No data gaps

### 5. Configure Tenant Quotas

**When to Use:**
- Prevent noisy neighbors
- Enforce fair usage
- Per-customer SLAs

**Procedure:**
```cpp
TenantBufferManager manager(&changefeed);

TenantConfig config;
config.tenant_id = "premium_tenant";
config.enable_quotas = true;
config.max_events_per_second = 20000;  // Higher quota
config.max_memory_bytes = 100 * 1024 * 1024;  // 100MB
config.max_buffered_events = 20000;

manager.configureTenant(config);
```

**Verification:**
1. Quota enforced (check metrics)
2. No errors for normal usage
3. Quota exceeded errors for over-usage

---

## Troubleshooting

### Issue: High Latency

**Symptoms:**
- P95/P99 latency > 500ms
- Slow event recording

**Diagnosis:**
```cpp
// Check diagnostics
DiagnosticsInfo diag = admin.getDiagnostics();
auto metrics_json = diag.toJson();

// Check:
// 1. Compression latency
// 2. Flush latency
// 3. Throughput
// 4. Buffer utilization
```

**Solutions:**

1. **Buffer Full:**
   - Increase flush frequency
   - Reduce buffer size
   - Enable rate limiting

2. **Slow Compression:**
   - Disable compression
   - Use faster compression level
   - Check CPU utilization

3. **RocksDB Bottleneck:**
   - Check disk I/O
   - Tune RocksDB settings
   - Add more storage

### Issue: Buffer Overflow

**Symptoms:**
- Buffer utilization > 90%
- `BUFFER_OVERFLOW` errors
- Events being dropped

**Diagnosis:**
```cpp
HealthStatus health = admin.healthCheck();
if (health.buffer_utilization > 0.9) {
    // Buffer nearly full
}

// Check metrics
auto metrics = buffer.getMetrics();
uint64_t buffered = metrics.events_recorded - metrics.events_flushed;
```

**Solutions:**

1. **Immediate:**
   ```cpp
   // Manual flush
   buffer.flushBuffer();
   // or
   manager.flushAll();
   ```

2. **Short-term:**
   - Increase `max_buffer_size`
   - Increase `flush_interval_ms`
   - Enable rate limiting

3. **Long-term:**
   - Scale consumers
   - Optimize flush performance
   - Add more capacity

### Issue: High Error Rate

**Symptoms:**
- Error count increasing
- `errors` metric > 100/minute

**Diagnosis:**
```cpp
DiagnosticsInfo diag = admin.getDiagnostics();
if (diag.health.error_count > threshold) {
    // Check logs for specific errors
    // Check error types via structured errors
}
```

**Solutions:**

1. **Compression Errors:**
   - Check payload size
   - Disable compression
   - Update compression library

2. **RocksDB Errors:**
   - Check disk space
   - Check file permissions
   - Verify RocksDB health

3. **Sequence Errors:**
   - Check for sequence gaps
   - Verify atomic operations
   - Review recent code changes

### Issue: Memory Leak

**Symptoms:**
- Memory usage increasing
- OOM errors
- System slowdown

**Diagnosis:**
```cpp
// Check buffer size
auto stats = manager.getTenantStats("tenant_id");
if (stats) {
    LOG("Buffer size: {}, Memory: {}%", 
        stats->current_buffer_size, 
        stats->memory_usage_percent);
}

// Check retention
auto watermarks = changefeed.getWatermarks();
uint64_t total_events = watermarks.high_watermark - watermarks.low_watermark;
if (total_events > expected) {
    // Retention not working
}
```

**Solutions:**

1. **Enable Retention:**
   ```cpp
   RetentionPolicy policy;
   policy.enabled = true;
   policy.max_age_hours = std::chrono::hours(24);  // Aggressive
   policy.cleanup_interval = std::chrono::minutes(15);  // Frequent
   ```

2. **Manual Purge:**
   ```cpp
   admin.purgeByTimestamp(cutoff);
   ```

3. **Restart (Last Resort):**
   - Stop CDC gracefully
   - Flush all buffers
   - Purge old data
   - Restart

### Issue: Retention Not Working

**Symptoms:**
- Events not being deleted
- Storage growing unbounded
- Old events still present

**Diagnosis:**
```cpp
auto watermarks = changefeed.getWatermarks();
uint64_t age_hours = (now_ms - watermarks.oldest_timestamp_ms) / 3600000;

if (age_hours > expected_max_age) {
    // Retention not cleaning up
}
```

**Solutions:**

1. **Manual Cleanup:**
   ```cpp
   changefeed.applyRetentionPolicy();
   ```

2. **Restart Retention:**
   ```cpp
   changefeed.stopRetentionCleanup();
   changefeed.startRetentionCleanup();
   ```

3. **Check Configuration:**
   - Verify `retention.enabled = true`
   - Verify `cleanup_interval` is reasonable
   - Check logs for retention errors

---

## Emergency Procedures

### Emergency: Complete System Failure

**Situation:** CDC completely unresponsive

**Procedure:**

1. **Assess Impact:**
   - Is database accessible?
   - Are consumers affected?
   - Is data being lost?

2. **Immediate Actions:**
   ```cpp
   // Try health check
   try {
       HealthStatus health = admin.healthCheck();
       if (!health.is_healthy) {
           LOG_CRITICAL("CDC unhealthy: {}", health.message);
       }
   } catch (const std::exception& e) {
       LOG_CRITICAL("Health check failed: {}", e.what());
   }
   ```

3. **Recovery:**
   ```cpp
   // Stop CDC
   buffer.stop();
   
   // Export diagnostics for analysis
   DiagnosticsInfo diag = admin.getDiagnostics();
   saveDiagnostics(diag.toJson());
   
   // Restart with safe config
   ChangefeedBufferConfig safe_config;
   safe_config.max_buffer_size = 1000;  // Small
   safe_config.flush_interval_ms = 1000;  // Frequent
   safe_config.enable_compression = false;  // Disable
   
   ChangefeedBuffer new_buffer(&changefeed, safe_config);
   new_buffer.start();
   ```

4. **Post-Recovery:**
   - Verify health
   - Replay missed events
   - Gradually restore normal config

### Emergency: Data Loss Detected

**Situation:** Sequence gaps or missing events

**Procedure:**

1. **Confirm Loss:**
   ```cpp
   auto watermarks = changefeed.getWatermarks();
   auto events = admin.replayFromSequence(watermarks.low_watermark);
   
   // Check for gaps
   uint64_t expected = watermarks.low_watermark;
   for (const auto& event : events) {
       if (event.sequence != expected) {
           LOG_CRITICAL("Gap detected: expected {}, got {}", expected, event.sequence);
       }
       expected = event.sequence + 1;
   }
   ```

2. **Stop Writes:**
   - Pause producers
   - Stop accepting new events

3. **Assess Damage:**
   - Count missing events
   - Identify affected tenants
   - Check if recoverable

4. **Recovery Options:**
   - **If recent:** Replay from backup/replica
   - **If old:** Accept loss, document

5. **Preventive Actions:**
   - Enable backups
   - Add monitoring for gaps
   - Review retention settings

### Emergency: Storage Full

**Situation:** Disk space exhausted

**Procedure:**

1. **Immediate:**
   ```cpp
   // Aggressive purge
   CDCAdmin admin(&changefeed);
   
   auto watermarks = changefeed.getWatermarks();
   uint64_t keep_recent = 10000;  // Keep only recent
   uint64_t purge_up_to = watermarks.high_watermark - keep_recent;
   
   PurgeResult result = admin.purgeBySequenceRange(
       watermarks.low_watermark, purge_up_to);
   
   LOG("Emergency purge: {} events deleted", result.events_deleted);
   ```

2. **Verify Space:**
   - Check disk usage
   - Verify events deleted

3. **Resume Operations:**
   - Restart CDC if stopped
   - Monitor closely

4. **Long-term Fix:**
   - Add more storage
   - Reduce retention period
   - Enable compression

---

## Maintenance

### Weekly Tasks

1. **Review Metrics:**
   - Check average latencies
   - Review error counts
   - Verify retention working

2. **Check Capacity:**
   - Review storage usage
   - Check buffer utilization trends
   - Plan for growth

3. **Log Review:**
   - Search for warnings/errors
   - Identify patterns
   - Update alerts if needed

### Monthly Tasks

1. **Performance Review:**
   - Analyze P95/P99 trends
   - Identify degradation
   - Plan optimizations

2. **Capacity Planning:**
   - Project storage needs
   - Plan scaling
   - Update quotas

3. **Configuration Review:**
   - Review retention settings
   - Update quotas
   - Optimize thresholds

4. **Test Recovery Procedures:**
   - Test purge operations
   - Test replay procedures
   - Verify backups

### Quarterly Tasks

1. **Full System Review:**
   - End-to-end testing
   - Chaos testing
   - Load testing

2. **Documentation Update:**
   - Update runbook
   - Update architecture docs
   - Document lessons learned

3. **Training:**
   - Train new team members
   - Review procedures
   - Update on-call guide

---

## Performance Tuning

### Optimization Checklist

**For Low Latency:**
- [ ] Disable compression
- [ ] Reduce buffer size
- [ ] Increase flush frequency
- [ ] Disable rate limiting
- [ ] Tune RocksDB

**For High Throughput:**
- [ ] Enable compression
- [ ] Increase buffer size
- [ ] Batch operations
- [ ] Enable rate limiting
- [ ] Scale horizontally

**For Low Resource Usage:**
- [ ] Enable compression
- [ ] Aggressive retention
- [ ] Disable metrics collection
- [ ] Reduce buffer sizes
- [ ] Lower flush frequency

### Configuration Examples

**Development:**
```cpp
ChangefeedBufferConfig dev_config;
dev_config.max_buffer_size = 100;
dev_config.flush_interval_ms = 5000;
dev_config.enable_compression = false;
dev_config.enable_rate_limiting = false;

RetentionPolicy dev_retention;
dev_retention.enabled = true;
dev_retention.max_age_hours = std::chrono::hours(24);  // 1 day
dev_retention.max_event_count = 10000;
```

**Production:**
```cpp
ChangefeedBufferConfig prod_config;
prod_config.max_buffer_size = 5000;
prod_config.flush_interval_ms = 1000;
prod_config.enable_compression = true;
prod_config.enable_rate_limiting = true;
prod_config.max_events_per_second = 10000;
prod_config.max_retry_attempts = 3;
prod_config.exponential_backoff = true;

RetentionPolicy prod_retention;
prod_retention.enabled = true;
prod_retention.max_age_hours = std::chrono::hours(168);  // 7 days
prod_retention.max_event_count = 1000000;
prod_retention.max_size_bytes = 100ULL << 30;  // 100GB
prod_retention.cleanup_interval = std::chrono::minutes(60);
```

---

## Capacity Planning

### Growth Projections

**Calculate Required Capacity:**

```
Events per day = avg_events_per_second * 86400
Storage per day = events_per_day * avg_event_size * (1 - compression_ratio)
Storage for retention period = storage_per_day * retention_days
```

**Example:**
```
10,000 events/sec
200 bytes per event (average)
0.5 compression ratio
7 days retention

= 10,000 * 86400 * 200 * 0.5 * 7
= 60.48 TB for 7 days
```

### Scaling Guidelines

| Throughput | Storage | Config |
|------------|---------|--------|
| < 1k eps | < 10GB | Small (single instance) |
| 1k-10k eps | 10-100GB | Medium (single with tuning) |
| 10k-50k eps | 100GB-1TB | Large (consider sharding) |
| > 50k eps | > 1TB | Extra Large (multi-instance) |

---

## Incident Response

### Severity Levels

**P0 (Critical):**
- Complete system failure
- Data loss
- < 1 hour to resolve

**P1 (High):**
- Degraded performance (P99 > 2s)
- High error rate (> 5%)
- < 4 hours to resolve

**P2 (Medium):**
- Minor performance issues
- Low error rate (1-5%)
- < 24 hours to resolve

**P3 (Low):**
- Cosmetic issues
- No user impact
- < 1 week to resolve

### Incident Response Flow

1. **Detect:** Monitoring alerts
2. **Assess:** Check health, diagnostics
3. **Communicate:** Update stakeholders
4. **Mitigate:** Apply emergency procedures
5. **Resolve:** Fix root cause
6. **Document:** Write postmortem
7. **Improve:** Update runbook, add alerts

### Postmortem Template

```markdown
# CDC Incident Postmortem

**Date:** YYYY-MM-DD
**Severity:** P0/P1/P2/P3
**Duration:** X hours
**Impact:** Description

## Timeline

- HH:MM - Event 1
- HH:MM - Event 2

## Root Cause

Description of the root cause.

## Resolution

What was done to resolve.

## Lessons Learned

- Lesson 1
- Lesson 2

## Action Items

- [ ] Action 1
- [ ] Action 2
```

---

## Appendix

### Useful Commands

```cpp
// Health check
CDCAdmin admin(&changefeed);
auto health = admin.healthCheck();
LOG("Health: {}", health.toJson().dump(2));

// Get diagnostics
auto diag = admin.getDiagnostics();
LOG("Diagnostics: {}", diag.toJson().dump(2));

// Get watermarks
auto wm = changefeed.getWatermarks();
LOG("Watermarks: low={}, high={}", wm.low_watermark, wm.high_watermark);

// Get metrics
const auto& metrics = buffer.getMetrics();
LOG("P95: {}μs, Throughput: {} eps", 
    metrics.record_event_latency.p95(),
    metrics.throughput.eventsPerSecond());
```

### Contact Information

- **On-call:** [On-call rotation]
- **Escalation:** [Manager contact]
- **Slack:** #cdc-ops
- **Email:** cdc-team@company.com

### Related Documentation

- [CDC Architecture](de/roadmap/cdc_roadmap.md)
- [API Reference](../docs/CDC_API_REFERENCE.md)
- [Configuration Guide](../docs/CDC_CONFIG_GUIDE.md)

---

**End of Runbook**
