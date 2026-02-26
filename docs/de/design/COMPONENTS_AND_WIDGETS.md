# Komponenten & Widgets – Themis.DocumentManager

**Kategorie:** 🎨 UX-Design  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Prozess-Komponenten](#-prozess-komponenten)
- [Graph-Komponenten](#-graph-komponenten)
- [Zeitliche Komponenten](#-zeitliche-komponenten)
- [KI & Analytics-Komponenten](#-ki--analytics-komponenten)
- [Kollaborations-Komponenten](#-kollaborations-komponenten)
- [Navigation & Layout-Komponenten](#-navigation--layout-komponenten)
- [Utility-Komponenten](#-utility-komponenten)
- [Accessibility-Anforderungen](#-accessibility-anforderungen)

---

## 🎯 Übersicht

Diese Komponentenbibliothek definiert 28 wiederverwendbare Widgets für das Themis.DocumentManager DMS. Jede Komponente ist nach Zweck, Verhalten und Accessibility-Anforderungen spezifiziert.

### Design-System-Basis

| Eigenschaft | Wert |
|-------------|------|
| **Framework** | React/TypeScript oder Blazor (je nach Stack) |
| **Design-Token** | CSS Custom Properties |
| **Icon-Set** | Lucide Icons (Open Source) |
| **Accessibility** | WCAG 2.1 AA |
| **Responsive** | Mobile-first |

---

## ⚙️ Prozess-Komponenten

### 1. ProcessNodeCard

**Zweck:** Darstellung eines einzelnen Prozessknotens als klickbare Karte.

```
┌──────────────────────────────────────┐
│  🔵 PRÜFUNG UNTERLAGEN               │
│  ──────────────────────              │
│  H. Müller  │  Frist: 3 Tage ⚠️      │
│  ██████░░░░  60%                     │
│  [Abschließen]  [Nachforderung]      │
└──────────────────────────────────────┘
```

**Props/Attribute:**
| Name | Typ | Beschreibung |
|------|-----|-------------|
| `nodeId` | string | Eindeutige Knoten-ID |
| `title` | string | Knotenname |
| `status` | NodeStatus | AKTIV \| WARTEND \| FERTIG \| FEHLER |
| `assignee` | User | Zugewiesener Sachbearbeiter |
| `deadline` | Date | Fälligkeitsdatum |
| `progress` | 0–100 | Fortschritt in Prozent |
| `actions` | Action[] | Verfügbare Aktionen |
| `aiScore` | 0–1 | KI-Empfehlungs-Konfidenz |

**Verhalten:**
- Klick: Öffnet Detail-Panel oder wechselt zu Process-Execution-View
- Hover: Zeigt Tooltip mit vollständigen Details
- Rechtsklick: Öffnet Kontextmenü mit Aktionen
- Status-Farbe: Automatisch basierend auf `status` und verbleibender Fristdauer
- Deadline < 24h: Pulsierender roter Rahmen + Warnsymbol

**Accessibility:**
- `role="article"`, `aria-label="Prozessknoten: {title}, Status: {status}"`
- Keyboard-fokussierbar (`tabindex="0"`)
- Enter/Space löst Hauptaktion aus
- Farbkodierung nicht alleiniges Informationsmedium (Symbole ergänzend)

---

### 2. ProcessStatusBadge

**Zweck:** Kompaktes Status-Indikator-Widget für Listen und Tabellen.

```
🔵 In Bearbeitung    🔴 Überfällig    ✅ Abgeschlossen    ⏸ Wartend
```

**Props:** `status: ProcessStatus`, `showLabel: boolean`, `size: 'sm'|'md'|'lg'`

**Verhalten:** Immer Symbol + (optionaler) Text, niemals Farbe allein.

---

### 3. SLACountdown

**Zweck:** Visueller Countdown bis zur Prozess-Deadline mit Status-Anzeige.

```
Standard (5+ Tage):    ████████████ 12 Tage
Warnung (1-4 Tage):    ████████░░░░ ⚠️ 3 Tage
Kritisch (< 24h):      ████░░░░░░░░ 🔴 18 Stunden  [← pulsiert]
Überfällig:            ██░░░░░░░░░░ 🔴 2 Tage überschritten
```

**Props:** `deadline: Date`, `slaDefinition: SLAConfig`, `showBar: boolean`

**Verhalten:**
- Automatische Farbänderung basierend auf verbleibender Zeit
- Pulsiereffekt bei < 24 Stunden
- Tooltip: Absolutes Datum + Uhrzeit on hover

---

### 4. ProcessActionPanel

**Zweck:** Kontextsensitives Panel mit verfügbaren Aktionen für einen Prozessknoten.

```
┌────────────────────────────────────┐
│  Verfügbare Aktionen               │
│  ─────────────────────────────     │
│  ⭐ [✅ Abschließen]  KI: 95%      │  ← KI-Empfehlung hervorgehoben
│  [❗ Nachforderung auslösen]        │
│  [→ Weiterleiten...]                │
│  [↑ Eskalieren]                    │
│  ──────────────────                 │
│  [💬 Kommentieren]                  │
│  [📎 Dokument anhängen]             │
└────────────────────────────────────┘
```

**Props:** `processNode: ProcessNode`, `userRole: Role`, `aiRecommendations: AIAction[]`

**Verhalten:** Aktionen werden dynamisch basierend auf Status + Rolle + KI-Score gefiltert und sortiert (KI-Empfehlungen oben).

---

### 5. ProcessBreadcrumb

**Zweck:** Zeigt den Pfad im Prozess-Graph von Start bis aktuellen Knoten.

```
Antrag eingereicht → Vollständigkeitsprüfung ✅ → Sachprüfung 🔵 (aktuell)
```

**Props:** `pathNodes: ProcessNode[]`, `currentNodeId: string`

**Verhalten:**
- Klick auf abgeschlossene Knoten öffnet deren History
- Nur relevanter Pfad bis zum aktuellen Knoten angezeigt
- Parallele Pfade werden mit `↔` Symbol angedeutet

---

## 🕸️ Graph-Komponenten

### 6. ProcessGraphCanvas

**Zweck:** Interaktive BPMN-ähnliche Graph-Visualisierung einer Prozessinstanz.

```
Technologie: SVG-basiert (D3.js oder Cytoscape.js)

Knoten-Typen:
  ○ Start/End Event (Kreis)
  □ Task/User Task (Rechteck, abgerundete Ecken)
  ◇ Gateway (Raute): XOR / AND / OR
  ◎ Intermediate Event (Doppelkreis)
  ▭ Sub-Process (Rechteck mit +)

Kanten-Typen:
  → Sequenzfluss (ausgefüllter Pfeil)
  ··→ Nachrichtenfluss (gestrichelter Pfeil)
  – Assoziation (einfache Linie)
```

**Props:** `processId: string`, `layoutMode: 'auto'|'manual'`, `readonly: boolean`

**Verhalten:**
- Zoom: Maus-Scroll, Touch-Pinch
- Pan: Maus-Drag, Touch-2-Finger
- Knoten-Klick: Details im Side-Panel
- Knoten-Doppelklick: Aufgabe öffnen (nur bei User Tasks)
- Mini-Map in Ecke für große Graphs (> 20 Knoten)
- Layout-Algorithmen: Hierarchisch (Standard), Organisch, Links-nach-Rechts
- Animations: Status-Änderungen werden animiert (Farb-Übergang)

**Accessibility:**
- Alternative Textdarstellung per Tabelle (Barrierefreiheitsmodus)
- Keyboard-Navigation durch Knoten per Tab/Arrow-Keys
- Screen-Reader: Knoten-Fokus liest Typ, Status, Assignee vor

---

### 7. EdgeVisualizer

**Zweck:** Darstellung von Kanten im Prozess-Graph mit Typ und Bedingungen.

```
Sequenzfluss:        ──────────────→
Bedingt:             ──── [cond] ──→  (mit Bedingungstext über Kante)
Parallel-Join:       ──────┤├────→  (AND-Gateway Notation)
Ausschließend:       ──────┤X├────→  (XOR-Gateway Notation)
Blockiert:           ──────//────→  (Roter X-Marker)
```

**Props:** `edge: ProcessEdge`, `highlighted: boolean`

**Verhalten:**
- Hover: Zeigt Bedingung/Kontext als Tooltip
- Klick auf Kante: Öffnet Kanten-Details (Bedingungslogik, History)

---

### 8. KnowledgeGraphBrowser

**Zweck:** Globale Graph-Exploration über alle Prozesse, Dokumente und Personen.

**Verhalten:**
- Cluster-Erkennung und automatisches Zusammenfassen (> 10 Knoten)
- Filter-Panel: Knotentypen, Status, Zeitraum, Beteiligte
- Suche: Text-Eingabe filtert sichtbare Knoten live
- Path-Finder: Eingabe zweier Knoten → kürzester Pfad hervorgehoben
- Heatmap-Overlay: Verweildauer, Auslastung, Anomalie-Score

---

### 9. DependencyMatrix

**Zweck:** Tabellarische Darstellung von Prozess-Abhängigkeiten (alternative zu Graph).

```
         │ FALL-A │ FALL-B │ FALL-C │ FALL-D │
─────────┼────────┼────────┼────────┼────────┤
FALL-A   │   –    │  ──→   │        │        │
FALL-B   │        │   –    │  ──→   │  ──→   │
FALL-C   │        │        │   –    │        │
FALL-D   │  ──→   │        │        │   –    │
```

**Props:** `processes: Process[]`

---

## ⏱️ Zeitliche Komponenten

### 10. TimelineWithBranches

**Zweck:** Nicht-lineare Zeitachse mit parallelen Zweigen und Meilensteinen.

```
Zeit →

│ Antrag  │  Statik (parallel)  │
│ ─────── │ ─────────────────── │
│ 01.01   │ 10.01 ─────────── 20.01 │
│   ↓     │                         │
│ 05.01   │ Umwelt (parallel)       │
│ (Eingang│ 08.01 ────────── 25.01  │
│ vollstd.)│                         │
│   ↓     │ ──────────────────────  │
│ 28.01   │  Entscheidung (warte beides)
│ (Entsch.)│
```

**Props:** `processGraph: ProcessGraph`, `showParallelPaths: boolean`

**Verhalten:**
- Scroll: Horizontal durch Zeitachse
- Zoom: Granularität ändern (Tage/Wochen/Monate)
- Klick auf Ereignis: Details anzeigen
- Parallele Pfade werden als übereinanderliegende Zeilen dargestellt
- Abhängigkeits-Linien (vertikal) verbinden zusammengehörige Events

---

### 11. GanttProcessView

**Zweck:** Gantt-Diagramm für Portfolio-Übersicht mit SLA-Anzeige.

```
               Jan    Feb    Mär    Apr
FALL-0847  ████████████████░░░░░░░░  (Deadline 15.Feb)
FALL-0341  ██████████████████████░░  (Deadline 01.Apr)
FALL-0901  ████████░░░░░░░░░░░░░░░░  (ÜBERFÄLLIG) 🔴
```

**Props:** `processes: Process[]`, `timeRange: DateRange`

**Verhalten:**
- Drag-Drop: Deadlines verschieben (falls erlaubt)
- Klick auf Balken: Process öffnen
- Farb-Kodierung: Fortschritt vs. SLA-Rahmen

---

### 12. SLADashboard

**Zweck:** Globale SLA-Übersicht über alle Prozesse eines Teams/Amts.

```
SLA-Einhaltung letzte 30 Tage:
████████████████████████░░░░░░  87%

Aufschlüsselung:
✅ On-Track:      34 Fälle
⚠️ Risiko:         7 Fälle
🔴 Verletzt:        3 Fälle

Trend: ↗ +5% gegenüber Vormonat
```

**Props:** `teamId: string`, `timeRange: DateRange`

---

## 🤖 KI & Analytics-Komponenten

### 13. AIInsightsPanel

**Zweck:** Kontextuelles Panel mit KI-generierten Empfehlungen und Insights.

```
┌─────────────────────────────────────────┐
│  🤖 KI-Insights  │  Konfidenz: 94%      │
│  ───────────────────────────────────    │
│  📊 Ähnliche Fälle: 3 (2025)            │
│  • FALL-0341: 97% ähnlich               │
│  • FALL-0198: 94% ähnlich               │
│                                         │
│  🎯 Empfohlene Aktion:                  │
│  "Abschließen" (95% Erfolgsrate bei     │
│   ähnlichen Fällen)                     │
│                                         │
│  ⚠️ Risikohinweise:                      │
│  • Lärmschutz (40% Nachforderungsrate)  │
│  • Grenzabstand (15%)                   │
│                                         │
│  [Alle ähnlichen Fälle anzeigen]        │
│  [Feedback geben: 👍 👎]                │
└─────────────────────────────────────────┘
```

**Props:** `processId: string`, `context: ProcessContext`

---

### 14. PredictionScore

**Zweck:** Visualisierung der KI-Genehmigungswahrscheinlichkeit.

```
Genehmigungswahrscheinlichkeit:
78% ███████████░░░  ← Balken + Prozent

Basiert auf: 142 ähnliche Fälle
Konfidenz-Intervall: 71% – 85%

Haupt-Einflussfaktoren:
+ Vollständige Unterlagen    (+12%)
+ Konformer Bebauungsplan    (+18%)
- Fehlender Lärmschutz       (-15%)
- Nähe Schutzgebiet          (-8%)
```

---

### 15. AnomalyAlert

**Zweck:** Warnung bei erkannten Anomalien (ungewöhnliche Bearbeitungszeiten, Muster).

```
┌──────────────────────────────────────────────┐
│  ⚠️ Anomalie erkannt                          │
│  ─────────────────────────────────────────   │
│  Bearbeitungszeit "Umweltprüfung": 12 Tage   │
│  Erwartet: Ø 4 Tage (±1.5 Tage)             │
│                                              │
│  Mögliche Ursachen (KI):                    │
│  • Gutachter-Kapazität überlastet (67%)      │
│  • Komplexeres Objekt als Durchschnitt (23%) │
│                                              │
│  [Eskalieren]  [Ignorieren]  [Feedback]      │
└──────────────────────────────────────────────┘
```

---

### 16. NLQueryInterface

**Zweck:** Natural-Language-Sucheingabe mit KI-Antwort (VSCode-Stil).

```
┌──────────────────────────────────────────────────┐
│  🔍 ThemisAI Suche                               │
│  ─────────────────────────────────────────────── │
│  > Zeige alle Baugenehmigungen in Stuttgart...   │
│  ───────────────────────────────────────────     │
│  🤖 Ergebnis: 14 Fälle gefunden                 │
│                                                  │
│  [Top-3 Karten]                                  │
│                                                  │
│  Generiertes AQL:  [Anzeigen ▾]                 │
│  [Alle 14 anzeigen] [In Graph] [Export]          │
└──────────────────────────────────────────────────┘
```

---

### 17. ProcessDebugger

**Zweck:** Step-by-Step Trace durch eine Prozessinstanz für Fehleranalyse.

```
┌──────────────────────────────────────────────────┐
│  🔧 Prozess-Debugger  │  FALL-2026-0847          │
│  ──────────────────────────────────────────────  │
│  Schritt 1: Antrag eingereicht  ✅  01.01 09:23  │
│  Schritt 2: Vollständigkeitsprüfg. ✅ 02.01 11:45│
│  Schritt 3: ► Sachprüfung  🔵 AKTIV  seit 5 Tage │
│    ├── Sub-Task: Statikprüfung  ✅  abgeschlossen │
│    └── Sub-Task: Umweltprüfung  ⏸  WARTEND       │
│        └── Blockade: Externer Gutachter (#GTR-22) │
│  Schritt 4: Entscheidung  ⏸  ausstehend          │
│  ──────────────────────────────────────────────  │
│  Edge-Bedingungen prüfen: [Schritt 3 → Schritt 4]│
│  Bedingung: ALL(sub_tasks.status == 'FERTIG')    │
│  Aktuell: STATIK=FERTIG, UMWELT=WARTEND → FALSCH │
└──────────────────────────────────────────────────┘
```

---

## 💬 Kollaborations-Komponenten

### 18. CommentThread

**Zweck:** Kommentar-Thread an einem Prozessknoten, Dokument oder im Kontext.

```
┌──────────────────────────────────────────────────┐
│  💬 Kommentare (3)                               │
│  ─────────────────────────────────────────────── │
│  [Avatar] H. Müller  │  14:23                   │
│  "Statiknachweis ist unvollständig.              │
│   @K. Schmidt kannst du prüfen?"                │
│  [↩ Antworten]  [❤️ 1]  [✏️ Bearbeiten]          │
│  ─────────────────────────────────────           │
│  ↳ K. Schmidt │ 14:45                           │
│  "Ja, prüfe ich. Brauche Fundament-Berechnung." │
│  ─────────────────────────────────────           │
│  ┌──────────────────────────────┐               │
│  │ Antwort schreiben...  @  📎  │               │
│  └──────────────────────────────┘               │
└──────────────────────────────────────────────────┘
```

**Props:** `entityId: string`, `entityType: 'node'|'document'|'process'`

---

### 19. CollaborationPresence

**Zweck:** Zeigt, welche anderen Benutzer gerade denselben Prozess ansehen.

```
Aktive Benutzer:
[HM] [KS] [TN] + 2 weitere  ← Avatare mit Tooltip (Name, Rolle)
```

**Verhalten:** Echtzeit-Updates via WebSocket. Hover auf Avatar: Name + aktuell betrachteter Bereich.

---

### 20. MentionSelector

**Zweck:** @-Mention-Autocomplete für Kommentar-Eingaben.

```
@Sch...
┌────────────────────────────────┐
│  🔵 K. Schmidt (Sachbearbeiter)│  ← gefilterte Vorschläge
│  🟣 T. Schneider (Supervisor)  │
│  🔵 A. Schumacher (Sachbearb.) │
└────────────────────────────────┘
```

---

### 21. NotificationCenter

**Zweck:** Zentrale Übersicht aller Benachrichtigungen mit Prioritäts-Filterung.

```
🔔 Benachrichtigungen (5 ungelesen)
─────────────────────────────────────
🔴 [14:23] SLA-Verletzung: FALL-0847 (Jetzt handeln!)
⚠️ [13:45] @Mention: K. Schmidt in FALL-0341
🔵 [11:30] Neue Aufgabe zugewiesen: FALL-0901
🔵 [09:15] Nachforderung beantwortet: FALL-0512
✅ [08:47] FALL-0298 abgeschlossen
─────────────────────────────────────
[Alle als gelesen markieren]  [Einstellungen]
```

---

## 🧭 Navigation & Layout-Komponenten

### 22. AdaptiveSidebar

**Zweck:** Hauptnavigation, kollabierbar, responsive.

**Verhalten:**
- Desktop: Ausgeklappt (220px) oder Icons-only (56px)
- Tablet: Standard Icons-only, Hover-Expand
- Mobile: Versteckt, per Hamburger-Button öffnen

---

### 23. ContextualToolbar

**Zweck:** Kontextsensitive Aktions-Leiste oberhalb des Hauptinhalts.

```
[← Zurück]  [FALL-0847 – Baugenehmigung Musterstraße]  [⭐][🔔][⚙️][...]
```

**Verhalten:** Breadcrumb + häufigste Aktionen für aktuellen Kontext.

---

### 24. ResizablePanelLayout

**Zweck:** Drei-Spalten-Layout mit Drag-Resize zwischen Panels.

**Verhalten:**
- Drag auf Trennlinie → Panel-Breite anpassen
- Doppelklick auf Trennlinie → Panel auf Standardbreite zurücksetzen
- Panel per Button ausblenden (maximiert Mittelpanel)

---

### 25. ViewModeSwitcher

**Zweck:** Umschalten zwischen verschiedenen Ansichtsmodi (Kanban, Liste, Gantt, Karte).

```
[📊 Kanban] [≡ Liste] [📅 Gantt] [🗺️ Karte]
```

---

## 🔧 Utility-Komponenten

### 26. DocumentPreview

**Zweck:** Vorschau von Anhängen (PDF, DXF, Bilder) direkt im DMS ohne externen Viewer.

```
┌──────────────────────────────────────────────────┐
│  📄 Lageplan.pdf  │  [Vollbild] [Download] [×]   │
│  ─────────────────────────────────────────────── │
│                                                  │
│  [PDF-Viewer integriert]                         │
│  Seite 1 von 3  [← →]  Zoom: [75% ▾]            │
└──────────────────────────────────────────────────┘
```

---

### 27. GeoMiniMap

**Zweck:** Kleine Geo-Karte im Kontext-Panel zur Verortung des Antragsobjekts.

```
┌────────────────────────┐
│  [OpenStreetMap Tile]  │
│         📍             │  ← Markierung des Objekts
│                        │
│  [Vollansicht öffnen]  │
└────────────────────────┘
```

**Props:** `coordinates: [lon, lat]`, `zoom: number`, `showRadius: boolean`

---

### 28. BatchProgressIndicator

**Zweck:** Fortschrittsanzeige für laufende Batch-Operationen.

```
📦 Archivierung läuft...
████████████████░░░░░░░░  224/247  (91%)
Verbleibend: ~2 Minuten
[⏸ Pause]  [⏹ Abbrechen]  [Details]
```

---

## ♿ Accessibility-Anforderungen

### WCAG 2.1 AA – Globale Anforderungen

| Kriterium | Anforderung | Umsetzung |
|-----------|------------|-----------|
| **1.1.1 Non-text Content** | Alle Bilder/Icons haben Alt-Text | `aria-label` an allen Icons |
| **1.3.1 Info & Relationships** | Semantisches HTML | `<nav>`, `<main>`, `<article>`, `<aside>` |
| **1.4.1 Use of Color** | Farbe nicht alleiniger Informationsträger | Symbol + Farbe immer kombiniert |
| **1.4.3 Contrast** | Kontrast ≥ 4.5:1 | Design-Token enforced |
| **2.1.1 Keyboard** | Alle Funktionen per Tastatur | `tabindex`, `onKeyDown` |
| **2.4.3 Focus Order** | Logische Tab-Reihenfolge | DOM-Reihenfolge = visuelle Reihenfolge |
| **2.4.7 Focus Visible** | Sichtbarer Fokus-Indikator | `focus-visible` CSS |
| **3.3.1 Error Identification** | Fehler klar benannt | Inline-Fehlermeldungen mit Text |
| **4.1.2 Name, Role, Value** | ARIA-Attribute | Alle interaktiven Elemente korrekt annotiert |

### Graph-spezifische Accessibility

Da Graphen für Screen-Reader herausfordernd sind, bietet das DMS:
1. **Tabellarische Alternative:** Graph-Daten als sortierbare Tabelle abrufbar
2. **Verbalisierte Beschreibung:** "Prozess hat 4 Schritte, aktuell bei Schritt 2 (Sachprüfung)"
3. **Keyboard-Navigation:** Tab navigiert durch Knoten, Arrow-Keys folgen Kanten
4. **High-Contrast-Modus:** System-Einstellung wird respektiert (`prefers-contrast`)

---

*Siehe auch:*
- [`docs/de/design/SCREEN_LAYOUTS.md`](SCREEN_LAYOUTS.md) – Layouts mit Komponenten-Verwendung
- [`docs/de/design/WORKFLOW_INTERACTION_PATTERNS.md`](WORKFLOW_INTERACTION_PATTERNS.md) – Interaktionsmuster
