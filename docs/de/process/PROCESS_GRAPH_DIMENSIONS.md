[docs](../../index.md) > [de](../index.md) > [process](./index.md) > [PROCESS_GRAPH_DIMENSIONS](./PROCESS_GRAPH_DIMENSIONS.md)  
**Datum:** 2026-03-19  
**Status:** draft  
**Modul:** `src/process/`  
**Version:** 1.0.0  

---

# Mehrdimensionale Prozessmodelle in ThemisDB

**(dims / view / YAML / BPMN 2.0 / LLM-Retrieval)**

---

## 📑 Inhaltsverzeichnis

- [1. Ziel](#1-ziel)
- [2. Trennung von Darstellung und Funktion](#2-trennung-von-darstellung-und-funktion)
- [3. YAML-Schema und Beispiel](#3-yaml-schema-und-beispiel)
- [4. Dimension Registry](#4-dimension-registry)
- [5. BPMN-2.0-Mapping](#5-bpmn-20-mapping)
- [6. LLM-Retrieval](#6-llm-retrieval)
- [7. Zusammenfassung](#7-zusammenfassung)

---

## 1. Ziel

Dieses Dokument beschreibt, wie Prozessmodelle in ThemisDB als **mehrdimensionale Graphen** gespeichert werden, wie die **2D-Darstellung** davon abgeleitet wird, wie das Modell als **YAML-Austauschformat** genutzt werden kann, und wie es für **LLM-gestützte semantische Suche** (Vector + Graph) verwendet wird.

Kernziel ist die **strikte Trennung** zwischen:

- **`dims`** – fachliche, semantisch bedeutsame Dimensionen (z. B. Org-Zuordnung, Zeitreihenfolge, Risiko)
- **`view`** – reine Darstellungskonfiguration (2D-Projektion, Farbgebung, Gruppierung)

Diese Trennung stellt sicher, dass der Graph **nicht hart kodiert** ist, sondern **datengetrieben** bleibt, während die Darstellungs- und RAG-Logik im Code verbleibt.

---

## 2. Trennung von Darstellung und Funktion

### 2.1 Funktionale Dimensionen (`dims`)

`dims` beschreibt die **fachliche Bedeutung** eines Prozessschritts oder einer Kante. Die Zuordnung ist **semantisch** und **unabhängig von der Darstellung**.

**Typische `dims`-Werte:**

| Dimension | Bedeutung | Beispielwert |
|-----------|-----------|--------------|
| `org.hierarchy` | Hierarchiestufe der zuständigen Stelle | `3` |
| `org.unit_id` | Behördenkennzeichen | `BRB-MIK-REF-24` |
| `org.unit_name` | Klarname der Stelle | `Referat 24` |
| `time.sequence` | Position in der zeitlichen Reihenfolge | `2` |
| `risk.level` | Risikobewertung des Schritts | `medium` |
| `geo.region` | Geografische Zuordnung | `Potsdam` |

`dims`-Werte können an **Activities** (Prozessschritten) und **Edges** (Kanten) angebracht werden.

### 2.2 Darstellungskonfiguration (`view`)

`view` legt fest, **welche Dimensionen** für die 2D-Projektion genutzt werden. Es enthält keine fachliche Information, sondern steuert ausschließlich die visuelle Ausgabe.

**`view`-Felder:**

| Feld | Bedeutung | Standardwert |
|------|-----------|--------------|
| `x_axis` | Horizontale Achse | `org.hierarchy` |
| `y_axis` | Vertikale Achse | `time.sequence` |
| `color_by` | Farbgebung nach Dimension | – |
| `size_by` | Größenskalierung nach Dimension | – |
| `shape_by` | Formwahl nach Dimension | – |
| `group_by` | Gruppierung nach Dimension | – |

---

## 3. YAML-Schema und Beispiel

### 3.1 JSON-Schema (Übersicht)

Das folgende JSON-Schema beschreibt die YAML-Struktur für ThemisDB-Prozessmodelle inklusive `dims` und `view`:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "ThemisDB Process Model (VCC-VPB YAML)",
  "type": "object",
  "required": ["id", "name", "version", "activities", "edges"],
  "properties": {
    "id":          { "type": "string" },
    "name":        { "type": "string" },
    "version":     { "type": "string" },
    "domain":      { "type": "string" },
    "owner":       { "type": "string" },
    "description": { "type": "string" },
    "compliance": {
      "type": "array",
      "items": { "type": "string" }
    },
    "view": {
      "type": "object",
      "properties": {
        "x_axis":   { "type": "string" },
        "y_axis":   { "type": "string" },
        "color_by": { "type": "string" },
        "size_by":  { "type": "string" },
        "shape_by": { "type": "string" },
        "group_by": { "type": "string" }
      },
      "additionalProperties": false
    },
    "activities": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["id", "name", "type"],
        "properties": {
          "id":               { "type": "string" },
          "name":             { "type": "string" },
          "type":             { "type": "string" },
          "description":      { "type": "string" },
          "responsible_role": { "type": "string" },
          "sla_hours":        { "type": ["number", "string"] },
          "dims": {
            "type": "object",
            "additionalProperties": true
          }
        },
        "additionalProperties": true
      }
    },
    "edges": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["from", "to"],
        "properties": {
          "id":        { "type": "string" },
          "from":      { "type": "string" },
          "to":        { "type": "string" },
          "type":      { "type": "string" },
          "condition": { "type": "string" },
          "dims": {
            "type": "object",
            "additionalProperties": true
          }
        },
        "additionalProperties": true
      }
    }
  },
  "additionalProperties": true
}
```

### 3.2 Vollständiges YAML-Beispiel

Das folgende Beispiel zeigt ein reales Prozessmodell für das Bauantragsverfahren:

```yaml
id: bauantrag_standard
name: Bauantragsverfahren Standard
version: 1.0.0
domain: verwaltung
owner: LBV
description: Standardprozess für Bauanträge
compliance:
  - "§34 BauO"
  - "DSGVO Art. 5"

# Darstellung (2D-Projektion)
view:
  x_axis: org.hierarchy
  y_axis: time.sequence
  color_by: risk.level
  group_by: org.unit_id

# Prozessknoten (n-dimensional)
activities:
  - id: start
    name: Antrag eingegangen
    type: start
    dims:
      org.hierarchy: 1
      org.unit_id: BRB-MIK
      time.sequence: 0

  - id: pruefung
    name: Antrag prüfen
    type: user_task
    responsible_role: Sachbearbeitung
    dims:
      org.hierarchy: 3
      org.unit_id: BRB-MIK-REF-24
      org.unit_name: Referat 24
      time.sequence: 2
      risk.level: medium
      geo.region: Potsdam

  - id: entscheidung
    name: Entscheidung treffen
    type: gateway
    dims:
      org.hierarchy: 2
      org.unit_id: BRB-MIK-REF-24
      time.sequence: 3

# Kanten (n-dimensional)
edges:
  - id: e1
    from: start
    to: pruefung
    type: sequence
    dims:
      time.sequence: 1

  - id: e2
    from: pruefung
    to: entscheidung
    type: sequence
    dims:
      time.sequence: 3
```

**Wichtig:** `dims` enthält die semantischen Daten; `view` steuert ausschließlich die Darstellung. RAG-Logik verbleibt im Code – nicht im YAML.

---

## 4. Dimension Registry

Die Dimension Registry definiert alle bekannten Dimensionen mit ihren Typen, Rollen und Standard-Achsenzuordnungen.

### 4.1 Registry-Definition

```json
{
  "dimension_registry": {
    "org.hierarchy": {
      "type": "ordinal",
      "role": "both",
      "default_axis": "x",
      "description": "Hierarchiestufe der zuständigen Stelle (1 = oberste Ebene)"
    },
    "time.sequence": {
      "type": "ordinal",
      "role": "both",
      "default_axis": "y",
      "description": "Zeitliche Reihenfolge des Prozessschritts (0 = Beginn)"
    },
    "org.unit_id": {
      "type": "category",
      "role": "semantic",
      "default_axis": null,
      "description": "Eindeutiger Behördenkennzeichner (z. B. BRB-MIK-REF-24)"
    },
    "org.unit_name": {
      "type": "category",
      "role": "semantic",
      "default_axis": null,
      "description": "Klartextname der zuständigen Stelle"
    },
    "risk.level": {
      "type": "category",
      "role": "renderable",
      "default_axis": null,
      "description": "Risikobewertung: low | medium | high | critical"
    },
    "geo.region": {
      "type": "category",
      "role": "renderable",
      "default_axis": null,
      "description": "Geografische Zuordnung des Prozessschritts"
    }
  }
}
```

### 4.2 Rollen-Übersicht

| Rolle | Bedeutung |
|-------|-----------|
| `both` | Geeignet für semantische Abfragen **und** Darstellung (Achsen, Farben) |
| `semantic` | Nur für semantische Abfragen und RAG-Retrieval |
| `renderable` | Kann für Darstellung genutzt werden (Farbe, Größe, Form) |

### 4.3 Standard-Defaults (fix)

- **`x_axis = org.hierarchy`** – horizontale Achse zeigt die Organisationshierarchie
- **`y_axis = time.sequence`** – vertikale Achse zeigt die zeitliche Reihenfolge

Diese Defaults gelten, wenn in `view` keine explizite Achsenkonfiguration angegeben wird.

---

## 5. BPMN-2.0-Mapping

### 5.1 Standard-Abbildung (ohne Extensions)

Die Kern-Prozessstruktur ist direkt auf BPMN 2.0 abbildbar:

| ThemisDB YAML | BPMN 2.0 Element |
|---------------|-----------------|
| `activity.type: start` | `bpmn:startEvent` |
| `activity.type: user_task` | `bpmn:userTask` |
| `activity.type: service_task` | `bpmn:serviceTask` |
| `activity.type: gateway` | `bpmn:exclusiveGateway` / `bpmn:parallelGateway` |
| `edge.type: sequence` | `bpmn:sequenceFlow` |
| `edge.condition` | `bpmn:conditionExpression` |
| `activity.id` / `activity.name` | `id` / `name` Attribute |
| `activity.description` | `bpmn:documentation` |

### 5.2 Extension Elements für `dims` und `view`

BPMN 2.0 unterstützt keine n-dimensionalen Metadaten direkt. ThemisDB nutzt **Extension Elements** gemäß BPMN-2.0-Spezifikation (Abschnitt 8.2.3), um `dims` und `view` zu transportieren.

**Namespace-Deklaration:**

```xml
xmlns:themis="http://themisdb.io/bpmn/extensions/1.0"
```

**`view` am Prozess-Level:**

```xml
<bpmn:process id="bauantrag_standard" name="Bauantragsverfahren Standard">
  <bpmn:extensionElements>
    <themis:view
      x_axis="org.hierarchy"
      y_axis="time.sequence"
      color_by="risk.level"
      group_by="org.unit_id"/>
  </bpmn:extensionElements>
  <!-- ... -->
</bpmn:process>
```

**`dims` an einem Activity-Knoten:**

```xml
<bpmn:userTask id="pruefung" name="Antrag prüfen">
  <bpmn:extensionElements>
    <themis:dims
      org.hierarchy="3"
      org.unit_id="BRB-MIK-REF-24"
      org.unit_name="Referat 24"
      time.sequence="2"
      risk.level="medium"
      geo.region="Potsdam"/>
  </bpmn:extensionElements>
</bpmn:userTask>
```

**`dims` an einer Kante:**

```xml
<bpmn:sequenceFlow id="e1" sourceRef="start" targetRef="pruefung">
  <bpmn:extensionElements>
    <themis:dims time.sequence="1"/>
  </bpmn:extensionElements>
</bpmn:sequenceFlow>
```

### 5.3 Vollständiges BPMN-2.0-Beispiel

```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions
  xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
  xmlns:themis="http://themisdb.io/bpmn/extensions/1.0"
  targetNamespace="http://themisdb.io/process"
  id="bauantrag_standard_definitions">

  <bpmn:process id="bauantrag_standard" name="Bauantragsverfahren Standard"
                isExecutable="false">
    <bpmn:extensionElements>
      <themis:view
        x_axis="org.hierarchy"
        y_axis="time.sequence"
        color_by="risk.level"
        group_by="org.unit_id"/>
    </bpmn:extensionElements>

    <bpmn:startEvent id="start" name="Antrag eingegangen">
      <bpmn:extensionElements>
        <themis:dims
          org.hierarchy="1"
          org.unit_id="BRB-MIK"
          time.sequence="0"/>
      </bpmn:extensionElements>
      <bpmn:outgoing>e1</bpmn:outgoing>
    </bpmn:startEvent>

    <bpmn:userTask id="pruefung" name="Antrag prüfen">
      <bpmn:extensionElements>
        <themis:dims
          org.hierarchy="3"
          org.unit_id="BRB-MIK-REF-24"
          org.unit_name="Referat 24"
          time.sequence="2"
          risk.level="medium"
          geo.region="Potsdam"/>
      </bpmn:extensionElements>
      <bpmn:incoming>e1</bpmn:incoming>
      <bpmn:outgoing>e2</bpmn:outgoing>
    </bpmn:userTask>

    <bpmn:exclusiveGateway id="entscheidung" name="Entscheidung treffen">
      <bpmn:extensionElements>
        <themis:dims
          org.hierarchy="2"
          org.unit_id="BRB-MIK-REF-24"
          time.sequence="3"/>
      </bpmn:extensionElements>
      <bpmn:incoming>e2</bpmn:incoming>
    </bpmn:exclusiveGateway>

    <bpmn:sequenceFlow id="e1" sourceRef="start" targetRef="pruefung">
      <bpmn:extensionElements>
        <themis:dims time.sequence="1"/>
      </bpmn:extensionElements>
    </bpmn:sequenceFlow>

    <bpmn:sequenceFlow id="e2" sourceRef="pruefung" targetRef="entscheidung">
      <bpmn:extensionElements>
        <themis:dims time.sequence="3"/>
      </bpmn:extensionElements>
    </bpmn:sequenceFlow>

  </bpmn:process>
</bpmn:definitions>
```

### 5.4 Kompatibilitätshinweis

BPMN-2.0-Tools, die den `themis:`-Namespace nicht kennen, ignorieren die Extension Elements gemäß BPMN-Spezifikation. Die Prozessstruktur (Tasks, Gateways, Flows) bleibt vollständig kompatibel.

---

## 6. LLM-Retrieval

### 6.1 Prozess-Embedding

Der gesamte Prozess erhält ein **Prozess-Embedding**, das die semantische Ähnlichkeit zwischen Prozessmodellen repräsentiert. Das Embedding wird aus den Meta-Daten (`name`, `description`, `domain`, `compliance`) sowie den aggregierten `dims`-Werten aller Knoten generiert.

```
ProcessRecord._embedding = embed(
    process.name + " " + process.description +
    " domain:" + process.domain +
    " dims_summary:" + aggregate(all_node_dims)
)
```

### 6.2 Node-Embedding

Jeder Prozessschritt (Activity) kann ein eigenes **Node-Embedding** erhalten. Dies ermöglicht Retrieval auf Schritt-Ebene, z. B. „Finde alle Prüfschritte mit Risikostufe medium".

```
NodeRecord._embedding = embed(
    node.name + " " + node.description +
    " role:" + node.responsible_role +
    " dims:" + serialize(node.dims)
)
```

Node-Embeddings werden idealerweise als eigene Base-Entities gespeichert und sind direkt graph-traversierbar (Vorgänger, Nachfolger, Bedingungen).

### 6.3 Hybrid Retrieval (Vector + Graph)

Das ThemisDB-Retrieval kombiniert **Vektor-Suche** und **Graph-Traversal** in einer mehrstufigen Pipeline:

| Stufe | Methode | Ergebnis |
|-------|---------|---------|
| 1 | **Vektor-Suche** über Prozess-Embeddings | Relevante Prozessmodelle |
| 2 | **Vektor-Suche** über Node-Embeddings | Relevante Einzelschritte |
| 3 | **Graph-Expansion** | Kontext: Vorgänger, Nachfolger, Bedingungen |
| 4 | **`dims`-Filter** | Einschränkung auf z. B. `org.hierarchy=3` oder `risk.level=medium` |
| 5 | **LLM-Kontext-Aufbau** | Strukturierter Kontext aus Schritten + Kanten + Metadaten |

**Beispiel-Retrieval-Anfrage:**

```
"Zeige alle Prüfschritte des Bauantragsverfahrens auf Hierarchieebene 3"
→ Vektor-Suche: pruefung-ähnliche Nodes
→ dims-Filter: org.hierarchy = 3
→ Graph-Expansion: Vorgänger (start), Nachfolger (entscheidung)
→ LLM-Kontext: strukturierte Liste mit Org-Einheit, Risikolevel, Zeitposition
```

### 6.4 `dims`-basierte Filterung

`dims`-Werte können als strukturierte Filter in Retrieval-Abfragen verwendet werden:

```json
{
  "query": "Prüfschritte mit mittlerem Risiko",
  "filters": {
    "dims.risk.level": "medium",
    "dims.org.hierarchy": { "$lte": 3 }
  }
}
```

Dies ermöglicht präzise, behördenbezogene Antworten statt generischer Prozessübersichten.

---

## 7. Zusammenfassung

Das ThemisDB-Prozessschema ermöglicht:

| Fähigkeit | Beschreibung |
|-----------|-------------|
| **Mehrdimensionale Prozessdaten** | Jeder Schritt/jede Kante trägt beliebig viele semantische Dimensionen (`dims`) |
| **Flexible 2D-Darstellung** | `view` steuert die Projektion unabhängig von den Funktionsdaten |
| **YAML als Austauschformat** | Vollständig validierbar über JSON-Schema; direkt von `VccVpbImporter` importierbar |
| **BPMN 2.0 kompatibel** | Standard-Prozessstruktur + ThemisDB Extension Elements für `dims`/`view` |
| **LLM-fähige Retrieval-Pipelines** | Prozess- und Node-Embeddings + Hybrid Vector+Graph-Suche + `dims`-Filter |

**Fixe Standard-Achsen:**

- `x_axis = org.hierarchy`
- `y_axis = time.sequence`

Diese Standards gelten, wenn in `view` keine explizite Konfiguration vorliegt.

---

*Dokument erstellt: 2026-03-19 · Modul: `src/process/` · Status: draft*
