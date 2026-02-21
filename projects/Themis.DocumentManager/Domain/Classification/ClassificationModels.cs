/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationModels.cs                            ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     341                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Domain.Classification;

/// <summary>
/// Repräsentiert eine Dokumenten-Klassifizierung durch ML.NET.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class DocumentClassification
{
    /// <summary>
    /// ID der Klassifizierung
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// ID des klassifizierten Dokuments
    /// </summary>
    public string DocumentId { get; set; } = string.Empty;

    /// <summary>
    /// Vorhergesagte Kategorie
    /// </summary>
    public string PredictedCategory { get; set; } = string.Empty;

    /// <summary>
    /// Confidence Score (0.0 - 1.0)
    /// </summary>
    public float ConfidenceScore { get; set; }

    /// <summary>
    /// Alternative Kategorien mit Scores
    /// </summary>
    public List<CategoryScore> AlternativeCategories { get; set; } = new();

    /// <summary>
    /// Zeitpunkt der Klassifizierung
    /// </summary>
    public DateTime ClassifiedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Verwendetes Modell (Version/ID)
    /// </summary>
    public string ModelVersion { get; set; } = string.Empty;

    /// <summary>
    /// Wurde die Klassifizierung von einem Benutzer bestätigt?
    /// </summary>
    public bool IsConfirmed { get; set; }

    /// <summary>
    /// Tatsächliche Kategorie (für Training-Feedback)
    /// </summary>
    public string? ActualCategory { get; set; }

    /// <summary>
    /// Feedback vom Benutzer
    /// </summary>
    public string? UserFeedback { get; set; }

    /// <summary>
    /// Prüft ob die Klassifizierung korrekt war
    /// </summary>
    public bool IsCorrect() => 
        !string.IsNullOrEmpty(ActualCategory) && 
        PredictedCategory.Equals(ActualCategory, StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Prüft ob die Confidence über dem Schwellwert liegt
    /// </summary>
    public bool IsHighConfidence(float threshold = 0.7f) => ConfidenceScore >= threshold;
}

/// <summary>
/// Score einer Kategorie
/// </summary>
public class CategoryScore
{
    public string Category { get; set; } = string.Empty;
    public float Score { get; set; }
}

/// <summary>
/// Repräsentiert ein ML-Modell für Dokumenten-Klassifizierung.
/// </summary>
public class ClassificationModel
{
    /// <summary>
    /// ID des Modells
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// Name/Version des Modells
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Beschreibung
    /// </summary>
    public string Description { get; set; } = string.Empty;

    /// <summary>
    /// Pfad zur Modell-Datei (.zip)
    /// </summary>
    public string ModelPath { get; set; } = string.Empty;

    /// <summary>
    /// Zeitpunkt des Trainings
    /// </summary>
    public DateTime TrainedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Trainingsdauer
    /// </summary>
    public TimeSpan TrainingDuration { get; set; }

    /// <summary>
    /// Accuracy auf Test-Set
    /// </summary>
    public float Accuracy { get; set; }

    /// <summary>
    /// Precision (Präzision)
    /// </summary>
    public float Precision { get; set; }

    /// <summary>
    /// Recall (Trefferquote)
    /// </summary>
    public float Recall { get; set; }

    /// <summary>
    /// F1-Score
    /// </summary>
    public float F1Score { get; set; }

    /// <summary>
    /// Anzahl Trainings-Beispiele
    /// </summary>
    public int TrainingExamples { get; set; }

    /// <summary>
    /// Anzahl Test-Beispiele
    /// </summary>
    public int TestExamples { get; set; }

    /// <summary>
    /// Unterstützte Kategorien
    /// </summary>
    public List<string> Categories { get; set; } = new();

    /// <summary>
    /// Ist dieses Modell aktiv/produktiv?
    /// </summary>
    public bool IsActive { get; set; }

    /// <summary>
    /// Confusion Matrix (für Analyse)
    /// </summary>
    public Dictionary<string, Dictionary<string, int>>? ConfusionMatrix { get; set; }

    /// <summary>
    /// Prüft ob das Modell die Mindest-Accuracy erreicht
    /// </summary>
    public bool MeetsQualityThreshold(float threshold = 0.9f) => Accuracy >= threshold;
}

/// <summary>
/// Trainingsdaten für ML-Modell.
/// </summary>
public class TrainingData
{
    /// <summary>
    /// ID des Trainingsbeispiels
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// Dokumenten-ID (optional)
    /// </summary>
    public string? DocumentId { get; set; }

    /// <summary>
    /// Text-Inhalt oder Features
    /// </summary>
    public string Content { get; set; } = string.Empty;

    /// <summary>
    /// Label/Kategorie
    /// </summary>
    public string Label { get; set; } = string.Empty;

    /// <summary>
    /// Metadaten (optional)
    /// </summary>
    public Dictionary<string, string> Metadata { get; set; } = new();

    /// <summary>
    /// Wurde manuell verifiziert?
    /// </summary>
    public bool IsVerified { get; set; }

    /// <summary>
    /// Zeitpunkt der Erstellung
    /// </summary>
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Quelle des Trainingsbeispiels
    /// </summary>
    public string Source { get; set; } = "Manual";

    /// <summary>
    /// Verwendung (Training, Test, Validation)
    /// </summary>
    public DataUsage Usage { get; set; } = DataUsage.Training;
}

/// <summary>
/// Verwendungszweck von Trainingsdaten
/// </summary>
public enum DataUsage
{
    Training,
    Test,
    Validation
}

/// <summary>
/// Extrahierte Metadaten aus einem Dokument.
/// </summary>
public class ExtractedMetadata
{
    /// <summary>
    /// ID der Extraktion
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// Dokumenten-ID
    /// </summary>
    public string DocumentId { get; set; } = string.Empty;

    /// <summary>
    /// Extrahierte Entities (Named Entities)
    /// </summary>
    public List<NamedEntity> Entities { get; set; } = new();

    /// <summary>
    /// Extrahierte Datumsangaben
    /// </summary>
    public List<DateTime> Dates { get; set; } = new();

    /// <summary>
    /// Automatisch generierte Tags
    /// </summary>
    public List<string> Tags { get; set; } = new();

    /// <summary>
    /// Schlüsselwörter
    /// </summary>
    public List<KeyPhrase> KeyPhrases { get; set; } = new();

    /// <summary>
    /// Zeitpunkt der Extraktion
    /// </summary>
    public DateTime ExtractedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Verwendetes Modell
    /// </summary>
    public string ModelVersion { get; set; } = string.Empty;

    /// <summary>
    /// Zusätzliche Metadaten
    /// </summary>
    public Dictionary<string, string> AdditionalMetadata { get; set; } = new();
}

/// <summary>
/// Named Entity (z.B. Person, Organisation, Ort)
/// </summary>
public class NamedEntity
{
    public string Text { get; set; } = string.Empty;
    public EntityType Type { get; set; }
    public float Confidence { get; set; }
    public int StartPosition { get; set; }
    public int EndPosition { get; set; }
}

/// <summary>
/// Typ einer Named Entity
/// </summary>
public enum EntityType
{
    Person,
    Organization,
    Location,
    Date,
    Money,
    Percentage,
    Custom
}

/// <summary>
/// Schlüsselphrase mit Relevanz-Score
/// </summary>
public class KeyPhrase
{
    public string Phrase { get; set; } = string.Empty;
    public float Relevance { get; set; }
}
