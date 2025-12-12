# Themis.DocumentManager - Architektur-Dokumentation

**Version:** 1.0 (Production Ready)  
**Architektur-Pattern:** Clean Architecture mit CQRS  
**Letzte Aktualisierung:** 11. Dezember 2025

---

## 🎯 Architektur-Übersicht

Themis.DocumentManager folgt **Clean Architecture** Prinzipien mit strikter Layer-Trennung und **CQRS** (Command Query Responsibility Segregation) via **MediatR**.

### Architektur-Diagramm

```
┌─────────────────────────────────────────────────────────────┐
│                  Presentation Layer (WPF)                   │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │  MainWindow │  │  ViewModels  │  │  User Controls   │   │
│  │   (XAML)    │  │  (MVVM)      │  │  (XAML)          │   │
│  └──────┬──────┘  └──────┬───────┘  └──────┬───────────┘   │
│         │                │                  │               │
│         └────────────────┼──────────────────┘               │
│                          ↓                                  │
│                    IMediator ✅                             │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│              Application Layer (Use Cases)                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Commands (Write Operations)                         │   │
│  │  • CreateDocumentCommand                             │   │
│  │  • UpdateDocumentCommand                             │   │
│  │  • CheckOutDocumentCommand                           │   │
│  │  • AddCommentCommand                                 │   │
│  │  IRequest<TResponse>                                 │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Queries (Read Operations)                           │   │
│  │  • GetDocumentQuery                                  │   │
│  │  • GetDocumentsQuery                                 │   │
│  │  • GetDocumentLockStatusQuery                        │   │
│  │  IRequest<TResponse>                                 │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Handlers (Orchestration)                            │   │
│  │  • CreateDocumentCommandHandler                      │   │
│  │  • GetDocumentQueryHandler                           │   │
│  │  IRequestHandler<TRequest, TResponse>                │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Events (Notifications)                              │   │
│  │  • DocumentCreatedEvent                              │   │
│  │  • TestDataGeneratedEvent                            │   │
│  │  INotification                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Validation (FluentValidation)                       │   │
│  │  • CreateDocumentCommandValidator                    │   │
│  │  • Automatic Pipeline Integration                    │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│                    Domain Layer                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Entities (Business Logic)                           │   │
│  │  • Document                                          │   │
│  │  • DocumentLock                                      │   │
│  │  • Comment                                           │   │
│  │  • Process                                           │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Value Objects                                       │   │
│  │  • GeoLocation                                       │   │
│  │  • DocumentPosition                                  │   │
│  │  • BoundingBox                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Domain Events                                       │   │
│  │  • DocumentCreatedEvent                              │   │
│  │  • TestDataGeneratedEvent                            │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│              Infrastructure Layer                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Services (50+ Services)                             │   │
│  │  • ThemisApiClient - REST API                        │   │
│  │  • DocumentService - CRUD Operations                 │   │
│  │  • SearchService - Fulltext + Vector                 │   │
│  │  • GeoService - Geo-Queries                          │   │
│  │  • TimelineService - Event Tracking                  │   │
│  │  • OfficeIntegrationService - COM Interop            │   │
│  │  • RevisionService - Versioning                      │   │
│  │  • OllamaContentGeneratorService - LLM               │   │
│  │  • StatusMonitorService - Health Monitoring          │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  External Dependencies                               │   │
│  │  • ThemisDB (REST API)                               │   │
│  │  • Microsoft Office (COM Interop)                    │   │
│  │  • DirectX 11 (P/Invoke)                             │   │
│  │  • SignalR (WebSocket)                               │   │
│  │  • Ollama LLM (HTTP API)                             │   │
│  │  • OpenStreetMap (Tile Server)                       │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 🏗️ Layer-Beschreibung

### 1. Presentation Layer (WPF)

**Verantwortlichkeit:** User Interface, User Interaction

**Komponenten:**
- **Views (XAML):** UI-Deklaration, Databinding
- **ViewModels (MVVM):** Presentation Logic, INotifyPropertyChanged
- **User Controls:** Wiederverwendbare UI-Komponenten
- **Value Converters:** Databinding-Transformationen

**Pattern:**
- **MVVM** (Model-View-ViewModel)
- **Databinding** (Two-Way, OneWay)
- **Commands** (RelayCommand, AsyncRelayCommand)

**Dependency Direction:**
```
Views → ViewModels → IMediator (Application Layer)
```

**Beispiel (ViewModel):**
```csharp
public class DocumentBrowserViewModel : ObservableObject
{
    private readonly IMediator _mediator;
    
    public DocumentBrowserViewModel(IMediator mediator)
    {
        _mediator = mediator;
    }
    
    [RelayCommand]
    private async Task LoadDocumentsAsync()
    {
        // ✅ ViewModel ruft KEINE Services direkt auf
        // ✅ Kommunikation über MediatR Query
        var query = new GetDocumentsQuery();
        var documents = await _mediator.Send(query);
        Documents = new ObservableCollection<Document>(documents);
    }
    
    [RelayCommand]
    private async Task CreateDocumentAsync(Document doc)
    {
        // ✅ Command über MediatR senden
        var command = new CreateDocumentCommand 
        { 
            Title = doc.Title, 
            Content = doc.Content 
        };
        var documentId = await _mediator.Send(command);
    }
}
```

**Keine direkten Service-Aufrufe:**
```csharp
// ❌ FALSCH: Direkte Service-Kopplung
private readonly IDocumentService _documentService;
var doc = await _documentService.CreateDocumentAsync(...);

// ✅ RICHTIG: MediatR Command
var command = new CreateDocumentCommand { ... };
var docId = await _mediator.Send(command);
```

---

### 2. Application Layer (Use Cases)

**Verantwortlichkeit:** Orchestrierung, Business Workflows, Validation

**Pattern:** CQRS (Command Query Responsibility Segregation)

#### 2.1 Commands (Write Operations)

**Konzept:** Commands ändern State (Create, Update, Delete)

**Struktur:**
```
Application/Documents/Commands/CreateDocument/
├── CreateDocumentCommand.cs           # Command DTO (IRequest<T>)
├── CreateDocumentCommandHandler.cs    # Handler (IRequestHandler<T,R>)
└── CreateDocumentCommandValidator.cs  # Validation (FluentValidation)
```

**Beispiel (Command):**
```csharp
public record CreateDocumentCommand : IRequest<string>
{
    public string Title { get; init; } = string.Empty;
    public string Content { get; init; } = string.Empty;
    public Dictionary<string, object> Metadata { get; init; } = new();
}
```

**Beispiel (Handler):**
```csharp
public class CreateDocumentCommandHandler 
    : IRequestHandler<CreateDocumentCommand, string>
{
    private readonly IDocumentService _documentService;
    private readonly IMediator _mediator;
    private readonly ILogger<CreateDocumentCommandHandler> _logger;
    
    public CreateDocumentCommandHandler(
        IDocumentService documentService,
        IMediator mediator,
        ILogger<CreateDocumentCommandHandler> logger)
    {
        _documentService = documentService;
        _mediator = mediator;
        _logger = logger;
    }
    
    public async Task<string> Handle(
        CreateDocumentCommand request, 
        CancellationToken cancellationToken)
    {
        _logger.LogInformation("Creating document: {Title}", request.Title);
        
        // ✅ Handler ruft Service auf (nicht ViewModel!)
        var document = new Document
        {
            Title = request.Title,
            Content = request.Content,
            Metadata = request.Metadata
        };
        
        var createdDoc = await _documentService.CreateDocumentAsync(document);
        
        // ✅ Domain Event publizieren
        await _mediator.Publish(
            new DocumentCreatedEvent(createdDoc.Id, DateTime.UtcNow),
            cancellationToken
        );
        
        return createdDoc.Id;
    }
}
```

**Beispiel (Validator):**
```csharp
public class CreateDocumentCommandValidator 
    : AbstractValidator<CreateDocumentCommand>
{
    public CreateDocumentCommandValidator()
    {
        RuleFor(x => x.Title)
            .NotEmpty()
            .MaximumLength(500);
            
        RuleFor(x => x.Content)
            .NotEmpty();
    }
}
```

**Implementierte Commands (19+):**
- `CreateDocumentCommand`, `UpdateDocumentCommand`, `DeleteDocumentCommand`
- `CheckOutDocumentCommand`, `CheckInDocumentCommand`, `ReleaseDocumentLockCommand`
- `AddCommentCommand`, `UpdateCommentCommand`, `DeleteCommentCommand`
- `AddToFavoritesCommand`, `RemoveFromFavoritesCommand`
- `CreateInboxItemCommand`, `CreateReminderCommand`
- `ApproveCosigningStepCommand`

#### 2.2 Queries (Read Operations)

**Konzept:** Queries lesen State (Get, List, Search)

**Struktur:**
```
Application/Documents/Queries/GetDocument/
├── GetDocumentQuery.cs           # Query DTO (IRequest<T>)
└── GetDocumentQueryHandler.cs    # Handler (IRequestHandler<T,R>)
```

**Beispiel (Query):**
```csharp
public record GetDocumentQuery(string DocumentId) : IRequest<Document?>;
```

**Beispiel (Handler):**
```csharp
public class GetDocumentQueryHandler 
    : IRequestHandler<GetDocumentQuery, Document?>
{
    private readonly IDocumentService _documentService;
    
    public GetDocumentQueryHandler(IDocumentService documentService)
    {
        _documentService = documentService;
    }
    
    public async Task<Document?> Handle(
        GetDocumentQuery request, 
        CancellationToken cancellationToken)
    {
        return await _documentService.GetDocumentByIdAsync(request.DocumentId);
    }
}
```

**Implementierte Queries (20+):**
- `GetDocumentQuery`, `GetDocumentsQuery`
- `GetDocumentLockStatusQuery`, `GetActiveLocksQuery`
- `GetDocumentCommentsQuery`, `GetCommentQuery`
- `GetFavoritesQuery`, `IsFavoriteQuery`
- `GetInboxItemsQuery`, `GetMyTasksQuery`
- `GetNavigationPathQuery`, `GetRelatedEntitiesQuery`
- `GetDueRemindersQuery`

#### 2.3 Events (Notifications)

**Konzept:** Events für Loose Coupling zwischen Modulen

**Beispiel (Event):**
```csharp
public record DocumentCreatedEvent(string DocumentId, DateTime Timestamp) 
    : INotification;
```

**Beispiel (Handler 1 - Logging):**
```csharp
public class DocumentCreatedEventHandler 
    : INotificationHandler<DocumentCreatedEvent>
{
    private readonly ILogger<DocumentCreatedEventHandler> _logger;
    
    public async Task Handle(
        DocumentCreatedEvent notification, 
        CancellationToken cancellationToken)
    {
        _logger.LogInformation(
            "Document created: {DocumentId} at {Timestamp}",
            notification.DocumentId,
            notification.Timestamp
        );
    }
}
```

**Beispiel (Handler 2 - UI Update):**
```csharp
public class DocumentBrowserViewModel 
    : ObservableObject, 
      INotificationHandler<DocumentCreatedEvent>
{
    public async Task Handle(
        DocumentCreatedEvent notification, 
        CancellationToken cancellationToken)
    {
        // ✅ UI automatisch aktualisieren
        await LoadDocumentsAsync();
    }
}
```

**Vorteile:**
- ✅ Loose Coupling (Handler kennen sich nicht)
- ✅ Erweiterbar (neue Handler ohne Code-Änderung)
- ✅ Testbar (Events isoliert testbar)

**Implementierte Events:**
- `DocumentCreatedEvent`
- `TestDataGeneratedEvent`

**Potenzial für weitere Events:**
- `DocumentUpdatedEvent`, `DocumentDeletedEvent`
- `DocumentCheckedOutEvent`, `DocumentCheckedInEvent`
- `CommentAddedEvent`, `TaskAssignedEvent`

#### 2.4 Validation Pipeline

**MediatR Pipeline Behavior:**
```csharp
services.AddMediatR(cfg =>
{
    cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly());
    cfg.AddOpenBehavior(typeof(ValidationBehavior<,>)); // Auto-Validation
});
```

**Automatische Validierung BEFORE Handler:**
```csharp
public class ValidationBehavior<TRequest, TResponse> 
    : IPipelineBehavior<TRequest, TResponse>
    where TRequest : IRequest<TResponse>
{
    private readonly IEnumerable<IValidator<TRequest>> _validators;
    
    public async Task<TResponse> Handle(
        TRequest request,
        RequestHandlerDelegate<TResponse> next,
        CancellationToken cancellationToken)
    {
        if (_validators.Any())
        {
            var context = new ValidationContext<TRequest>(request);
            var validationResults = await Task.WhenAll(
                _validators.Select(v => v.ValidateAsync(context, cancellationToken))
            );
            
            var failures = validationResults
                .SelectMany(r => r.Errors)
                .Where(f => f != null)
                .ToList();
            
            if (failures.Count != 0)
            {
                throw new ValidationException(failures);
            }
        }
        
        return await next();
    }
}
```

---

### 3. Domain Layer

**Verantwortlichkeit:** Business Logic, Domain Rules

**Komponenten:**

#### 3.1 Entities

**Beispiel (Document Entity):**
```csharp
public class Document
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:document:{Id}";
    
    public string Title { get; set; } = string.Empty;
    public string Content { get; set; } = string.Empty;
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime ModifiedAt { get; set; } = DateTime.UtcNow;
    
    // Multi-Model Properties
    public GeoLocation? GeoLocation { get; set; }
    public List<DocumentRelation> Relations { get; set; } = new();
    public List<TimelineEvent> TimelineEvents { get; set; } = new();
    
    // Business Logic
    public bool IsLocked() => LockInfo != null && LockInfo.IsActive();
    public bool CanBeEditedBy(string userId) 
        => !IsLocked() || LockInfo?.UserId == userId;
}
```

**Weitere Entities:**
- `DocumentLock`, `Comment`, `UserPresence` (Collaboration)
- `Process`, `File`, `Subfile` (Administrative Structure)
- `InboxItem`, `Reminder`, `CosigningStep` (VIS Features)

#### 3.2 Value Objects

**Beispiel (GeoLocation):**
```csharp
public record GeoLocation(double Latitude, double Longitude)
{
    public double DistanceTo(GeoLocation other)
    {
        // Haversine Distance
        const double R = 6371; // km
        var dLat = ToRadians(other.Latitude - Latitude);
        var dLon = ToRadians(other.Longitude - Longitude);
        var a = Math.Sin(dLat/2) * Math.Sin(dLat/2) +
                Math.Cos(ToRadians(Latitude)) * 
                Math.Cos(ToRadians(other.Latitude)) *
                Math.Sin(dLon/2) * Math.Sin(dLon/2);
        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1-a));
        return R * c;
    }
    
    private static double ToRadians(double degrees) 
        => degrees * Math.PI / 180;
}
```

**Weitere Value Objects:**
- `DocumentPosition` (X, Y für Annotation)
- `BoundingBox` (GeoFence)

#### 3.3 Domain Events

**Siehe:** Application Layer → Events

---

### 4. Infrastructure Layer

**Verantwortlichkeit:** External Dependencies, I/O, API Clients

**Komponenten:**

#### 4.1 Services (50+ Services)

**Beispiel (DocumentService):**
```csharp
public class DocumentService : IDocumentService
{
    private readonly IThemisApiClient _apiClient;
    
    public DocumentService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }
    
    public async Task<Document> CreateDocumentAsync(Document document)
    {
        // ✅ Service kapselt API-Zugriff
        var response = await _apiClient.PutAsync<DocumentRequest, DocumentResponse>(
            $"/entities/documents:{document.Id}",
            new DocumentRequest { Blob = JsonSerializer.Serialize(document) }
        );
        
        return document;
    }
    
    public async Task<Document?> GetDocumentByIdAsync(string id)
    {
        return await _apiClient.GetAsync<Document>($"/entities/documents:{id}");
    }
}
```

**Service-Kategorien:**

**Core Services:**
- `DocumentService`, `SearchService`, `MetadataService`
- `GeoService`, `TimelineService`, `VectorService`, `GraphService`

**Workflow Services:**
- `InboxService`, `ReminderService`, `CosigningService`
- `ProcessLogService`, `FilingPlanService`, `NotificationService`

**Compliance Services:**
- `FourEyesPrincipleService`, `FileAccessLogService`
- `SubstitutionService`, `EGovService`, `TransferNoteService`

**Integration Services:**
- `ThemisApiClient` (REST API)
- `OfficeIntegrationService` (COM Interop)
- `SignalRService` (WebSocket)
- `OllamaContentGeneratorService` (LLM)
- `StatusMonitorService` (Health Checks)

#### 4.2 External Dependencies

**ThemisDB (REST API):**
```csharp
public class ThemisApiClient : IThemisApiClient
{
    private readonly HttpClient _httpClient;
    
    public async Task<T?> GetAsync<T>(string endpoint)
    {
        var response = await _httpClient.GetAsync(endpoint);
        response.EnsureSuccessStatusCode();
        return await response.Content.ReadFromJsonAsync<T>();
    }
}
```

**Office COM Interop:**
```csharp
public class OfficeIntegrationService : IOfficeIntegrationService
{
    public async Task OpenInWordAsync(string filePath)
    {
        var wordType = Type.GetTypeFromProgID("Word.Application");
        dynamic wordApp = Activator.CreateInstance(wordType);
        wordApp.Visible = true;
        
        dynamic doc = wordApp.Documents.Open(filePath);
        doc.TrackRevisions = true; // ✅ Revisionssicherheit
    }
}
```

**DirectX 11:**
```csharp
public class DirectXDevice
{
    [DllImport("d3d11.dll")]
    private static extern int D3D11CreateDevice(...);
    
    public bool Initialize(IntPtr windowHandle)
    {
        // P/Invoke DirectX API
        var hr = D3D11CreateDevice(...);
        return hr == 0;
    }
}
```

---

## 🔄 CQRS Pattern mit MediatR

### MediatR Konfiguration

**App.xaml.cs:**
```csharp
// Dependency Injection Setup
public App()
{
    var services = new ServiceCollection();
    
    // MediatR für CQRS
    services.AddMediatR(cfg => 
        cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly()));
    
    // FluentValidation
    services.AddValidatorsFromAssembly(Assembly.GetExecutingAssembly());
    
    // Services registrieren
    services.AddSingleton<IThemisApiClient, ThemisApiClient>();
    services.AddSingleton<IDocumentService, DocumentService>();
    // ... 50+ Services
    
    // ViewModels registrieren
    services.AddTransient<MainViewModel>();
    services.AddTransient<DocumentBrowserViewModel>();
    // ... 30+ ViewModels
    
    ServiceProvider = services.BuildServiceProvider();
}
```

### Command Flow

```
┌──────────────┐
│   ViewModel  │  CreateDocumentAsync()
└──────┬───────┘
       │ _mediator.Send(command)
       ↓
┌──────────────┐
│   MediatR    │  Pipeline: Validation → Handler → Post-Processing
└──────┬───────┘
       │ Validation via FluentValidation
       ↓
┌──────────────┐
│  Validator   │  CreateDocumentCommandValidator
└──────┬───────┘
       │ Valid ✅
       ↓
┌──────────────┐
│   Handler    │  CreateDocumentCommandHandler
└──────┬───────┘
       │ _documentService.CreateDocumentAsync()
       ↓
┌──────────────┐
│   Service    │  DocumentService → ThemisApiClient
└──────┬───────┘
       │ HTTP POST /entities/documents:...
       ↓
┌──────────────┐
│  ThemisDB    │  Store Document
└──────┬───────┘
       │ Return DocumentId
       ↓
┌──────────────┐
│   Handler    │  _mediator.Publish(DocumentCreatedEvent)
└──────┬───────┘
       │ Event publizieren
       ↓
┌──────────────┐
│Event Handlers│  Logging, UI Update, Timeline
└──────────────┘
```

### Query Flow

```
┌──────────────┐
│   ViewModel  │  LoadDocumentsAsync()
└──────┬───────┘
       │ _mediator.Send(query)
       ↓
┌──────────────┐
│   MediatR    │  Route to Handler
└──────┬───────┘
       ↓
┌──────────────┐
│   Handler    │  GetDocumentsQueryHandler
└──────┬───────┘
       │ _documentService.GetAllDocumentsAsync()
       ↓
┌──────────────┐
│   Service    │  DocumentService → ThemisApiClient
└──────┬───────┘
       │ HTTP POST /query/aql
       ↓
┌──────────────┐
│  ThemisDB    │  AQL Query: FOR doc IN documents RETURN doc
└──────┬───────┘
       │ Return List<Document>
       ↓
┌──────────────┐
│   ViewModel  │  Documents = new ObservableCollection<Document>(result)
└──────────────┘
```

---

## 🎯 Design Principles

### SOLID Principles

#### Single Responsibility Principle (SRP)
✅ **Jede Klasse hat genau eine Verantwortung**
```csharp
// ✅ RICHTIG: Service nur für Dokument-CRUD
public class DocumentService : IDocumentService
{
    public Task<Document> CreateDocumentAsync(Document doc);
    public Task<Document?> GetDocumentByIdAsync(string id);
    public Task<Document> UpdateDocumentAsync(Document doc);
    public Task<bool> DeleteDocumentAsync(string id);
}

// ✅ RICHTIG: Separater Service für Suche
public class SearchService : ISearchService
{
    public Task<List<Document>> FullTextSearchAsync(string query);
    public Task<List<Document>> VectorSearchAsync(string query);
}
```

#### Open/Closed Principle (OCP)
✅ **Erweiterbar ohne Änderung**
```csharp
// ✅ Neue Commands hinzufügen ohne bestehende zu ändern
public record UpdateDocumentCommand(...) : IRequest<Document>;

// ✅ Neue Event-Handler ohne Core-Code zu ändern
public class NewFeatureEventHandler : INotificationHandler<DocumentCreatedEvent>
{
    // Automatisch von MediatR registriert
}
```

#### Liskov Substitution Principle (LSP)
✅ **Interfaces können ausgetauscht werden**
```csharp
// ✅ DocumentService ist austauschbar
IDocumentService service = useThemisDB 
    ? new DocumentService(apiClient)
    : new MockDocumentService();
```

#### Interface Segregation Principle (ISP)
✅ **Kleine, fokussierte Interfaces**
```csharp
// ✅ Separate Interfaces statt ein großes
public interface IDocumentService { ... }
public interface ISearchService { ... }
public interface IMetadataService { ... }

// ❌ FALSCH: Ein riesiges Interface
public interface IDocumentManager 
{
    // 50+ Methoden - zu groß!
}
```

#### Dependency Inversion Principle (DIP)
✅ **Abhängig von Abstraktionen, nicht Konkretionen**
```csharp
// ✅ RICHTIG: Abhängig von IDocumentService
public class CreateDocumentCommandHandler
{
    private readonly IDocumentService _documentService; // Interface!
    
    public CreateDocumentCommandHandler(IDocumentService documentService)
    {
        _documentService = documentService;
    }
}

// ❌ FALSCH: Abhängig von konkreter Klasse
private readonly DocumentService _documentService; // Konkrete Klasse!
```

### Clean Architecture Regeln

#### Dependency Rule
✅ **Abhängigkeiten zeigen nur nach innen**
```
Presentation → Application → Domain ← Infrastructure
```

**Erlaubt:**
- ✅ ViewModel → IMediator (Application)
- ✅ Handler → IDocumentService (Infrastructure)
- ✅ Service → Domain Entities

**Verboten:**
- ❌ Domain → Infrastructure
- ❌ ViewModel → Service (direkt)
- ❌ Domain → Application

#### Keine zirkulären Abhängigkeiten
```csharp
// ✅ RICHTIG: Einseitige Abhängigkeit
Application → Domain (OK)

// ❌ FALSCH: Zirkuläre Abhängigkeit
Application ↔ Domain (NICHT OK)
```

---

## 📦 Dependency Injection

### Service Registration

**App.xaml.cs:**
```csharp
var services = new ServiceCollection();

// ✅ MediatR (CQRS)
services.AddMediatR(cfg => 
    cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly()));

// ✅ FluentValidation
services.AddValidatorsFromAssembly(Assembly.GetExecutingAssembly());

// ✅ Infrastructure Services
services.AddSingleton<IThemisApiClient, ThemisApiClient>(sp =>
    new ThemisApiClient(new Uri("http://localhost:8765")));

// ✅ Core Services
services.AddSingleton<IDocumentService, DocumentService>();
services.AddSingleton<ISearchService, SearchService>();
services.AddSingleton<IMetadataService, MetadataService>();
services.AddSingleton<IGeoService, GeoService>();
services.AddSingleton<ITimelineService, TimelineService>();

// ✅ Collaboration Services
services.AddSingleton<IDocumentLockingService, DocumentLockingService>();
services.AddSingleton<ICommentService, CommentService>();
services.AddSingleton<ISignalRService, SignalRService>();

// ✅ ViewModels (Transient)
services.AddTransient<MainViewModel>();
services.AddTransient<DocumentBrowserViewModel>();
services.AddTransient<AIChatViewModel>();

// ✅ DirectX Services
services.AddSingleton<IDirectX3DRenderer, EnhancedDirectX3DGraphRenderer>();
services.AddSingleton<IOSMMapManager, OSMMapManager>();

ServiceProvider = services.BuildServiceProvider();
```

### Lifetime Scopes

- **Singleton:** Ein Service für gesamte Anwendung (Services, Singletons)
- **Transient:** Neue Instanz bei jedem Request (ViewModels, Commands)
- **Scoped:** Nicht verwendet in WPF (nur Web-Apps)

---

## 🧪 Testbarkeit

### Unit Testing

**Commands/Queries:**
```csharp
[Fact]
public async Task CreateDocument_ValidInput_ReturnsDocumentId()
{
    // Arrange
    var mockService = new Mock<IDocumentService>();
    mockService.Setup(x => x.CreateDocumentAsync(It.IsAny<Document>()))
        .ReturnsAsync(new Document { Id = "doc123" });
    
    var handler = new CreateDocumentCommandHandler(
        mockService.Object,
        Mock.Of<IMediator>(),
        Mock.Of<ILogger<CreateDocumentCommandHandler>>()
    );
    
    var command = new CreateDocumentCommand 
    { 
        Title = "Test", 
        Content = "Test Content" 
    };
    
    // Act
    var result = await handler.Handle(command, default);
    
    // Assert
    Assert.Equal("doc123", result);
}
```

**ViewModels:**
```csharp
[Fact]
public async Task LoadDocuments_CallsMediatR()
{
    // Arrange
    var mockMediator = new Mock<IMediator>();
    mockMediator.Setup(x => x.Send(
        It.IsAny<GetDocumentsQuery>(), 
        default))
        .ReturnsAsync(new List<Document> { new Document() });
    
    var viewModel = new DocumentBrowserViewModel(mockMediator.Object);
    
    // Act
    await viewModel.LoadDocumentsCommand.ExecuteAsync(null);
    
    // Assert
    Assert.Single(viewModel.Documents);
}
```

---

## 📚 Weiterführende Dokumentation

- [CLEAN_ARCHITECTURE_README.md](CLEAN_ARCHITECTURE_README.md) - Clean Architecture Implementierung
- [../MEDIATR_ARCHITECTURE_AUDIT.md](../MEDIATR_ARCHITECTURE_AUDIT.md) - MediatR CQRS Audit
- [VIS_INTEGRATION_GUIDE.md](VIS_INTEGRATION_GUIDE.md) - VIS-Features mit CQRS
- [BEST_PRACTICES.md](BEST_PRACTICES.md) - Coding Standards
- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Testing Best Practices

---

**Architekturversion:** 1.0  
**Pattern:** Clean Architecture + CQRS  
**Letzte Aktualisierung:** 11. Dezember 2025
