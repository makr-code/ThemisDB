# MediatR Architektur Audit - Themis.DocumentManager

**Datum:** 2025-12-11  
**Status:** ✅ CQRS-Architektur teilweise implementiert  
**MediatR Version:** 12.2.0

---

## 🎯 Executive Summary

Die Themis.DocumentManager Anwendung nutzt **MediatR** für CQRS (Command Query Responsibility Segregation) und Event-Driven Architecture. Die Implementierung ist **teilweise vollständig**:

### ✅ Erfolgreich implementiert:
1. **Application Layer** - Vollständiges CQRS mit Commands/Queries/Handlers
2. **ViewModels** - Saubere Entkopplung über IMediator
3. **Events** - Domain Events für lose Kopplung
4. **Handlers** - Automatische Registrierung via Assembly-Scan

### ⚠️ Architektur-Lücke identifiziert:
- **Services Layer** nutzt **KEIN MediatR** → Direkte Service-zu-Service Kopplung
- Services verwenden Constructor Injection von anderen Services (z.B. `InboxService(IThemisApiClient, INotificationService)`)
- Dies ist **akzeptabel** für Infrastructure/Repository-Pattern, aber **nicht CQRS-konform**

---

## 📊 Architektur-Übersicht

```
┌─────────────────────────────────────────────┐
│         Presentation Layer (WPF)            │
│  ViewModels → IMediator ✅                  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│      Application Layer (Use Cases)          │
│  Commands → IMediator ✅                    │
│  Queries → IMediator ✅                     │
│  Handlers → IRequestHandler ✅              │
│  Events → INotification ✅                  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│          Services Layer                     │
│  Services → Direct DI ⚠️                    │
│  (ThemisApiClient, NotificationService)     │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│       Infrastructure Layer                  │
│  ThemisDB API, SignalR, Ollama LLM          │
└─────────────────────────────────────────────┘
```

---

## ✅ MediatR Usage Analysis

### 1. Application Layer (CQRS Pattern) ✅

#### **Commands** (19 Commands gefunden)
```
Application/
├── Collaboration/Commands/CollaborationCommands.cs
│   ├── CheckOutDocumentCommand : IRequest<Result<DocumentLock>>
│   ├── CheckInDocumentCommand : IRequest<Result<bool>>
│   ├── ReleaseDocumentLockCommand : IRequest<Result<bool>>
│   ├── AddCommentCommand : IRequest<Result<Comment>>
│   ├── UpdateCommentCommand : IRequest<Result<Comment>>
│   ├── DeleteCommentCommand : IRequest<Result<bool>>
│   └── AddCommentReactionCommand : IRequest<Result<bool>>
│
├── Classification/Commands/ClassificationCommands.cs
│   ├── ClassifyDocumentCommand
│   └── TrainClassificationModelCommand
│
├── Documents/Commands/
│   ├── CreateDocument/CreateDocumentCommand.cs
│   ├── UpdateDocument/UpdateDocumentCommand.cs (geplant)
│   └── DeleteDocument/DeleteDocumentCommand.cs (geplant)
│
├── Favorites/Commands/
│   ├── AddToFavoritesCommand.cs
│   └── RemoveFromFavoritesCommand.cs
│
├── Inbox/Commands/
│   └── CreateInboxItemCommand.cs
│
├── Reminders/Commands/
│   └── CreateReminderCommand.cs
│
└── Cosigning/Commands/
    └── ApproveCosigningStepCommand.cs
```

#### **Queries** (20 Queries gefunden)
```
Application/
├── Collaboration/Queries/CollaborationQueries.cs
│   ├── GetDocumentLockStatusQuery : IRequest<Result<DocumentLock?>>
│   ├── GetActiveLocksQuery : IRequest<Result<List<DocumentLock>>>
│   ├── GetDocumentCommentsQuery : IRequest<Result<CommentCollection>>
│   ├── GetCommentQuery : IRequest<Result<Comment?>>
│   ├── GetDocumentPresencesQuery : IRequest<Result<List<UserPresence>>>
│   └── CanUserEditDocumentQuery : IRequest<Result<bool>>
│
├── Classification/Queries/ClassificationQueries.cs
│   ├── GetDocumentClassificationsQuery
│   └── GetClassificationConfidenceQuery
│
├── Documents/Queries/
│   ├── GetDocument/GetDocumentQuery.cs
│   └── GetDocuments/GetDocumentsQuery.cs
│
├── Favorites/Queries/
│   ├── GetFavoritesQuery.cs
│   └── IsFavoriteQuery.cs
│
├── Inbox/Queries/
│   └── GetInboxItemsQuery.cs
│
├── Navigation/Queries/
│   ├── GetNavigationPathQuery.cs
│   └── GetRelatedEntitiesQuery.cs
│
├── Reminders/Queries/
│   └── GetDueRemindersQuery.cs
│
└── Tasks/Queries/
    └── GetMyTasksQuery.cs
```

#### **Handlers** (2 Handler-Dateien gefunden)
```
Application/
├── Collaboration/Handlers/CollaborationCommandHandlers.cs
│   ├── CheckOutDocumentHandler : IRequestHandler<CheckOutDocumentCommand, Result<DocumentLock>>
│   ├── CheckInDocumentHandler : IRequestHandler<CheckInDocumentCommand, Result<bool>>
│   └── AddCommentHandler : IRequestHandler<AddCommentCommand, Result<Comment>>
│
└── Classification/Handlers/ClassificationHandlers.cs
    ├── ClassifyDocumentHandler
    └── TrainClassificationModelHandler
```

**Note:** Handlers für Documents/Favorites/Inbox/etc. befinden sich in den jeweiligen Command/Query-Ordnern (z.B. `CreateDocumentCommandHandler.cs`).

#### **Events** (2 Domain Events)
```
Domain/Events/
├── DocumentCreatedEvent.cs : INotification
└── TestDataGeneratedEvent.cs : INotification
```

**Event-Flow Beispiel:**
```csharp
// Publisher (TestDataGeneratorViewModel)
await _mediator.Publish(new TestDataGeneratedEvent(count, DateTime.UtcNow));

// Handler (DocumentBrowserViewModel)
public async Task Handle(TestDataGeneratedEvent notification, CancellationToken ct)
{
    await LoadDocumentsAsync(); // UI automatisch aktualisieren
}
```

---

### 2. Presentation Layer (ViewModels) ✅

**ViewModels mit MediatR-Integration:**

| ViewModel | MediatR Usage | Purpose |
|-----------|---------------|---------|
| `TestDataGeneratorViewModel` | `private readonly IMediator? _mediator;` | Publiziert TestDataGeneratedEvent |
| `DocumentCollaborationView` | `private readonly IMediator _mediator;` | Real-time Collaboration Commands/Queries |
| `TaskBasketViewModel` | `private readonly IMediator _mediator;` | CQRS für Task-Operationen |
| `IntelligentBreadcrumbViewModel` | `private readonly IMediator _mediator;` | Navigation-Queries |
| `FavoritesViewModel` | `private readonly IMediator _mediator;` | Favorites Commands |
| `DocumentBrowserViewModel` | `INotificationHandler<TestDataGeneratedEvent>` | Event-Handler für UI-Updates |

**Beispiel (TaskBasketViewModel):**
```csharp
public class TaskBasketViewModel
{
    private readonly IMediator _mediator;
    
    // ✅ Best practice: MVVM pattern with IMediator for CQRS
    public TaskBasketViewModel(IMediator mediator)
    {
        _mediator = mediator;
    }
    
    public async Task LoadTasksAsync()
    {
        var query = new GetMyTasksQuery(CurrentUserId);
        var tasks = await _mediator.Send(query);
        Tasks = tasks;
    }
}
```

---

### 3. Services Layer ⚠️

**Status:** Services nutzen **KEIN MediatR**, sondern **direkte Dependency Injection**.

**Services gefunden (50+):**
```
Services/
├── ServiceImplementations.cs
│   ├── DocumentService : IDocumentService
│   ├── SearchService : ISearchService
│   ├── MetadataService : IMetadataService
│   ├── GeoService : IGeoService
│   ├── TimelineService : ITimelineService
│   ├── VectorService : IVectorService
│   └── GraphService : IGraphService
│
├── Phase1ServiceImplementations.cs
│   ├── InboxService : IInboxService
│   ├── ReminderService : IReminderService
│   ├── CosigningService : ICosigningService
│   └── ProcessLogService : IProcessLogService
│
├── Phase3ComplianceServices.cs
│   ├── FourEyesPrincipleService : IFourEyesPrincipleService
│   ├── FileAccessLogService : IFileAccessLogService
│   ├── SubstitutionService : ISubstitutionService
│   ├── EGovService : IEGovService
│   └── TransferNoteService : ITransferNoteService
│
├── DocumentTreeService.cs
├── RevisionService.cs
├── ProcessWatchService.cs
├── ThemisDBService.cs
├── OllamaContentGeneratorService.cs
├── StatusMonitorService.cs
└── ... 30+ weitere Services
```

**Beispiel - Direkte Service-Kopplung:**
```csharp
// InboxService (Phase1ServiceImplementations.cs)
public class InboxService : IInboxService
{
    private readonly IThemisApiClient _apiClient;        // ⚠️ Direkte DI
    private readonly INotificationService _notificationService;  // ⚠️ Direkte DI

    public InboxService(IThemisApiClient apiClient, INotificationService notificationService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
    }

    // ⚠️ Direkte Service-Aufrufe statt MediatR
    public async Task<InboxItem> CreateInboxItemAsync(InboxItem item)
    {
        await _apiClient.PutAsync<object, object>(...);
        // Kein _mediator.Send(new CreateInboxItemCommand(...))
        return item;
    }
}
```

**Warum ist das OK?**
- Services sind **Infrastructure/Repository Layer**
- Sie kapseln externe APIs (ThemisDB, Ollama, SignalR)
- Sie sollten **nicht direkt von ViewModels aufgerufen werden**
- ViewModels → **MediatR** → **Handlers** → Services ✅

**Warum ist das NICHT ideal?**
- Services rufen direkt andere Services auf (z.B. InboxService → NotificationService)
- Besseres Pattern: Handlers rufen mehrere Services auf, Services bleiben isoliert

---

## 🔧 Dependency Injection Setup

**App.xaml.cs:**
```csharp
// ✅ MediatR Registrierung (automatischer Assembly-Scan)
services.AddMediatR(cfg => 
    cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly()));

// ✅ FluentValidation für Command-Validierung
services.AddValidatorsFromAssembly(Assembly.GetExecutingAssembly());

// ⚠️ Services (direkte Registrierung)
services.AddSingleton<IThemisDBService, ThemisDBService>();
services.AddSingleton<IDocumentService, DocumentService>();
services.AddSingleton<IInboxService, InboxService>();
services.AddSingleton<INotificationService, NotificationService>();
// ... 50+ Services

// ✅ Phase 2 Collaboration Services
services.AddSingleton<IDocumentLockingService, DocumentLockingService>();
services.AddSingleton<ICommentService, CommentService>();
services.AddSingleton<ISignalRService, SignalRService>();
```

**Handler-Registrierung:**
```csharp
// Automatisch registriert via Assembly-Scan:
// - IRequestHandler<TRequest, TResponse>
// - INotificationHandler<TNotification>
```

---

## 📈 Verwendungsbeispiele

### ✅ RICHTIG: ViewModel → MediatR → Handler → Service

```csharp
// 1. ViewModel (Presentation Layer)
public class DocumentBrowserViewModel
{
    private readonly IMediator _mediator;

    public async Task CreateDocumentAsync(Document doc)
    {
        // ✅ Sende Command über MediatR
        var command = new CreateDocumentCommand 
        { 
            Title = doc.Title, 
            Content = doc.Content 
        };
        var documentId = await _mediator.Send(command);
    }
}

// 2. Handler (Application Layer)
public class CreateDocumentCommandHandler 
    : IRequestHandler<CreateDocumentCommand, string>
{
    private readonly IDocumentService _documentService;
    private readonly ILogger<CreateDocumentCommandHandler> _logger;

    public async Task<string> Handle(CreateDocumentCommand request, CancellationToken ct)
    {
        _logger.LogInformation("Creating document: {Title}", request.Title);
        
        // ✅ Handler ruft Service auf (nicht ViewModel!)
        var document = await _documentService.CreateDocumentAsync(new Document 
        {
            Title = request.Title,
            Content = request.Content
        });
        
        return document.Id;
    }
}

// 3. Service (Infrastructure Layer)
public class DocumentService : IDocumentService
{
    private readonly IThemisApiClient _apiClient;

    public async Task<Document> CreateDocumentAsync(Document doc)
    {
        // ✅ Service kapselt API-Zugriff
        return await _apiClient.PutAsync<Document>($"/entities/documents:{doc.Id}", doc);
    }
}
```

### ⚠️ AKZEPTABEL (aber nicht ideal): Service → Service

```csharp
// InboxService ruft NotificationService direkt auf
public class InboxService : IInboxService
{
    private readonly INotificationService _notificationService;

    public async Task<InboxItem> CreateInboxItemAsync(InboxItem item)
    {
        // Inbox-Item erstellen
        await _apiClient.PutAsync(...);
        
        // ⚠️ Direkte Service-zu-Service Kopplung
        await _notificationService.NotifyAsync(item.AssignedTo, "Neue Inbox-Aufgabe");
        
        return item;
    }
}
```

**Besseres Pattern:**
```csharp
// Handler koordiniert mehrere Services
public class CreateInboxItemHandler : IRequestHandler<CreateInboxItemCommand, string>
{
    private readonly IInboxService _inboxService;
    private readonly INotificationService _notificationService;
    private readonly IMediator _mediator;

    public async Task<string> Handle(CreateInboxItemCommand request, CancellationToken ct)
    {
        // ✅ Handler orchestriert
        var item = await _inboxService.CreateInboxItemAsync(...);
        await _notificationService.NotifyAsync(...);
        
        // ✅ Publish Domain Event
        await _mediator.Publish(new InboxItemCreatedEvent(item.Id));
        
        return item.Id;
    }
}
```

---

## 🎯 Bewertung nach CQRS-Prinzipien

| Layer | MediatR Usage | Status | Bewertung |
|-------|---------------|--------|-----------|
| **ViewModels** | ✅ Ja (IMediator) | Vollständig | ⭐⭐⭐⭐⭐ Exzellent |
| **Commands** | ✅ Ja (IRequest\<T\>) | 19 Commands | ⭐⭐⭐⭐⭐ Exzellent |
| **Queries** | ✅ Ja (IRequest\<T\>) | 20 Queries | ⭐⭐⭐⭐⭐ Exzellent |
| **Handlers** | ✅ Ja (IRequestHandler) | Auto-registriert | ⭐⭐⭐⭐⭐ Exzellent |
| **Events** | ✅ Ja (INotification) | 2 Events | ⭐⭐⭐⭐ Gut (ausbaubar) |
| **Services** | ❌ Nein (direkte DI) | 50+ Services | ⭐⭐⭐ Akzeptabel |

---

## 🚀 Empfehlungen

### 1. **Aktueller Zustand: Gut** ✅
- Application Layer ist **vollständig CQRS-konform**
- ViewModels sind **sauber entkoppelt**
- Handlers koordinieren Business Logic
- Services kapseln Infrastructure

### 2. **Optional: Service-Entkopplung verbessern** ⚠️

**Problem:**
Services rufen direkt andere Services auf (z.B. `InboxService → NotificationService`).

**Lösung 1: Handler koordiniert (empfohlen):**
```csharp
public class CreateInboxItemHandler
{
    private readonly IInboxService _inbox;
    private readonly INotificationService _notification;
    
    public async Task Handle(...)
    {
        var item = await _inbox.CreateInboxItemAsync(...);
        await _notification.NotifyAsync(...);
    }
}
```

**Lösung 2: Domain Events (fortgeschritten):**
```csharp
// Service publiziert Event
public class InboxService
{
    private readonly IMediator _mediator;
    
    public async Task<InboxItem> CreateInboxItemAsync(...)
    {
        // ... Item erstellen ...
        await _mediator.Publish(new InboxItemCreatedEvent(item));
    }
}

// Notification-Handler reagiert
public class InboxItemCreatedHandler : INotificationHandler<InboxItemCreatedEvent>
{
    private readonly INotificationService _notification;
    
    public async Task Handle(InboxItemCreatedEvent evt, CancellationToken ct)
    {
        await _notification.NotifyAsync(...);
    }
}
```

### 3. **Mehr Domain Events einführen** 📢

**Aktuell:** Nur 2 Events (`DocumentCreatedEvent`, `TestDataGeneratedEvent`)

**Empfohlene Events:**
```csharp
// Document Lifecycle
- DocumentCreatedEvent ✅
- DocumentUpdatedEvent ⏳
- DocumentDeletedEvent ⏳
- DocumentPublishedEvent ⏳

// Collaboration
- DocumentCheckedOutEvent ⏳
- DocumentCheckedInEvent ⏳
- CommentAddedEvent ⏳

// Workflow
- InboxItemCreatedEvent ⏳
- TaskAssignedEvent ⏳
- ReminderDueEvent ⏳

// Search & Classification
- SearchCompletedEvent ⏳
- DocumentClassifiedEvent ⏳
```

**Vorteile:**
- Lose Kopplung zwischen Modulen
- Audit-Trail automatisch
- Benachrichtigungen deklarativ
- Erweiterbarkeit ohne Code-Änderung

### 4. **Plugin-System über MediatR** 🔌

**Konzept:** Plugins registrieren sich als Event-Handler

```csharp
// Core publiziert Event
await _mediator.Publish(new DocumentCreatedEvent(documentId));

// Plugin reagiert (ohne Core-Änderung)
public class OcrPlugin : INotificationHandler<DocumentCreatedEvent>
{
    public async Task Handle(DocumentCreatedEvent evt, CancellationToken ct)
    {
        // OCR automatisch starten
        await PerformOcrAsync(evt.DocumentId);
    }
}

// Auto-Classification Plugin
public class ClassificationPlugin : INotificationHandler<DocumentCreatedEvent>
{
    public async Task Handle(DocumentCreatedEvent evt, CancellationToken ct)
    {
        await ClassifyDocumentAsync(evt.DocumentId);
    }
}
```

**Registrierung:**
```csharp
// Plugins laden und automatisch als Handler registrieren
services.AddMediatR(cfg => 
{
    cfg.RegisterServicesFromAssembly(typeof(OcrPlugin).Assembly);
    cfg.RegisterServicesFromAssembly(typeof(ClassificationPlugin).Assembly);
});
```

---

## 📚 Dokumentations-Referenzen

- **EVENTS.md**: Event-basierte Kommunikation (bereits vorhanden)
- **CLEAN_ARCHITECTURE_README.md**: Clean Architecture Pattern
- **PHASE_2_EXECUTIVE_SUMMARY.md**: Collaboration Features mit CQRS
- **VIS_INTEGRATION_GUIDE.md**: VIS-Feature Integration mit MediatR
- **PHASE1_SPRINT1_PLAN.md**: Initial CQRS Implementation

---

## 🎓 Learnings & Best Practices

### ✅ Was funktioniert gut:

1. **Strikte Layer-Trennung**
   - ViewModels kennen nur IMediator
   - Handlers orchestrieren Business Logic
   - Services kapseln Infrastructure

2. **CQRS Separation of Concerns**
   - Commands für Schreiboperationen
   - Queries für Leseoperationen
   - Klare Verantwortlichkeiten

3. **Testbarkeit**
   - Handlers sind isoliert testbar
   - Services sind mockbar
   - Commands/Queries sind DTOs (einfach zu testen)

4. **Automatische Handler-Registrierung**
   - Assembly-Scan via MediatR
   - Keine manuelle Registrierung nötig

### ⚠️ Verbesserungspotenzial:

1. **Service-zu-Service Kopplung**
   - Aktuell: Services rufen direkt andere Services auf
   - Besser: Handlers koordinieren mehrere Services

2. **Mehr Domain Events**
   - Aktuell: Nur 2 Events
   - Potenzial: 20+ Events für Lifecycle, Workflow, Collaboration

3. **Plugin-System**
   - MediatR ideal für Hot-Reload Plugins
   - Event-basierte Plugin-Architektur

---

## ✅ Fazit

**Status: ARCHITEKTUR IST SOLIDE** ✅

Die Themis.DocumentManager Anwendung nutzt MediatR **korrekt und konsequent** auf Application Layer und ViewModel-Ebene:

- ✅ **ViewModels** sind vollständig entkoppelt über IMediator
- ✅ **Application Layer** implementiert CQRS mit Commands/Queries/Handlers
- ✅ **Events** ermöglichen lose Kopplung (ausbaubar)
- ⚠️ **Services** nutzen direkte DI (akzeptabel für Infrastructure Layer)

**Empfehlung:**
- ✅ **Keine Refactoring-Notwendigkeit** - Architektur ist sauber
- 💡 **Optional:** Service-zu-Service Calls durch Handler-Orchestrierung ersetzen
- 💡 **Optional:** Mehr Domain Events für bessere Modularität
- 💡 **Optional:** Plugin-System über MediatR-Events realisieren

---

**Audit abgeschlossen:** 2025-12-11  
**Status:** ✅ CQRS-Architektur erfolgreich implementiert  
**Nächster Schritt:** Optional - Service-Entkopplung verbessern + Mehr Domain Events
