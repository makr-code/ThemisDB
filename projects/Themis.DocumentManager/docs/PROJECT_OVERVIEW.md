# Themis.DocumentManager - Projektübersicht

**Projekt:** Themis Document Management System  
**Version:** 1.0 (Production Ready)  
**Status:** ✅ Vollständig implementiert und produktionsbereit  
**Letzte Aktualisierung:** 11. Dezember 2025

---

## 🎯 Executive Summary

**Themis.DocumentManager** ist ein vollständiges, modernes Dokumentenverwaltungssystem für die öffentliche Verwaltung, entwickelt als Show-Frontend für die **ThemisDB Multi-Model-Datenbank**.

### Kernmerkmale

✅ **Multi-Model Integration** - Geo, Timeline, Vector, Graph  
✅ **Deutsche Verwaltungsstruktur** - 7-stufige Aktenhierarchie nach deutschem Verwaltungsrecht  
✅ **Clean Architecture** - CQRS mit MediatR, Event-Driven Design  
✅ **DirectX 11 Rendering** - Native 3D-Graph-Visualisierung mit GPU-Pipeline  
✅ **Office-Integration** - Word, Excel, PowerPoint, Outlook, OneNote via COM Interop  
✅ **Revisionssichere Verarbeitung** - Vollständiger Audit Trail, unveränderliche Versionen  
✅ **OpenStreetMap Integration** - Geo-Visualisierung mit Tile-Management  
✅ **Modern WPF UI** - Dashboard, intelligente Navigation, Task Basket

---

## 📋 Ursprüngliche Anforderungen

### Anforderung 1: Dokumentenverwaltungssystem
> "Ich möchte auf Grundlage der ThemisDB ein Dokumentenverwaltungssystem als show-frontend präsentieren (Vorbild soll das PDV VIS5 oder ähnliches sein). Es soll aber die volle Integration von Geo, timeline, vector, graph Funktionalitäten beinhalten. Es soll in C# und .NET programmiert sein."

**Status:** ✅ **VOLLSTÄNDIG ERFÜLLT**

- ✅ Dokumentenverwaltungssystem als Show-Frontend erstellt
- ✅ PDV VIS5 analysiert und Feature-Vergleich dokumentiert
- ✅ Volle Integration: Geo, Timeline, Vector, Graph implementiert
- ✅ In C# und .NET 8 programmiert
- ✅ Vollständige WPF-Anwendung mit ModernWPF-Design

### Anforderung 2: Office-Integration
> "Das Werkzeug soll Word, Excel, Outlook, Powerpoint, Onenote und ähnliche Programme direkt benutzen für die Dokumentenbearbeitung und eine revisionsichere nahtlose Verarbeitung sichern."

**Status:** ✅ **VOLLSTÄNDIG ERFÜLLT**

- ✅ Direkte Integration mit Microsoft Office via COM Interop
- ✅ Word, Excel, PowerPoint, Outlook, OneNote vollständig integriert
- ✅ Automatische Track Changes Aktivierung in Word
- ✅ Revisionsichere Verarbeitung mit SHA256-Hashing
- ✅ Vollständiger Audit Trail für alle Änderungen
- ✅ Unveränderliche Versionsspeicherung in ThemisDB

### Anforderung 3: Verwaltungsstruktur
> "Da wir eine auf Prozessen und Akten basierende Verwaltungsstruktur haben brauchen wir immer eine Prozess-timeline über allem und eine korrespondierende Aktenstruktur nach deutschem Verwaltungsrecht (Behörde, Ablage, Akte, Unterakte, Vorgang, Dokument, Datei)."

**Status:** ✅ **VOLLSTÄNDIG ERFÜLLT**

- ✅ 7-stufige Hierarchie nach deutschem Verwaltungsrecht implementiert
- ✅ Vollständiges URN-System integriert (`urn:themis:authority:...`)
- ✅ Zentrale Prozess-Timeline über alle Hierarchieebenen
- ✅ 24 Event-Typen für vollständiges Lifecycle-Tracking
- ✅ Automatische Timeline-Generierung bei allen Operationen

---

## 🏗️ Architektur

### Clean Architecture mit CQRS

```
┌─────────────────────────────────────────────┐
│         Presentation Layer (WPF)            │
│  ViewModels → IMediator ✅                  │
│  Views, User Controls, Styles               │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│      Application Layer (Use Cases)          │
│  • Commands (19+) → IRequest<T>             │
│  • Queries (20+) → IRequest<T>              │
│  • Handlers → IRequestHandler<T,R>          │
│  • Events (2+) → INotification              │
│  • FluentValidation                         │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│          Domain Layer                       │
│  • Entities (Document, Process, File)       │
│  • Value Objects (GeoLocation, etc.)        │
│  • Domain Events                            │
│  • Business Logic                           │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│       Infrastructure Layer                  │
│  • ThemisDB API Client                      │
│  • Office COM Interop                       │
│  • DirectX 11 Renderer                      │
│  • SignalR Client                           │
│  • Ollama LLM Client                        │
└─────────────────────────────────────────────┘
```

### MediatR Pattern (CQRS)

**Commands** (Write Operations):
- `CreateDocumentCommand`, `UpdateDocumentCommand`, `DeleteDocumentCommand`
- `CheckOutDocumentCommand`, `CheckInDocumentCommand`
- `AddCommentCommand`, `AddFavoriteCommand`, `CreateInboxItemCommand`

**Queries** (Read Operations):
- `GetDocumentQuery`, `GetDocumentsQuery`, `GetDocumentLockStatusQuery`
- `GetFavoritesQuery`, `GetInboxItemsQuery`, `GetMyTasksQuery`
- `GetNavigationPathQuery`, `GetRelatedEntitiesQuery`

**Events** (Notifications):
- `DocumentCreatedEvent`, `TestDataGeneratedEvent`
- Loose coupling zwischen ViewModels über MediatR-Events

**Siehe:** [../MEDIATR_ARCHITECTURE_AUDIT.md](../MEDIATR_ARCHITECTURE_AUDIT.md)

---

## 🚀 Hauptfeatures

### 1. Multi-Model-Datenbank-Integration (ThemisDB)

#### Dokumentenverwaltung
- CRUD-Operationen über REST API
- Metadaten-Management (dynamische Felder)
- Volltext-Suche mit Ranking
- Revisionsverwaltung mit automatischer Versionierung

#### Graph-Funktionalität
- Dokumenten-Beziehungen (Verweise, Abhängigkeiten)
- Graph-Traversierung (Vorgänger, Nachfolger)
- Visualisierung mit DirectX 11 3D-Rendering
- Interaktive Node-Selection via Ray-Casting

#### Vector-Search
- Semantische Ähnlichkeitssuche
- Embedding-Generierung via Microsoft.ML
- Hybrid-Suche (Volltext + Semantik)
- K-Nearest-Neighbor Queries

#### Geo-Integration
- OpenStreetMap Tile-Loading & Caching
- Geo-Spatial Queries (Bounding Box, Radius, Nearest)
- GeoPoint, GeoTrack, GeoFence Datenstrukturen
- Haversine-Distance-Berechnung

#### Timeline-Aggregation
- Automatisches Tracking aller Dokumenten-Events
- 24 Event-Typen (Created, Updated, Viewed, Locked, etc.)
- Prozess-übergreifende Timeline
- Filtering nach User, Typ, Zeitraum

### 2. Deutsche Verwaltungsstruktur

**7-stufige Hierarchie:**

```
1. Behörde (Authority)
   └─ 2. Ablage (Registry)
      └─ 3. Akte (File)
         └─ 4. Unterakte (Subfile)
            └─ 5. Vorgang (Process)
               └─ 6. Dokument (Document)
                  └─ 7. Datei (Attachment)
```

**URN-System:**
```
urn:themis:authority:{authorityId}
urn:themis:registry:{registryId}
urn:themis:file:{fileId}
urn:themis:subfile:{subfileId}
urn:themis:process:{processId}
urn:themis:document:{documentId}
urn:themis:attachment:{attachmentId}
```

**Features:**
- Automatische URN-Generierung
- Hierarchie-Navigation (Breadcrumbs)
- Berechtigungsvererbung
- Aktenzeichen-Generierung nach deutschem Standard

### 3. Office-Integration (COM Interop)

**Unterstützte Anwendungen:**
- ✅ **Microsoft Word** - Track Changes, Kommentare, Revisionen
- ✅ **Microsoft Excel** - Formeln, Arbeitsmappen
- ✅ **Microsoft PowerPoint** - Präsentationen
- ✅ **Microsoft Outlook** - E-Mail-Integration, Aufgaben
- ✅ **Microsoft OneNote** - Notizen, Notizbücher

**Revisionssicherheit:**
- Automatische Aktivierung von Track Changes
- SHA256-Hashing aller Versionen
- Unveränderliche Speicherung in ThemisDB
- Vollständiger Audit Trail (Wer, Wann, Was)
- Diff-Visualisierung zwischen Versionen

**Workflow:**
1. Dokument aus ThemisDB laden
2. In Office öffnen (COM Automation)
3. Benutzer bearbeitet lokal
4. Bei Save: Automatisches Hashing + Track Changes
5. Neue Revision in ThemisDB speichern
6. Timeline-Event generieren

### 4. DirectX 11 3D-Rendering (9 Phasen)

**Implementierte Features:**

#### Phase 1-2: Grundlagen + Advanced Rendering
- DirectX 11 Device Management (P/Invoke)
- Parametrische Mesh-Generierung (Sphere, Cylinder)
- Camera3D mit View/Projection-Matrices
- Lighting System (Ambient + Diffuse)
- 60 FPS Render-Loop

#### Phase 3: GPU Pipeline
- HLSL Shader-Management (Vertex/Pixel)
- 5-Phase Rendering Pipeline:
  1. Mesh Preparation (Caching)
  2. Command Generation (Depth-Sorted)
  3. Buffer Updates (Constant Buffers)
  4. Execution (GPU Draw Calls)
  5. Performance Logging
- Depth Buffer mit Z-Testing
- Node Picking via Ray-Casting

#### Phase 4: OSM Map Integration
- OpenStreetMap Tile-Loading
- Geo-Spatial Query Engine
- MapLayerManager (Z-Ordering)
- Hybrid 2D Map + 3D Graph Rendering

#### Phase 5-9: Performance & Optimization
- GPU Buffer Management (Vertex/Index/Constant)
- Memory Pooling & Resource Management
- Real-world Testing mit 100+ Node Graphs
- Production Release Optimierungen

**Performance:**
- 60 FPS bei 100+ Nodes
- <16ms Frame Time
- GPU-beschleunigtes Rendering
- Automatisches Mesh-Caching

**Siehe:** [DIRECTX_OVERVIEW.md](DIRECTX_OVERVIEW.md)

### 5. Modern WPF UI

#### Dashboard
- Zentrale Startansicht mit Widgets
- Statistiken (Dokumente, Prozesse, Aufgaben)
- Schnellzugriff auf häufige Aktionen
- Recent Documents Timeline

#### Intelligente Navigation
- Breadcrumb-Navigation mit Kontextmenü
- Tab-basiertes Dokumenten-Browsing
- Favoritenverwaltung
- Intelligent Breadcrumb mit Workflow-Integration

#### Task Basket (Sidebar)
- Outlook-Style Aufgabenübersicht
- Gruppierung nach Status/Priorität/Fälligkeit
- Überfällige Aufgaben automatisch hervorgehoben
- Integriert mit Inbox, Reminders, Cosigning

#### Theming
- ModernWPF Light/Dark Mode
- Benutzerdefinierte Themis-Styles
- Responsive Layout
- Accessibility-Support

---

## 🛠️ Technologie-Stack

### Core Framework
- **.NET 8.0** (Windows)
- **WPF** (Windows Presentation Foundation)
- **C# 12**

### UI/UX
- **ModernWPF** - Modern UI-Design
- **CommunityToolkit.Mvvm** - MVVM Helpers
- **MaterialDesignThemes** - Icons & Controls

### Architecture & Patterns
- **MediatR** 12.2.0 - CQRS Pattern
- **FluentValidation** 11.9.0 - Input Validation
- **Dependency Injection** - Built-in .NET DI

### Data Access
- **ThemisDB REST API** - Multi-Model Database
- **HTTP Client** - REST API Communication
- **System.Text.Json** - JSON Serialization

### Office Integration
- **Microsoft Office Interop** - COM Automation
- **Word.Interop**, **Excel.Interop**, **PowerPoint.Interop**
- **Outlook.Interop**, **OneNote.Interop**

### DirectX Rendering
- **SharpDX** 4.2.0 - DirectX 11 Wrapper
- **D3D11**, **DXGI**, **D3DCompiler**
- **WPF D3D11 Integration** - D3DImage

### Machine Learning
- **Microsoft.ML** 3.0.1 - Vector Embeddings
- **Microsoft.ML.AutoML** 0.21.1

### Real-time Collaboration
- **SignalR.Client** 8.0.0 - WebSocket Communication
- **DocumentLocking**, **Comments**, **Presence Tracking**

### LLM Integration
- **Ollama Client** - Local LLM (llama3.2, qwen2.5)
- **Streaming API** - Server-Sent Events (SSE)
- **Testdaten-Generierung** - Authentische Pseudodaten

### Logging
- **Microsoft.Extensions.Logging** - Structured Logging
- **Serilog** (empfohlen für Production)

---

## 📦 Projektstruktur

```
Themis.DocumentManager/
├── App.xaml / App.xaml.cs           # Application Entry Point, DI Setup
├── MainWindow.xaml / .cs            # Hauptfenster mit Navigation
│
├── Models/                          # Domain Models
│   ├── Document.cs                  # Haupt-Dokumentenmodell
│   ├── DocumentRevision.cs          # Versionsverwaltung
│   ├── AdministrativeStructure.cs   # 7-stufige Hierarchie
│   ├── AIAssistantModels.cs         # LLM Chat Models
│   ├── DashboardModels.cs           # Dashboard Widgets
│   ├── DocumentTreeModels.cs        # Hierarchie-Tree
│   ├── GeoModels.cs                 # GeoPoint, GeoTrack, GeoFence
│   └── ... (20+ Model-Dateien)
│
├── ViewModels/                      # MVVM ViewModels
│   ├── MainViewModel.cs             # Haupt-Navigation
│   ├── DocumentBrowserViewModel.cs  # Dokumentenliste
│   ├── AIChatViewModel.cs           # AI Assistant Chat
│   ├── InboxViewModel.cs            # Eingangskorb
│   ├── TimelineViewModel.cs         # Timeline-Aggregation
│   └── ViewModels.cs                # Weitere ViewModels (20+)
│
├── Views/                           # WPF Views
│   ├── MainWindow.xaml              # Hauptfenster
│   ├── DashboardView.xaml           # Dashboard-Startseite
│   ├── DocumentBrowserView.xaml     # Dokumenten-Grid
│   ├── GraphView3D.xaml             # DirectX 3D Graph
│   ├── DocumentCollaborationView.xaml # Real-time Collaboration
│   └── ... (30+ View-Dateien)
│
├── Services/                        # Business Logic & Infrastructure
│   ├── ThemisApiClient.cs           # REST API Client
│   ├── DocumentService.cs           # CRUD Operations
│   ├── SearchService.cs             # Volltext + Vector Search
│   ├── MetadataService.cs           # Metadaten-Verwaltung
│   ├── GeoServices.cs               # Geo-Queries
│   ├── TimelineAggregationService.cs # Timeline-Events
│   ├── OfficeIntegrationService.cs  # Office COM Interop
│   ├── RevisionService.cs           # Versionsverwaltung
│   ├── OllamaContentGeneratorService.cs # LLM Content Generation
│   ├── StatusMonitorService.cs      # ThemisDB + Ollama Health
│   └── ... (50+ Service-Dateien)
│
├── Application/                     # CQRS Commands/Queries/Handlers
│   ├── Documents/
│   │   ├── Commands/
│   │   │   └── CreateDocument/
│   │   │       ├── CreateDocumentCommand.cs
│   │   │       ├── CreateDocumentCommandHandler.cs
│   │   │       └── CreateDocumentCommandValidator.cs
│   │   └── Queries/
│   │       └── GetDocument/
│   │           ├── GetDocumentQuery.cs
│   │           └── GetDocumentQueryHandler.cs
│   ├── Collaboration/               # Check-out/Check-in, Comments
│   ├── Favorites/                   # Favoriten-Management
│   ├── Inbox/                       # Eingangskorb
│   ├── Navigation/                  # Breadcrumb, Relations
│   └── ... (10+ Feature-Ordner)
│
├── Domain/                          # Domain Layer
│   ├── Events/
│   │   ├── DocumentCreatedEvent.cs
│   │   └── TestDataGeneratedEvent.cs
│   └── Collaboration/
│       ├── DocumentLock.cs          # Locking-Entität
│       ├── Comment.cs               # Kommentar-Entität
│       └── UserPresence.cs          # Real-time Präsenz
│
├── Infrastructure/                  # External Dependencies
│   ├── DirectX/                     # DirectX 11 Rendering
│   │   ├── DirectXCore.cs
│   │   ├── MeshGenerator.cs
│   │   ├── RenderingPipeline.cs
│   │   ├── ShaderPipeline.cs
│   │   ├── BufferManagement.cs
│   │   ├── NodePickingSystem.cs
│   │   ├── OSMMapManager.cs
│   │   └── OSMMapRenderer.cs
│   └── SignalR/
│       └── SignalRService.cs        # Real-time WebSocket
│
├── Converters/                      # WPF Value Converters
│   └── ValueConverters.cs           # 20+ Converter
│
├── Styles/                          # WPF Styles & Themes
│   └── ThemisStyles.xaml
│
└── docs/                            # Dokumentation (70+ MD-Dateien)
    ├── README.md                    # Dokumentationsindex
    ├── PROJECT_OVERVIEW.md          # Diese Datei
    ├── ARCHITECTURE.md              # Architektur-Details
    ├── QUICKSTART.md                # Entwickler-Quickstart
    └── ... (Siehe Dokumentationsindex)
```

---

## 📊 Statistiken

### Code-Umfang
- **Gesamt:** ~40,000+ Zeilen Code
- **Models:** ~5,000 Zeilen (60+ Dateien)
- **ViewModels:** ~8,000 Zeilen (30+ Dateien)
- **Services:** ~12,000 Zeilen (50+ Services)
- **Application Layer:** ~6,000 Zeilen (39 Commands/Queries)
- **Infrastructure:** ~9,000 Zeilen (DirectX, SignalR, etc.)

### Komponenten
- **50+ Services** - Vollständige Business Logic
- **30+ ViewModels** - MVVM Pattern
- **60+ Models** - Domain Entities
- **39 Commands/Queries** - CQRS Pattern
- **70+ Dokumentations-Dateien** - Umfassende Docs

### Features
- ✅ 7-stufige Verwaltungsstruktur
- ✅ 24 Timeline-Event-Typen
- ✅ 5 Office-Anwendungen integriert
- ✅ 4 ThemisDB-Datenmodelle (Document, Graph, Vector, Geo)
- ✅ 9 DirectX-Rendering-Phasen
- ✅ 2 LLM-Modelle (Ollama: llama3.2, qwen2.5)

---

## 🧪 Testing & Quality

### Build Status
- ✅ **0 Errors**
- ⚠️ **30 Warnings** (harmless, P/Invoke-related)
- ⏱️ **Build Time:** 7-12 Sekunden

### Testing-Strategie
- **Unit Tests:** Commands/Queries/Handlers (FluentValidation)
- **Integration Tests:** ThemisDB API Client
- **UI Tests:** ViewModels mit Mock-Services
- **Performance Tests:** DirectX Rendering (100+ Nodes @ 60 FPS)

### Code Quality
- ✅ **SOLID Principles** durchgehend angewendet
- ✅ **Clean Architecture** strikte Layer-Trennung
- ✅ **CQRS Pattern** vollständig implementiert
- ✅ **Async/Await** durchgehend verwendet
- ✅ **Dependency Injection** für alle Services
- ✅ **FluentValidation** für alle Commands

**Siehe:** [TESTING_GUIDE.md](TESTING_GUIDE.md), [BEST_PRACTICES.md](BEST_PRACTICES.md)

---

## 🚀 Getting Started

### Voraussetzungen

**Software:**
- Visual Studio 2022 (Version 17.8+)
- .NET 8.0 SDK
- Windows 10/11 (x64)
- Microsoft Office (für Office-Integration)

**Optional:**
- Ollama (für LLM-Features): `https://ollama.ai`
- ThemisDB Server: `http://localhost:8765`

### Installation

1. **Repository klonen:**
   ```powershell
   git clone <repository-url>
   cd Themis.DocumentManager
   ```

2. **Dependencies installieren:**
   ```powershell
   dotnet restore
   ```

3. **Konfiguration anpassen:**
   Bearbeite `appsettings.json`:
   ```json
   {
     "ThemisDB": {
       "BaseUrl": "http://localhost:8765"
     },
     "Ollama": {
       "BaseUrl": "http://localhost:11434",
       "DefaultModel": "llama3.2"
     }
   }
   ```

4. **Build & Run:**
   ```powershell
   dotnet build
   dotnet run
   ```

### Erster Start

1. **Dashboard** öffnet sich automatisch
2. **ThemisDB-Status** in StatusBar prüfen (🟢 = Online)
3. **Testdaten generieren:**
   - Menü: Extras → Testdaten-Generator
   - Quick Generate (10 Dokumente) oder
   - Mit Inhalten generieren (Ollama LLM)
4. **Dokumente browsen** im Document Browser

**Siehe:** [QUICKSTART.md](QUICKSTART.md)

---

## 📖 Weitere Dokumentation

### Architektur & Design
- [ARCHITECTURE.md](ARCHITECTURE.md) - Vollständige Architektur-Dokumentation
- [CLEAN_ARCHITECTURE_README.md](CLEAN_ARCHITECTURE_README.md) - Clean Architecture Pattern
- [MEDIATR_ARCHITECTURE_AUDIT.md](../MEDIATR_ARCHITECTURE_AUDIT.md) - MediatR CQRS Audit

### Feature-Guides
- [VIS_INTEGRATION_GUIDE.md](VIS_INTEGRATION_GUIDE.md) - VIS-Feature Integration
- [ADMINISTRATIVE_STRUCTURE.md](ADMINISTRATIVE_STRUCTURE.md) - Deutsche Verwaltungsstruktur
- [METADATA_BADGE_SYSTEM.md](METADATA_BADGE_SYSTEM.md) - Metadaten-Badge-System
- [OSM_MAP_INTEGRATION.md](OSM_MAP_INTEGRATION.md) - OpenStreetMap Integration

### DirectX Rendering
- [DIRECTX_OVERVIEW.md](DIRECTX_OVERVIEW.md) - Phasen 1-9 Übersicht
- [DIRECTX11_PROJECT_COMPLETE.md](DIRECTX11_PROJECT_COMPLETE.md) - Projekt-Abschluss
- [GPU_RENDERING_PIPELINE.md](GPU_RENDERING_PIPELINE.md) - GPU Pipeline Details

### UI/UX
- [UI_IMPLEMENTATION_GUIDE.md](UI_IMPLEMENTATION_GUIDE.md) - UI Guidelines
- [DASHBOARD_SIDEBAR_INTEGRATION.md](DASHBOARD_SIDEBAR_INTEGRATION.md) - Dashboard & Sidebar
- [INTELLIGENT_BREADCRUMB_NAVIGATION.md](INTELLIGENT_BREADCRUMB_NAVIGATION.md) - Breadcrumb

### Best Practices
- [BEST_PRACTICES.md](BEST_PRACTICES.md) - Coding Standards
- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Testing-Leitfaden
- [LOGGING_GUIDE.md](LOGGING_GUIDE.md) - Logging Best Practices

---

## 🗺️ Roadmap

### Phase 10 (Geplant)
- [ ] Mobile App (Xamarin/MAUI)
- [ ] Web-Frontend (Blazor)
- [ ] REST API Gateway

### Phase 11 (Geplant)
- [ ] Erweiterte AI-Features (Document Classification, OCR)
- [ ] Workflow-Automation
- [ ] E-Akte-Integration

### Langfristig
- [ ] Multi-Tenancy
- [ ] Cloud-Deployment (Azure/AWS)
- [ ] Microservices-Architektur

---

## 👥 Team & Kontakt

**Entwicklung:** Themis Development Team  
**Architektur:** Clean Architecture mit CQRS  
**Status:** Production Ready (Version 1.0)

---

## 📄 Lizenz

Proprietary - Alle Rechte vorbehalten

---

**Projektversion:** 1.0  
**Dokumentversion:** 2.0  
**Letzte Aktualisierung:** 11. Dezember 2025
