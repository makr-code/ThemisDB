# Rollenbasierte Berechtigungen, Kontextmenüs & Prozess-Verknüpfungen
## Themis Document Manager - Phase 4 Implementation

---

## 📋 Übersicht

Diese Phase implementiert erweiterte Zugriffskontrolle und CRUD-Operationen nach Benutzerrolle, rollenabhängige Kontextmenüs für Maus-Aktionen sowie die Möglichkeit, vorgefertigte Prozesse an Entitäten zu verknüpfen.

### Features:
1. **Rollenbasierte Berechtigungen (RBAC)** - Admin, Manager, Editor, User, Viewer
2. **CRUD-Operationen nach Rolle** - Create, Read, Update, Delete Kontrolle
3. **Dynamische Kontextmenüs** - Rechts-Klick Menüs mit rollenabhängigen Aktionen
4. **Metadatenvorlagen (YAML)** - Strukturierte Schemas für alle Entitätstypen
5. **Prozess-Verknüpfung** - 8 vordefinierte Prozessvorlagen

---

## 🔐 Rollenbasierte Berechtigungen

### Architektur

```csharp
public interface IRoleBasedPermissionService
{
    Task<bool> CanCreateAsync(string userId, EntityType entityType);
    Task<bool> CanReadAsync(string userId, string entityId, EntityType entityType);
    Task<bool> CanUpdateAsync(string userId, string entityId, EntityType entityType);
    Task<bool> CanDeleteAsync(string userId, string entityId, EntityType entityType);
    Task<List<string>> GetPermittedActionsAsync(string userId, EntityType entityType);
    Task<UserRole> GetUserRoleAsync(string userId);
}
```

### Benutzerrollen & Berechtigungen

```
┌─────────────┬─────────┬──────────┬──────────┬─────────┬────────┐
│    Rolle    │ Datei   │ Dokument │ Vorgang  │ Akte    │ Ablage │
├─────────────┼─────────┼──────────┼──────────┼─────────┼────────┤
│ Admin       │ CRUD    │ CRUD     │ CRUD     │ CRUD    │ CRUD   │
│ Manager     │ CRU     │ CRU      │ CRU      │ RU      │ RU     │
│ Editor      │ CRU     │ CRU      │ RU       │ R       │ R      │
│ User        │ R       │ R        │ R        │ R       │ R      │
│ Viewer      │ R       │ R        │ R        │ R       │ R      │
└─────────────┴─────────┴──────────┴──────────┴─────────┴────────┘

C = Create | R = Read | U = Update | D = Delete
```

### Verwendungsbeispiel

```csharp
var permissionService = serviceProvider.GetRequiredService<IRoleBasedPermissionService>();

// Berechtigungen prüfen
bool canCreate = await permissionService.CanCreateAsync(userId, EntityType.Dokument);
bool canDelete = await permissionService.CanDeleteAsync(userId, docId, EntityType.Dokument);

// Alle erlaubten Aktionen abrufen
var actions = await permissionService.GetPermittedActionsAsync(userId, EntityType.Dokument);
// Returns: ["Create", "Read", "Update"]

// Benutzerrolle ermitteln
var role = await permissionService.GetUserRoleAsync(userId);
// Returns: UserRole.Editor
```

---

## 🖱️ Dynamische Kontextmenüs

### Service-Übersicht

```csharp
public interface IContextMenuService
{
    Task<List<ContextMenuAction>> GetContextMenuActionsAsync(
        string userId,
        EntityType entityType,
        string entityId
    );
}
```

### Kontextmenü-Gruppen

Abhängig von Entitätstyp und Benutzerrolle werden folgende Menü-Gruppen generiert:

#### Für Dateien (File Operations):
- 🔍 Anzeigen
- ✏️ Bearbeiten
- 📋 Duplizieren
- 🗑️ Löschen
- ⬇️ Herunterladen
- 👁️ Vorschau
- 🔤 Umbenennen

#### Für Dokumente (Document Operations):
- 🔍 Anzeigen
- ✏️ Bearbeiten
- 📋 Duplizieren
- 🗑️ Löschen
- ⬇️ Herunterladen
- 📜 Versionshistorie
- ✨ Neue Version
- 🔗 Prozess anhängen

#### Für Vorgänge (Process Operations):
- 🔍 Anzeigen
- ✏️ Bearbeiten
- 📋 Duplizieren
- 🗑️ Löschen
- 📅 Timeline anzeigen
- 💬 Kommentare
- 👤 Zuweisen
- 🔗 Prozess anhängen

#### Für Akten & Ablagen (Folder Operations):
- 🔍 Anzeigen
- ✏️ Bearbeiten
- 📋 Duplizieren
- 🗑️ Löschen
- 📂 Inhalte anzeigen
- 📁 Unterordner erstellen
- 🔗 Prozess anhängen

#### Allgemein (alle Entitäten):
- ⭐ Zu Favoriten
- 🏷️ Markierung hinzufügen
- ℹ️ Eigenschaften

### Verwendungsbeispiel

```csharp
// In XAML Code-Behind oder ViewModel
var contextMenuService = App.GetService<IContextMenuService>();

var actions = await contextMenuService.GetContextMenuActionsAsync(
    userId: "user@example.com",
    entityType: EntityType.Dokument,
    entityId: "doc-123"
);

// actions enthält nur die für diese Rolle erlaubten Menü-Einträge
foreach (var action in actions)
{
    // action.Label = "Bearbeiten"
    // action.Action = "Edit"
    // action.Icon = "✏️"
}
```

---

## 📦 Metadatenvorlagen (YAML)

### Dateien (`metadata_datei.yaml`)

**Felder:**
- dateiName (Text, erforderlich)
- dateityp (Dropdown: PDF, DOCX, XLSX, Image, Video, Audio, Archive)
- dateigröße (Number, schreibgeschützt)
- erstellDatum (DateTime, schreibgeschützt)
- änderDatum (DateTime, schreibgeschützt)
- ersteller (User, schreibgeschützt)
- scan_status (Dropdown: Pending, In Progress, Completed, Failed)
- klassifizierung (Dropdown: Public, Internal, Confidential, Restricted)
- tags (MultiSelect)
- beschreibung (TextArea)

### Dokumente (`metadata_dokument.yaml`)

**Felder:**
- dokumentnummer (Text, erforderlich)
- dokumenttitel (Text, erforderlich)
- dokumenttyp (Dropdown: Vertrag, Rechnung, Protokoll, Bericht, Antrag, Beschluss, Schriftverkehr, Genehmigung)
- ausstellDatum (DateTime, erforderlich)
- gültigVon / gültigBis (DateTime)
- aussteller (User)
- unterzeichner (MultiSelect)
- gültigkeitsstatus (Dropdown: Draft, For Signature, Signed, Expired, Revoked)
- klassifizierung (Dropdown)
- language (Dropdown: German, English, French, Dutch)
- tags (MultiSelect)
- referenzen (TextArea)
- notizen (TextArea)

### Vorgänge (`metadata_vorgang.yaml`)

**Felder:**
- vorgangsnummer (Text, erforderlich)
- vorgangstitel (Text, erforderlich)
- vorgangstyp (Dropdown: Anfrage, Antrag, Beschwerde, Genehmigung, Beschaffung, Personal, IT-Support, Sonstiges)
- status (Dropdown: Neu, In Bearbeitung, Wartend, Abgeschlossen, Abgelehnt, Abgebrochen)
- priorität (Dropdown: Niedrig, Mittel, Hoch, Kritisch)
- erstellDatum (DateTime, erforderlich, schreibgeschützt)
- bearbeitungsfrist (DateTime)
- erstellerName (User)
- verantwortlichePerson (User)
- beteiligte (MultiSelect)
- abteilung (Dropdown: HR, IT, Finance, Legal, Operations, Sales, Marketing)
- kostenstelle (Text)
- klassifizierung (Dropdown)
- tags (MultiSelect)
- beschreibung (TextArea)
- nächsteSchritte (TextArea)

### Akten (`metadata_akte.yaml`)

**Felder:**
- aktensignatur (Text, erforderlich)
- aktentitel (Text, erforderlich)
- aktenart (Dropdown: Personenakte, Fallakte, Projektakte, Vertragsakte, Beschwerdakte, Verwaltungsakte)
- eröffnungsDatum (DateTime, erforderlich)
- abschlußDatum (DateTime)
- aufbewahrungsFrist (Number, Jahre)
- vernichtungsDatum (DateTime)
- verantwortlichePerson (User)
- abteilung (Dropdown)
- betreffPerson (Text)
- status (Dropdown: Aktiv, Inaktiv, Archiviert, Vernichtet)
- klassifizierung (Dropdown)
- aufbewahrungsort (Dropdown: Digital, Schrank A, Schrank B, Archiv, Extern)
- tags (MultiSelect)
- notizen (TextArea)
- rechtlicheGrundlage (TextArea)

### Ablagen (`metadata_ablage.yaml`)

**Felder:**
- ablagebezeichnung (Text, erforderlich)
- ablagepfad (Text, erforderlich)
- ablageart (Dropdown: Dokumentsammlung, Prozessordner, Projektordner, Persönlicher Ordner, Archiv, Temporär)
- erstellDatum (DateTime, erforderlich, schreibgeschützt)
- ersteller (User, schreibgeschützt)
- verantwortlichePerson (User)
- abteilung (Dropdown)
- status (Dropdown: Aktiv, Gesperrt, Archiviert, Gelöscht)
- klassifizierung (Dropdown)
- zugriffsbeschränkung (Dropdown: Öffentlich, Abteilungsinternes, Begrenzte Gruppe, Nur Verantwortlicher)
- erlaubteGruppen (MultiSelect)
- aufbewahrungsFrist (Number)
- tags (MultiSelect)
- beschreibung (TextArea)

---

## 🔗 Prozess-Verknüpfungen

### Service-Übersicht

```csharp
public interface IProcessLinkingService
{
    Task<ProcessLink> LinkProcessAsync(ProcessLinkRequest request);
    Task<List<ProcessLink>> GetLinkedProcessesAsync(string entityId);
    Task<bool> UnlinkProcessAsync(string linkId);
    Task<List<ProcessTemplate>> GetAvailableProcessTemplatesAsync(EntityType entityType);
    Task<ProcessTemplate> CreateProcessTemplateAsync(ProcessTemplate template);
}
```

### Vordefinierte Prozessvorlagen

#### 1. **Vier-Augen-Prinzip** (`proc-001`)
- **Kategorie:** Approval
- **Für:** Dokument, Vorgang
- **Beschreibung:** Genehmigung durch zwei autorisierte Personen
- **Schritte:** Submit → Approve1 → Approve2 → Complete

#### 2. **Versionskontrolle** (`proc-002`)
- **Kategorie:** Document Management
- **Für:** Dokument, Datei
- **Beschreibung:** Automatische Versionsverwaltung und Revisions-Tracking
- **Schritte:** Create Version → Document Revision → Update History

#### 3. **Datenschutz-Klassifizierung** (`proc-003`)
- **Kategorie:** Compliance
- **Für:** Datei, Dokument, Akte
- **Beschreibung:** Automatische Einstufung nach DSGVO-Anforderungen
- **Schritte:** Analyze → Classify → Configure Access

#### 4. **Vorgangsbearbeitung** (`proc-004`)
- **Kategorie:** Workflow
- **Für:** Vorgang
- **Beschreibung:** Standard-Workflow: Neu → In Bearbeitung → Abgeschlossen
- **Schritte:** Assign → In Progress → Review → Complete

#### 5. **Aufbewahrungsfrist-Management** (`proc-005`)
- **Kategorie:** Archive
- **Für:** Akte, Ablage
- **Beschreibung:** Automatische Verwaltung von Aufbewahrungsfristen und Vernichtung
- **Schritte:** Calculate Retention → Monitor → Approve Deletion → Execute

#### 6. **Vertraulichkeits-Markierung** (`proc-006`)
- **Kategorie:** Security
- **Für:** Dokument, Akte
- **Beschreibung:** Markierung und Verfolgung von vertraulichen Dokumenten
- **Schritte:** Set Level → Enable Logging → Notify Users

#### 7. **Massenimport-Verarbeitung** (`proc-007`)
- **Kategorie:** Data Import
- **Für:** Datei, Dokument
- **Beschreibung:** Workflow für Massenimporte mit Validierung
- **Schritte:** Validate → Parse → Assign Metadata → Complete

#### 8. **Export & Archivierung** (`proc-008`)
- **Kategorie:** Archive
- **Für:** Akte, Ablage
- **Beschreibung:** Strukturierter Export und Archivierung mit Integritätsprüfung
- **Schritte:** Configure → Export → Validate (SHA-256) → Update Metadata

### Verwendungsbeispiel

```csharp
var processService = serviceProvider.GetRequiredService<IProcessLinkingService>();

// Verfügbare Prozesse für Dokumenttyp abrufen
var templates = await processService.GetAvailableProcessTemplatesAsync(EntityType.Dokument);
// Returns: [proc-001, proc-002, proc-003, proc-006, proc-007]

// Prozess an Dokument verknüpfen
var link = await processService.LinkProcessAsync(new ProcessLinkRequest
{
    EntityId = "doc-123",
    EntityType = EntityType.Dokument,
    ProcessTemplateId = "proc-001",  // Vier-Augen-Prinzip
    LinkedBy = "user@example.com"
});

// Verknüpfte Prozesse abrufen
var linkedProcesses = await processService.GetLinkedProcessesAsync("doc-123");
foreach (var proc in linkedProcesses)
{
    // proc.ProcessTemplateName = "Vier-Augen-Prinzip"
    // proc.Status = ProcessLinkStatus.Active
    // proc.ExecutionCount = 0
}

// Prozess später trennen
await processService.UnlinkProcessAsync(link.Id);
```

---

## 🏗️ Dependency Injection

Die Services sind bereits in `App.xaml.cs` registriert:

```csharp
services.AddSingleton<IRoleBasedPermissionService, RoleBasedPermissionService>();
services.AddSingleton<IContextMenuService, ContextMenuService>();
services.AddSingleton<IProcessLinkingService, ProcessLinkingService>();
```

### Verwendung in ViewModels

```csharp
public partial class DocumentViewModel : ObservableObject
{
    private readonly IRoleBasedPermissionService _permissionService;
    private readonly IContextMenuService _contextMenuService;
    private readonly IProcessLinkingService _processService;

    public DocumentViewModel(
        IRoleBasedPermissionService permissionService,
        IContextMenuService contextMenuService,
        IProcessLinkingService processService)
    {
        _permissionService = permissionService;
        _contextMenuService = contextMenuService;
        _processService = processService;
    }

    private async Task OnRightClickAsync(string docId)
    {
        var actions = await _contextMenuService.GetContextMenuActionsAsync(
            userId,
            EntityType.Dokument,
            docId
        );
        
        // Display context menu
    }
}
```

---

## 🎯 Integrations-Szenarien

### Szenario 1: Admin verwaltet Berechtigungen

```csharp
// Admin kann alle CRUD-Operationen durchführen
var canDelete = await permissionService.CanDeleteAsync(
    "admin@company.com",
    "doc-123",
    EntityType.Dokument
); // true
```

### Szenario 2: Editor mit eingeschränkten Rechten

```csharp
// Editor kann Dokumente erstellen, lesen und ändern, aber nicht löschen
var actions = await permissionService.GetPermittedActionsAsync(
    "editor@company.com",
    EntityType.Dokument
);
// Returns: ["Create", "Read", "Update"]
```

### Szenario 3: Vorgang mit Vier-Augen-Prinzip

```csharp
// Vorgang als kritisch markieren und Vier-Augen-Prinzip verknüpfen
var link = await processService.LinkProcessAsync(new ProcessLinkRequest
{
    EntityId = "proc-456",
    EntityType = EntityType.Vorgang,
    ProcessTemplateId = "proc-001",  // Vier-Augen-Prinzip
    LinkedBy = "manager@company.com"
});

// Nachfolgende Änderungen müssen jetzt zwei Genehmigungen durchlaufen
```

### Szenario 4: Dokumentarchivierung

```csharp
// Akte mit Aufbewahrungsfrist und Archivierungsprozess
var link = await processService.LinkProcessAsync(new ProcessLinkRequest
{
    EntityId = "akte-789",
    EntityType = EntityType.Akte,
    ProcessTemplateId = "proc-005",  // Aufbewahrungsfrist-Management
    LinkedBy = "admin@company.com"
});

// System überwacht automatisch Aufbewahrungsfristen
```

---

## 📝 Dateistruktur

```
Services/
├── RoleBasedPermissionService.cs    # RBAC Service
├── ContextMenuService.cs             # Context Menu Service
└── ProcessLinkingService.cs           # Process Linking Service

Config/
├── metadata_datei.yaml               # Schema für Dateien
├── metadata_dokument.yaml            # Schema für Dokumente
├── metadata_vorgang.yaml             # Schema für Vorgänge
├── metadata_akte.yaml                # Schema für Akten
└── metadata_ablage.yaml              # Schema für Ablagen
```

---

## 🚀 Nächste Schritte

1. **UI-Integration** - Kontextmenüs in XAML-Views implementieren
2. **Audit-Logging** - Alle CRUD-Operationen protokollieren
3. **Prozess-Automation** - Automatische Prozess-Ausführung
4. **Benachrichtigungen** - Events beim Statuswechsel von Prozessen
5. **Erweiterte Filterung** - Vorlagen nach Abteilung/Team filtern
6. **Berechtigungsgruppen** - Rollenbasierte Gruppen statt Einzelbenutzer

---

## 📚 Referenzen

- **Rollenkonzept:** Admin > Manager > Editor > User > Viewer (hierarchisch)
- **Entity-Typen:** Datei, Dokument, Vorgang, Akte, Ablage
- **CRUD-Operationen:** Create, Read, Update, Delete
- **Prozess-Kategorien:** Approval, Document Management, Compliance, Workflow, Archive, Security, Data Import
