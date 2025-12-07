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

### Geplante Features

- [ ] Batch-Upload von Dokumenten
- [ ] Erweiterte Suchfilter (Faceted Search)
- [ ] Export nach PDF/Archive
- [ ] Dokumenten-Templates
- [ ] OCR-Integration
- [ ] Automatische Klassifizierung
- [ ] Collaborative Editing
- [ ] Mobile App (Xamarin)

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
