# Breadcrumb Tab-Integration

## Übersicht

Die Breadcrumb-Navigation passt sich nun automatisch an den aktiven Tab an und zeigt kontextspezifische Navigationspfade.

## Implementierung

### BreadcrumbViewModel Erweiterungen

#### `SetContextForTab(string tabName)`
Setzt den Breadcrumb-Pfad basierend auf dem aktiven Tab:

**Dashboard** (`TabDashboard`)
```
🏠 Startseite → 📊 Dashboard
```

**AI Chat** (`TabAIChat`)
```
🏠 Startseite → 🤖 AI Assistent → 💬 Neue Unterhaltung
```

**Gantt** (`TabGantt`)
```
🏠 Startseite → 📈 Projektmanagement → 📊 Gantt-Ansicht
```

**Timeline** (`TabTimeline`)
```
🏠 Startseite → 📈 Projektmanagement → 📅 Timeline-Ansicht
```

**Aufgaben** (`TabTasks`)
```
🏠 Startseite → 📋 Aufgabenverwaltung → ✓ Meine Aufgaben
```

**Vorschau** (`TabPreview`)
```
🏠 Startseite → 📄 Dokumente → 👁 Vorschau
```

#### `AppendToBreadcrumb(string icon, string title, BreadcrumbLevel level)`
Erweitert den bestehenden Breadcrumb-Pfad um ein weiteres Element. Nützlich für tiefere Navigation innerhalb eines Tabs.

**Beispiel:**
```csharp
// In einem Dokument-Tab
_viewModel.BreadcrumbViewModel.AppendToBreadcrumb(
    "📄", 
    "Bauplan_Entwurf_v2.pdf", 
    BreadcrumbLevel.Document
);
```

### MainWindow Integration

#### Event Handler
```csharp
private void CenterContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
{
    if (CenterContent.SelectedItem is TabItem selectedTab)
    {
        string? tabName = selectedTab.Name;
        
        // Fallback: Extrahiere aus Header wenn Name leer
        if (string.IsNullOrEmpty(tabName))
        {
            if (selectedTab.Header is StackPanel headerPanel)
            {
                var textBlock = headerPanel.Children.OfType<TextBlock>().FirstOrDefault();
                tabName = textBlock?.Text;
            }
            else
            {
                tabName = selectedTab.Header?.ToString();
            }
        }
        
        // Aktualisiere Breadcrumb
        if (!string.IsNullOrEmpty(tabName) && _viewModel?.BreadcrumbViewModel != null)
        {
            _viewModel.BreadcrumbViewModel.SetContextForTab(tabName);
        }
    }
}
```

#### Registrierung im Constructor
```csharp
Loaded += (s, e) => 
{
    // ...
    CenterContent.SelectionChanged += CenterContent_SelectionChanged;
    
    // Initiale Breadcrumb für Dashboard
    _viewModel.BreadcrumbViewModel?.SetContextForTab("TabDashboard");
};
```

## Verwendung

### Automatische Updates
Die Breadcrumb wird automatisch aktualisiert wenn:
- Ein anderer Tab ausgewählt wird
- Ein neuer Tab dynamisch geöffnet wird (via `OpenOrSwitchToTab`)

### Manuelle Updates
Für spezifische Szenarien können Sie die Breadcrumb manuell erweitern:

```csharp
// Nach Öffnen eines Dokuments
_viewModel.BreadcrumbViewModel.AppendToBreadcrumb(
    "📂", 
    "Baugenehmigungen 2025", 
    BreadcrumbLevel.File
);

_viewModel.BreadcrumbViewModel.AppendToBreadcrumb(
    "📋", 
    "Antrag Mustermann", 
    BreadcrumbLevel.Process
);
```

### Vollständiger Custom-Pfad
```csharp
_viewModel.BreadcrumbViewModel.UpdateBreadcrumb(
    authority: "Stadtverwaltung München",
    repository: "Bauamt",
    file: "Baugenehmigungen 2025",
    process: "Antrag Mustermann",
    document: "Bauplan_Entwurf_v2.pdf"
);
```

## Tab-Namen Konvention

Stellen Sie sicher, dass dynamisch erstellte Tabs konsistente Namen verwenden:

```csharp
// ✅ Gut - Name mit Präfix
OpenOrSwitchToTab("TabAIChat", "🤖 AI Chat", chatView);

// ✅ Auch gut - Header wird als Fallback verwendet
OpenOrSwitchToTab("", "🤖 AI Chat", chatView);

// ❌ Schlecht - Inkonsistente Namen
OpenOrSwitchToTab("Chat1", "AI Chat", chatView); 
```

## Zukünftige Erweiterungen

- **Dokument-Navigation**: Bei Öffnen eines Dokuments sollte die vollständige Hierarchie geladen werden
- **AI-Vorschläge**: `SuggestedItems` in `BreadcrumbItem` können für intelligente Navigation genutzt werden
- **Navigation-Befehle**: `NavigateToCommand` kann erweitert werden um tatsächlich Views zu wechseln
- **Persistenz**: Breadcrumb-Historie speichern für Zurück/Vor-Navigation

## Status

✅ Implementiert und getestet
- Tab-Wechsel aktualisiert Breadcrumb
- Initiale Breadcrumb beim Start (Dashboard)
- Kontextspezifische Pfade für alle Standard-Tabs

⏳ Geplant
- Integration mit Dokument-Metadaten
- Breadcrumb-basierte Navigation (Click-Handler)
- Historie und Zurück-Button
