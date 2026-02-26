# Workflow Interaction Patterns – Themis.DocumentManager

**Kategorie:** 🎨 UX-Design  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [User Journey 1: Antrag einreichen (Bürger)](#-user-journey-1-antrag-einreichen-bürger)
- [User Journey 2: Aufgabe bearbeiten (Sachbearbeiter)](#-user-journey-2-aufgabe-bearbeiten-sachbearbeiter)
- [User Journey 3: Prozess überwachen (Supervisor)](#-user-journey-3-prozess-überwachen-supervisor)
- [User Journey 4: Graph-Explorer (Analyst)](#-user-journey-4-graph-explorer-analyst)
- [User Journey 5: Batch-Operation (Admin)](#-user-journey-5-batch-operation-admin)
- [User Journey 6: KI-gestützte Suche](#-user-journey-6-ki-gestützte-suche)
- [User Journey 7: Kollaborative Prüfung](#-user-journey-7-kollaborative-prüfung)
- [Kontextmenü-Intelligenz](#-kontextmenü-intelligenz)
- [Drag-Drop-Semantik](#-drag-drop-semantik)
- [Real-time Collaboration Patterns](#-real-time-collaboration-patterns)

---

## 🎯 Übersicht

Diese Spezifikation beschreibt detaillierte Benutzerinteraktions-Szenarien für das Themis.DocumentManager DMS. Jede User Journey deckt einen realistischen Verwaltungsprozess ab und beschreibt die erwarteten Interaktionsmuster, Systemreaktionen und KI-Unterstützung.

### Benutzerrollen

| Rolle | Beschreibung | Primäre Aktionen |
|-------|-------------|-----------------|
| **Bürger** | Externer Antragsteller | Antrag stellen, Status verfolgen |
| **Sachbearbeiter** | Interner Bearbeiter | Aufgaben bearbeiten, Entscheidungen treffen |
| **Supervisor** | Teamleiter, Abteilungsleiter | Überwachen, Eskalieren, Ressourcen planen |
| **Analyst** | Prozessoptimierung, Qualitätssicherung | Auswerten, Berichte, Graphen explorieren |
| **Administrator** | Systemverwaltung | Workflows konfigurieren, Benutzer verwalten |

---

## 👤 User Journey 1: Antrag einreichen (Bürger)

**Kontext:** Ein Bürger möchte eine Baugenehmigung beantragen.  
**Kanal:** Web-Portal (OZG-konform)  
**Dauer:** ~20 Minuten

### Schritte

```
1. EINSTIEG
   Bürger öffnet Antrags-Portal
   → System: Zeigt "Antrag stellen" Dashboard
   → KI: Erkennt Benutzer-Kontext (zurückkehrender Nutzer? Offene Anträge?)

2. PROZESS-AUSWAHL
   Bürger tippt "Baugenehmigung" in Suchfeld
   → System: Zeigt Top-3-Vorschläge mit Beschreibung und Bearbeitungsdauer
   → KI: Schlägt "Baugenehmigung – Neubau" vs. "Baugenehmigung – Umbau" vor
            basierend auf historischem Kontext des Bürgers

3. FORMULAR-WIZARD
   Schritt 1: Grunddaten (Adresse des Bauvorhabens)
   → System: Geo-Autocomplete für Straßen + Validierung (liegt in Stadtgebiet?)
   → KI: Erkennt Schutzzone → Warnung: "Gebiet liegt im Landschaftsschutzgebiet"

   Schritt 2: Dokument-Upload
   → System: Akzeptierte Formate: PDF, DXF, IFC
   → KI: Automatische Klassifizierung: "Lageplan erkannt ✓", "Bauzeichnung erkannt ✓"
            Fehlende Dokumente: "Statiknachweis fehlt – erforderlich für Neubau"

   Schritt 3: Zusammenfassung
   → System: Vollständigkeitsprüfung
   → KI: "Geschätzte Bearbeitungszeit: 35-45 Werktage"
            "Genehmigungswahrscheinlichkeit: 87% (basierend auf 142 ähnlichen Fällen)"

4. ABSENDEN
   Bürger klickt "Antrag einreichen"
   → System: Erstellt PROCESS-Instanz in ThemisDB, generiert Aktenzeichen
   → E-Mail-Bestätigung mit Tracking-Link
   → Prozess-Graph wird initialisiert (Start-Event → erste Tasks)

5. STATUS-TRACKING
   Bürger kann jederzeit Status verfolgen:
   → Prozess-Timeline mit aktueller Position
   → Nächster erwarteter Schritt und Frist
   → Direkter Kontakt zum zuständigen Sachbearbeiter (DSGVO-konform)
```

### Interaktions-Highlights

- **Progressive Disclosure:** Nur relevante Felder werden angezeigt
- **Geo-Intelligence:** Automatische Erkennung von Zuständigkeiten anhand Adresse
- **KI-Vollständigkeitsprüfung:** Fehlende Dokumente werden vor Absenden erkannt
- **Predictive Feedback:** Bürger erhält sofortiges Feedback zu Erfolgschancen

---

## 🔧 User Journey 2: Aufgabe bearbeiten (Sachbearbeiter)

**Kontext:** Sachbearbeiter Hans Müller beginnt seinen Arbeitstag.  
**Interface:** Desktop-App (Split-Screen)  
**Dauer:** Typischer 8-Stunden-Tag

### Schritte

```
1. DASHBOARD-EINSTIEG (08:00)
   Sachbearbeiter loggt sich ein
   → System: Zeigt personalisiertes Dashboard:
     - 3 neue Aufgaben (heute fällig)
     - 1 Aufgabe überfällig (SLA-Alert!)
     - 5 Aufgaben diese Woche
   → KI: "Empfehlung: Beginnen Sie mit 'FALL-2026-0847' (höchste Priorität,
            KI-Score 0.91, Bürger hat nachgefragt)"

2. AUFGABE ÖFFNEN
   Sachbearbeiter klickt auf FALL-2026-0847
   → System: Split-View öffnet:
     - Links: Dokumenten-Preview (Antrag, Anhänge)
     - Mitte: Prozess-Graph (aktueller Knoten hervorgehoben)
     - Rechts: Aufgaben-Panel (Checkliste, Kommentare, KI-Insights)
   → KI-Insights-Panel zeigt:
     - "3 ähnliche genehmigte Fälle aus 2025"
     - "Hauptrisiko: Lärmschutz (in 40% der ähnlichen Fälle Nachforderung)"
     - "Vorgeschlagene Aktion: Lärmschutzgutachten anfordern"

3. ENTSCHEIDUNG TREFFEN
   Sachbearbeiter prüft Dokumente, nutzt KI-Insights
   → Kontextmenü (Rechtsklick auf Prozessknoten):
     [✓] Aufgabe abschließen (weiter zu nächstem Schritt)
     [!] Nachforderung auslösen (blockiert Prozess, benachrichtigt Bürger)
     [→] Weiterleiten (an anderen Sachbearbeiter/Amt)
     [↑] Eskalieren (an Supervisor)
     [✎] Kommentar hinzufügen
   → Sachbearbeiter wählt "Nachforderung auslösen"

4. NACHFORDERUNG
   → Dialog öffnet: Art der Nachforderung auswählen
   → KI: Schlägt Standard-Formulierung vor
   → System: Erstellt neuen Knoten im Prozess-Graph (NACHFORDERUNG)
     - Fügt Edge hinzu: PRÜFUNG → NACHFORDERUNG (bedingter Pfad)
     - Setzt Deadline für Bürger-Antwort: 14 Tage
     - Sendet automatisch Benachrichtigung an Bürger

5. NÄCHSTE AUFGABE (08:45)
   → System: Dashboard aktualisiert sich automatisch
   → KI: Neuer Vorschlag für nächste Aufgabe
   → Drag-Drop: Sachbearbeiter verschiebt Aufgabe in Kalender-Slot "Nachmittag"
```

### Interaktions-Highlights

- **Fokussierte Ansicht:** Alles Nötige in einem Screen sichtbar
- **KI-Shortcuts:** Ein-Klick-Aktionen für KI-empfohlene nächste Schritte
- **Dynamischer Graph:** Prozess-Graph aktualisiert sich bei jeder Aktion live
- **Smart Notifications:** Nur relevante Benachrichtigungen (kein Noise)

---

## 👁️ User Journey 3: Prozess überwachen (Supervisor)

**Kontext:** Supervisorin möchte SLA-Einhaltung für ihr Team prüfen.  
**Interface:** Dashboard-View (Gantt + Graph)  
**Dauer:** ~30 Minuten morgens

### Schritte

```
1. PORTFOLIO-ÜBERSICHT
   Supervisorin öffnet Portfolio-View
   → System: Zeigt alle 47 aktiven Prozesse in ihrem Bereich:
     - Farb-kodiert: Grün (on-track), Gelb (risiko), Rot (überfällig)
     - 3 rote Prozesse sofort sichtbar
   → KI: "2 Prozesse drohen SLA-Verletzung in 48h"

2. SLA-DRILL-DOWN
   Supervisorin klickt auf roten Prozess
   → System: Öffnet Process-Execution-View mit:
     - Aktueller Graph-Status (welcher Knoten ist blockiert)
     - SLA-Countdown (noch 1 Tag 4 Stunden)
     - Blockade-Ursache: "Warten auf Umweltgutachten (extern)"
   → KI: "Eskalationsoption: Gutachter per Direktkontakt erinnern"

3. RESSOURCEN-AUSGLEICH
   Supervisorin sieht: Sachbearbeiter A hat 12 offene Tasks, B nur 3
   → Drag-Drop: Supervisorin zieht 3 Tasks von A zu B
   → System:
     - Prüft Kompetenzen von B für diese Prozesstypen
     - Warnung falls fehlende Qualifikation
     - Aktualisiert Prozess-Graphen (neue Assignee-Edges)
   → KI: "Neue Verteilung reduziert A's SLA-Risiko um 67%"

4. GLOBALE SLA-ANALYSE
   → Dashboard zeigt Zeitreihe: SLA-Einhaltungsrate letzte 30 Tage
   → KI: "Montags 23% mehr Verspätungen – Muster erkannt"
   → Empfehlung: "Kapazität Montags erhöhen oder Deadline-Policy anpassen"

5. BERICHT EXPORTIEREN
   → Supervisorin wählt Zeitraum und Metriken
   → System generiert PDF/Excel-Report
   → Export: Direkt in ThemisDB-Archiv mit Metadaten versehen
```

---

## 🔍 User Journey 4: Graph-Explorer (Analyst)

**Kontext:** Prozessanalyst untersucht Engpässe in Baugenehmigungsverfahren.  
**Interface:** Graph-Explorer-View (Zentral, interaktiv)

### Schritte

```
1. GRAPH-EINSTIEG
   Analyst öffnet Graph-Explorer
   → System: Zeigt Knowledge-Graph aller Prozesse:
     - Nodes: Prozess-Instanzen, Beteiligte, Dokumente, Ämter
     - Edges: Abhängigkeiten, Zuweisungen, Referenzen
   → Filter: "Nur BAUGENEHMIGUNG, letzte 6 Monate"

2. CLUSTER-ANALYSE
   → Analyst sieht Cluster: Gruppe von 18 Prozessen mit ähnlicher Struktur
   → Klick auf Cluster → Zoom-In
   → KI: "Cluster = Wohnbau in Bezirk Nord, alle haben Lärmschutz-Problem"

3. PATH-ANALYSE
   → Analyst wählt zwei Prozessknoten
   → System: Findet kürzeste/häufigste Pfade zwischen ihnen (AQL Graph-Traversal)
   → Visualisierung: Pfade werden animiert dargestellt

4. ANOMALIE-MARKIERUNG
   → Analyst sieht isolierten Prozess-Knoten (keine Verbindung zu anderen)
   → Rechtsklick → "Als Anomalie markieren"
   → System: Erstellt Audit-Eintrag, benachrichtigt Supervisor

5. BOTTLENECK-VISUALISIERUNG
   → Filter: "Zeige Knoten mit durchschnittlicher Verweildauer > 5 Tage"
   → System: Färbt Knoten nach Verweildauer (Heatmap)
   → Ergebnis: "Umweltprüfung" = Engpass (Ø 12 Tage statt SLA 5 Tage)
   → KI: "Empfehlung: Externe Gutachter-Liste erweitern (3 Optionen)"
```

---

## ⚡ User Journey 5: Batch-Operation (Admin)

**Kontext:** Admin möchte 50 abgeschlossene Prozesse archivieren.  
**Interface:** Batch-Operations-Panel

### Schritte

```
1. SELEKTION
   Admin öffnet Batch-Operations-View
   → Filter: Status=ABGESCHLOSSEN, Abschlussdatum < vor 2 Jahren
   → System: 247 Prozesse gefunden
   → KI: "23 Prozesse haben referenzierte offene Folgeprozesse – ausschließen?"
   → Admin: Ja → 224 Prozesse selektiert

2. AKTION AUSWÄHLEN
   → Kontextmenü mit Batch-Aktionen:
     [📦] Archivieren (mit Kompression)
     [🔒] Einfrieren (Read-Only, kein Archiv)
     [🗑️] Löschen (nur mit Admin-Berechtigung + Grund)
     [📤] Exportieren (ZIP mit Metadaten)
   → Admin wählt "Archivieren"

3. VORSCHAU & BESTÄTIGUNG
   → System: Zeigt Zusammenfassung:
     - 224 Prozesse (gesamt 4.7 GB)
     - Geschätzte Kompression: 73% → 1.3 GB
     - Archivort: /archive/2024/baugenehmigungen/
   → Admin bestätigt mit Zwei-Faktor-Bestätigung

4. AUSFÜHRUNG
   → Progress-Bar mit Echtzeit-Status
   → System: Batch läuft als Hintergrundprozess
   → Benachrichtigung bei Abschluss (inkl. Fehlerprotokoll falls nötig)

5. PROTOKOLL
   → Vollständiges Audit-Log in ThemisDB: Wer, was, wann, wie viele
   → Export des Protokolls als signiertes PDF (elektronische Signatur)
```

---

## 🤖 User Journey 6: KI-gestützte Suche

**Kontext:** Sachbearbeiterin sucht nach einem Präzedenzfall.  
**Interface:** KI-Chat-Panel (VSCode-Style, rechte Seite)

### Interaktions-Flow

```
Eingabe (Natural Language):
"Zeige mir alle Baugenehmigungen in Stuttgart-Nord aus 2025,
 bei denen Lärmschutz das Hauptproblem war, und die am Ende
 genehmigt wurden"

System-Verarbeitung:
1. LLM: Text → AQL-Query konvertieren
2. AQL ausführen gegen ThemisDB
3. Ergebnisse anreichern (Vector-Similarity für "ähnliche Probleme")
4. Antwort strukturieren

Ausgabe:
┌─────────────────────────────────────────────────────────────┐
│ 🤖 ThemisAI                                                 │
│                                                             │
│ Gefunden: 14 Fälle in Stuttgart-Nord (2025), Lärmschutz,   │
│ Status: GENEHMIGT                                          │
│                                                             │
│ Top-3 ähnlichste Fälle:                                     │
│ • FALL-2025-0341 (Ähnlichkeit: 97%) – Hauptstraße 12       │
│ • FALL-2025-0198 (Ähnlichkeit: 94%) – Rosenbergstraße 5    │
│ • FALL-2025-0512 (Ähnlichkeit: 91%) – Schlossplatz 3       │
│                                                             │
│ Gemeinsames Lösungsmuster:                                  │
│ "In 12/14 Fällen wurde Schallschutzgutachten nach          │
│  DIN 4109 eingereicht. Durchschnittliche Bearbeitungszeit:  │
│  52 Tage."                                                  │
│                                                             │
│ [Alle 14 Fälle anzeigen] [In Graph explorieren] [Export]   │
└─────────────────────────────────────────────────────────────┘

Follow-up:
"Welche Gutachter wurden in diesen Fällen verwendet?"
→ System: Aggregiert Gutachter-Namen aus Dokument-Metadaten
→ Zeigt Top-5-Gutachter mit Erfolgsquote
```

---

## 👥 User Journey 7: Kollaborative Prüfung

**Kontext:** Zwei Sachbearbeiter prüfen gemeinsam einen komplexen Fall.  
**Interface:** Real-time Collaboration View

### Interaktions-Flow

```
1. SESSION STARTEN
   Sachbearbeiter A: "@ Sachbearbeiter B – bitte prüfe Abschnitt 3.2"
   → B erhält Push-Benachrichtigung
   → B klickt auf Link → öffnet exakt dieselbe Ansicht wie A

2. GLEICHZEITIGE BEARBEITUNG
   → Beide sehen sich gegenseitig (Cursor-Avatare wie in Google Docs)
   → A markiert Abschnitt im Dokument → Kommentar hinzufügen
   → B sieht A's Markierung in Echtzeit (grüner Cursor)
   → Keine Konflikte: MVCC in ThemisDB sichert konsistente Sicht

3. KOMMENTAR-THREAD
   A: "Statiknachweis ist unvollständig – fehlt Fundament-Berechnung"
   B: "@A Ich sehe das auch. Nachforderung auslösen?"
   A: "Ja, ich mache das"
   → A löst Nachforderung aus → Prozess-Graph aktualisiert sich für beide

4. ENTSCHEIDUNG PROTOKOLLIEREN
   → System: Erstellt gemeinsamen Entscheidungs-Audit-Eintrag
   → "Entschieden von: A + B um 14:23 Uhr"
   → Vier-Augen-Prinzip für kritische Entscheidungen konfigurierbar

5. SESSION BEENDEN
   → Chat-Verlauf wird in Prozess-Akte gespeichert
   → Beide erhalten Zusammenfassung der gemeinsamen Aktionen
```

---

## 🖱️ Kontextmenü-Intelligenz

Das Kontextmenü zeigt **nur relevante Aktionen** basierend auf:
- Aktueller Prozess-Status des Knotens
- Benutzerrolle und Berechtigungen
- KI-Empfehlungen (beste nächste Aktionen)

### Beispiel: Prozessknoten-Kontextmenü

```
Kontext: Knoten PRÜFUNG_UNTERLAGEN, Status=AKTIV, Benutzer=Sachbearbeiter

┌─────────────────────────────────────────────┐
│  📄 Prüfung Unterlagen                       │
│  Status: In Bearbeitung  │  Frist: 3 Tage   │
├─────────────────────────────────────────────┤
│  ⭐ KI-Empfehlung: Abschließen (95% Conf.)  │
├─────────────────────────────────────────────┤
│  ✅ Aufgabe abschließen                      │
│  ❗ Nachforderung auslösen                   │
│  → Weiterleiten...                           │
│  ↑ Eskalieren                                │
├─────────────────────────────────────────────┤
│  💬 Kommentar hinzufügen                     │
│  📎 Dokument anhängen                        │
│  📊 Zeiterfassung starten                    │
├─────────────────────────────────────────────┤
│  👁️ Ähnliche Fälle anzeigen                  │
│  📈 Prozess-Analyse                          │
└─────────────────────────────────────────────┘
```

### Kontextmenü-Regeln

| Bedingung | Sichtbare Aktionen |
|-----------|-------------------|
| `status == WARTEND` | Reaktivieren, Grund einsehen |
| `status == AKTIV && assignee == currentUser` | Abschließen, Nachfordern, Eskalieren |
| `status == AKTIV && assignee != currentUser` | Nur-Lesen, Kommentieren |
| `deadline < 24h` | Eskalieren (immer sichtbar) |
| `role == SUPERVISOR` | Alle Aktionen + Reassign |
| `KI-Score > 0.9` | KI-Empfehlung prominent anzeigen |

---

## 🖱️ Drag-Drop-Semantik

### Drag-Drop-Operationen

| Aktion | Quelle | Ziel | Verhalten |
|--------|--------|------|-----------|
| **Reassignment** | Task-Karte | Sachbearbeiter-Avatar | Aufgabe neu zuweisen (mit Kompetenz-Check) |
| **Prioritäts-Verschiebung** | Task-Karte | Kalender-Slot | Bearbeitungszeitraum planen |
| **Graph-Reorganisation** | Prozessknoten | neue Position | Visuelle Reorganisation (kein semantischer Effekt) |
| **Dokument-Zuordnung** | Dokument | Prozessknoten | Dokument an Knoten anheften |
| **Prozess-Merging** | Prozess-Karte | anderer Prozess | Abhängigkeit erstellen (mit Bestätigungsdialog) |

### Drag-Drop-Validierung

Beim Drag-Over-Event wird sofort visuelles Feedback gegeben:
- ✅ **Grüner Rahmen:** Aktion ist zulässig
- ⚠️ **Gelber Rahmen:** Aktion möglich, aber Warnung (z.B. fehlende Kompetenz)
- ❌ **Roter Rahmen:** Aktion nicht erlaubt (z.B. keine Berechtigung)
- 💬 **Tooltip:** Erklärt warum erlaubt/verboten

---

## 🔄 Real-time Collaboration Patterns

### Technische Grundlage

```
WebSocket-Verbindung: Client ↔ ThemisDB-Server
Event-Typen:
  - PROCESS_NODE_STATUS_CHANGED
  - PROCESS_EDGE_ADDED
  - COMMENT_ADDED
  - ASSIGNMENT_CHANGED
  - DOCUMENT_UPLOADED
  - USER_CURSOR_MOVED (bei aktiver Co-Editing-Session)

Konflikt-Auflösung:
  - ThemisDB MVCC (Multi-Version Concurrency Control)
  - Optimistic Locking für Prozessknoten-Zustand
  - Last-Write-Wins für Kommentare
  - Merge-Dialog für gleichzeitige Entscheidungen (Vier-Augen-Prinzip)
```

### Presence-Awareness

- **Online-Indikator:** Grüner Punkt bei aktiven Benutzern in demselben Prozess
- **Cursor-Tracking:** Avatare bewegen sich in Echtzeit (nur in Ko-Editing-Sessions)
- **"Jemand schreibt...":** Typing-Indicator in Kommentar-Threads
- **Zusammenfassung:** "3 Personen sehen diesen Prozess gerade"

### Notification-Hierarchie

```
SOFORT (Push/Sound):
  - @-Mention an aktuellen Benutzer
  - SLA-Verletzung in eigenen Prozessen
  - Eskalation

BALD (Badge, 5min):
  - Neue Aufgabe zugewiesen
  - Nachforderung beantwortet

TÄGLICH (E-Mail-Digest):
  - Statusupdates ohne Handlungsbedarf
  - KI-Insights-Zusammenfassung
```

---

*Siehe auch:*
- [`docs/de/design/SCREEN_LAYOUTS.md`](SCREEN_LAYOUTS.md) – Bildschirmlayouts
- [`docs/de/design/COMPONENTS_AND_WIDGETS.md`](COMPONENTS_AND_WIDGETS.md) – Komponentenbibliothek
- [`examples/modern_dms_workflow_scenarios.md`](../../examples/modern_dms_workflow_scenarios.md) – Konkrete Szenarien
