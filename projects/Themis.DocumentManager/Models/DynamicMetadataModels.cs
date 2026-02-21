/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DynamicMetadataModels.cs                           ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     232                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Represents a binding between a document and ThemisDB metadata
/// </summary>
public class DocumentMetadataBinding
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string DocumentId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public List<MetadataField> BoundFields { get; set; } = new();
    public BindingStatus Status { get; set; } = BindingStatus.Active;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    public DateTime? FinalizedAt { get; set; }
    public string? FinalizedBy { get; set; }
    public string? FinalizationHash { get; set; }
    public string? SignatureId { get; set; }
    public int Version { get; set; } = 1;
}

/// <summary>
/// Represents a metadata field binding
/// </summary>
public class MetadataField
{
    public string FieldName { get; set; } = string.Empty;
    public string ThemisPath { get; set; } = string.Empty;
    public string ContentControlId { get; set; } = string.Empty;
    public FieldType Type { get; set; } = FieldType.Text;
    public bool IsRequired { get; set; }
    public string? DefaultValue { get; set; }
    public string? CurrentValue { get; set; }
    public DateTime? LastUpdated { get; set; }
    public List<string>? Options { get; set; }  // For Dropdown fields
}

/// <summary>
/// Result of document finalization
/// </summary>
public class DocumentFinalizationResult
{
    public string FinalizedDocumentId { get; set; } = string.Empty;
    public string OriginalDocumentId { get; set; } = string.Empty;
    public string SHA256Hash { get; set; } = string.Empty;
    public DateTime FinalizedAt { get; set; } = DateTime.UtcNow;
    public string FinalizedBy { get; set; } = string.Empty;
    public int RemovedBindingsCount { get; set; }
    public List<string> ConvertedFields { get; set; } = new();
    public bool DigitallySigned { get; set; }
    public string? SignatureId { get; set; }
    public TimeSpan ProcessingTime { get; set; }
}

/// <summary>
/// Proof of document finalization for audit trail
/// </summary>
public class FinalizationProof
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string DocumentId { get; set; } = string.Empty;
    public string SHA256Hash { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public string FinalizedBy { get; set; } = string.Empty;
    public string? SignatureId { get; set; }
    public string? SignatureCertificate { get; set; }
    public string? TimeStampToken { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Result of manipulation detection
/// </summary>
public class ManipulationDetectionResult
{
    public bool IsManipulated { get; set; }
    public string? ExpectedHash { get; set; }
    public string? ActualHash { get; set; }
    public DateTime? FinalizedAt { get; set; }
    public DateTime LastCheckedAt { get; set; } = DateTime.UtcNow;
    public string? Details { get; set; }
}

/// <summary>
/// Validation result for metadata
/// </summary>
public class MetadataValidationResult
{
    public bool IsValid { get; set; }
    public List<MetadataValidationError> Errors { get; set; } = new();
    public List<MetadataValidationWarning> Warnings { get; set; } = new();
}

public class MetadataValidationError
{
    public string FieldName { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
    public ErrorSeverity Severity { get; set; } = ErrorSeverity.Error;
}

public class MetadataValidationWarning
{
    public string FieldName { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
}

/// <summary>
/// Enumerations
/// </summary>
public enum BindingStatus
{
    Active,      // Dynamically connected
    Finalized,   // Disconnected & tamper-proof
    Archived     // Archived
}

public enum FieldType
{
    Text,
    Number,
    Date,
    DateTime,
    Boolean,
    Dropdown,
    RichText
}

public enum ErrorSeverity
{
    Warning,
    Error,
    Critical
}

/// <summary>
/// Standard metadata fields for German administration
/// </summary>
public static class StandardMetadataFields
{
    public static List<MetadataField> GetGermanAdministrationFields()
    {
        return new List<MetadataField>
        {
            new() 
            {
                FieldName = "Aktenzeichen",
                ThemisPath = "process.fileReference",
                Type = FieldType.Text,
                IsRequired = true
            },
            new()
            {
                FieldName = "Betreff",
                ThemisPath = "process.subject",
                Type = FieldType.Text,
                IsRequired = true
            },
            new()
            {
                FieldName = "Datum",
                ThemisPath = "document.createdAt",
                Type = FieldType.Date,
                IsRequired = true,
                DefaultValue = "{{TODAY}}"
            },
            new()
            {
                FieldName = "Sachbearbeiter",
                ThemisPath = "process.assignedTo",
                Type = FieldType.Text,
                IsRequired = false
            },
            new()
            {
                FieldName = "Behörde",
                ThemisPath = "authority.name",
                Type = FieldType.Text,
                IsRequired = true
            },
            new()
            {
                FieldName = "Abteilung",
                ThemisPath = "process.department",
                Type = FieldType.Text,
                IsRequired = false
            },
            new()
            {
                FieldName = "Geschäftszeichen",
                ThemisPath = "file.fileNumber",
                Type = FieldType.Text,
                IsRequired = false
            },
            new()
            {
                FieldName = "Wiedervorlage",
                ThemisPath = "process.nextDeadline",
                Type = FieldType.Date,
                IsRequired = false
            }
        };
    }
}
