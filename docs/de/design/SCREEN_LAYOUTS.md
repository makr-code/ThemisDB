# Screen Layouts – Themis.DocumentManager

**Kategorie:** 🎨 UX-Design  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht & Design-Prinzipien](#-übersicht--design-prinzipien)
- [Layout 1: Process Execution](#-layout-1-process-execution)
- [Layout 2: Process Portfolio](#-layout-2-process-portfolio)
- [Layout 3: Graph Explorer](#-layout-3-graph-explorer)
- [Layout 4: Batch Operations](#-layout-4-batch-operations)
- [Tablet-Layouts](#-tablet-layouts)
- [Mobile-Layouts](#-mobile-layouts)
- [Responsive Breakpoints](#-responsive-breakpoints)
- [Navigation & Routing](#-navigation--routing)

---

## 🎯 Übersicht & Design-Prinzipien

### Design-Prinzipien

| Prinzip | Umsetzung |
|---------|-----------|
| **Progressive Disclosure** | Komplexe Details erst auf Anfrage anzeigen |
| **Kontextsensitivität** | UI passt sich Prozess-Status und Benutzerrolle an |
| **Information Hierarchy** | Wichtigstes zuerst, Details per Drill-Down |
| **Konsistenz** | Gleiche Widgets, gleiche Interaktionsmuster überall |
| **Barrierefreiheit** | WCAG 2.1 AA, Keyboard-Navigation, Screen-Reader |
| **Performance** | Virtualisierte Listen, lazy Loading für Graph-Nodes |

### Farb-Schema (Status-Kodierung)

```
GRÜN    (#2ECC71) – Abgeschlossen, On-Track, Genehmigt
BLAU    (#3498DB) – Aktiv, In Bearbeitung
GELB    (#F1C40F) – Warnung, Risiko, Frist nähert sich
ROT     (#E74C3C) – Überfällig, Abgelehnt, Fehler
GRAU    (#95A5A6) – Inaktiv, Warten, Archiviert
LILA    (#9B59B6) – KI-Empfehlung, Insight
```

---

## 📋 Layout 1: Process Execution

**Zweck:** Fokussierte Bearbeitung eines einzelnen Verwaltungsprozesses  
**Primäre Nutzer:** Sachbearbeiter  
**Breakpoint:** Desktop (≥ 1280px)

### ASCII-Mockup

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  🏛️ Themis DMS  │  FALL-2026-0847 – Baugenehmigung Musterstraße 12          ║
║  ──────────────────────────────────────────────────────────────────────────  ║
║  [← Zurück]  Status: 🔵 IN BEARBEITUNG  │  Frist: ⚠️ 3 Tage  │  [Eskalieren]  ║
╠═══════════════╦══════════════════════════════╦═══════════════════════════════╣
║               ║                              ║                               ║
║  📂 KONTEXT   ║   🔄 PROZESS-GRAPH           ║   📋 AUFGABEN-PANEL           ║
║  ─────────    ║   ──────────────────         ║   ──────────────────          ║
║  Antragsteller║                              ║   Aktuelle Aufgabe:           ║
║  Max Muster   ║   [START]                    ║   "Unterlagen prüfen"         ║
║  Musterstr.12 ║      ↓                       ║   ─────────────────────       ║
║               ║   [EINGANG] ✅               ║   □ Lageplan vorhanden        ║
║  Objekttyp:   ║      ↓                       ║   ✓ Bauzeichnung vorhanden    ║
║  Neubau EFH   ║   [VOLLST.PRÜFG.] 🔵 ← Hier ║   □ Statiknachweis FEHLT ⚠️   ║
║               ║      ↓              aktuell  ║   □ Umweltgutachten           ║
║  Eingereicht: ║   [FACHPRÜFUNG]              ║   ─────────────────────       ║
║  15.01.2026   ║    ↙          ↘              ║   [✅ Abschließen]             ║
║               ║ [STATIK]    [UMWELT]         ║   [❗ Nachforderung]           ║
║  Aktenzeichen:║    ↘          ↙              ║   [→ Weiterleiten]            ║
║  BG-2026-0847 ║   [ENTSCHEIDG.]              ║                               ║
║               ║      ↓                       ║   💬 KOMMENTARE               ║
║  🤖 KI-SCORE  ║   [ARCHIV]                   ║   ──────────────              ║
║  ─────────    ║                              ║   14:23 – H.Müller:           ║
║  Genehmigungs-║   [Timeline-Ansicht]         ║   "Statiknachweis anfordern"  ║
║  wahrsch.:    ║   [Zoom: - ○ +]              ║   [Antworten] [@-Mention]     ║
║  78% ████░    ║                              ║   ────────────────────        ║
║               ║                              ║   [Kommentar schreiben...]    ║
║  Risiken:     ║                              ║                               ║
║  • Lärmschutz ║                              ║   🤖 KI-INSIGHTS              ║
║  • Grenzabst. ║                              ║   ──────────────              ║
║               ║                              ║   Ähnliche Fälle: 3 ✓         ║
║  [Ähn. Fälle] ║                              ║   Empfehlung: Abschließen     ║
║               ║                              ║   (95% Konfidenz)             ║
╠═══════════════╩══════════════════════════════╩═══════════════════════════════╣
║  📎 DOKUMENTE: [Antrag.pdf]  [Lageplan.pdf]  [Bauzeichnung.dwg]  [+Hinzuf.]  ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### Panel-Beschreibung

#### Linkes Panel – Kontext (280px)
- **Antragsteller-Info:** Name, Adresse, Kontakt
- **Prozess-Metadaten:** Typ, Objektkategorie, Eingangsdatum, Aktenzeichen
- **Geo-Minimap:** Kleines Kartenausschnitt mit markiertem Objekt
- **KI-Score-Widget:** Genehmigungswahrscheinlichkeit + Top-3-Risiken
- **Ähnliche Fälle:** Links zu vergleichbaren abgeschlossenen Fällen

#### Mittleres Panel – Prozess-Graph (variabel, ~60%)
- **BPMN-ähnliche Graph-Visualisierung:** Knoten + Kanten
- **Status-Farben:** Grün=abgeschlossen, Blau=aktuell, Grau=ausstehend, Rot=blockiert
- **Aktueller Knoten hervorgehoben** mit Pulsiereffekt
- **Timeline-Ansicht umschaltbar** (horizontal, Zeitachse)
- **Zoom:** Maus-Scroll oder +-Buttons

#### Rechtes Panel – Aufgaben (320px)
- **Aktuelle Aufgabe:** Checkliste der notwendigen Schritte
- **Aktions-Buttons:** Abschließen, Nachfordern, Weiterleiten, Eskalieren
- **Kommentar-Thread:** @-Mentions, Antworten, Zeitstempel
- **KI-Insights:** Empfehlungen, Ähnlichkeiten, Risikohinweise

#### Untere Leiste – Dokumente
- **Angehängte Dokumente:** Klickbare Vorschauen
- **Dokument hinzufügen:** Drag-Drop-Zone oder Dateiauswahl

---

## 📊 Layout 2: Process Portfolio

**Zweck:** Übersicht über mehrere Prozesse gleichzeitig  
**Primäre Nutzer:** Supervisoren, Sachbearbeiter (Tagesübersicht)  
**Breakpoint:** Desktop (≥ 1280px)

### ASCII-Mockup

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  🏛️ Themis DMS  │  📊 Portfolio – Mein Team                                 ║
║  ─────────────────────────────────────────────────────────────────────────  ║
║  [🔴 Überfällig: 3] [⚠️ Risiko: 7] [🔵 Aktiv: 34] [✅ Heute fertig: 2]     ║
╠═════════════════════╦════════════════════════════════════════════════════════╣
║  🔍 FILTER          ║  PROZESS-LISTE (KANBAN-STIL)                           ║
║  ─────────────────  ║  ─────────────────────────────────────────────────    ║
║  Prozesstyp:        ║                                                        ║
║  [Alle ▾]           ║  WARTEND (5)    IN BEARBEITUNG (27)    REVIEW (9)      ║
║                     ║  ─────────────  ──────────────────     ──────────     ║
║  Sachbearbeiter:    ║  ┌──────────┐  ┌──────────┐           ┌──────────┐   ║
║  [H.Müller ▾]       ║  │FALL-0341 │  │FALL-0847 │🔴 3T      │FALL-0612 │   ║
║                     ║  │Baugenehm.│  │Baugenehm.│           │Wohnraum  │   ║
║  SLA:               ║  │Warten auf│  │H.Müller  │           │K.Schmidt │   ║
║  [Alle ▾]           ║  │Gutachten │  │Statik? ⚠️│           │→ Freigabe│   ║
║                     ║  └──────────┘  └──────────┘           └──────────┘   ║
║  Zeitraum:          ║  ┌──────────┐  ┌──────────┐           ┌──────────┐   ║
║  [Diese Woche ▾]    ║  │FALL-0298 │  │FALL-0901 │           │FALL-0733 │   ║
║                     ║  │Gewerbe   │  │Gewerbe   │           │Teilung   │   ║
║  Ansicht:           ║  │Externe   │  │T.Schneider│          │A.Weber   │   ║
║  ○ Kanban           ║  │Genehmig. │  │Lärmschutz│           │Abnahme   │   ║
║  ○ Liste            ║  └──────────┘  └──────────┘           └──────────┘   ║
║  ○ Gantt            ║                                                        ║
║  ○ Karte            ║  ─────── SLA-LEISTE ──────────────────────────────    ║
║                     ║  FALL-0847: ████████░░░░ 67% │ Frist: 3 Tage ⚠️       ║
║  📈 Statistik:      ║  FALL-0341: ██████████░░ 83% │ Frist: 8 Tage          ║
║  Ø Bearb.: 42 Tage  ║  FALL-0901: ██████░░░░░░ 51% │ Frist: 12 Tage        ║
║  SLA-Quote: 91%     ║                                                        ║
║  Offen: 47          ║  [Gantt-Ansicht umschalten]  [Export]  [Bericht]      ║
╚═════════════════════╩════════════════════════════════════════════════════════╝
```

### Ansichts-Modi

| Modus | Beschreibung | Best für |
|-------|-------------|----------|
| **Kanban** | Spalten nach Status, Drag-Drop zwischen Spalten | Tagesplanung |
| **Liste** | Tabellarisch, sortierbar, filterbar | Massenübersicht |
| **Gantt** | Zeitachse mit Balken, Fristen sichtbar | SLA-Monitoring |
| **Karte** | Geo-Karte mit Prozess-Pins | Räumliche Verteilung |

---

## 🕸️ Layout 3: Graph Explorer

**Zweck:** Erkundung komplexer Abhängigkeiten zwischen Prozessen  
**Primäre Nutzer:** Analysten, Prozessoptimierung  
**Breakpoint:** Desktop (≥ 1440px empfohlen)

### ASCII-Mockup

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  🏛️ Themis DMS  │  🕸️ Graph Explorer                                        ║
║  ─────────────────────────────────────────────────────────────────────────  ║
║  Filter: [Typ ▾] [Status ▾] [Zeitraum ▾] [Sachbearb. ▾]  [Query eingeben]  ║
╠════════════╦═══════════════════════════════════════════════╦════════════════╣
║  📊 LEGENDE║                                               ║  🔍 DETAILS    ║
║  ──────    ║         GRAPH-CANVAS (interaktiv)             ║  ────────────  ║
║  🟢 Fertig ║                                               ║  [Klick auf    ║
║  🔵 Aktiv  ║    ●──────●           ●                       ║  Node für      ║
║  🔴 Fällig ║    │   Baugen.│     │Umwelt│                  ║  Details]      ║
║  ⚪ Warten ║    │   2026-01│     │2026-02│                 ║                ║
║  🟣 KI-Emp.║    └──┬───────┘     └───┬──┘                 ║  Ausgewählt:   ║
║            ║       │                 │                     ║  FALL-2026-003 ║
║  Kanten:   ║       └────────┬────────┘                     ║                ║
║  ── Abh.   ║                │                              ║  Typ: Baugen.  ║
║  ·· Refer. ║           ●────┘                              ║  Status: Aktiv ║
║  == Block  ║         │Entsch.│                             ║  Frist: 5T ⚠️  ║
║            ║         │2026-03│                             ║  Sacherb.: A.W.║
║  🔭 ZOOM   ║                                               ║                ║
║  [- ○○ +]  ║      ●    ●    ●    ●    ●                   ║  Abhäng. von:  ║
║  [Fit All] ║      │Gewerbe-Cluster (12 Fälle)│             ║  FALL-2026-001 ║
║            ║      (Cluster zusammengefasst)                ║  FALL-2026-002 ║
║  🔍 SUCHE  ║                                               ║                ║
║  [Knoten   ║  [Physik-Layout]  [Hierarchisch]  [Kreis]    ║  [Öffnen]      ║
║  suchen..]  ║                                               ║  [Graph Trace] ║
║  [Pfad     ║  Cluster: ●────● (Zoom für Details)          ║  [Pfad zu...]  ║
║  suchen..]  ║                                               ║                ║
║            ║  🤖 KI: "18er-Cluster = Lärmschutz-Muster   ║  📊 Statistik  ║
║  LAYOUT:   ║       in Bezirk Nord (Anomalie erkannt)"     ║  Knoten: 247   ║
║  ○ Physik  ║                                               ║  Kanten: 891   ║
║  ○ Hierarch║  [Screenshot]  [Export SVG]  [AQL-Analyse]   ║  Cluster: 12   ║
╚════════════╩═══════════════════════════════════════════════╩════════════════╝
```

### Graph-Interaktionen

| Interaktion | Aktion | Ergebnis |
|-------------|--------|---------|
| **Klick auf Knoten** | Einzelauswahl | Details im rechten Panel |
| **Doppelklick** | Öffnen | Wechselt zu Layout 1 (Process Execution) |
| **Rechtsklick** | Kontextmenü | Graph-Aktionen (Pfad suchen, Trace, etc.) |
| **Strg+Klick** | Mehrfachauswahl | Vergleich mehrerer Knoten |
| **Scroll** | Zoom | Zoom in/out |
| **Drag** | Panning | Graph verschieben |
| **Drag auf Knoten** | Layout-Anpassung | Knoten manuell positionieren |

---

## ⚡ Layout 4: Batch Operations

**Zweck:** Massenverarbeitung mehrerer Prozesse gleichzeitig  
**Primäre Nutzer:** Administratoren, Supervisoren  
**Breakpoint:** Desktop (≥ 1280px)

### ASCII-Mockup

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  🏛️ Themis DMS  │  ⚡ Batch Operations                                       ║
║  ─────────────────────────────────────────────────────────────────────────  ║
║  ⚠️ Achtung: Batch-Aktionen können nicht rückgängig gemacht werden           ║
╠══════════════════════════════╦═════════════════════════════════════════════╣
║  🎯 AUSWAHL-KRITERIEN        ║  📋 VORSCHAU (247 ausgewählt)               ║
║  ──────────────────────────  ║  ───────────────────────────────────────    ║
║  Prozesstyp:                 ║  □ Alle  [Seite 1 von 25]  [50 pro Seite]  ║
║  [✓] Baugenehmigung          ║                                             ║
║  [✓] Wohnraumgenehmigung     ║  ✓ FALL-2024-0001  Abgeschl. 12.01.2024   ║
║  [ ] Gewerbeerlaubnis        ║  ✓ FALL-2024-0002  Abgeschl. 14.01.2024   ║
║                              ║  ✓ FALL-2024-0003  Abgeschl. 18.01.2024   ║
║  Status:                     ║    FALL-2024-0004  ⚠️ Offener Folgefall    ║
║  [✓] Abgeschlossen           ║  ✓ FALL-2024-0005  Abgeschl. 22.01.2024   ║
║  [ ] Aktiv                   ║  ...                                        ║
║                              ║                                             ║
║  Zeitraum bis:               ║  KI-Warnung:                                ║
║  [01.01.2024   📅]           ║  ⚠️ 23 Fälle haben Folgeprozesse           ║
║                              ║  [23 ausschließen] [Trotzdem einschließen]  ║
║  [🔍 Auswahl aktualisieren]  ║                                             ║
║                              ║  Speicherbedarf: 4.7 GB → ~1.3 GB (72% ↓) ║
║  ⚡ BATCH-AKTION:             ║                                             ║
║  ○ 📦 Archivieren            ║  ─────────────────────────────────────      ║
║  ○ 🔒 Einfrieren             ║  [◀ Zurück]  [Vorschau]  [▶ Ausführen]     ║
║  ○ 📤 Exportieren            ║                                             ║
║  ○ 🔄 Reassign               ║  FORTSCHRITT (nach Start):                  ║
║  ○ 🗑️ Löschen (Admin)        ║  ████████████░░░░░░░░  224/247 (91%)       ║
║                              ║  Verbleibend: ca. 2 Minuten                 ║
║  [▶ Aktion starten]          ║  [⏸ Pause]  [⏹ Abbrechen]                  ║
╚══════════════════════════════╩═════════════════════════════════════════════╝
```

---

## 📱 Tablet-Layouts

**Breakpoint:** 768px – 1279px  
**Strategie:** Vereinfachte Zwei-Spalten-Layouts, Navigation per Tab-Bar

### Tablet: Process Execution

```
╔════════════════════════════════════════════╗
║  ← FALL-2026-0847  │  Status: 🔵 Aktiv     ║
║  ─────────────────────────────────────     ║
║  [Graph] [Dokumente] [Aufgaben] [Komm.]   ║  ← Tab-Navigation
╠════════════════════════════════════════════╣
║                                            ║
║   PROZESS-GRAPH (aktiver Tab)              ║
║                                            ║
║   [START] → [EINGANG ✅] → [PRÜFUNG 🔵]   ║
║                   ↓                        ║
║          [FACHPRÜFUNG]                     ║
║           ↙         ↘                     ║
║      [STATIK]    [UMWELT]                  ║
║           ↘         ↙                     ║
║          [ENTSCHEIDUNG]                    ║
║               ↓                            ║
║           [ARCHIV]                         ║
║                                            ║
╠════════════════════════════════════════════╣
║  ⚠️ Frist: 3 Tage  │  [Abschließen] [Mehr] ║
╚════════════════════════════════════════════╝
```

### Tablet: Portfolio (Kompakt)

```
╔════════════════════════════════════════════╗
║  📊 Portfolio  │  🔴 3  ⚠️ 7  🔵 34       ║
║  ──────────────────────────────────────    ║
║  [Liste ▾]  [Filter ▾]  [Sortieren ▾]     ║
╠════════════════════════════════════════════╣
║  🔴 FALL-0847  Baugen.  Frist: 3T  H.M.   ║
║  🔴 FALL-0341  Baugen.  Frist: 1T  K.S.   ║
║  ⚠️ FALL-0901  Gewerbe  Frist: 8T  T.N.   ║
║  🔵 FALL-0612  Wohnrm.  Frist:12T  A.W.   ║
║  ...                                       ║
║                                            ║
║  [← Vorherige]        [Nächste →]         ║
╚════════════════════════════════════════════╝
```

---

## 📱 Mobile-Layouts

**Breakpoint:** < 768px  
**Strategie:** Single-Column, Task-fokussiert, minimale Komplexität  
**Primäre Funktion:** Schnelle Statuschecks und einfache Aktionen

### Mobile: Meine Aufgaben

```
╔══════════════════════════╗
║  🏛️ ThemisDMS  ☰  🔔3   ║
╠══════════════════════════╣
║  Guten Morgen, H.Müller  ║
║  3 Aufgaben heute        ║
╠══════════════════════════╣
║  🔴 FALL-0847            ║
║  Baugenehmigung          ║
║  ⚠️ Frist heute!          ║
║  [Öffnen →]              ║
╠══════════════════════════╣
║  🔵 FALL-0341            ║
║  Baugenehmigung          ║
║  Frist: morgen           ║
║  [Öffnen →]              ║
╠══════════════════════════╣
║  🔵 FALL-0512            ║
║  Wohnraumgenehmigung     ║
║  Frist: 5 Tage           ║
║  [Öffnen →]              ║
╠══════════════════════════╣
║  [Alle 47 Fälle anzeigen]║
╚══════════════════════════╝
```

### Mobile: Prozess-Detail (vereinfacht)

```
╔══════════════════════════╗
║  ← FALL-2026-0847        ║
║  Baugenehmigung          ║
╠══════════════════════════╣
║  Status: 🔵 In Bearbeitg.║
║  Frist: ⚠️ 3 Tage        ║
║  Sachbearbeiter: H.Müller║
╠══════════════════════════╣
║  Aktueller Schritt:      ║
║  📋 Unterlagen prüfen    ║
║  ─────────────────────   ║
║  [✅ Abschließen]         ║
║  [❗ Nachforderung]       ║
║  [📞 Anrufen]             ║
╠══════════════════════════╣
║  💬 Letzter Kommentar:   ║
║  "Statiknachweis fehlt"  ║
║  H.Müller, 14:23         ║
║  [Antworten]             ║
╚══════════════════════════╝
```

---

## 📐 Responsive Breakpoints

| Breakpoint | Breite | Layout-Strategie | Haupteinschränkungen |
|-----------|--------|-----------------|----------------------|
| **XS** (Mobile) | < 480px | Single Column, Tab-Nav | Kein Graph-Explorer |
| **SM** (Mobile L) | 480–767px | Single Column | Vereinfachter Graph |
| **MD** (Tablet) | 768–1023px | 2 Spalten, Tabs | Graph ohne Sidebar |
| **LG** (Desktop) | 1024–1279px | 3 Spalten (schmal) | Alle Features |
| **XL** (Desktop L) | 1280–1535px | 3 Spalten (standard) | Alle Features |
| **2XL** (Wide) | ≥ 1536px | 3+ Spalten (erweitert) | Alle Features + Extra-Panels |

### Adaptive Komponenten

```javascript
// Pseudocode: Adaptive Panel-Strategie
if (viewport.width < 768) {
  // Mobile: Nur Tab-Navigation, kein Split-View
  renderLayout('single-column', { navigation: 'tab-bar' });
} else if (viewport.width < 1280) {
  // Tablet: Zwei Spalten, Tab-Navigation für drittes Panel
  renderLayout('two-column', { thirdPanel: 'tab-overlay' });
} else {
  // Desktop: Vollständiges 3-Spalten-Layout
  renderLayout('three-column', { allPanels: 'visible' });
}
```

### CSS-Breakpoint-Definitionen

```css
/* Themis DMS Breakpoints */
--breakpoint-xs:  480px;
--breakpoint-sm:  768px;
--breakpoint-md: 1024px;
--breakpoint-lg: 1280px;
--breakpoint-xl: 1536px;

/* Layout-Variablen */
--sidebar-width-left:   280px;   /* Kontext-Panel */
--sidebar-width-right:  320px;   /* Aufgaben-Panel */
--main-panel-min-width: 400px;   /* Graph/Hauptinhalt */
--toolbar-height:        56px;
--statusbar-height:      32px;
--document-bar-height:   64px;
```

---

## 🧭 Navigation & Routing

### Hauptnavigation

```
Sidebar (kollabierbar, 56px → 220px):
  🏠  Übersicht / Dashboard
  📋  Meine Aufgaben
  📊  Portfolio / Team
  🕸️  Graph Explorer
  🔍  Suche & KI-Chat
  ⚡  Batch Operations
  ⚙️  Einstellungen
  ❓  Hilfe / Dokumentation
```

### URL-Schema

```
/dms/                              → Dashboard
/dms/tasks/                        → Meine Aufgaben
/dms/portfolio/                    → Portfolio-Übersicht
/dms/portfolio/:teamId             → Team-Portfolio
/dms/process/:processId            → Process Execution View
/dms/process/:processId/graph      → Prozess-Graph-Ansicht
/dms/graph/                        → Global Graph Explorer
/dms/graph/?filter=...             → Graph mit Filter
/dms/search/?q=...                 → Suchergebnisse / KI-Chat
/dms/batch/                        → Batch Operations
```

### Keyboard-Shortcuts

| Shortcut | Aktion |
|---------|--------|
| `Ctrl+K` | Globale Suche / KI-Chat öffnen |
| `Ctrl+P` | Prozess-Schnellwechsel (ähnlich VSCode Ctrl+P) |
| `Alt+1..4` | Zwischen den 4 Layouts wechseln |
| `Escape` | Panel schließen / Dialog abbrechen |
| `Space` | Aktuellen Prozessknoten öffnen (im Graph) |
| `Enter` | Aktion bestätigen |
| `F5` | Ansicht aktualisieren |

---

*Siehe auch:*
- [`docs/de/design/COMPONENTS_AND_WIDGETS.md`](COMPONENTS_AND_WIDGETS.md) – Komponentenbibliothek
- [`docs/de/design/WORKFLOW_INTERACTION_PATTERNS.md`](WORKFLOW_INTERACTION_PATTERNS.md) – User Journeys
