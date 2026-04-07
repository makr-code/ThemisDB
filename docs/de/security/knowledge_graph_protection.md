# Schutz von Wissensgraphen gegen KI-Datendiebstahl

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Security, Knowledge Graph Protection

---

## 📋 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Bedrohungsszenarien](#bedrohungsszenarien)
- [Schutzmechanismen](#schutzmechanismen)
- [ThemisDB-Implementierung](#themisdb-implementierung)
- [Konfiguration](#konfiguration)
- [Best Practices](#best-practices)

---

## Übersicht

### Hintergrund

Aktuelle Forschung zeigt, dass Wissensgraphen und strukturierte Daten zunehmend Ziel von Datendiebstahl werden, insbesondere für das Training von KI-Modellen. Angreifer können:

1. **Wissensgraphen exfiltrieren** - Systematisches Abgreifen von Graphstrukturen und -inhalten
2. **Embeddings extrahieren** - Diebstahl von vorberechneten Vektor-Embeddings
3. **Training Data Extraction** - Rekonstruktion von Trainingsdaten aus Modellen
4. **Model Inversion** - Ableitung sensibler Informationen aus Modellverhalten

### Forschungsergebnisse

Neueste Studien (Januar 2026) zeigen Methoden zum Schutz von Wissensgraphen:

- **Watermarking**: Einbettung nicht-wahrnehmbarer Markierungen in Graphstrukturen
- **Data Poisoning Defense**: Gezielte Perturbationen, die gestohlene Daten für KI-Training unbrauchbar machen
- **Fingerprinting**: Identifikation gestohlener Daten in trainierten Modellen
- **Access Pattern Analysis**: Erkennung systematischer Datenexfiltration

---

## Bedrohungsszenarien

### 1. Systematische Graphexfiltration

**Angriffsszenario:**
```
Angreifer führt systematische Traversal-Queries durch:
- BFS/DFS über gesamten Graphen
- Schrittweise Extraktion aller Knoten und Kanten
- Export von Attributen und Beziehungen
```

**Risiken für ThemisDB:**
- GraphIndexManager erlaubt Traversal-Operationen
- PropertyGraph enthält strukturierte Beziehungen
- GNN-Embeddings offenbaren Graphtopologie

### 2. Embedding-Diebstahl

**Angriffsszenario:**
```
Diebstahl vorberechneter Vektor-Embeddings:
- Zugriff auf VectorIndexManager
- Download aller Embedding-Vektoren
- Verwendung für eigene Modelle
```

**Risiken für ThemisDB:**
- VectorIndexManager speichert Embeddings persistent
- HNSW-Indizes sind exportierbar
- Keine native Watermarking-Unterstützung

### 3. Training Data Reconstruction

**Angriffsszenario:**
```
Rekonstruktion von Trainingsdaten:
- Abfrage-basierte Angriffe auf LLM-Integration
- Model Inversion Attacks
- Membership Inference Attacks
```

**Risiken für ThemisDB:**
- Embedded LLM (llama.cpp) könnte Trainingsdaten preisgeben
- Response Cache könnte sensitive Patterns offenlegen
- Keine differenzielle Privacy-Mechanismen

### 4. Temporal Data Mining

**Angriffsszenario:**
```
Analyse zeitlicher Muster:
- TemporalGraph zeigt Entwicklung über Zeit
- Audit Logs offenbaren Zugriffsmuster
- Change Data Capture (CDC) zeigt Änderungen
```

**Risiken für ThemisDB:**
- TemporalGraph speichert Historie
- CDC/Changefeed ermöglicht Timeline-Rekonstruktion

---

## Schutzmechanismen

### 1. Graph Watermarking

**Konzept:**
Einbettung unschädlicher, aber identifizierbarer Muster in die Graphstruktur:

```cpp
// Pseudo-Code für Graph Watermarking
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

**Eigenschaften:**
- Unschädlich für Graphalgorithmen
- Robust gegen Modifikationen
- Nachweisbar in gestohlenen Kopien

### 2. Vector Embedding Protection

**Konzept:**
Schutz von Embeddings durch kontrollierte Perturbationen:

```cpp
// Pseudo-Code für Embedding Protection
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

**Eigenschaften:**
- Minimaler Einfluss auf Suchqualität
- Deterministisch basierend auf Secret Key
- Nachweisbar in gestohlenen Embeddings

### 3. Access Pattern Anomaly Detection

**Konzept:**
Erkennung verdächtiger Zugriffsmuster:

```cpp
// Pseudo-Code für Anomaly Detection
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

**Detektionskriterien:**
- Ungewöhnlich hohe Traversal-Tiefe
- Systematische Enumeration von Knoten
- Große Mengen exportierter Daten
- Abnormale Zeitpunkte

### 4. Rate Limiting für Graph-Operationen

**Konzept:**
Beschränkung der Anzahl und Geschwindigkeit von Graph-Queries:

```cpp
// Pseudo-Code für Graph Rate Limiting
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
- Maximale Traversal-Tiefe
- Maximale Anzahl zurückgegebener Knoten
- Query-Frequenz pro Nutzer
- Datenvolumen pro Zeitfenster

---

## ThemisDB-Implementierung

### Aktuelle Sicherheitsfunktionen

ThemisDB verfügt bereits über robuste Sicherheitsmechanismen:

#### ✅ Verschlüsselung
- **Field-Level Encryption** (AES-256-GCM) - `src/security/field_encryption.cpp`
- **Column Encryption** - Selektive Verschlüsselung sensibler Felder
- **Vector Encryption** - Verschlüsselung von Embeddings

#### ✅ Zugriffskontrolle
- **RBAC** (Role-Based Access Control) - Ranger-kompatibel
- **ABAC** (Attribute-Based Access Control)
- **mTLS** - Mutual TLS für Client-Authentifizierung

#### ✅ Audit & Monitoring
- **Audit Logging** - 65+ Ereignistypen, Hash-Chain, PKI-Signaturen
- **SIEM Integration** - Syslog RFC 5424, Splunk HEC
- **Prometheus Metrics** - Real-time Monitoring

#### ✅ Rate Limiting
- **Token Bucket Algorithm** - Standardmäßig 100 req/min
- **Per-IP und Per-User Limits**
- **Konfigurierbare Schwellwerte**

### Fehlende Funktionen

#### ❌ Graph-spezifischer Schutz
- Kein natives Graph Watermarking
- Keine Embedding-Fingerprinting
- Keine spezielle Graph-Anomalieerkennung
- Keine Graph-spezifischen Rate Limits

#### ❌ Differenzielle Privacy
- Keine DP-Mechanismen für Aggregationen
- Keine Noise-Injection für Queries
- Keine Privacy-Budget-Verwaltung

#### ❌ Advanced Anomaly Detection
- Keine ML-basierte Anomalieerkennung
- Keine verhaltensbasierte Analyse
- Keine automatische Bedrohungsklassifikation

---

## Empfohlene Implementierung

### Phase 1: Basis-Schutzmaßnahmen (Sofort)

#### 1.1 Erweiterte Audit-Logs für Graph-Operationen

**Datei:** `src/index/graph_index.cpp`, `src/index/vector_index.cpp`

Zusätzliche Audit-Events:
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

#### 1.2 Graph-spezifische Rate Limits

**Datei:** `src/security/rate_limiter.cpp` (erweitern)

Neue Konfigurationsoptionen:
```yaml
graph_limits:
  max_traversal_depth: 5
  max_nodes_per_query: 1000
  max_edges_per_query: 10000
  max_embeddings_per_query: 500
  graph_queries_per_minute: 50
  bulk_export_enabled: false  # Deaktiviert standardmäßig
```

#### 1.3 Access Pattern Monitoring

**Neue Datei:** `include/security/graph_access_monitor.h`

Tracking-Metriken:
- Anzahl Traversal-Operationen pro Nutzer
- Zugriffsmuster (zeitlich, räumlich)
- Exportierte Datenmengen
- Verdächtige Query-Sequenzen

### Phase 2: Watermarking & Fingerprinting (Mittelfristig)

#### 2.1 Graph Watermarking

**Neue Dateien:**
- `include/security/graph_watermark.h`
- `src/security/graph_watermark.cpp`

Features:
- Deterministisches Watermark-Embedding
- Robustheit gegen Modifikationen
- Nachweismechanismen

#### 2.2 Embedding Fingerprinting

**Neue Dateien:**
- `include/security/embedding_fingerprint.h`
- `src/security/embedding_fingerprint.cpp`

Features:
- Imperceptible Noise-Injection
- Fingerprint-Verifikation
- Secret Key Management

### Phase 3: Advanced Protection (Langfristig)

#### 3.1 Differenzielle Privacy

**Neue Dateien:**
- `include/privacy/differential_privacy.h`
- `src/privacy/differential_privacy.cpp`

Features:
- ε-differenzielle Privacy für Aggregationen
- Privacy Budget Management
- Laplace/Gaussian Noise Mechanisms

#### 3.2 ML-basierte Anomalieerkennung

**Neue Dateien:**
- `include/security/ml_anomaly_detector.h`
- `src/security/ml_anomaly_detector.cpp`

Features:
- Online-Learning für Nutzerprofil
- Verhaltensbasierte Anomalieerkennung
- Automatische Alarmierung

---

## Konfiguration

### Empfohlene Security-Konfiguration

**Datei:** `config/config.yaml` (erweitern)

```yaml
security:
  # Bestehende Konfiguration
  encryption:
    enabled: true
    field_encryption: true
    
  rbac:
    enabled: true
    
  audit:
    enabled: true
    events: all
    
  # Neue Graph-Schutz-Konfiguration
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
      enabled: false  # Optional, nach Implementierung
      strength: medium
      key_rotation_days: 90
      
    # Differential Privacy (Phase 3)
    differential_privacy:
      enabled: false  # Optional, nach Implementierung
      epsilon: 1.0
      delta: 1e-5
```

---

## Best Practices

### 1. Defense in Depth

Kombinieren Sie mehrere Schutzschichten:

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

Gewähren Sie minimale erforderliche Berechtigungen:

```yaml
# Beispiel: Restriktive Graph-Berechtigungen
roles:
  - name: data_analyst
    permissions:
      graph:
        read: true
        traverse: true
        max_depth: 3       # Limitiert
        export: false      # Kein Export
        
  - name: data_scientist
    permissions:
      graph:
        read: true
        traverse: true
        max_depth: 5
        export: true
        export_approval_required: true  # Approval-Workflow
```

### 3. Kontinuierliches Monitoring

Überwachen Sie verdächtige Aktivitäten:

```python
# Beispiel: Prometheus Alert für verdächtige Graph-Zugriffe
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

### 4. Regelmäßige Security Audits

Führen Sie regelmäßige Audits durch:

- **Wöchentlich:** Review Audit Logs auf Anomalien
- **Monatlich:** Analyse Zugriffsmuster
- **Quartalsweise:** Penetration Tests
- **Jährlich:** Umfassende Security-Assessments

### 5. Incident Response Plan

Bereiten Sie sich auf Sicherheitsvorfälle vor:

1. **Detection:** Automatische Alarmierung bei Anomalien
2. **Analysis:** Untersuchung des Vorfalls
3. **Containment:** Sofortige Sperrung betroffener Accounts
4. **Eradication:** Entfernung von Backdoors/Malware
5. **Recovery:** Wiederherstellung normaler Operations
6. **Lessons Learned:** Post-Mortem-Analyse

---

## Zusammenfassung

### Aktuelle Situation

ThemisDB verfügt über **solide Basis-Sicherheit**:
- ✅ Verschlüsselung (Field/Column/Vector)
- ✅ RBAC/ABAC
- ✅ Audit Logging
- ✅ Rate Limiting

### Empfohlene Verbesserungen

**Phase 1 (Sofort implementierbar):**
1. Erweiterte Audit-Logs für Graph-Operationen
2. Graph-spezifische Rate Limits
3. Access Pattern Monitoring
4. Export Controls

**Phase 2 (Mittelfristig):**
1. Graph Watermarking
2. Embedding Fingerprinting
3. Enhanced Anomaly Detection

**Phase 3 (Langfristig):**
1. Differenzielle Privacy
2. ML-basierte Anomalieerkennung
3. Advanced Threat Intelligence

### Priorität

**Hoch (Sofort):**
- Graph-spezifische Rate Limits konfigurieren
- Audit-Logs für Graph-Operationen aktivieren
- Monitoring-Alerts einrichten

**Mittel (3-6 Monate):**
- Access Pattern Monitoring implementieren
- Export Controls verfeinern

**Niedrig (6-12 Monate):**
- Watermarking/Fingerprinting evaluieren
- Differenzielle Privacy für spezielle Use Cases

---

## Referenzen

### Forschungsliteratur

1. **Graph Watermarking:**
   - "Watermarking Techniques for Knowledge Graphs" (2025)
   - "Robust Graph Structure Fingerprinting" (2024)

2. **Embedding Protection:**
   - "Protecting Vector Embeddings from Theft" (2025)
   - "Imperceptible Perturbations for Embedding Security" (2024)

3. **Training Data Protection:**
   - "Making Stolen Data Unusable for AI Training" (2026)
   - "Data Poisoning Defenses for Knowledge Graphs" (2025)

### ThemisDB-Dokumentation

- [Security Overview](security_overview.md)
- [Encryption Strategy](security_encryption_strategy.md)
- [Audit Logging](../development/auditlog.md)
- [RBAC Implementation](security_implementation.md)

---

**Autor:** ThemisDB Security Team  
**Letzte Aktualisierung:** 7. Januar 2026  
**Review-Status:** Draft - Implementierung ausstehend
