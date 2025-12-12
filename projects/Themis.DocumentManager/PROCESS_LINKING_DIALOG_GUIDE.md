# Process Linking Dialog - Dokumentation

## Übersicht

Das **ProcessLinkingDialog** ist ein modales Fenster, das es Benutzern ermöglicht, vordefinierte Prozessvorlagen mit Dokumenten, Vorgängen, Akten oder anderen Entitäten zu verknüpfen.

---

## Komponenten

### 1. ProcessLinkingDialogViewModel
**Datei:** `ViewModels/ProcessLinkingDialogViewModel.cs` (269 Zeilen)

**Hauptverantwortlichkeiten:**
- Laden verfügbarer Prozessvorlagen
- Anzeige bereits verknüpfter Prozesse
- Handling von Prozess-Verknüpfung und -Entfernung
- Audit-Logging aller Aktionen

**Key Properties:**
```csharp
[ObservableProperty] ObservableCollection<ProcessTemplateViewModel> AvailableTemplates
[ObservableProperty] ProcessTemplateViewModel? SelectedTemplate
[ObservableProperty] ObservableCollection<ProcessLinkViewModel> LinkedProcesses
[ObservableProperty] string? StatusMessage
[ObservableProperty] bool IsLoading
```

**Key Commands:**
```csharp
[RelayCommand] public async Task LinkProcessAsync()
[RelayCommand] public async Task UnlinkProcessAsync(ProcessLinkViewModel? link)
[RelayCommand] public void CloseDialog()
```

**Initialisierung:**
```csharp
public async Task InitializeAsync(string entityId, string entityType)
{
    EntityId = entityId;
    EntityType = entityType;
    IsLoading = true;
    
    await LoadAvailableTemplatesAsync();
    await LoadLinkedProcessesAsync();
    
    IsLoading = false;
}
```

---

### 2. ProcessLinkingDialog.xaml (XAML-View)
**Datei:** `Views/ProcessLinkingDialog.xaml` (136 Zeilen)

**Struktur:**

```
┌─ Header (Title + Description)
│
├─ Main Content (ScrollViewer)
│  ├─ Available Templates Section
│  │  ├─ ItemsControl mit DataTemplate
│  │  └─ Template Cards mit Mouse-Click-Selection
│  │
│  ├─ Divider
│  │
│  └─ Linked Processes Section
│     ├─ ItemsControl mit DataTemplate
│     ├─ Process Items (grüner Hintergrund)
│     └─ Remove Button pro Process
│
├─ Status Message Bar
│
└─ Action Buttons
   ├─ Cancel Button
   └─ Link Process Button
```

**Styling:**
- Hintergrund: Weiß (#ffffff)
- Header: Hellgrau (#f8fafc)
- Template Cards: Weiß, blauer Border bei Auswahl
- Linked Processes: Grüner Hintergrund (#f0fdf4)
- Accent Color: Blau (#3b82f6)
- Danger Button: Rot (#dc2626)

---

### 3. ProcessLinkingDialog.xaml.cs (Code-Behind)
**Datei:** `Views/ProcessLinkingDialog.xaml.cs` (23 Zeilen)

**Event Handler:**
```csharp
private void TemplateItem_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
{
    // Handles template selection when user clicks on a template card
    if (sender is Border border && border.DataContext is ProcessTemplateViewModel template)
    {
        var vm = this.DataContext as ProcessLinkingDialogViewModel;
        if (vm != null)
        {
            vm.SelectedTemplate = template;
        }
    }
}
```

---

## UI Flow

### User Flow: "Prozess anhängen"

```
1. Benutzer rechtsklickt auf Dokument
   ↓
2. ContextMenu erscheint
   ↓
3. Benutzer wählt "🔄 Prozess anhängen"
   ↓
4. ProcessLinkingDialog öffnet sich
   ↓
5. Dialog lädt alle verfügbaren Templates
   ↓
6. Dialog zeigt bereits verknüpfte Prozesse
   ↓
7. Benutzer klickt auf Template (wird blau hervorgehoben)
   ↓
8. Benutzer klickt "✓ Prozess verknüpfen"
   ↓
9. ViewModel ruft LinkProcessAsync() auf
   ↓
10. Service erstellt ProcessLink + Audit Log
   ↓
11. UI aktualisiert + Status "Erfolgreich verknüpft"
   ↓
12. Dialog schließt sich automatisch
```

---

## Verfügbare Prozessvorlagen

Die folgenden 8 Prozessvorlagen sind verfügbar:

### 1. **Vier-Augen-Prinzip** (Category: Genehmigung)
- Anwendbar auf: Dokument, Akte
- Schritte: Review, Approval, Notification

### 2. **Versionskontrolle** (Category: Management)
- Anwendbar auf: Dokument
- Schritte: Create Version, Update Metadata, Archive Previous

### 3. **Datenschutz-Klassifizierung** (Category: Compliance)
- Anwendbar auf: Dokument, Akte, Vorgang
- Schritte: Classify, Tag, Notify

### 4. **Vorgangsbearbeitung** (Category: Workflow)
- Anwendbar auf: Vorgang
- Schritte: Assign, Track, Complete

### 5. **Aufbewahrungsfrist-Management** (Category: Archivierung)
- Anwendbar auf: Dokument, Akte, Ablage
- Schritte: Set Retention, Schedule Disposal

### 6. **Vertraulichkeits-Markierung** (Category: Sicherheit)
- Anwendbar auf: Dokument, Vorgang
- Schritte: Apply Mark, Audit Trail

### 7. **Massenimport-Verarbeitung** (Category: Import)
- Anwendbar auf: Datei, Dokument
- Schritte: Validate, Transform, Import

### 8. **Export & Archivierung** (Category: Export)
- Anwendbar auf: Dokument, Akte, Ablage
- Schritte: Export, Archive, Verify

---

## Service Integration

### ProcessLinkingService Methoden

```csharp
// Alle Prozess-Templates laden
public async Task<List<ProcessTemplate>> GetAllProcessTemplatesAsync()

// Templates basierend auf Entity-Type filtern
public async Task<List<ProcessTemplate>> GetAvailableProcessTemplatesAsync(
    EntityType entityType)

// Prozess mit Entität verknüpfen
public async Task<ProcessLink> LinkProcessAsync(ProcessLinkRequest request)

// Verknüpfung entfernen
public async Task<bool> UnlinkProcessAsync(string linkId)

// Alle verknüpften Prozesse für Entity abrufen
public async Task<List<ProcessLink>> GetLinkedProcessesAsync(string entityId)
```

### AuditLoggingService Integration

```csharp
// Logging bei erfolgreicher Verknüpfung
await _auditLoggingService.LogActionAsync(new AuditLogEntry
{
    UserId = _authService.CurrentUserId,
    ActionType = "LinkProcess",
    EntityType = EntityType,
    EntityId = EntityId,
    Details = $"Prozess '{template.Name}' verknüpft",
    Timestamp = DateTime.UtcNow,
    Result = AuditActionResult.Success
});
```

---

## Fehlerbehandlung

### Fehlerszenarien

| Fehler | Behandlung |
|--------|-----------|
| Template nicht gefunden | StatusMessage: "Template nicht verfügbar" |
| LinkProcess schlägt fehl | Log mit AuditActionResult.Failed |
| UnlinkProcess schlägt fehl | Benutzer informiert + Retry möglich |
| Keine Templates verfügbar | "Keine Prozessvorlagen verfügbar" |

### Beispiel: Permission Denied

```csharp
if (!await _permissionService.CanAttachProcess(userId, entityId))
{
    StatusMessage = "Sie haben nicht die Berechtigung, Prozesse anzuhängen";
    await _auditLoggingService.LogActionAsync(new AuditLogEntry
    {
        Result = AuditActionResult.Denied
    });
}
```

---

## Converter & Resources

### In App.xaml registriert:

```xaml
<local:BoolToVisibilityConverter x:Key="BoolToVisibilityConverter"/>
<local:NullToBoolConverter x:Key="NullToBoolConverter"/>
<local:BoolToColorConverter x:Key="BoolToColorConverter"/>
<local:EmptyToVisibilityConverter x:Key="EmptyToVisibilityConverter"/>
```

### Verwendung in XAML:

```xaml
<!-- Selection-Highlight -->
Background="{Binding RelativeSource={RelativeSource Self}, 
    Path=(local:ProcessLinkingDialog.IsSelected), 
    Converter={StaticResource BoolToColorConverter}, 
    ConverterParameter='#eff6ff|#ffffff'}"

<!-- Link Button deaktivieren wenn nichts ausgewählt -->
IsEnabled="{Binding SelectedTemplate, 
    Converter={StaticResource NullToBoolConverter}}"

<!-- Leerer Zustand -->
Visibility="{Binding LinkedProcesses, 
    Converter={StaticResource EmptyToVisibilityConverter}}"
```

---

## Workflow mit Screenshots (konzeptionell)

### 1. Dialog öffnet sich
```
┌─────────────────────────────────────────────┐
│ 🔄 Prozess verknüpfen                       │
│ Wählen Sie einen Prozess zum Verknüpfen     │
├─────────────────────────────────────────────┤
│ Verfügbare Prozessvorlagen                  │
│                                             │
│ ┌─────────────────────────────────────────┐ │
│ │ Vier-Augen-Prinzip          [Genehm.] │ │
│ │ Geheime Genehmigungen...               │ │
│ │ Schritte: 3 | Anwendbar: Dokument, ... │ │
│ └─────────────────────────────────────────┘ │
│                                             │
│ ┌─────────────────────────────────────────┐ │
│ │ Versionskontrolle            [Manage] │ │
│ │ Automatische Versionierung...          │ │
│ │ Schritte: 3 | Anwendbar: Dokument     │ │
│ └─────────────────────────────────────────┘ │
│                                             │
│ ...                                         │
│                                             │
│ Bereits verknüpfte Prozesse                 │
│ (keine Prozesse verknüpft)                  │
│                                             │
│ ─────────────────────────────────────────   │
│ Bereit für Prozess-Verknüpfung              │
│                                             │
│        [Abbrechen]  [✓ Verknüpfen]         │
└─────────────────────────────────────────────┘
```

### 2. Template ausgewählt
```
┌─ Header ─────────────────────────────────┐
│ 🔄 Prozess verknüpfen                   │
├──────────────────────────────────────────┤
│ ┌────────────────────────────────────┐  │
│ │ Versionskontrolle (SELECTED) ████  │  │
│ │ Blue Border                        │  │
│ └────────────────────────────────────┘  │
├──────────────────────────────────────────┤
│ Abbrechen          [✓ Verknüpfen] (aktiv)│
└──────────────────────────────────────────┘
```

### 3. Nach erfolgreicher Verknüpfung
```
Bereits verknüpfte Prozesse

┌─────────────────────────────────────────┐
│ proc-002 | Active | [Entfernen]         │
└─────────────────────────────────────────┘
```

---

## Zukünftige Erweiterungen

- [ ] Process-Ausführungs-Monitor (Schritte anzeigen)
- [ ] Prozess-Template-Editor (Custom Templates erstellen)
- [ ] Batch-Verknüpfung (mehrere Dokumente)
- [ ] Process-Automation (zeitgesteuert ausführen)
- [ ] Process-Status-Dashboard
- [ ] Prozess-Abhängigkeiten (Reihenfolge)

---

## Best Practices

### Für Entwickler

1. **Immer InitializeAsync() aufrufen** vor dem Dialog anzeigen
2. **Audit-Logging ist automatisch** - keine zusätzliche Programmierung nötig
3. **Permissions checken** über RoleBasedPermissionService
4. **Status-Messages anzeigen** für User-Feedback

### Für User

1. Wählen Sie einen Prozess durch Klick
2. Klicken Sie "Verknüpfen" um zu bestätigen
3. Dialog schließt sich automatisch bei erfolgreicher Verknüpfung
4. Verknüpfte Prozesse können mit "Entfernen" gelöscht werden

---

## Build-Status

✅ **Erfolgreich kompiliert: 0 Fehler, 110 Warnungen**

```
ProcessLinkingDialogViewModel.cs: OK
ProcessLinkingDialog.xaml: OK
ProcessLinkingDialog.xaml.cs: OK
ValueConverters.cs: 4 neue Converter hinzugefügt
App.xaml: Converter registriert
```
