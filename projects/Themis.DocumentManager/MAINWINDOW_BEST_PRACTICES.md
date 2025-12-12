# MainWindow Best-Practice Architektur

## 📋 Überblick

Die neue MainWindow-Architektur folgt modernen WPF/MVVM-Prinzipien mit Fokus auf Wartbarkeit, Skalierbarkeit und Fehlerresistenz.

## 🎯 Kernprinzipien

### 1. **Minimales Code-Behind**
- **Nur Constructor + ViewModel-Binding**
- Keine Event Handler im Code-Behind (außer für Menu-Navigation)
- Alle Business-Logik in ViewModels/Services
- Maximale Testbarkeit

```csharp
public partial class MainWindow : Window
{
    private readonly MainViewModel _viewModel;
    private readonly IThemeService _themeService;

    public MainWindow(MainViewModel viewModel, IThemeService themeService, ...)
    {
        try
        {
            InitializeComponent();
            _viewModel = viewModel;
            _themeService = themeService;
            DataContext = _viewModel;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"MainWindow Error: {ex}");
            throw;
        }
    }
}
```

### 2. **XAML-Struktur: Robuste, Nicht-Dynamische Ressourcen**

#### ✅ Sicher:
- **Hex-Farben**: `#f8fafc`, `#ffffff`, `#e5e7eb`
- **SolidColorBrush direkt**: 
  ```xaml
  <SolidColorBrush Color="#f8fafc"/>
  ```
- **Statische TextBlock/Label Styles**

#### ❌ Problematisch:
- **DynamicResource Brushes**: `{DynamicResource SystemControlBackgroundBaseLowBrush}`
- **ModernWpf Theme Dependencies**: Können zur BAML-Compile-Fehlern führen
- **Externe Theme Dictionaries**: Nur bei Garantie der Verfügbarkeit verwenden

### 3. **4-Row Grid-Layout**

```xaml
<Grid>
    <Grid.RowDefinitions>
        <RowDefinition Height="30"/>    <!-- Menu Bar -->
        <RowDefinition Height="50"/>    <!-- Ribbon/Toolbar -->
        <RowDefinition Height="*"/>     <!-- Content Area -->
        <RowDefinition Height="30"/>    <!-- Status Bar -->
    </Grid.RowDefinitions>
    
    <!-- Schnelle Navigation & Controls in jeder Reihe -->
</Grid>
```

### 4. **Event Handler Pattern für Menu-Navigation**

Menu-Clicks sind akzeptabel im Code-Behind (nur für Navigation):

```csharp
private void MenuAuditLogs_Click(object sender, RoutedEventArgs e)
{
    try
    {
        var auditWindow = new AuditLogViewerWindow
        {
            Owner = this,
            WindowStartupLocation = WindowStartupLocation.CenterOwner
        };
        auditWindow.ShowDialog();
    }
    catch (Exception ex)
    {
        StatusText.Text = $"Fehler: {ex.Message}";
    }
}
```

**NICHT zu verwenden:**
- TabControl SelectionChanged Handler im Code-Behind
- ContentControl Content Binding auf komplexe Views mit Event Handling
- PropertyChanged Events direkt im Code-Behind

### 5. **Service-Injected Dependencies**

Alle Services über Constructor DI:

```csharp
public MainWindow(
    MainViewModel viewModel,
    IOfficeIntegrationService officeService,
    IThemeService themeService,
    ISettingsService settingsService,
    IAnimationService animationService,
    AIChatViewModel aiChatViewModel,
    StatusMonitorService statusMonitor,
    IFormTemplateService? formTemplateService = null)
```

## 🏗️ Architektur-Diagramm

```
App.xaml.cs
    ↓
DI Container (ServiceCollection)
    ├── Singleton: MainWindow, MainViewModel
    ├── Transient: Views (AuditLogViewerView, etc.)
    └── Scoped: Services
    
MainWindow.xaml / MainWindow.xaml.cs
    ├── Menu Handler (Code-Behind)
    ├── DataContext = MainViewModel
    └── TabControl (Content über MVVM Binding)
        ├── TabItem: Start (ViewModel)
        ├── TabItem: Documents (lazy-load)
        ├── TabItem: Timeline (lazy-load)
        └── TabItem: Dashboard (lazy-load)
```

## 📐 Color Palette (Safe, Nicht-Dynamisch)

| Farbe | Hex Code | Zweck |
|-------|----------|-------|
| Background Light | `#f8fafc` | Window/Panel Background |
| White | `#ffffff` | Card/Content Background |
| Border | `#e2e8f0` | Separator/Border |
| Text Primary | `#1e293b` | Normal Text |
| Text Secondary | `#64748b` | Subtle Text |

**Implementierung in XAML:**
```xaml
<Window Background="#f8fafc">
    <Menu Background="#ffffff" BorderBrush="#e2e8f0"/>
    <TextBlock Foreground="#334155" Text="..."/>
</Window>
```

## 🔄 Feature-Integration Pattern

### 1. **Neue Top-Level Feature hinzufügen (z.B. Reports)**

**Schritt 1: ViewModel erstellen**
```csharp
public class ReportsViewModel : ObservableObject
{
    [ObservableProperty]
    private ObservableCollection<Report> reports;
    
    [RelayCommand]
    private async Task LoadReportsAsync() { ... }
}
```

**Schritt 2: View erstellen** (`Views/ReportsView.xaml`)
```xaml
<UserControl x:Class="Themis.DocumentManager.Views.ReportsView">
    <DataGrid ItemsSource="{Binding Reports}" />
</UserControl>
```

**Schritt 3: Tab in MainWindow hinzufügen**
```xaml
<TabItem Header="📊 Reports" x:Name="TabReports">
    <views:ReportsView />
</TabItem>
```

**Schritt 4: ViewModel in DI registrieren** (`App.xaml.cs`)
```csharp
services.AddTransient<ReportsViewModel>();
services.AddTransient<ReportsView>();
```

**Schritt 5: Menu-Eintrag optionally**
```xaml
<MenuItem Header="Reports" Click="MenuReports_Click"/>
```

### 2. **Fenster-Dialog hinzufügen**

Pattern wie `AuditLogViewerWindow`:

```csharp
// In MainWindow.xaml.cs
private void MenuNewFeatureWindow_Click(object sender, RoutedEventArgs e)
{
    var window = new FeatureWindow
    {
        Owner = this,
        WindowStartupLocation = WindowStartupLocation.CenterOwner
    };
    window.ShowDialog();
}

// In FeatureWindow.xaml.cs Code-Behind
public partial class FeatureWindow : Window
{
    public FeatureWindow()
    {
        InitializeComponent();
        DataContext = App.GetService<FeatureViewModel>();
    }
    
    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
        if (DataContext is FeatureViewModel vm)
        {
            vm.LoadAsync();
        }
    }
}
```

## 🚀 Best-Practice Checklist

### XAML-Validierung
- [ ] Keine `{DynamicResource}` Brush-Definitionen
- [ ] Alle Farben als Hex-Codes oder `<SolidColorBrush Color="#..."/>`
- [ ] Keine External Theme Dependencies (ModernWpf, SystemControl)
- [ ] Alle x:Names dokumentiert und verwendet
- [ ] Build ohne MC3XXX Fehler (XAML Compilation)

### Code-Behind
- [ ] < 50 Lines für MainWindow.xaml.cs
- [ ] Nur Constructor, ViewModel Binding, Menu-Handlers
- [ ] Try-Catch um InitializeComponent()
- [ ] Keine PropertyChanged/CollectionChanged Handler

### ViewModel
- [ ] Alle UI-Events via RelayCommand
- [ ] ObservableProperty für Binding
- [ ] ObservableCollection für Daten-Listen
- [ ] Async/Await für langwierige Operationen

### Services
- [ ] DI-Injection über Constructor
- [ ] Keine Static Singletons (außer als Last Resort)
- [ ] Interface-Definition in IServices.cs
- [ ] Exception Handling + Logging

### Testing
- [ ] ViewModel separaten Unit-Tests (ohne UI)
- [ ] Services mockbar via Interfaces
- [ ] Integration Tests für Dialog-Flows

## ⚠️ Häufige Fehler

### Problem 1: BAML Cache nach XAML-Änderungen
**Symptom:** `XamlParseException` trotz korrekter Änderungen
**Lösung:** 
```powershell
dotnet clean
Remove-Item bin -Recurse -Force
Remove-Item obj -Recurse -Force
dotnet build
```

### Problem 2: DynamicResource "invalid token"
**Symptom:** Runtime Error "Brush.Parse() invalid token"
**Fehler:**
```xaml
<!-- ❌ FALSCH -->
<Window Background="{DynamicResource SystemControlBackgroundBaseLowBrush}">
```
**Lösung:**
```xaml
<!-- ✅ RICHTIG -->
<Window Background="#f8fafc">
```

### Problem 3: Event Handlers cascade
**Symptom:** Multiple handler calls, state inconsistency
**Fehler:**
```csharp
// ❌ Code-Behind Handler
TabControl.SelectionChanged += (s,e) => { /* nested logic */ }
```
**Lösung:** Verwende MVVM Command in ViewModel statt Code-Behind Handler

### Problem 4: Large Code-Behind
**Symptom:** > 100 Lines Code-Behind
**Fehler:**
```csharp
// ❌ FALSCH: 88+ kompilation errors
public partial class MainWindow : Window {
    private void InitializeAllContent() { /* 500 lines */ }
}
```
**Lösung:** Alles zu ViewModels/Services auslagern

## 📚 Referenzen

- **MVVM Community Toolkit**: https://learn.microsoft.com/de-de/windows/communitytoolkit/mvvm/
- **WPF Data Binding**: https://learn.microsoft.com/de-de/dotnet/desktop/wpf/data/
- **DI in .NET**: https://learn.microsoft.com/de-de/dotnet/core/extensions/dependency-injection
- **BAML Compilation**: https://learn.microsoft.com/de-de/dotnet/desktop/wpf/xaml-services/xamlservices-class

## 🔗 Implementierte Features (aktuell)

- ✅ Menu Bar (File, View, Extras, Help)
- ✅ Ribbon Toolbar (Neu, Öffnen, Speichern, etc.)
- ✅ Tab Control (4 Tabs: Start, Documents, Timeline, Dashboard)
- ✅ Status Bar (mit StatusText binding)
- ✅ Theme Switching (Hell/Dunkel/System)
- ✅ Audit-Logs Fenster Integration
- ✅ Fullscreen Toggle

## 🚀 Nächste Schritte

1. **Tab-Inhalte füllen** mit echten Views
2. **Keyboard Shortcuts** hinzufügen (Ctrl+F, etc.)
3. **Toolbar Button Commands** mit ViewModels verbinden
4. **Sidebar/Navigation** für komplexe Workflows
5. **Lazy Loading** für schwere Views implementieren

---

**Version:** 1.0 | **Datum:** 2024 | **Status:** Production-Ready
