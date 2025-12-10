# Phase 1 - Sprint 1: Clean Architecture Foundation + VIS Integration

**Start Date:** 2025-12-10  
**Duration:** 2 Wochen  
**Ziel:** Clean Architecture Layers etablieren + MediatR Integration + VIS Features

---

## Sprint 1 Tasks

### 1. NuGet Packages installieren ✅
- [x] MediatR (v12.2.0)
- [x] FluentValidation (v11.9.0)  
- [x] FluentValidation.DependencyInjectionExtensions (v11.9.0)

### 2. Domain Layer erstellen ✅
- [x] Domain/Events/ - DocumentCreatedEvent
- [x] Domain/Exceptions/ - DocumentNotFoundException

### 3. Application Layer erstellen ✅
- [x] Application/Common/Interfaces/ - IThemisRepository
- [x] Application/Documents/Commands/CreateDocument/ - Command, Handler, Validator
- [x] Application/Documents/Queries/GetDocument/ - Query, Handler
- [x] Application/Documents/Queries/GetDocuments/ - Query, Handler

### 4. VIS-Feature Integration (PDV_VIS_ANALYSIS.md) ✅
- [x] Application/Inbox/Commands/CreateInboxItem/ - Posteingang Command
- [x] Application/Inbox/Queries/GetInboxItems/ - Posteingang Query
- [x] Application/Reminders/Commands/CreateReminder/ - Wiedervorlage Command
- [x] Application/Reminders/Queries/GetDueReminders/ - Fällige Fristen Query
- [x] Application/Cosigning/Commands/ApproveCosigningStep/ - Mitzeichnung Command

### 5. Infrastructure Layer erstellen ✅
- [x] Infrastructure/Persistence/ThemisRepository - Repository Implementation

### 6. Erste Commands & Queries ✅
- [x] CreateDocumentCommand + Handler + Validator
- [x] GetDocumentQuery + Handler  
- [x] GetDocumentsQuery + Handler
- [x] **CreateInboxItemCommand + Handler + Validator (VIS)**
- [x] **CreateReminderCommand + Handler + Validator (VIS)**
- [x] **ApproveCosigningStepCommand + Handler + Validator (VIS)**

### 7. DI Container konfigurieren ✅
- [x] MediatR registrieren in App.xaml.cs
- [x] FluentValidation registrieren in App.xaml.cs
- [x] IThemisRepository → ThemisRepository registrieren

---

## Success Metrics Sprint 1

- ✅ Clean Architecture Struktur etabliert (Domain, Application, Infrastructure)
- ✅ MediatR konfiguriert (CQRS Pattern)
- ✅ 6 Commands/Queries implementiert (3 Dokumente + 3 VIS)
- ✅ FluentValidation aktiv
- ✅ Repository Pattern für ThemisDB implementiert
- ✅ **VIS-Features (Inbox, Reminders, Cosigning) integriert**

---

## Dateien erstellt

### Domain Layer (2 Dateien)
- Domain/Events/DocumentCreatedEvent.cs
- Domain/Exceptions/DocumentNotFoundException.cs

### Application Layer - Documents (7 Dateien)
- Application/Common/Interfaces/IThemisRepository.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommand.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommandHandler.cs
- Application/Documents/Commands/CreateDocument/CreateDocumentCommandValidator.cs
- Application/Documents/Queries/GetDocument/GetDocumentQuery.cs
- Application/Documents/Queries/GetDocument/GetDocumentQueryHandler.cs
- Application/Documents/Queries/GetDocuments/GetDocumentsQuery.cs
- Application/Documents/Queries/GetDocuments/GetDocumentsQueryHandler.cs

### Application Layer - VIS Features (10 Dateien) 🆕
**Posteingang (Inbox):**
- Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommand.cs
- Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommandHandler.cs
- Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommandValidator.cs
- Application/Inbox/Queries/GetInboxItems/GetInboxItemsQuery.cs
- Application/Inbox/Queries/GetInboxItems/GetInboxItemsQueryHandler.cs

**Wiedervorlage (Reminders):**
- Application/Reminders/Commands/CreateReminder/CreateReminderCommand.cs
- Application/Reminders/Commands/CreateReminder/CreateReminderCommandHandler.cs
- Application/Reminders/Commands/CreateReminder/CreateReminderCommandValidator.cs
- Application/Reminders/Queries/GetDueReminders/GetDueRemindersQuery.cs
- Application/Reminders/Queries/GetDueReminders/GetDueRemindersQueryHandler.cs

**Mitzeichnung (Cosigning):**
- Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommand.cs
- Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommandHandler.cs
- Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommandValidator.cs

### Infrastructure Layer (1 Datei)
- Infrastructure/Persistence/ThemisRepository.cs

### Configuration (1 Datei geändert)
- App.xaml.cs - MediatR + FluentValidation DI Setup
- Themis.DocumentManager.csproj - NuGet Packages

### Documentation (2 Dateien) 🆕
- VIS_INTEGRATION_GUIDE.md - VIS-Feature Integration Guide (7.5 KB)
- CLEAN_ARCHITECTURE_README.md - Architektur-Guide

---

## PDV VIS Compliance ✅

Basierend auf **PDV_VIS_ANALYSIS.md** wurden folgende VIS-Features als CQRS implementiert:

| VIS Feature | CQRS Implementation | Dateien | Status |
|-------------|---------------------|---------|--------|
| **Posteingang** | CreateInboxItemCommand, GetInboxItemsQuery | 5 | ✅ |
| **Wiedervorlage/Fristen** | CreateReminderCommand, GetDueRemindersQuery | 5 | ✅ |
| **Mitzeichnung** | ApproveCosigningStepCommand | 3 | ✅ |

**Verwendung:**
```csharp
// VIS Posteingang
var command = new CreateInboxItemCommand
{
    Subject = "Baugenehmigung",
    Sender = "Max Mustermann",
    Priority = InboxPriority.High
};
var inboxId = await _mediator.Send(command);

// VIS Wiedervorlage mit Eskalation
var reminder = new CreateReminderCommand
{
    ProcessId = "process123",
    DueDate = DateTime.UtcNow.AddDays(30),
    EscalationLevels = new List<EscalationLevelDto>
    {
        new() { Level = 1, DaysBeforeDue = 7, EscalateTo = "teamleiter@verwaltung.de" }
    }
};
```

---

## Next: Sprint 2

- Migration bestehender Services zu Application Layer
- UpdateDocumentCommand + Handler + Validator
- DeleteDocumentCommand + Handler  
- AssignInboxItemCommand (VIS)
- CompleteCosigningCommand (VIS)
- Validation Behavior für MediatR Pipeline
- Unit Tests erstellen (Ziel: 30% Coverage)
- Logging mit Serilog integrieren
