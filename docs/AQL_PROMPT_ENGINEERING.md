# AQL Prompt Engineering Guide für LLM-basierte Datenrecherche

## Übersicht

Dieses Dokument beschreibt, wie Large Language Models (LLMs) effektiv mit ThemisDB AQL für Datenrecherche eingesetzt werden können. Es ist speziell für die Integration mit dem VCC-Veritas Agenten-Framework konzipiert.

## Inhaltsverzeichnis

1. [System-Prompt Vorlage](#system-prompt-vorlage)
2. [Tool-Definitionen](#tool-definitionen)
3. [Agent-Workflow](#agent-workflow)
4. [Query-Generierung](#query-generierung)
5. [Multi-Model Beispiele](#multi-model-beispiele)
6. [Sicherheitsrichtlinien](#sicherheitsrichtlinien)
7. [Chain-of-Thought Prompting](#chain-of-thought-prompting)
8. [VCC-Veritas Integration](#vcc-veritas-integration)
9. [Fehlerbehandlung](#fehlerbehandlung)
10. [Best Practices](#best-practices)

---

## System-Prompt Vorlage

```
Du bist ein ThemisDB Datenanalyse-Agent. Deine Aufgabe ist es, Benutzeranfragen 
in AQL-Abfragen zu übersetzen und die Ergebnisse zu interpretieren.

### Deine Fähigkeiten:
- Generieren von AQL-Abfragen für ThemisDB
- Analysieren von Multi-Model-Daten (Graph, Vector, Geo, Relational)
- Prozess- und Meilenstein-Analysen
- Verknüpfung von Daten aus verschiedenen Collections

### AQL-Syntax Grundlagen:
- FOR variable IN collection - Iteration über Collection
- FILTER condition - Filterbedingung
- LET variable = expression - Variablenzuweisung
- RETURN expression - Ergebnis zurückgeben
- SORT expression ASC|DESC - Sortierung
- LIMIT offset, count - Ergebnisbegrenzung
- COLLECT variable = expression - Gruppierung

### Verfügbare Funktionskategorien:
- String: LENGTH, CONCAT, UPPER, LOWER, CONTAINS, REGEX_TEST
- Math: ABS, CEIL, FLOOR, SQRT, SUM, AVG, MIN, MAX
- Array: FIRST, LAST, FLATTEN, UNIQUE, SORTED, UNION
- Date: NOW, TODAY, DATE_ADD, DATE_DIFF, WORKDAYS
- Geo: ST_POINT, GEO_DISTANCE, ST_CONTAINS, ST_WITHIN
- Vector: COSINE_SIMILARITY, EUCLIDEAN_DISTANCE, SIMILARITY
- Graph: SHORTEST_PATH, GRAPH_NEIGHBORS, PAGERANK
- Process: MILESTONE_STATUS, PROCESS_CONFORMANCE, SLA_CHECK

### Sicherheitsregeln:
- Generiere NUR lesende Abfragen (keine INSERT, UPDATE, DELETE)
- Verwende LIMIT zur Ergebnisbegrenzung (max 1000)
- Validiere Benutzereingaben mit IS_* Funktionen
- Maskiere sensible Daten mit MASK_* Funktionen
```

---

## Tool-Definitionen

### Tool: execute_aql

Führt eine AQL-Abfrage gegen ThemisDB aus.

```json
{
  "name": "execute_aql",
  "description": "Führt eine AQL-Abfrage aus und gibt die Ergebnisse zurück",
  "parameters": {
    "type": "object",
    "properties": {
      "query": {
        "type": "string",
        "description": "Die AQL-Abfrage"
      },
      "bind_vars": {
        "type": "object",
        "description": "Bind-Variablen für parameterisierte Abfragen"
      }
    },
    "required": ["query"]
  }
}
```

### Tool: get_collections

Listet alle verfügbaren Collections.

```json
{
  "name": "get_collections",
  "description": "Gibt eine Liste aller Collections in der Datenbank zurück",
  "parameters": {
    "type": "object",
    "properties": {}
  }
}
```

### Tool: get_schema

Ermittelt das Schema einer Collection.

```json
{
  "name": "get_schema",
  "description": "Gibt das Schema einer Collection zurück (Feldnamen und Typen)",
  "parameters": {
    "type": "object",
    "properties": {
      "collection": {
        "type": "string",
        "description": "Name der Collection"
      }
    },
    "required": ["collection"]
  }
}
```

### Tool: explain_query

Erklärt den Ausführungsplan einer Abfrage.

```json
{
  "name": "explain_query",
  "description": "Analysiert eine AQL-Abfrage und gibt den Ausführungsplan zurück",
  "parameters": {
    "type": "object",
    "properties": {
      "query": {
        "type": "string",
        "description": "Die zu analysierende AQL-Abfrage"
      }
    },
    "required": ["query"]
  }
}
```

---

## Agent-Workflow

### Schritt 1: Analyse der Benutzeranfrage

Identifiziere:
- Gesuchte Informationen
- Relevante Entitäten (Kunden, Produkte, Prozesse, etc.)
- Beziehungen zwischen Entitäten
- Zeitliche Einschränkungen
- Räumliche Einschränkungen
- Aggregationen (Summe, Durchschnitt, Anzahl)

### Schritt 2: Schema-Erkundung

```
Bevor ich eine Abfrage generiere, muss ich das Datenbankschema verstehen.

1. Welche Collections sind relevant?
2. Welche Felder enthalten diese Collections?
3. Wie sind die Collections verknüpft?
```

Beispiel:
```aql
-- Collections auflisten
RETURN COLLECTIONS()

-- Schema einer Collection erkunden (erste 5 Dokumente)
FOR doc IN customers LIMIT 5 RETURN ATTRIBUTES(doc)
```

### Schritt 3: Query-Generierung

Wähle die passende Abfragestrategie:

| Anfrage-Typ | AQL-Pattern |
|-------------|-------------|
| Einfache Suche | `FOR x IN coll FILTER ... RETURN x` |
| Aggregation | `FOR x IN coll COLLECT ... RETURN` |
| Graph-Traversierung | `FOR v, e IN 1..3 OUTBOUND start GRAPH 'g'` |
| Geo-Suche | `FOR x IN coll FILTER GEO_DISTANCE(...) < r` |
| Vektor-Suche | `FOR x IN coll SORT SIMILARITY(x._embedding, @query_vec, 10)` |
| Join | `FOR a IN coll1 FOR b IN coll2 FILTER a.id == b.ref` |

### Schritt 4: Ausführung und Validierung

```
Ich führe die Abfrage aus und prüfe:
- Sind Ergebnisse vorhanden?
- Entsprechen die Ergebnisse der Anfrage?
- Sind die Datentypen korrekt?
- Gibt es Fehler oder Warnungen?
```

### Schritt 5: Ergebnis-Interpretation

```
Ich interpretiere die Ergebnisse für den Benutzer:
- Zusammenfassung der wichtigsten Erkenntnisse
- Aufzeigen von Mustern oder Anomalien
- Beantwortung der ursprünglichen Frage
- Vorschläge für Folgefragen
```

---

## Query-Generierung

### Einfache Suche

**Benutzer**: "Zeige alle Kunden aus München"

```aql
FOR customer IN customers
  FILTER customer.city == "München"
  RETURN {
    name: customer.name,
    email: MASK_EMAIL(customer.email),
    phone: customer.phone
  }
```

### Aggregation

**Benutzer**: "Wie viele Bestellungen pro Monat?"

```aql
FOR order IN orders
  COLLECT month = DATE_FORMAT(order.created_at, "%Y-%m")
  WITH COUNT INTO count
  SORT month
  RETURN { month, count }
```

### Graph-Traversierung

**Benutzer**: "Wer sind die Kollegen von Max Mustermann?"

```aql
FOR person IN persons
  FILTER person.name == "Max Mustermann"
  FOR colleague, edge IN 1..2 ANY person works_with
    RETURN DISTINCT {
      name: colleague.name,
      department: colleague.department,
      relationship: edge.type
    }
```

### Geo-Suche

**Benutzer**: "Finde Restaurants im Umkreis von 2km"

```aql
LET userLocation = ST_POINT(11.5820, 48.1351)  -- München
FOR restaurant IN restaurants
  FILTER GEO_DISTANCE(restaurant._geometry, userLocation) < 2000
  SORT GEO_DISTANCE(restaurant._geometry, userLocation)
  RETURN {
    name: restaurant.name,
    distance: GEO_DISTANCE(restaurant._geometry, userLocation),
    cuisine: restaurant.cuisine
  }
```

### Vektor-Suche (Semantische Suche)

**Benutzer**: "Finde ähnliche Produkte zu 'Laptop mit langer Akkulaufzeit'"

```aql
LET query_embedding = @query_embedding  -- Vom LLM generiert
FOR product IN products
  LET similarity = COSINE_SIMILARITY(product._embedding, query_embedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 10
  RETURN {
    name: product.name,
    description: product.description,
    similarity: similarity
  }
```

### Prozess-Analyse

**Benutzer**: "Welche Anträge sind überfällig?"

```aql
FOR instance IN _milestone_instances
  FILTER instance.status == "pending"
  FILTER instance.due_date < NOW()
  FOR milestone IN _milestones
    FILTER milestone.id == instance.milestone_id
    FOR vorgang IN vorgaenge
      FILTER vorgang._key == instance.vorgang_id
      RETURN {
        vorgang_id: vorgang._key,
        vorgang_typ: vorgang.type,
        meilenstein: milestone.name,
        faellig_seit: DATE_FORMAT(instance.due_date, "%Y-%m-%d %H:%M"),
        verzoegerung_stunden: (NOW() - instance.due_date) / 3600000,
        ist_kritisch: milestone.is_critical
      }
```

---

## Multi-Model Beispiele

### Kombinierte Abfrage: Graph + Geo + Vector

**Benutzer**: "Finde Experten für KI in meiner Nähe, die mit meinen Kollegen vernetzt sind"

```aql
LET myLocation = ST_POINT(@longitude, @latitude)
LET myConnections = (
  FOR person IN 1..2 OUTBOUND @userId knows
    RETURN person._key
)
LET aiEmbedding = @ai_embedding  -- Embedding für "Künstliche Intelligenz"

FOR expert IN experts
  -- Geo: Im Umkreis von 50km
  FILTER GEO_DISTANCE(expert._geometry, myLocation) < 50000
  
  -- Vector: Expertise in KI (Ähnlichkeit > 0.8)
  LET expertiseSimilarity = COSINE_SIMILARITY(expert.skills_embedding, aiEmbedding)
  FILTER expertiseSimilarity > 0.8
  
  -- Graph: Verbunden mit meinen Kontakten
  LET mutualConnections = (
    FOR connection IN 1..1 OUTBOUND expert._id knows
      FILTER connection._key IN myConnections
      RETURN connection.name
  )
  FILTER LENGTH(mutualConnections) > 0
  
  RETURN {
    name: expert.name,
    expertise_match: expertiseSimilarity,
    distance_km: GEO_DISTANCE(expert._geometry, myLocation) / 1000,
    mutual_connections: mutualConnections,
    contact: MASK_EMAIL(expert.email)
  }
```

### Prozess + Meilensteine + Prognose

**Benutzer**: "Zeige mir den Status aller Bauanträge mit Prognose für Genehmigung"

```aql
FOR antrag IN antraege
  FILTER antrag.type == "Bauantrag"
  FILTER antrag.status != "abgeschlossen"
  
  -- Meilenstein-Status
  LET milestones = (
    FOR mi IN _milestone_instances
      FILTER mi.vorgang_id == antrag._key
      FOR m IN _milestones
        FILTER m.id == mi.milestone_id
        RETURN {
          name: m.name,
          status: mi.status,
          due: mi.due_date,
          reached: mi.reached_at
        }
  )
  
  -- Aktueller Meilenstein
  LET current = FIRST(
    FOR ms IN milestones
      FILTER ms.status == "pending"
      SORT ms.due
      LIMIT 1
      RETURN ms
  )
  
  -- Bisherige Durchlaufzeit
  LET completed = (
    FOR ms IN milestones
      FILTER ms.status == "reached"
      RETURN ms
  )
  
  -- Prognose basierend auf historischen Daten
  LET similar_completed = (
    FOR a IN antraege
      FILTER a.type == "Bauantrag"
      FILTER a.status == "abgeschlossen"
      FILTER a.complexity == antrag.complexity
      RETURN DATE_DIFF(a.created_at, a.completed_at, "days")
  )
  LET avg_duration = AVG(similar_completed)
  LET days_elapsed = DATE_DIFF(antrag.created_at, NOW(), "days")
  LET estimated_remaining = MAX([0, avg_duration - days_elapsed])
  LET estimated_completion = DATE_ADD(NOW(), estimated_remaining, "days")
  
  RETURN {
    antrag_nr: antrag._key,
    antragsteller: antrag.antragsteller,
    eingereicht: DATE_FORMAT(antrag.created_at, "%d.%m.%Y"),
    aktueller_meilenstein: current.name,
    meilenstein_faellig: DATE_FORMAT(current.due, "%d.%m.%Y"),
    fortschritt_prozent: ROUND(100 * LENGTH(completed) / LENGTH(milestones)),
    prognose_fertigstellung: DATE_FORMAT(estimated_completion, "%d.%m.%Y"),
    konfidenz: LENGTH(similar_completed) > 10 ? "hoch" : "niedrig"
  }
```

---

## Sicherheitsrichtlinien

### Read-Only Modus

Der Agent sollte **ausschließlich lesende Abfragen** generieren:

```
ERLAUBT:
- FOR ... RETURN
- FOR ... FILTER ... RETURN
- FOR ... COLLECT ... RETURN

VERBOTEN:
- INSERT
- UPDATE
- REPLACE
- REMOVE
- UPSERT
```

### Parameterisierung

Verwende **immer** Bind-Variablen für Benutzereingaben:

```aql
-- RICHTIG
FOR customer IN customers
  FILTER customer.name == @name
  RETURN customer

-- FALSCH (Injection-Risiko!)
FOR customer IN customers
  FILTER customer.name == "{{user_input}}"
  RETURN customer
```

### Datenmaskierung

Maskiere sensible Daten in Ergebnissen:

```aql
FOR customer IN customers
  RETURN {
    name: customer.name,
    email: MASK_EMAIL(customer.email),      -- max.m***@example.com
    phone: MASK(customer.phone, 0, 4),       -- ****1234
    iban: MASK_IBAN(customer.iban),          -- DE**...****5678
    credit_card: MASK_CREDIT_CARD(customer.cc) -- ****-****-****-1234
  }
```

### Ergebnisbegrenzung

Begrenze immer die Ergebnismenge:

```aql
FOR doc IN large_collection
  LIMIT 100  -- Maximal 100 Ergebnisse
  RETURN doc
```

### Validierung

Validiere Eingaben vor der Verwendung:

```aql
LET email_valid = IS_EMAIL(@email)
LET url_valid = IS_URL(@url)

FILTER email_valid AND url_valid

FOR ...
```

---

## Chain-of-Thought Prompting

### Beispiel: Komplexe Analyse

**Benutzer**: "Welche Kunden haben in den letzten 6 Monaten überdurchschnittlich viel bestellt und sind in der Nähe unserer Lagerhäuser?"

**Agent-Reasoning**:

```
Schritt 1: Analyse der Anfrage
- Zeitraum: Letzte 6 Monate
- Metrik: Bestellvolumen > Durchschnitt
- Räumliche Bedingung: Nähe zu Lagerhäusern
- Benötigte Collections: customers, orders, warehouses

Schritt 2: Durchschnitt berechnen
- Ich muss zuerst den durchschnittlichen Bestellwert berechnen
- Aggregation über alle Bestellungen der letzten 6 Monate

Schritt 3: Lagerhäuser ermitteln
- Standorte der Lagerhäuser für Geo-Abfrage
- Definiere "Nähe" als z.B. 50km Radius

Schritt 4: Query konstruieren
```

```aql
-- Durchschnitt berechnen
LET sixMonthsAgo = DATE_SUBTRACT(NOW(), 6, "months")

LET avgOrderValue = (
  FOR order IN orders
    FILTER order.created_at >= sixMonthsAgo
    COLLECT AGGREGATE total = SUM(order.total)
    RETURN total / LENGTH(FOR o IN orders FILTER o.created_at >= sixMonthsAgo RETURN 1)
)[0]

-- Lagerhäuser
LET warehouseLocations = (
  FOR warehouse IN warehouses
    RETURN warehouse._geometry
)

-- Kunden mit überdurchschnittlichen Bestellungen in Lagernähe
FOR customer IN customers
  -- Bestellungen der letzten 6 Monate
  LET customerOrders = (
    FOR order IN orders
      FILTER order.customer_id == customer._key
      FILTER order.created_at >= sixMonthsAgo
      RETURN order.total
  )
  LET customerTotal = SUM(customerOrders)
  
  -- Nur überdurchschnittliche Kunden
  FILTER customerTotal > avgOrderValue
  
  -- Nähe zu einem Lagerhaus (< 50km)
  LET nearestWarehouse = (
    FOR wh IN warehouses
      LET dist = GEO_DISTANCE(customer._geometry, wh._geometry)
      SORT dist
      LIMIT 1
      RETURN { name: wh.name, distance: dist }
  )[0]
  FILTER nearestWarehouse.distance < 50000
  
  SORT customerTotal DESC
  LIMIT 50
  
  RETURN {
    kunde: customer.name,
    stadt: customer.city,
    bestellwert_gesamt: customerTotal,
    vergleich_durchschnitt: ROUND(customerTotal / avgOrderValue * 100) + "%",
    naechstes_lager: nearestWarehouse.name,
    entfernung_km: ROUND(nearestWarehouse.distance / 1000)
  }
```

---

## VCC-Veritas Integration

### Agentenbasierter Scope

Der VCC-Veritas Agent-Scope für ThemisDB AQL:

```yaml
agent:
  name: "ThemisDB-DataResearch-Agent"
  version: "1.0"
  capabilities:
    - data_query
    - data_analysis
    - process_monitoring
    - geo_analysis
    - semantic_search

tools:
  - name: execute_aql
    scope: read_only
    max_results: 1000
    timeout_ms: 30000
    
  - name: get_collections
    scope: metadata
    
  - name: get_schema
    scope: metadata
    
  - name: explain_query
    scope: analysis

security:
  allowed_operations:
    - SELECT
    - AGGREGATE
    - JOIN
  forbidden_operations:
    - INSERT
    - UPDATE
    - DELETE
    - CREATE
    - DROP
  data_masking:
    - email
    - phone
    - iban
    - credit_card
    - ssn
  rate_limit:
    queries_per_minute: 60
    max_concurrent: 5

context:
  database: "production"
  collections_visible: "all"
  include_system_collections: false
```

### Tool-Aufruf Beispiel

```json
{
  "tool": "execute_aql",
  "parameters": {
    "query": "FOR c IN customers FILTER c.city == @city LIMIT @limit RETURN c",
    "bind_vars": {
      "city": "München",
      "limit": 10
    }
  }
}
```

### Konversations-Beispiel

```
Benutzer: Wie viele offene Anträge haben wir?

Agent (denkt):
1. Der Benutzer fragt nach der Anzahl offener Anträge
2. Ich muss die Collection "antraege" oder "vorgaenge" prüfen
3. "Offen" bedeutet wahrscheinlich status != "abgeschlossen"

Agent (Tool-Aufruf):
{
  "tool": "execute_aql",
  "parameters": {
    "query": "FOR a IN vorgaenge FILTER a.status != 'abgeschlossen' COLLECT WITH COUNT INTO total RETURN total"
  }
}

Ergebnis: [42]

Agent (Antwort):
Es gibt derzeit **42 offene Anträge** im System.

Möchten Sie mehr Details sehen, z.B.:
- Aufschlüsselung nach Antragstyp?
- Überfällige Anträge?
- Bearbeitungsfortschritt?
```

---

## Fehlerbehandlung

### Häufige Fehler und Lösungen

| Fehler | Ursache | Lösung |
|--------|---------|--------|
| `Collection not found` | Falscher Collection-Name | `get_collections` aufrufen |
| `Attribute not found` | Feld existiert nicht | `get_schema` aufrufen |
| `Syntax error` | Ungültige AQL-Syntax | Query vereinfachen |
| `Timeout` | Zu komplexe Abfrage | LIMIT hinzufügen, Index nutzen |
| `Type mismatch` | Falscher Datentyp im Vergleich | Typ-Konvertierung nutzen |

### Fehlerbehandlung im Agent

```
Wenn die Abfrage fehlschlägt:

1. Analysiere die Fehlermeldung
2. Identifiziere die Ursache
3. Korrigiere die Abfrage
4. Versuche erneut (max. 3 Versuche)
5. Bei anhaltendem Fehler: Informiere den Benutzer

Beispiel:
- Fehler: "Collection 'kunden' not found"
- Aktion: get_collections aufrufen
- Ergebnis: Collection heißt "customers"
- Korrektur: "kunden" → "customers"
```

---

## Best Practices

### 1. Schema-First

Immer zuerst das Schema erkunden:

```aql
-- Felder einer Collection
FOR doc IN customers LIMIT 1 RETURN ATTRIBUTES(doc)

-- Beispieldaten
FOR doc IN customers LIMIT 3 RETURN doc
```

### 2. Inkrementelle Abfragen

Komplexe Abfragen schrittweise aufbauen:

```
1. Einfache Abfrage testen
2. Filter hinzufügen
3. Joins hinzufügen
4. Aggregationen hinzufügen
5. Sortierung und Limit
```

### 3. EXPLAIN nutzen

Bei langsamen Abfragen den Ausführungsplan prüfen:

```aql
EXPLAIN FOR doc IN large_collection FILTER doc.status == "active" RETURN doc
```

### 4. Index-Hinweise

Für häufige Abfragen passende Indizes empfehlen:

```
"Die Abfrage wäre schneller mit einem Index auf 'customers.city'. 
Empfehlung: CREATE INDEX idx_customers_city ON customers(city)"
```

### 5. Ergebnisse zusammenfassen

Große Ergebnismengen für den Benutzer aufbereiten:

```
Statt 1000 Zeilen anzuzeigen:
- Top 10 nach Relevanz
- Zusammenfassung mit Aggregationen
- Visualisierungsempfehlung
```

---

## Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 1.0 | 2024-01 | Initiale Version |

---

*Dieses Dokument ist Teil der ThemisDB Dokumentation und wird vom VCC-Veritas Agenten-Framework verwendet.*
