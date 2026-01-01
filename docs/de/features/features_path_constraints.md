---
category: "📈 Graph Features"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🛣️ Graph Traversal Path Constraints

Constraints für Graph-Pfade und Traversals.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

---

## Motivation

Aktuell werden FILTER-Ausdrücke in Traversals **nur am letzten Level** vor dem Enqueue angewendet (konservatives Pruning). Dies ist sicher, aber lässt Optimierungspotenzial auf Zwischenebenen ungenutzt.

**Pfad-Constraints** ermöglichen aggressiveres Pruning auf allen Tiefen, indem Prädikate entlang des gesamten Pfads gelten.

---

## Problem: Naive Anwendung ist unsicher

### Beispiel: Unsicheres Edge-Pruning

**Query:**
```aql
FOR v IN 1..3 OUTBOUND 'user1' GRAPH 'social'
  FILTER e.type == 'follows'
  RETURN v
```

**Naive (falsche) Interpretation:**
- "Schneide alle Kanten ab, bei denen `e.type != 'follows'`"
- **Problem:** Bei depth=1 ist `e` die Kante von user1 → v1, aber bei depth=2 ist `e` die Kante zum aktuellen Knoten (v2), nicht die gesamte Pfadhistorie.

**Ergebnis:** Zu viele Pfade abgeschnitten, die über alternative Routen erreichbar wären.

---

## Lösung: Pfad-Constraints explizit definieren

### 1. Constraint-Typen

#### 1.1 Last-Edge Constraint (bereits implementiert)
**Semantik:** FILTER gilt nur für die **eingehende Kante** zur aktuellen Zeile (depth).

**Syntax:**
```aql
FILTER e.type == 'follows'  -- nur am letzten Level sicher
```

**Anwendung:**
- Am letzten Level vor Enqueue prüfen (✅ implementiert)
- Auf Zwischenebenen: **nicht** prunen (würde Pfade abschneiden)

---

#### 1.2 All-Edges Constraint (Pfad-weites Prädikat)
**Semantik:** FILTER gilt für **alle Kanten** entlang des Pfads von Start bis aktueller Zeile.

**Syntax (zukünftig):**
```aql
FILTER PATH.ALL(e, e.type == 'follows')
```

**Bedeutung:**
- Prüfe bei jedem Expand: Ist die neue Kante ein `follows`?
- Wenn nein: Pfad ist ungültig → nicht enqueuen
- **Sicher auf allen Tiefen!**

**Implementierung:**
- Beim Enqueue: Prüfe `a.edgeId` gegen Constraint
- Tracking: Optional Pfad-Historie (Liste der edgeIds) mitführen, falls Constraints auf "vorherige Kante" prüfen

---

#### 1.3 Any-Edge Constraint
**Semantik:** Mindestens **eine Kante** entlang des Pfads erfüllt Prädikat.

**Syntax (zukünftig):**
```aql
FILTER PATH.ANY(e, e.weight > 10)
```

**Implementierung:**
- Pfad-State: Boolean Flag `hasSeenHeavyEdge`
- Beim Enqueue: Update Flag
- Bei Result-Zeile: Prüfe Flag

---

#### 1.4 No-Vertex Constraint (Blockierte Knoten)
**Semantik:** **Kein Vertex** entlang des Pfads darf Prädikat verletzen.

**Syntax (zukünftig):**
```aql
FILTER PATH.NONE(v, v.blocked == true)
```

**Implementierung:**
- Beim Enqueue: Prüfe neuen Vertex `nb` gegen Constraint
- Wenn `nb.blocked == true`: Nicht enqueuen
- **Sicher auf allen Tiefen!**

---

### 2. Sichere Pruning-Regeln

| Constraint-Typ | Anwendungstiefe | Implementierung |
|----------------|-----------------|-----------------|
| **Last-Edge** (e.field OP value) | Nur letztes Level | ✅ Implementiert (evalSingleE) |
| **Last-Vertex** (v.field OP value) | Nur letztes Level | ✅ Implementiert (evalSingleV) |
| **PATH.ALL(e, ...)** | Alle Tiefen | 🔜 Geplant (Expand-Zeit-Check) |
| **PATH.NONE(v, ...)** | Alle Tiefen | 🔜 Geplant (Expand-Zeit-Check) |
| **PATH.ANY(e, ...)** | Alle Tiefen (State) | 🔜 Geplant (Flag-basiert) |

---

### 3. AST-Erweiterungen (Parser)

#### 3.1 Neue Expression-Typen

```cpp
struct PathConstraintExpr : Expression {
    enum class Type { All, Any, None };
    Type type;
    char varName;  // 'e' oder 'v'
    std::unique_ptr<Expression> predicate;
};
```

**Parser-Syntax:**
```aql
PATH.ALL(e, e.type == 'follows')
PATH.NONE(v, v.blocked == true)
PATH.ANY(e, e.weight > 10)
```

---

#### 3.2 AST-Classifier (Filter-Analyse)

```cpp
struct FilterClassification {
    std::vector<Expression*> lastEdgeOnly;
    std::vector<Expression*> lastVertexOnly;
    std::vector<Expression*> pathAllEdge;
    std::vector<Expression*> pathNoneVertex;
    std::vector<Expression*> pathAnyEdge;
    std::vector<Expression*> mixed;  // AND/OR kombiniert, keine einfache Klassifikation
};

FilterClassification classifyFilters(const std::vector<std::unique_ptr<FilterClause>>& filters);
```

---

### 4. BFS-Anpassungen

#### 4.1 Expand-Zeit-Checks (PATH.ALL/NONE)

```cpp
auto enqueueOut = [&](const std::vector<AdjacencyInfo>& adj) {
    for (const auto& a : adj) {
        // PATH.ALL(e, e.type == 'follows')
        for (const auto& pathAllE : pathAllEdgeConstraints) {
            if (!evalEdgeConstraint(a.edgeId, pathAllE)) {
                prunedAllDepths++;
                continue;  // sicher auf allen Tiefen!
            }
        }
        
        // PATH.NONE(v, v.blocked == true)
        for (const auto& pathNoneV : pathNoneVertexConstraints) {
            if (evalVertexConstraint(a.targetPk, pathNoneV)) {
                prunedAllDepths++;
                continue;  // blockierter Vertex → skip
            }
        }
        
        // Konservative Prüfungen (nur letztes Level)
        if (depth + 1 == t.maxDepth) {
            // ... (wie bisher)
        }
        
        if (visited.insert(a.targetPk).second) {
            parent[a.targetPk] = {node, a.edgeId};
            qnodes.push({a.targetPk, depth + 1});
            enqueuedPerDepth[depth + 1]++;
        }
    }
};
```

---

#### 4.2 State-basierte Constraints (PATH.ANY)

```cpp
struct PathState {
    bool hasSeenHeavyEdge = false;
    // weitere Flags je Constraint
};

std::unordered_map<std::string, PathState> pathStates;

// Beim Enqueue:
PathState newState = pathStates[node];
if (checkEdgeWeight(a.edgeId) > 10) newState.hasSeenHeavyEdge = true;
pathStates[a.targetPk] = newState;

// Bei Result-Zeile:
if (pathAnyEdgeConstraints.hasHeavyEdge && !pathStates[node].hasSeenHeavyEdge) {
    pass = false;  // PATH.ANY nicht erfüllt
}
```

---

### 5. Performance-Implikationen

#### Vorteile
- **Frontier-Reduktion:** Aggressives Pruning auf allen Tiefen
- **Frühzeitiger Abbruch:** Ungültige Pfade werden sofort verworfen
- **Weniger Entity-Loads:** Nur validierte Pfade landen in Result-Set

#### Kosten
- **Expand-Zeit-Overhead:** Jede Kante wird gegen PATH.ALL/NONE geprüft
- **Memory:** PathState für PATH.ANY (HashMap, kleine Keys)

**Faustregel:**
- Nutzen > Kosten, wenn Constraints selektiv sind (z. B. nur 10% der Kanten sind `follows`)

---

### 6. Implementierungs-Roadmap

1. **Phase 1:** Parser-Erweiterung (PATH.ALL/NONE/ANY Syntax)
2. **Phase 2:** AST-Classifier (Filter-Typen erkennen)
3. **Phase 3:** BFS Expand-Zeit-Checks (PATH.ALL/NONE)
4. **Phase 4:** State-Tracking (PATH.ANY)
5. **Phase 5:** Metriken (`pruned_all_depths`, `path_state_size`)
6. **Phase 6:** Tests & Benchmarks (Vergleich mit/ohne Constraints)

---

### 7. Beispiel-Queries

#### Nur follows-Kanten erlauben
```aql
FOR v IN 1..3 OUTBOUND 'user1' GRAPH 'social'
  FILTER PATH.ALL(e, e.type == 'follows')
  RETURN v
```

**Effekt:** BFS expandiert nur über `follows`-Kanten, alle anderen werden auf **allen Tiefen** gedroppt.

---

#### Keine blockierten Vertices im Pfad
```aql
FOR v IN 1..5 OUTBOUND 'user1' GRAPH 'social'
  FILTER PATH.NONE(v, v.blocked == true)
  RETURN v
```

**Effekt:** Pfade, die einen blockierten Vertex passieren, werden sofort verworfen.

---

#### Mindestens eine starke Beziehung
```aql
FOR v IN 1..4 OUTBOUND 'user1' GRAPH 'social'
  FILTER PATH.ANY(e, e.weight > 10)
  RETURN v
```

**Effekt:** Nur Pfade mit mindestens einer starken Kante (weight > 10) landen im Result.

---

## Zusammenfassung

| Aktuelle Implementierung | Pfad-Constraints (geplant) |
|--------------------------|----------------------------|
| Pruning nur am letzten Level | Pruning auf **allen Tiefen** |
| Unsicher für Zwischenebenen | Sichere Semantik durch PATH.ALL/NONE |
| Einfach (kein State) | State-Tracking für PATH.ANY |
| Konservativ (viele False Positives) | Aggressiv (nur valide Pfade expandiert) |

**Empfehlung:**
- Phase 1-3 implementieren (PATH.ALL/NONE) für sofortigen Nutzen
- Phase 4 (PATH.ANY) optional, falls Use-Cases existieren
- Metriken sammeln: `pruned_all_depths` vs. `pruned_last_level` Vergleich

---

**Siehe auch:**
- [AQL EXPLAIN & PROFILE](../aql/aql_explain_profile.md)
- [BFS Pruning (aktuell)](../development/todo.md#performance--planung)
