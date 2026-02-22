/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase3ComplianceModels.cs                          ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     469                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

// ============================================================================
// Phase 3: Compliance & Integration Models
// ============================================================================

#region 4-Augen-Prinzip (Four-Eyes Principle)

/// <summary>
/// 4-Augen-Prinzip Regel - Definiert wann 4-Augen-Prinzip angewendet wird
/// </summary>
public class FourEyesPrincipleRule
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    // Trigger-Bedingungen
    public FourEyesTriggerType TriggerType { get; set; }
    public decimal? AmountThreshold { get; set; } // Schwellenwert für Beträge
    public SecurityClassification? SecurityLevel { get; set; }
    public string? ProcessType { get; set; }
    public string? DocumentType { get; set; }
    
    // Anforderungen
    public int RequiredApprovers { get; set; } = 2;
    public List<string> RequiredRoles { get; set; } = new();
    public bool RequiresDifferentDepartments { get; set; } = false;
    public TimeSpan? MaxApprovalTime { get; set; }
    
    // Status
    public bool IsActive { get; set; } = true;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
}

public enum FourEyesTriggerType
{
    AmountThreshold,      // Betragsschwelle überschritten
    SecurityClassification, // Sicherheitseinstufung
    ProcessType,          // Bestimmte Vorgangsarten
    DocumentType,         // Bestimmte Dokumenttypen
    CombinedCriteria      // Kombination mehrerer Kriterien
}

/// <summary>
/// 4-Augen-Prüfung - Einzelne Prüfungsinstanz
/// </summary>
public class FourEyesApproval
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:four-eyes-approval:{Id}";
    
    public string RuleId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string? DocumentId { get; set; }
    
    // Genehmiger
    public List<FourEyesApprover> Approvers { get; set; } = new();
    
    // Status
    public FourEyesApprovalStatus Status { get; set; } = FourEyesApprovalStatus.Pending;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? CompletedAt { get; set; }
    public DateTime? ExpiresAt { get; set; }
    
    // Grund & Kontext
    public string Reason { get; set; } = string.Empty;
    public Dictionary<string, object> Context { get; set; } = new();
}

public class FourEyesApprover
{
    public string UserId { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    
    public FourEyesApproverStatus Status { get; set; } = FourEyesApproverStatus.Pending;
    public DateTime? ApprovedAt { get; set; }
    public string? Comment { get; set; }
    public string? RejectReason { get; set; }
}

public enum FourEyesApprovalStatus
{
    Pending,      // Warte auf Genehmigungen
    Approved,     // Alle haben genehmigt
    Rejected,     // Mindestens einer hat abgelehnt
    Expired       // Zeitlimit überschritten
}

public enum FourEyesApproverStatus
{
    Pending,
    Approved,
    Rejected
}

#endregion

#region Akteneinsichts-Protokoll (File Access Logging)

/// <summary>
/// Akteneinsicht - Protokollierung von Akteneinsichten
/// </summary>
public class FileAccessLog
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:file-access-log:{Id}";
    
    // Zugriffsinformationen
    public string FileId { get; set; } = string.Empty;
    public string FileReference { get; set; } = string.Empty;
    public DateTime AccessTime { get; set; } = DateTime.UtcNow;
    public FileAccessType AccessType { get; set; }
    
    // Nutzer
    public string UserId { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
    
    // Begründung (DSGVO-relevant)
    public string Purpose { get; set; } = string.Empty;
    public string LegalBasis { get; set; } = string.Empty; // Rechtsgrundlage
    public bool IsAuthorized { get; set; } = true;
    
    // Zugriff im Detail
    public List<string> AccessedDocuments { get; set; } = new();
    public List<string> ViewedFields { get; set; } = new();
    public FileAccessDuration Duration { get; set; } = new();
    
    // IP & System
    public string IpAddress { get; set; } = string.Empty;
    public string UserAgent { get; set; } = string.Empty;
    public string WorkstationName { get; set; } = string.Empty;
}

public enum FileAccessType
{
    Read,          // Lesezugriff
    Download,      // Download
    Print,         // Druck
    Export,        // Export
    Modify,        // Bearbeitung
    Delete,        // Löschung
    Share          // Weitergabe
}

public class FileAccessDuration
{
    public DateTime StartTime { get; set; }
    public DateTime? EndTime { get; set; }
    public TimeSpan? Duration => EndTime.HasValue ? EndTime.Value - StartTime : null;
}

/// <summary>
/// Akteneinsichts-Anfrage - Formale Anfrage auf Akteneinsicht
/// </summary>
public class FileAccessRequest
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string FileId { get; set; } = string.Empty;
    public string RequestedBy { get; set; } = string.Empty;
    public DateTime RequestedAt { get; set; } = DateTime.UtcNow;
    
    public string Purpose { get; set; } = string.Empty;
    public string LegalBasis { get; set; } = string.Empty;
    
    public FileAccessRequestStatus Status { get; set; } = FileAccessRequestStatus.Pending;
    public string? ApprovedBy { get; set; }
    public DateTime? ApprovedAt { get; set; }
    public string? RejectionReason { get; set; }
}

public enum FileAccessRequestStatus
{
    Pending,
    Approved,
    Rejected,
    Expired
}

#endregion

#region Stellvertretungsregeln (Substitution Rules)

/// <summary>
/// Stellvertretungsregel - Automatische Vertretungsregelung
/// </summary>
public class SubstitutionRule
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:substitution-rule:{Id}";
    
    // Vertretener
    public string UserId { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    
    // Vertreter
    public string SubstituteUserId { get; set; } = string.Empty;
    public string SubstituteUserName { get; set; } = string.Empty;
    
    // Zeitraum
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public bool IsActive => DateTime.UtcNow >= StartDate && DateTime.UtcNow <= EndDate;
    
    // Umfang
    public SubstitutionScope Scope { get; set; } = SubstitutionScope.Full;
    public List<string> IncludedProcessTypes { get; set; } = new(); // Nur diese Vorgangsarten
    public List<string> ExcludedProcessTypes { get; set; } = new(); // Ausgenommen diese
    public List<string> IncludedRoles { get; set; } = new(); // Nur für diese Rollen
    
    // Benachrichtigungen
    public bool NotifyOriginalUser { get; set; } = true;
    public bool NotifySubstitute { get; set; } = true;
    
    // Status
    public SubstitutionRuleStatus Status { get; set; } = SubstitutionRuleStatus.Active;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    
    // Grund
    public string Reason { get; set; } = string.Empty; // z.B. "Urlaub", "Krankheit"
}

public enum SubstitutionScope
{
    Full,          // Vollständige Vertretung
    Limited,       // Eingeschränkt (nur bestimmte Typen)
    ReadOnly       // Nur Lesezugriff
}

public enum SubstitutionRuleStatus
{
    Active,
    Inactive,
    Expired
}

/// <summary>
/// Stellvertretungsaktion - Protokollierung von Vertretungshandlungen
/// </summary>
public class SubstitutionAction
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string SubstitutionRuleId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    
    public DateTime ActionTime { get; set; } = DateTime.UtcNow;
    public string Action { get; set; } = string.Empty;
    public string SubstituteUserId { get; set; } = string.Empty;
    public string OriginalUserId { get; set; } = string.Empty;
    
    public bool WasNotified { get; set; } = false;
}

#endregion

#region eGov-Schnittstellen (eGovernment Interfaces)

/// <summary>
/// eGov-Nachricht - Basis für elektronische Behördenkommunikation
/// </summary>
public class EGovMessage
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:egov-message:{Id}";
    
    public EGovProtocol Protocol { get; set; }
    public EGovMessageType MessageType { get; set; }
    public EGovDirection Direction { get; set; }
    
    // Sender/Empfänger
    public EGovParticipant Sender { get; set; } = new();
    public EGovParticipant Receiver { get; set; } = new();
    
    // Inhalt
    public string Subject { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public string? XmlContent { get; set; } // XÖV/XJustiz XML
    public List<EGovAttachment> Attachments { get; set; } = new();
    
    // Metadaten
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? SentAt { get; set; }
    public DateTime? ReceivedAt { get; set; }
    public EGovMessageStatus Status { get; set; } = EGovMessageStatus.Draft;
    
    // Prozess-Verlinkung
    public string? ProcessId { get; set; }
    public string? FileId { get; set; }
    
    // Sicherheit
    public bool IsEncrypted { get; set; } = true;
    public bool IsSigned { get; set; } = true;
    public string? SignatureCertificate { get; set; }
    
    // Transport-Details
    public string? TransportId { get; set; }
    public Dictionary<string, string> TransportHeaders { get; set; } = new();
}

public enum EGovProtocol
{
    OSCI,          // Online Services Computer Interface
    XTA,           // XML-Transport-Adapter
    SAFE,          // Secure Access to Federated E-Justice
    DE_Mail,       // DE-Mail
    Custom         // Benutzerdefiniert
}

public enum EGovMessageType
{
    Request,       // Anfrage
    Response,      // Antwort
    Notification,  // Benachrichtigung
    DataTransfer,  // Datenübermittlung
    StatusUpdate   // Statusmeldung
}

public enum EGovDirection
{
    Inbound,       // Eingehend
    Outbound       // Ausgehend
}

public class EGovParticipant
{
    public string Id { get; set; } = string.Empty; // Behördenkennzeichen
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty; // "Authority", "Citizen", "Company"
    public string? Email { get; set; }
    public string? CertificateId { get; set; }
}

public class EGovAttachment
{
    public string FileName { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long Size { get; set; }
    public string? DocumentId { get; set; }
    public byte[]? Data { get; set; }
    public bool IsEncrypted { get; set; } = true;
}

public enum EGovMessageStatus
{
    Draft,         // Entwurf
    Sending,       // Wird gesendet
    Sent,          // Gesendet
    Delivered,     // Zugestellt
    Failed,        // Fehlgeschlagen
    Received       // Empfangen
}

/// <summary>
/// XÖV-Standard - XML in der öffentlichen Verwaltung
/// </summary>
public class XOeVStandard
{
    public string StandardName { get; set; } = string.Empty; // z.B. "XJustiz", "XMeld", "XPersonenstand"
    public string Version { get; set; } = string.Empty;
    public string SchemaLocation { get; set; } = string.Empty;
    public string? Namespace { get; set; }
}

/// <summary>
/// eGov-Konfiguration
/// </summary>
public class EGovConfiguration
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public EGovProtocol Protocol { get; set; }
    
    // Endpunkte
    public string EndpointUrl { get; set; } = string.Empty;
    public string? AlternativeEndpointUrl { get; set; }
    
    // Authentifizierung
    public string CertificatePath { get; set; } = string.Empty;
    public string? CertificatePassword { get; set; }
    public string ClientId { get; set; } = string.Empty;
    
    // Einstellungen
    public bool EnableEncryption { get; set; } = true;
    public bool EnableSignature { get; set; } = true;
    public bool ValidateXml { get; set; } = true;
    public int TimeoutSeconds { get; set; } = 300;
    
    // Proxy (falls erforderlich)
    public string? ProxyUrl { get; set; }
    public string? ProxyUsername { get; set; }
    public string? ProxyPassword { get; set; }
}

#endregion

#region Übergabevermerke (Transfer Notes)

/// <summary>
/// Übergabevermerk - Dokumentation der Aktenübergabe
/// </summary>
public class TransferNote
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:transfer-note:{Id}";
    
    public string FileId { get; set; } = string.Empty;
    public string FileReference { get; set; } = string.Empty;
    
    // Übergabe
    public DateTime TransferDate { get; set; } = DateTime.UtcNow;
    public string TransferredFrom { get; set; } = string.Empty;
    public string TransferredFromDepartment { get; set; } = string.Empty;
    public string TransferredTo { get; set; } = string.Empty;
    public string TransferredToDepartment { get; set; } = string.Empty;
    
    // Grund
    public TransferReason Reason { get; set; }
    public string ReasonText { get; set; } = string.Empty;
    
    // Inhalt
    public List<string> DocumentIds { get; set; } = new();
    public int DocumentCount { get; set; }
    public string Notes { get; set; } = string.Empty;
    
    // Bestätigung
    public bool IsAcknowledged { get; set; } = false;
    public DateTime? AcknowledgedAt { get; set; }
    public string? AcknowledgedBy { get; set; }
    public string? AcknowledgementComment { get; set; }
}

public enum TransferReason
{
    ZuständigkeitswechselUnderCompetenceChange,          // Zuständigkeitswechsel
    PersonalChange,        // Personalwechsel
    Reorganization,        // Organisationsänderung
    Archiving,            // Archivierung
    Closure,              // Abschluss
    Other                 // Sonstiges
}

#endregion
