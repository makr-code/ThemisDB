# RBAC & Context Menu UI Integration Guide

## Status: ✅ Phase 4 Complete

Integration von rollenbasierten Berechtigungen, Kontextmenüs und Audit-Logging in die Benutzeroberfläche abgeschlossen.

---

## 1. Architektur-Übersicht

```
DocumentBrowserView (XAML)
    ↓
DocumentBrowserView.xaml.cs (Event Handler)
    ↓
DocumentBrowserViewModel (MVVM)
    ├── LoadDocumentsAsync()
    ├── OnDocumentRightClickAsync() ← Trigger Context Menu
    └── ExecuteContextMenuActionAsync() ← Execute Action
        ↓
    IRoleBasedPermissionService (Permission Check)
    IContextMenuService (Menu Generation)
    IProcessLinkingService (Process Attachment)
    IAuditLoggingService (Audit Logging)
```

---

## 2. Services

### IAuditLoggingService

**Dateien:**
- `Services/AuditLoggingService.cs` (150 Zeilen)

**Hauptfunktionen:**
- `LogActionAsync()` - Protokolliert CRUD-Operationen
- `GetAuditLogsAsync()` - Ruft Logs für ein bestimmtes Dokument ab
- `GetUserAuditLogsAsync()` - Ruft Logs für einen Benutzer ab
- `GetLogsInRangeAsync()` - Zeitbasierte Abfrage
- `GetStatisticsAsync()` - Audit-Statistiken

**Datenmodell:**
```csharp
public class AuditLogEntry
{
    public string Id { get; set; }
    public string UserId { get; set; }
    public string ActionType { get; set; }      // "View", "Edit", "Delete", etc.
    public string EntityType { get; set; }      // "Dokument", "Akte", etc.
    public string EntityId { get; set; }
    public string? Details { get; set; }
    public DateTime Timestamp { get; set; }
    public DateTime LoggedAt { get; set; }
    public AuditActionResult? Result { get; set; }  // Success/Failed/Denied
}

public enum AuditActionResult
{
    Success,
    Failed,
    Denied,
    PartialSuccess
}
```

**Beispiel:**
```csharp
// Log eines Zugriffs
await _auditLoggingService.LogActionAsync(new AuditLogEntry
{
    UserId = CurrentUserId,
    ActionType = "Edit",
    EntityType = "Dokument",
    EntityId = documentId,
    Details = $"Document '{title}' edited",
    Timestamp = DateTime.UtcNow
});
```

---

## 3. UI-Komponenten

### DocumentBrowserView.xaml

**Kontextmenü-Integration:**
- Rechtsklick auf Dokument triggert `OnPreviewMouseRightButtonDown`
- Kontextmenü wird dynamisch aus `ContextMenuActions` befüllt
- Menüpunkte sind basierend auf Benutzerrolle gefiltert

**XAML-Struktur:**
```xaml
<Border Style="{StaticResource DocumentCardStyle}" Width="250" Height="200">
    <Border.ContextMenu>
        <ContextMenu ItemsSource="{Binding DataContext.ContextMenuActions, 
                                  RelativeSource={RelativeSource AncestorType=ItemsControl}}"
                     Tag="{Binding DataContext, 
                           RelativeSource={RelativeSource AncestorType=ItemsControl}}">
            <ContextMenu.ItemTemplate>
                <DataTemplate>
                    <MenuItem Header="{Binding Label}" 
                              IsEnabled="{Binding IsEnabled}"
                              Click="MenuItem_Click"
                              Tag="{Binding}">
                        <MenuItem.Icon>
                            <TextBlock Text="{Binding Icon}" FontSize="14"/>
                        </MenuItem.Icon>
                    </MenuItem>
                </DataTemplate>
            </ContextMenu.ItemTemplate>
        </ContextMenu>
    </Border.ContextMenu>
    <!-- Document Card Content -->
</Border>
```

### DocumentBrowserViewModel

**Key Properties:**
```csharp
[ObservableProperty] ObservableCollection<DocumentItemViewModel> documents
[ObservableProperty] DocumentItemViewModel? selectedDocument
[ObservableProperty] List<ContextMenuAction> contextMenuActions
[ObservableProperty] bool isLoading
[ObservableProperty] string? currentUserId
```

**Event Flow:**
1. User right-clicks on document
2. `OnPreviewMouseRightButtonDown` in Code-Behind fires
3. `OnDocumentRightClickAsync(DocumentItemViewModel)` called
4. ContextMenuService generates role-based actions
5. Menu displayed to user
6. User clicks menu item
7. `MenuItem_Click` handler executes
8. `ExecuteContextMenuActionAsync(ContextMenuAction)` dispatches to action handler
9. Action executed + Audit log created

---

## 4. Berechtigungsfluss

### Rollenmatrix

| Role | Datei | Dokument | Vorgang | Akte | Ablage |
|------|-------|----------|---------|------|--------|
| **Admin** | CRUD | CRUD | CRUD | CRUD | CRUD |
| **Manager** | CR__ | CRU_ | CRU_ | CRU_ | CRU_ |
| **Editor** | CR__ | CRU_ | CR__ | CR__ | ___D |
| **User** | R___ | RU__ | R___ | R___ | R___ |
| **Viewer** | R___ | R___ | R___ | R___ | R___ |

*C=Create, R=Read, U=Update, D=Delete*

### Kontext-Menu Generierung

**Beispiel für Admin-Benutzer auf Dokument:**
```
✎ Edit
📋 View Revisions
🔗 Create Revision
🔄 Attach Process
❤️ Add to Favorites
🏷️ Add Tag
⚙️ Properties
---
🗑️ Delete (red)
```

**Beispiel für Viewer auf Dokument:**
```
📋 View Revisions
❤️ Add to Favorites
🏷️ Add Tag
⚙️ Properties
```

---

## 5. Audit-Logging

### Protokollierte Aktionen

```
[AUDIT] ContextMenu | User: urn:themis:user:max-mustermann | Entity: doc-12345 | 2024-12-19 14:32:15
[AUDIT] Edit | User: urn:themis:user:max-mustermann | Entity: doc-12345 | 2024-12-19 14:32:18
[AUDIT] Delete | User: urn:themis:user:max-mustermann | Entity: doc-12345 | 2024-12-19 14:32:20
```

### Zugriff auf Audit-Logs

```csharp
// Alle Logs für ein Dokument
var docLogs = await _auditLoggingService.GetAuditLogsAsync(documentId);

// Alle Logs eines Benutzers
var userLogs = await _auditLoggingService.GetUserAuditLogsAsync(userId);

// Zeitbereich-Abfrage
var timeLogs = await _auditLoggingService.GetLogsInRangeAsync(
    from: DateTime.Today,
    to: DateTime.Now
);

// Statistiken
var stats = await _auditLoggingService.GetStatisticsAsync();
Console.WriteLine($"Insgesamt {stats.TotalLogCount} Logs");
Console.WriteLine($"{stats.UniqueUsers} einzigartige Benutzer");
```

---

## 6. Implementierte Aktionen

### Standard-Aktionen (pro Rolle gefiltert)

| Aktion | Icon | Gruppe | Beschreibung |
|--------|------|--------|-------------|
| View | 👁️ | Basic | Dokument anschauen |
| Edit | ✎ | Basic | Dokument bearbeiten |
| Delete | 🗑️ | Danger | Dokument löschen |
| Download | ⬇️ | FileOps | Download |
| ViewRevisions | 📋 | DocOps | Versionsverlauf anschauen |
| CreateRevision | 🔗 | DocOps | Neue Version erstellen |
| AttachProcess | 🔄 | Organization | Prozess anhängen |
| AddFavorite | ❤️ | Basic | Zu Favoriten hinzufügen |
| AddTag | 🏷️ | Info | Tag hinzufügen |
| ShowProperties | ⚙️ | Info | Eigenschaften anzeigen |

---

## 7. Integration in App.xaml.cs

```csharp
// DI-Registrierung
services.AddSingleton<IRoleBasedPermissionService, RoleBasedPermissionService>();
services.AddSingleton<IContextMenuService, ContextMenuService>();
services.AddSingleton<IProcessLinkingService, ProcessLinkingService>();
services.AddSingleton<IAuditLoggingService, AuditLoggingService>();
```

---

## 8. Workflow-Beispiel: Dokument Löschen

```
1. Benutzer mit "Manager"-Rolle rechtsklickt auf Dokument
2. OnPreviewMouseRightButtonDown() triggert
3. OnDocumentRightClickAsync() aufgerufen
   - ContextMenuService prüft Manager-Berechtigungen
   - Generiert Menü (Edit, ViewRevisions, Download, Properties)
   - Delete ist NICHT verfügbar (nur für Admin/Manager auf diesem Dokument)
4. Benutzer klickt "Delete"
5. MenuItem_Click() Handler ausgeführt
6. ExecuteContextMenuActionAsync("Delete") aufgerufen
   - IPermissionService.CanDeleteAsync() prüft Berechtigung → ✓ Erlaubt
   - DeleteDocumentAsync() wird aufgerufen
   - Audit Log erstellt: "User x deleted Document y"
   - UI aktualisiert
```

---

## 9. Fehlerbehandlung

### Permission Denied
```csharp
if (!await _permissionService.CanDeleteAsync(userId, entityId, entityType))
{
    // Log: Permission Denied
    await _auditLoggingService.LogActionAsync(new AuditLogEntry
    {
        ActionType = "Delete",
        Result = AuditActionResult.Denied
    });
    
    // UI: Show error message
    MessageBox.Show("Sie haben keine Berechtigung, dieses Dokument zu löschen.");
    return;
}
```

### Action Execution Failure
```csharp
try
{
    await DeleteDocumentAsync(selectedDocument.Id);
}
catch (Exception ex)
{
    await _auditLoggingService.LogActionAsync(new AuditLogEntry
    {
        Result = AuditActionResult.Failed,
        Details = ex.Message
    });
}
```

---

## 10. Nächste Phasen (TODO)

- [ ] Prozess-Ankettungs-Dialog mit Vorschau
- [ ] Metadaten-Formular mit YAML-Template-Binding
- [ ] Massenoperationen (Mehrfach-Select + Context Menu)
- [ ] Audit-Log-Viewer in Admin-Panel
- [ ] Workflow-Status-Tracking
- [ ] Benachrichtigungen bei Aktionen

---

## 11. Dateien-Übersicht

```
Services/
├── AuditLoggingService.cs (NEU - 150 Zeilen)
├── RoleBasedPermissionService.cs (185 Zeilen)
├── ContextMenuService.cs (290 Zeilen)
└── ProcessLinkingService.cs (320 Zeilen)

ViewModels/
├── DocumentBrowserViewModel.cs (250 Zeilen - erweitert)
└── ViewModels.cs (MODIFIZIERT - alte DocBrowserVM entfernt)

Views/
├── DocumentBrowserView.xaml (erweitert um ContextMenu)
└── DocumentBrowserView.xaml.cs (erweitert um Event Handler)

Config/
└── metadata_*.yaml (5 Dateien - unverändert)

App.xaml.cs (MODIFIZIERT - AuditLoggingService registriert)
```

---

## 12. Build-Status

✅ **Build erfolgreich: 0 Fehler, 110 Warnungen**

```
Themis.DocumentManager.csproj erfolgreich kompiliert
Alle Services registriert und verfügbar
UI-Bindings aktualisiert
RBAC-Fluss implementiert
```

---

## Kontakt & Support

Bei Fragen zur Integration kontaktieren Sie das Development Team.
