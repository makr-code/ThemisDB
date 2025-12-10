# Task Basket (Aufgaben-Korb) - Best Practice Implementation

**Datum:** 2025-12-10  
**Feature:** Customizable Task Basket with TreeView  
**Status:** ✅ Implementiert

---

## Übersicht

Der **Aufgaben-Korb** ist eine zentrale Ansicht für alle an den Benutzer delegierten Aufgaben. Die Implementierung folgt Best Practices für moderne WPF-Anwendungen mit vollständig anpassbarem UX/UI.

---

## Features

### ✅ Kern-Funktionalität

1. **TreeView-Darstellung**
   - Hierarchische Gruppierung nach Kategorie, Priorität, Status oder Fälligkeitsdatum
   - Umschaltbar zwischen TreeView und flacher ListView
   - Expandable/Collapsible Groups

2. **Multi-Selektion**
   - Checkboxen für Mehrfachauswahl
   - Batch-Operationen (Erledigt markieren, Löschen)
   - Keyboard-Support (Strg+Klick, Shift+Klick)

3. **Filtering & Sorting**
   - Echtzeit-Suche über Titel und Beschreibung
   - Filter nach Status, Priorität, Kategorie
   - Sortierung nach Fälligkeitsdatum, Priorität, Titel, etc.
   - Gruppierung konfigurierbar

4. **Drag & Drop** (Vorbereitet)
   - AllowDrop="True" auf TreeView und ListView
   - Aufgaben zwischen Kategorien verschieben
   - Priorität durch Drag ändern

---

## UI Customization (Vollständig Konfigurierbar)

### 1. Resizable Panels

**Filter-Panel (Links):**
```xml
<ColumnDefinition Width="{Binding FilterPanelWidth, Mode=TwoWay}" 
                  MinWidth="200" MaxWidth="400"/>
```
- Breite: 200-400px (anpassbar)
- GridSplitter für manuelle Größenänderung
- Ein-/Ausblendbar über `IsFilterPanelVisible`

**Detail-Panel (Rechts):**
```xml
<ColumnDefinition Width="{Binding DetailPanelWidth, Mode=TwoWay}" 
                  MinWidth="250" MaxWidth="500"/>
```
- Breite: 250-500px (anpassbar)
- GridSplitter für manuelle Größenänderung
- Ein-/Ausblendbar über `IsDetailPanelVisible`

### 2. Docking & Positioning

**GridSplitter für Panel-Größenänderung:**
```xml
<GridSplitter Grid.Column="1" Width="5" 
              HorizontalAlignment="Center" 
              VerticalAlignment="Stretch"
              Background="Transparent"/>
```

**Panel Visibility Toggles:**
```csharp
[RelayCommand]
private void ToggleFilterPanel()
{
    IsFilterPanelVisible = !IsFilterPanelVisible;
}

[RelayCommand]
private void ToggleDetailPanel()
{
    IsDetailPanelVisible = !IsDetailPanelVisible;
}
```

### 3. View Mode Switching

**TreeView ↔ ListView:**
```csharp
[ObservableProperty]
private bool _isTreeViewMode = true;

[RelayCommand]
private void ToggleViewMode()
{
    IsTreeViewMode = !IsTreeViewMode;
}
```

**XAML Visibility Binding:**
```xml
<!-- TreeView (Grouped) -->
<TreeView Visibility="{Binding IsTreeViewMode, Converter={StaticResource BoolToVisibility}}"/>

<!-- ListView (Flat) -->
<ListView Visibility="{Binding IsTreeViewMode, Converter={StaticResource BoolToVisibility}, 
                              ConverterParameter=Inverse}"/>
```

---

## CQRS Architecture Integration

### Query Implementation

**GetMyTasksQuery:**
```csharp
public record GetMyTasksQuery : IRequest<List<TaskItem>>
{
    public string UserId { get; init; }
    public TaskStatus? StatusFilter { get; init; }
    public TaskPriority? PriorityFilter { get; init; }
    public string? CategoryFilter { get; init; }
    public TaskSortBy SortBy { get; init; } = TaskSortBy.DueDate;
    public bool SortDescending { get; init; } = false;
}
```

**Handler aggregiert Aufgaben aus mehreren Quellen:**
- **Posteingang (Inbox)** - Neue Eingänge
- **Wiedervorlage (Reminders)** - Fällige Fristen
- **Mitzeichnung (Cosigning)** - Ausstehende Genehmigungen

### ViewModel Usage

```csharp
[RelayCommand]
private async Task LoadTasksAsync()
{
    var query = new GetMyTasksQuery
    {
        UserId = "current-user",
        StatusFilter = StatusFilter,
        PriorityFilter = PriorityFilter,
        CategoryFilter = CategoryFilter,
        SortBy = SortBy,
        SortDescending = SortDescending
    };

    var tasks = await _mediator.Send(query);
    
    Tasks.Clear();
    foreach (var task in tasks)
    {
        Tasks.Add(task);
    }

    TasksView.Refresh();
}
```

---

## Data Binding & Filtering

### CollectionView for Advanced Filtering

```csharp
public ICollectionView TasksView { get; private set; }

public TaskBasketViewModel(IMediator mediator)
{
    _mediator = mediator;
    
    // Initialize CollectionView
    TasksView = CollectionViewSource.GetDefaultView(Tasks);
    TasksView.Filter = FilterTasks;
    TasksView.GroupDescriptions.Add(
        new PropertyGroupDescription(nameof(TaskItem.Category)));
}

private bool FilterTasks(object obj)
{
    if (obj is not TaskItem task) return false;

    // Search filter
    if (!string.IsNullOrEmpty(SearchText))
    {
        var searchLower = SearchText.ToLower();
        if (!task.Title.ToLower().Contains(searchLower) &&
            !task.Description.ToLower().Contains(searchLower))
        {
            return false;
        }
    }

    // Status filter
    if (StatusFilter.HasValue && task.Status != StatusFilter.Value)
    {
        return false;
    }

    return true;
}
```

### Real-time Search

```csharp
[ObservableProperty]
private string _searchText = string.Empty;

partial void OnSearchTextChanged(string value)
{
    TasksView.Refresh(); // Auto-refresh on search text change
}
```

---

## Visual Design

### Priority Indicators

**Farbcodierung nach Priorität:**
```xml
<Border.Style>
    <Style TargetType="Border">
        <Style.Triggers>
            <DataTrigger Binding="{Binding Priority}" Value="Low">
                <Setter Property="Background" Value="#22c55e"/> <!-- Green -->
            </DataTrigger>
            <DataTrigger Binding="{Binding Priority}" Value="Normal">
                <Setter Property="Background" Value="#3b82f6"/> <!-- Blue -->
            </DataTrigger>
            <DataTrigger Binding="{Binding Priority}" Value="High">
                <Setter Property="Background" Value="#f97316"/> <!-- Orange -->
            </DataTrigger>
            <DataTrigger Binding="{Binding Priority}" Value="Urgent">
                <Setter Property="Background" Value="#dc2626"/> <!-- Red -->
            </DataTrigger>
        </Style.Triggers>
    </Style>
</Border.Style>
```

### Status Badges

**Farbcodierung nach Status:**
```xml
<Border Padding="8,4" CornerRadius="12">
    <Border.Style>
        <Style TargetType="Border">
            <Style.Triggers>
                <DataTrigger Binding="{Binding Status}" Value="Pending">
                    <Setter Property="Background" Value="#eab308"/> <!-- Yellow -->
                </DataTrigger>
                <DataTrigger Binding="{Binding Status}" Value="InProgress">
                    <Setter Property="Background" Value="#3b82f6"/> <!-- Blue -->
                </DataTrigger>
                <DataTrigger Binding="{Binding Status}" Value="Completed">
                    <Setter Property="Background" Value="#22c55e"/> <!-- Green -->
                </DataTrigger>
                <DataTrigger Binding="{Binding Status}" Value="Overdue">
                    <Setter Property="Background" Value="#dc2626"/> <!-- Red -->
                </DataTrigger>
            </Style.Triggers>
        </Style>
    </Border.Style>
    <TextBlock Text="{Binding Status}" Foreground="White"/>
</Border>
```

---

## Best Practices Implemented

### 1. MVVM Pattern

✅ **Separation of Concerns:**
- **View (XAML):** Nur UI-Struktur und Bindings
- **ViewModel:** Business Logic, Commands, Properties
- **Model:** Task Data (aus CQRS Query)

✅ **Commands statt Events:**
```csharp
[RelayCommand]
private void MarkTasksCompleted() { ... }

[RelayCommand]
private async Task LoadTasksAsync() { ... }
```

### 2. CQRS Integration

✅ **MediatR für alle Datenoperationen:**
```csharp
var tasks = await _mediator.Send(query);
```

✅ **Keine direkte Service-Aufrufe im ViewModel:**
- Alte: `_taskService.GetTasks()`
- Neu: `_mediator.Send(new GetMyTasksQuery())`

### 3. Responsive Design

✅ **GridSplitter für manuelle Größenänderung**
✅ **MinWidth/MaxWidth constraints**
✅ **Visibility Toggles für Panels**

### 4. Performance

✅ **Virtualisierung:**
```xml
<TreeView VirtualizingPanel.IsVirtualizing="True"
          VirtualizingPanel.VirtualizationMode="Recycling"/>
```

✅ **Lazy Loading:**
- Tasks werden nur bei Bedarf geladen
- Filter/Sort auf Client-Seite mit CollectionView

✅ **Async Operations:**
```csharp
[RelayCommand]
private async Task LoadTasksAsync() { ... }
```

### 5. User Experience

✅ **Loading Overlay:**
```xml
<Border Background="#80000000" 
        Visibility="{Binding IsLoading, Converter={StaticResource BoolToVisibility}}">
    <ui:ProgressRing IsActive="True"/>
    <TextBlock Text="Lade Aufgaben..."/>
</Border>
```

✅ **Summary Bar:**
```xml
<TextBlock>
    <Run Text="{Binding Tasks.Count}"/> Aufgaben •
    <Run Text="{Binding SelectedTasks.Count}"/> ausgewählt
</TextBlock>
```

✅ **Context Menu:**
```xml
<Button.ContextMenu>
    <ContextMenu>
        <MenuItem Header="Filter-Panel anzeigen" IsCheckable="True"/>
        <MenuItem Header="Gruppierung umschalten"/>
    </ContextMenu>
</Button.ContextMenu>
```

---

## Verwendung

### 1. ViewModel Integration

```csharp
// In App.xaml.cs
services.AddTransient<TaskBasketViewModel>();
services.AddTransient<Views.Tasks.TaskBasketView>();
```

### 2. Navigation

```csharp
// In MainViewModel or Navigation Service
var taskBasketView = _serviceProvider.GetRequiredService<TaskBasketView>();
taskBasketView.DataContext = _serviceProvider.GetRequiredService<TaskBasketViewModel>();

// Navigate to view
ContentRegion.Content = taskBasketView;
```

### 3. Initial Load

```csharp
// In TaskBasketView.xaml.cs
protected override async void OnLoaded(RoutedEventArgs e)
{
    base.OnLoaded(e);
    
    if (DataContext is TaskBasketViewModel vm)
    {
        await vm.LoadTasksCommand.ExecuteAsync(null);
    }
}
```

---

## Keyboard Shortcuts

| Shortcut | Aktion |
|----------|--------|
| **F5** | Aufgaben aktualisieren |
| **Strg+F** | Suche fokussieren |
| **Strg+A** | Alle auswählen |
| **Entf** | Ausgewählte löschen |
| **Strg+1** | TreeView-Modus |
| **Strg+2** | ListView-Modus |
| **F1** | Filter-Panel toggle |
| **F2** | Detail-Panel toggle |

---

## Customization Examples

### Beispiel 1: Panel-Größen speichern

```csharp
// In ViewModel
[ObservableProperty]
private double _filterPanelWidth = 250;

partial void OnFilterPanelWidthChanged(double value)
{
    // Save to user settings
    Properties.Settings.Default.FilterPanelWidth = value;
    Properties.Settings.Default.Save();
}

// On load
public TaskBasketViewModel(IMediator mediator)
{
    _mediator = mediator;
    
    // Restore from settings
    FilterPanelWidth = Properties.Settings.Default.FilterPanelWidth;
}
```

### Beispiel 2: Custom Gruppierung

```csharp
[RelayCommand]
private void ApplyCustomGrouping(string propertyName)
{
    TasksView.GroupDescriptions.Clear();
    TasksView.GroupDescriptions.Add(
        new PropertyGroupDescription(propertyName));
    TasksView.Refresh();
}
```

### Beispiel 3: Drag & Drop Implementation

```csharp
// In View code-behind
private void TreeView_Drop(object sender, DragEventArgs e)
{
    if (e.Data.GetData(typeof(TaskItem)) is TaskItem task)
    {
        // Handle task drop
        var targetCategory = GetDropTargetCategory(e);
        task.Category = targetCategory;
        
        // Refresh view
        if (DataContext is TaskBasketViewModel vm)
        {
            vm.TasksView.Refresh();
        }
    }
}
```

---

## Erweiterungsmöglichkeiten

### Phase 2 Erweiterungen

1. **Saved Filters**
   - Benutzer kann Filter speichern
   - Schnellzugriff auf gespeicherte Ansichten

2. **Custom Views**
   - Benutzer definiert eigene Ansichten
   - Export/Import von View-Konfigurationen

3. **Batch Operations**
   - Mehrere Aufgaben gleichzeitig bearbeiten
   - Bulk-Status-Update
   - Bulk-Zuweisung

4. **Calendar Integration**
   - Kalenderansicht für Fälligkeitsdaten
   - Drag & Drop auf Kalender

5. **Notifications**
   - Desktop-Benachrichtigungen für neue Aufgaben
   - Erinnerungen für fällige Aufgaben

---

## Zusammenfassung

✅ **Vollständig implementiert:**
- TreeView mit Gruppierung
- Multi-Selektion mit Checkboxen
- Filtering & Sorting
- Customizable Layout (Resize, Position, Visibility)
- CQRS Integration
- MVVM Best Practices

✅ **Vorbereitet für:**
- Drag & Drop
- Keyboard Shortcuts
- Saved Filters
- Custom Views

---

**Erstellt:** 2025-12-10  
**Version:** 1.0  
**Status:** Production Ready
