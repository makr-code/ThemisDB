# Kapitel 2.5: MVCC Timeline-Funktionen & Prozessmanagement

## 2.5.1 Einführung in MVCC Timeline

> *"Time travel isn't science fiction - it's built into your database!"*

### Was sind Timeline-Funktionen?

MVCC (Multi-Version Concurrency Control) speichert nicht nur die aktuelle Version der Daten, sondern auch historische Versionen. ThemisDB ermöglicht **Time-Travel Queries** - Sie können auf beliebige Zeitpunkte in der Vergangenheit zugreifen!

**Real-World Use Cases:**
- "Zeige mir alle Bestellungen, wie sie am 31.12.2023 um 23:59 Uhr waren"
- "Was hat sich am Vertrag zwischen Version 3 und Version 5 geändert?"
- "Wer hat wann welche Änderung vorgenommen?" (Audit Trail)
- "Rollback zu einem konsistenten Snapshot von gestern"

### MVCC in ThemisDB: Die Implementierung

Basierend auf RocksDB TransactionDB implementiert ThemisDB MVCC mit:

```cpp
// Aus src/storage/rocksdb_wrapper.cpp
rocksdb::TransactionDBOptions txn_db_options;
txn_db_options.transaction_lock_timeout = 1000;  // 1 second
txn_db_options.default_lock_timeout = 1000;

// Snapshot Isolation aktivieren
rocksdb::TransactionOptions txn_options;
txn_options.set_snapshot = true;
```

**Jeder Write erzeugt eine neue Version:**
- Alte Versionen werden nicht überschrieben
- Timestamps markieren Gültigkeit
- Compaction räumt alte Versionen auf (nach Retention Period)

## 2.5.2 Time-Travel Queries

### AS OF SYSTEM TIME Syntax

```aql
-- Aktueller Stand
FOR order IN orders
  FILTER order.customer_id == 123
  RETURN order

-- Stand von gestern 18:00 Uhr (Temporal Query)
FOR order IN orders
AS OF SYSTEM TIME '2024-01-14 18:00:00'
  FILTER order.customer_id == 123
  RETURN order

-- Stand vor 7 Tagen
FOR order IN orders
AS OF SYSTEM TIME NOW() - INTERVAL '7 days'
  FILTER order.customer_id == 123
  RETURN order
```

### Version History Navigation

```aql
-- Alle Versionen eines bestimmten Datensatzes
FOR order IN orders
FOR SYSTEM_TIME ALL
  FILTER order.order_id == 'ORD-12345'
  SORT order.system_time_start
  RETURN {
    order: order,
    system_time_start: order.system_time_start,
    system_time_end: order.system_time_end
  }

-- Nur Änderungen in einem Zeitfenster
FOR order IN orders
FOR SYSTEM_TIME BETWEEN 
    '2024-01-01 00:00:00' AND '2024-01-31 23:59:59'
  FILTER order.order_id == 'ORD-12345'
  RETURN order
```

### Point-in-Time Recovery

```python
import themis_client as themis
from datetime import datetime, timedelta

def restore_to_timestamp(table_name, target_timestamp):
    """Restore table zu einem bestimmten Zeitpunkt"""
    # 1. Sichere aktuellen Stand
    themis.execute(f"""
        CREATE TABLE {table_name}_backup AS 
        FOR doc IN {table_name}
          RETURN doc
    """)
    
    # 2. Lösche aktuelle Daten
    themis.execute(f"""
        FOR doc IN {table_name}
          REMOVE doc IN {table_name}
    """)
    
    # 3. Restore von Snapshot
    themis.execute(f"""
        FOR doc IN {table_name}
        AS OF SYSTEM TIME @timestamp
          INSERT doc INTO {table_name}
    """, {"timestamp": target_timestamp})
    
    print(f"✅ Restored {table_name} to {target_timestamp}")

# Beispiel: Restore von gestern
restore_to_timestamp('orders', datetime.now() - timedelta(days=1))
```

## 2.5.3 Transaction Manager & Isolation Levels

### MVCC Transaction Lifecycle

```cpp
// Aus src/query/transaction_manager.cpp (429 Zeilen)
class TransactionManager {
    // Session-basierte Transaction-Verwaltung
    std::atomic<uint64_t> next_transaction_id_;
    std::unordered_map<std::string, Transaction*> active_transactions_;
    
    Transaction* beginTransaction(IsolationLevel level) {
        auto txn_id = next_transaction_id_.fetch_add(1);
        auto* txn = new Transaction(txn_id, level);
        
        if (level == IsolationLevel::SNAPSHOT) {
            txn->snapshot = storage_->GetSnapshot();
        }
        
        return txn;
    }
};
```

### Isolation Levels

| Level | Dirty Read | Non-Repeatable Read | Phantom Read | Implementation |
|-------|------------|---------------------|--------------|----------------|
| Read Uncommitted | ✅ Möglich | ✅ Möglich | ✅ Möglich | - |
| Read Committed | ❌ Verhindert | ✅ Möglich | ✅ Möglich | Default in RocksDB |
| Snapshot Isolation | ❌ Verhindert | ❌ Verhindert | ❌ Verhindert | `set_snapshot = true` |
| Serializable | ❌ Verhindert | ❌ Verhindert | ❌ Verhindert | + Conflict Detection |

**ThemisDB Default: Snapshot Isolation**

```aql
-- Explizite Transaction mit Isolation Level
BEGIN TRANSACTION ISOLATION LEVEL SNAPSHOT;

FOR order IN orders
  FILTER order.order_id == 123
  UPDATE order WITH {status: 'shipped'} IN orders

FOR product IN inventory
  FILTER product.product_id == 456
  UPDATE product WITH {quantity: product.quantity - 1} IN inventory

COMMIT;
```

### Write Policies

```cpp
// Aus src/storage/rocksdb_wrapper.cpp
enum class WritePolicy {
    WRITE_COMMITTED,      // Default: Writes erst bei Commit sichtbar
    WRITE_PREPARED,       // 2PC für distributed transactions
    WRITE_UNPREPARED      // Optimistic: weniger Overhead
};
```

## 2.5.4 Prozessmodellierung mit Multi-Model Architecture

### Das Problem: Administrative Prozesse abbilden

Typische Verwaltungsprozesse (z.B. Urlaubsantrag, Genehmigungsworkflow, Vertragsabschluss) haben:

1. **Zustandsübergänge** (Graph-Modell)
2. **Ähnliche Prozessverläufe** (Vector-Modell)
3. **Strukturierte Daten** (Relational-Modell)

ThemisDB erlaubt alle drei Modelle in **einer** Datenbank!

### Graph-basierte Workflow-States

```aql
-- Workflow-Zustände als Knoten
CREATE TABLE workflow_states (
    state_id UUID PRIMARY KEY,
    workflow_type VARCHAR(100),  -- 'leave_request', 'invoice_approval', ...
    state_name VARCHAR(100),     -- 'submitted', 'approved', 'rejected'
    description TEXT,
    is_terminal BOOLEAN DEFAULT FALSE
);

-- Transitionen als Kanten
CREATE TABLE workflow_transitions (
    transition_id UUID PRIMARY KEY,
    from_state_id UUID REFERENCES workflow_states(state_id),
    to_state_id UUID REFERENCES workflow_states(state_id),
    transition_name VARCHAR(100),  -- 'approve', 'reject', 'request_changes'
    condition TEXT,                -- Optional: AQL-Expression
    required_role VARCHAR(100),    -- Wer darf diese Transition ausführen?
    UNIQUE(from_state_id, to_state_id, transition_name)
);

-- Graph-Index für schnelle Traversierung
CREATE INDEX idx_transitions_from ON workflow_transitions (from_state_id);
CREATE INDEX idx_transitions_to ON workflow_transitions (to_state_id);
```

**Beispiel: Urlaubsantrag-Workflow**

```aql
-- States definieren
INSERT [
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'draft', is_terminal: false},
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'submitted', is_terminal: false},
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'manager_review', is_terminal: false},
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'hr_review', is_terminal: false},
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'approved', is_terminal: true},
  {state_id: gen_random_uuid(), workflow_type: 'leave_request', state_name: 'rejected', is_terminal: true}
] INTO workflow_states

-- Transitionen definieren
LET draft_state = (
  FOR state IN workflow_states
    FILTER state.state_name == 'draft'
    RETURN state.state_id
)[0]

LET submitted_state = (
  FOR state IN workflow_states
    FILTER state.state_name == 'submitted'
    RETURN state.state_id
)[0]

LET manager_review_state = (
  FOR state IN workflow_states
    FILTER state.state_name == 'manager_review'
    RETURN state.state_id
)[0]

LET approved_state = (
  FOR state IN workflow_states
    FILTER state.state_name == 'approved'
    RETURN state.state_id
)[0]

INSERT [
  {from_state_id: draft_state, to_state_id: submitted_state, transition_name: 'submit', required_role: 'employee'},
  {from_state_id: submitted_state, to_state_id: manager_review_state, transition_name: 'forward_to_manager', required_role: 'system'},
  {from_state_id: manager_review_state, to_state_id: approved_state, transition_name: 'approve', required_role: 'manager'}
] INTO workflow_transitions
```
    'approve',
    'manager';
```

### Prozess-Instanzen verwalten

```aql
-- Konkrete Prozess-Instanzen
CREATE TABLE process_instances (
    instance_id UUID PRIMARY KEY,
    workflow_type VARCHAR(100),
    current_state_id UUID REFERENCES workflow_states(state_id),
    created_by VARCHAR(100),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    data JSONB  -- Prozess-spezifische Daten
);

-- Prozess-Historie (Audit Trail)
CREATE TABLE process_history (
    history_id UUID PRIMARY KEY,
    instance_id UUID REFERENCES process_instances(instance_id),
    from_state_id UUID REFERENCES workflow_states(state_id),
    to_state_id UUID REFERENCES workflow_states(state_id),
    transition_name VARCHAR(100),
    performed_by VARCHAR(100),
    performed_at TIMESTAMP DEFAULT NOW(),
    comment TEXT,
    data_snapshot JSONB  -- Snapshot der Daten zu diesem Zeitpunkt
);
```

### Workflow-Engine Implementation

```python
class WorkflowEngine:
    def __init__(self, themis_client):
        self.themis = themis_client
    
    def start_workflow(self, workflow_type, created_by, initial_data):
        """Starte einen neuen Workflow-Prozess"""
        # Hole Initial-State und erstelle Instanz in einer Transaktion
        result = self.themis.query("""
            FOR ws IN workflow_states
              FILTER ws.workflow_type == @workflow_type && ws.state_name == 'draft'
              INSERT {instance_id: GENERATE_ID(), workflow_type: @workflow_type, 
                      current_state_id: ws.state_id, created_by: @created_by, data: @data}
              INTO process_instances
              RETURN {instance_id: NEW.instance_id}
        """, {
            'workflow_type': workflow_type,
            'created_by': created_by,
            'data': initial_data
        })
        return result[0]['instance_id']
    
    def transition(self, instance_id, transition_name, performed_by, comment=None):
        """Führe Zustandsübergang durch"""
        # Hole aktuelle Instance
        instance = self.themis.query_one("""
            SELECT current_state_id, data 
            FROM process_instances 
            WHERE instance_id = ?
        """, (instance_id,))
        
        # Finde erlaubte Transition
        transition = self.themis.query_one("""
            SELECT t.*, s_to.state_name as to_state_name
            FROM workflow_transitions t
            JOIN workflow_states s_to ON t.to_state_id = s_to.state_id
            WHERE t.from_state_id = ?
              AND t.transition_name = ?
              AND (t.required_role = ? OR t.required_role IS NULL)
        """, (instance['current_state_id'], transition_name, self.get_user_role(performed_by)))
        
        if not transition:
            raise ValueError(f"Transition '{transition_name}' not allowed")
        
        # Führe Transition durch (in Transaction!)
        with self.themis.transaction():
            # Update Instance
            self.themis.execute("""
                UPDATE process_instances
                SET current_state_id = ?,
                    updated_at = NOW()
                WHERE instance_id = ?
            """, (transition['to_state_id'], instance_id))
            
            # Log History
            self.themis.execute("""
                INSERT INTO process_history
                (history_id, instance_id, from_state_id, to_state_id,
                 transition_name, performed_by, comment, data_snapshot)
                VALUES (gen_random_uuid(), ?, ?, ?, ?, ?, ?, ?)
            """, (instance_id, instance['current_state_id'], 
                  transition['to_state_id'], transition_name,
                  performed_by, comment, instance['data']))
        
        print(f"✅ Transitioned to: {transition['to_state_name']}")
        return transition['to_state_name']

# Verwendung
engine = WorkflowEngine(themis)

# 1. Start Leave Request
instance_id = engine.start_workflow('leave_request', 'john.doe', {
    'start_date': '2024-07-01',
    'end_date': '2024-07-14',
    'reason': 'Family vacation'
})

# 2. Submit
engine.transition(instance_id, 'submit', 'john.doe')

# 3. Manager Approval
engine.transition(instance_id, 'forward_to_manager', 'system')
engine.transition(instance_id, 'approve', 'jane.manager', 'Approved')
```

### Graph-Queries für Workflow-Analyse

```aql
-- Alle möglichen Pfade von State A zu State B (AQL Rekursion)
LET paths = (
  FOR t IN workflow_transitions
    FILTER t.from_state_id == @start_state_id
    LET path = [t.from_state_id]
    
    -- Erweitere Pfade rekursiv
    FOR i IN 1..10
      LET extensions = (
        FOR next_t IN workflow_transitions
          FILTER next_t.from_state_id IN LAST(path)
               && POSITION(next_t.from_state_id, path) == -1
          RETURN next_t
      )
      FILTER LENGTH(extensions) > 0
      LET path = FLATTEN(path, APPEND(path, extensions[*].from_state_id))
    
    FILTER LAST(path) == @target_state_id
    RETURN path
)
FOR p IN paths RETURN {path: p, length: LENGTH(p)}
```

## 2.5.5 Vector-basierte Prozess-Optimierung

### Ähnliche Prozessverläufe finden

Nutze Vector-Embeddings um ähnliche Prozesse zu finden:

```aql
-- Speichere Prozess-"Fingerprint" als Vector
CREATE TABLE process_embeddings (
    instance_id UUID PRIMARY KEY REFERENCES process_instances(instance_id),
    embedding VECTOR(128),  -- Repräsentation des Prozessverlaufs
    created_at TIMESTAMP DEFAULT NOW()
);

-- HNSW Index für Similarity Search
CREATE INDEX idx_process_embedding ON process_embeddings 
USING hnsw (embedding vector_cosine_ops);
```

**Embedding-Generierung aus Prozess-Historie:**

```python
import numpy as np
from sklearn.preprocessing import StandardScaler

def generate_process_embedding(instance_id):
    """Generiere Embedding aus Prozess-Verlauf"""
    # Hole Historie
    history = themis.query("""
        SELECT 
            from_state_id,
            to_state_id,
            transition_name,
            EXTRACT(EPOCH FROM (performed_at - LAG(performed_at) 
                               OVER (ORDER BY performed_at))) as duration_seconds
        FROM process_history
        WHERE instance_id = ?
        ORDER BY performed_at
    """, (instance_id,))
    
    # Feature-Extraktion
    features = []
    
    # 1. Anzahl Transitionen
    features.append(len(history))
    
    # 2. Durchschnittliche Duration
    durations = [h['duration_seconds'] or 0 for h in history]
    features.append(np.mean(durations) if durations else 0)
    
    # 3. State-Sequence Encoding (One-Hot oder Hash)
    state_sequence = [h['transition_name'] for h in history]
    # Simplified: Hash transitions to fixed-size vector
    for i in range(10):  # Max 10 transitions
        if i < len(state_sequence):
            features.append(hash(state_sequence[i]) % 100)
        else:
            features.append(0)
    
    # ... mehr Features (insgesamt 128)
    
    # Normalisierung
    embedding = StandardScaler().fit_transform([features])[0]
    
    # Speichere Embedding
    themis.execute("""
        INSERT INTO process_embeddings (instance_id, embedding)
        VALUES (?, ?)
        ON CONFLICT (instance_id) DO UPDATE
        SET embedding = EXCLUDED.embedding
    """, (instance_id, embedding.tolist()))
    
    return embedding

# Generiere Embeddings für alle Prozesse
for instance in themis.query("SELECT instance_id FROM process_instances"):
    generate_process_embedding(instance['instance_id'])
```

### Pattern Discovery

```aql
-- Finde ähnliche Prozessverläufe
SELECT 
    pi.instance_id,
    pi.workflow_type,
    pi.created_by,
    pi.data,
    1 - (pe.embedding <=> :query_embedding) AS similarity
FROM process_instances pi
JOIN process_embeddings pe ON pi.instance_id = pe.instance_id
WHERE pi.workflow_type = 'leave_request'
ORDER BY pe.embedding <=> :query_embedding
LIMIT 10;
```

**Use Case: Anomalie-Erkennung**

```python
def detect_anomalous_processes(workflow_type, threshold=0.7):
    """Finde ungewöhnliche Prozessverläufe"""
    # Hole alle Prozesse des Typs
    processes = themis.query("""
        SELECT instance_id, embedding
        FROM process_instances pi
        JOIN process_embeddings pe ON pi.instance_id = pe.instance_id
        WHERE workflow_type = ?
    """, (workflow_type,))
    
    # In AQL: Anomalie-Detection als einzelne Query
    anomalies = themis.query("""
        FOR proc IN process_instances
          FILTER proc.workflow_type == @workflow_type
          LET similar = (
            FOR pe IN process_embeddings
              FILTER pe.instance_id != proc.instance_id
              LET sim = 1 - (COSINE(proc.embedding, pe.embedding) || 0)
              SORT sim DESC LIMIT 5
              RETURN sim
          )
          LET max_sim = MAX(similar)
          FILTER max_sim < @threshold
          RETURN {instance_id: proc.instance_id, max_similarity: max_sim}
    """, {'workflow_type': workflow_type, 'threshold': threshold})
    
    return anomalies

# Finde Anomalien
anomalies = detect_anomalous_processes('leave_request', threshold=0.75)
print(f"Found {len(anomalies)} anomalous processes")
```

## 2.5.6 Relational: Audit Trails & Compliance

### Vollständiger Audit Trail

```aql
-- Compliance-View: Alle Änderungen mit Timestamps
CREATE VIEW audit_trail AS
SELECT 
    ph.instance_id,
    pi.workflow_type,
    ws_from.state_name as from_state,
    ws_to.state_name as to_state,
    ph.transition_name,
    ph.performed_by,
    ph.performed_at,
    ph.comment,
    ph.data_snapshot
FROM process_history ph
JOIN process_instances pi ON ph.instance_id = pi.instance_id
JOIN workflow_states ws_from ON ph.from_state_id = ws_from.state_id
JOIN workflow_states ws_to ON ph.to_state_id = ws_to.state_id
ORDER BY ph.performed_at DESC;

-- Compliance-Report
SELECT 
    workflow_type,
    DATE_TRUNC('month', performed_at) as month,
    COUNT(*) as transition_count,
    COUNT(DISTINCT instance_id) as process_count,
    AVG(EXTRACT(EPOCH FROM (performed_at - created_at))) / 3600 as avg_duration_hours
FROM audit_trail at
JOIN process_instances pi ON at.instance_id = pi.instance_id
WHERE performed_at >= NOW() - INTERVAL '12 months'
GROUP BY workflow_type, month
ORDER BY month DESC, workflow_type;
```

### Immutable Audit Log mit Blockchain-Konzept

```aql
-- Erweitere History mit Hash-Chain
ALTER TABLE process_history 
ADD COLUMN previous_hash VARCHAR(64),
ADD COLUMN current_hash VARCHAR(64);

-- Compute Hash inkl. previous_hash (Blockchain-Style)
CREATE OR REPLACE FUNCTION compute_history_hash()
RETURNS TRIGGER AS $$
BEGIN
    NEW.current_hash := encode(sha256(
        (COALESCE(NEW.previous_hash, '') || 
         NEW.instance_id::text ||
         NEW.transition_name ||
         NEW.performed_by ||
         NEW.performed_at::text ||
         NEW.data_snapshot::text)::bytea
    ), 'hex');
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER history_hash_trigger
BEFORE INSERT ON process_history
FOR EACH ROW EXECUTE FUNCTION compute_history_hash();
```

## 2.5.7 Best Practices für Prozess-Management

### 1. Workflow-Design

✅ **DO:**
- Definiere klare States & Transitions
- Nutze Terminal-States (is_terminal=TRUE)
- Implementiere RBAC (required_role)

❌ **DON'T:**
- Keine zyklischen Workflows ohne Exit-Condition
- Vermeide zu granulare States (zu viele Übergänge)
- Keine Business-Logic in Transitions (nur in Application Layer)

### 2. Performance

✅ **DO:**
- Index auf current_state_id für schnelle State-Lookups
- Materialized Views für Reporting
- Partitioniere process_history nach Zeit

❌ **DON'T:**
- Keine Rekursive Queries ohne Depth-Limit
- Vermeide JOINs über alle Historien-Einträge
- Keine Embeddings ohne HNSW-Index

### 3. Compliance

✅ **DO:**
- Logge ALLE Zustandsänderungen
- Speichere data_snapshot für Audit
- Implementiere Hash-Chain für Tamper-Detection

❌ **DON'T:**
- Lösche nie History-Einträge
- Keine nachträgliche Änderung von Timestamps
- Vermeide PII in Audit-Logs ohne Verschlüsselung

## 2.5.8 Zusammenfassung

ThemisDB's MVCC Timeline & Multi-Model Architecture ermöglichen:

✅ **Time-Travel:**
- AS OF SYSTEM TIME für historische Queries
- Point-in-Time Recovery
- Vollständige Version History

✅ **Prozess-Management:**
- **Graph:** Workflow-States & Transitionen
- **Vector:** Prozess-Ähnlichkeit & Pattern Discovery
- **Relational:** Audit Trails & Compliance

✅ **Production-Ready:**
- Transaction Isolation (Snapshot)
- Immutable Audit Logs
- Performance durch richtige Indexes

**Key Takeaways:**
1. MVCC ist mehr als nur Concurrency Control - es's eine Time Machine
2. Multi-Model = Ein Prozess, drei Perspektiven (Graph/Vector/Relational)
3. Audit Trails sind Pflicht, Hash-Chains sind Best-Practice
4. Vector-Embeddings für Prozess-Optimierung nutzen
5. Graph-Traversierung für komplexe Workflows
