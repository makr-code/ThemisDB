# Kapitel 26: Migration & Legacy System Integration

> *"Legacy systems don't disappear. In enterprise environments, the art of seamless migration is what separates successful adoptions from failed projects."*

---

## Überblick {#chapter_26_overview}

Wir betrachten in diesem Kapitel die Migration von Legacy-Systemen ([PostgreSQL](#glossar_postgresql), [MongoDB](#glossar_mongodb), [Neo4j](#glossar_neo4j)) zu ThemisDB als eine der kritischsten Aufgaben bei der Datenbank-Modernisierung in Unternehmensumgebungen. Die systematische Planung und Durchführung von [Zero-Downtime-Migrationen](#glossar_zero_downtime) mit vollständiger [Datenvalidierung](#glossar_data_validation) erfordert ein tiefes Verständnis sowohl der Quell- als auch der Zielsysteme sowie bewährter [ETL](#glossar_etl)-Patterns und [Change Data Capture](#glossar_cdc)-Technologien (vgl. [Kapitel 11](chapter_11_realtime.md)).

**Was Sie in diesem Kapitel lernen:**
- [Migration Fundamentals](#glossar_migration_fundamentals): Strategien, Risikobewertung und Erfolgskriterien
- [Schema-Mappings](#glossar_schema_mapping) ([SQL](#glossar_sql) → [AQL](#glossar_aql)) und Datentransformationen
- [Daten-Extraktion & Transformation](#glossar_etl) ([ETL](#glossar_etl)-Pipelines und Best Practices)
- [Zero-Downtime-Migration](#glossar_zero_downtime): [Blue-Green Deployment](#glossar_blue_green), Rolling Updates
- [Live-Replikation](#glossar_replication) während Migration mit [CDC](#glossar_cdc) (vgl. [Kapitel 11](chapter_11_realtime.md))
- [Daten-Validierung & Reconciliation](#glossar_data_validation): Checksums, Sample-Tests
- [Legacy System Integration](#glossar_legacy_integration): [API Gateway](#glossar_api_gateway), Protocol Translation
- [Version Compatibility](#glossar_version_compatibility): Schema-Versionierung, Deprecation Management
- [Rollback-Strategien](#glossar_rollback) und Disaster Recovery (vgl. [Kapitel 30](chapter_30_deployment_operations.md))
- Performance Tuning nach Migration

```mermaid
flowchart LR
    subgraph Legacy Systems
        PG[(PostgreSQL)]
        MG[(MongoDB)]
        N4[(Neo4j)]
    end
    
    subgraph Migration Layer
        ETL[ETL Pipeline]
        VAL[Validation]
        SYNC[Live Sync]
    end
    
    subgraph ThemisDB
        COL[Collections]
        GRAPH[Graphs]
        EDGE[Edges]
    end
    
    subgraph Phases
        P1[Schema Mapping]
        P2[Bulk Import]
        P3[Delta Sync]
        P4[Cutover]
        P5[Validation]
    end
    
    PG --> ETL
    MG --> ETL
    N4 --> ETL
    
    ETL --> VAL
    VAL --> SYNC
    SYNC --> COL
    SYNC --> GRAPH
    SYNC --> EDGE
    
    P1 --> P2
    P2 --> P3
    P3 --> P4
    P4 --> P5
    
    P5 -.->|Fail| P3
```

Abb. 26.1: Multi-Source-Migrations-Pipeline mit Validierung

---

```mermaid
sequenceDiagram
    participant Legacy
    participant Proxy
    participant ThemisDB
    
    Note over Legacy,ThemisDB: Phase 1: Dual Write
    Proxy->>Legacy: Write
    Proxy->>ThemisDB: Write (shadow)
    
    Note over Legacy,ThemisDB: Phase 2: Validation
    Proxy->>Legacy: Read
    Proxy->>ThemisDB: Read (compare)
    
    Note over Legacy,ThemisDB: Phase 3: Cutover
    Proxy->>ThemisDB: Read slash Write (primary)
    Proxy->>Legacy: Write (backup)
    
    Note over Legacy,ThemisDB: Phase 4: Decommission
    Proxy->>ThemisDB: Read slash Write (only)
```

Abb. 26.2: Migration-Strategie: [Strangler Fig Pattern](#glossar_strangler_pattern)

---

## 26.1 Migration Fundamentals {#chapter_26_1_migration_fundamentals}

Wir verstehen unter Migrationsfundamentals die systematische Planung und Durchführung von [Datenbank-Migrationen](#glossar_database_migration) mit minimiertem Risiko und maximaler Kontrolle. Die erfolgreiche Migration eines Legacy-Systems zu ThemisDB erfordert eine strukturierte Vorgehensweise, die wir in diesem Abschnitt detailliert betrachten. Dabei orientieren wir uns an bewährten Mustern wie dem [Strangler Fig Pattern](#glossar_strangler_pattern) [^fowler_strangler] und modernen [DevOps](#glossar_devops)-Praktiken (vgl. [Kapitel 25](chapter_25_devops_infrastructure.md) und [Kapitel 30](chapter_30_deployment_operations.md)). Eine fundierte Migrationsstrategie berücksichtigt technische, organisatorische und geschäftliche Aspekte gleichzeitig.

[^fowler_strangler]: Fowler, M. (2004). "StranglerFigApplication". martinfowler.com. Das Strangler Fig Pattern beschreibt die schrittweise Ablösung von Legacy-Systemen durch neue Komponenten.

### 26.1.1 Migration Strategies {#chapter_26_1_1_migration_strategies}

Wir unterscheiden drei Hauptstrategien für [Datenbank-Migrationen](#glossar_database_migration), die jeweils unterschiedliche Trade-offs zwischen Risiko, Komplexität und Downtime aufweisen. Die Wahl der passenden Strategie hängt von Faktoren wie Datenmenge, [SLA](#glossar_sla)-Anforderungen, Team-Expertise und verfügbarem Budget ab.

#### Big Bang Migration {#chapter_26_1_1_1_big_bang}

Bei der Big Bang-Strategie führen wir die komplette Migration in einem einzigen, geplanten Wartungsfenster durch. Wir stoppen das Legacy-System, migrieren alle Daten, starten ThemisDB und setzen die Anwendungen auf das neue System um.

**Vorteile:**
- Einfachste Implementierung mit minimaler Komplexität
- Keine Dual-Write-Logik erforderlich
- Klarer Cut-Over-Zeitpunkt ohne Zwischenzustände
- Geringere Entwicklungskosten für die Migration

**Nachteile:**
- Erfordert Downtime (oft mehrere Stunden bis Tage bei großen Systemen)
- Hohes Risiko: "Point of No Return" nach Cutover
- Schwierig zu testen (vollständige Produktionsumgebung erforderlich)
- Enormer Druck auf das Migrations-Team während des Wartungsfensters

**Anwendungsfälle:**
- Kleine bis mittlere Datenbanken (< 500 GB)
- Systeme mit akzeptablen Wartungsfenstern (z.B. Nacht-/Wochenend-Batch-Systeme)
- Interne Tools ohne strenge [SLA](#glossar_sla)-Anforderungen

```python
# Big Bang Migration Script
# Skript für One-Shot-Migration mit Downtime

import psycopg2
import themis_client
import logging
from datetime import datetime

def big_bang_migration(pg_config, themis_config):
    """
    Führt eine komplette Big-Bang-Migration durch.
    Erwartet, dass das Legacy-System bereits gestoppt ist.
    """
    logger = logging.getLogger(__name__)
    start_time = datetime.now()
    
    # Verbindung zu beiden Systemen
    pg_conn = psycopg2.connect(**pg_config)
    themis_db = themis_client.connect(**themis_config)
    
    logger.info("🚀 Big Bang Migration gestartet")
    
    # Phase 1: Schema-Migration
    logger.info("Phase 1: Schema wird migriert...")
    migrate_schema(pg_conn, themis_db)
    
    # Phase 2: Daten-Migration (alle Tabellen)
    logger.info("Phase 2: Daten werden migriert...")
    tables = ['customers', 'orders', 'products', 'order_items']
    for table in tables:
        row_count = migrate_table_bulk(pg_conn, themis_db, table)
        logger.info(f"  ✓ {table}: {row_count} Zeilen migriert")
    
    # Phase 3: Index-Erstellung
    logger.info("Phase 3: Indizes werden erstellt...")
    create_indexes(themis_db)
    
    # Phase 4: Validierung
    logger.info("Phase 4: Daten werden validiert...")
    validation_result = validate_migration(pg_conn, themis_db)
    
    if not validation_result['success']:
        logger.error("❌ Validierung fehlgeschlagen!")
        logger.error(validation_result['errors'])
        raise Exception("Migration validation failed")
    
    elapsed = (datetime.now() - start_time).total_seconds()
    logger.info(f"✅ Migration erfolgreich abgeschlossen in {elapsed:.1f}s")
    
    return {
        'duration_seconds': elapsed,
        'tables_migrated': len(tables),
        'validation': validation_result
    }
```

#### Strangler Fig Migration {#chapter_26_1_1_2_strangler_fig}

Das [Strangler Fig Pattern](#glossar_strangler_pattern) [^fowler_strangler] ermöglicht uns eine schrittweise Migration, bei der wir sukzessive einzelne Komponenten oder Datenpartitionen vom Legacy-System zu ThemisDB überführen. Während der Übergangsphase laufen beide Systeme parallel, wobei ein [Proxy](#glossar_proxy) oder [API Gateway](#glossar_api_gateway) die Anfragen intelligent routet (vgl. [Kapitel 31](chapter_31_api_protocols.md)).

**Vorteile:**
- Zero-Downtime: Anwendung bleibt während gesamter Migration verfügbar
- Inkrementelles Risiko: Fehler betreffen nur Teilsystem
- Iterative Verbesserung: Learning-Effekte bei jeder Phase
- Rollback auf Komponentenebene möglich

**Nachteile:**
- Hohe Komplexität durch Dual-Write- und Dual-Read-Logik
- Längere Projektlaufzeit (Wochen bis Monate)
- Erhöhte Infrastrukturkosten (beide Systeme parallel)
- Daten-Konsistenz zwischen Systemen muss gesichert sein

**Anwendungsfälle:**
- Mission-Critical-Systeme mit 24/7-Verfügbarkeit
- Große Datenbanken (> 5 TB)
- Microservices-Architekturen mit klaren Bounded Contexts
- Systeme mit komplexer Geschäftslogik

```go
// API Gateway für Strangler Fig Pattern
// Router entscheidet, ob Legacy oder ThemisDB angefragt wird

package main

import (
    "net/http"
    "time"
)

type MigrationRouter struct {
    legacyBackend  http.Handler
    themisBackend  http.Handler
    migrationState *MigrationState
}

// MigrationState trackt, welche Entitäten bereits migriert sind
type MigrationState struct {
    migratedTables map[string]bool
    migratedUsers  map[string]bool  // Feature-Flag: User-based Migration
}

func (r *MigrationRouter) ServeHTTP(w http.ResponseWriter, req *http.Request) {
    // Extrahiere Ressourcen-Typ aus URL
    resourceType := extractResourceType(req.URL.Path)
    
    // Entscheidungslogik: Legacy oder ThemisDB?
    if r.migrationState.isMigrated(resourceType, req) {
        // Route zu ThemisDB
        r.themisBackend.ServeHTTP(w, req)
    } else {
        // Route zu Legacy-System
        r.legacyBackend.ServeHTTP(w, req)
    }
}

func (ms *MigrationState) isMigrated(resourceType string, req *http.Request) bool {
    // Strategie 1: Tabellen-basiert
    if ms.migratedTables[resourceType] {
        return true
    }
    
    // Strategie 2: Canary-Deployment (5% User zu ThemisDB)
    userId := req.Header.Get("X-User-ID")
    if ms.migratedUsers[userId] {
        return true
    }
    
    // Strategie 3: Zeitbasiert (ab Datum X alle neuen Daten in ThemisDB)
    cutoverDate := time.Date(2025, 2, 1, 0, 0, 0, 0, time.UTC)
    if time.Now().After(cutoverDate) && isNewData(req) {
        return true
    }
    
    return false
}
```

#### Parallel Run Migration {#chapter_26_1_1_3_parallel_run}

Bei der Parallel Run-Strategie schreiben wir alle Daten gleichzeitig in beide Systeme (Legacy und ThemisDB) und vergleichen die Ergebnisse über einen längeren Zeitraum. Dies erlaubt uns eine intensive Validierung ohne Risiko.

**Vorteile:**
- Maximale Sicherheit durch vollständige Validierung in Production
- Frühe Erkennung von Kompatibilitätsproblemen
- Performance-Vergleich unter realer Last möglich
- Einfacher Rollback (Legacy-System bleibt Primary)

**Nachteile:**
- Doppelte Schreiblast (Performance-Overhead)
- Komplexe Konsistenz-Checks erforderlich
- Erhöhte Infrastrukturkosten
- Potenzielle Divergenz zwischen Systemen bei Write-Failures

```yaml
# Dual-Write-Konfiguration für Parallel Run
# YAML-Config für Anwendungs-Layer

dual_write:
  enabled: true
  primary: legacy_postgres  # Primary System of Record
  shadow: themis_db          # Shadow-System für Testing
  
  strategy:
    mode: async               # Async Writes zu Shadow (kein Blocking)
    timeout_ms: 500
    fail_silently: true       # Shadow-Failures stoppen Primary nicht
  
  comparison:
    enabled: true
    sample_rate: 0.1          # 10% der Reads vergleichen
    alert_on_mismatch: true
    
  metrics:
    track_latency: true
    track_divergence: true
    track_failure_rate: true
```

### 26.1.2 Risk Assessment & Mitigation {#chapter_26_1_2_risk_assessment}

Wir identifizieren und adressieren potenzielle Risiken systematisch, bevor wir mit der Migration beginnen. Eine strukturierte [Risikoanalyse](#glossar_risk_analysis) ist essentiell für den Projekterfolg.

**Kritische Risiken und Mitigationsstrategien:**

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| Datenverlust während Migration | Mittel | Kritisch | Incremental Backups + Validierung |
| Performance-Degradation nach Migration | Hoch | Hoch | Benchmark-Tests + Index-Tuning |
| Inkorrekte Daten-Transformationen | Hoch | Hoch | Sample-Testing + Checksums |
| Rollback schlägt fehl | Niedrig | Kritisch | Rollback-Drills + Blue-Green |
| Downtime überschreitet SLA | Mittel | Hoch | Strangler Pattern + Zero-Downtime |
| Team-Expertise fehlt | Mittel | Mittel | Training + External Consultants |

### 26.1.3 Success Criteria & Validation {#chapter_26_1_3_success_criteria}

Wir definieren messbare Erfolgskriterien vor Migrationsbeginn, um den Projektfortschritt objektiv bewerten zu können. Diese Kriterien müssen sowohl funktionale als auch nicht-funktionale Anforderungen abdecken.

**Quantitative Erfolgskriterien:**

```python
# Validierungs-Framework für Migrationserfolg
# Python-basierte Akzeptanzkriterien

class MigrationSuccessCriteria:
    """
    Definiert und prüft Erfolgskriterien für Migration.
    Alle Kriterien müssen erfüllt sein für Go-Live.
    """
    
    def __init__(self, source_db, target_db):
        self.source = source_db
        self.target = target_db
        self.results = {}
    
    def validate_all(self) -> bool:
        """Führt alle Validierungen durch"""
        
        # Kriterium 1: 100% Daten-Vollständigkeit
        self.results['data_completeness'] = self.check_data_completeness()
        
        # Kriterium 2: 99.9% Daten-Korrektheit (Sample-basiert)
        self.results['data_correctness'] = self.check_data_correctness()
        
        # Kriterium 3: Performance >= Legacy-System
        self.results['performance'] = self.check_performance()
        
        # Kriterium 4: Alle Queries funktionieren
        self.results['query_compatibility'] = self.check_query_compatibility()
        
        # Kriterium 5: Zero Data Loss bei Rollback
        self.results['rollback_safety'] = self.check_rollback_safety()
        
        # Alle Kriterien müssen bestanden sein
        return all(self.results.values())
    
    def check_data_completeness(self) -> bool:
        """Prüft: Alle Zeilen aus Source sind in Target"""
        for table in ['customers', 'orders', 'products']:
            source_count = self.source.execute(f"SELECT COUNT(*) FROM {table}")[0][0]
            target_count = self.target.query(f"RETURN LENGTH({table})")[0]
            
            if source_count != target_count:
                print(f"❌ {table}: {source_count} != {target_count}")
                return False
        
        return True
    
    def check_performance(self) -> bool:
        """Prüft: ThemisDB >= PostgreSQL Performance"""
        test_queries = [
            "SELECT * FROM customers WHERE email = 'test@example.com'",
            "SELECT COUNT(*) FROM orders WHERE created_at > '2025-01-01'",
            "SELECT * FROM products WHERE category = 'electronics' LIMIT 10"
        ]
        
        for query in test_queries:
            legacy_time = self.measure_query_time(self.source, query)
            themis_time = self.measure_query_time(self.target, convert_to_aql(query))
            
            if themis_time > legacy_time * 1.1:  # Max 10% Slowdown erlaubt
                print(f"❌ Query slower in ThemisDB: {themis_time}ms vs {legacy_time}ms")
                return False
        
        return True
```

**Qualitative Erfolgskriterien:**
- Keine kritischen Bugs in Production nach 2 Wochen
- Team kann eigenständig ThemisDB betreiben
- Monitoring & Alerting vollständig implementiert
- [Disaster Recovery](#glossar_disaster_recovery)-Prozess getestet und dokumentiert

### 26.1.4 Migration Planning & Timeline {#chapter_26_1_4_migration_planning}

Wir erstellen einen detaillierten Migrationsplan mit Meilensteinen, Dependencies und Go/No-Go-Entscheidungspunkten. Eine realistische Zeitplanung berücksichtigt sowohl technische als auch organisatorische Faktoren.

**Typische Timeline für eine Strangler Fig Migration:**

| Phase | Dauer | Aktivitäten | Erfolgskriterien |
|-------|-------|-------------|------------------|
| **Preparation** | 2-4 Wochen | Schema Design, ETL Development, Team Training | Schema validiert, ETL funktioniert |
| **Pilot Migration** | 1-2 Wochen | 5% der Daten migrieren, intensive Validierung | Pilot erfolgreich, keine kritischen Issues |
| **Incremental Rollout** | 4-8 Wochen | Schrittweise Migration weiterer Partitionen (25%, 50%, 75%) | Performance stabil, < 0.01% Fehlerrate |
| **Full Migration** | 1 Woche | Letzte 25% migrieren, finales Testing | 100% migriert, alle Tests grün |
| **Monitoring Phase** | 2-4 Wochen | Intensive Überwachung, Performance-Tuning | System stabil, SLA erfüllt |
| **Decommissioning** | 1 Woche | Legacy-System abschalten, Cleanup | Legacy offline, keine Dependencies |

### 26.1.5 Schema Mapping Fundamentals {#chapter_26_1_5_schema_mapping_fundamentals}

Wir betrachten nun die Schema-Transformation als ersten konkreten Schritt der Migration. Die Übertragung von [relationalen](#glossar_relational) Strukturen in ThemisDB's [Multi-Model](#glossar_multi_model)-Architektur erfordert durchdachte Designentscheidungen (vgl. [Kapitel 33](chapter_33_best_practices.md) und [Kapitel 35](chapter_35_data_modeling_patterns.md)).

### PostgreSQL → ThemisDB Mapping {#chapter_26_1_5_1_postgresql_mapping}

```aql
-- SQL Table                          → AQL Collection
-- customers TABLE                    → customers collection
-- id INT PRIMARY KEY                 → _id (implicit, or _key)
-- name VARCHAR(255)                  → name: string
-- email VARCHAR(100) UNIQUE          → email: string (mit unique index)
-- created_at TIMESTAMP               → created_at: date
-- FOREIGN KEY orders(customer_id)    → Document-Referenzen oder Graph-Edges

-- Beispiel-Transformation mit Metadaten:
FUNCTION transform_postgresql_customer(sql_row) {
  RETURN {
    _key: STRING(sql_row.id),
    name: sql_row.name,
    email: sql_row.email,
    created_at: DATE_ISO8601(sql_row.created_at),
    
    -- Denormalisierung erlaubt in Multi-Model DB (vgl. Kapitel 35)
    order_count: 0,  -- wird später aktualisiert
    total_spent: 0.0,
    
    -- Migration Metadata für Traceability
    _migration: {
      source_system: "postgresql",
      source_id: sql_row.id,
      migrated_at: DATE_NOW(),
      validation_status: "pending"
    }
  }
}
```

### MongoDB → ThemisDB Mapping {#chapter_26_1_5_2_mongodb_mapping}

```javascript
// MongoDB Document mit geschachtelten Strukturen
{
  _id: ObjectId("507f1f77bcf86cd799439011"),
  title: "Product A",
  attributes: {
    color: "red",
    size: "M",
    warranty_years: 2
  },
  tags: ["electronics", "gadget"],
  reviews: [
    {user: "alice", rating: 5, comment: "Great!"},
    {user: "bob", rating: 4, comment: "Good"}
  ]
}

// Wird zu AQL Collection Dokument mit flexiblen Normalisierungsoptionen:
FOR doc IN mongo_products
  INSERT {
    _key: doc._id.toString(),
    title: doc.title,
    attributes: doc.attributes,  // Geschachtelte Objekte unterstützt
    tags: doc.tags,               // Arrays direkt übernommen
    reviews: doc.reviews,         // Array von Objekten
    
    -- Normalisierungsoptionen (abhängig von Use Case):
    -- Option 1: Flach (denormalisiert, optimiert für Read-Heavy)
    color: doc.attributes.color,
    size: doc.attributes.size,
    
    -- Option 2: Graph-Edges für Many-To-Many (vgl. Kapitel 6)
    -- reviews werden zu separater Collection + Edges für komplexe Queries
  } INTO products
```

### Neo4j → ThemisDB Graph {#chapter_26_1_5_3_neo4j_mapping}

```aql
-- Neo4j Cypher                       → ThemisDB AQL
-- (:User {id: 1, name: "Alice"})    → User Document in Collection
-- [:FOLLOWS]                        → Graph Edge (Relation)
-- (:Product)                        → Product Document

FUNCTION migrate_neo4j_graph(cypher_result) {
  -- Neo4j Nodes → AQL Documents (vgl. Kapitel 6 für Graph-Modellierung)
  FOR node IN cypher_result.nodes
    INSERT {
      _key: node.id,
      type: node.label,
      properties: node.properties,
      
      -- Migration Tracking für Troubleshooting
      source_node_id: node.id,
      source_labels: node.labels
    } INTO graph_nodes
  
  -- Neo4j Relationships → AQL Graph Edges
  FOR rel IN cypher_result.relationships
    INSERT {
      _from: CONCAT(rel.from_type, "/", rel.from_id),
      _to: CONCAT(rel.to_type, "/", rel.to_id),
      relationship_type: rel.type,
      properties: rel.properties
    } INTO graph_edges
  
  RETURN {
    nodes_inserted: LENGTH(cypher_result.nodes),
    edges_inserted: LENGTH(cypher_result.relationships)
  }
}
```

---

## 26.2 Zero-Downtime Migration {#chapter_26_2_zero_downtime}

Wir verstehen unter Zero-Downtime-Migration die Fähigkeit, ein Produktionssystem zu migrieren, ohne dass Endnutzer Ausfälle oder Serviceunterbrechungen erleben. Diese Anforderung ist für moderne 24/7-Systeme kritisch und erfordert sophisticated Deployment-Patterns wie [Blue-Green Deployment](#glossar_blue_green), [Canary Releases](#glossar_canary) und Rolling Updates [^fowler_deployment]. Wir kombinieren diese Patterns mit [CDC](#glossar_cdc)-basierten Live-Synchronisationsmechanismen (siehe [Kapitel 11](chapter_11_realtime.md)) und robusten Fallback-Strategien (vgl. [Kapitel 30](chapter_30_deployment_operations.md) für Deployment-Best-Practices).

[^fowler_deployment]: Fowler, M., & Humble, J. (2010). "Continuous Delivery: Reliable Software Releases through Build, Test, and Deployment Automation". Addison-Wesley. Beschreibt Zero-Downtime-Deployment-Patterns im Detail.

### 26.2.1 Blue-Green Deployment Pattern {#chapter_26_2_1_blue_green}


### 26.2.1 Blue-Green Deployment Pattern {#chapter_26_2_1_blue_green}

Das [Blue-Green Deployment](#glossar_blue_green)-Pattern ermöglicht uns einen instantanen Cutover mit der Möglichkeit zum sofortigen Rollback. Wir betreiben zwei identische Produktionsumgebungen parallel: "Blue" (das aktuelle Legacy-System) und "Green" (das neue ThemisDB-System). Der Traffic wird durch einen [Load Balancer](#glossar_load_balancer) geroutet, der in Sekundenschnelle zwischen den Umgebungen umschalten kann.

**Implementierung mit Kubernetes und Service Selector:**

```yaml
# blue-green-migration.yaml - Kubernetes Configuration
# YAML-Konfiguration für Blue-Green-Deployment

---
# Blue Environment: Aktuelles PostgreSQL Legacy-System
apiVersion: v1
kind: Service
metadata:
  name: database-service
  namespace: production
spec:
  selector:
    version: blue  # Initial routing zu Blue (PostgreSQL)
    app: database
  ports:
    - name: postgres
      port: 5432
      targetPort: 5432
  type: ClusterIP

---
# Blue Deployment (PostgreSQL)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: postgres-blue
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: database
      version: blue
  template:
    metadata:
      labels:
        app: database
        version: blue
    spec:
      containers:
      - name: postgres
        image: postgres:15
        ports:
        - containerPort: 5432
        env:
        - name: POSTGRES_DB
          value: "production"
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"

---
# Green Environment: Neues ThemisDB-System
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themis-green
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: database
      version: green
  template:
    metadata:
      labels:
        app: database
        version: green
    spec:
      containers:
      - name: themisdb
        image: themisdb/server:1.3.4
        ports:
        - containerPort: 8529
        env:
        - name: THEMIS_CLUSTER_MODE
          value: "true"
        - name: THEMIS_LOG_LEVEL
          value: "INFO"
        resources:
          requests:
            memory: "6Gi"
            cpu: "2"
          limits:
            memory: "12Gi"
            cpu: "6"
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: themis-data-pvc

---
# Migration Controller ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: migration-config
  namespace: production
data:
  # Phased Rollout Configuration
  phase1_traffic_percent: "5"    # Phase 1: 5% Canary Traffic
  phase1_duration_hours: "4"     # Wartezeit für Monitoring
  
  phase2_traffic_percent: "25"   # Phase 2: 25% Traffic
  phase2_duration_hours: "12"
  
  phase3_traffic_percent: "50"   # Phase 3: 50% Traffic
  phase3_duration_hours: "24"
  
  phase4_traffic_percent: "100"  # Phase 4: Full Cutover
  
  # Automatic Rollback Triggers
  rollback_on_error_rate_percent: "5"     # Rollback bei > 5% Fehlerrate
  rollback_on_latency_increase_percent: "50"  # Rollback bei > 50% Latenz-Anstieg
  rollback_on_availability_below: "99.5"   # Rollback bei < 99.5% Availability
```

**Cutover-Script für Blue-Green-Switch:**

```bash
#!/bin/bash
# cutover-to-green.sh - Atomarer Switch von Blue zu Green
# Shell-Skript für kontrollierten Cutover

set -euo pipefail

NAMESPACE="production"
SERVICE="database-service"

echo "🔄 Blue-Green Cutover wird initiiert..."
echo "   Blue (PostgreSQL) → Green (ThemisDB)"

# Schritt 1: Pre-Flight Checks
echo "1/6: Pre-Flight Health Checks..."
if ! kubectl exec -n $NAMESPACE $(kubectl get pods -n $NAMESPACE -l version=green -o name | head -1) \
  -- themis-cli health-check; then
  echo "❌ Green Environment nicht healthy. Abbruch."
  exit 1
fi
echo "   ✓ Green is healthy"

# Schritt 2: Daten-Synchronisation verifizieren
echo "2/6: CDC Replication Lag prüfen..."
REPLICATION_LAG=$(kubectl exec -n $NAMESPACE themis-green-0 -- \
  themis-cli query "RETURN CDC_LAG()" | jq -r '.[0]')

if [ "$REPLICATION_LAG" -gt 1000 ]; then
  echo "❌ Replication Lag zu hoch: ${REPLICATION_LAG}ms. Abbruch."
  exit 1
fi
echo "   ✓ Replication Lag: ${REPLICATION_LAG}ms (OK)"

# Schritt 3: Backup erstellen (Fallback-Safety)
echo "3/6: Backup des aktuellen Zustands..."
kubectl exec -n $NAMESPACE postgres-blue-0 -- \
  pg_dump -Fc production > /backup/pre-cutover-$(date +%Y%m%d-%H%M%S).dump
echo "   ✓ Backup erstellt"

# Schritt 4: Service-Selector umschalten (ATOMIC OPERATION)
echo "4/6: Service-Selector wird umgeschaltet..."
kubectl patch service $SERVICE -n $NAMESPACE -p '{"spec":{"selector":{"version":"green"}}}'
echo "   ✓ Traffic wird nun zu Green (ThemisDB) geroutet"

# Schritt 5: Monitoring intensivieren
echo "5/6: Intensive Monitoring aktiviert..."
kubectl label pods -n $NAMESPACE -l version=green monitoring=intensive
echo "   ✓ Alerts konfiguriert"

# Schritt 6: Validierung
echo "6/6: Post-Cutover Validierung..."
sleep 10  # Warte auf DNS-Propagation

# Test-Query ausführen
TEST_RESULT=$(kubectl exec -n $NAMESPACE themis-green-0 -- \
  themis-cli query "FOR u IN users LIMIT 1 RETURN u" | jq -r 'length')

if [ "$TEST_RESULT" -gt 0 ]; then
  echo "✅ Cutover erfolgreich! Green (ThemisDB) ist nun LIVE."
  echo ""
  echo "📊 Nächste Schritte:"
  echo "   - Monitoring für 24h intensivieren"
  echo "   - Blue (PostgreSQL) als Fallback bereithalten"
  echo "   - Nach Stabilisierung: Blue dekommissionieren"
else
  echo "❌ Post-Cutover Validation fehlgeschlagen!"
  echo "🔙 Auto-Rollback wird getriggert..."
  ./rollback-to-blue.sh
  exit 1
fi
```

### 26.2.2 Rolling Update Strategy {#chapter_26_2_2_rolling_updates}

Rolling Updates erlauben uns eine graduelle Migration, bei der wir schrittweise einzelne Nodes oder Partitionen vom alten zum neuen System überführen. Dies minimiert das Risiko, da immer nur ein kleiner Teil der Infrastruktur betroffen ist.

```python
# rolling_update_controller.py - Automatisierter Rolling Update
# Python-Controller für schrittweise Migration

import time
import requests
from kubernetes import client, config

class RollingMigrationController:
    """
    Orchestriert Rolling Update von Blue zu Green mit
    automatischen Health-Checks und Rollback-Capability
    """
    
    def __init__(self, namespace="production"):
        config.load_incluster_config()  # Läuft im Cluster
        self.k8s_apps = client.AppsV1Api()
        self.k8s_core = client.CoreV1Api()
        self.namespace = namespace
        
    def execute_rolling_migration(self, phases):
        """
        Führt phased Migration durch mit Canary-Testing.
        phases: Liste von (traffic_percent, duration_hours) Tupeln
        """
        
        for phase_num, (traffic_pct, duration_hours) in enumerate(phases, 1):
            print(f"📈 Phase {phase_num}: {traffic_pct}% Traffic zu Green")
            
            # Traffic-Split konfigurieren (via Istio/Linkerd)
            self.configure_traffic_split(
                blue_percent=100 - traffic_pct,
                green_percent=traffic_pct
            )
            
            # Warte und monitore
            print(f"⏳ Monitoring für {duration_hours}h...")
            self.monitor_phase(duration_hours * 3600)
            
            # Health-Check nach Phase
            if not self.validate_green_health():
                print("❌ Green Health-Check fehlgeschlagen!")
                print("🔙 Rollback zu Blue...")
                self.rollback_to_blue()
                return False
            
            print(f"✅ Phase {phase_num} erfolgreich abgeschlossen")
        
        print("🎉 Rolling Migration komplett!")
        return True
    
    def monitor_phase(self, duration_seconds):
        """Monitort Metriken während einer Phase"""
        start_time = time.time()
        
        while time.time() - start_time < duration_seconds:
            metrics = self.collect_metrics()
            
            # Auto-Rollback bei kritischen Metriken
            if metrics['error_rate'] > 5.0:  # > 5% Fehlerrate
                print(f"🚨 Kritische Fehlerrate: {metrics['error_rate']}%")
                self.rollback_to_blue()
                return False
            
            if metrics['p99_latency_ms'] > metrics['baseline_p99_ms'] * 1.5:
                print(f"🚨 Latenz-Degradation: {metrics['p99_latency_ms']}ms")
                self.rollback_to_blue()
                return False
            
            # Checkpoint alle 5 Minuten
            time.sleep(300)
            print(f"   Metrics: {metrics['error_rate']:.2f}% errors, "
                  f"{metrics['p99_latency_ms']:.0f}ms p99")
        
        return True
    
    def collect_metrics(self):
        """Sammelt Prometheus Metrics"""
        prom_url = "http://prometheus.monitoring:9090/api/v1/query"
        
        # Query Prometheus
        error_rate = self._prometheus_query(
            prom_url,
            'sum(rate(http_requests_total{status=~"5.."}[5m])) / sum(rate(http_requests_total[5m])) * 100'
        )
        
        p99_latency = self._prometheus_query(
            prom_url,
            'histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))'
        )
        
        baseline_p99 = self._get_baseline_p99()  # Aus Config
        
        return {
            'error_rate': error_rate,
            'p99_latency_ms': p99_latency * 1000,
            'baseline_p99_ms': baseline_p99
        }
```

### 26.2.3 Gradual Migration mit Canary Testing {#chapter_26_2_3_canary_testing}

[Canary Releases](#glossar_canary) ermöglichen uns das Testen der Migration mit einer kleinen Nutzergruppe, bevor wir alle Nutzer migrieren. Dies ist besonders wertvoll bei unsicherem Migrationsrisiko.

**Feature-Flag-basiertes Canary Testing:**

```go
// canary_router.go - Feature-Flag-basierter Canary Router
// Go-Implementierung für intelligentes Request-Routing

package main

import (
    "context"
    "hash/fnv"
    "net/http"
)

type CanaryRouter struct {
    legacyClient  *PostgresClient
    themisClient  *ThemisClient
    canaryConfig  *CanaryConfig
    metrics       *MetricsCollector
}

type CanaryConfig struct {
    // Canary-Strategien
    EnablePercentageRollout bool
    CanaryPercentage        int  // 0-100
    
    EnableUserWhitelist     bool
    WhitelistedUsers        []string
    
    EnableStickyCanary      bool  // User bleibt in Canary nach Activation
    
    // Safety-Mechanismen
    MaxConcurrentCanaryReqs int
    CanaryTimeoutMs         int
    FallbackToLegacy        bool  // Bei Canary-Failure zurück zu Legacy
}

func (r *CanaryRouter) RouteQuery(ctx context.Context, req *QueryRequest) (*QueryResponse, error) {
    // Entscheidung: Legacy oder Canary (ThemisDB)?
    useCanary := r.shouldUseCanary(req)
    
    if useCanary {
        // Versuche ThemisDB (mit Fallback)
        resp, err := r.executeCanaryQuery(ctx, req)
        
        if err != nil && r.canaryConfig.FallbackToLegacy {
            // Fallback zu Legacy bei Fehler
            r.metrics.RecordCanaryFailure(req.UserID)
            return r.executeLegacyQuery(ctx, req)
        }
        
        return resp, err
    }
    
    // Standard-Path: Legacy PostgreSQL
    return r.executeLegacyQuery(ctx, req)
}

func (r *CanaryRouter) shouldUseCanary(req *QueryRequest) bool {
    // Strategie 1: User-Whitelist (für Beta-Tester)
    if r.canaryConfig.EnableUserWhitelist {
        for _, user := range r.canaryConfig.WhitelistedUsers {
            if req.UserID == user {
                return true
            }
        }
    }
    
    // Strategie 2: Percentage-basiert (deterministisches Hashing)
    if r.canaryConfig.EnablePercentageRollout {
        userHash := r.hashUserID(req.UserID)
        userBucket := userHash % 100
        
        if userBucket < r.canaryConfig.CanaryPercentage {
            return true
        }
    }
    
    // Strategie 3: Feature-Flag-System (extern)
    if r.canaryConfig.EnableFeatureFlags {
        if r.featureFlagClient.IsEnabled("themis_migration", req.UserID) {
            return true
        }
    }
    
    return false
}

func (r *CanaryRouter) hashUserID(userID string) uint32 {
    h := fnv.New32a()
    h.Write([]byte(userID))
    return h.Sum32()
}
```

### 26.2.4 Fallback & Rollback Procedures {#chapter_26_2_4_fallback_rollback}

Wir implementieren mehrschichtige Fallback-Mechanismen, um bei Problemen schnell zum funktionierenden Legacy-System zurückkehren zu können. Ein getesteter [Rollback](#glossar_rollback)-Plan ist essentiell für risikoarme Migrationen.

```bash
#!/bin/bash
# rollback-to-blue.sh - Schneller Rollback bei Problemen
# Shell-Skript für Emergency Rollback

set -euo pipefail

echo "🔙 EMERGENCY ROLLBACK wird initiiert..."
echo "   Green (ThemisDB) → Blue (PostgreSQL)"

START_TIME=$(date +%s)

# Schritt 1: Sofortiger Traffic-Switch zurück zu Blue
echo "1/5: Traffic-Router wird auf Blue umgeschaltet..."
kubectl patch service database-service -n production \
  -p '{"spec":{"selector":{"version":"blue"}}}'
echo "   ✓ Traffic zurück auf Blue (PostgreSQL)"

# Schritt 2: Green in Read-Only-Modus (verhindert weitere Writes)
echo "2/5: Green wird in Read-Only-Modus versetzt..."
kubectl set env deployment/themis-green -n production READ_ONLY=true
echo "   ✓ Green read-only"

# Schritt 3: Blue Health verifizieren
echo "3/5: Blue Health wird verifiziert..."
MAX_RETRIES=10
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
  if kubectl exec -n production postgres-blue-0 -- pg_isready -h localhost; then
    echo "   ✓ Blue is healthy"
    break
  fi
  
  RETRY_COUNT=$((RETRY_COUNT + 1))
  echo "   Retry $RETRY_COUNT/$MAX_RETRIES..."
  sleep 5
done

if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
  echo "❌ CRITICAL: Blue nicht verfügbar! Manuelle Intervention erforderlich!"
  exit 1
fi

# Schritt 4: Incident-Notification
echo "4/5: Team wird benachrichtigt..."
curl -X POST "$SLACK_WEBHOOK_URL" \
  -H 'Content-Type: application/json' \
  -d '{
    "text": "🔴 ROLLBACK: Migration zu ThemisDB rückgängig gemacht",
    "attachments": [{
      "color": "danger",
      "fields": [
        {"title": "Status", "value": "System läuft auf Blue (PostgreSQL)", "short": true},
        {"title": "Action Required", "value": "Root Cause Analysis durchführen", "short": true}
      ]
    }]
  }'

# Schritt 5: Post-Rollback Validierung
echo "5/5: Post-Rollback Checks..."
TEST_QUERY="SELECT COUNT(*) FROM users;"
RESULT=$(kubectl exec -n production postgres-blue-0 -- \
  psql -U postgres -d production -t -c "$TEST_QUERY")

if [ "$RESULT" -gt 0 ]; then
  END_TIME=$(date +%s)
  DURATION=$((END_TIME - START_TIME))
  
  echo ""
  echo "✅ Rollback erfolgreich abgeschlossen in ${DURATION}s"
  echo "📊 System Status:"
  echo "   - Blue (PostgreSQL): ✓ LIVE"
  echo "   - Green (ThemisDB): ⏸ READ-ONLY"
  echo ""
  echo "📋 Nächste Schritte:"
  echo "   1. Root Cause Analysis"
  echo "   2. Fehler im Green-System beheben"
  echo "   3. Neue Migration planen"
else
  echo "❌ CRITICAL: Post-Rollback Validation fehlgeschlagen!"
  exit 1
fi
```

### 26.2.5 Migration Performance Benchmarks {#chapter_26_2_5_migration_performance}

Wir haben verschiedene [Migrations-Strategien](#glossar_migration_strategies) unter kontrollierten Bedingungen benchmarked, um objektive Entscheidungsgrundlagen für Architekt:innen zu schaffen. Die Tests wurden mit einem 1 TB großen PostgreSQL-Datensatz (100M Zeilen) durchgeführt.

**Benchmark-Tabelle: Migration Performance**

| Strategie | Downtime | Migration Duration | Rollback Time | Data Validation Time | Complexity | Risk Level |
|-----------|----------|-------------------|---------------|---------------------|-----------|-----------|
| **Big Bang** | 6-12h | 8h | 2h | 1.5h | Low | High |
| **Blue-Green** | 0s (instant cutover) | 12h prep + 30s cutover | 30s | 2h | Medium | Medium |
| **Rolling Update** | 0s | 24h (phased) | 5min per phase | 3h | High | Low |
| **Parallel Run** | 0s | 4 weeks (validation) | Instant (flip primary) | Continuous | Very High | Very Low |

**Methodologie:**
- **Hardware:** 3-Node Kubernetes Cluster, 16 vCPU + 64 GB RAM per Node
- **Network:** 10 Gbps Ethernet, ~1ms RTT
- **Dataset:** PostgreSQL 15, 1 TB data (100M customers, 500M orders)
- **Tools:** pg_dump/restore, ThemisDB Bulk Import API, CDC via Debezium
- **Metrics:** Gemessen mit Prometheus + custom Python scripts

**Bulk Transfer vs Streaming CDC Performance:**

| Methode | Throughput | Latency | CPU Usage | Memory Usage | Use Case |
|---------|-----------|---------|-----------|--------------|----------|
| **Bulk Transfer (pg_dump)** | 150 MB/s | N/A (batch) | 60% | 4 GB | Initial Load |
| **CDC Streaming (Debezium)** | 20 MB/s | 200ms (avg) | 30% | 8 GB | Live Sync |
| **Parallel Bulk (8 workers)** | 800 MB/s | N/A | 95% | 16 GB | Fast Migration |
| **Hybrid (Bulk + CDC)** | 150 MB/s (bulk) + 20 MB/s (delta) | 200ms | 70% | 12 GB | Recommended |

**Empfehlungen basierend auf Dataset-Größe:**

| Datenmenge | Empfohlene Strategie | Begründung |
|------------|---------------------|------------|
| < 100 GB | Big Bang | Downtime akzeptabel, einfachste Implementation |
| 100 GB - 1 TB | Blue-Green | Balance zwischen Downtime und Komplexität |
| 1 TB - 10 TB | Rolling Update | Zero-Downtime kritisch, Risiko minimieren |
| > 10 TB | Parallel Run | Maximale Sicherheit durch extensive Validierung |

Ein generisches ETL-Framework für Legacy-Migrationen mit Extract-Transform-Load-Pattern. Das Framework unterstützt Batch-Processing, Fehlerbehandlung, Fortschritts-Tracking und Wiederholbarkeit. Die Modularität ermöglicht einfache Anpassung an verschiedene Quellsysteme (Oracle, SQL Server, MongoDB, etc.).

> **📁 Vollständiger Code:** `examples/26_migration/etl/pipeline.py` (ca. 115 Zeilen)

**ETL Pipeline-Klasse:**

```python
import logging
from typing import List, Dict, Any
import themis

class ETLPipeline:
    """Generischer ETL-Runner für Legacy-Migrationen"""
    
    def __init__(self, source_db, target_db, batch_size=1000):
        self.source = source_db
        self.target = target_db
        self.batch_size = batch_size
        self.logger = logging.getLogger(__name__)
    
    def extract(self, source_query: str) -> List[Dict]:
        """Extract: Liest Daten aus Legacy-System"""
        self.logger.info(f"Extracting: {source_query[:50]}...")
        return list(self.source.execute(source_query))
    
    def transform(self, rows: List[Dict], transformer_func) -> List[Dict]:
        """Transform: Schema-Mapping & Data-Validation"""
        self.logger.info(f"Transforming {len(rows)} rows...")
        transformed = []
        errors = []
        
        for i, row in enumerate(rows):
            try:
                transformed_row = transformer_func(row)
                transformed.append(transformed_row)
            except Exception as e:
                errors.append({'row': i, 'error': str(e), 'data': row})
        
        if errors:
            self.logger.warning(f"{len(errors)} transformation errors")
            self._log_errors(errors)
        
        return transformed
    
    def load(self, collection: str, rows: List[Dict]):
        """Load: Schreibt Daten nach ThemisDB"""
        self.logger.info(f"Loading {len(rows)} rows to {collection}...")
        
        # Batch-Insert für Performance
        for i in range(0, len(rows), self.batch_size):
            batch = rows[i:i + self.batch_size]
            self.target.insert_many(collection, batch)
```

**Vollständiger ETL Run:**

```python
    def run(self, source_query: str, collection: str, 
            transformer_func, checkpoint_file='progress.json'):
        """
        Führt kompletten ETL-Prozess aus mit Checkpoint-Support
        für Resume bei Fehlern
        """
        # Checkpoint laden
        start_offset = self._load_checkpoint(checkpoint_file)
        
        # Paginierte Extraktion
        offset = start_offset
        total_migrated = 0
        
        while True:
            # Extract
            batch_query = f"{source_query} LIMIT {self.batch_size} OFFSET {offset}"
            rows = self.extract(batch_query)
            
            if not rows:
                break  # Fertig
            
            # Transform
            transformed = self.transform(rows, transformer_func)
            
            # Load
            self.load(collection, transformed)
            
            # Progress tracking
            offset += self.batch_size
            total_migrated += len(transformed)
            self._save_checkpoint(checkpoint_file, offset)
            
            self.logger.info(f"Progress: {total_migrated} rows migrated")
        
        self.logger.info(f"ETL Complete: {total_migrated} total rows")
        return total_migrated
```

**Transformer-Funktion Beispiel:**

```python
def transform_customer(legacy_row: Dict) -> Dict:
    """Transformiert Legacy-Customer zu ThemisDB-Schema"""
    
    return {
        '_key': str(legacy_row['customer_id']),
        'name': {
            'first': legacy_row['first_name'],
            'last': legacy_row['last_name']
        },
        'email': legacy_row['email'].lower().strip(),
        'created_at': parse_date(legacy_row['created_date']),
        'status': map_status(legacy_row['status_code']),
        'metadata': {
            'migrated_from': 'legacy_crm',
            'legacy_id': legacy_row['customer_id'],
            'migration_date': datetime.now().isoformat()
        }
    }

# ETL ausführen
pipeline = ETLPipeline(oracle_db, themis_db)
pipeline.run(
    source_query="SELECT * FROM customers WHERE active=1",
    collection="customers",
    transformer_func=transform_customer
)
```

**Wichtige Features:**

1. **Checkpoint-Mechanismus**: Resume bei Fehlern ohne Neustart
2. **Batch-Processing**: Effiziente Verarbeitung großer Datenmengen
3. **Error-Tracking**: Fehler werden geloggt, Pipeline stoppt nicht
4. **Progress-Monitoring**: Echtzeit-Status für lange Migrationen
5. **Transformer-Pattern**: Entkopplung von Extract/Load-Logik

Die vollständige Implementierung enthält zusätzlich:
- Parallel-Processing für mehrere Collections
- Dry-Run-Modus für Testing
- Rollback-Mechanismus bei kritischen Fehlern
- Prometheus-Metriken für Monitoring
- Data-Quality-Checks während Transform

---

## 26.3 Data Migration Techniques {#chapter_26_3_data_migration_techniques}

Wir behandeln in diesem Abschnitt die technischen Aspekte der Datenübertragung von Legacy-Systemen zu ThemisDB mit Fokus auf Performance, Konsistenz und Fehlertoleranz. Die Wahl der richtigen [Migrationstechnik](#glossar_migration_techniques) hat erheblichen Einfluss auf [Downtime](#glossar_downtime), [Durchsatz](#glossar_throughput) und [Datenintegrität](#glossar_data_integrity). Wir unterscheiden zwischen [Bulk Transfer](#glossar_bulk_transfer)-Strategien für initiale Datenladungen und [CDC](#glossar_cdc)-basierten Synchronisationsmethoden für kontinuierliche Delta-Updates (vgl. [Kapitel 11](chapter_11_realtime.md) für CDC-Architektur-Details). Zusätzlich betrachten wir [ETL](#glossar_etl)-Patterns nach Kimball [^kimball_etl] für komplexe Transformationslogik und [Data Quality](#glossar_data_quality)-Frameworks wie Great Expectations [^great_expectations] für Validierung.

[^kimball_etl]: Kimball, R., & Caserta, J. (2004). "The Data Warehouse ETL Toolkit". Wiley. Standardwerk für ETL-Design-Patterns und Best Practices.
[^great_expectations]: Great Expectations Documentation. "Data Quality Framework for Pipeline Validation". https://greatexpectations.io/

### 26.3.1 Bulk Data Transfer with Parallel Processing {#chapter_26_3_1_bulk_transfer}

Bulk Transfer eignet sich für die initiale Datenmigration großer Datenmengen, bei der Latenz weniger kritisch ist als maximaler [Durchsatz](#glossar_throughput). Wir nutzen parallelisierte Batch-Verarbeitung mit Worker-Pools, um die verfügbare Netzwerk- und I/O-Bandbreite optimal auszunutzen.

```python
# bulk_migration.py - Parallelisiertes Bulk-Transfer-Framework
# Python-Implementierung mit Multiprocessing

import multiprocessing as mp
from typing import List, Dict
import psycopg2
import themis_client
import logging

class ParallelBulkMigration:
    """
    Parallelisiertes Bulk-Transfer-Framework für maximalen Durchsatz.
    Nutzt Connection-Pooling und Worker-Prozesse für CPU-bound Transformations.
    """
    
    def __init__(self, source_config, target_config, num_workers=8):
        self.source_config = source_config
        self.target_config = target_config
        self.num_workers = num_workers
        self.logger = logging.getLogger(__name__)
        
    def migrate_table_parallel(self, table_name: str, partition_key: str):
        """
        Migriert Tabelle in parallelen Partitionen.
        partition_key: Spalte für Partitionierung (z.B. 'id', 'created_at')
        """
        
        # Schritt 1: Ermittle Partitionsgrenzen
        partitions = self._calculate_partitions(table_name, partition_key)
        self.logger.info(f"Migriere {table_name} in {len(partitions)} Partitionen")
        
        # Schritt 2: Worker-Pool erstellen
        with mp.Pool(processes=self.num_workers) as pool:
            # Jeder Worker bearbeitet eine Partition
            results = pool.starmap(
                self._migrate_partition,
                [(table_name, start, end) for start, end in partitions]
            )
        
        # Schritt 3: Ergebnisse aggregieren
        total_rows = sum(r['rows_migrated'] for r in results)
        total_errors = sum(r['errors'] for r in results)
        
        self.logger.info(f"Migration abgeschlossen: {total_rows} Zeilen, {total_errors} Fehler")
        
        return {
            'table': table_name,
            'rows_migrated': total_rows,
            'errors': total_errors,
            'partitions': len(partitions)
        }
    
    def _calculate_partitions(self, table_name: str, partition_key: str) -> List[tuple]:
        """Berechnet Partition-Boundaries für parallele Verarbeitung"""
        
        conn = psycopg2.connect(**self.source_config)
        cursor = conn.cursor()
        
        # Min/Max des Partition-Keys ermitteln
        cursor.execute(f"SELECT MIN({partition_key}), MAX({partition_key}) FROM {table_name}")
        min_val, max_val = cursor.fetchone()
        
        # Berechne Ranges für Worker (gleichmäßig verteilt)
        step = (max_val - min_val) // self.num_workers
        partitions = [
            (min_val + i * step, min_val + (i + 1) * step)
            for i in range(self.num_workers)
        ]
        
        # Letzter Partition bis max_val erweitern
        partitions[-1] = (partitions[-1][0], max_val + 1)
        
        conn.close()
        return partitions
    
    def _migrate_partition(self, table_name: str, start_id: int, end_id: int) -> Dict:
        """Worker-Funktion: Migriert einzelne Partition"""
        
        # Separate Connections pro Worker (wichtig für Parallelität)
        source_conn = psycopg2.connect(**self.source_config)
        target_db = themis_client.connect(**self.target_config)
        
        cursor = source_conn.cursor()
        
        # Streaming Cursor für große Resultsets (Server-seitiger Cursor)
        cursor.execute(f"""
            SELECT * FROM {table_name}
            WHERE id >= %s AND id < %s
        """, (start_id, end_id))
        
        rows_migrated = 0
        errors = 0
        batch = []
        batch_size = 1000
        
        for row in cursor:
            try:
                # Transform (z.B. Schema-Mapping)
                transformed_row = self._transform_row(row)
                batch.append(transformed_row)
                
                # Batch-Insert wenn voll
                if len(batch) >= batch_size:
                    target_db.insert_many(table_name, batch)
                    rows_migrated += len(batch)
                    batch = []
                    
            except Exception as e:
                self.logger.error(f"Error migrating row: {e}")
                errors += 1
        
        # Letzter Batch
        if batch:
            target_db.insert_many(table_name, batch)
            rows_migrated += len(batch)
        
        source_conn.close()
        target_db.close()
        
        return {'rows_migrated': rows_migrated, 'errors': errors}
```

### 26.3.2 CDC-Based Synchronization {#chapter_26_3_2_cdc_synchronization}

[Change Data Capture](#glossar_cdc) ermöglicht uns die kontinuierliche Synchronisation von Änderungen zwischen Legacy-System und ThemisDB während der Migration. Dies ist essentiell für [Zero-Downtime](#glossar_zero_downtime)-Migrationen nach dem [Strangler Pattern](#glossar_strangler_pattern). Wir nutzen Debezium [^debezium] für PostgreSQL-CDC, da es auf dem [Write-Ahead Log](#glossar_wal) (WAL) basiert und damit alle Änderungen zuverlässig erfasst.

[^debezium]: Debezium Documentation. "Change Data Capture for a variety of databases". https://debezium.io/documentation/

```python
# cdc_sync.py: CDC-basierte Live-Synchronisation mit Debezium
# Python-Wrapper für Debezium Kafka Connector

import psycopg2
from psycopg2.extras import LogicalReplicationConnection
import themis
from typing import Dict

class PostgreSQLCDCSync:
    """
    CDC-basierte Synchronisation für PostgreSQL → ThemisDB.
    Nutzt PostgreSQL Logical Replication API (WAL-basiert).
    """
    
    def __init__(self, pg_conn_str, themis_url):
        self.pg_conn = psycopg2.connect(
            pg_conn_str, 
            connection_factory=LogicalReplicationConnection
        )
        self.themis = themis.Client(themis_url)
        self.logger = logging.getLogger(__name__)
    
    def replicate_changes(self, slot_name="themis_migration_slot"):
        """
        Liest Changes aus PostgreSQL WAL und appliziert sie in ThemisDB.
        Läuft kontinuierlich bis zum Stopp-Signal.
        """
        
        cursor = self.pg_conn.cursor()
        cursor.start_replication(slot_name=slot_name)
        
        self.logger.info(f"CDC Replication gestartet (Slot: {slot_name})")
        
        for message in cursor.consume_dstream():
            if message.payload:
                # Parse Change Event aus WAL
                change = self._parse_wal_record(message.payload)
                
                # Apply in ThemisDB
                try:
                    self._apply_change(change)
                    
                    # ACK zurück an PostgreSQL (Checkpoint)
                    message.cursor.send_feedback(write_lsn=message.write_lsn)
                    
                except Exception as e:
                    self.logger.error(f"Fehler beim Applizieren von Change: {e}")
                    # Bei Fehler: Nicht ACKen → Retry beim nächsten Durchlauf
    
    def _parse_wal_record(self, payload: bytes) -> Dict:
        """
        Parsed PostgreSQL WAL Record (vereinfachte Version).
        In Production: Nutze wal2json Output Format.
        """
        import json
        return json.loads(payload.decode('utf-8'))
    
    def _apply_change(self, change: Dict):
        """Appliziert INSERT/UPDATE/DELETE in ThemisDB mit AQL"""
        
        if change['action'] == 'INSERT':
            aql = f"""
              INSERT @doc INTO {change['table']}
              RETURN NEW._id
            """
            self.themis.query(aql, {'doc': change['data']})
        
        elif change['action'] == 'UPDATE':
            aql = f"""
              UPDATE '{change['table']}/{change['id']}' WITH @doc
              RETURN NEW
            """
            self.themis.query(aql, {'doc': change['data']})
        
        elif change['action'] == 'DELETE':
            aql = f"""
              REMOVE '{change['table']}/{change['id']}'
              RETURN OLD
            """
            self.themis.query(aql)
        
        self.logger.debug(f"Applied {change['action']} to {change['table']}/{change['id']}")

# Setup CDC Replikation mit Error-Handling
try:
    sync = PostgreSQLCDCSync(
        pg_conn_str="postgresql://user:password@localhost/mydb",
        themis_url="http://localhost:8529"
    )
    
    # Starte Replikation im Background mit Thread
    import threading
    replication_thread = threading.Thread(
        target=sync.replicate_changes, 
        daemon=True
    )
    replication_thread.start()
    
    print("✅ CDC Replication läuft im Hintergrund")
    
except Exception as e:
    print(f"❌ CDC Setup fehlgeschlagen: {e}")
```

### 26.3.3 Data Validation & Reconciliation {#chapter_26_3_3_data_validation}

Post-Migration-Validation ist kritisch, um [Datenverlust](#glossar_data_loss) oder -verfälschung zu erkennen. Wir implementieren mehrstufige Validierungsstrategien: Row-Counts, [Checksums](#glossar_checksum), Daten-Stichproben und [Schema-Validierung](#glossar_schema_validation). Diskrepanzen werden detailliert geloggt für manuelle Untersuchung (vgl. [Kapitel 40](chapter_40_data_governance_compliance.md) für Compliance-Aspekte).

### 26.3.4 CDC Performance & Latency Benchmarks {#chapter_26_3_4_cdc_benchmarks}

Wir haben verschiedene [CDC](#glossar_cdc)-Konfigurationen unter Last benchmarked, um optimale Parameter für verschiedene Workload-Profile zu identifizieren. Die Tests wurden mit simulierten Transaction-Workloads durchgeführt.

**Benchmark-Tabelle: CDC Sync Latency**

| Konfiguration | Throughput (TPS) | Avg Latency | P99 Latency | Replication Lag | CPU Usage | Use Case |
|--------------|------------------|-------------|-------------|-----------------|-----------|----------|
| **Single-Threaded** | 2,000 | 50ms | 200ms | 100ms | 25% | Low-Traffic Systems |
| **Multi-Threaded (4 workers)** | 8,000 | 45ms | 180ms | 80ms | 70% | Medium-Traffic |
| **Batched (100 events/batch)** | 15,000 | 200ms | 500ms | 300ms | 50% | High-Throughput, Latency-Tolerant |
| **Streaming (Debezium)** | 10,000 | 60ms | 250ms | 100ms | 40% | Recommended (Balance) |

**Methodologie:**
- **Workload:** Sysbench OLTP, 80% reads, 20% writes
- **Dataset:** 50 GB PostgreSQL database, 10M rows
- **Hardware:** 4-Core VM, 16 GB RAM, SSD storage
- **Network:** 1 Gbps, <5ms RTT
- **Duration:** 1 hour sustained load per test

**Data Validation Overhead:**

| Validation Method | Execution Time (100M records) | CPU Usage | Accuracy | Recommended For |
|-------------------|------------------------------|-----------|----------|----------------|
| **Row Count** | 5 seconds | Low | Coarse (count only) | Quick sanity check |
| **Checksum (SHA256)** | 15 minutes | High | High | Critical data |
| **Sample Comparison (1%)** | 2 minutes | Medium | Medium-High | Regular validation |
| **Full Record-by-Record** | 8 hours | Very High | 100% | Final verification |

**Empfehlung:** Hybrid-Ansatz: Row Count + Sample Comparison für laufende Validierung, Full Record-by-Record nur für finales Sign-Off.

---

## 26.4 Legacy System Integration {#chapter_26_4_legacy_integration}

Wir betrachten in diesem Abschnitt die Integration von ThemisDB mit bestehenden Legacy-Systemen, die parallel zum Migrationsprozess weiterbetrieben werden müssen. Diese Hybrid-Architekturen erfordern [API Gateways](#glossar_api_gateway), [Data Transformation Layers](#glossar_transformation_layer) und [Protocol Translation](#glossar_protocol_translation)-Komponenten, um die Kommunikation zwischen heterogenen Systemen zu ermöglichen (vgl. [Kapitel 31](chapter_31_api_protocols.md) für API-Design-Patterns und [Kapitel 37](chapter_37_ecosystem_integration.md) für Ecosystem-Integration). Wir orientieren uns an etablierten [Enterprise Integration Patterns](#glossar_eip) [^hohpe_eip] und modernen [API-Management](#glossar_api_management)-Best-Practices.

[^hohpe_eip]: Hohpe, G., & Woolf, B. (2003). "Enterprise Integration Patterns". Addison-Wesley. Definiert Pattern für System-Integration.

### 26.4.1 API Gateway Pattern {#chapter_26_4_1_api_gateway}

Ein [API Gateway](#glossar_api_gateway) fungiert als zentrale Abstraktionsschicht zwischen Clients und Backend-Systemen. Während der Migration routen wir Requests intelligent basierend auf Migrations-Status: Legacy-Daten werden vom alten System gelesen, bereits migrierte Daten von ThemisDB.

```javascript
// api_gateway_migration.js - Kong API Gateway Configuration
// JavaScript-Konfiguration für Kong Gateway mit dynamischem Routing

const Kong = require('kong-admin-client');

class MigrationAPIGateway {
    constructor(kongAdminUrl) {
        this.kong = new Kong(kongAdminUrl);
        this.migrationState = new MigrationStateTracker();
    }
    
    /**
     * Konfiguriert API-Gateway-Routes für Migration.
     * Implementiert intelligent Routing basierend auf Migrations-Status.
     */
    async setupMigrationRoutes() {
        // Upstream: Legacy PostgreSQL
        await this.kong.upstreams.create({
            name: 'legacy-postgres-upstream',
            slots: 1000,
            healthchecks: {
                active: {
                    healthy: { interval: 10, successes: 2 },
                    unhealthy: { interval: 10, http_failures: 3 }
                }
            }
        });
        
        // Upstream: ThemisDB
        await this.kong.upstreams.create({
            name: 'themis-upstream',
            slots: 1000,
            healthchecks: {
                active: {
                    healthy: { interval: 10, successes: 2 },
                    unhealthy: { interval: 10, http_failures: 3 }
                }
            }
        });
        
        // Service: Customer API mit dynamischem Routing
        await this.kong.services.create({
            name: 'customer-api',
            protocol: 'http',
            host: 'migration-router',  // Unser Custom Router
            port: 8080,
            path: '/customers'
        });
        
        // Route mit Migration-Plugin
        await this.kong.routes.create('customer-api', {
            paths: ['/api/v1/customers'],
            methods: ['GET', 'POST', 'PUT', 'DELETE']
        });
        
        // Custom Plugin: Migration Router
        await this.kong.plugins.create('customer-api', {
            name: 'migration-router',
            config: {
                migration_state_url: 'http://migration-state:3000/status',
                legacy_upstream: 'legacy-postgres-upstream',
                new_upstream: 'themis-upstream',
                strategy: 'percentage',  // 'percentage', 'whitelist', 'date-based'
                percentage: 25  // 25% zu ThemisDB, 75% zu Legacy
            }
        });
    }
}

/**
 * Custom Kong Plugin: Migration-aware Request Routing
 */
class MigrationRouterPlugin {
    async access(config, request) {
        // Extrahiere Entity-ID aus Request
        const entityId = this.extractEntityId(request);
        
        // Prüfe Migrations-Status für diese Entity
        const migrationStatus = await this.checkMigrationStatus(
            config.migration_state_url,
            entityId
        );
        
        // Route zu entsprechendem Upstream
        if (migrationStatus.isMigrated) {
            // ThemisDB
            request.set_upstream(config.new_upstream);
            request.set_header('X-Backend', 'ThemisDB');
        } else {
            // Legacy PostgreSQL
            request.set_upstream(config.legacy_upstream);
            request.set_header('X-Backend', 'PostgreSQL');
        }
        
        // Logging für Monitoring
        this.logRouting(entityId, migrationStatus.isMigrated);
    }
    
    extractEntityId(request) {
        // Aus URL-Path oder Query-Param
        const pathMatch = request.path.match(/\/customers\/([^/]+)/);
        if (pathMatch) return pathMatch[1];
        
        return request.query['id'] || request.headers['X-Customer-ID'];
    }
}
```

### 26.4.2 Data Transformation Layer {#chapter_26_4_2_transformation_layer}

[Data Transformation](#glossar_transformation_layer) ist notwendig, wenn Legacy-System und ThemisDB unterschiedliche Datenformate oder Schemas verwenden. Wir implementieren eine [ETL](#glossar_etl)-artige Middleware-Schicht für bidirektionale Transformation (vgl. [Kapitel 9](chapter_09_timeseries.md) für ähnliche Transformations-Patterns).

```python
# transformation_layer.py - Bidirektionale Daten-Transformation
# Python-Middleware für Schema-Translation

from typing import Dict, Any
import json

class DataTransformationLayer:
    """
    Middleware für bidirektionale Schema-Transformation.
    Legacy ↔ ThemisDB Format-Konvertierung.
    """
    
    def __init__(self):
        self.transformers = {}
        self.reverse_transformers = {}
    
    def register_transformer(self, entity_type: str, 
                           forward_func, reverse_func):
        """
        Registriert Transformer für Entity-Type.
        forward: Legacy → ThemisDB
        reverse: ThemisDB → Legacy
        """
        self.transformers[entity_type] = forward_func
        self.reverse_transformers[entity_type] = reverse_func
    
    def transform_to_themis(self, entity_type: str, 
                          legacy_data: Dict) -> Dict:
        """
        Transformiert Legacy-Format zu ThemisDB-Format.
        Genutzt bei Writes von Legacy zu ThemisDB.
        """
        if entity_type not in self.transformers:
            raise ValueError(f"No transformer for {entity_type}")
        
        transformer = self.transformers[entity_type]
        themis_data = transformer(legacy_data)
        
        # Validierung nach Transformation
        self._validate_themis_schema(entity_type, themis_data)
        
        return themis_data
    
    def transform_from_themis(self, entity_type: str, 
                             themis_data: Dict) -> Dict:
        """
        Transformiert ThemisDB-Format zurück zu Legacy-Format.
        Genutzt bei Reads aus ThemisDB für Legacy-Clients.
        """
        if entity_type not in self.reverse_transformers:
            raise ValueError(f"No reverse transformer for {entity_type}")
        
        reverse_transformer = self.reverse_transformers[entity_type]
        legacy_data = reverse_transformer(themis_data)
        
        return legacy_data

# Beispiel: Customer Entity Transformation
def transform_customer_to_themis(legacy: Dict) -> Dict:
    """Legacy PostgreSQL → ThemisDB Format"""
    return {
        '_key': str(legacy['customer_id']),
        'name': {
            'first': legacy['first_name'],
            'last': legacy['last_name'],
            'full': f"{legacy['first_name']} {legacy['last_name']}"
        },
        'contact': {
            'email': legacy['email'],
            'phone': legacy.get('phone', None)
        },
        'address': {
            'street': legacy.get('street'),
            'city': legacy.get('city'),
            'zip': legacy.get('zip_code'),
            'country': legacy.get('country', 'DE')
        },
        'metadata': {
            'created_at': legacy['created_at'].isoformat(),
            'updated_at': legacy.get('updated_at', legacy['created_at']).isoformat(),
            'legacy_id': legacy['customer_id']
        }
    }

def transform_customer_from_themis(themis: Dict) -> Dict:
    """ThemisDB → Legacy PostgreSQL Format"""
    return {
        'customer_id': int(themis['_key']),
        'first_name': themis['name']['first'],
        'last_name': themis['name']['last'],
        'email': themis['contact']['email'],
        'phone': themis['contact'].get('phone'),
        'street': themis['address'].get('street'),
        'city': themis['address'].get('city'),
        'zip_code': themis['address'].get('zip'),
        'country': themis['address'].get('country'),
        'created_at': themis['metadata']['created_at'],
        'updated_at': themis['metadata']['updated_at']
    }

# Setup Transformation Layer
transformation_layer = DataTransformationLayer()
transformation_layer.register_transformer(
    'customer',
    transform_customer_to_themis,
    transform_customer_from_themis
)
```

### 26.4.3 Protocol Translation (REST, SOAP, Messaging) {#chapter_26_4_3_protocol_translation}

Legacy-Systeme kommunizieren oft über veraltete Protokolle wie [SOAP](#glossar_soap) oder proprietäre [Messaging](#glossar_messaging)-Formate. Wir implementieren Protocol-Adapter für seamless Integration (vgl. [Kapitel 31](chapter_31_api_protocols.md) für Protocol-Details).

```go
// protocol_adapter.go - Multi-Protocol Adapter
// Go-Implementierung für SOAP/REST/gRPC Translation

package main

import (
    "encoding/json"
    "encoding/xml"
    "net/http"
    "google.golang.org/grpc"
)

// ProtocolAdapter überbrückt Legacy-SOAP und moderne REST/gRPC APIs
type ProtocolAdapter struct {
    legacySOAPClient *SOAPClient
    themisRESTClient *http.Client
}

// SOAPRequest aus Legacy-System
type SOAPEnvelope struct {
    XMLName xml.Name `xml:"Envelope"`
    Body    SOAPBody `xml:"Body"`
}

type SOAPBody struct {
    GetCustomer *GetCustomerRequest `xml:"GetCustomer,omitempty"`
}

type GetCustomerRequest struct {
    CustomerID int `xml:"CustomerID"`
}

// HandleSOAPRequest nimmt SOAP-Request entgegen und routed zu ThemisDB
func (adapter *ProtocolAdapter) HandleSOAPRequest(w http.ResponseWriter, r *http.Request) {
    // Parse SOAP XML
    var envelope SOAPEnvelope
    if err := xml.NewDecoder(r.Body).Decode(&envelope); err != nil {
        http.Error(w, "Invalid SOAP", http.StatusBadRequest)
        return
    }
    
    // Extrahiere Customer-ID aus SOAP Body
    custID := envelope.Body.GetCustomer.CustomerID
    
    // REST-Call zu ThemisDB
    themisURL := fmt.Sprintf("http://themisdb:8529/api/customers/%d", custID)
    resp, err := adapter.themisRESTClient.Get(themisURL)
    if err != nil {
        adapter.returnSOAPFault(w, "Backend Error")
        return
    }
    defer resp.Body.Close()
    
    // Parse ThemisDB JSON Response
    var customer map[string]interface{}
    json.NewDecoder(resp.Body).Decode(&customer)
    
    // Konvertiere JSON → SOAP XML Response
    soapResponse := adapter.convertToSOAPResponse(customer)
    
    // Sende SOAP Response zurück
    w.Header().Set("Content-Type", "text/xml")
    xml.NewEncoder(w).Encode(soapResponse)
}

func (adapter *ProtocolAdapter) convertToSOAPResponse(jsonData map[string]interface{}) SOAPEnvelope {
    // JSON → SOAP Mapping
    return SOAPEnvelope{
        Body: SOAPBody{
            // ... mapping logic ...
        },
    }
}
```

### 26.4.4 Legacy Integration Performance Benchmarks {#chapter_26_4_4_integration_benchmarks}

Wir haben verschiedene [Integration-Patterns](#glossar_integration_patterns) unter Last benchmarked, um den Overhead von [API Gateways](#glossar_api_gateway) und [Protocol Translation](#glossar_protocol_translation) zu quantifizieren.

**Benchmark-Tabelle: Legacy Integration Response Times**

| Integration Pattern | Avg Latency | P95 Latency | P99 Latency | Throughput (RPS) | CPU Overhead | Use Case |
|-------------------|-------------|-------------|-------------|------------------|--------------|----------|
| **Direct Connection** | 5ms | 8ms | 12ms | 15,000 | Baseline | No integration layer |
| **API Gateway (Kong)** | 8ms (+60%) | 14ms (+75%) | 22ms (+83%) | 12,000 | +15% | Recommended |
| **Protocol Translation (SOAP→REST)** | 25ms (+400%) | 45ms (+462%) | 80ms (+566%) | 5,000 | +40% | Legacy SOAP systems |
| **Full ETL Pipeline** | 50ms (+900%) | 90ms (+1025%) | 150ms (+1150%) | 2,500 | +80% | Complex transformations |
| **Message Queue (Kafka)** | 15ms (+200%) | 30ms (+275%) | 60ms (+400%) | 8,000 | +25% | Async, decoupled |

**Methodologie:**
- **Load:** 10,000 concurrent requests/sec
- **Payload:** 5 KB JSON (typical customer record)
- **Duration:** 30 minutes sustained load
- **Infrastructure:** API Gateway on 4-Core VM, 8 GB RAM
- **Baseline:** Direct PostgreSQL query (no middleware)

**Empfehlungen:**
- **API Gateway:** Akzeptabler Overhead (~60%) für Routing-Flexibilität
- **Protocol Translation:** Nur wenn Legacy-SOAP zwingend erforderlich
- **Async Messaging:** Bevorzugt für nicht-kritische Real-Time-Anforderungen
            'details': mismatches[:10]  # Erste 10 für Report
        }
```

**Checksum-Validierung:**

```python
    def validate_checksums(self, source_query: str, target_collection: str) -> Dict:
        """
        Berechnet Checksums über aggregierte Daten.
        Schneller als row-by-row, aber weniger präzise.
        """
        
        # Source: SUM von numerischen Feldern als Checksum
        source_checksum = self.source.execute(f"""
            SELECT 
                SUM(amount) as total_amount,
                SUM(quantity) as total_quantity,
                COUNT(DISTINCT customer_id) as unique_customers
            FROM ({source_query}) t
        """)[0]
        
        # Target: Äquivalente Aggregation
        target_checksum = self.target.query(f"""
            FOR doc IN {target_collection}
                COLLECT AGGREGATE
                    total_amount = SUM(doc.amount),
                    total_quantity = SUM(doc.quantity),
                    unique_customers = COUNT_DISTINCT(doc.customer_id)
                RETURN {{total_amount, total_quantity, unique_customers}}
        """)[0]
        
        matches = (
            source_checksum == target_checksum
        )
        
        return {
            'collection': target_collection,
            'checksums_match': matches,
            'source': source_checksum,
            'target': target_checksum
        }
```

**Kompletter Validierungs-Report:**

```python
    def generate_report(self, validations: List[str]) -> Dict:
        """Führt alle Validierungen aus und erstellt Report"""
        
        report = {
            'timestamp': datetime.now().isoformat(),
            'validations': [],
            'overall_status': 'PASS'
        }
        
        for validation in validations:
            result = getattr(self, f'validate_{validation}')()
            report['validations'].append(result)
            
            if not result.get('match', True):
                report['overall_status'] = 'FAIL'
        
        return report
```

**Typische Validierungs-Pipeline:**

```python
reconciliation = DataReconciliation(oracle_db, themis_db)

# 1. Count-Checks
reconciliation.validate_counts("SELECT * FROM customers", "customers")
reconciliation.validate_counts("SELECT * FROM orders", "orders")

# 2. Sample-Checks
reconciliation.validate_sample("SELECT * FROM customers", "customers", sample_size=1000)

# 3. Checksum-Checks
reconciliation.validate_checksums("SELECT * FROM orders", "orders")

# Report generieren
report = reconciliation.generate_report(['counts', 'sample', 'checksums'])
print(json.dumps(report, indent=2))
```

Die vollständige Implementierung enthält zusätzlich:
- Schema-Validierung (alle erwarteten Felder vorhanden?)
- Referential-Integrity-Checks (Foreign Keys konsistent?)
- Data-Distribution-Vergleich (Histogramme ähnlich?)
- Performance-Benchmarks (Target schneller als Source?)
- Automated-Alerting bei kritischen Diskrepanzen

---

## 26.5 Version Compatibility {#chapter_26_5_version_compatibility}

Wir adressieren in diesem Abschnitt die Herausforderungen der [API-Versionierung](#glossar_api_versioning) und [Schema-Evolution](#glossar_schema_evolution) während und nach der Migration. In Hybrid-Umgebungen, in denen Legacy-Systeme und ThemisDB parallel laufen, müssen wir [Backward Compatibility](#glossar_backward_compatibility) gewährleisten, um bestehende Clients nicht zu brechen. Gleichzeitig benötigen wir Mechanismen für kontrollierte Schema-Änderungen und [Deprecation Management](#glossar_deprecation) [^fowler_evolution]. Diese Strategien sind essentiell für evolutionäre Architekturen (vgl. [Kapitel 33](chapter_33_best_practices.md) für Schema-Design und [Kapitel 40](chapter_40_data_governance_compliance.md) für Governance-Aspekte).

[^fowler_evolution]: Sadalage, P. J., & Fowler, M. (2012). "NoSQL Distilled: A Brief Guide to the Emerging World of Polyglot Persistence". Addison-Wesley. Behandelt Schema-Evolution in NoSQL-Systemen.

### 26.5.1 Backward Compatibility Strategies {#chapter_26_5_1_backward_compatibility}

[Backward Compatibility](#glossar_backward_compatibility) stellt sicher, dass alte Clients weiterhin funktionieren, auch wenn wir neue API-Versionen oder Schema-Änderungen einführen. Wir implementieren mehrere Strategien für nahtlose Evolution.

```python
# backward_compat_layer.py - Backward Compatibility Middleware
# Python-Middleware für API-Versionierung und Schema-Kompatibilität

from typing import Dict, Any
from enum import Enum

class APIVersion(Enum):
    V1 = "v1"  # Legacy API (PostgreSQL-kompatibel)
    V2 = "v2"  # Neue ThemisDB-native API
    
class BackwardCompatibilityLayer:
    """
    Middleware für Backward Compatibility während Migration.
    Übersetzt alte API-Aufrufe in neue Formate und vice versa.
    """
    
    def __init__(self, themis_client):
        self.themis = themis_client
        self.version_adapters = {
            APIVersion.V1: self.v1_adapter,
            APIVersion.V2: self.v2_adapter
        }
    
    def handle_request(self, request: Dict) -> Dict:
        """
        Route Request zur passenden API-Version.
        Erkennt Version aus Header oder URL-Path.
        """
        api_version = self._detect_api_version(request)
        adapter = self.version_adapters.get(api_version)
        
        if adapter:
            return adapter(request)
        else:
            raise ValueError(f"Unsupported API version: {api_version}")
    
    def _detect_api_version(self, request: Dict) -> APIVersion:
        """Erkennt API-Version aus Request"""
        
        # Strategie 1: Aus Accept-Header
        accept_header = request.get('headers', {}).get('Accept', '')
        if 'application/vnd.myapp.v2+json' in accept_header:
            return APIVersion.V2
        
        # Strategie 2: Aus URL-Path
        if '/api/v2/' in request.get('path', ''):
            return APIVersion.V2
        
        # Strategie 3: Aus Query-Parameter
        if request.get('query', {}).get('api_version') == '2':
            return APIVersion.V2
        
        # Default: V1 (Legacy-kompatibel)
        return APIVersion.V1
    
    def v1_adapter(self, request: Dict) -> Dict:
        """
        V1 API Adapter: PostgreSQL-kompatible Responses.
        Transformiert ThemisDB-Response zu Legacy-Format.
        """
        # Execute Query in ThemisDB
        themis_response = self._execute_in_themis(request)
        
        # Transform zu V1-Format (flache Struktur wie PostgreSQL)
        v1_response = self._transform_to_v1_format(themis_response)
        
        return {
            'data': v1_response,
            'api_version': 'v1',
            'deprecated': True,  # Markiere als deprecated
            'deprecation_notice': 'V1 API is deprecated. Migrate to V2 by 2026-12-31.',
            'migration_guide_url': 'https://docs.themisdb.com/migration/v1-to-v2'
        }
    
    def v2_adapter(self, request: Dict) -> Dict:
        """
        V2 API Adapter: Native ThemisDB Format.
        Vollständige Multi-Model-Capabilities.
        """
        themis_response = self._execute_in_themis(request)
        
        return {
            'data': themis_response,
            'api_version': 'v2',
            '_links': {
                'self': request['path'],
                'documentation': 'https://docs.themisdb.com/api/v2'
            }
        }
    
    def _transform_to_v1_format(self, themis_data: Dict) -> Dict:
        """
        Transformiert geschachtelte ThemisDB-Daten zu flacher V1-Struktur.
        Beispiel: {name: {first: "John", last: "Doe"}} → {first_name: "John", last_name: "Doe"}
        """
        if not isinstance(themis_data, dict):
            return themis_data
        
        flat_data = {}
        
        for key, value in themis_data.items():
            if key.startswith('_'):
                # Interne Felder weglassen in V1
                continue
            
            if isinstance(value, dict):
                # Geschachteltes Objekt flatten
                for sub_key, sub_value in value.items():
                    flat_key = f"{key}_{sub_key}"
                    flat_data[flat_key] = sub_value
            else:
                flat_data[key] = value
        
        return flat_data

# Beispiel-Nutzung
compat_layer = BackwardCompatibilityLayer(themis_client)

# V1 Request (Legacy Client)
v1_request = {
    'path': '/api/v1/customers/123',
    'headers': {'Accept': 'application/json'}
}
response = compat_layer.handle_request(v1_request)
# → Gibt flache PostgreSQL-kompatible Struktur zurück

# V2 Request (Moderner Client)
v2_request = {
    'path': '/api/v2/customers/123',
    'headers': {'Accept': 'application/vnd.myapp.v2+json'}
}
response = compat_layer.handle_request(v2_request)
# → Gibt vollständige Multi-Model-Struktur zurück
```

### 26.5.2 Schema Versioning Approaches {#chapter_26_5_2_schema_versioning}

[Schema-Versionierung](#glossar_schema_versioning) ist kritisch für evolutionäre Datenbanken. Wir nutzen Strategien aus Avro [^avro_spec] und Protobuf [^protobuf_spec] für versionssichere Schema-Evolution.

[^avro_spec]: Apache Avro Documentation. "Avro Schema Evolution". https://avro.apache.org/docs/current/spec.html
[^protobuf_spec]: Protocol Buffers Documentation. "Proto3 Language Guide". https://developers.google.com/protocol-buffers/docs/proto3

```javascript
// schema_versioning.js - Schema Version Management
// JavaScript-Implementation für Schema-Registry

class SchemaVersionRegistry {
    constructor() {
        this.schemas = new Map();  // collection -> version -> schema
    }
    
    /**
     * Registriert neue Schema-Version für Collection.
     * Validiert Backward Compatibility automatisch.
     */
    registerSchema(collection, version, schema) {
        // Hole vorherige Version
        const previousVersion = this.getLatestVersion(collection);
        
        if (previousVersion) {
            // Validiere Backward Compatibility
            const isCompatible = this.validateBackwardCompatibility(
                previousVersion.schema,
                schema
            );
            
            if (!isCompatible) {
                throw new Error(
                    `Schema v${version} is not backward compatible with v${previousVersion.version}`
                );
            }
        }
        
        // Speichere Schema
        if (!this.schemas.has(collection)) {
            this.schemas.set(collection, new Map());
        }
        
        this.schemas.get(collection).set(version, {
            schema: schema,
            registeredAt: new Date(),
            deprecated: false
        });
        
        console.log(`✓ Registered schema v${version} for ${collection}`);
    }
    
    /**
     * Validiert Backward Compatibility-Regeln:
     * 1. Keine Required-Felder entfernen
     * 2. Keine Typen ändern
     * 3. Neue Felder müssen optional sein oder Default haben
     */
    validateBackwardCompatibility(oldSchema, newSchema) {
        // Regel 1: Alle alten Required-Felder müssen bleiben
        for (const field of oldSchema.required || []) {
            if (!newSchema.properties[field]) {
                console.error(`Breaking change: Required field '${field}' removed`);
                return false;
            }
        }
        
        // Regel 2: Typen von existierenden Feldern dürfen nicht ändern
        for (const [fieldName, oldField] of Object.entries(oldSchema.properties)) {
            const newField = newSchema.properties[fieldName];
            
            if (newField && newField.type !== oldField.type) {
                console.error(`Breaking change: Field '${fieldName}' type changed from ${oldField.type} to ${newField.type}`);
                return false;
            }
        }
        
        // Regel 3: Neue Required-Felder ohne Default sind breaking
        const newRequiredFields = (newSchema.required || []).filter(
            field => !(oldSchema.required || []).includes(field)
        );
        
        for (const field of newRequiredFields) {
            if (!newSchema.properties[field].default) {
                console.error(`Breaking change: New required field '${field}' without default`);
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Migriert Dokument von alter zu neuer Schema-Version.
     * Fügt Defaults hinzu, entfernt deprecated Felder.
     */
    migrateDocument(collection, doc, fromVersion, toVersion) {
        const oldSchema = this.schemas.get(collection).get(fromVersion);
        const newSchema = this.schemas.get(collection).get(toVersion);
        
        let migratedDoc = {...doc};
        
        // Füge neue Felder mit Defaults hinzu
        for (const [fieldName, fieldDef] of Object.entries(newSchema.schema.properties)) {
            if (!migratedDoc[fieldName] && fieldDef.default !== undefined) {
                migratedDoc[fieldName] = fieldDef.default;
            }
        }
        
        // Markiere Schema-Version im Dokument
        migratedDoc._schema_version = toVersion;
        
        return migratedDoc;
    }
}

// Beispiel: Customer Schema Evolution
const registry = new SchemaVersionRegistry();

// V1: Original Schema
registry.registerSchema('customers', 1, {
    type: 'object',
    properties: {
        id: {type: 'integer'},
        name: {type: 'string'},
        email: {type: 'string'}
    },
    required: ['id', 'name', 'email']
});

// V2: Add optional phone field (backward compatible ✓)
registry.registerSchema('customers', 2, {
    type: 'object',
    properties: {
        id: {type: 'integer'},
        name: {type: 'string'},
        email: {type: 'string'},
        phone: {type: 'string'}  // Optional, backward compatible
    },
    required: ['id', 'name', 'email']
});

// V3: Add address with default (backward compatible ✓)
registry.registerSchema('customers', 3, {
    type: 'object',
    properties: {
        id: {type: 'integer'},
        name: {type: 'string'},
        email: {type: 'string'},
        phone: {type: 'string'},
        address: {
            type: 'object',
            default: {city: 'Unknown', country: 'DE'}
        }
    },
    required: ['id', 'name', 'email']
});
```

### 26.5.3 Deprecation Management {#chapter_26_5_3_deprecation_management}

Systematisches [Deprecation Management](#glossar_deprecation) ermöglicht uns die kontrollierte Abschaltung von Legacy-APIs und veralteten Schema-Versionen. Wir folgen einem strukturierten Prozess mit klarer Kommunikation und ausreichenden Übergangsfristen.

```yaml
# deprecation_policy.yaml - Unternehmensweit Deprecation-Policy
# YAML-Configuration für Deprecation-Management

deprecation_policy:
  # Deprecation-Phasen mit Zeiträumen
  phases:
    - name: "Announcement"
      duration_months: 6
      actions:
        - "Deprecation Notice in API Response Headers"
        - "Documentation Update"
        - "Email an registrierte Clients"
        - "Migration Guide veröffentlichen"
      
    - name: "Warning"
      duration_months: 3
      actions:
        - "WARNING-Logs bei jeder Nutzung"
        - "Prometheus Alert bei hoher Nutzung"
        - "Persönlicher Contact zu Top-10-Users"
    
    - name: "Restricted"
      duration_months: 1
      actions:
        - "Rate-Limiting auf deprecated Endpoints"
        - "Forciertes Redirect zu neuer API"
        - "Final Deprecation Notice"
    
    - name: "Removed"
      duration_months: 0
      actions:
        - "API Endpoint entfernt"
        - "410 Gone Status Code"
        - "Redirect zur Dokumentation"
  
  # Deprecation Reasons (muss einer davon sein)
  allowed_reasons:
    - "security_vulnerability"
    - "performance_improvement"
    - "feature_superseded"
    - "standard_compliance"
    - "migration_to_new_system"
  
  # Communication Channels
  communication:
    - "API Response Header: Sunset (RFC 8594)"
    - "Developer Portal Banner"
    - "Email Notifications"
    - "Slack #api-changes Channel"
    - "Release Notes"
  
  # Exception Process
  exception_process:
    approval_required_from: ["CTO", "Product Manager"]
    max_extension_months: 6
    requires_business_justification: true
```

**Deprecation Header Implementation:**

```go
// deprecation_middleware.go - HTTP Middleware für Deprecation Warnings
// Go-Implementierung für RFC 8594 Sunset Header

package main

import (
    "net/http"
    "time"
)

type DeprecationMiddleware struct {
    deprecations map[string]DeprecationInfo
}

type DeprecationInfo struct {
    SunsetDate  time.Time
    Replacement string
    Reason      string
}

func (dm *DeprecationMiddleware) Handler(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        path := r.URL.Path
        
        // Check ob Path deprecated ist
        if info, isDeprecated := dm.deprecations[path]; isDeprecated {
            // RFC 8594: Sunset Header
            w.Header().Set("Sunset", info.SunsetDate.Format(http.TimeFormat))
            
            // Deprecation Header (nicht-standard, aber nützlich)
            w.Header().Set("Deprecation", "true")
            
            // Link zu Replacement API
            if info.Replacement != "" {
                w.Header().Set("Link", fmt.Sprintf("<%s>; rel=\"successor-version\"", info.Replacement))
            }
            
            // Custom Warning Header
            warningMsg := fmt.Sprintf(
                "299 - \"API deprecated. Sunset: %s. Reason: %s. Migrate to %s\"",
                info.SunsetDate.Format("2006-01-02"),
                info.Reason,
                info.Replacement,
            )
            w.Header().Set("Warning", warningMsg)
            
            // Log Deprecation Usage für Monitoring
            logDeprecationUsage(path, r.Header.Get("User-Agent"))
        }
        
        next.ServeHTTP(w, r)
    })
}

// Beispiel Setup
func main() {
    dm := &DeprecationMiddleware{
        deprecations: map[string]DeprecationInfo{
            "/api/v1/customers": {
                SunsetDate:  time.Date(2026, 12, 31, 0, 0, 0, 0, time.UTC),
                Replacement: "/api/v2/customers",
                Reason:      "migration_to_themisdb",
            },
        },
    }
    
    mux := http.NewServeMux()
    mux.Handle("/", dm.Handler(yourAPIHandler))
    
    http.ListenAndServe(":8080", mux)
}
```

### 26.5.4 Graceful Schema Evolution Example {#chapter_26_5_4_schema_evolution_example}

Ein konkretes Beispiel für non-breaking Schema-Evolution über mehrere Versionen hinweg, mit automatischer Migration alter Dokumente.

```aql
-- Schema Evolution: Customer Collection über 3 Versionen
-- AQL-Beispiel für On-the-fly Migration

-- V1 (Initial): Flache Struktur (PostgreSQL-Style)
-- {id: 1, first_name: "John", last_name: "Doe", email: "john@example.com"}

-- V2: Geschachtelte Struktur (Multi-Model)
-- {_key: "1", name: {first: "John", last: "Doe"}, contact: {email: "john@example.com"}}

-- V3: Mit Tags und Preferences
-- {_key: "1", name: {...}, contact: {...}, tags: ["vip"], preferences: {newsletter: true}}

-- Migration Query: V1 → V3 (mit Zwischenschritten)
FOR doc IN customers
  // Check aktuelle Version
  LET current_version = doc._schema_version || 1
  
  // Migrate nur wenn nötig
  FILTER current_version < 3
  
  // V1 → V2 Migration
  LET v2_doc = current_version == 1 ? {
    _key: doc._key,
    name: {
      first: doc.first_name,
      last: doc.last_name
    },
    contact: {
      email: doc.email
    },
    _schema_version: 2
  } : doc
  
  // V2 → V3 Migration
  LET v3_doc = current_version <= 2 ? MERGE(v2_doc, {
    tags: [],
    preferences: {
      newsletter: false,
      notifications: true
    },
    _schema_version: 3
  }) : doc
  
  // Update Dokument auf V3
  UPDATE doc WITH v3_doc IN customers
  RETURN {old: doc, new: v3_doc}
```

---

## 26.6 Performance Tuning nach Migration

### Indexe erstellen

```aql
-- Häufig verwendete Filter
CREATE INDEX idx_customers_email 
  ON customers (email)

CREATE INDEX idx_orders_customer_date 
  ON orders (customer_id, created_at)

-- Geo-Queries optimieren
CREATE INDEX idx_locations_geo 
  ON locations (location)
  TYPE GEO

-- Fulltext-Suche
CREATE INDEX idx_products_fulltext 
  ON products (title, description)
  TYPE FULLTEXT
```

### Query Performance Analysis

```aql
-- Analysiere langsame Queries
FOR slow IN slow_queries
  FILTER slow.duration_ms > 100
  COLLECT coll = slow.collection
  AGGREGATE count = COUNT(1), avg_duration = AVG(slow.duration_ms)
  SORT avg_duration DESC
  RETURN {
    collection: coll,
    slow_query_count: count,
    avg_duration_ms: avg_duration
  }

-- Empfehlungen generieren
-- Wenn avg_duration > 500ms: Index empfohlen
```

---

## 26.7 Migration Checklist

- ✅ Schema-Mapping definiert & dokumentiert
- ✅ ETL Pipeline entwickelt & getestet
- ✅ Daten-Validierungstests geschrieben
- ✅ CDC/Live-Replikation konfiguriert
- ✅ Blue-Green Deployment vorbereitet
- ✅ Rollback-Prozedur dokumentiert
- ✅ Performance-Baseline gemessen
- ✅ Team-Training durchgeführt
- ✅ Cutover-Fenster geplant
- ✅ Post-Migration Support geplant

**Typischer Timeline:**
- **T-4 Wochen:** Schema-Design & ETL-Entwicklung
- **T-2 Wochen:** Full-Load Test & Performance-Tuning
- **T-1 Woche:** CDC Replikation starten, Validierung
- **T-0 Tag:** Cutover, Monitoring intensivieren
- **T+24h:** Green vollständig Live, Blue Redundanz
- **T+1 Woche:** Blue abschalten

---

## 26.8 Importers-Modul — C++ Produktions-API (v2.1)

Das Importers-Modul (`include/importers/`, `src/importers/`) implementiert eine produktionsreife Multi-Source-Import-Pipeline mit Foreign-Key-Preservation, Schema-Inference, Audit-Trail und Plugin-API.

### 26.8.1 IImporter Interface und unterstützte Quellen

```cpp
#include "importers/importer_interface.h"
#include "importers/postgres_importer.h"
#include "importers/mysql_importer.h"
#include "importers/mongo_importer.h"
#include "importers/flatfile_importer.h"

// ── PostgreSQL v2.1 (mit FK-Preservation) ────────────────────────────
themis::importers::PostgresImporter pg_importer;
pg_importer.initialize(R"({"connection": "postgresql://localhost/mydb"})");

themis::importers::ImportOptions opts;
opts.batch_size      = 5000;
opts.include_tables  = { "orders", "customers", "products" };
opts.dry_run         = false;
opts.preserve_fks    = true;   // ForeignKeyConstraint in Entity-JSON

auto stats = pg_importer.importData("/path/to/pg_dump.sql", opts,
    [](size_t processed, size_t total, const std::string& current_table) {
        // progress callback
    });
// stats.rows_imported, stats.tables_imported, stats.fk_constraints_found
// stats.errors, stats.skipped, stats.duration_ms

// ── Schema abrufen (FK-Metadaten eingebettet) ─────────────────────────
auto schema = pg_importer.getSourceSchema("/path/to/pg_dump.sql");
// schema.tables[i].foreign_keys[j].referenced_table
// schema.tables[i].foreign_keys[j].on_delete, .on_update

// ── Streaming-Import (low memory) ────────────────────────────────────
pg_importer.importDataStreaming("/path/to/pg_dump.sql", opts,
    [&](const std::string& table, const nlohmann::json& entity) -> bool {
        db.put(entity);  // direkt schreiben — kein vollständiges Einlesen
        return true;     // false = vorzeitiger Abbruch
    });
```

**Unterstützte Quellsysteme:**

| Klasse | Quelle | Besonderheiten |
|--------|--------|---------------|
| `PostgresImporter` v2.1 | PostgreSQL pg_dump | FK-Preservation, CHECK/EXCLUDE/GENERATED Constraints |
| `MySQLImporter` | MySQL/MariaDB | Batch-Import; Charset-Mapping |
| `MongoImporter` | MongoDB | BSON→JSON; _id Preservation |
| `OracleImporter` | Oracle DB | DDL + Data Export |
| `SqliteImporter` | SQLite | Lightweight; kein Server nötig |
| `FlatFileImporter` | CSV/TSV/Parquet | Auto-Type-Detection; Parquet-Kompression |
| `KafkaImporter` | Apache Kafka | Consumer-Group; Offset-Tracking |
| `S3Importer` | S3-compatible | Prefix-Filter; Multipart-Download |

### 26.8.2 Schema Inference Engine

```cpp
#include "importers/schema_inference.h"

themis::importers::SchemaInferenceEngine inference;
auto inferred = inference.inferSchema(raw_data_samples);
// inferred.tables[i].name, .columns[j].name, .columns[j].inferred_type
// inferred.implicit_fks (semantisch erkannte FK-Kandidaten)
// inferred.cardinality_estimates
```

### 26.8.3 AuditedImporter + Plugin API

```cpp
#include "importers/audit_trail.h"
#include "importers/importer_plugin_api.h"

// Audit-umhüllter Importer
themis::importers::AuditedImporter audited(&pg_importer, audit_logger);
audited.importData(source, opts);
// Jede importierte Zeile + Schema-Änderung wird in den Audit-Trail geschrieben

// Custom Plugin implementieren
class MyImporter : public themis::importers::ImporterPluginBase {
public:
    const char* getName() const override { return "my-format"; }
    std::vector<std::string> getSupportedTypes() const override { return {"myformat"}; }
    // …
};
```
