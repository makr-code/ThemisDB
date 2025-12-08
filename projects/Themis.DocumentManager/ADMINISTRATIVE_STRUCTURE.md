# Behördliche Aktenstruktur nach deutschem Verwaltungsrecht

## Übersicht

Das ThemisDB Document Manager System implementiert eine vollständige hierarchische Aktenstruktur nach deutschem Verwaltungsrecht mit integrierter Prozess-Timeline. Die Struktur basiert auf dem ThemisDB URN-System für eindeutige Identifizierung aller Entitäten.

## Hierarchie-Ebenen

### 7-Ebenen-Modell

```
1. Behörde (Authority)
   └── 2. Ablage (Filing)
       └── 3. Akte (File)
           ├── 4. Unterakte (SubFile) [optional]
           └── 5. Vorgang (Process)
               └── 6. Dokument (Document)
                   └── 7. Datei (FileAttachment)
```

## URN-Schema

Jede Ebene hat ein eindeutiges URN-Schema basierend auf dem ThemisDB-Standard:

### 1. Behörde (Authority)
```
URN: urn:themis:authority:{id}
Beispiel: urn:themis:authority:bmi-001
```

### 2. Ablage (Filing)
```
URN: urn:themis:authority:{authorityId}:filing:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv
```

### 3. Akte (File)
```
URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv:file:2024-123
```

### 4. Unterakte (SubFile)
```
URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:subfile:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv:file:2024-123:subfile:01
```

### 5. Vorgang (Process)
```
URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv:file:2024-123:process:v-001
```

### 6. Dokument (Document)
```
URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{processId}:document:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv:file:2024-123:process:v-001:document:d-001
```

### 7. Datei (FileAttachment)
```
URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{processId}:document:{documentId}:attachment:{id}
Beispiel: urn:themis:authority:bmi-001:filing:abt-iv:file:2024-123:process:v-001:document:d-001:attachment:a-001
```

## Prozess-Timeline

### Zentrale Timeline für alle Vorgänge

Die Prozess-Timeline erfasst **ALLE** Ereignisse über die gesamte Hierarchie hinweg:

```
URN: urn:themis:timeline:event:{id}
```

**Jedes Timeline-Event enthält:**
- Verknüpfungen zu allen relevanten Hierarchie-Ebenen
- Zeitstempel
- Event-Typ (FileCreated, ProcessAssigned, DocumentSigned, etc.)
- Akteur (wer hat die Aktion durchgeführt)
- Änderungsdetails (was wurde geändert)
- Kontext und Kommentare

### Event-Typen

**Akte (File):**
- `FileCreated` - Akte angelegt
- `FileOpened` - Akte geöffnet
- `FileClosed` - Akte geschlossen
- `FileArchived` - Akte archiviert
- `FileStatusChanged` - Status geändert

**Vorgang (Process):**
- `ProcessCreated` - Vorgang angelegt
- `ProcessStarted` - Vorgang gestartet
- `ProcessAssigned` - Vorgang zugewiesen
- `ProcessStatusChanged` - Status geändert
- `ProcessCompleted` - Vorgang abgeschlossen
- `ProcessCancelled` - Vorgang abgebrochen

**Dokument (Document):**
- `DocumentCreated` - Dokument erstellt
- `DocumentReceived` - Dokument eingegangen
- `DocumentSent` - Dokument versandt
- `DocumentSigned` - Dokument unterschrieben
- `DocumentApproved` - Dokument genehmigt
- `DocumentRejected` - Dokument abgelehnt

**Workflow:**
- `StepCompleted` - Schritt abgeschlossen
- `StepSkipped` - Schritt übersprungen
- `WorkflowStateChanged` - Workflow-Status geändert

**Sonstiges:**
- `CommentAdded` - Kommentar hinzugefügt
- `DeadlineChanged` - Frist geändert
- `ParticipantAdded` - Beteiligter hinzugefügt
- `ParticipantRemoved` - Beteiligter entfernt
- `MetadataChanged` - Metadaten geändert

## Datenmodell-Details

### Behörde (Authority)

```csharp
public class Authority
{
    public string Id { get; set; }
    public string Urn => $"urn:themis:authority:{Id}";
    public string Name { get; set; }           // "Bundesministerium des Innern"
    public string ShortName { get; set; }      // "BMI"
    public string Type { get; set; }           // "Bundesbehörde"
    public string OfficialCode { get; set; }   // Behördenschlüssel
    public DateTime CreatedAt { get; set; }
}
```

### Akte (AdministrativeFile)

```csharp
public class AdministrativeFile
{
    public string Id { get; set; }
    public string FileNumber { get; set; }     // "IV C 5 - 123/2024"
    public string Subject { get; set; }        // Betreff der Akte
    public string Category { get; set; }       // Sachgebiet
    public FileStatus Status { get; set; }     // Active, Closed, Archived
    public DateTime OpenedAt { get; set; }
    public DateTime? ClosedAt { get; set; }
    public DateTime? ArchiveDate { get; set; }
    public int RetentionPeriodYears { get; set; } = 10;
    
    // Beteiligte
    public List<string> Participants { get; set; }
    public string ResponsibleOfficer { get; set; }
    public string FileManager { get; set; }    // Aktenführer
    
    // Klassifizierung
    public SecurityClassification SecurityLevel { get; set; }
    public string AccessRestriction { get; set; }
}
```

**Status-Werte:**
- `Active` - In Bearbeitung
- `Suspended` - Ruhend
- `Closed` - Geschlossen
- `Archived` - Archiviert

**Sicherheitsklassifizierung:**
- `Public` - Öffentlich
- `Internal` - Intern
- `Confidential` - Vertraulich
- `Secret` - Geheim
- `TopSecret` - Streng geheim

### Vorgang (AdministrativeProcess)

```csharp
public class AdministrativeProcess
{
    public string Id { get; set; }
    public string ProcessNumber { get; set; }
    public string Subject { get; set; }
    public ProcessType Type { get; set; }
    public ProcessStatus Status { get; set; }
    
    // Zeitstempel
    public DateTime CreatedAt { get; set; }
    public DateTime? StartedAt { get; set; }
    public DateTime? CompletedAt { get; set; }
    public DateTime? DueDate { get; set; }
    
    // Workflow
    public string WorkflowState { get; set; }
    public List<ProcessStep> Steps { get; set; }
}
```

**Vorgangstypen:**
- `Administrative` - Verwaltungsvorgang
- `Legal` - Rechtsvorgang
- `Financial` - Finanzvorgang
- `Personnel` - Personalvorgang
- `Procurement` - Beschaffungsvorgang
- `Construction` - Bauvorgang
- `Licensing` - Genehmigungsvorgang
- `Complaint` - Beschwerdeverfahren
- `Information` - Informationsvorgang

### Dokument (AdministrativeDocument)

```csharp
public class AdministrativeDocument
{
    public string Id { get; set; }
    public string DocumentNumber { get; set; }
    public string Title { get; set; }
    public DocumentType Type { get; set; }
    public DocumentDirection Direction { get; set; }
    
    // Zeitstempel
    public DateTime CreatedAt { get; set; }
    public DateTime DocumentDate { get; set; }  // Dokumentendatum
    public DateTime ReceivedAt { get; set; }    // Eingangsdatum
    
    // Parteien
    public string Sender { get; set; }
    public string Recipient { get; set; }
    public string Author { get; set; }
    
    // Signaturen
    public bool RequiresSignature { get; set; }
    public List<Signature> Signatures { get; set; }
}
```

**Dokumenttypen:**
- `Letter` - Brief
- `Email` - E-Mail
- `Memo` - Vermerk
- `Report` - Bericht
- `Decision` - Bescheid
- `Application` - Antrag
- `Contract` - Vertrag
- `Invoice` - Rechnung
- `Protocol` - Protokoll
- `Certificate` - Bescheinigung
- `Form` - Formular

**Richtung:**
- `Incoming` - Eingang
- `Outgoing` - Ausgang
- `Internal` - Intern

## Verwendungsbeispiele

### 1. Akte anlegen

```csharp
var authority = await adminService.GetAuthorityByIdAsync("bmi-001");
var filing = (await adminService.GetFilingsByAuthorityAsync(authority.Id)).First();

var file = new AdministrativeFile
{
    AuthorityId = authority.Id,
    FilingId = filing.Id,
    FileNumber = "IV C 5 - 123/2024",
    Subject = "Beschaffung neuer IT-Systeme",
    Category = "Beschaffung",
    ResponsibleOfficer = "Max Mustermann",
    FileManager = "Max Mustermann",
    SecurityLevel = SecurityClassification.Internal,
    RetentionPeriodYears = 10
};

file = await adminService.CreateFileAsync(file);
// Erzeugt automatisch Timeline-Event: FileCreated
```

### 2. Vorgang starten

```csharp
var process = new AdministrativeProcess
{
    FileId = file.Id,
    FilingId = file.FilingId,
    AuthorityId = file.AuthorityId,
    ProcessNumber = "V-001-2024",
    Subject = "Ausschreibung IT-Systeme",
    Type = ProcessType.Procurement,
    InitiatedBy = "Max Mustermann",
    DueDate = DateTime.UtcNow.AddDays(30),
    Steps = new List<ProcessStep>
    {
        new ProcessStep { StepNumber = 1, Name = "Bedarfsermittlung" },
        new ProcessStep { StepNumber = 2, Name = "Ausschreibung vorbereiten" },
        new ProcessStep { StepNumber = 3, Name = "Angebote einholen" },
        new ProcessStep { StepNumber = 4, Name = "Angebote bewerten" },
        new ProcessStep { StepNumber = 5, Name = "Zuschlag erteilen" }
    }
};

process = await adminService.CreateProcessAsync(process);
// Erzeugt Timeline-Event: ProcessCreated
```

### 3. Dokument erstellen und unterschreiben

```csharp
var document = new AdministrativeDocument
{
    ProcessId = process.Id,
    FileId = process.FileId,
    FilingId = process.FilingId,
    AuthorityId = process.AuthorityId,
    DocumentNumber = "D-001-2024",
    Title = "Ausschreibungsunterlagen",
    Type = DocumentType.Form,
    Direction = DocumentDirection.Outgoing,
    DocumentDate = DateTime.UtcNow,
    Author = "Max Mustermann",
    Subject = "Ausschreibung IT-Systeme - Unterlagen",
    RequiresSignature = true
};

document = await adminService.CreateDocumentAsync(document);
// Erzeugt Timeline-Event: DocumentCreated

// Unterschreiben
var signature = new Signature
{
    SignerId = "user-001",
    SignerName = "Dr. Erika Musterfrau",
    SignerRole = "Abteilungsleiterin",
    SignedAt = DateTime.UtcNow,
    SignatureType = "qualified",
    CertificateFingerprint = "SHA256:abc123..."
};

await adminService.SignDocumentAsync(document.Id, signature);
// Erzeugt Timeline-Event: DocumentSigned
```

### 4. Timeline abfragen

```csharp
// Alle Events für eine Akte
var fileEvents = await timelineService.GetEventsByFileAsync(
    file.Id,
    startDate: DateTime.UtcNow.AddMonths(-1),
    endDate: DateTime.UtcNow
);

foreach (var evt in fileEvents)
{
    Console.WriteLine($"{evt.Timestamp:yyyy-MM-dd HH:mm} - {evt.EventType}: {evt.Description}");
    Console.WriteLine($"  Akteur: {evt.Actor}");
    if (evt.ChangedFields.Any())
    {
        Console.WriteLine($"  Geändert: {string.Join(", ", evt.ChangedFields.Keys)}");
    }
}

// Output:
// 2024-12-07 14:30 - FileCreated: Akte IV C 5 - 123/2024 angelegt: Beschaffung neuer IT-Systeme
//   Akteur: Max Mustermann
// 2024-12-07 14:35 - ProcessCreated: Vorgang V-001-2024 angelegt: Ausschreibung IT-Systeme
//   Akteur: Max Mustermann
// 2024-12-07 14:40 - DocumentCreated: Dokument erstellt: Ausschreibungsunterlagen
//   Akteur: Max Mustermann
// 2024-12-07 15:00 - DocumentSigned: Dokument unterschrieben von Dr. Erika Musterfrau
//   Akteur: user-001
```

### 5. Behördenübergreifende Timeline

```csharp
// Alle Events einer Behörde
var authorityEvents = await timelineService.GetEventsByAuthorityAsync(
    "bmi-001",
    startDate: DateTime.UtcNow.AddDays(-7)
);

// Events nach Typ filtern
var signatureEvents = await timelineService.GetEventsByTypeAsync(
    ProcessEventType.DocumentSigned,
    startDate: DateTime.UtcNow.AddDays(-30)
);

// Events eines bestimmten Akteurs
var userEvents = await timelineService.GetEventsByActorAsync(
    "Max Mustermann",
    startDate: DateTime.UtcNow.AddMonths(-1)
);
```

## ThemisDB-Integration

### Collections in ThemisDB

Die folgenden Collections werden in ThemisDB erstellt:

```
- authorities              # Behörden
- filings                  # Ablagen
- administrative_files     # Akten
- subfiles                 # Unterakten
- administrative_processes # Vorgänge
- administrative_documents # Dokumente
- file_attachments         # Dateien
- process_timeline_events  # Timeline-Events
```

### Indizierung

**Primary Keys (URN):**
Alle Entitäten verwenden ihre URN als Primary Key für eindeutige Identifizierung.

**Secondary Indexes:**
```
authorities:
  - name, shortName, type, officialCode

filings:
  - authorityId, department

administrative_files:
  - fileNumber, filingId, status, category, responsibleOfficer
  - openedAt, closedAt, archiveDate

administrative_processes:
  - fileId, processNumber, status, type
  - assignedTo, dueDate

administrative_documents:
  - processId, documentNumber, type, direction
  - documentDate, status

process_timeline_events:
  - authorityId, filingId, fileId, processId, documentId
  - timestamp, eventType, actor
```

## Compliance & Rechtliche Anforderungen

### Aufbewahrungsfristen

Gemäß deutscher Verwaltungsvorschriften:

- **Standardakte**: 10 Jahre (konfigurierbar pro Akte)
- **Personalakten**: 10 Jahre nach Ausscheiden
- **Finanzakten**: 10 Jahre
- **Bauakten**: 30 Jahre
- **Daueraufbewahrung**: Für besonders wichtige Vorgänge

### Revisionssicherheit

- ✅ Unveränderliche Timeline (Append-Only)
- ✅ SHA256-Hashes für alle Dateien
- ✅ Digitale Signaturen (eIDAS-konform)
- ✅ Vollständiger Audit-Trail
- ✅ Nachvollziehbarkeit aller Änderungen

### Datenschutz (DSGVO)

- ✅ Zugriffskontrolle über SecurityClassification
- ✅ Löschfristen über RetentionPeriod
- ✅ Berechtigungskonzept
- ✅ Audit-Logging aller Zugriffe

### Archivierung

Nach Ablauf der Aufbewahrungsfrist:

1. Status → `Archived`
2. ArchiveDate setzen
3. Timeline-Event: `FileArchived`
4. Optional: Export für Langzeitarchivierung
5. Optional: Löschung nach gesetzlicher Frist

## Best Practices

### Aktenzeichen-Schema

Empfohlenes Schema nach deutschem Verwaltungsrecht:

```
[Abteilung] [Referat] [Sachgebiet] - [Laufnummer]/[Jahr]

Beispiele:
IV C 5 - 123/2024        (BMI, Abteilung IV, Referat C, Sachgebiet 5)
I A 2 - 456/2024-01      (mit Unterakte 01)
```

### Workflow-Schritte

Definieren Sie klare Workflow-Schritte für jeden Vorgangstyp:

```csharp
var procurementSteps = new List<ProcessStep>
{
    new ProcessStep { StepNumber = 1, Name = "Bedarfsermittlung" },
    new ProcessStep { StepNumber = 2, Name = "Haushaltsfreigabe" },
    new ProcessStep { StepNumber = 3, Name = "Ausschreibung" },
    new ProcessStep { StepNumber = 4, Name = "Angebotsbewertung" },
    new ProcessStep { StepNumber = 5, Name = "Zuschlagsentscheidung" },
    new ProcessStep { StepNumber = 6, Name = "Vertragsabschluss" }
};
```

### Timeline-Nutzung

Nutzen Sie die Timeline für:

- ✅ Compliance-Nachweise
- ✅ Prozess-Monitoring
- ✅ Performance-Analysen
- ✅ Audit-Berichte
- ✅ Eskalations-Management

---

**Erstellt**: 2024-12-07  
**Version**: 1.0  
**Status**: Produktionsreif  
**Compliance**: Deutsches Verwaltungsrecht, DSGVO, eIDAS
