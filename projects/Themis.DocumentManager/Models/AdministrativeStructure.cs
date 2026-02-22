/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AdministrativeStructure.cs                         ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     425                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Hierarchische Aktenstruktur nach deutschem Verwaltungsrecht
/// Mapping auf ThemisDB URN-System
/// </summary>

/// <summary>
/// Behörde (Authority) - Oberste Ebene
/// URN: urn:themis:authority:{id}
/// </summary>
public class Authority
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{Id}";
    public string Name { get; set; } = string.Empty;
    public string ShortName { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty; // z.B. "Bundesbehörde", "Landesbehörde", "Kommunalbehörde"
    public string OfficialCode { get; set; } = string.Empty; // Amtlicher Behördenschlüssel
    public DateTime CreatedAt { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Ablage (Filing System) - Organisationseinheit innerhalb Behörde
/// URN: urn:themis:authority:{authorityId}:filing:{id}
/// </summary>
public class Filing
{
    public string Id { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{AuthorityId}:filing:{Id}";
    public string Name { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty; // Abteilung/Referat
    public string ResponsibleOfficer { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Akte (File) - Hauptakte
/// URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{id}
/// </summary>
public class AdministrativeFile
{
    public string Id { get; set; } = string.Empty;
    public string FilingId { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{Id}";
    
    public string FileNumber { get; set; } = string.Empty; // Aktenzeichen (z.B. "IV C 5 - 123/2024")
    public string Subject { get; set; } = string.Empty; // Betreff
    public string Category { get; set; } = string.Empty; // Sachgebiet
    public FileStatus Status { get; set; } = FileStatus.Active;
    public DateTime OpenedAt { get; set; }
    public DateTime? ClosedAt { get; set; }
    public DateTime? ArchiveDate { get; set; }
    public int RetentionPeriodYears { get; set; } = 10;
    
    // Verfahrensbeteiligte
    public List<string> Participants { get; set; } = new(); // Beteiligte Personen/Organisationen
    public string ResponsibleOfficer { get; set; } = string.Empty;
    public string FileManager { get; set; } = string.Empty; // Aktenführer
    
    // Klassifizierung
    public SecurityClassification SecurityLevel { get; set; } = SecurityClassification.Public;
    public string AccessRestriction { get; set; } = string.Empty;
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Unterakte (Sub-File) - Teil einer Hauptakte
/// URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:subfile:{id}
/// </summary>
public class SubFile
{
    public string Id { get; set; } = string.Empty;
    public string ParentFileId { get; set; } = string.Empty;
    public string FilingId { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{ParentFileId}:subfile:{Id}";
    
    public string SubFileNumber { get; set; } = string.Empty; // z.B. "IV C 5 - 123/2024-01"
    public string Subject { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Vorgang (Process/Transaction) - Einzelner Bearbeitungsvorgang
/// URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{id}
/// </summary>
public class AdministrativeProcess
{
    public string Id { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string SubFileId { get; set; } = string.Empty; // Optional
    public string FilingId { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => string.IsNullOrEmpty(SubFileId)
        ? $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{FileId}:process:{Id}"
        : $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{FileId}:subfile:{SubFileId}:process:{Id}";
    
    public string ProcessNumber { get; set; } = string.Empty;
    public string Subject { get; set; } = string.Empty;
    public ProcessType Type { get; set; }
    public ProcessStatus Status { get; set; } = ProcessStatus.InProgress;
    
    // Zeitstempel
    public DateTime CreatedAt { get; set; }
    public DateTime? StartDate { get; set; }
    public DateTime? StartedAt { get; set; }
    public DateTime? CompletedAt { get; set; }
    public DateTime? TargetCompletionDate { get; set; }
    public DateTime? DueDate { get; set; }
    
    // Verantwortlichkeiten
    public string InitiatedBy { get; set; } = string.Empty;
    public string AssignedTo { get; set; } = string.Empty;
    public string CurrentProcessor { get; set; } = string.Empty;
    
    // Workflow
    public string WorkflowState { get; set; } = string.Empty;
    public List<ProcessStep> Steps { get; set; } = new();
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Dokument (Document) - Einzelnes Dokument in einem Vorgang
/// URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{processId}:document:{id}
/// </summary>
public class AdministrativeDocument
{
    public string Id { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string FilingId { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{FileId}:process:{ProcessId}:document:{Id}";
    
    public string DocumentNumber { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public DocumentType Type { get; set; }
    public DocumentDirection Direction { get; set; }
    
    // Zeitstempel
    public DateTime CreatedAt { get; set; }
    public DateTime DocumentDate { get; set; } // Dokumentendatum
    public DateTime ReceivedAt { get; set; } // Eingangsdatum
    
    // Herkunft/Ziel
    public string Sender { get; set; } = string.Empty;
    public string Recipient { get; set; } = string.Empty;
    public string Author { get; set; } = string.Empty;
    
    // Inhalt
    public string Subject { get; set; } = string.Empty;
    public string Summary { get; set; } = string.Empty;
    public string MimeType { get; set; } = string.Empty;
    
    // Verknüpfungen
    public List<string> AttachedFileIds { get; set; } = new(); // Verweis auf physische Dateien
    public List<string> ReferencedDocuments { get; set; } = new(); // Bezugsdokumente
    
    // Bearbeitung
    public DocumentLifecycleStatus Status { get; set; } = DocumentLifecycleStatus.Draft;
    public bool RequiresSignature { get; set; }
    public List<Signature> Signatures { get; set; } = new();
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Datei (File/Attachment) - Physische Datei
/// URN: urn:themis:authority:{authorityId}:filing:{filingId}:file:{fileId}:process:{processId}:document:{documentId}:attachment:{id}
/// </summary>
public class FileAttachment
{
    public string Id { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string FilingId { get; set; } = string.Empty;
    public string AuthorityId { get; set; } = string.Empty;
    public string Urn => $"urn:themis:authority:{AuthorityId}:filing:{FilingId}:file:{FileId}:process:{ProcessId}:document:{DocumentId}:attachment:{Id}";
    
    public string Filename { get; set; } = string.Empty;
    public string MimeType { get; set; } = string.Empty;
    public long SizeBytes { get; set; }
    public string BlobPath { get; set; } = string.Empty;
    public string FileHash { get; set; } = string.Empty; // SHA256
    
    public DateTime UploadedAt { get; set; }
    public string UploadedBy { get; set; } = string.Empty;
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Prozess-Timeline Event
/// Zentrales Timeline-Event-System für alle Vorgänge
/// URN: urn:themis:timeline:event:{id}
/// </summary>
public class ProcessTimelineEvent
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:timeline:event:{Id}";
    
    // Verknüpfungen zur Hierarchie
    public string AuthorityId { get; set; } = string.Empty;
    public string FilingId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty; // Optional
    
    // Event-Details
    public DateTime Timestamp { get; set; }
    public ProcessEventType EventType { get; set; }
    public ProcessEventType Type { get; set; }  // Alias for EventType
    public string EventCategory { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    // Akteur
    public string Actor { get; set; } = string.Empty; // Wer hat die Aktion durchgeführt
    public string ActorRole { get; set; } = string.Empty;
    
    // Änderungen
    public Dictionary<string, object> Changes { get; set; } = new();
    public Dictionary<string, object> ChangedFields { get; set; } = new();
    public Dictionary<string, object> PreviousValues { get; set; } = new();
    public Dictionary<string, object> NewValues { get; set; } = new();
    
    // Kontext
    public string Comment { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Prozessschritt (Process Step) - Einzelner Schritt im Workflow
/// </summary>
public class ProcessStep
{
    public string Id { get; set; } = string.Empty;
    public int StepNumber { get; set; }
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public ProcessStepStatus Status { get; set; } = ProcessStepStatus.Pending;
    public DateTime? StartedAt { get; set; }
    public DateTime? CompletedAt { get; set; }
    public string AssignedTo { get; set; } = string.Empty;
    public string CompletedBy { get; set; } = string.Empty;
    public string Comment { get; set; } = string.Empty;
}

/// <summary>
/// Unterschrift (Signature)
/// </summary>
public class Signature
{
    public string SignerId { get; set; } = string.Empty;
    public string SignerName { get; set; } = string.Empty;
    public string SignerRole { get; set; } = string.Empty;
    public DateTime SignedAt { get; set; }
    public string SignatureType { get; set; } = string.Empty; // "qualified", "advanced", "simple"
    public string CertificateFingerprint { get; set; } = string.Empty;
}

#region Enumerations

public enum FileStatus
{
    Active,      // In Bearbeitung
    Suspended,   // Ruhend
    Closed,      // Geschlossen
    Archived     // Archiviert
}

public enum SecurityClassification
{
    Public,              // Öffentlich
    Internal,            // Intern
    Confidential,        // Vertraulich
    Secret,              // Geheim
    TopSecret            // Streng geheim
}

public enum ProcessType
{
    Administrative,      // Verwaltungsvorgang
    Legal,              // Rechtsvorgang
    Financial,          // Finanzvorgang
    Personnel,          // Personalvorgang
    Procurement,        // Beschaffungsvorgang
    Construction,       // Bauvorgang
    Licensing,          // Genehmigungsvorgang
    Complaint,          // Beschwerdeverfahren
    Information,        // Informationsvorgang
    Other               // Sonstiges
}

public enum ProcessStatus
{
    Draft,              // Entwurf
    InProgress,         // In Bearbeitung
    Suspended,          // Ausgesetzt
    Completed,          // Abgeschlossen
    Cancelled,          // Abgebrochen
    Archived            // Archiviert
}

public enum DocumentType
{
    Letter,             // Brief
    Email,              // E-Mail
    Memo,               // Vermerk
    Report,             // Bericht
    Decision,           // Bescheid
    Application,        // Antrag
    Contract,           // Vertrag
    Invoice,            // Rechnung
    Protocol,           // Protokoll
    Certificate,        // Bescheinigung
    Form,               // Formular
    Other               // Sonstiges
}

public enum DocumentDirection
{
    Incoming,           // Eingang
    Outgoing,           // Ausgang
    Internal            // Intern
}

public enum DocumentLifecycleStatus
{
    Draft,              // Entwurf
    InReview,           // In Prüfung
    Approved,           // Genehmigt
    Signed,             // Unterschrieben
    Sent,               // Versandt
    Archived            // Archiviert
}

public enum ProcessStepStatus
{
    Pending,            // Ausstehend
    InProgress,         // In Bearbeitung
    Completed,          // Abgeschlossen
    Skipped             // Übersprungen
}

public enum ProcessEventType
{
    // Akte
    FileCreated,
    FileOpened,
    FileClosed,
    FileArchived,
    FileStatusChanged,
    
    // Vorgang
    ProcessCreated,
    ProcessStarted,
    ProcessAssigned,
    ProcessStatusChanged,
    ProcessCompleted,
    ProcessCancelled,
    
    // Dokument
    DocumentCreated,
    DocumentReceived,
    DocumentSent,
    DocumentSigned,
    DocumentApproved,
    DocumentRejected,
    Approved,
    Rejected,
    FileAccessed,
    FileTransferred,
    
    // Workflow
    StepCompleted,
    StepSkipped,
    WorkflowStateChanged,
    
    // Sonstiges
    CommentAdded,
    DeadlineChanged,
    ParticipantAdded,
    ParticipantRemoved,
    MetadataChanged
}

#endregion
