# Clean Architecture Implementation - Phase 1, Sprint 1

**Datum:** 2025-12-10  
**Status:** ✅ Sprint 1 abgeschlossen

---

## Übersicht

Phase 1 der Roadmap zur systematischen Weiterentwicklung des Themis.DocumentManager ist gestartet. Sprint 1 etabliert die **Clean Architecture Foundation** mit CQRS Pattern.

---

## Neue Projekt-Struktur

```
Themis.DocumentManager/
├── Domain/                          # Business Logic Layer
│   ├── Events/
│   │   └── DocumentCreatedEvent.cs
│   └── Exceptions/
│       └── DocumentNotFoundException.cs
│
├── Application/                     # Use Cases Layer (CQRS)
│   ├── Common/
│   │   └── Interfaces/
│   │       └── IThemisRepository.cs
│   └── Documents/
│       ├── Commands/
│       │   └── CreateDocument/
│       │       ├── CreateDocumentCommand.cs
│       │       ├── CreateDocumentCommandHandler.cs
│       │       └── CreateDocumentCommandValidator.cs
│       └── Queries/
│           ├── GetDocument/
│           │   ├── GetDocumentQuery.cs
│           │   └── GetDocumentQueryHandler.cs
│           └── GetDocuments/
│               ├── GetDocumentsQuery.cs
│               └── GetDocumentsQueryHandler.cs
│
├── Infrastructure/                  # External Concerns Layer
│   └── Persistence/
│       └── ThemisRepository.cs
│
├── Models/                          # (Legacy - wird migriert)
├── Services/                        # (Legacy - wird migriert)
├── ViewModels/                      # Presentation Layer
└── Views/                           # UI Layer
```

---

## Implementierte Features

### 1. CQRS Pattern mit MediatR

**Commands (Write Operations):**
```csharp
// Verwendung in ViewModel
var command = new CreateDocumentCommand
{
    Title = "Vertrag.pdf",
    Description = "Mietvertrag 2025",
    Filename = "vertrag.pdf",
    MimeType = "application/pdf",
    SizeBytes = 1024000,
    Author = "Max Mustermann",
    Category = "Vertrag"
};

var documentId = await _mediator.Send(command);
```

**Queries (Read Operations):**
```csharp
// Einzelnes Dokument abrufen
var query = new GetDocumentQuery("doc123");
var document = await _mediator.Send(query);

// Liste von Dokumenten mit Pagination
var listQuery = new GetDocumentsQuery { Page = 1, PageSize = 50 };
var documents = await _mediator.Send(listQuery);
```

---

### 2. FluentValidation

Automatische Validierung aller Commands:

```csharp
public class CreateDocumentCommandValidator : AbstractValidator<CreateDocumentCommand>
{
    public CreateDocumentCommandValidator()
    {
        RuleFor(v => v.Title)
            .NotEmpty().WithMessage("Title ist erforderlich.")
            .MaximumLength(200).WithMessage("Title darf maximal 200 Zeichen lang sein.");

        RuleFor(v => v.Filename)
            .NotEmpty().WithMessage("Filename ist erforderlich.");

        RuleFor(v => v.MimeType)
            .Matches(@"^[\w.-]+/[\w.-]+$")
            .WithMessage("MimeType muss gültig sein (z.B. application/pdf).");
    }
}
```

---

### 3. Repository Pattern

Abstraktion der ThemisDB-Zugriffe:

```csharp
public interface IThemisRepository
{
    Task<string> CreateDocumentAsync(Document document, CancellationToken ct = default);
    Task<Document?> GetDocumentAsync(string id, CancellationToken ct = default);
    Task<List<Document>> GetDocumentsAsync(int page, int pageSize, CancellationToken ct = default);
    Task<bool> UpdateDocumentAsync(Document document, CancellationToken ct = default);
    Task<bool> DeleteDocumentAsync(string id, CancellationToken ct = default);
}
```

**Vorteile:**
- Backend austauschbar (ThemisDB → SharePoint → SQL Server)
- Einfaches Mocking für Unit Tests
- Klare Trennung von UI und Datenzugriff

---

### 4. Domain Events

Event-driven Architecture für lose Kopplung:

```csharp
// Event wird automatisch published nach Document-Erstellung
public record DocumentCreatedEvent(string DocumentId, string Title, DateTime CreatedAt) 
    : INotification;

// Event Handler können reagieren (z.B. Email-Benachrichtigung)
public class SendEmailOnDocumentCreatedHandler 
    : INotificationHandler<DocumentCreatedEvent>
{
    public async Task Handle(DocumentCreatedEvent evt, CancellationToken ct)
    {
        // Send email notification
    }
}
```

---

## Dependency Injection Setup

In `App.xaml.cs`:

```csharp
// MediatR for CQRS
services.AddMediatR(cfg => 
    cfg.RegisterServicesFromAssembly(Assembly.GetExecutingAssembly()));

// FluentValidation
services.AddValidatorsFromAssembly(Assembly.GetExecutingAssembly());

// Repository Pattern
services.AddSingleton<IThemisRepository, ThemisRepository>();
```

---

## Migration Pfad (Bestehende Services)

**Aktuell:**
```csharp
// Direkte Service-Aufrufe in ViewModel
var document = await _documentService.GetDocumentAsync(id);
```

**Neu (CQRS):**
```csharp
// Command/Query Pattern über MediatR
var query = new GetDocumentQuery(id);
var document = await _mediator.Send(query);
```

**Migration Schritte:**
1. ViewModels injizieren `IMediator` statt konkreter Services
2. Service-Aufrufe schrittweise zu Commands/Queries konvertieren
3. Alte Services als Adapter-Pattern beibehalten (temporär)
4. Schrittweise alte Services entfernen

---

## Vorteile der neuen Architektur

### Testbarkeit
```csharp
[Fact]
public async Task CreateDocument_ValidInput_ReturnsDocumentId()
{
    // Arrange
    var mockRepo = new Mock<IThemisRepository>();
    mockRepo.Setup(x => x.CreateDocumentAsync(It.IsAny<Document>(), default))
        .ReturnsAsync("doc123");
    
    var handler = new CreateDocumentCommandHandler(mockRepo.Object, _mediator);
    var command = new CreateDocumentCommand { Title = "Test", ... };
    
    // Act
    var result = await handler.Handle(command, default);
    
    // Assert
    Assert.Equal("doc123", result);
}
```

### Wartbarkeit
- **Single Responsibility:** Jeder Handler hat genau eine Aufgabe
- **Separation of Concerns:** UI, Business Logic, Data Access getrennt
- **Open/Closed:** Neue Features als neue Commands/Queries hinzufügen

### Flexibilität
- Backend austauschbar (ThemisDB → SQL Server → SharePoint)
- Neue Features ohne Änderung bestehenden Codes
- Event-driven für lose Kopplung

---

## Nächste Schritte (Sprint 2)

1. **Weitere Commands/Queries:**
   - UpdateDocumentCommand
   - DeleteDocumentCommand
   - SearchDocumentsQuery

2. **Validation Pipeline Behavior:**
   - Automatische Validierung in MediatR Pipeline
   - Zentrale Exception-Behandlung

3. **Unit Tests:**
   - Command/Query Handler Tests
   - Validator Tests
   - Repository Tests
   - Ziel: 30% Coverage

4. **Logging:**
   - Serilog Integration
   - Structured Logging für alle Commands/Queries

5. **Migration:**
   - 3-5 bestehende Services zu CQRS migrieren
   - ViewModels auf IMediator umstellen

---

## Ressourcen

- **Vollständige Roadmap:** `docs/DEVELOPMENT_STRATEGY_SUMMARY.md`
- **Best Practices:** `docs/REFERENCE_IMPLEMENTATIONS_AND_BEST_PRACTICES.md`
- **Sprint 1 Plan:** `PHASE1_SPRINT1_PLAN.md`

---

**Erstellt:** 2025-12-10  
**Version:** 1.0 - Sprint 1 Foundation
