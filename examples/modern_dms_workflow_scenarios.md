> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Moderne DMS-Workflow-Szenarien

**Kategorie:** 📖 Beispiele  
**Version:** v1.0.0  
**Status:** 📋 Referenz  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Szenario 1: Baugenehmigung mit parallelen Gutachten](#-szenario-1-baugenehmigung-mit-parallelen-gutachten)
- [Szenario 2: Conditional Routing via KI-Klassifizierung](#-szenario-2-conditional-routing-via-ki-klassifizierung)
- [Szenario 3: Kollaborativer Multi-User-Prozess](#-szenario-3-kollaborativer-multi-user-prozess)
- [Szenario 4: Batch-Verarbeitung mit intelligenter Priorisierung](#-szenario-4-batch-verarbeitung-mit-intelligenter-priorisierung)
- [Szenario 5: Prozessoptimierung durch historische Analyse](#-szenario-5-prozessoptimierung-durch-historische-analyse)

---

## 🎯 Übersicht

Diese Szenarien illustrieren, wie das Themis.DocumentManager DMS mit ThemisDB's Multi-Model-Fähigkeiten komplexe, reale Verwaltungsprozesse abbildet. Jedes Szenario enthält Graph-Visualisierungen, AQL-Queries und Benutzerinteraktions-Beschreibungen.

---

## 🏗️ Szenario 1: Baugenehmigung mit parallelen Gutachten

### Kontext

Familie Müller beantragt eine Baugenehmigung für ein Einfamilienhaus in Stuttgart. Das Grundstück liegt nahe einem Landschaftsschutzgebiet und an einer stark befahrenen Straße. Deshalb müssen **parallel** drei Fachgutachten eingeholt werden, bevor eine Entscheidung getroffen werden kann.

### Prozess-Graph

```
                    ┌─────────────────────────────────────────────┐
                    │           BAUGENEHMIGUNG EFH                │
                    │             FALL-2026-1247                  │
                    └─────────────────────────────────────────────┘

[START]
   ↓
[ANTRAG EINGEREICHT]  ─────── 01.02.2026
   ↓
[VOLLSTÄNDIGKEITSPRÜFUNG] ─── 03.02.2026  (SB: Schmidt)
   │
   ├─────────────────────────────────────────────┐
   │                                             │
   ↓                    ↓                        ↓
[STATIKPRÜFUNG]    [UMWELTPRÜFUNG]         [LÄRMSCHUTZ-
(intern)           (extern: TÜV)            GUTACHTEN]
SB: Müller         Gutachter: GTR-22        (extern: Akustik GmbH)
Frist: 15.02       Frist: 28.02             Frist: 28.02
Status: ✅ FERTIG   Status: ⏸ WARTEND        Status: 🔵 IN BEARBEITUNG
   │                    │                        │
   └────────────────────┴────────────────────────┘
                        ↓
              [AND-GATEWAY: Alle 3 Gutachten fertig?]
                        ↓ (wenn ja)
              [GESAMTPRÜFUNG]  ─── SB: Supervisor Braun
                        ↓
              [ENTSCHEIDUNG]   ─── KI-Score: 82% Genehmigung
                    ↙               ↘
          [GENEHMIGT]           [ABGELEHNT]
              ↓                      ↓
          [BESCHEID               [BESCHEID
          ERSTELLEN]              ERSTELLEN]
              ↓                      ↓
          [ARCHIV]               [ARCHIV]
```

### Aktueller Zustand (15.02.2026)

```
Status: IN_BEARBEITUNG
Aktive Knoten: [UMWELTPRÜFUNG, LÄRMSCHUTZGUTACHTEN]
Abgeschlossene Knoten: [ANTRAG_EINGEREICHT, VOLLSTÄNDIGKEITSPRÜFUNG, STATIKPRÜFUNG]
Blockade: AND-Gateway wartet auf 2 von 3 Gutachten

SLA-Status:
  Gesamt-Deadline: 15.03.2026 (28 Tage verbleibend)
  Kritischer Pfad: LÄRMSCHUTZGUTACHTEN (Frist: 28.02 → noch 13 Tage)
  SLA-Risiko: MITTEL (Akustik GmbH hat in 2 ähnlichen Fällen Frist überschritten)
```

### ThemisDB AQL – Prozessstatus abfragen

```aql
-- Aktuellen Zustand des Prozesses inklusive Graph-Status
FOR p IN process
  FILTER p.id == 'FALL-2026-1247'
  LET active_nodes = (
    FOR n IN process_node
      FILTER n.process_id == p.id
        AND n.status IN ['AKTIV', 'IN_BEARBEITUNG', 'WARTEND']
      RETURN {
        id:           n.id,
        title:        n.title,
        status:       n.status,
        assignee:     n.assignee_id,
        deadline:     n.deadline,
        days_left:    DATE_DIFF(n.deadline, NOW(), 'days')
      }
  )
  LET blocking_path = (
    GRAPH TRAVERSE
      FROM (SELECT id FROM process_node WHERE process_id == p.id AND node_type == 'GATEWAY_AND')
      EDGES process_edge DIRECTION INBOUND
      MAX_DEPTH 3
    FILTER vertex.status != 'ABGESCHLOSSEN'
    RETURN vertex.title
  )
  LET ai_pred = FIRST(
    FOR pred IN ki_prediction
      FILTER pred.process_id == p.id
        AND pred.prediction_type == 'APPROVAL_PROBABILITY'
      SORT pred.predicted_at DESC
      LIMIT 1
      RETURN pred
  )
  RETURN {
    reference_number:  p.reference_number,
    status:            p.status,
    active_nodes:      active_nodes,
    blocking_reasons:  blocking_path,
    ai_approval_score: ai_pred.score,
    sla_deadline:      p.deadline,
    days_to_deadline:  DATE_DIFF(p.deadline, NOW(), 'days')
  }
```

### Benutzer-Interaktion: SLA-Eskalation

Der Supervisor sieht, dass das Lärmschutzgutachten noch aussteht und die Akustik GmbH historisch Fristen überschreitet:

```
Supervisor-Ansicht (15.02.2026):

🔴 FALL-2026-1247 – Risiko: Lärmschutzgutachten
────────────────────────────────────────────────────
Gutachter: Akustik GmbH (extern)
Vergeben am: 05.02 | Frist: 28.02 | Noch: 13 Tage

🤖 KI-Warnung:
"Akustik GmbH hat in 3/5 ähnlichen Fällen die Frist um Ø 8 Tage
 überschritten. Empfehlung: Jetzt Erinnerung senden."

Optionen:
[📧 Erinnerung senden]  [📞 Direktkontakt]
[🔄 Alternativen Gutachter suchen]  [Ignorieren]
```

---

## 🤖 Szenario 2: Conditional Routing via KI-Klassifizierung

### Kontext

Das Bürgeramt empfängt täglich ~200 Eingaben (E-Mail, Portal, Fax). Statt manueller Kategorisierung übernimmt das KI-System die Klassifizierung und leitet automatisch zum richtigen Prozess-Template weiter.

### Eingang-zu-Prozess Pipeline

```
E-Mail/Portal-Eingang
         ↓
[KI-KLASSIFIZIERUNG]
Dokument-Text → LLM
         │
         ├─ BAUGENEHMIGUNG (Konfidenz: 94%) ──────→ [BAUGENEHMIGUNG-TEMPLATE]
         │                                              │
         │                                         [VOLLSTÄNDIGKEITSPRÜFUNG]
         │                                              │
         │                                         [GEO-CHECK: Zuständigkeit?]
         │                                              ↓
         │                                         [ROUTING zu Amt A/B/C]
         │
         ├─ BESCHWERDE (Konfidenz: 89%) ──────────→ [BESCHWERDE-TEMPLATE]
         │                                              │
         │                                         [EINGANGBESTÄTIGUNG 24h SLA]
         │                                              │
         │                                         [SACHBEARBEITER ZUWEISEN]
         │                                         (KI: basierend auf Fachgebiet)
         │
         ├─ ANFRAGE_INFORMATION (Konfidenz: 97%) →  [ANFRAGE-TEMPLATE]
         │                                              │
         │                                         [VOLLAUTOMATISCHE ANTWORT?]
         │                                              │
         │                                   ┌──────────────────────────┐
         │                              Ja (KI-Score > 0.9)          Nein
         │                                   ↓                         ↓
         │                            [AUTO-ANTWORT]           [SACHBEARBEITER]
         │                            aus Wissensbasis
         │
         └─ UNBEKANNT (Konfidenz: 45%) ──────────→ [MANUELLE PRÜFUNG]
                                                   SB: Poststelle
```

### AQL: Routing-Entscheidung

```aql
-- Routing-Entscheidung für einen neuen Eingang basierend auf KI-Klassifizierung
LET doc_id = @new_document_id
LET doc = FIRST(FOR d IN document_attachment FILTER d.id == doc_id RETURN d)

-- Schritt 1: KI-Klassifizierung (extern aufgerufen, Ergebnis in Metadaten)
LET classification = doc.metadata.ai_classification

-- Schritt 2: Geographische Zuständigkeit ermitteln
LET responsible_authority = FIRST(
    FOR a IN authority
      JOIN d IN district ON d.id = a.district_id
      FILTER CONTAINS_GEO(d.boundary_polygon, doc.metadata.geo_location)
      RETURN a
)

-- Schritt 3: Passendes Template und Sachbearbeiter finden
LET template = FIRST(
    FOR t IN process_template
      FILTER t.process_type == classification.category
        AND t.authority_id == responsible_authority.id
        AND t.is_active == TRUE
      RETURN t
)

-- Schritt 4: Optimalen Sachbearbeiter vorschlagen
LET suggested_assignee = FIRST(
    FOR u IN user_account
      FILTER u.authority_id == responsible_authority.id
        AND classification.category IN u.competencies
      LET workload = LENGTH(
        FOR n IN process_node
          FILTER n.assignee_id == u.id
            AND n.status IN ['AKTIV', 'IN_BEARBEITUNG']
          RETURN n
      )
      SORT workload ASC
      LIMIT 1
      RETURN { id: u.id, name: u.name, workload: workload }
)

RETURN {
    routing_decision:      classification.category,
    confidence:            classification.confidence,
    template_id:           template.id,
    authority:             responsible_authority.name,
    suggested_assignee:    suggested_assignee,
    auto_processable:      classification.confidence > 0.90
                           AND classification.category == 'ANFRAGE_INFORMATION'
}
```

### Ergebnis-Beispiel

```json
{
  "routing_decision": "BAUGENEHMIGUNG_NEUBAU",
  "confidence": 0.94,
  "template_id": "tmpl-baug-neubau-v3",
  "authority": "Baurechtsamt Stuttgart-Mitte",
  "suggested_assignee": {
    "id": "user-hm-001",
    "name": "H. Müller",
    "workload": 4
  },
  "auto_processable": false,
  "routing_explanation": "Erkannte Schlüsselbegriffe: 'Neubau', 'Bauantrag', 
                           'Musterstraße 12'. Geo-Prüfung: Stuttgart-Mitte (Zuständigkeit: Baurechtsamt)."
}
```

---

## 👥 Szenario 3: Kollaborativer Multi-User-Prozess

### Kontext

Ein komplexer Gewerbeantrag (Restaurant mit Außenbereich) wird von drei Sachbearbeitern aus verschiedenen Ämtern gemeinsam bearbeitet: Baurechtsamt, Gesundheitsamt und Ordnungsamt.

### Prozess-Graph (Cross-Department)

```
[ANTRAG EINGEREICHT]
        ↓
[KOORDINATIONS-MEETING]  ← Supervisor legt alle 3 SBs fest
        │
        ├─────────────────────────────────────────────┐
        │                                             │
   Baurechtsamt                              Gesundheitsamt
   [BAULICHE PRÜFUNG]                        [HYGIENEPRÜFUNG]
   SB: A. Weber                              SB: Dr. K. Bauer
   Frist: 21.02                              Frist: 28.02
   Status: ✅ FERTIG                          Status: 🔵 AKTIV
        │                                             │
        └─────────────────┬───────────────────────────┘
                          │
                   Ordnungsamt
                [LÄRMSCHUTZ & ORDNUNG]
                SB: T. Schneider
                Frist: 21.02
                Status: ⚠️ VERZÖGERT (5 Tage überfällig)
                          │
             (XOR-GATEWAY: alle Ämter positiv?)
             ↙                               ↘
     [GESAMTGENEHMIGUNG]              [AUFLAGEN-BESCHEID]
     (keine Einwände)                 (Auflagen von Ordnungsamt)
             ↓                               ↓
         [BESCHEID]                      [BESCHEID]
```

### Kollaborations-Session

```
Cross-Department-Kommentar-Thread (im Prozess FALL-2026-0892):

[14:23] A. Weber (Baurechtsamt):
"Bauliche Prüfung abgeschlossen. Keine Einwände. 
 @T.Schneider – der Außenbereich überschreitet die erlaubte Fläche 
 lt. Bebauungsplan um 12%. Das könnte eure Lärmschutzbewertung beeinflussen."

[14:47] T. Schneider (Ordnungsamt):
"@A.Weber Danke für den Hinweis. Wir brauchen ein aktualisiertes 
 Lageplan-Dokument mit korrekter Außenfläche. 
 @Dr.K.Bauer – betrifft auch euren Hygienebereich?"

[15:12] Dr. K. Bauer (Gesundheitsamt):
"Ja, Außenbereich-Größe relevant für Hygiene-Kapazitätsberechnung. 
 @Antragsteller bitte neuen Lageplan einreichen."

→ System: @Antragsteller-Benachrichtigung ausgelöst
→ System: Automatischer Prozessknoten "NACHFORDERUNG_LAGEPLAN" 
          in allen 3 parallel laufenden Teilprozessen erstellt
→ System: Alle 3 Sachbearbeiter erhalten Update wenn neuer Lageplan eingereicht

[16:35] Portal-Benachrichtigung an Antragsteller:
"Zu Ihrem Antrag FALL-2026-0892 wurde eine Nachforderung gestellt:
 Aktualisierter Lageplan mit korrekten Außenbereich-Maßen erforderlich.
 Frist: 28.02.2026"
```

### AQL: Änderungs-Propagation

```aql
-- Wenn Nachforderung in einem Teilprozess ausgelöst wird,
-- alle abhängigen Teilprozesse informieren

FOR edge IN process_edge
  FILTER edge.source_node_id == @triggering_node_id
    AND edge.edge_type == 'DEPENDENCY'
LET dependent_node = FIRST(FOR n IN process_node WHERE n.id == edge.target_node_id RETURN n)
LET dependent_process = FIRST(FOR p IN process WHERE p.id == dependent_node.process_id RETURN p)
INSERT INTO collaboration_comment {
    process_id:   dependent_process.id,
    author_id:    'SYSTEM',
    content:      CONCAT('Automatische Benachrichtigung: Abhängiger Prozess ',
                  @triggering_process_reference,
                  ' hat Nachforderung ausgelöst. Ihre Bearbeitung kann betroffen sein.'),
    mentioned_user_ids: [dependent_node.assignee_id]
}
RETURN { notified_process: dependent_process.reference_number,
         notified_user:    dependent_node.assignee_id }
```

---

## ⚡ Szenario 4: Batch-Verarbeitung mit intelligenter Priorisierung

### Kontext

Ende des Quartals: Supervisor möchte 180 offene Anträge priorisieren und effizient an das Team verteilen. Das KI-System unterstützt bei der optimalen Zuweisung.

### Intelligente Priorisierungs-Matrix

```
KI-Priorisierungs-Algorithmus:

Priority Score = 
    0.35 × (1 - days_to_deadline / total_sla_days)   # Frist-Druck
  + 0.25 × ai_approval_probability                    # Aufwand vs. Nutzen
  + 0.20 × applicant_waiting_days / 30               # Bürger-Wartezeit
  + 0.15 × strategic_importance_score                 # Politisch/wirtschaftlich
  + 0.05 × case_complexity_inverse                    # Einfache Fälle bevorzugen

Ergebnis-Gruppen:
  🔴 SOFORT (Score ≥ 0.85):  23 Fälle  → Heute bearbeiten
  🟡 BALD   (Score 0.6–0.85): 67 Fälle  → Diese Woche
  🔵 NORMAL (Score < 0.60):   90 Fälle  → Planmäßig
```

### Batch-AQL: Priorisierter Assignments-Plan

```aql
-- Generiert optimalen Bearbeitungsplan für 180 offene Fälle
LET open_cases = (
    FOR p IN process
      FILTER p.status IN ['EINGEREICHT', 'IN_BEARBEITUNG']
        AND p.authority_id == @authority_id
      LET days_to_deadline = DATE_DIFF(p.deadline, NOW(), 'days')
      LET total_sla_days = FIRST(
          FOR s IN sla_definition WHERE s.id == p.sla_definition_id
          RETURN s.total_duration_days
      )
      LET ai_pred = FIRST(
          FOR pred IN ki_prediction
            FILTER pred.process_id == p.id
              AND pred.prediction_type == 'APPROVAL_PROBABILITY'
            SORT pred.predicted_at DESC LIMIT 1
            RETURN pred.score
      )
      LET waiting_days = DATE_DIFF(p.submitted_at, NOW(), 'days')
      LET complexity = LENGTH(
          FOR n IN process_node WHERE n.process_id == p.id RETURN n
      )
      LET priority_score = (
          0.35 * (1 - days_to_deadline / total_sla_days)
        + 0.25 * (ai_pred ?? 0.5)
        + 0.20 * MIN(waiting_days / 30, 1.0)
        + 0.05 * (1 / complexity)
      )
      SORT priority_score DESC
      RETURN {
          process_id:       p.id,
          reference:        p.reference_number,
          process_type:     p.process_type,
          priority_score:   priority_score,
          days_to_deadline: days_to_deadline,
          ai_score:         ai_pred,
          waiting_days:     waiting_days,
          complexity:       complexity
      }
)

-- Verteile auf Sachbearbeiter (Round-Robin mit Kapazitätsprüfung)
LET available_processors = (
    FOR u IN user_account
      FILTER u.authority_id == @authority_id
        AND u.is_available == TRUE
      LET current_load = LENGTH(
          FOR n IN process_node
            FILTER n.assignee_id == u.id
              AND n.status IN ['AKTIV', 'IN_BEARBEITUNG']
            RETURN n
      )
      FILTER current_load < u.max_concurrent_cases
      RETURN { id: u.id, name: u.name, load: current_load,
               capacity: u.max_concurrent_cases - current_load }
)

-- Assignments generieren
FOR i IN 0..LENGTH(open_cases)-1
  LET case = open_cases[i]
  LET processor = available_processors[i MOD LENGTH(available_processors)]
  RETURN {
      case_reference: case.reference,
      assign_to:      processor.name,
      priority:       CASE
                        WHEN case.priority_score >= 0.85 THEN 'SOFORT'
                        WHEN case.priority_score >= 0.60 THEN 'BALD'
                        ELSE 'NORMAL'
                      END,
      priority_score: case.priority_score
  }
```

### Ergebnis-Übersicht

```
Batch-Assignment-Plan – 12.02.2026

Sachbearbeiter  │ Neue Zuweis. │ Sofort │ Bald │ Normal │ Neue Last
────────────────┼──────────────┼────────┼──────┼────────┼──────────
H. Müller       │     35       │   8    │  14  │   13   │  35/40
K. Schmidt      │     32       │   7    │  12  │   13   │  32/40
T. Schneider    │     38       │   8    │  16  │   14   │  38/40
A. Weber        │     30       │   0    │  12  │   18   │  30/35 (Teilzeit)
Dr. K. Bauer    │     45       │  ─     │  13  │   32   │  45/50 (Vollzeit+KV)
────────────────┼──────────────┼────────┼──────┼────────┼──────────
Gesamt          │    180       │  23    │  67  │   90   │  

KI-Warnung: Dr. K. Bauer's neue Last (45) nahe Kapazitätsgrenze (50).
Empfehlung: 5 "Normal"-Fälle auf H. Müller verschieben.
[Vorschlag übernehmen] [Manuell anpassen] [Ignorieren]
```

---

## 📊 Szenario 5: Prozessoptimierung durch historische Analyse (Vector-Search)

### Kontext

Das Baurechtsamt bemerkt, dass Baugenehmigungsverfahren im Durchschnitt 52 Tage dauern, obwohl die gesetzliche Frist 60 Tage beträgt. Der Analyst möchte herausfinden, welche Faktoren die schnellsten Verfahren (< 30 Tage) ermöglicht haben, um Prozesse zu optimieren.

### Analyse-Workflow

```
Analyst-Anfrage:
"Welche Faktoren unterscheiden die schnellsten 20% der 
 Baugenehmigungsverfahren von den langsamsten 20%?"

Schritt 1: Segmentierung der historischen Fälle
─────────────────────────────────────────────────
Fast Cases:  Alle GENEHMIGT/ABGELEHNT mit Dauer < 30 Tage (n=145)
Slow Cases:  Alle GENEHMIGT/ABGELEHNT mit Dauer > 75 Tage (n=132)

Schritt 2: Feature-Extraktion via ThemisDB
──────────────────────────────────────────
Pro Fall extrahiert:
  - Dokumenten-Vollständigkeit bei Eingang
  - Anzahl Prozessknoten
  - Anzahl externer Abhängigkeiten
  - Sachbearbeiter-Wechsel (Reassignments)
  - Anzahl Nachforderungen
  - Geografische Lage (Schutzzone ja/nein)

Schritt 3: Vector-Embedding für Ähnlichkeitsvergleich
──────────────────────────────────────────────────────
Jeder Fall als Embedding → Cluster-Analyse
→ Fast-Cluster: 3 Cluster erkannt
→ Slow-Cluster: 4 Cluster erkannt
```

### AQL: Fast vs. Slow Case Analyse

```aql
-- Vergleichsanalyse: Schnelle vs. langsame Fälle
LET fast_cases = (
    FOR p IN process
      FILTER p.process_type == 'BAUGENEHMIGUNG'
        AND p.status IN ['GENEHMIGT', 'ABGELEHNT']
        AND p.completed_at IS NOT NULL
      LET duration = DATE_DIFF(p.submitted_at, p.completed_at, 'days')
      FILTER duration < 30
      RETURN {
          id: p.id,
          duration: duration,
          metadata: p.metadata,
          embedding: p.embedding
      }
)

LET slow_cases = (
    FOR p IN process
      FILTER p.process_type == 'BAUGENEHMIGUNG'
        AND p.status IN ['GENEHMIGT', 'ABGELEHNT']
        AND p.completed_at IS NOT NULL
      LET duration = DATE_DIFF(p.submitted_at, p.completed_at, 'days')
      FILTER duration > 75
      RETURN {
          id: p.id,
          duration: duration,
          metadata: p.metadata,
          embedding: p.embedding
      }
)

-- Feature-Vergleich
LET fast_stats = (
    FOR c IN fast_cases
      LET node_count = LENGTH(FOR n IN process_node WHERE n.process_id == c.id RETURN n)
      LET reassign_count = LENGTH(
          FOR al IN audit_log
            FILTER al.process_id == c.id
              AND al.action == 'NODE_ASSIGNED'
            RETURN al
      )
      LET demand_count = LENGTH(
          FOR n IN process_node
            FILTER n.process_id == c.id
              AND n.node_type == 'INTERMEDIATE_EVENT'
              AND n.metadata.event_type == 'NACHFORDERUNG'
            RETURN n
      )
      COLLECT
        AGGREGATE avg_nodes = AVG(node_count),
                  avg_reassigns = AVG(reassign_count),
                  avg_demands = AVG(demand_count),
                  avg_initial_completeness = AVG(c.metadata.initial_doc_completeness)
      RETURN { avg_nodes, avg_reassigns, avg_demands, avg_initial_completeness }
)

RETURN {
    fast_cases_count: LENGTH(fast_cases),
    slow_cases_count: LENGTH(slow_cases),
    fast_stats: fast_stats[0],
    interpretation: "Analyse abgeschlossen – Faktoren extrahiert"
}
```

### Ergebnis-Report

```
Prozessoptimierungs-Analyse – Baugenehmigung
Zeitraum: 01.01.2024 – 01.02.2026
───────────────────────────────────────────────────────────

SCHNELLE FÄLLE (< 30 Tage, n=145)   LANGSAME FÄLLE (> 75 Tage, n=132)
─────────────────────────────────   ────────────────────────────────────
Ø Vollständigkeit Eingang:  94%     Ø Vollständigkeit Eingang:  61%
Ø Prozessknoten:            6.2     Ø Prozessknoten:            9.8
Ø Nachforderungen:          0.3     Ø Nachforderungen:          2.7
Ø Sachbearbeiter-Wechsel:   0.1     Ø Sachbearbeiter-Wechsel:   1.8
Ø Externe Gutachten:        0.4     Ø Externe Gutachten:        2.1
Schutzzone (Anteil):         8%     Schutzzone (Anteil):        41%

🔍 HAUPTFAKTOREN für Verzögerungen:

1. Unvollständige Unterlagen bei Eingang
   Impact: -23 Tage Ø Mehraufwand
   Maßnahme: KI-Vollständigkeitsprüfung vor Einreichung (Portal)

2. Externe Gutachten ohne Fristmanagement
   Impact: -18 Tage Ø Mehraufwand pro externem Gutachten
   Maßnahme: Automatische Erinnerungen ab Tag 7

3. Häufige Sachbearbeiter-Wechsel
   Impact: -12 Tage Ø Einarbeitungszeit
   Maßnahme: Workload-Balancing zu Beginn, nicht reaktiv

4. Schutzzonen-Anträge strukturell aufwändiger
   Impact: Unvermeidlich, aber SLA-Anpassung möglich

💡 KI-EMPFEHLUNGEN:

Wenn alle 3 Maßnahmen umgesetzt:
  Erwartete Ø Reduktion: -15 bis -20 Tage
  Neue Ø Bearbeitungszeit: 32–37 Tage (statt 52)
  SLA-Einhaltungsrate: +12% erwartet

[Report exportieren] [Maßnahmen-Plan erstellen] [Dashboard teilen]
```

### Vector-Search: Ähnliche Erfolgsfälle finden

```aql
-- Für einen aktuell schwierigen Fall: Finde ähnliche Fälle die trotzdem schnell waren
LET current_case = FIRST(FOR p IN process WHERE p.id == @current_case_id RETURN p)
LET current_embedding = current_case.embedding

-- Suche ähnliche SCHNELLE Fälle als Orientierung
FOR p IN process
  FILTER p.id != @current_case_id
    AND p.process_type == current_case.process_type
    AND p.status IN ['GENEHMIGT', 'ABGELEHNT']
  LET duration = DATE_DIFF(p.submitted_at, p.completed_at, 'days')
  FILTER duration < 35  -- Nur schnelle Fälle
  LET similarity = SIMILARITY(p.embedding, current_embedding)
  FILTER similarity > 0.78
  SORT similarity DESC
  LIMIT 5
  LET process_nodes = (
      FOR n IN process_node
        FILTER n.process_id == p.id
        SORT n.started_at ASC
        RETURN { title: n.title, duration_days: DATE_DIFF(n.started_at, n.completed_at, 'days') }
  )
  RETURN {
      reference:        p.reference_number,
      similarity:       similarity,
      total_duration:   duration,
      completed:        p.status,
      key_steps:        process_nodes,
      success_factors:  p.metadata.success_notes  -- Manuell annotiert vom SB
  }
```

---

*Verweise:*
- [`docs/de/architecture/DMS_MODERN_ARCHITECTURE.md`](docs/de/architecture/DMS_MODERN_ARCHITECTURE.md) – Architektur-Hintergrund
- [`docs/de/design/THEMIS_DMS_DATA_MODEL.md`](docs/de/design/THEMIS_DMS_DATA_MODEL.md) – Datenmodell der obigen Queries
- [`docs/de/integration/KI_ML_INTEGRATION_GUIDE.md`](docs/de/integration/KI_ML_INTEGRATION_GUIDE.md) – KI-Systemdetails
- [`docs/de/design/WORKFLOW_INTERACTION_PATTERNS.md`](docs/de/design/WORKFLOW_INTERACTION_PATTERNS.md) – User Journeys
