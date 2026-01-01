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

```python
# etl/pipeline.py: Generischer ETL Runner
import logging
from typing import List, Dict, Any
import themis

class ETLPipeline:
    def __init__(self, source_db, target_db, batch_size=1000):
        self.source = source_db
        self.target = target_db
        self.batch_size = batch_size
        self.logger = logging.getLogger(__name__)
    
    def extract(self, source_query: str) -> List[Dict]:
        """Extract: Daten aus Legacy-System lesen"""
        self.logger.info(f"Extracting from source: {source_query[:50]}...")
        return list(self.source.execute(source_query))
    
    def transform(self, rows: List[Dict], transformer_func) -> List[Dict]:
        """Transform: Schema-Mapping & Validierung"""
        self.logger.info(f"Transforming {len(rows)} rows...")
        transformed = []
        errors = []
        
        for i, row in enumerate(rows):
            try:
                transformed.append(transformer_func(row))
            except Exception as e:
                errors.append({'row_index': i, 'error': str(e), 'data': row})
        
        if errors:
            self.logger.warning(f"Transformation errors: {len(errors)}")
        
        return transformed, errors
    
    def load(self, docs: List[Dict], collection: str) -> Dict:
        """Load: In ThemisDB einfügen"""
        self.logger.info(f"Loading {len(docs)} documents into {collection}...")
        
        result = {
            'inserted': 0,
            'errors': 0,
            'error_details': []
        }
        
        # Batch-wise einfügen
        for i in range(0, len(docs), self.batch_size):
            batch = docs[i:i+self.batch_size]
            
            try:
                aql = f"""
                  FOR doc IN @docs
                    INSERT doc INTO {collection}
                    RETURN NEW._id
                """
                ids = self.target.query(aql, {'docs': batch})
                result['inserted'] += len(ids)
            except Exception as e:
                self.logger.error(f"Batch insert error: {e}")
                result['errors'] += len(batch)
                result['error_details'].append({
                    'batch_start': i,
                    'batch_size': len(batch),
                    'error': str(e)
                })
        
        return result
    
    def run(self, source_query: str, transformer_func, target_collection: str) -> Dict:
        """Orchestriere E-T-L Pipeline"""
        try:
            rows = self.extract(source_query)
            transformed, errors = self.transform(rows, transformer_func)
            result = self.load(transformed, target_collection)
            
            result['transformation_errors'] = len(errors)
            result['total_processed'] = len(rows)
            result['success_rate'] = result['inserted'] / len(rows) * 100
            
            return result
        except Exception as e:
            self.logger.error(f"Pipeline failed: {e}")
            raise

# Nutzungsbeispiel
from adapters.postgresql import PostgreSQLAdapter
from adapters.themis import ThemisAdapter

source_db = PostgreSQLAdapter(connection_string="postgresql://...")
target_db = ThemisAdapter(url="http://localhost:8529")

etl = ETLPipeline(source_db, target_db, batch_size=5000)

# Transform-Funktion
def transform_customer(row):
    return {
        '_key': str(row['id']),
        'name': row['name'],
        'email': row['email'],
        'created_at': row['created_at'].isoformat(),
        'migrated_at': datetime.now().isoformat()
    }

result = etl.run(
    source_query="SELECT * FROM customers WHERE status = 'active'",
    transformer_func=transform_customer,
    target_collection="customers"
)

print(f"Migration: {result['inserted']} inserted, {result['errors']} errors, Success: {result['success_rate']:.1f}%")
```

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

```python
# validation/reconciliation.py: Post-Migration Validation
class DataReconciliation:
    def __init__(self, source_db, target_db):
        self.source = source_db
        self.target = target_db
    
    def validate_counts(self, source_query: str, target_collection: str) -> Dict:
        """Vergleiche Zeilen-Anzahl Source vs Target"""
        
        source_count = self.source.execute(f"SELECT COUNT(*) FROM ({source_query})")[0]['count']
        
        target_count = self.target.query(f"""
          RETURN LENGTH(
            FOR doc IN {target_collection}
            RETURN doc
          )
        """)[0]
        
        match = source_count == target_count
        return {
            'collection': target_collection,
            'source_count': source_count,
            'target_count': target_count,
            'match': match,
            'difference': abs(source_count - target_count)
        }
    
    def validate_checksums(self, table: str, key_column: str) -> Dict:
        """Vergleiche Row-Level Checksums"""
        
        # Source Checksums
        source_checksums = self.source.execute(f"""
          SELECT {key_column}, MD5(ROW(*)) as checksum
          FROM {table}
          ORDER BY {key_column}
        """)
        
        # Target Checksums (via AQL)
        target_checksums = self.target.query(f"""
          FOR doc IN {table.lower()}
            SORT doc._key
            RETURN {{
              key: doc._key,
              checksum: MD5(JSON_STRINGIFY(doc))
            }}
        """)
        
        # Reconcile
        mismatches = []
        for src in source_checksums:
            tgt = next((t for t in target_checksums 
                       if t['key'] == src[key_column]), None)
            
            if not tgt or tgt['checksum'] != src['checksum']:
                mismatches.append({
                    'id': src[key_column],
                    'source_checksum': src['checksum'],
                    'target_checksum': tgt['checksum'] if tgt else None
                })
        
        return {
            'table': table,
            'total_rows': len(source_checksums),
            'mismatches': len(mismatches),
            'match_rate': (len(source_checksums) - len(mismatches)) / len(source_checksums) * 100,
            'mismatched_ids': mismatches[:10]  # First 10
        }
    
    def run_full_validation(self) -> Dict:
        """Komplette Post-Migration Validation"""
        
        validations = [
            self.validate_counts("SELECT * FROM customers", "customers"),
            self.validate_counts("SELECT * FROM orders", "orders"),
            self.validate_checksums("customers", "id"),
            self.validate_checksums("orders", "id")
        ]
        
        all_pass = all(v.get('match', v.get('match_rate', 0) >= 99.9) for v in validations)
        
        return {
            'all_pass': all_pass,
            'validations': validations,
            'timestamp': datetime.now().isoformat()
        }

# Führe Validation durch
recon = DataReconciliation(source_db, target_db)
result = recon.run_full_validation()

if result['all_pass']:
    print("✅ Alle Validierungen erfolgreich!")
else:
    print("❌ Validierungsfehler gefunden:")
    for v in result['validations']:
        if not v.get('match'):
            print(f"  - {v}")
```

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
