/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationQueries.cs                           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     141                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bbce87a5a  2025-12-10  Begin Phase 2 Sprint 7-8: Add ML.NET document classificat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Domain.Classification;

namespace Themis.DocumentManager.Application.Classification.Queries;

/// <summary>
/// Query zum Abrufen einer Klassifizierungs-Vorhersage.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public record GetClassificationPredictionQuery(
    string DocumentId
) : IRequest<Result<DocumentClassification?>>;

/// <summary>
/// Query zum Abrufen aller Trainingsdaten.
/// </summary>
public record GetTrainingDataQuery(
    DataUsage? Usage = null,
    string? Label = null,
    bool VerifiedOnly = false,
    int PageNumber = 1,
    int PageSize = 100
) : IRequest<Result<TrainingDataCollection>>;

/// <summary>
/// Query zum Abrufen aller Modelle.
/// </summary>
public record GetClassificationModelsQuery(
    bool ActiveOnly = false
) : IRequest<Result<List<ClassificationModel>>>;

/// <summary>
/// Query zum Abrufen eines spezifischen Modells.
/// </summary>
public record GetClassificationModelQuery(
    string ModelId
) : IRequest<Result<ClassificationModel?>>;

/// <summary>
/// Query zum Abrufen der Model-Performance Metriken.
/// </summary>
public record GetModelPerformanceQuery(
    string ModelId
) : IRequest<Result<ModelPerformance>>;

/// <summary>
/// Query zum Abrufen extrahierter Metadaten.
/// </summary>
public record GetExtractedMetadataQuery(
    string DocumentId
) : IRequest<Result<ExtractedMetadata?>>;

/// <summary>
/// Query zum Abrufen der Klassifizierungs-Historie eines Dokuments.
/// </summary>
public record GetClassificationHistoryQuery(
    string DocumentId
) : IRequest<Result<List<DocumentClassification>>>;

/// <summary>
/// Query zum Abrufen von Klassifizierungs-Statistiken.
/// </summary>
public record GetClassificationStatisticsQuery(
    DateTime? StartDate = null,
    DateTime? EndDate = null
) : IRequest<Result<ClassificationStatistics>>;

/// <summary>
/// Sammlung von Trainingsdaten mit Paginierung.
/// </summary>
public class TrainingDataCollection
{
    public List<TrainingData> Data { get; set; } = new();
    public int TotalCount { get; set; }
    public int PageNumber { get; set; }
    public int PageSize { get; set; }
    public bool HasNextPage => PageNumber * PageSize < TotalCount;
}

/// <summary>
/// Performance-Metriken eines Modells.
/// </summary>
public class ModelPerformance
{
    public string ModelId { get; set; } = string.Empty;
    public string ModelName { get; set; } = string.Empty;
    public float Accuracy { get; set; }
    public float Precision { get; set; }
    public float Recall { get; set; }
    public float F1Score { get; set; }
    public Dictionary<string, CategoryPerformance> PerCategoryMetrics { get; set; } = new();
    public List<string> TopMisclassifications { get; set; } = new();
}

/// <summary>
/// Performance-Metriken pro Kategorie.
/// </summary>
public class CategoryPerformance
{
    public string Category { get; set; } = string.Empty;
    public float Precision { get; set; }
    public float Recall { get; set; }
    public float F1Score { get; set; }
    public int TruePositives { get; set; }
    public int FalsePositives { get; set; }
    public int FalseNegatives { get; set; }
}

/// <summary>
/// Klassifizierungs-Statistiken.
/// </summary>
public class ClassificationStatistics
{
    public int TotalClassifications { get; set; }
    public int ConfirmedClassifications { get; set; }
    public int CorrectClassifications { get; set; }
    public float OverallAccuracy { get; set; }
    public Dictionary<string, int> ClassificationsByCategory { get; set; } = new();
    public Dictionary<string, float> ConfidenceDistribution { get; set; } = new();
    public List<string> MostCommonCategories { get; set; } = new();
}
