---
name: Add Prometheus Metrics for Kerberos Authentication
about: Implement Prometheus metrics for monitoring Kerberos authentication performance and security
title: 'Add Prometheus Metrics for Kerberos Authentication'
labels: type:enhancement, area:security, area:observability, priority:P2, effort:small
assignees: ''
---

## 📋 Summary

Implement Prometheus metrics for Kerberos/GSSAPI authentication to enable monitoring, alerting, and performance analysis.

**Parent Feature:** Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support

## 🔍 Problem Statement

### Current State
- ✅ Kerberos authentication implemented
- ✅ Existing Prometheus metrics system
- ❌ No Kerberos-specific metrics exposed
- ❌ Cannot monitor authentication performance
- ❌ Limited visibility into authentication patterns

### Customer Need
Enterprise customers require:
1. **Real-time monitoring** of authentication status
2. **Performance metrics** for SLA compliance
3. **Security alerting** for failed authentication attempts
4. **Capacity planning** based on authentication load

### Business Impact
**Without Metrics:**
- No visibility into authentication performance
- Cannot detect authentication attacks
- No proactive capacity planning
- Difficult to troubleshoot issues

**With Metrics:**
- ✅ Real-time authentication monitoring
- ✅ Proactive security alerting
- ✅ Performance SLA tracking
- ✅ Data-driven capacity planning

## 🎯 Requirements

### Functional Requirements

#### FR-1: Authentication Metrics
- [ ] Total authentication attempts counter
- [ ] Successful authentications counter
- [ ] Failed authentications counter
- [ ] Authentication duration histogram
- [ ] Active authenticated sessions gauge

#### FR-2: Performance Metrics
- [ ] GSSAPI token validation duration
- [ ] Principal-to-role mapping duration
- [ ] Ticket cache hit/miss ratio
- [ ] Security context creation time

#### FR-3: Security Metrics
- [ ] Failed authentication by principal counter
- [ ] Failed authentication by IP counter
- [ ] Ticket expiration events counter
- [ ] KDC connectivity status gauge

#### FR-4: Resource Metrics
- [ ] Memory usage by authentication contexts
- [ ] Keytab reload events counter
- [ ] Credential cache size gauge
- [ ] Active security contexts gauge

### Non-Functional Requirements

#### NFR-1: Performance
- [ ] Metrics collection overhead <0.1ms
- [ ] Async metrics updates
- [ ] Efficient label cardinality

#### NFR-2: Observability
- [ ] Grafana dashboard template
- [ ] Alert rule examples
- [ ] Integration with existing metrics
- [ ] Time series data retention

## 🛠️ Technical Design

### Metrics Definition

```cpp
// File: include/auth/kerberos_metrics.h
namespace themis {
namespace auth {
namespace metrics {

// Authentication counters
extern prometheus::Counter& kerberos_auth_total;
extern prometheus::Counter& kerberos_auth_success_total;
extern prometheus::Counter& kerberos_auth_failure_total;

// Histograms
extern prometheus::Histogram& kerberos_auth_duration_seconds;
extern prometheus::Histogram& kerberos_token_validation_duration_seconds;

// Gauges
extern prometheus::Gauge& kerberos_active_sessions;
extern prometheus::Gauge& kerberos_kdc_available;

// Labels
struct AuthMetricLabels {
    std::string realm;
    std::string service_principal;
    std::string failure_reason;
};

} // namespace metrics
} // namespace auth
} // namespace themis
```

### Metric Names and Labels

```prometheus
# Authentication counters
themisdb_kerberos_auth_total{realm="EXAMPLE.COM", service_principal="themisdb/host"}
themisdb_kerberos_auth_success_total{realm="EXAMPLE.COM"}
themisdb_kerberos_auth_failure_total{realm="EXAMPLE.COM", reason="expired_ticket"}

# Duration histograms (buckets: 0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0 seconds)
themisdb_kerberos_auth_duration_seconds_bucket{le="0.01"}
themisdb_kerberos_auth_duration_seconds_sum
themisdb_kerberos_auth_duration_seconds_count

# Token validation duration
themisdb_kerberos_token_validation_duration_seconds_bucket{le="0.005"}

# Active sessions
themisdb_kerberos_active_sessions{realm="EXAMPLE.COM"}

# KDC status
themisdb_kerberos_kdc_available{kdc="kdc.example.com"}
```

### Implementation

```cpp
// File: src/auth/gssapi_authenticator.cpp
#include "auth/kerberos_metrics.h"

GSSAPIAuthResult GSSAPIAuthenticator::authenticateToken(const std::string& token) {
    auto start_time = std::chrono::steady_clock::now();
    
    // Increment total attempts
    metrics::kerberos_auth_total.Increment();
    
    // ... authentication logic ...
    
    auto duration = std::chrono::steady_clock::now() - start_time;
    metrics::kerberos_auth_duration_seconds.Observe(
        std::chrono::duration<double>(duration).count()
    );
    
    if (result.success) {
        metrics::kerberos_auth_success_total.Increment();
        metrics::kerberos_active_sessions.Increment();
    } else {
        metrics::kerberos_auth_failure_total
            .Labels({{"reason", result.error_message}})
            .Increment();
    }
    
    return result;
}
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "Kerberos Authentication Monitoring",
    "panels": [
      {
        "title": "Authentication Rate",
        "targets": [
          {
            "expr": "rate(themisdb_kerberos_auth_total[5m])"
          }
        ]
      },
      {
        "title": "Success Rate",
        "targets": [
          {
            "expr": "rate(themisdb_kerberos_auth_success_total[5m]) / rate(themisdb_kerberos_auth_total[5m])"
          }
        ]
      },
      {
        "title": "Authentication Duration (p95)",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(themisdb_kerberos_auth_duration_seconds_bucket[5m]))"
          }
        ]
      }
    ]
  }
}
```

### Alert Rules

```yaml
# File: config/prometheus/kerberos_alerts.yml
groups:
  - name: kerberos_authentication
    rules:
      - alert: HighKerberosAuthFailureRate
        expr: |
          (
            rate(themisdb_kerberos_auth_failure_total[5m])
            /
            rate(themisdb_kerberos_auth_total[5m])
          ) > 0.1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High Kerberos authentication failure rate"
          description: "{{ $value | humanizePercentage }} of Kerberos authentications are failing"
      
      - alert: KerberosAuthenticationSlow
        expr: |
          histogram_quantile(0.95,
            rate(themisdb_kerberos_auth_duration_seconds_bucket[5m])
          ) > 0.1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Kerberos authentication is slow"
          description: "95th percentile authentication time is {{ $value }}s"
      
      - alert: KDCUnavailable
        expr: themisdb_kerberos_kdc_available == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "KDC is unavailable"
          description: "Cannot reach Kerberos KDC"
```

## 📝 Implementation Plan

### Phase 1: Metrics Definition (Week 1)
- [ ] **Task 1.1**: Define metrics schema
- [ ] **Task 1.2**: Create metrics registration code
- [ ] **Task 1.3**: Add metrics to `GSSAPIAuthenticator`
- [ ] **Task 1.4**: Add metrics to `AuthMiddleware`

### Phase 2: Grafana Integration (Week 1)
- [ ] **Task 2.1**: Create Grafana dashboard template
- [ ] **Task 2.2**: Create alert rule examples
- [ ] **Task 2.3**: Test dashboard with sample data
- [ ] **Task 2.4**: Document dashboard setup

### Phase 3: Testing & Documentation (Week 2)
- [ ] **Task 3.1**: Unit tests for metrics collection
- [ ] **Task 3.2**: Load testing to verify metrics accuracy
- [ ] **Task 3.3**: Create monitoring guide
- [ ] **Task 3.4**: Document alert thresholds
- [ ] **Task 3.5**: Create troubleshooting playbook

## ✅ Acceptance Criteria

### Functional Acceptance
- [ ] All defined metrics exposed on `/metrics` endpoint
- [ ] Metrics update in real-time
- [ ] Label cardinality is reasonable (<1000 per metric)
- [ ] Metrics persist across restarts
- [ ] Grafana dashboard displays correctly

### Technical Acceptance
- [ ] Unit test coverage >80%
- [ ] Metrics overhead <0.1ms per operation
- [ ] No memory leaks from metrics collection
- [ ] Compatible with Prometheus 2.x+
- [ ] Async metrics updates work correctly

### Documentation Acceptance
- [ ] Metrics reference documentation
- [ ] Grafana dashboard setup guide
- [ ] Alert rules documented
- [ ] Troubleshooting guide created
- [ ] Example PromQL queries provided

## 🧪 Testing Strategy

### Unit Tests
- Metrics registration
- Counter incrementation
- Histogram observations
- Gauge updates
- Label handling

### Integration Tests
- End-to-end metrics collection
- Prometheus scraping
- Grafana dashboard visualization
- Alert rule firing
- High-load stress testing

### Performance Tests
- Metrics collection overhead
- Memory usage with many time series
- Scrape endpoint performance
- Label cardinality impact

## 📚 References

- [Prometheus Best Practices](https://prometheus.io/docs/practices/naming/)
- [Prometheus C++ Client](https://github.com/jupp0r/prometheus-cpp)
- [Existing Metrics](../../src/observability/metrics.cpp)
- [Kerberos Implementation](../../docs/en/security/KERBEROS_AUTHENTICATION.md)

## 🔗 Related Issues

- Parent: Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support
- Related: Issue #[audit-logging-issue] - Kerberos Audit Logging
- Related: Issue #[grpc-interceptor-issue] - gRPC Kerberos Interceptor

## 💬 Notes

**Dependencies:**
- Requires `prometheus-cpp` library
- Requires Kerberos authentication implementation (completed)
- Requires Prometheus server for scraping

**Monitoring Strategy:**
- Real-time alerting for authentication failures
- Capacity planning with authentication trends
- Security incident detection

**Estimated Effort:** 2 weeks (1 developer)

---

**Created:** 2026-01-12 (Future Enhancement from Kerberos Implementation)  
**Status:** 📋 Planned  
**Priority:** MEDIUM  
**Labels:** `type:enhancement`, `area:security`, `area:observability`, `priority:P2`, `effort:small`
