# Kapitel 26: Migration & Legacy System Integration

> *"Legacy systems don't disappear. In enterprise environments, the art of seamless migration is what separates successful adoptions from failed projects."*

---

## Überblick

Migration von Legacy-Systemen (PostgreSQL, MongoDB, Neo4j) zu ThemisDB ist eine Kernaufgabe bei der Datenbank-Modernisierung. Dieses Kapitel zeigt pragmatische Strategien für zero-downtime Migrationen mit vollständiger Datenvalidierung.

**Was Sie in diesem Kapitel lernen:**
- Schema-Mappings (SQL → AQL)
- Daten-Extraktion & Transformation (ETL)
- Live-Replikation während Migration
- Daten-Validierung & Reconciliation
- Rollback-Strategien
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

Abb. 26.0: Migration-Strategie: Strangler Pattern

---

## 26.1 Schema-Mapping Strategien

### PostgreSQL → ThemisDB Mapping

```aql
-- SQL Table                          → AQL Collection
-- customers TABLE                    → customers collection
-- id INT PRIMARY KEY                 → _id (implicit, or _key)
-- name VARCHAR(255)                  → name: string
-- email VARCHAR(100) UNIQUE          → email: string (with unique index)
-- created_at TIMESTAMP               → created_at: date
-- FOREIGN KEY orders(customer_id)    → Document-Referenzen

-- Beispiel-Transformation:
FUNCTION transform_postgresql_customer(sql_row) {
  RETURN {
    _key: STRING(sql_row.id),
    name: sql_row.name,
    email: sql_row.email,
    created_at: DATE_ISO8601(sql_row.created_at),
    
    -- Denormalisierung erlaubt in Multi-Model DB
    order_count: 0,  -- wird später aktualisiert
    total_spent: 0.0,
    
    -- Migration Metadata
    _migration: {
      source_system: "postgresql",
      source_id: sql_row.id,
      migrated_at: DATE_NOW(),
      validation_status: "pending"
    }
  }
}
```

### MongoDB → ThemisDB Mapping

```javascript
// MongoDB Document
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

// Wird zu AQL Collection Dokument:
FOR doc IN mongo_products
  INSERT {
    _key: doc._id.toString(),
    title: doc.title,
    attributes: doc.attributes,  // Geschachtelte Objekte erlaubt
    tags: doc.tags,               // Arrays
    reviews: doc.reviews,         // Array von Objekten
    
    -- Normalisierungsoptionen:
    -- Option 1: Flach (denormalisiert)
    color: doc.attributes.color,
    size: doc.attributes.size,
    
    -- Option 2: Graph-Edges für Many-To-Many
    -- reviews werden zu separater Collection + Edges
  } INTO products
```

### Neo4j → ThemisDB Graph

```aql
-- Neo4j                              → ThemisDB
-- (:User {id: 1, name: "Alice"})    → User Document
-- [:FOLLOWS]                        → Graph Edge (Relation)
-- (:Product)                        → Product Document

FUNCTION migrate_neo4j_graph(cypher_result) {
  -- Neo4j Nodes → AQL Documents
  FOR node IN cypher_result.nodes
    INSERT {
      _key: node.id,
      type: node.label,
      properties: node.properties,
      
      -- Migration Tracking
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

## 26.2 ETL Pipeline Design

### Python ETL Framework


### Python ETL Framework

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

## 26.3 Live Replikation während Migration

### CDC (Change Data Capture) Ansatz

```python
# replication/cdc_sync.py: Kontinuierliche Replikation
import psycopg2
from psycopg2.extras import LogicalReplicationConnection
import themis

class PostgreSQLCDCSync:
    def __init__(self, pg_conn_str, themis_url):
        self.pg_conn = psycopg2.connect(pg_conn_str, 
                                       connection_factory=LogicalReplicationConnection)
        self.themis = themis.Client(themis_url)
    
    def replicate_changes(self, slot_name="themis_slot"):
        """Lese Changes aus PostgreSQL WAL und appliziere sie"""
        
        cursor = self.pg_conn.cursor()
        cursor.start_replication(slot_name=slot_name)
        
        print(f"Replication started, listening for changes...")
        
        for message in cursor.consume_dstream():
            if message.payload:
                # Parse Change Event
                change = self._parse_wal_record(message.payload)
                
                # Apply in ThemisDB
                self._apply_change(change)
                
                # Acknowledge
                message.cursor.send_feedback(write_lsn=message.write_lsn)
    
    def _parse_wal_record(self, payload: bytes):
        """Parse PostgreSQL WAL Format"""
        # Vereinfacht: In Production wäre dies komplexer
        import json
        return json.loads(payload.decode('utf-8'))
    
    def _apply_change(self, change: Dict):
        """Appliziere INSERT/UPDATE/DELETE in ThemisDB"""
        
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
        
        print(f"Applied {change['action']} to {change['table']}/{change['id']}")

# Setup CDC Replikation
sync = PostgreSQLCDCSync(
    pg_conn_str="postgresql://user:password@localhost/mydb",
    themis_url="http://localhost:8529"
)

# Starte Replikation im Background
import threading
replication_thread = threading.Thread(target=sync.replicate_changes, daemon=True)
replication_thread.start()

print("CDC Replication running in background...")
```

---

## 26.4 Daten-Validierung & Reconciliation

### Reconciliation Framework


### Reconciliation Framework

Post-Migration-Validation ist kritisch um Datenverlust oder -verfälschung zu erkennen. Das Reconciliation-Framework vergleicht Source und Target systematisch: Row-Counts, Checksums, Daten-Stichproben. Diskrepanzen werden detailliert geloggt für manuelle Untersuchung.

> **📁 Vollständiger Code:** `examples/26_migration/validation/reconciliation.py` (ca. 100 Zeilen)

**Reconciliation-Klasse:**

```python
class DataReconciliation:
    """Validiert migrierte Daten gegen Quellsystem"""
    
    def __init__(self, source_db, target_db):
        self.source = source_db
        self.target = target_db
        self.report = []
    
    def validate_counts(self, source_query: str, target_collection: str) -> Dict:
        """Vergleicht Row-Counts zwischen Source und Target"""
        
        # Source Count
        source_count = self.source.execute(
            f"SELECT COUNT(*) as count FROM ({source_query}) t"
        )[0]['count']
        
        # Target Count
        target_count = self.target.query(f"""
            RETURN LENGTH(
                FOR doc IN {target_collection}
                RETURN doc
            )
        """)[0]
        
        match = source_count == target_count
        
        result = {
            'collection': target_collection,
            'source_count': source_count,
            'target_count': target_count,
            'match': match,
            'discrepancy': abs(source_count - target_count)
        }
        
        self.report.append(result)
        return result
```

**Daten-Sampling für Detail-Vergleich:**

```python
    def validate_sample(self, source_query: str, target_collection: str,
                       sample_size=100, key_field='_key') -> Dict:
        """
        Vergleicht Stichprobe von Datensätzen im Detail.
        Prüft ob alle Felder korrekt transformiert wurden.
        """
        
        # Random Sample aus Source
        sample_query = f"{source_query} ORDER BY RANDOM() LIMIT {sample_size}"
        source_rows = self.source.execute(sample_query)
        
        mismatches = []
        for source_row in source_rows:
            key = str(source_row[key_field])
            
            # Korrespondierender Target-Record
            target_row = self.target.query(f"""
                FOR doc IN {target_collection}
                    FILTER doc._key == @key
                    RETURN doc
            """, bind_vars={'key': key})
            
            if not target_row:
                mismatches.append({
                    'key': key,
                    'issue': 'missing_in_target',
                    'source_data': source_row
                })
                continue
            
            # Field-by-Field Vergleich
            differences = self._compare_records(source_row, target_row[0])
            if differences:
                mismatches.append({
                    'key': key,
                    'issue': 'data_mismatch',
                    'differences': differences
                })
        
        return {
            'collection': target_collection,
            'sample_size': sample_size,
            'mismatches': len(mismatches),
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

## 26.5 Rollback-Strategien

### Blue-Green Deployment Pattern

```yaml
# k8s/blue-green-migration.yaml
---
# Blue: Alte PostgreSQL
apiVersion: v1
kind: Service
metadata:
  name: db-service
spec:
  selector:
    version: blue  # Initial zeigt auf Blue (PostgreSQL)
  ports:
    - port: 5432

---
# Green: Neue ThemisDB
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themis-green
spec:
  replicas: 3
  template:
    metadata:
      labels:
        version: green
    spec:
      containers:
      - name: themis
        image: themisdb/server:1.3.4
        ports:
        - containerPort: 8529

---
# Rollout-Strategie
apiVersion: v1
kind: ConfigMap
metadata:
  name: migration-config
data:
  # Phase 1: 5% Traffic zu Green
  traffic_green_percent: "5"
  
  # Phase 2: 25% Traffic nach X Stunden
  phase2_hours: "4"
  phase2_traffic: "25"
  
  # Phase 3: 50% Traffic nach X Stunden
  phase3_hours: "12"
  phase3_traffic: "50"
  
  # Phase 4: 100% Traffic (oder Rollback zu Blue)
  phase4_hours: "24"
  
  # Rollback bei Critical Errors
  rollback_on_error_rate: "5"  # 5% error rate triggers rollback
```

### Schneller Rollback

```bash
#!/bin/bash
# rollback.sh: Schneller Rollback zur PostgreSQL (Blue)

echo "🔄 Initiating immediate rollback to Blue (PostgreSQL)..."

# Schritt 1: Verkehrs-Router back to Blue
kubectl patch service db-service -p '{"spec":{"selector":{"version":"blue"}}}'
echo "✓ Traffic router updated to Blue"

# Schritt 2: Stoppe Green (ThemisDB) Schreibvorgänge
kubectl set env deployment/themis-green READ_ONLY=true
echo "✓ Green set to read-only mode"

# Schritt 3: Verifiziere Blue Health
until kubectl exec $(kubectl get pods -l version=blue -o name | head -1) \
  -- pg_isready -h localhost; do
  echo "Waiting for Blue to be ready..."
  sleep 5
done
echo "✓ Blue is healthy"

# Schritt 4: Notify Teams
curl -X POST https://hooks.slack.com/services/YOUR/WEBHOOK \
  -d '{"text":"🔴 Migration rollback initiated. Reverting to PostgreSQL."}'

echo "✅ Rollback complete. System running on PostgreSQL (Blue)"
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
