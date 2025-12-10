# VIS-Feature Integration mit Clean Architecture

**Datum:** 2025-12-10  
**Basierend auf:** PDV_VIS_ANALYSIS.md  
**Status:** Phase 1 Sprint 1 - VIS Integration

---

## Übersicht

Integration der PDV VIS Suite Features in die Clean Architecture mit CQRS Pattern. Alle VIS-Features werden als Commands und Queries implementiert, um die Wartbarkeit und Testbarkeit zu gewährleisten.

---

## Implementierte VIS-Features (CQRS)

### 1. Posteingang (Inbox) ✅

**VIS Anforderung:**
- Zentraler Posteingangskorb für alle eingehenden Dokumente
- Zuweisungsfunktion an Sachbearbeiter
- Status-Tracking und Prioritätsstufen

**CQRS Implementation:**

**Command - Posteingang erstellen:**
```csharp
var command = new CreateInboxItemCommand
{
    Subject = "Antrag auf Baugenehmigung",
    Sender = "Max Mustermann",
    DocumentId = "doc123",
    Priority = InboxPriority.High,
    AssignedTo = "sachbearbeiter@verwaltung.de"
};
var inboxId = await _mediator.Send(command);
```

**Query - Posteingang abrufen:**
```csharp
var query = new GetInboxItemsQuery
{
    Status = InboxStatus.New,
    AssignedTo = "sachbearbeiter@verwaltung.de"
};
var inboxItems = await _mediator.Send(query);
```

**Dateien:**
- `Application/Inbox/Commands/CreateInboxItem/`
  - CreateInboxItemCommand.cs
  - CreateInboxItemCommandHandler.cs
  - CreateInboxItemCommandValidator.cs
- `Application/Inbox/Queries/GetInboxItems/`
  - GetInboxItemsQuery.cs
  - GetInboxItemsQueryHandler.cs

---

### 2. Wiedervorlage/Fristen (Reminders) ✅

**VIS Anforderung:**
- Automatische Überwachung von Fristen
- Mehrstufige Eskalation
- Wiedervorlage-Kalender

**CQRS Implementation:**

**Command - Wiedervorlage erstellen:**
```csharp
var command = new CreateReminderCommand
{
    ProcessId = "process123",
    DueDate = DateTime.UtcNow.AddDays(30),
    ReminderDate = DateTime.UtcNow.AddDays(25),
    Type = ReminderType.Deadline,
    AssignedTo = "sachbearbeiter@verwaltung.de",
    EscalationLevels = new List<EscalationLevelDto>
    {
        new() { Level = 1, DaysBeforeDue = 7, EscalateTo = "teamleiter@verwaltung.de" },
        new() { Level = 2, DaysBeforeDue = 3, EscalateTo = "abteilungsleiter@verwaltung.de" }
    }
};
var reminderId = await _mediator.Send(command);
```

**Query - Fällige Wiedervorlagen:**
```csharp
var query = new GetDueRemindersQuery
{
    UpToDate = DateTime.UtcNow.AddDays(7)
};
var dueReminders = await _mediator.Send(query);
```

**Dateien:**
- `Application/Reminders/Commands/CreateReminder/`
  - CreateReminderCommand.cs
  - CreateReminderCommandHandler.cs
  - CreateReminderCommandValidator.cs
- `Application/Reminders/Queries/GetDueReminders/`
  - GetDueRemindersQuery.cs
  - GetDueRemindersQueryHandler.cs

---

### 3. Mitzeichnung (Cosigning) ✅

**VIS Anforderung:**
- Parallele und serielle Mitzeichnungsverfahren
- Genehmigung/Ablehnung mit Kommentar
- Workflow-Status-Tracking

**CQRS Implementation:**

**Command - Mitzeichnung genehmigen:**
```csharp
var command = new ApproveCosigningStepCommand
{
    CosigningId = "cosign123",
    CosignerId = "mitzeichner@verwaltung.de",
    Comment = "Genehmigt. Keine Einwände."
};
var success = await _mediator.Send(command);
```

**Dateien:**
- `Application/Cosigning/Commands/ApproveCosigningStep/`
  - ApproveCosigningStepCommand.cs
  - ApproveCosigningStepCommandHandler.cs
  - ApproveCosigningStepCommandValidator.cs

---

## Architektur-Vorteile für VIS-Features

### 1. Separation of Concerns
- **VIS Business Logic** in Application Layer (Commands/Queries)
- **VIS Data Access** in Infrastructure Layer (Services)
- **VIS UI** in Presentation Layer (ViewModels)

### 2. Testbarkeit
```csharp
[Fact]
public async Task CreateInboxItem_ValidInput_CreatesItem()
{
    // Arrange
    var mockInboxService = new Mock<IInboxService>();
    mockInboxService.Setup(x => x.CreateInboxItemAsync(It.IsAny<InboxItem>()))
        .ReturnsAsync(new InboxItem { Id = "inbox123" });
    
    var handler = new CreateInboxItemCommandHandler(mockInboxService.Object, _mediator);
    var command = new CreateInboxItemCommand { Subject = "Test", Sender = "Test" };
    
    // Act
    var result = await handler.Handle(command, default);
    
    // Assert
    Assert.Equal("inbox123", result);
}
```

### 3. Validierung
Alle VIS-Commands haben FluentValidation:
- Pflichtfelder prüfen
- Geschäftsregeln validieren
- Benutzerfreundliche Fehlermeldungen

### 4. Erweiterbarkeit
Neue VIS-Features als Commands/Queries hinzufügen:
- Keine Änderung bestehenden Codes
- Klare Struktur
- Plugin-fähig

---

## VIS-Feature Roadmap Integration

### Sprint 1 (Aktuell) ✅
- [x] Clean Architecture Foundation
- [x] CQRS für Dokumente
- [x] **CQRS für Posteingang (Inbox)**
- [x] **CQRS für Wiedervorlage (Reminders)**
- [x] **CQRS für Mitzeichnung (Cosigning)**

### Sprint 2 (Nächste Schritte)
- [ ] UpdateDocument, DeleteDocument Commands
- [ ] AssignInboxItem Command
- [ ] CompleteCosigning Command
- [ ] GetOverdueReminders Query
- [ ] Validation Pipeline Behavior
- [ ] Unit Tests für VIS Commands

### Sprint 3-4 (Plugin System)
- [ ] IDocumentPlugin Interface
- [ ] VIS-Feature-Plugins
- [ ] OCR Plugin
- [ ] Email Integration Plugin

---

## PDV VIS Compliance

### ✅ Erfüllt durch Clean Architecture

| VIS Anforderung | CQRS Implementation | Status |
|-----------------|---------------------|--------|
| Posteingang-Modul | CreateInboxItemCommand | ✅ |
| Wiedervorlage | CreateReminderCommand | ✅ |
| Mitzeichnung | ApproveCosigningStepCommand | ✅ |
| Status-Tracking | Queries mit Filtern | ✅ |
| Audit-Trail | Domain Events | ✅ |
| Validierung | FluentValidation | ✅ |

### 🔜 Noch zu implementieren

| VIS Feature | Geplant für | Aufwand |
|-------------|-------------|---------|
| Vorgangslaufzettel | Sprint 2 | Niedrig |
| Aktenplan-Verwaltung | Sprint 2 | Mittel |
| Scan-Integration | Sprint 7-8 | Mittel |
| OCR-Integration | Sprint 7-8 | Mittel |
| Formulare | Sprint 9-10 | Hoch |

---

## Integration mit bestehendem Code

Die neuen CQRS Commands/Queries arbeiten mit den **bestehenden VIS Services** zusammen:

```
ViewModel → IMediator → Command/Query Handler → VIS Service (IInboxService, IReminderService, etc.)
```

**Vorteile:**
- Bestehende VIS Services bleiben unverändert
- Schrittweise Migration möglich
- Keine Breaking Changes
- Neue Features nutzen CQRS, alte Services parallel

---

## Verwendung in ViewModels

**Alt (Direct Service Call):**
```csharp
public class InboxViewModel
{
    private readonly IInboxService _inboxService;
    
    public async Task CreateInboxItemAsync()
    {
        var item = new InboxItem { ... };
        await _inboxService.CreateInboxItemAsync(item);
    }
}
```

**Neu (CQRS mit MediatR):**
```csharp
public class InboxViewModel
{
    private readonly IMediator _mediator;
    
    public async Task CreateInboxItemAsync()
    {
        var command = new CreateInboxItemCommand { ... };
        var inboxId = await _mediator.Send(command);
    }
}
```

---

## Zusammenfassung

✅ **VIS-Features erfolgreich in Clean Architecture integriert**

- 3 VIS-Module als CQRS implementiert (Inbox, Reminders, Cosigning)
- 10 neue Command/Query Dateien erstellt
- FluentValidation für alle Commands
- Kompatibel mit PDV_VIS_ANALYSIS.md Anforderungen
- Testbar, wartbar, erweiterbar

**Nächste Schritte:** Sprint 2 mit weiteren VIS Commands und Unit Tests

---

**Erstellt:** 2025-12-10  
**Basierend auf:** PDV_VIS_ANALYSIS.md  
**Version:** 1.0 - VIS Integration Sprint 1
