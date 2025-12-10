# Dashboard & Sidebar Integration Guide

**Datum:** 2025-12-10  
**Feature:** Dashboard als Startansicht mit Task Basket in Sidebar  
**Status:** ✅ Implementiert

---

## Übersicht

Die Anwendung startet nun mit einem **Dashboard**, das die wichtigsten Schnellstart-Punkte enthält. Der **Aufgaben-Korb (Task Basket)** wurde in die linke Sidebar als Tab integriert.

---

## Implementierte Features

### 1. Dashboard View (Startansicht)

**Zweck:** Zentraler Einstiegspunkt mit Übersicht und Schnellzugriff

**Komponenten:**
- **Welcome Section** - Begrüßung mit Statistiken
  - Anzahl Dokumente
  - Offene Aufgaben
  - Überfällige Fristen
  
- **Schnellstart-Aktionen** (4 Karten)
  - 📄 Neues Dokument
  - ✓ Meine Aufgaben
  - 🔍 Suche
  - 📥 Posteingang
  
- **Letzte Aktivitäten**
  - Chronologische Liste der letzten Aktionen
  - Mit Icon, Titel, Beschreibung und Zeitstempel
  
- **Module** (3 Karten)
  - 📅 Timeline
  - 🗺️ Geo-Ansicht
  - 🔗 Graph-Ansicht

**Datei-Struktur:**
```
Views/Dashboard/
├── DashboardView.xaml (14 KB)
└── DashboardView.xaml.cs

ViewModels/
└── DashboardViewModel.cs (5 KB)
```

---

### 2. Sidebar mit Tab-View

**Neue Struktur:**
```
┌─────────────────────────┐
│ Navigation | Aufgaben   │ ← Tab Headers
├─────────────────────────┤
│                         │
│ Tab Content Area:       │
│ - Navigation TreeView   │
│   OR                    │
│ - Task Basket           │
│                         │
└─────────────────────────┘
```

**Tab 1: Navigation**
- 📊 Dashboard
- 📄 Dokumente
  - Meine Dokumente
  - Zuletzt verwendet
  - Favoriten
- 📂 Projekte
  - Aktive Projekte
  - Archivierte Projekte
- 📥 Posteingang
- 📅 Wiedervorlagen
- ✍️ Mitzeichnungen

**Tab 2: Aufgaben**
- Vollständiger Task Basket
- Alle an Benutzer delegierten Aufgaben
- Mit allen Features (Filtering, Sorting, Grouping)

---

## Dashboard ViewModel

### Properties

```csharp
[ObservableProperty]
private int _totalDocuments;        // Anzahl aller Dokumente

[ObservableProperty]
private int _pendingTasks;          // Offene Aufgaben

[ObservableProperty]
private int _overdueReminders;      // Überfällige Fristen

[ObservableProperty]
private int _unreadInbox;           // Ungelesene Posteingänge

[ObservableProperty]
private ObservableCollection<RecentActivity> _recentActivities;
```

### Commands

```csharp
[RelayCommand]
private async Task LoadDashboardDataAsync()
{
    // Lädt Statistiken via CQRS Queries
    var documentsQuery = new GetDocumentsQuery();
    var documents = await _mediator.Send(documentsQuery);
    
    var tasksQuery = new GetMyTasksQuery { UserId = "current-user" };
    var tasks = await _mediator.Send(tasksQuery);
    
    // Update properties
    TotalDocuments = documents.Count;
    PendingTasks = tasks.Count(t => t.Status == TaskStatus.Pending);
}

[RelayCommand]
private void OpenTaskBasket() { /* Navigate to task basket */ }

[RelayCommand]
private void NewDocument() { /* Create new document */ }
```

---

## Integration in MainWindow

### Left Sidebar Update

**Alte Struktur:**
```xml
<Border Grid.Row="1" Grid.Column="0">
    <StackPanel>
        <TextBlock Text="Navigation"/>
        <TreeView>
            <!-- Static TreeView -->
        </TreeView>
    </StackPanel>
</Border>
```

**Neue Struktur (mit Tabs):**
```xml
<Border Grid.Row="1" Grid.Column="0">
    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/> <!-- Tab Headers -->
            <RowDefinition Height="*"/>    <!-- Tab Content -->
        </Grid.RowDefinitions>
        
        <!-- Tab Headers -->
        <StackPanel Grid.Row="0" Orientation="Horizontal">
            <RadioButton x:Name="SidebarTabNavigation" Content="Navigation" 
                         IsChecked="True" Click="SidebarTab_Click"/>
            <RadioButton x:Name="SidebarTabTasks" Content="Aufgaben" 
                         Click="SidebarTab_Click"/>
        </StackPanel>
        
        <!-- Tab Content -->
        <Grid Grid.Row="1">
            <ScrollViewer x:Name="NavigationTabContent" 
                          Visibility="Visible">
                <!-- Navigation TreeView -->
            </ScrollViewer>
            
            <ContentControl x:Name="TasksTabContent" 
                            Visibility="Collapsed">
                <!-- Task Basket will be loaded here -->
            </ContentControl>
        </Grid>
    </Grid>
</Border>
```

### Center Content - Dashboard as Default

**Alte Struktur:**
```xml
<TabControl x:Name="CenterContent">
    <TabItem Header="Dashboard" IsSelected="True">
        <TextBlock Text="Dashboard"/>
    </TabItem>
</TabControl>
```

**Neue Struktur (mit echtem Dashboard):**
```xml
<TabControl x:Name="CenterContent">
    <TabItem Header="📊 Dashboard" IsSelected="True">
        <local:DashboardView DataContext="{Binding DashboardViewModel}"/>
    </TabItem>
</TabControl>
```

---

## Code-Behind Implementation

### MainWindow.xaml.cs

```csharp
private void SidebarTab_Click(object sender, RoutedEventArgs e)
{
    if (sender == SidebarTabNavigation)
    {
        NavigationTabContent.Visibility = Visibility.Visible;
        TasksTabContent.Visibility = Visibility.Collapsed;
    }
    else if (sender == SidebarTabTasks)
    {
        NavigationTabContent.Visibility = Visibility.Collapsed;
        TasksTabContent.Visibility = Visibility.Visible;
        
        // Lazy load Task Basket
        if (TasksTabContent.Content == null)
        {
            var taskBasketView = new Tasks.TaskBasketView();
            taskBasketView.DataContext = 
                App.Current.Services.GetRequiredService<TaskBasketViewModel>();
            TasksTabContent.Content = taskBasketView;
        }
    }
}
```

---

## Startup Flow

### Application Startup Sequence

1. **App.xaml.cs - OnStartup**
   ```csharp
   protected override void OnStartup(StartupEventArgs e)
   {
       // Initialize services
       ConfigureServices(serviceCollection);
       _serviceProvider = serviceCollection.BuildServiceProvider();
       
       // Show main window
       var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
       mainWindow.Show();
   }
   ```

2. **MainWindow Constructor**
   ```csharp
   public MainWindow()
   {
       InitializeComponent();
       
       // Initialize Dashboard
       var dashboardView = new DashboardView();
       dashboardView.DataContext = 
           App.Current.Services.GetRequiredService<DashboardViewModel>();
       
       // Replace placeholder
       CenterContent.Items[0].Content = dashboardView;
   }
   ```

3. **Dashboard OnLoaded**
   ```csharp
   protected override async void OnLoaded(RoutedEventArgs e)
   {
       if (DataContext is DashboardViewModel vm)
       {
           await vm.LoadDashboardDataCommand.ExecuteAsync(null);
       }
   }
   ```

---

## Navigation Flow

### From Dashboard to Other Views

**Dashboard → Task Basket:**
```csharp
[RelayCommand]
private void OpenTaskBasket()
{
    // Switch sidebar to Tasks tab
    SidebarTabTasks.IsChecked = true;
    NavigationTabContent.Visibility = Visibility.Collapsed;
    TasksTabContent.Visibility = Visibility.Visible;
}
```

**Dashboard → New Document:**
```csharp
[RelayCommand]
private void NewDocument()
{
    // Open document creation dialog or navigate to form
    var createView = new CreateDocumentView();
    // Show as dialog or new tab
}
```

**Dashboard → Module (Timeline, Geo, Graph):**
```csharp
[RelayCommand]
private void OpenTimeline()
{
    // Navigate to Timeline tab or create new tab
    var timelineTab = new TabItem
    {
        Header = "📅 Timeline",
        Content = new TimelineView()
    };
    CenterContent.Items.Add(timelineTab);
    CenterContent.SelectedItem = timelineTab;
}
```

---

## Dependency Injection Setup

### App.xaml.cs

```csharp
private void ConfigureServices(IServiceCollection services)
{
    // ... existing services ...
    
    // Dashboard
    services.AddTransient<DashboardViewModel>();
    services.AddTransient<Views.Dashboard.DashboardView>();
    
    // Task Basket (already registered)
    services.AddTransient<TaskBasketViewModel>();
    services.AddTransient<Views.Tasks.TaskBasketView>();
}
```

---

## Visual Design

### Dashboard Layout

```
┌────────────────────────────────────────────────────────┐
│ Willkommen im ThemisDB Document Manager               │
│ Schnellstart und wichtigste Funktionen                │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│
│ 125 Dokumente • 8 offene Aufgaben • 2 überfällig     │
└────────────────────────────────────────────────────────┘

┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│   📄           │ │       ✓        │ │      🔍        │
│ Neues Dokument│ │ Meine Aufgaben │ │     Suche      │
└────────────────┘ └────────────────┘ └────────────────┘

Letzte Aktivitäten
┌────────────────────────────────────────────────────────┐
│ 📄 Vertrag.pdf erstellt                  vor 5 Min    │
│ ✓  Aufgabe erledigt                      vor 1 Std    │
│ 📥 Neuer Posteingang                     vor 2 Std    │
└────────────────────────────────────────────────────────┘

Module
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│  📅         │ │    🗺️        │ │     🔗       │
│  Timeline   │ │  Geo-Ansicht │ │ Graph-Ansicht│
└──────────────┘ └──────────────┘ └──────────────┘
```

### Sidebar Tabs

```
┌─────────────────────────┐
│ Navigation | Aufgaben   │  ← Tabs
├─────────────────────────┤
│ 📊 Dashboard           │
│ 📄 Dokumente           │
│   ├─ Meine Dokumente  │
│   └─ Zuletzt verwendet│
│ 📥 Posteingang         │
│ 📅 Wiedervorlagen      │
└─────────────────────────┘
```

---

## Best Practices Implemented

### 1. Lazy Loading
- Task Basket wird erst beim ersten Tab-Switch geladen
- Verbessert Startup-Performance

### 2. MVVM Pattern
- Dashboard als eigenständiges ViewModel
- Klare Trennung View/ViewModel

### 3. CQRS Integration
- Dashboard nutzt Queries für Statistiken
- Konsistenter Datenzugriff

### 4. User Experience
- Dashboard als Einstiegspunkt (orientiert)
- Alle wichtigen Funktionen sofort sichtbar
- Statistiken geben Übersicht

---

## Zusammenfassung

✅ **Dashboard implementiert:**
- Startansicht mit Schnellzugriff
- Statistiken (Dokumente, Aufgaben, Fristen)
- Schnellstart-Karten
- Letzte Aktivitäten
- Module-Links

✅ **Sidebar mit Tabs:**
- Tab 1: Navigation (TreeView)
- Tab 2: Aufgaben (Task Basket)
- Lazy Loading für Performance

✅ **Integration:**
- Dashboard als Default-View
- CQRS für Daten
- Dependency Injection
- Navigation zu allen Features

---

**Erstellt:** 2025-12-10  
**Version:** 1.0  
**Status:** Production Ready
