# ThemisDB: Integrierte Multi-Model Architektur

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Design-Prinzipien](#design-prinzipien)
- [Prozess-Collections](#prozess-collections)
- [Integrierte Multi-Model-Abfragen](#integrierte-multi-model-abfragen)


## Übersicht

ThemisDB verwendet einen **vollständig integrierten** Ansatz für Multi-Model-Abfragen.
Anstatt separate Sprachelemente für Graph, Vektor, Geo und Prozesse einzuführen,
werden alle Datentypen als **Collections** behandelt und über den bestehenden
AQL-Wortschatz abgefragt.

## Design-Prinzipien

### 1. Einheitliche Collection-Abstraktion

Alle Datentypen werden als Collections modelliert:

```
┌─────────────────────────────────────────────────────────────────┐
│                     ThemisDB Collection                          │
├─────────────────────────────────────────────────────────────────┤
│  Dokument (Relational)  │  Felder, Indizes, Constraints         │
│  Graph (Knoten/Kanten)  │  _from, _to, _type, Adjazenzlisten    │
│  Vektor (Embeddings)    │  _embedding, Dimensionen, Index       │
│  Geo (Geometrie)        │  _geometry, SRID, Spatial-Index       │
│  Temporal (Zeit)        │  _valid_from, _valid_to               │
│  Prozess (Workflow)     │  _state, _tokens, _variables          │
└─────────────────────────────────────────────────────────────────┘
```

### 2. Reservierte Felder für Multi-Model

| Feld | Typ | Beschreibung |
|------|-----|--------------|
| `_from` | string | Graph: Quellknoten-ID |
| `_to` | string | Graph: Zielknoten-ID |
| `_type` | string | Graph: Kantentyp / Prozess: Knotentyp |
| `_embedding` | float[] | Vektor: Embedding-Array |
| `_geometry` | string/object | Geo: WKT oder GeoJSON |
| `_valid_from` | int64 | Temporal: Gültig ab (ms) |
| `_valid_to` | int64 | Temporal: Gültig bis (ms) |
| `_state` | string | Prozess: Ausführungszustand |
| `_parent` | string | Prozess: Eltern-Instanz |

### 3. Bestehende AQL-Syntax für alle Modelle

#### FOR ... IN Collection
```aql
-- Dokumente (Relational)
FOR doc IN customers
  FILTER doc.country == "DE"
  RETURN doc

-- Graph-Knoten
FOR node IN process_nodes
  FILTER node._type == "USER_TASK"
  RETURN node

-- Graph-Kanten
FOR edge IN process_edges
  FILTER edge._type == "SEQUENCE_FLOW"
  RETURN edge

-- Prozess-Instanzen (sind auch Dokumente)
FOR instance IN process_instances
  FILTER instance._state == "RUNNING"
  RETURN instance
```

#### Graph-Traversierung (bestehende Syntax)
```aql
-- Prozess-Fluss traversieren
FOR v, e, p IN 1..10 OUTBOUND "start_event" process_edges
  FILTER e._type == "SEQUENCE_FLOW"
  RETURN { node: v, edge: e, path: p }

-- Kürzester Prozess-Pfad
FOR v IN SHORTEST_PATH "start" TO "end" process_edges
  RETURN v
```

#### Vektor-Suche (SIMILARITY)
```aql
-- Ähnliche Prozesse finden
FOR process IN process_definitions
  LET sim = SIMILARITY(process._embedding, [0.1, 0.2, ...], 10)
  FILTER sim > 0.8
  RETURN { process, similarity: sim }
```

#### Geo-Abfragen (PROXIMITY, GEO_*)
```aql
-- Aufgaben in der Nähe
FOR task IN active_tasks
  FILTER GEO_DISTANCE(task._geometry, [8.68, 50.11]) < 10000
  RETURN task
```

## Prozess-Collections

### Systemdefinierte Collections

| Collection | Beschreibung |
|------------|--------------|
| `_process_definitions` | Prozess-Modelle (BPMN/EPK) |
| `_process_nodes` | Knoten im Prozess-Modell |
| `_process_edges` | Kanten/Flüsse im Prozess-Modell |
| `_process_instances` | Laufende Prozess-Instanzen |
| `_process_tokens` | Token (Ausführungsposition) |
| `_process_history` | Audit-Log der Ausführung |
| `_process_variables` | Prozess-Variablen |

### Beispiel-Abfragen

#### 1. Alle laufenden Bestellprozesse
```aql
FOR instance IN _process_instances
  FILTER instance.process_id == "order-process"
  FILTER instance._state == "RUNNING"
  RETURN instance
```

#### 2. Aktive Benutzeraufgaben für einen Benutzer
```aql
FOR token IN _process_tokens
  FOR node IN _process_nodes
    FILTER token.current_node == node.id
    FILTER node._type == "USER_TASK"
    FILTER node.assignee == "john.doe"
    RETURN { task: node, token: token }
```

#### 3. Prozess-Fluss-Analyse
```aql
FOR v, e IN 1..20 OUTBOUND "start_event" _process_edges
  COLLECT type = e._type WITH COUNT INTO count
  RETURN { edgeType: type, count: count }
```

#### 4. Cross-Instance Korrelation
```aql
FOR order IN _process_instances
  FILTER order.process_id == "order-process"
  FOR shipping IN _process_instances
    FILTER shipping.process_id == "shipping-process"
    FILTER order.variables.orderId == shipping.variables.orderId
    RETURN { order, shipping }
```

## Integrierte Multi-Model-Abfragen

### Kombination aller Modelle in einer Abfrage

```aql
-- Finde überfällige Aufgaben in der Nähe des Kunden
-- mit ähnlichen historischen Fällen

FOR task IN _process_tokens
  -- Relational: Join mit Prozess-Knoten
  FOR node IN _process_nodes
    FILTER task.current_node == node.id
    FILTER node._type == "USER_TASK"
    
  -- Relational: Join mit Kunden-Daten
  LET customer = DOCUMENT("customers", task.variables.customerId)
  
  -- Temporal: Überfällige Aufgaben (> 24h)
  FILTER DATE_DIFF(task.created_at, DATE_NOW(), "hour") > 24
  
  -- Geo: Aufgaben in der Nähe des Kunden
  FILTER GEO_DISTANCE(node._geometry, customer._geometry) < 50000
  
  -- Vektor: Ähnliche historische Fälle
  LET similar = (
    FOR hist IN _process_history
      FILTER SIMILARITY(hist._embedding, task._embedding) > 0.85
      LIMIT 5
      RETURN hist
  )
  
  RETURN {
    task: task,
    node: node,
    customer: customer,
    waitingHours: DATE_DIFF(task.created_at, DATE_NOW(), "hour"),
    distanceKm: GEO_DISTANCE(node._geometry, customer._geometry) / 1000,
    similarCases: similar
  }
```

## Prozess-Funktionen (als reguläre AQL-Funktionen)

### Ausführungsfunktionen

| Funktion | Beschreibung |
|----------|--------------|
| `PROCESS_START(processId, vars)` | Startet neue Instanz |
| `PROCESS_SIGNAL(instanceId, event, payload)` | Sendet Signal |
| `PROCESS_SUSPEND(instanceId)` | Pausiert Instanz |
| `PROCESS_RESUME(instanceId)` | Setzt Instanz fort |
| `PROCESS_TERMINATE(instanceId, reason)` | Beendet Instanz |

### Aufgaben-Funktionen

| Funktion | Beschreibung |
|----------|--------------|
| `TASK_COMPLETE(instanceId, nodeId, output)` | Schließt Aufgabe ab |
| `TASK_CLAIM(instanceId, nodeId, user)` | Übernimmt Aufgabe |
| `TASK_DELEGATE(instanceId, nodeId, newUser)` | Delegiert Aufgabe |

### Analyse-Funktionen

| Funktion | Beschreibung |
|----------|--------------|
| `PROCESS_DURATION(instanceId)` | Laufzeit in ms |
| `TASK_DURATION(instanceId, nodeId)` | Aufgaben-Dauer in ms |
| `PROCESS_PATH(instanceId)` | Durchlaufener Pfad |
| `PROCESS_VARIABLES(instanceId)` | Alle Variablen |

### Beispiel mit Funktionen

```aql
-- Starte Prozess und erhalte Instanz-ID
LET instanceId = PROCESS_START("order-process", { orderId: "ORD-123", amount: 500 })

-- Gib Instanz-Details zurück
FOR instance IN _process_instances
  FILTER instance.id == instanceId
  RETURN {
    id: instance.id,
    state: instance._state,
    variables: PROCESS_VARIABLES(instance.id),
    path: PROCESS_PATH(instance.id)
  }
```

## Implementierungs-Hinweise

### 1. Collection-Registry

Die Collection-Registry erkennt automatisch den Datentyp basierend auf:
- Präfix `_process_*` → Prozess-Collection
- Feld `_from`/`_to` vorhanden → Graph-Edge
- Feld `_embedding` vorhanden → Vektor-fähig
- Feld `_geometry` vorhanden → Geo-fähig

### 2. Query-Optimizer

Der Query-Optimizer wählt automatisch den optimalen Ausführungsplan:
- Graph-Traversierung → GraphIndexManager
- Vektor-Suche → VectorIndex (HNSW/FAISS)
- Geo-Abfragen → SpatialIndex (R-Tree)
- Relationale Filter → SecondaryIndex

### 3. Einheitlicher Executor

Ein einziger QueryExecutor verarbeitet alle Abfragetypen:

```cpp
class QueryExecutor {
    // Dispatch basierend auf Collection-Typ und Operationen
    Result execute(const Query& query) {
        for (const auto& forNode : query.for_nodes) {
            auto collType = registry_.getCollectionType(forNode.collection);
            
            switch (collType) {
                case CollectionType::Document:
                    return executeRelational(forNode);
                case CollectionType::Graph:
                    return executeGraphTraversal(forNode);
                case CollectionType::Process:
                    return executeProcess(forNode);
            }
        }
    }
};
```

## Fazit

Durch die vollständige Integration:

1. **Keine neuen Sprachelemente** - Bestehender AQL-Wortschatz reicht
2. **Einheitliche Semantik** - FOR, FILTER, RETURN für alles
3. **Transparente Optimierung** - System wählt besten Index
4. **Kombinierbare Abfragen** - Multi-Model in einer Query
5. **Einfache Lernkurve** - Ein Sprachkonzept für alle Modelle
