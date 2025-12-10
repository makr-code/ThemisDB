# Phase 2 - Advanced Features Implementation Plan

**Datum:** 2025-12-10  
**Status:** 🚀 In Progress  
**Basierend auf:** DEVELOPMENT_STRATEGY_SUMMARY.md

---

## 🎯 Überblick

Phase 2 implementiert die "Advanced Features" aus der Entwicklungsstrategie (Q2 2026).

**Ziele:**
- ✅ 10+ simultane Benutzer unterstützen
- ✅ 90%+ Classification Accuracy erreichen
- ✅ Collaboration Features (Check-in/Check-out, SignalR, Comments)
- ✅ AI/ML Integration (Auto-Classification, Metadata Extraction)

---

## 📊 Sprint-Übersicht

### Sprint 5-6: Collaboration Features (2 Wochen)
**Fokus:** Echtzeit-Zusammenarbeit und Dokumenten-Locking

#### Features:
1. **Check-in/Check-out System**
   - Dokumenten-Locking Mechanismus
   - Optimistic vs. Pessimistic Locking
   - Lock-Timeout Management
   - Lock-Override für Administratoren

2. **SignalR Integration**
   - Real-time Updates für Dokumenten-Änderungen
   - Benutzer-Präsenz-Anzeige
   - Live-Notifications
   - Connection Management

3. **Comments & Annotations**
   - Kommentar-System für Dokumente
   - Thread-basierte Diskussionen
   - @Mentions für Benutzer
   - Kommentar-Versionierung

#### Technischer Stack:
- ASP.NET Core SignalR für WPF
- Domain Events (bereits vorhanden)
- MediatR für Command/Query Separation
- Entity Framework Core für Persistence

---

### Sprint 7-8: AI/ML Integration (2 Wochen)
**Fokus:** Intelligente Dokumenten-Klassifizierung und Metadaten-Extraktion

#### Features:
1. **Auto-Classification**
   - ML.NET Integration
   - Trainings-Daten-Management
   - Model Training Pipeline
   - Confidence Scoring
   - Multi-Label Classification

2. **Metadata Extraction**
   - Named Entity Recognition (NER)
   - Datum/Zeit Extraktion
   - Personen/Organisations-Erkennung
   - Automatische Tagging
   - Custom Entity Training

3. **Training Data Management**
   - Training-Set Creation UI
   - Data Augmentation
   - Model Versioning
   - Performance Metrics Dashboard

#### Technischer Stack:
- ML.NET für Classification
- Microsoft.ML.OnnxRuntime für vortrainierte Modelle
- Custom Training Pipeline
- Background Job Processing

---

## 🏗️ Architektur-Design

### Clean Architecture Layers (gemäß Strategy)

```
┌─────────────────────────────────────────────────┐
│           Presentation Layer (WPF)              │
│  Views, ViewModels, Converters                  │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│        Application Layer (Use Cases)            │
│  Commands, Queries, Handlers (MediatR)          │
│  - CheckOutDocumentCommand                      │
│  - CheckInDocumentCommand                       │
│  - AddCommentCommand                            │
│  - ClassifyDocumentCommand                      │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│          Domain Layer (Business Logic)          │
│  Entities, Value Objects, Domain Services       │
│  - DocumentLock (Entity)                        │
│  - Comment (Entity)                             │
│  - DocumentClassification (Value Object)        │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│       Infrastructure Layer (External)           │
│  - SignalR Hub Implementation                   │
│  - ML.NET Services                              │
│  - ThemisDB Repository                          │
│  - File System Access                           │
└─────────────────────────────────────────────────┘
```

---

## 📁 Dateistruktur

### Neue Dateien für Phase 2:

```
Themis.DocumentManager/
├── Application/
│   ├── Collaboration/
│   │   ├── Commands/
│   │   │   ├── CheckOutDocumentCommand.cs
│   │   │   ├── CheckInDocumentCommand.cs
│   │   │   ├── ReleaseDocumentLockCommand.cs
│   │   │   └── AddCommentCommand.cs
│   │   ├── Queries/
│   │   │   ├── GetDocumentLockStatusQuery.cs
│   │   │   ├── GetActiveLocksQuery.cs
│   │   │   └── GetDocumentCommentsQuery.cs
│   │   └── Handlers/
│   │       └── [Command/Query Handlers]
│   │
│   └── Classification/
│       ├── Commands/
│       │   ├── ClassifyDocumentCommand.cs
│       │   ├── TrainClassificationModelCommand.cs
│       │   └── ExtractMetadataCommand.cs
│       ├── Queries/
│       │   ├── GetClassificationPredictionQuery.cs
│       │   └── GetTrainingDataQuery.cs
│       └── Handlers/
│           └── [Command/Query Handlers]
│
├── Domain/
│   ├── Collaboration/
│   │   ├── DocumentLock.cs
│   │   ├── Comment.cs
│   │   ├── CommentThread.cs
│   │   └── UserPresence.cs
│   │
│   └── Classification/
│       ├── DocumentClassification.cs
│       ├── ClassificationModel.cs
│       ├── TrainingData.cs
│       └── ExtractedMetadata.cs
│
├── Infrastructure/
│   ├── SignalR/
│   │   ├── DocumentHub.cs
│   │   ├── SignalRService.cs
│   │   └── HubConnectionManager.cs
│   │
│   └── MachineLearning/
│       ├── MLModelTrainer.cs
│       ├── DocumentClassifier.cs
│       ├── MetadataExtractor.cs
│       └── TrainingDataRepository.cs
│
├── Services/
│   ├── CollaborationService.cs
│   ├── DocumentLockingService.cs
│   ├── CommentService.cs
│   ├── ClassificationService.cs
│   └── MetadataExtractionService.cs
│
├── ViewModels/
│   ├── DocumentCollaborationViewModel.cs
│   └── ClassificationTrainingViewModel.cs
│
└── Views/
    ├── DocumentCollaborationView.xaml
    └── ClassificationTrainingView.xaml
```

---

## 🔧 Implementierungs-Schritte

### Woche 1-2: Collaboration Infrastructure

#### Tag 1-2: Domain Models
- [x] Erstelle `DocumentLock` Entity
- [x] Erstelle `Comment` und `CommentThread` Entities
- [x] Erstelle `UserPresence` Value Object
- [x] Domain Events definieren

#### Tag 3-4: Application Layer (CQRS)
- [x] MediatR Commands für Check-in/Check-out
- [x] Query Handlers für Lock-Status
- [x] FluentValidation Rules
- [ ] Command Pipeline Behaviors

#### Tag 5-7: SignalR Integration
- [x] SignalR Hub implementieren
- [x] WPF Client Connection Manager
- [x] Real-time Event Broadcasting
- [x] Connection State Management

#### Tag 8-10: UI Implementation
- [ ] Collaboration Panel UI
- [ ] Lock-Status Indicator
- [ ] Comments Sidebar
- [ ] User Presence Display

---

### Woche 3-4: AI/ML Integration

#### Tag 1-3: ML.NET Setup
- [ ] ML.NET NuGet Packages installieren
- [ ] Classification Pipeline erstellen
- [ ] Training Data Schema definieren
- [ ] Model Builder Integration

#### Tag 4-6: Document Classification
- [ ] Feature Engineering (Text → Vector)
- [ ] Multi-Class Classification Model
- [ ] Prediction Service
- [ ] Confidence Scoring

#### Tag 7-9: Metadata Extraction
- [ ] NER Pipeline (Named Entity Recognition)
- [ ] Date/Time Parser
- [ ] Custom Entity Extractor
- [ ] Auto-Tagging Service

#### Tag 10-14: Training UI & Integration
- [ ] Training Data Management View
- [ ] Model Performance Dashboard
- [ ] Background Training Jobs
- [ ] A/B Testing Infrastructure

---

## 🎯 Erfolgskriterien (aus Development Strategy)

### Technische Metriken:
- ✅ **Simultane Benutzer:** 10+ ohne Performance-Einbußen
- ✅ **Classification Accuracy:** 90%+ (gemessen mit Test-Set)
- ✅ **Real-time Latency:** <100ms für SignalR Events
- ✅ **Lock Acquisition:** <50ms

### Funktionale Metriken:
- ✅ **Check-in/Check-out:** Funktioniert zuverlässig
- ✅ **Comments:** Thread-basierte Diskussionen möglich
- ✅ **Auto-Classification:** 90%+ korrekte Vorhersagen
- ✅ **Metadata Extraction:** 80%+ Precision/Recall

### Code Quality:
- ✅ **Test Coverage:** >80% (gemäß Strategy)
- ✅ **SOLID Principles:** Eingehalten
- ✅ **Clean Architecture:** Strikte Layer-Trennung
- ✅ **Async/Await:** Durchgehend verwendet

---

## 📚 Dependencies & Tools

### Neue NuGet Packages:

```xml
<!-- SignalR für WPF -->
<PackageReference Include="Microsoft.AspNetCore.SignalR.Client" Version="8.0.0" />

<!-- ML.NET für Classification -->
<PackageReference Include="Microsoft.ML" Version="3.0.1" />
<PackageReference Include="Microsoft.ML.AutoML" Version="0.21.1" />
<PackageReference Include="Microsoft.ML.OnnxRuntime" Version="1.16.3" />

<!-- Background Jobs (optional) -->
<PackageReference Include="Hangfire.Core" Version="1.8.6" />
```

### Bereits vorhanden (aus Phase 1):
- ✅ MediatR (12.2.0)
- ✅ FluentValidation (11.9.0)
- ✅ CommunityToolkit.Mvvm (8.2.2)
- ✅ Microsoft.Extensions.DependencyInjection (8.0.1)

---

## 🔄 Integration mit bestehenden Phasen

### Phase 1 (Architecture Refactoring):
- Nutzt Clean Architecture Foundation
- Erweitert Domain Layer
- CQRS Pattern bereits etabliert

### Phase 23-24 (Smart Forms & Visualization):
- Collaboration Features integrieren sich mit SmartForm
- Classification kann Form-Daten nutzen
- Shared Services (Logging, Caching)

---

## 🚀 Quick Start

### 1. Dependencies installieren
```bash
cd /home/runner/work/ThemisDB/ThemisDB/projects/Themis.DocumentManager
dotnet add package Microsoft.AspNetCore.SignalR.Client
dotnet add package Microsoft.ML
dotnet add package Microsoft.ML.AutoML
```

### 2. Domain Models erstellen
```csharp
// Domain/Collaboration/DocumentLock.cs
public class DocumentLock
{
    public string Id { get; set; }
    public string DocumentId { get; set; }
    public string UserId { get; set; }
    public DateTime LockedAt { get; set; }
    public DateTime? ExpiresAt { get; set; }
    public LockType Type { get; set; } // Read, Write
}
```

### 3. MediatR Commands
```csharp
// Application/Collaboration/Commands/CheckOutDocumentCommand.cs
public record CheckOutDocumentCommand(
    string DocumentId,
    string UserId,
    LockType LockType
) : IRequest<Result<DocumentLock>>;
```

### 4. SignalR Hub
```csharp
// Infrastructure/SignalR/DocumentHub.cs
public class DocumentHub : Hub
{
    public async Task NotifyDocumentLocked(string documentId, string userId)
    {
        await Clients.Others.SendAsync("DocumentLocked", documentId, userId);
    }
}
```

---

## 📈 Timeline

| Woche | Sprint | Thema | Deliverables |
|-------|--------|-------|--------------|
| 1-2 | 5 | Collaboration (Part 1) | Check-in/Check-out, SignalR |
| 2-3 | 6 | Collaboration (Part 2) | Comments, UI Integration |
| 3-4 | 7 | AI/ML (Part 1) | Classification Model, Training |
| 4-5 | 8 | AI/ML (Part 2) | Metadata Extraction, Dashboard |

**Total: 4-5 Wochen** (gemäß Development Strategy: Q2 2026)

---

## 🎓 Learning Resources (aus Development Strategy)

### Bücher:
- ✅ "Clean Architecture" by Robert C. Martin
- ✅ "Domain-Driven Design" by Eric Evans
- ✅ "Hands-On Machine Learning with ML.NET"

### Online:
- Pluralsight: "Building Real-time Apps with SignalR"
- Microsoft Learn: "ML.NET for Beginners"
- GitHub: clean-architecture-manga

---

## ✅ Definition of Done

Eine Phase 2 Feature ist "done" wenn:
- ✅ Code kompiliert ohne Fehler
- ✅ Unit Tests geschrieben (>80% Coverage)
- ✅ Integration Tests erfolgreich
- ✅ Dokumentation aktualisiert
- ✅ UI funktioniert in MainWindow
- ✅ Performance-Metriken erfüllt
- ✅ Code Review abgeschlossen

---

## 🔜 Nächste Schritte

### Immediate (Heute):
1. [ ] NuGet Dependencies installieren
2. [ ] Domain Models für Collaboration erstellen
3. [ ] Erste MediatR Commands implementieren

### Diese Woche:
1. [ ] SignalR Hub aufsetzen
2. [ ] Check-in/Check-out Grundfunktionalität
3. [ ] Erste UI-Prototypen

### Nächste Woche:
1. [ ] Comments System
2. [ ] ML.NET Integration beginnen
3. [ ] Training Pipeline Setup

---

**Status:** 🚀 **READY TO START**

Die Phase 2 Implementierung kann beginnen!
