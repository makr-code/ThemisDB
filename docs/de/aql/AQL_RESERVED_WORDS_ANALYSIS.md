# AQL Reserved Words Analysis - v1.3.0 vs v1.3.1 Proposal

**Datum:** 22. Dezember 2025  
**Zweck:** Aufwandsabschätzung für Wire Protocol und Client-Implementierungen  
**Status:** Analyse für Proposal v1.3.1

---

## Zusammenfassung

| Version | Reservierte Wörter | Neue Wörter | Änderung |
|---------|-------------------|-------------|----------|
| **v1.3.0 (Aktuell)** | **72** | - | Baseline |
| **v1.3.1 (Final)** ✅ | **119** | **+47** | **+65%** |

### Kategorisierung der Änderung

- **Core Language Extensions**: +28 Wörter (OOP, Control Flow)
- **Vision Extensions**: +9 Wörter (optimiert, Detection Types als Strings)
- **Type System**: +6 Wörter (Primitive Typen)
- **Async/Macros**: +4 Wörter (Future Features)

**✅ Optimierung umgesetzt:** Detection Types (objects, text, faces, landmarks, emotions, brands, celebrities, scenes) werden als **String-Konstanten** statt Keywords implementiert → **-8 Keywords**

---

## Detaillierte Aufschlüsselung

### v1.3.0 - Aktuelle Reservierte Wörter (72 Total)

#### Core Language (8 Wörter)
```
FOR, IN, LET, FILTER, COLLECT, SORT, LIMIT, RETURN
```

#### Logical Operators (3 Wörter)
```
AND, OR, NOT
```

#### Comparison (2 Wörter)
```
IN (bereits in Core), LIKE
```

#### Aggregation (8 Wörter)
```
COUNT, SUM, AVG, MIN, MAX, VARIANCE, STDDEV, UNIQUE
```

#### Graph (4 Wörter)
```
OUTBOUND, INBOUND, ANY, SHORTEST_PATH
```

#### DDL - Data Definition (5 Wörter)
```
CREATE, DROP, COLLECTION, INDEX, VIEW
```

#### DML - Data Manipulation (6 Wörter)
```
INSERT, UPDATE, REPLACE, REMOVE, UPSERT, INTO, WITH
```

#### LLM Extensions v1.3.0 (13 Wörter)
```
LLM, INFER, RAG, EMBED, MODEL, LORA, STATS, CACHE, 
LOAD, UNLOAD, LIST, INGEST, BLOB
```

#### Index Types (7 Wörter)
```
HASH, SKIPLIST, FULLTEXT, GEO, PERSISTENT, TTL, VECTOR
```

#### Options & Modifiers (13 Wörter)
```
OPTIONS, DISTINCT, ASC, DESC, FROM, TO, TOP, USING, 
PIN, REPLICATE, VERSION, RESPONSE, PREFIX
```

#### Literals (3 Wörter)
```
null, true, false
```

**Gesamt v1.3.0: 72 Reservierte Wörter**

---

### v1.3.1 - Neue Reservierte Wörter (47 Neue) ✅ FINAL

#### 1. OOP - Namespace System (2 Wörter)
```
NAMESPACE, IMPORT
```

#### 2. OOP - Type System (6 Wörter)
```
TYPE, String, Int, Float, Bool, Any
```

#### 3. OOP - Functions & Classes (7 Wörter)
```
FUNCTION, CLASS, PUBLIC, PRIVATE, CONSTRUCTOR, METHOD, NEW
```

#### 4. OOP - References (3 Wörter)
```
THIS, SELF, EXTENDS
```

#### 5. Control Flow (10 Wörter)
```
IF, THEN, ELSE, ELSEIF, ENDIF, 
TRY, CATCH, THROW, CASE, END
```

#### 6. Pattern Matching (2 Wörter)
```
MATCH, WHEN
```

#### 7. Async Operations (4 Wörter)
```
ASYNC, AWAIT, PARALLEL, TIMEOUT
```

#### 8. Vision-Specific Commands (9 Wörter) ✅ OPTIMIERT
```
VISION, ANALYZE, DETECT, QUESTION, ABOUT, IMAGE, IMAGES,
TRANSFORM, COMPARE, BATCH, OPERATIONS, OUTPUT, METRIC
```

**✅ OPTIMIERUNG UMGESETZT:**
Detection Types (objects, text, faces, landmarks, emotions, brands, celebrities, scenes) werden als **String-Konstanten** behandelt:
```aql
-- Statt Keywords: DETECT [objects, text, faces]
-- Als Strings: DETECT ['objects', 'text', 'faces']
```
**Einsparung: -8 Keywords**

#### 9. Additional Type Keywords (4 Wörter)
```
Array, Map, Result, Object
```

#### 10. Macro System (1 Wort)
```
MACRO
```

**Gesamt v1.3.1 Neu: +47 Wörter** ✅ FINAL (statt +55)

**Gesamt v1.3.1: 119 Reservierte Wörter** ✅ FINAL

---

## Komplexitätsanalyse für Implementierung

### 1. Wire Protocol Änderungen

#### Minimale Änderungen (Empfohlen)
**Aufwand:** ~1-2 Personenwochen

- Wire Protocol muss nur **Tokens/Keywords** serialisieren
- Kein neues Format nötig, da Keywords als String-Tokens übertragen werden
- Bestehende Protokoll-Version kann erweitert werden

**Änderungen:**
```protobuf
// Keine Breaking Changes nötig
// Nur Parser muss neue Keywords erkennen
message AQLQuery {
  string query_text = 1;  // Enthält bereits alle Keywords als Text
  map<string, Value> bind_vars = 2;
  QueryOptions options = 3;
}
```

**Vorteil:** Wire Protocol ist text-basiert, keine Serialisierungs-Änderungen nötig.

#### Wenn Token-basiertes Protocol (Alternativ)
**Aufwand:** ~3-4 Personenwochen

Falls Wire Protocol Token-IDs statt Text nutzt:
```protobuf
enum AQLKeyword {
  // v1.3.0 (72 existing)
  FOR = 1;
  IN = 2;
  // ... (70 more)
  
  // v1.3.1 additions (55 new)
  NAMESPACE = 73;
  IMPORT = 74;
  TYPE = 75;
  // ... (52 more)
}
```

---

### 2. Client Library Änderungen

#### Client-seitige Komplexität

**Aufwand pro Client:** ~0.5-1 Personenwoche

Client-Libraries müssen hauptsächlich:
1. **Query Builder aktualisieren** (falls vorhanden)
2. **Syntax Highlighting** erweitern
3. **Autocomplete** aktualisieren
4. **Dokumentation** anpassen

#### Beispiel: Python Client
```python
# Kein Breaking Change nötig
# v1.3.0 Code funktioniert weiterhin
result = client.aql.execute("""
    FOR doc IN collection
      RETURN doc
""")

# v1.3.1 neue Features optional nutzbar
result = client.aql.execute("""
    NAMESPACE myapp;
    TYPE MyType { field: String }
    FUNCTION myFunc() -> MyType { ... }
    
    FOR doc IN collection
      RETURN doc
""")
```

**Backward Compatibility:** Vollständig gegeben, da v1.3.0 Queries unverändert funktionieren.

#### Betroffene Clients
1. **Python** (`themis-python`) - Query Builder, Typing
2. **JavaScript/TypeScript** (`themis-js`) - Query Builder, TypeScript Definitions
3. **Java** (`themis-java`) - Query Builder
4. **Go** (`themis-go`) - Query Builder
5. **C#** (`themis-csharp`) - Query Builder, LINQ Provider
6. **Ruby** (`themis-ruby`) - Query Builder
7. **PHP** (`themis-php`) - Query Builder
8. **Rust** (`themis-rust`) - Query Builder
9. **Swift** (`themis-swift`) - Query Builder

**Gesamt-Aufwand für alle Clients:** ~4-9 Personenwochen (parallel durchführbar)

---

### 3. Parser & Compiler Änderungen

#### Server-seitige Komplexität
**Aufwand:** ~8-16 Personenwochen (abhängig von Phase)

- **Lexer:** +55 neue Tokens erkennen (~1 Woche)
- **Parser:** Neue Grammatik-Regeln (~4-8 Wochen je nach Phase)
- **Type Checker:** Neues Type System (~2-4 Wochen)
- **Code Generator:** Neue Konstrukte (~2-4 Wochen)

**Phasen-Aufschlüsselung:**

| Phase | Features | Parser-Aufwand | Client-Aufwand |
|-------|----------|----------------|----------------|
| v1.3.1 (Q1 2026) | Namespace, UDFs, Basic Types, Pipeline | 4-6 Wochen | 2-3 Wochen |
| v1.3.2 (Q2 2026) | Vision, Error Handling | 2-3 Wochen | 1-2 Wochen |
| v1.4.0 (Q3 2026) | Full Types, Pattern Match, Classes | 4-6 Wochen | 2-3 Wochen |
| v1.5.0 (Q4 2026) | Async/Await | 3-4 Wochen | 1-2 Wochen |

---

## Optimierungen zur Aufwandsreduktion

### Option 1: Detection Types als String-Konstanten (Empfohlen)
**Einsparung:** -8 Reservierte Wörter

Statt:
```aql
DETECT [objects, text, faces]  -- Keywords
```

Nutze:
```aql
DETECT ['objects', 'text', 'faces']  -- Strings
```

**Neue Anzahl:** 119 statt 127 (-6%)

### Option 2: Context-sensitive Keywords
**Einsparung:** Keine Reduktion, aber weniger Konflikte

Manche Keywords nur in bestimmtem Kontext reserviert:
- `VISION` nur nach `LLM`
- `ANALYZE`, `DETECT` nur nach `LLM VISION`
- `CONSTRUCTOR`, `METHOD` nur in `CLASS`

**Vorteil:** Nutzer können diese Wörter als Identifier außerhalb des Kontexts verwenden.

### Option 3: Verzögerte Implementierung optionaler Features
**Einsparung:** -11 Wörter (Phase 1)

Implementiere nur kritische Features in v1.3.1:
- Namespace, Import (2)
- Type, Function (2)
- String, Int, Float, Bool, Any (5) 
- Pipeline Operator (0 neue Keywords, nur Operator `|>`)

**Neue Anzahl v1.3.1 minimal:** 81 statt 127 (-36%)

---

## Risikoanalyse

### Niedriges Risiko
- ✅ Wire Protocol: Text-basiert, keine Breaking Changes
- ✅ Backward Compatibility: v1.3.0 Queries funktionieren unverändert
- ✅ Client Libraries: Nur additive Änderungen

### Mittleres Risiko
- ⚠️ Keyword-Konflikte: Nutzer könnten `TYPE`, `CLASS` als Identifier nutzen
- ⚠️ Learning Curve: Mehr Konzepte für Entwickler

### Mitigations
- Context-sensitive Keywords wo möglich
- Ausführliche Migration Guides
- Backward Compatibility testen
- Deprecation Warnings für konfliktäre Identifier

---

## Empfehlung

### Für v1.3.1 (Minimale Erweiterung)
**Reservierte Wörter:** 81 (+9 von v1.3.0)

Nur kritische Features:
- Namespace/Import (2)
- Type System Basics (7)
- Function (1)
- Vision Extensions ohne Detection Types als Keywords (9)

**Aufwand:**
- Wire Protocol: 1 Woche
- Parser/Compiler: 4-6 Wochen
- Clients (alle 9): 2-3 Wochen (parallel)
- **Gesamt: 7-10 Wochen** (~2 Monate)

### Für v1.3.1 (Vollständige Proposal)
**Reservierte Wörter:** 119-127 (+47-55 von v1.3.0)

Alle in Proposal beschriebenen Features.

**Aufwand:**
- Wire Protocol: 1-2 Wochen
- Parser/Compiler: 8-12 Wochen
- Clients (alle 9): 4-6 Wochen (parallel)
- **Gesamt: 13-20 Wochen** (~3-5 Monate)

### Empfohlener Ansatz: Phasenweise Einführung
Implementiere Features in 4 Phasen über 12 Monate, um:
- Aufwand zu verteilen
- Feedback einzuarbeiten
- Breaking Changes zu vermeiden
- Team-Kapazität zu schonen

---

## Anhang: Vollständige Keyword-Liste v1.3.1

### Alphabetisch sortiert (119 Wörter) ✅ FINAL

```
ALL, ANALYZE, AND, ANY, Array, AS, ASC, ASYNC, AWAIT, AVG,
BATCH, BLOB, Bool, 

CACHE, CASE, CATCH, CLASS, CLEAR, COLLECT, COLLECTION, COMPARE, 
CONSTRUCTOR, COUNT, CREATE,

DESC, DETECT, DISTINCT, DROP,

ELSEIF, ELSE, EMBED, END, ENDIF, EXTENDS,

FILTER, Float, FOR, FROM, FULLTEXT, FUNCTION,

GEO,

HASH,

IF, IMAGE, IMAGES, IMPORT, INBOUND, INDEX, INFER, INGEST, INSERT, 
Int, INTO,

LET, LIKE, LIMIT, LIST, LOAD, LORA,

MACRO, MAP, MATCH, MAX, METHOD, METRIC, MIN, MODEL,

NAMESPACE, NEW, NOT,

Object, OPERATIONS, OPTIONS, OR, OUTBOUND, OUTPUT,

PARALLEL, PERSISTENT, PIN, PREFIX, PRIVATE, PUBLIC,

QUESTION,

RAG, REMOVE, REPLACE, REPLICATE, RESPONSE, Result, RETURN,

SCALE, SELF, SHORTEST_PATH, SKIPLIST, SORT, STATS, STDDEV, String, SUM,

THEN, THIS, THROW, TIMEOUT, TO, TOP, TRANSFORM, TRY, TTL, TYPE,

UNLOAD, UNIQUE, UPDATE, UPSERT, USING,

VARIANCE, VECTOR, VERSION, VIEW, VISION,

WHEN, WHERE, WITH,

false, null, true
```

**Hinweis:** Detection Types (objects, text, faces, landmarks, emotions, brands, celebrities, scenes) sind **NICHT** in dieser Liste, da sie als String-Konstanten implementiert werden.

---

## Kontakt
Für Rückfragen zum Aufwand oder zur Implementierung:
- **Proposal:** `/docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md`
- **Grammar:** `/aql/AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf`
- **Summary:** `/docs/de/aql/AQL_EXTENSION_EXECUTIVE_SUMMARY.md`
