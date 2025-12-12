# Event-basierte Kommunikation in Themis DMS

## Übersicht

Die Themis DMS Anwendung nutzt **MediatR** als Event-Bus für die Kommunikation zwischen ViewModels und Komponenten.

## Architektur

### MediatR Pattern
- **IMediator**: Zentrale Message-Bus-Schnittstelle
- **INotification**: Basis-Interface für alle Events
- **INotificationHandler<T>**: Handler für spezifische Events

### Komponenten

1. **Domain Events** (`Domain/Events/`)
   - `DocumentCreatedEvent`: Wird publiziert, wenn ein Dokument erstellt wurde
   - `TestDataGeneratedEvent`: Wird publiziert, wenn Testdaten generiert wurden

2. **Event Handler** (`Application/Documents/EventHandlers/`)
   - `TestDataGeneratedEventHandler`: Loggt Testdaten-Generierung

3. **ViewModels als Handler**
   - `DocumentBrowserViewModel`: Implementiert `INotificationHandler<TestDataGeneratedEvent>`
   - Aktualisiert automatisch die Dokumentenliste nach Testdaten-Generierung

## Event-Flow: Testdaten-Generierung

```
TestDataGeneratorViewModel
    └─> GenerateDataAsync()
        └─> _mediator.Publish(new TestDataGeneratedEvent(...))
            ├─> DocumentBrowserViewModel.Handle() → Refresh UI
            └─> TestDataGeneratedEventHandler.Handle() → Logging
```

## Verwendung

### Event publizieren

```csharp
public class TestDataGeneratorViewModel
{
    private readonly IMediator _mediator;

    public async Task GenerateDataAsync()
    {
        // ... Daten generieren ...
        
        // Event publizieren
        await _mediator.Publish(new TestDataGeneratedEvent(count, DateTime.UtcNow));
    }
}
```

### Event empfangen

```csharp
public class DocumentBrowserViewModel : 
    ObservableObject, 
    INotificationHandler<TestDataGeneratedEvent>
{
    public async Task Handle(TestDataGeneratedEvent notification, CancellationToken cancellationToken)
    {
        // UI automatisch aktualisieren
        await LoadDocumentsAsync();
    }
}
```

## Neue Events hinzufügen

1. **Event definieren** in `Domain/Events/`:
   ```csharp
   public record MyNewEvent(string Data) : INotification;
   ```

2. **Handler erstellen** (optional):
   ```csharp
   public class MyNewEventHandler : INotificationHandler<MyNewEvent>
   {
       public Task Handle(MyNewEvent notification, CancellationToken ct)
       {
           // Event-Logik
           return Task.CompletedTask;
       }
   }
   ```

3. **ViewModel als Handler** (alternative):
   ```csharp
   public class MyViewModel : INotificationHandler<MyNewEvent>
   {
       public Task Handle(MyNewEvent notification, CancellationToken ct)
       {
           // UI aktualisieren
           return Task.CompletedTask;
       }
   }
   ```

4. **MediatR registriert Handler automatisch** per Assembly-Scan

## Vorteile

✅ **Entkopplung**: ViewModels müssen sich nicht gegenseitig kennen  
✅ **Testbarkeit**: Events können isoliert getestet werden  
✅ **Erweiterbarkeit**: Neue Handler ohne Code-Änderung hinzufügen  
✅ **CQRS-Ready**: Commands und Queries bereits implementiert  

## Weitere Events

- `DocumentCreatedEvent`: Neues Dokument erstellt
- `DocumentUpdatedEvent`: Dokument aktualisiert (TODO)
- `DocumentDeletedEvent`: Dokument gelöscht (TODO)
- `SearchCompletedEvent`: Suche abgeschlossen (TODO)

## Referenzen

- [MediatR GitHub](https://github.com/jbogard/MediatR)
- [CQRS Pattern](https://martinfowler.com/bliki/CQRS.html)
- [Domain Events](https://docs.microsoft.com/en-us/dotnet/architecture/microservices/microservice-ddd-cqrs-patterns/domain-events-design-implementation)
