# Protecting Knowledge Graphs Against AI Data Theft

**Date:** January 7, 2026  
**Version:** 1.0.0  
**Category:** Security, Knowledge Graph Protection

---

## 📋 Table of Contents

- [Overview](#overview)
- [Threat Scenarios](#threat-scenarios)
- [Protection Mechanisms](#protection-mechanisms)
- [ThemisDB Implementation](#themisdb-implementation)
- [Configuration](#configuration)
- [Best Practices](#best-practices)

---

## Overview

### Background

Recent research shows that knowledge graphs and structured data are increasingly targeted for data theft, particularly for training AI models. Attackers can:

1. **Exfiltrate Knowledge Graphs** - Systematic extraction of graph structures and content
2. **Extract Embeddings** - Theft of pre-computed vector embeddings
3. **Training Data Extraction** - Reconstruction of training data from models
4. **Model Inversion** - Deriving sensitive information from model behavior

### Research Findings

Latest studies (January 2026) demonstrate methods to protect knowledge graphs:

- **Watermarking**: Embedding imperceptible marks in graph structures
- **Data Poisoning Defense**: Targeted perturbations that make stolen data unusable for AI training
- **Fingerprinting**: Identification of stolen data in trained models
- **Access Pattern Analysis**: Detection of systematic data exfiltration

---

## Threat Scenarios

### 1. Systematic Graph Exfiltration

**Attack Scenario:**
```
Attacker performs systematic traversal queries:
- BFS/DFS over entire graph
- Incremental extraction of all nodes and edges
- Export of attributes and relationships
```

**Risks for ThemisDB:**
- GraphIndexManager allows traversal operations
- PropertyGraph contains structured relationships
- GNN embeddings reveal graph topology

### 2. Embedding Theft

**Attack Scenario:**
```
Theft of pre-computed vector embeddings:
- Access to VectorIndexManager
- Download all embedding vectors
- Use for own models
```

**Risks for ThemisDB:**
- VectorIndexManager stores embeddings persistently
- HNSW indices are exportable
- No native watermarking support

### 3. Training Data Reconstruction

**Attack Scenario:**
```
Reconstruction of training data:
- Query-based attacks on LLM integration
- Model inversion attacks
- Membership inference attacks
```

**Risks for ThemisDB:**
- Embedded LLM (llama.cpp) could leak training data
- Response cache could reveal sensitive patterns
- No differential privacy mechanisms

### 4. Temporal Data Mining

**Attack Scenario:**
```
Analysis of temporal patterns:
- TemporalGraph shows evolution over time
- Audit logs reveal access patterns
- Change Data Capture (CDC) shows modifications
```

**Risks for ThemisDB:**
- TemporalGraph stores history
- CDC/Changefeed enables timeline reconstruction

---

## Protection Mechanisms

### 1. Graph Watermarking

**Concept:**
Embedding harmless but identifiable patterns in graph structure:

```cpp
// Pseudo-code for Graph Watermarking
class GraphWatermarkingManager {
public:
    // Embed watermark in graph structure
    Status embedWatermark(
        std::string_view graph_id,
        const WatermarkConfig& config,
        const std::vector<uint8_t>& watermark_bits
    );
    
    // Detect watermark in extracted graph
    std::pair<Status, bool> detectWatermark(
        std::string_view graph_id,
        const WatermarkConfig& config
    );
    
private:
    // Add dummy edges with specific patterns
    Status addDummyEdges(const WatermarkConfig& config);
    
    // Modify edge weights imperceptibly
    Status perturbWeights(const WatermarkConfig& config);
};
```

**Properties:**
- Harmless for graph algorithms
- Robust against modifications
- Provable in stolen copies

### 2. Vector Embedding Protection

**Concept:**
Protecting embeddings through controlled perturbations:

```cpp
// Pseudo-code for Embedding Protection
class EmbeddingProtectionManager {
public:
    // Add imperceptible noise to embeddings
    Status protectEmbeddings(
        std::string_view object_name,
        float noise_magnitude = 0.01f,
        uint64_t seed = 0
    );
    
    // Verify embedding integrity
    std::pair<Status, bool> verifyEmbedding(
        const std::vector<float>& embedding,
        const std::string& fingerprint
    );
    
private:
    // Add deterministic noise based on secret key
    std::vector<float> addProtectiveNoise(
        const std::vector<float>& original,
        const ProtectionKey& key
    );
};
```

**Properties:**
- Minimal impact on search quality
- Deterministic based on secret key
- Provable in stolen embeddings

### 3. Access Pattern Anomaly Detection

**Concept:**
Detection of suspicious access patterns:

```cpp
// Pseudo-code for Anomaly Detection
class GraphAccessMonitor {
public:
    // Track graph access patterns
    Status recordAccess(
        const std::string& user_id,
        const std::string& operation,
        const std::vector<std::string>& accessed_nodes
    );
    
    // Detect suspicious patterns
    std::pair<Status, std::vector<Anomaly>> detectAnomalies(
        std::string_view user_id,
        const DetectionConfig& config
    );
    
private:
    // Suspicious patterns:
    // - Systematic traversal
    // - High-frequency queries
    // - Broad scope access
    bool isSuspiciousPattern(const AccessPattern& pattern);
};
```

**Detection Criteria:**
- Unusually high traversal depth
- Systematic enumeration of nodes
- Large amounts of exported data
- Abnormal timing

### 4. Rate Limiting for Graph Operations

**Concept:**
Restricting number and speed of graph queries:

```cpp
// Pseudo-code for Graph Rate Limiting
class GraphRateLimiter {
public:
    struct Limits {
        size_t max_nodes_per_query = 1000;
        size_t max_edges_per_query = 10000;
        size_t max_depth = 5;
        size_t queries_per_minute = 100;
    };
    
    // Check if operation is allowed
    std::pair<Status, bool> checkLimit(
        const std::string& user_id,
        const GraphOperation& op
    );
    
    // Update usage counters
    Status recordUsage(
        const std::string& user_id,
        const GraphOperation& op
    );
};
```

**Limits:**
- Maximum traversal depth
- Maximum number of returned nodes
- Query frequency per user
- Data volume per time window

---

## ThemisDB Implementation

### Current Security Features

ThemisDB already has robust security mechanisms:

#### ✅ Encryption
- **Field-Level Encryption** (AES-256-GCM) - `src/security/field_encryption.cpp`
- **Column Encryption** - Selective encryption of sensitive fields
- **Vector Encryption** - Encryption of embeddings

#### ✅ Access Control
- **RBAC** (Role-Based Access Control) - Ranger-compatible
- **ABAC** (Attribute-Based Access Control)
- **mTLS** - Mutual TLS for client authentication

#### ✅ Audit & Monitoring
- **Audit Logging** - 65+ event types, hash chain, PKI signatures
- **SIEM Integration** - Syslog RFC 5424, Splunk HEC
- **Prometheus Metrics** - Real-time monitoring

#### ✅ Rate Limiting
- **Token Bucket Algorithm** - Default 100 req/min
- **Per-IP and Per-User Limits**
- **Configurable Thresholds**

### Missing Features

#### ❌ Graph-Specific Protection
- No native graph watermarking
- No embedding fingerprinting
- No specialized graph anomaly detection
- No graph-specific rate limits

#### ❌ Differential Privacy
- No DP mechanisms for aggregations
- No noise injection for queries
- No privacy budget management

#### ❌ Advanced Anomaly Detection
- No ML-based anomaly detection
- No behavioral analysis
- No automatic threat classification

---

## Recommended Implementation

### Phase 1: Basic Protection Measures (Immediate)

#### 1.1 Enhanced Audit Logs for Graph Operations

**Files:** `src/index/graph_index.cpp`, `src/index/vector_index.cpp`

Additional audit events:
```cpp
enum class GraphAuditEvent {
    GRAPH_TRAVERSAL,          // BFS/DFS operations
    BULK_NODE_ACCESS,         // Large-scale node queries
    BULK_EDGE_ACCESS,         // Large-scale edge queries
    EMBEDDING_EXPORT,         // Vector embedding downloads
    GRAPH_EXPORT,             // Full graph exports
    TEMPORAL_QUERY            // Historical graph queries
};
```

#### 1.2 Graph-Specific Rate Limits

**File:** `src/security/rate_limiter.cpp` (extend)

New configuration options:
```yaml
graph_limits:
  max_traversal_depth: 5
  max_nodes_per_query: 1000
  max_edges_per_query: 10000
  max_embeddings_per_query: 500
  graph_queries_per_minute: 50
  bulk_export_enabled: false  # Disabled by default
```

#### 1.3 Access Pattern Monitoring

**New File:** `include/security/graph_access_monitor.h`

Tracking metrics:
- Number of traversal operations per user
- Access patterns (temporal, spatial)
- Exported data volumes
- Suspicious query sequences

### Phase 2: Watermarking & Fingerprinting (Medium-term)

#### 2.1 Graph Watermarking

**New Files:**
- `include/security/graph_watermark.h`
- `src/security/graph_watermark.cpp`

Features:
- Deterministic watermark embedding
- Robustness against modifications
- Detection mechanisms

#### 2.2 Embedding Fingerprinting

**New Files:**
- `include/security/embedding_fingerprint.h`
- `src/security/embedding_fingerprint.cpp`

Features:
- Imperceptible noise injection
- Fingerprint verification
- Secret key management

### Phase 3: Advanced Protection (Long-term)

#### 3.1 Differential Privacy

**New Files:**
- `include/privacy/differential_privacy.h`
- `src/privacy/differential_privacy.cpp`

Features:
- ε-differential privacy for aggregations
- Privacy budget management
- Laplace/Gaussian noise mechanisms

#### 3.2 ML-based Anomaly Detection

**New Files:**
- `include/security/ml_anomaly_detector.h`
- `src/security/ml_anomaly_detector.cpp`

Features:
- Online learning for user profiles
- Behavioral anomaly detection
- Automatic alerting

---

## Configuration

### Recommended Security Configuration

**File:** `config/config.yaml` (extend)

```yaml
security:
  # Existing configuration
  encryption:
    enabled: true
    field_encryption: true
    
  rbac:
    enabled: true
    
  audit:
    enabled: true
    events: all
    
  # New graph protection configuration
  graph_protection:
    # Access Monitoring
    access_monitoring:
      enabled: true
      track_traversals: true
      track_exports: true
      anomaly_detection: true
      
    # Rate Limiting
    rate_limits:
      max_traversal_depth: 5
      max_nodes_per_query: 1000
      max_edges_per_query: 10000
      max_embeddings_per_query: 500
      queries_per_minute: 50
      
    # Export Controls
    export_controls:
      bulk_export_enabled: false
      require_approval: true
      max_export_size_mb: 100
      
    # Watermarking (Phase 2)
    watermarking:
      enabled: false  # Optional, after implementation
      strength: medium
      key_rotation_days: 90
      
    # Differential Privacy (Phase 3)
    differential_privacy:
      enabled: false  # Optional, after implementation
      epsilon: 1.0
      delta: 1e-5
```

---

## Best Practices

### 1. Defense in Depth

Combine multiple protection layers:

```
┌─────────────────────────────────────────────┐
│ Layer 1: Authentication (mTLS, JWT)        │
├─────────────────────────────────────────────┤
│ Layer 2: Authorization (RBAC, ABAC)        │
├─────────────────────────────────────────────┤
│ Layer 3: Rate Limiting (Query Throttling)  │
├─────────────────────────────────────────────┤
│ Layer 4: Access Monitoring (Audit Logs)    │
├─────────────────────────────────────────────┤
│ Layer 5: Encryption (Field/Column)         │
├─────────────────────────────────────────────┤
│ Layer 6: Watermarking (Optional)           │
└─────────────────────────────────────────────┘
```

### 2. Principle of Least Privilege

Grant minimal required permissions:

```yaml
# Example: Restrictive graph permissions
roles:
  - name: data_analyst
    permissions:
      graph:
        read: true
        traverse: true
        max_depth: 3       # Limited
        export: false      # No export
        
  - name: data_scientist
    permissions:
      graph:
        read: true
        traverse: true
        max_depth: 5
        export: true
        export_approval_required: true  # Approval workflow
```

### 3. Continuous Monitoring

Monitor suspicious activities:

```python
# Example: Prometheus alert for suspicious graph access
groups:
  - name: graph_security
    rules:
      - alert: SuspiciousGraphTraversal
        expr: |
          rate(themis_graph_traversal_depth_bucket{le="10"}[5m]) > 10
        annotations:
          summary: "Unusual deep graph traversal detected"
          
      - alert: BulkGraphExport
        expr: |
          rate(themis_graph_nodes_exported[5m]) > 1000
        annotations:
          summary: "Large-scale graph export detected"
```

### 4. Regular Security Audits

Conduct regular audits:

- **Weekly:** Review audit logs for anomalies
- **Monthly:** Analyze access patterns
- **Quarterly:** Penetration tests
- **Annually:** Comprehensive security assessments

### 5. Incident Response Plan

Prepare for security incidents:

1. **Detection:** Automatic alerting on anomalies
2. **Analysis:** Investigation of incident
3. **Containment:** Immediate blocking of affected accounts
4. **Eradication:** Removal of backdoors/malware
5. **Recovery:** Restoration of normal operations
6. **Lessons Learned:** Post-mortem analysis

---

## Summary

### Current Situation

ThemisDB has **solid baseline security**:
- ✅ Encryption (Field/Column/Vector)
- ✅ RBAC/ABAC
- ✅ Audit Logging
- ✅ Rate Limiting

### Recommended Improvements

**Phase 1 (Immediate):**
1. Enhanced audit logs for graph operations
2. Graph-specific rate limits
3. Access pattern monitoring
4. Export controls

**Phase 2 (Medium-term):**
1. Graph watermarking
2. Embedding fingerprinting
3. Enhanced anomaly detection

**Phase 3 (Long-term):**
1. Differential privacy
2. ML-based anomaly detection
3. Advanced threat intelligence

### Priority

**High (Immediate):**
- Configure graph-specific rate limits
- Enable audit logs for graph operations
- Set up monitoring alerts

**Medium (3-6 months):**
- Implement access pattern monitoring
- Refine export controls

**Low (6-12 months):**
- Evaluate watermarking/fingerprinting
- Differential privacy for specific use cases

---

## References

### Research Literature

1. **Graph Watermarking:**
   - "Watermarking Techniques for Knowledge Graphs" (2025)
   - "Robust Graph Structure Fingerprinting" (2024)

2. **Embedding Protection:**
   - "Protecting Vector Embeddings from Theft" (2025)
   - "Imperceptible Perturbations for Embedding Security" (2024)

3. **Training Data Protection:**
   - "Making Stolen Data Unusable for AI Training" (2026)
   - "Data Poisoning Defenses for Knowledge Graphs" (2025)

### ThemisDB Documentation

- [Security Overview](security_overview.md)
- [Encryption Strategy](security_encryption_strategy.md)
- [Audit Logging](../../de/development/auditlog.md)
- [RBAC Implementation](security_implementation.md)

---

**Author:** ThemisDB Security Team  
**Last Updated:** April 2026  
**Review Status:** Draft - Implementation pending
