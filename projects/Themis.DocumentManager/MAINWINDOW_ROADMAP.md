# MainWindow Erweiterungs-Roadmap

## 🗺️ Phase-basierte Erweiterung

Dieser Guide zeigt, wie man die MainWindow schrittweise mit komplexeren Features erweitert, ohne den stabilen Kern zu gefährden.

---

## **Phase 1: Tab-Content Implementieren** ✅ Aktuell

### Ziel: Alle 4 Tabs mit echtem Content füllen

#### 1.1 Start Tab → Dashboard Preview
```xaml
<!-- MainWindow.xaml, TabItem TabStart -->
<TabItem Header="📊 Start" x:Name="TabStart" IsSelected="True">
    <views:DashboardPreviewView />
</TabItem>
```

**Neu-Datei: `Views/DashboardPreviewView.xaml`**
```xaml
<UserControl x:Class="Themis.DocumentManager.Views.DashboardPreviewView"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
    <Grid Background="#ffffff">
        <StackPanel Margin="20" VerticalAlignment="Top">
            <TextBlock Text="🎯 Quick Stats" FontSize="18" FontWeight="Bold" Foreground="#1e293b"/>
            <TextBlock Text="Total Documents: 42" Margin="0,10,0,0" Foreground="#64748b"/>
            <TextBlock Text="Recent Changes: 5" Margin="0,5,0,0" Foreground="#64748b"/>
        </StackPanel>
    </Grid>
</UserControl>
```

#### 1.2 Documents Tab → Document Browser
```xaml
<TabItem Header="📑 Documents" x:Name="TabDocuments">
    <views:DocumentBrowserView />
</TabItem>
```

#### 1.3 Timeline Tab → Timeline View
```xaml
<TabItem Header="🔗 Timeline" x:Name="TabTimeline">
    <views:TimelineView />
</TabItem>
```

#### 1.4 Dashboard Tab → Full Dashboard
```xaml
<TabItem Header="📊 Dashboard" x:Name="TabDashboard">
    <views:FullDashboardView />
</TabItem>
```

**DI Registration in App.xaml.cs:**
```csharp
// Views registrieren
services.AddTransient<DashboardPreviewView>();
services.AddTransient<DocumentBrowserView>();
services.AddTransient<TimelineView>();
services.AddTransient<FullDashboardView>();
```

---

## **Phase 2: Toolbar Button Commands** 🔶 Nächst

### Ziel: Ribbon-Buttons mit Funktionalität

#### 2.1 XAML Anpassung
```xaml
<!-- MainWindow.xaml Ribbon-Section -->
<Border Grid.Row="1" Background="#ffffff" BorderBrush="#e2e8f0" BorderThickness="0,0,0,1">
    <StackPanel Orientation="Horizontal" Margin="10,5">
        <Button Command="{Binding NewCommand}" Content="📄 Neu" Padding="10,5" Margin="0,0,5,0"/>
        <Button Command="{Binding OpenCommand}" Content="📂 Öffnen" Padding="10,5" Margin="0,0,5,0"/>
        <Button Command="{Binding SaveCommand}" Content="💾 Speichern" Padding="10,5" Margin="0,0,5,0"/>
        <Separator Margin="5,0" Background="#e2e8f0" Width="1"/>
        <Button Command="{Binding SearchCommand}" Content="🔍 Suchen" Padding="10,5" Margin="0,0,5,0"/>
        <Button Command="{Binding SettingsCommand}" Content="⚙️ Einstellungen" Padding="10,5"/>
    </StackPanel>
</Border>
```

#### 2.2 MainViewModel Commands hinzufügen
```csharp
public partial class MainViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;
    
    public MainViewModel(IDocumentService documentService)
    {
        _documentService = documentService;
    }

    [RelayCommand]
    private async Task NewAsync()
    {
        // Neues Dokument erstellen
        var newDoc = await _documentService.CreateNewAsync();
        // Tab zum Document wechseln
    }

    [RelayCommand]
    private async Task OpenAsync()
    {
        // Open-Dialog + Load
    }

    [RelayCommand]
    private async Task SaveAsync()
    {
        // Save current document
    }

    [RelayCommand]
    private void Search()
    {
        // Show search panel/dialog
    }

    [RelayCommand]
    private void ShowSettings()
    {
        // Open settings window
    }
}
```

---

## **Phase 3: Sidebar Navigation** 🔶 Nach Phase 2

### Ziel: Links/Rechts Sidebars für Context

#### 3.1 3-Column Layout
```xaml
<Grid>
    <Grid.ColumnDefinitions>
        <ColumnDefinition Width="200"/>       <!-- Left Sidebar -->
        <ColumnDefinition Width="*"/>         <!-- Main Content -->
        <ColumnDefinition Width="250"/>       <!-- Right Sidebar -->
    </Grid.ColumnDefinitions>
    
    <!-- Left Navigation -->
    <TreeView Grid.Column="0" ItemsSource="{Binding NavigationItems}"/>
    
    <!-- Main Content (verschobenes Grid) -->
    <Grid Grid.Column="1">
        <!-- Menu, Ribbon, TabControl, Status Bar -->
    </Grid>
    
    <!-- Right Properties -->
    <StackPanel Grid.Column="2" Background="#f8fafc" BorderBrush="#e2e8f0" BorderThickness="1,0,0,0">
        <TextBlock Text="Properties" Margin="10"/>
        <ContentControl Content="{Binding SelectedItemProperties}"/>
    </StackPanel>
</Grid>
```

---

## **Phase 4: Keyboard Shortcuts** 🟡 Parallel mit Phase 1-3

### Ziel: Standard Shortcuts (Ctrl+S, Ctrl+O, etc.)

#### 4.1 InputBindings in MainWindow.xaml
```xaml
<Window ... >
    <Window.InputBindings>
        <KeyBinding Key="N" Modifiers="Control" Command="{Binding NewCommand}"/>
        <KeyBinding Key="O" Modifiers="Control" Command="{Binding OpenCommand}"/>
        <KeyBinding Key="S" Modifiers="Control" Command="{Binding SaveCommand}"/>
        <KeyBinding Key="F" Modifiers="Control" Command="{Binding SearchCommand}"/>
    </Window.InputBindings>
    
    <!-- Rest of Window -->
</Window>
```

**Tipp:** Immer hex-Farben + keine DynamicResource verwenden!

---

## **Phase 5: Settings/Preferences Window** 🟡 Nach Phase 2

### Ziel: App-Konfiguration

#### 5.1 Neue Window erstellen
```csharp
public partial class SettingsWindow : Window
{
    public SettingsWindow()
    {
        InitializeComponent();
        DataContext = App.GetService<SettingsViewModel>();
    }
}
```

#### 5.2 Im Menü integrieren
```xaml
<MenuItem Header="Einstellungen" Click="MenuSettings_Click"/>
```

```csharp
private void MenuSettings_Click(object sender, RoutedEventArgs e)
{
    var settingsWindow = new SettingsWindow
    {
        Owner = this,
        WindowStartupLocation = WindowStartupLocation.CenterOwner
    };
    settingsWindow.ShowDialog();
}
```

---

## **Phase 6: Advanced Features** 🟡 Nach Phase 1-5

### 6.1 Drag-Drop Support
```csharp
// In MainWindow.xaml.cs
public MainWindow(...)
{
    // ...
    Loaded += (s, e) => {
        AllowDrop = true;
        Drop += MainWindow_Drop;
    };
}

private void MainWindow_Drop(object sender, DragEventArgs e)
{
    if (e.Data.GetDataPresent(DataFormats.FileDrop))
    {
        string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
        // Handle file drop
    }
}
```

### 6.2 Tab Schließen-Button
```xaml
<TabItem x:Name="TabStart">
    <TabItem.Header>
        <StackPanel Orientation="Horizontal">
            <TextBlock Text="📊 Start"/>
            <Button Content="✕" Margin="5,0,0,0" 
                    Click="CloseTab_Click" 
                    Padding="2,0" Foreground="#64748b"/>
        </StackPanel>
    </TabItem.Header>
</TabItem>
```

### 6.3 Dynamische Tabs hinzufügen
```csharp
[RelayCommand]
public void AddNewTab(string documentId)
{
    var newTab = new TabItem
    {
        Header = $"Document-{documentId}",
        Content = new DocumentDetailView()
    };
    // Add to TabControl.Items
}
```

---

## 📋 Implementierungs-Checklist

### Phase 1: Tab-Content
- [ ] DashboardPreviewView erstellen
- [ ] DocumentBrowserView erstellen
- [ ] TimelineView erstellen
- [ ] FullDashboardView erstellen
- [ ] ViewModels für jede View
- [ ] DI Registration
- [ ] Build testen (0 Fehler)
- [ ] App Launch testen

### Phase 2: Toolbar Commands
- [ ] MainViewModel Relay-Commands hinzufügen
- [ ] Binding in XAML
- [ ] Command-Implementierungen
- [ ] Build testen
- [ ] Click-Handler prüfen

### Phase 3: Sidebar
- [ ] Layout zu 3-Column ändern
- [ ] Left TreeView implementieren
- [ ] Right Properties Panel
- [ ] Data Binding
- [ ] Navigation Click Handler

### Phase 4: Keyboard Shortcuts
- [ ] InputBindings XAML
- [ ] Command-Routing prüfen
- [ ] Conflict-Testing

### Phase 5: Settings Window
- [ ] SettingsWindow.xaml/.cs
- [ ] SettingsViewModel
- [ ] Settings UI
- [ ] Persistence

### Phase 6: Advanced Features
- [ ] Drag-Drop Handler
- [ ] Tab-Close Buttons
- [ ] Dynamische Tabs

---

## 🧪 Testing-Strategie für jede Phase

```csharp
// Beispiel: Phase 1 Testing
[TestClass]
public class MainWindowPhase1Tests
{
    [TestMethod]
    public void TabContent_Should_Load_Without_Errors()
    {
        // Arrange
        var mainWindow = new MainWindow(...);
        
        // Act
        mainWindow.Show();
        
        // Assert
        Assert.IsNotNull(mainWindow.FindName("CenterContent"));
        Assert.AreEqual(4, ((TabControl)mainWindow.FindName("CenterContent")).Items.Count);
    }
}
```

---

## ⚠️ Phase-übergreifende Regeln

1. **XAML-First:** Änderungen immer in XAML, dann Code-Behind minimal halten
2. **Build-Check:** Nach jeder Phase: `dotnet build` → 0 Fehler
3. **App-Launch:** Nach jedem Build: App starten ohne Crash
4. **Backward-Compatible:** Alte Features nicht brechen
5. **Git-Commit:** Nach jeder Phase commiten mit Description

---

## 🎯 Success Criteria pro Phase

| Phase | Kriterium | Status |
|-------|-----------|--------|
| 1 | Alle 4 Tabs mit Content | ⏳ TODO |
| 2 | Toolbar Buttons funktional | ⏳ TODO |
| 3 | Sidebars responsive | ⏳ TODO |
| 4 | Keyboard Shortcuts aktiv | ⏳ TODO |
| 5 | Settings Window launchable | ⏳ TODO |
| 6 | Advanced Features (Drag-Drop) | ⏳ TODO |

---

**Version:** 1.0 | **Autor:** Architecture Guide | **Status:** Active Development
