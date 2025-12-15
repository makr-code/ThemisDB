# ThemisDB Document Manager

Ein modernes Dokumentenverwaltungssystem (DMS) als Show-Frontend für ThemisDB mit vollständiger Integration aller Multi-Model-Funktionalitäten.

## Konzept

Das ThemisDB Document Manager ist ein professionelles Dokumentenverwaltungssystem, inspiriert von PDV VIS5, das die leistungsstarken Multi-Model-Fähigkeiten von ThemisDB demonstriert:

- **Geo-Integration**: Kartenbasierte Dokumentensuche und Location-Tracking
- **Timeline-Ansicht**: Chronologische Navigation und Zeitreihen-Visualisierung  
- **Vector Search**: Semantische Ähnlichkeitssuche über Dokumente
- **Graph-Visualisierung**: Dokumentbeziehungen und Abhängigkeiten

## Besondere Features

### 🔧 Nahtlose Office-Integration

Das System integriert direkt mit Microsoft Office-Anwendungen über COM Interop:

- **Word**: Erstellen und Bearbeiten von Textdokumenten mit aktivierter Änderungsverfolgung
- **Excel**: Arbeitsmappen mit automatischer Versionskontrolle
- **PowerPoint**: Präsentationen mit Revisionstracking
- **Outlook**: E-Mail-Integration für dokumentbezogene Kommunikation
- **OneNote**: Notizen und Dokumentation

### 📝 Revisionsichere Verarbeitung

Alle Dokumente werden automatisch versioniert:

- **Automatische Revisionserstellung**: Jede Änderung wird als neue Revision gespeichert
- **Vollständige Audit-Trail**: Wer hat wann was geändert
- **Track Changes**: Automatische Aktivierung der Office-Änderungsverfolgung
- **SHA256-Hashing**: Integritätsprüfung jeder Revision
- **Restore-Funktionalität**: Rückkehr zu früheren Versionen
- **Vergleichsfunktion**: Unterschiede zwischen Revisionen anzeigen

### 🔒 Compliance-Features

- DSGVO-konforme Dokumentenverwaltung
- Vollständige Nachverfolgbarkeit aller Änderungen
- Unveränderliche Revisionshistorie in ThemisDB
- Automatische Metadaten-Erfassung

### 📊 Intelligentes Metadaten-System (NEU!)

Das brandneue Metadaten-System bietet professionelle Verwaltung von Dokumentmetadaten:

- **Gruppierte Ansicht**: Metadaten werden in logische Kategorien organisiert (Vorgang, Status, Organisation, Zeit, Schlagwörter)
- **Editierbare Felder**: Direkte Bearbeitung mit verschiedenen Eingabetypen (Text, Date, Number, Boolean, **Dropdown**)
- **Dropdown-Listen**: Vordefinierte Auswahlmöglichkeiten für Status, Priorität, Vorgangsart etc.
- **Automatische Validierung**: Erkennung und Warnung bei fehlenden Pflichtfeldern
- **YAML-Konfiguration**: Flexibles Layout über `Config/metadata_layout.yaml`
- **Smart Hiding**: Automatisches Verstecken leerer Felder für bessere Übersicht
- **Persistierung**: Speichern/Laden mit intelligentem Caching
- **Finalisierung**: Dokumente gegen weitere Änderungen sperren
- **Keyboard Shortcuts**: Schneller Zugriff (Ctrl+S speichern, F5 neu laden, Ctrl+F finalisieren)

**Quick Start**: Siehe [QUICKSTART.md](QUICKSTART.md)  
**Vollständige Doku**: Siehe [METADATA_GUIDE.md](METADATA_GUIDE.md)

### 🗂️ ERM/ERD Visualisierung (NEU!)

Das Entity-Relationship-Diagramm-System bietet professionelle Datenbankschema-Visualisierung:

- **Interaktive Diagramme**: Visuelle Darstellung aller Entitäten und Beziehungen
- **Schema-Introspection**: Automatisches Laden des Datenbankschemas aus ThemisDB
- **Entitäts-Details**: Anzeige aller Attribute mit Datentypen und Constraints
- **Zoom & Pan**: Intuitive Navigation durch große Diagramme
- **Auto-Layout**: Automatische Anordnung der Entitäten für optimale Übersicht
- **Beziehungs-Typen**: OneToOne, OneToMany, ManyToOne, ManyToMany
- **Farbcodierung**: Primärschlüssel und Indizes werden hervorgehoben

**Verwendung**: Navigieren Sie zum Tab "🗂️ ERD" im Hauptfenster

### 🔍 Query Editor (NEU!)

Der professionelle Query Editor ermöglicht direkten Zugriff auf ThemisDB:

- **AQL-Unterstützung**: Vollständige Unterstützung für ArangoDB Query Language
- **Syntax-Validierung**: Echtzeit-Validierung der Query-Syntax
- **Gespeicherte Queries**: Speichern und Wiederverwenden häufig genutzter Abfragen
- **Beispiel-Queries**: Vorkonfigurierte Queries für häufige Anwendungsfälle
- **Ergebnis-Anzeige**: Tabellarische Darstellung der Query-Ergebnisse
- **Performance-Metriken**: Anzeige von Ausführungszeit und Anzahl der Zeilen
- **Fehlerbehandlung**: Detaillierte Fehlermeldungen bei ungültigen Queries
- **Query-Formatierung**: Automatische Formatierung für bessere Lesbarkeit

**Verwendung**: Navigieren Sie zum Tab "🔍 Query" im Hauptfenster

**Beispiel AQL-Query**:
```aql
FOR doc IN documents
  FILTER doc.created_at >= DATE_SUBTRACT(DATE_NOW(), 7, 'days')
  SORT doc.created_at DESC
  LIMIT 10
  RETURN doc
```

**Quick Start**: Siehe [QUICKSTART.md](QUICKSTART.md)  
**Vollständige Doku**: Siehe [METADATA_GUIDE.md](METADATA_GUIDE.md)

## Architektur

### Technologie-Stack

- **.NET 8.0**: Moderne .NET-Plattform
- **WPF**: Windows Presentation Foundation für Desktop-UI
- **MVVM Pattern**: Saubere Trennung von UI und Business Logic
- **ModernWPF**: Modernes UI-Framework
- **CommunityToolkit.Mvvm**: MVVM-Helpers und Commands
- **COM Interop**: Direkte Office-Integration

### Komponenten

```
Themis.DocumentManager/
├── Models/              # Domain-Modelle
│   ├── Document.cs
│   ├── DocumentRevision.cs
│   └── ...
├── Services/            # Business Logic
│   ├── ThemisApiClient.cs
│   ├── DocumentService.cs
│   ├── OfficeIntegrationService.cs
│   ├── RevisionService.cs
│   └── ...
├── ViewModels/          # MVVM ViewModels
│   ├── MainViewModel.cs
│   ├── DocumentBrowserViewModel.cs
│   └── ...
├── Views/               # WPF Views
│   ├── MainWindow.xaml
│   ├── DocumentBrowserView.xaml
│   └── ...
├── Controls/            # Wiederverwendbare Controls
├── Styles/              # UI-Styles und Themes
└── Resources/           # Icons, Bilder, etc.
```

## Installation

### Voraussetzungen

- Windows 10/11
- .NET 8.0 SDK
- Microsoft Office (Word, Excel, PowerPoint, Outlook, OneNote)
- ThemisDB Server (läuft auf http://localhost:8765)

### Build

```powershell
cd tools/Themis.DocumentManager
dotnet restore
dotnet build
```

### Run

```powershell
dotnet run
```

## Verwendung

### Dokumenten-Browser

Der Hauptbereich zeigt alle in ThemisDB gespeicherten Dokumente. Klicken Sie auf ein Dokument für Details.

### Office-Integration

Verwenden Sie die Buttons in der Sidebar, um neue Office-Dokumente zu erstellen:

1. **📝 New Word Document**: Erstellt ein neues Word-Dokument
2. **📊 New Excel Workbook**: Erstellt eine neue Excel-Arbeitsmappe
3. **📧 New Outlook Email**: Öffnet ein neues E-Mail-Fenster
4. **🎯 New PowerPoint**: Erstellt eine neue Präsentation
5. **📓 New OneNote Page**: Erstellt eine neue OneNote-Seite

Alle erstellten Dokumente werden automatisch:
- In ThemisDB registriert
- Mit Metadaten versehen
- Für Revisionstracking vorbereitet
- In einem Timeline-Event erfasst

### Revisionsverwaltung

Jedes Mal, wenn Sie ein Dokument speichern:

1. Eine neue Revision wird automatisch erstellt
2. Der Datei-Hash wird berechnet
3. Ein Timeline-Event wird generiert
4. Die alte Version bleibt erhalten

### Multi-Model-Features

#### Geo-Suche
Finden Sie Dokumente basierend auf geografischen Koordinaten oder Regionen.

#### Timeline
Visualisieren Sie Dokumente und Ereignisse chronologisch.

#### Vector Search
Nutzen Sie semantische Ähnlichkeit, um verwandte Dokumente zu finden.

#### Graph-View
Erkunden Sie Dokumentbeziehungen und Abhängigkeiten visuell.

## API-Integration

Das System kommuniziert mit ThemisDB über die REST API:

- **Documents**: `/entities/documents:{id}`
- **Revisions**: `/entities/document_revisions:{id}`
- **Timeline**: `/entities/timeline_events:{id}`
- **Search**: `/query/aql`
- **Vector**: `/vector/search`
- **Graph**: `/graph/traverse`

## Konfiguration

Die ThemisDB-Server-URL kann in `ThemisApiClient.cs` konfiguriert werden (Standard: `http://localhost:8765`).

## Weiterentwicklung

### Neu implementiert ✅

#### v1.2.0 - 15. Dezember 2025
- [x] **ERM/ERD Visualisierung** - Interaktive Entity-Relationship-Diagramme zur Datenbankschema-Visualisierung
- [x] **Query Editor** - Professioneller AQL-Query-Editor mit Syntaxvalidierung und Ergebnisanzeige
- [x] **Schema-Introspection** - Automatisches Laden und Anzeigen des Datenbankschemas
- [x] **Gespeicherte Abfragen** - Speichern und Wiederverwenden häufig genutzter Queries

#### v1.1.0 - 11. Dezember 2025
- [x] **Dropdown-Felder** - ComboBox-Support für Auswahlfelder mit YAML-Konfiguration

#### v1.0.0 - 11. Dezember 2025
- [x] **Metadaten-System** - Vollständig editierbare, gruppierte Metadatenverwaltung
- [x] **YAML-Konfiguration** - Flexibles Layout-System
- [x] **Validierung** - Automatische Pflichtfeld-Prüfung
- [x] **Persistierung** - Service mit Caching und Finalisierung
- [x] **Keyboard Shortcuts** - Ctrl+S, F5, Ctrl+F
- [x] **Resizable Sidebars** - Anpassbare UI-Layout

### Geplante Features

- [ ] Batch-Upload von Dokumenten
- [ ] Erweiterte Suchfilter (Faceted Search)
- [ ] Export nach PDF/Archive
- [ ] Dokumenten-Templates
- [ ] OCR-Integration
- [ ] Automatische Klassifizierung
- [ ] Collaborative Editing
- [ ] Mobile App (Xamarin)
- [ ] ERD-Export (PNG, SVG, GraphML)
- [ ] Visual Query Builder (Drag & Drop)
- [ ] RichText-Editor für Metadaten
- [ ] Undo/Redo für Metadaten-Änderungen

### Erweiterungspunkte

- Custom Document Processors
- Additional Office Apps (Visio, Project, Publisher)
- Integration mit SharePoint
- WebDAV-Unterstützung
- S3/Azure Blob Storage Backend

## Sicherheit

- Alle Dokumente werden lokal in `%USERPROFILE%\Documents\ThemisDB\Documents` gespeichert
- Revisionen werden in Unterordner `Revisions/{DocumentId}/` abgelegt
- SHA256-Hashing zur Integritätsprüfung
- Office Track Changes für Änderungsverfolgung
- Vollständige Audit-Logs in ThemisDB

## Lizenz

Teil des ThemisDB-Projekts, siehe Haupt-LICENSE-Datei.

## Support

Bei Fragen oder Problemen erstellen Sie bitte ein Issue im ThemisDB-Repository.
