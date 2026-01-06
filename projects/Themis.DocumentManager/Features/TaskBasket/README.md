# Aufgaben-Tab (Tasks Tab) - Implementierung

## Überblick

Das Aufgaben-Tab ist eine neue Funktion in der rechten Seitenleiste des DocumentManagers, die alle offenen Aufgaben für einen Benutzer anzeigt. Die Implementierung folgt Best Practices von führenden DMS-Systemen wie PDV VIS und Microsoft Teams.

## Features

### 1. Card-basiertes Layout
- Kompakte Kartenansicht für einfache Übersicht
- Farbcodierte Prioritätsindikatoren (Niedrig, Normal, Hoch, Dringend)
- Status-Badges (Ausstehend, In Bearbeitung, Erledigt, Überfällig)
- Kategorie-Tags (Posteingang, Wiedervorlage, Mitzeichnung)

### 2. Suche und Filterung
- **Echtzeit-Suche**: Durchsucht Titel und Beschreibung (wie Microsoft Teams)
- **Status-Filter**: Alle, Offen, In Bearbeitung, Erledigt
- **Sortierung**: Nach Fälligkeit, Priorität oder Titel
- **Kontext-Filterung**: Automatische Hervorhebung von Aufgaben, die zur aktuell ausgewählten Akte/Vorgang gehören

### 3. Drag & Drop Funktionalität
- Aufgaben können per Drag & Drop verschoben werden
- Visuelle Feedback während des Ziehens (Opazität, Highlight)
- Drop-Zonen werden beim Hovern hervorgehoben
- Ermöglicht Umverteilung von Aufgaben zwischen Benutzern oder Kategorien

### 4. Schnellaktionen
- **✓ Erledigt markieren**: Markiert Aufgabe als abgeschlossen
- **▶ Öffnen**: Navigiert zur verknüpften Entität (Dokument/Akte/Vorgang)

### 5. Kontextbewusstsein (VIS-Style)
- Passt sich automatisch an die ausgewählte Akte/Vorgang an
- Hebt verwandte Aufgaben mit orangem Rahmen hervor
- Zeigt nur relevante Aufgaben für den aktuellen Kontext

### 6. Collaborative Features (Teams-inspiriert)
- Ungelesene Aufgaben-Zähler (neue Aufgaben der letzten 24h)
- Aufgaben-Zuweisung (vorbereitet für @mention-Style)
- Real-time Updates durch MVVM Observable Pattern

## Architektur

### MVVM Pattern
```
View (TaskCardView.xaml)
    ↓
ViewModel (TasksRightSidebarViewModel.cs)
    ↓
MediatR (GetMyTasksQuery)
    ↓
Handler (GetMyTasksQueryHandler.cs)
    ↓
Services (IInboxService, IReminderService, ICosigningService)
```

### Komponenten

#### 1. TaskCardView.xaml
- UserControl für die Kartenansicht
- Implementiert Drag & Drop Event-Handler
- Responsive Layout mit ScrollViewer
- Empty State für "Keine Aufgaben"

#### 2. TaskCardView.xaml.cs
- Code-Behind für Drag & Drop Logik
- Event-Handler für MouseDown, MouseMove, DragEnter, DragLeave, Drop
- Visuelle Feedback-Mechanismen

#### 3. TasksRightSidebarViewModel.cs
- ViewModel mit MVVM Toolkit (CommunityToolkit.Mvvm)
- ObservableObject für Property-Change-Notifications
- RelayCommands für Benutzeraktionen
- ICollectionView für Filterung und Sortierung

#### 4. TaskItem Model
- Erweitert um `IsRelatedToCurrentEntity` Eigenschaft
- `LinkedEntityId` für Verknüpfung mit Dokumenten/Akten/Vorgängen
- Status, Priority, Category, DueDate Eigenschaften

## Integration

### MainWindow.xaml
```xml
<TabControl x:Name="RightSidebarTabs">
    <TabItem Header="✅ Aufgaben" x:Name="RightTasksTab">
        <taskbasket:TaskCardView x:Name="RightTasksPanel"/>
    </TabItem>
    <!-- Andere Tabs... -->
</TabControl>
```

### MainWindow.xaml.cs
```csharp
// Wiring im WireRightPanelsAsync
var tasksVm = App.GetService<TasksRightSidebarViewModel>();
if (tasksVm != null && RightTasksPanel != null)
{
    RightTasksPanel.DataContext = tasksVm;
    await tasksVm.LoadTasksAsync();
}

// Kontext-Update bei Dokument-Auswahl
public async Task UpdateTaskContextAsync(string? entityId, LinkedEntityType? entityType)
{
    var tasksVm = App.GetService<TasksRightSidebarViewModel>();
    if (tasksVm != null)
    {
        await tasksVm.UpdateEntityContextAsync(entityId, entityType);
    }
}
```

### App.xaml.cs (DI Registration)
```csharp
services.AddTransient<TasksRightSidebarViewModel>();
```

## Best Practices von DMS-Systemen

### Von PDV VIS gelernt:
1. **Kontext-bewusste Anzeige**: Aufgaben werden im Kontext der aktuellen Akte/Vorgang gefiltert
2. **Farbcodierung**: Visuelle Priorisierung durch Farben
3. **Kategorien**: Posteingang, Wiedervorlage, Mitzeichnung
4. **Schnellaktionen**: Direkt aus der Kartenansicht

### Von Microsoft Teams gelernt:
1. **Card-Layout**: Übersichtliche Kartendarstellung
2. **Drag & Drop**: Intuitive Umverteilung
3. **Echtzeit-Suche**: Sofortige Filterung beim Tippen
4. **Unread Badges**: Kennzeichnung neuer Aufgaben
5. **Quick Actions**: Inline-Aktionen ohne Kontextwechsel

### OOP Prinzipien:
1. **Single Responsibility**: Jede Klasse hat eine klare Verantwortung
2. **Dependency Injection**: Lose Kopplung durch DI Container
3. **Command Pattern**: Benutzeraktionen als Commands
4. **Observer Pattern**: MVVM mit INotifyPropertyChanged
5. **CQRS**: Trennung von Queries und Commands mit MediatR

## Verwendung

### Aufgaben laden
Die Aufgaben werden automatisch beim Öffnen des Tabs geladen. Manuelles Aktualisieren über den ↻-Button.

### Suchen
Einfach Text in das Suchfeld eingeben. Die Liste wird sofort gefiltert.

### Filtern
- Status-Filter: Dropdown mit "Alle", "Offen", "Läuft", "Erledigt"
- Sortierung: Dropdown mit "Fälligkeit", "Priorität", "Titel"

### Drag & Drop
1. Klicken und halten auf einer Aufgabenkarte
2. Karte wird leicht transparent
3. Über eine andere Karte ziehen (wird blau hervorgehoben)
4. Loslassen zum Ablegen

### Schnellaktionen
- **✓**: Aufgabe als erledigt markieren
- **▶**: Zum verknüpften Dokument/Vorgang navigieren

### Kontext-Update
Wenn ein Dokument/Akte/Vorgang ausgewählt wird, ruft das System automatisch:
```csharp
await UpdateTaskContextAsync(entityId, entityType);
```
Dies hebt alle verwandten Aufgaben mit einem orangenen Rahmen hervor.

## Erweiterungsmöglichkeiten

### Kurzfristig
1. ✅ Aufgaben-Zuweisung an andere Benutzer
2. ✅ @mention-Style Benachrichtigungen
3. ✅ Aufgaben-Historie und Audit-Log
4. ✅ Anpassbare Ansichten (Kompakt/Erweitert)

### Mittelfristig
1. Aufgaben-Templates
2. Wiederkehrende Aufgaben
3. Abhängigkeiten zwischen Aufgaben
4. Gantt-Chart Integration

### Langfristig
1. KI-gestützte Aufgaben-Priorisierung
2. Automatische Aufgaben-Generierung aus Dokumenten
3. Integration mit externen Task-Management-Systemen
4. Mobile App Synchronisierung

## Testing

### Manuelle Tests
1. **Laden**: Tab öffnen → Aufgaben sollten laden
2. **Suche**: Text eingeben → Liste wird gefiltert
3. **Filter**: Status ändern → Liste wird aktualisiert
4. **Sortierung**: Sortierung ändern → Reihenfolge ändert sich
5. **Drag & Drop**: Karte ziehen → Visuelle Feedback, Drop funktioniert
6. **Schnellaktionen**: ✓ klicken → Status ändert sich

### Unit Tests (TODO)
```csharp
[Fact]
public void FilterTasks_WithSearchText_FiltersCorrectly()
{
    // Arrange
    var vm = new TasksRightSidebarViewModel(mediator);
    vm.Tasks.Add(new TaskItem { Title = "Test Task" });
    
    // Act
    vm.SearchText = "Test";
    
    // Assert
    Assert.Single(vm.TasksView.Cast<TaskItem>());
}
```

## Performance

- **Lazy Loading**: Aufgaben werden nur geladen, wenn das Tab geöffnet wird
- **Virtualisierung**: ScrollViewer mit ItemsControl für große Listen
- **Debouncing**: Suche wird nach Eingabestopp ausgeführt
- **Observable Collection**: Effiziente UI-Updates durch MVVM

## Zugänglichkeit

- Tastaturnavigation unterstützt
- Screen-Reader-freundliche Labels
- Hoher Kontrast für Status-Farben
- Tooltips für alle Aktionen

## Sicherheit

- Rollenbasierte Berechtigungen (vorbereitet)
- Audit-Logging für Aufgaben-Aktionen
- Keine sensiblen Daten in UI-Logs

## Dokumentation

- XML-Kommentare in allen öffentlichen Methoden
- Inline-Kommentare für komplexe Logik
- Dieses README für Übersicht

## Lizenz

Teil des ThemisDB Projekts. Siehe Hauptlizenz.
