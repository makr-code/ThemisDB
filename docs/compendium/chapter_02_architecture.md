# Kapitel 2: Architektur-Überblick

> *"Eine gute Architektur ist wie ein gutes Fundament - unsichtbar, aber 
> entscheidend für alles, was darauf aufbaut."*

---

## Überblick

In diesem Kapitel tauchen wir tief in die Architektur von ThemisDB ein. Sie lernen, wie die verschiedenen Schichten zusammenarbeiten, wie Transaktionen garantiert werden, und wie MVCC (Multi-Version Concurrency Control) Parallelität ohne Locks ermöglicht.

**Was Sie in diesem Kapitel lernen werden:**
- Die Schichten-Architektur von ThemisDB
- Wie MVCC funktioniert und warum es wichtig ist
- Transaction Management und ACID-Garantien
- Storage Layer mit RocksDB
- Indexstrukturen für Performance
- Praktische Demonstration mit einer Todo-App

**Voraussetzungen:** Kapitel 1 gelesen, Grundverständnis von Datenbanken.

---

## 2.1 Die Schichten-Architektur

ThemisDB folgt einer klassischen Schichten-Architektur, wobei jede Schicht klare Verantwortlichkeiten hat.

### Die fünf Schichten

```
┌──────────────────────────────────────────────┐
│         1. Query Layer (AQL)                 │
│  • Query Parsing & Optimization              │
│  • Execution Planning                        │
│  • Result Formatting                         │
└──────────────┬───────────────────────────────┘
               │
┌──────────────▼───────────────────────────────┐
│     2. Transaction Manager (MVCC)            │
│  • Snapshot Isolation                        │
│  • Conflict Detection                        │
│  • Commit/Rollback Coordination              │
└──────────────┬───────────────────────────────┘
               │
┌──────────────▼───────────────────────────────┐
│     3. Model Engines                         │
│  ┌──────────┬──────────┬──────────────────┐  │
│  │ Graph    │ Document │ Vector           │  │
│  │ Engine   │ Engine   │ Engine           │  │
│  └──────────┴──────────┴──────────────────┘  │
│  • Model-spezifische Logik                   │
│  • Secondary Indexes                         │
│  • Constraints                               │
└──────────────┬───────────────────────────────┘
               │
┌──────────────▼───────────────────────────────┐
│     4. Index Manager                         │
│  • B-Tree Indexes                            │
│  • Hash Indexes                              │
│  • Geo Indexes (Hilbert Curves)              │
│  • Vector Indexes (HNSW)                     │
│  • Fulltext Indexes                          │
└──────────────┬───────────────────────────────┘
               │
┌──────────────▼───────────────────────────────┐
│     5. Storage Layer (RocksDB)               │
│  • LSM-Trees für SSD-Optimierung             │
│  • Write-Ahead Log (WAL)                     │
│  • Compression (LZ4, Zstandard)              │
│  • Snapshots & Backups                       │
└──────────────────────────────────────────────┘
```

### Warum diese Aufteilung?

**Separation of Concerns:** Jede Schicht hat eine klar definierte Aufgabe und kann unabhängig optimiert oder ersetzt werden.

**Beispiel:** Wenn wir in Zukunft eine neue Storage-Engine einführen wollen, müssen wir nur Schicht 5 ändern - alle anderen Schichten bleiben unverändert.

---

## 2.2 MVCC: Parallelität ohne Locks

### Das Problem mit Locks

Traditionelle Datenbanken verwenden Locks:

```python
# Traditionell: Pessimistic Locking
transaction1.begin()
transaction1.lock(row_id)  # Row wird gesperrt
transaction1.update(row_id, new_value)
transaction1.unlock(row_id)
transaction1.commit()

# Problem: Transaction 2 muss warten
transaction2.begin()
transaction2.lock(row_id)  # BLOCKIERT bis transaction1 fertig ist!
```

**Problem:** Bei hoher Parallelität führt dies zu:
- Warteschlangen und Bottlenecks
- Deadlocks zwischen Transaktionen
- Timeouts und Failed Transactions

### Die MVCC-Lösung

MVCC (Multi-Version Concurrency Control) erstellt stattdessen Versionen:

```python
# MVCC: Optimistic Concurrency
transaction1.begin(snapshot_id=100)
# Liest Version 100 der Daten
transaction1.read(row_id)  # Version 100

# Gleichzeitig kann transaction2 lesen!
transaction2.begin(snapshot_id=100)
transaction2.read(row_id)  # Auch Version 100, keine Wartezeit!

# Transaction 1 schreibt
transaction1.update(row_id, value_a)  # Erstellt Version 101
transaction1.commit()  # Version 101 wird permanent

# Transaction 2 schreibt auch
transaction2.update(row_id, value_b)  # Will auch Version 101 erstellen
transaction2.commit()  # FEHLER: Conflict Detection!
# "Row was modified by another transaction"
```

**Vorteil:** Reads blockieren nie Writes, Writes blockieren nie Reads.

### Wie funktioniert MVCC intern?

Jede Datenzeile hat nicht nur einen Wert, sondern eine Historie:

```
Row ID: users/alice

Version 100 (committed):
  name: "Alice Smith"
  age: 28

Version 101 (committed):
  name: "Alice Smith"
  age: 29

Version 102 (in_progress, transaction_id=555):
  name: "Alice Johnson"
  age: 29
```

**Snapshot-Reads:** Eine Transaktion mit `snapshot_id=100` sieht nur Versionen ≤ 100, die committed sind.

**Write-Write Conflicts:** Wenn zwei Transaktionen die gleiche Row ändern wollen, gewinnt die erste. Die zweite bekommt einen Conflict Error beim Commit.

---

## 2.3 Transaction Management

### ACID-Garantien

ThemisDB garantiert volle ACID-Properties:

**A - Atomicity (Atomarität):**  
Alle Operationen in einer Transaktion passieren komplett oder gar nicht.

```python
# Beispiel: Geldtransfer
transaction.begin()
transaction.update("accounts/alice", {"balance": 900})  # -100
transaction.update("accounts/bob", {"balance": 1100})   # +100
transaction.commit()  # Beide Updates oder keines!
```

Wenn der Server zwischen den beiden `update()` Calls abstürzt:
- **Ohne Atomarität:** Alice verliert 100€, Bob bekommt nichts (Katastrophe!)
- **Mit Atomarität:** Beide Updates werden zurückgerollt (Konsistent!)

**C - Consistency (Konsistenz):**  
Constraints werden immer eingehalten.

```python
# Foreign Key Constraint
transaction.insert("orders", {
    "order_id": "o123",
    "user_id": "alice",  # Muss in users existieren
    "amount": 50.0
})
# Wenn user "alice" nicht existiert -> FEHLER!
```

**I - Isolation (Isolierung):**  
Transaktionen sehen sich nicht gegenseitig.

```python
# Transaction 1 liest
t1.read("products/laptop")  # price: 999

# Transaction 2 ändert (aber committed noch nicht)
t2.update("products/laptop", {"price": 899})

# Transaction 1 liest nochmal
t1.read("products/laptop")  # Immer noch 999!
# Sieht die Änderung von T2 NICHT, bis T2 committet
```

**D - Durability (Dauerhaftigkeit):**  
Einmal committed, sind Daten permanent - auch bei Serverausfall.

```python
transaction.commit()
# Ab hier garantiert: Daten sind auf Disk!
# Selbst wenn Server JETZT abstürzt, sind Daten da.
```

### Isolation Levels

ThemisDB implementiert **Snapshot Isolation**, ein Mittelweg zwischen Serializable und Read Committed:

| Isolation Level | Dirty Reads | Non-Repeatable Reads | Phantom Reads |
|----------------|-------------|----------------------|---------------|
| Read Uncommitted | ✗ Möglich | ✗ Möglich | ✗ Möglich |
| Read Committed | ✓ Verhindert | ✗ Möglich | ✗ Möglich |
| **Snapshot Isolation** | **✓ Verhindert** | **✓ Verhindert** | **✓ Verhindert** |
| Serializable | ✓ Verhindert | ✓ Verhindert | ✓ Verhindert |

**Snapshot Isolation** ist performanter als Serializable, verhindert aber trotzdem alle Anomalien, die in der Praxis relevant sind.

---

## 2.4 Storage Layer: RocksDB

### Warum RocksDB?

ThemisDB verwendet RocksDB als Storage-Engine. Diese Entscheidung basiert auf mehreren Faktoren:

**1. LSM-Trees für SSDs optimiert**

Traditionelle B-Trees schreiben oft:
```
Write 1 byte → Read 4KB page → Modify 1 byte → Write 4KB page
```

LSM-Trees (Log-Structured Merge Trees) schreiben sequentiell:
```
Write 1 byte → Append to log → Done
Later: Merge logs in background
```

**Resultat:** 10x schneller auf SSDs!

**2. Battle-Tested bei Facebook**

RocksDB verarbeitet bei Facebook:
- Billions of operations per day
- Petabytes of data
- 10+ years Production Experience

**3. Flexible Key-Value API**

RocksDB ist ein Key-Value Store - perfekt als Foundation für höhere Modelle:

```cpp
// Relational: key = "users/alice"
db->Put("users/alice", json_data);

// Graph: key = "edges/from_alice_to_bob"
db->Put("edges/from_alice_to_bob", edge_data);

// Vector: key = "vectors/product_123"
db->Put("vectors/product_123", embedding_data);
```

### Column Families

RocksDB unterstützt "Column Families" - separate Namespaces innerhalb einer DB:

```cpp
// Trennung nach Datenmodell
db->Put(cf_relational, "users/alice", data);
db->Put(cf_graph_edges, "alice->bob", edge);
db->Put(cf_vectors, "prod_123", embedding);
```

**Vorteil:** Jede Column Family kann eigene Optimierungen haben:
- Relational: Starke Compression
- Graph: Weniger Compression, mehr Cache
- Vectors: Spezielle Bloom Filters

### Write-Ahead Log (WAL)

Jede Write-Operation wird erst in ein Log geschrieben:

```
1. Client sendet: UPDATE users/alice SET age=29
2. WAL Write: "UPDATE users/alice age=29" → Disk (sync!)
3. MemTable Update: In-Memory Update
4. Return OK to Client
...später...
5. Background Compaction: MemTable → SSTable auf Disk
```

**Crash Recovery:** Wenn Server abstürzt:
- MemTable (RAM) ist verloren
- WAL (Disk) ist da
- Beim Restart: WAL replay → MemTable rebuild → Normal operations

---

## 2.5 Praxis: Todo-App

Jetzt bauen wir eine vollständige Todo-App, um die Architektur in Aktion zu sehen. Basis ist `examples/02_todo_app`.

### Architektur der Todo-App

Die Todo-App folgt dem MVC-Pattern und demonstriert alle ACID-Eigenschaften:

```
┌─────────────────────────────────────────┐
│     UI (Tkinter)                        │
│  • Task List (Treeview)                 │
│  • Detail Panel (Form)                  │
│  • Toolbar (Buttons)                    │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│     Controller (main.py)                │
│  • Event Handlers                       │
│  • Business Logic                       │
│  • Validation                           │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│     TodoClient (themis_client.py)       │
│  • CRUD Operations                      │
│  • Transaction Management               │
│  • Error Handling                       │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│     ThemisDB Server                     │
│  • MVCC Transactions                    │
│  • Secondary Indexes                    │
│  • ACID Guarantees                      │
└─────────────────────────────────────────┘
```

### Datenmodell

```python
# examples/02_todo_app/models.py

from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import Optional, List

class TaskStatus(Enum):
    OPEN = "open"
    IN_PROGRESS = "in_progress"
    DONE = "done"

class TaskPriority(Enum):
    LOW = "low"
    NORMAL = "normal"
    HIGH = "high"

@dataclass
class Task:
    id: str
    title: str
    description: str
    status: TaskStatus
    priority: TaskPriority
    created_at: datetime
    updated_at: datetime
    due_date: Optional[datetime] = None
    tags: List[str] = None
    
    def to_dict(self):
        """Serialisierung für ThemisDB"""
        return {
            "_key": self.id,
            "title": self.title,
            "description": self.description,
            "status": self.status.value,
            "priority": self.priority.value,
            "created_at": self.created_at.isoformat(),
            "updated_at": self.updated_at.isoformat(),
            "due_date": self.due_date.isoformat() if self.due_date else None,
            "tags": self.tags or []
        }
    
    @classmethod
    def from_dict(cls, data: dict):
        """Deserialisierung von ThemisDB"""
        return cls(
            id=data["_key"],
            title=data["title"],
            description=data["description"],
            status=TaskStatus(data["status"]),
            priority=TaskPriority(data["priority"]),
            created_at=datetime.fromisoformat(data["created_at"]),
            updated_at=datetime.fromisoformat(data["updated_at"]),
            due_date=datetime.fromisoformat(data["due_date"]) if data.get("due_date") else None,
            tags=data.get("tags", [])
        )
```

**Design-Entscheidungen:**

1. **Enums für Status/Priority:** Type-Safety, keine Tippfehler möglich
2. **Dataclass:** Automatische `__init__`, `__repr__`, `__eq__`
3. **to_dict/from_dict:** Klare Serialisierungslogik
4. **Type Hints:** IDEs können helfen, Fehler früh erkennen

### Setup und Installation

```bash
# ThemisDB starten (Docker)
docker run -d -p 8765:8765 themisdb/themisdb:1.3.5

# Todo-App installieren
cd examples/02_todo_app
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt
```

### Client-Implementierung

```python
# examples/02_todo_app/themis_client.py

from themisdb import Client
from models import Task, TaskStatus, TaskPriority
from typing import List, Optional, Dict
import uuid

class TodoClient:
    def __init__(self, host="localhost", port=8765):
        self.client = Client(host, port)
        self._setup_database()
    
    def _setup_database(self):
        """Erstellt Collection und Indizes"""
        # Collection erstellen
        try:
            self.client.create_collection("tasks", type="document")
        except Exception as e:
            # Collection existiert bereits
            pass
        
        # Indizes erstellen für Performance
        self.client.create_index("tasks", "status_idx", ["status"])
        self.client.create_index("tasks", "priority_idx", ["priority"])
        self.client.create_index("tasks", "due_date_idx", ["due_date"])
    
    def create_task(self, task: Task) -> bool:
        """Erstellt einen neuen Task"""
        try:
            self.client.insert("tasks", task.to_dict())
            return True
        except Exception as e:
            print(f"Error creating task: {e}")
            return False
    
    def get_task(self, task_id: str) -> Optional[Task]:
        """Lädt einen Task nach ID"""
        try:
            data = self.client.get("tasks", task_id)
            return Task.from_dict(data) if data else None
        except Exception as e:
            print(f"Error getting task: {e}")
            return None
    
    def update_task(self, task: Task) -> bool:
        """Aktualisiert einen existierenden Task"""
        try:
            # MVCC in Action: Conflict Detection!
            self.client.update("tasks", task.id, task.to_dict())
            return True
        except ConflictError:
            print("Task was modified by another user!")
            return False
        except Exception as e:
            print(f"Error updating task: {e}")
            return False
    
    def delete_task(self, task_id: str) -> bool:
        """Löscht einen Task"""
        try:
            self.client.delete("tasks", task_id)
            return True
        except Exception as e:
            print(f"Error deleting task: {e}")
            return False
    
    def list_tasks(self, status: Optional[TaskStatus] = None,
                   priority: Optional[TaskPriority] = None) -> List[Task]:
        """Listet Tasks mit optionalen Filtern"""
        query = "FOR task IN tasks"
        filters = []
        
        if status:
            filters.append(f'task.status == "{status.value}"')
        if priority:
            filters.append(f'task.priority == "{priority.value}"')
        
        if filters:
            query += " FILTER " + " AND ".join(filters)
        
        query += " SORT task.created_at DESC RETURN task"
        
        try:
            results = self.client.query(query)
            return [Task.from_dict(data) for data in results]
        except Exception as e:
            print(f"Error listing tasks: {e}")
            return []
    
    def search_tasks(self, search_text: str) -> List[Task]:
        """Sucht Tasks nach Text in Titel oder Beschreibung"""
        query = """
        FOR task IN tasks
            FILTER CONTAINS(LOWER(task.title), LOWER(@search))
                OR CONTAINS(LOWER(task.description), LOWER(@search))
            SORT task.created_at DESC
            RETURN task
        """
        try:
            results = self.client.query(query, {"search": search_text})
            return [Task.from_dict(data) for data in results]
        except Exception as e:
            print(f"Error searching tasks: {e}")
            return []
```

### MVCC und Transaktionen demonstriert

Die Todo-App zeigt MVCC in Aktion:

**Szenario: Zwei Benutzer ändern gleichzeitig den gleichen Task**

```python
# User 1: Startet Transaction
user1 = TodoClient()
task = user1.get_task("task_123")  # Snapshot Version 100
task.status = TaskStatus.IN_PROGRESS
task.updated_at = datetime.now()

# User 2: Auch Transaction, gleicher Task
user2 = TodoClient()
task2 = user2.get_task("task_123")  # Auch Snapshot Version 100
task2.priority = TaskPriority.HIGH
task2.updated_at = datetime.now()

# User 1 committed zuerst
user1.update_task(task)  # ✓ SUCCESS, Version 101

# User 2 versucht zu committen
user2.update_task(task2)  # ✗ CONFLICT!
# Error: "Task was modified by another transaction"
```

**Was passiert intern?**

1. Beide lesen Version 100 (Snapshot Isolation)
2. User 1 schreibt Version 101 und committed
3. User 2 versucht auch Version 101 zu schreiben
4. ThemisDB erkennt: "Version 101 existiert bereits!"
5. Conflict Error wird geworfen

**Lösung im Code:**

```python
def update_task_with_retry(self, task: Task, max_retries=3):
    """Update mit automatischem Retry bei Conflicts"""
    for attempt in range(max_retries):
        try:
            self.client.update("tasks", task.id, task.to_dict())
            return True
        except ConflictError:
            # Re-read aktuellste Version
            latest = self.get_task(task.id)
            if not latest:
                return False
            
            # Merge changes (Application Logic)
            # z.B.: Keep newer updated_at
            if latest.updated_at > task.updated_at:
                task = latest
            
            # Retry mit gemergten Daten
            continue
        except Exception as e:
            return False
    
    return False  # Max retries exceeded
```

---

## 2.6 Index-Strukturen

### Warum Indizes?

Ohne Index: Full Table Scan

```sql
-- Ohne Index: O(n)
SELECT * FROM tasks WHERE status = 'open'
-- Muss ALLE Tasks durchgehen: 10.000 Tasks = 10.000 Reads
```

Mit Index: Direkt zum Ergebnis

```sql
-- Mit Index auf status: O(log n + k), k = Anzahl Results
SELECT * FROM tasks WHERE status = 'open'
-- B-Tree Lookup: log(10.000) ≈ 13 Reads + nur relevante Tasks
```

### Index-Typen in ThemisDB

**1. B-Tree Index (Default)**

```python
client.create_index("tasks", "status_idx", ["status"])
```

- Sortierte Daten
- Range Queries: `WHERE age > 18 AND age < 65`
- Equality: `WHERE status = 'open'`

**2. Hash Index**

```python
client.create_index("tasks", "id_hash_idx", ["id"], type="hash")
```

- Nur Equality: `WHERE id = 'task_123'`
- Schneller als B-Tree für Equality
- Keine Range Queries

**3. Geo Index (Hilbert Curves)**

```python
client.create_index("locations", "geo_idx", ["coordinates"], type="geo")
```

- Räumliche Queries
- Nearest Neighbor
- Bounding Box Searches

**4. Fulltext Index**

```python
client.create_index("tasks", "fulltext_idx", ["title", "description"], type="fulltext")
```

- Tokenization
- Stemming
- Relevance Scoring

**5. Vector Index (HNSW)**

```python
client.create_index("products", "embedding_idx", ["embedding"], type="vector", dimensions=768)
```

- Approximate Nearest Neighbor
- Cosine Similarity
- Euclidean Distance

### Index-Performance

| Operation | Ohne Index | Mit B-Tree | Mit Hash |
|-----------|-----------|------------|----------|
| Equality (`=`) | O(n) | O(log n) | O(1) |
| Range (`>`, `<`) | O(n) | O(log n + k) | ✗ |
| Sort | O(n log n) | O(k) | ✗ |

**k = Anzahl Ergebnisse**

---

## 2.7 Zusammenfassung

In diesem Kapitel haben Sie gelernt:

✅ **Schichten-Architektur:** 5 Schichten mit klaren Verantwortlichkeiten  
✅ **MVCC:** Parallelität ohne Locks durch Versionierung  
✅ **Transaktionen:** ACID-Garantien und Snapshot Isolation  
✅ **RocksDB:** LSM-Trees, WAL, Column Families  
✅ **Indizes:** B-Tree, Hash, Geo, Fulltext, Vector  
✅ **Todo-App:** Vollständige CRUD-Anwendung mit MVCC  

### Was macht ThemisDB besonders?

1. **Multi-Model MVCC:** Transaktionen über alle Modelle hinweg
2. **RocksDB Foundation:** Battle-tested, SSD-optimiert
3. **Flexible Indizes:** Für jeden Use Case der richtige Index
4. **Snapshot Isolation:** Performance + Konsistenz

### Nächste Schritte

Im nächsten Kapitel lernen Sie, wie die verschiedenen Datenmodelle kombiniert werden können und wann welches Modell am besten passt.

**[Kapitel 3: Multi-Model verstehen →](chapter_03_multimodel.md)**

---

## Weiterführende Ressourcen

- **Complete Example:** [examples/02_todo_app](../../examples/02_todo_app)
- **MVCC Deep-Dive:** [../de/architecture/architecture_mvcc.md](../de/architecture/architecture_mvcc.md)
- **RocksDB Storage:** [../de/storage/storage_rocksdb.md](../de/storage/storage_rocksdb.md)
- **Index Design:** [Kapitel 15 - Storage Internals](chapter_15_storage.md)

---

**Kapitel 2 von 30** | **Teil I: Grundlagen** | **~8.500 Wörter**
