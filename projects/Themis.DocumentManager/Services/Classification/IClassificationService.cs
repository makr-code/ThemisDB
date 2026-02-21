/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IClassificationService.cs                          ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Domain.Classification;

namespace Themis.DocumentManager.Services.Classification;

/// <summary>
/// Interface für ML-basierte Dokumenten-Klassifizierung.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public interface IClassificationService
{
    /// <summary>
    /// Klassifiziert ein Dokument.
    /// </summary>
    Task<DocumentClassification> ClassifyDocumentAsync(string documentId, string content, CancellationToken cancellationToken = default);

    /// <summary>
    /// Extrahiert Metadaten aus einem Dokument.
    /// </summary>
    Task<ExtractedMetadata> ExtractMetadataAsync(string documentId, string content, CancellationToken cancellationToken = default);

    /// <summary>
    /// Trainiert ein neues Modell.
    /// </summary>
    Task<ClassificationModel> TrainModelAsync(string modelName, string description, List<string> trainingDataIds, CancellationToken cancellationToken = default);

    /// <summary>
    /// Fügt Trainingsdaten hinzu.
    /// </summary>
    Task<TrainingData> AddTrainingDataAsync(TrainingData data, CancellationToken cancellationToken = default);

    /// <summary>
    /// Lädt alle Trainingsdaten.
    /// </summary>
    Task<List<TrainingData>> GetTrainingDataAsync(DataUsage? usage = null, CancellationToken cancellationToken = default);

    /// <summary>
    /// Lädt alle Modelle.
    /// </summary>
    Task<List<ClassificationModel>> GetModelsAsync(bool activeOnly = false, CancellationToken cancellationToken = default);

    /// <summary>
    /// Aktiviert ein Modell als Produktiv-Modell.
    /// </summary>
    Task<bool> ActivateModelAsync(string modelId, CancellationToken cancellationToken = default);

    /// <summary>
    /// Bestätigt/Korrigiert eine Klassifizierung.
    /// </summary>
    Task<bool> ConfirmClassificationAsync(string classificationId, string actualCategory, string? feedback = null, CancellationToken cancellationToken = default);

    /// <summary>
    /// Lädt Klassifizierungs-Historie für ein Dokument.
    /// </summary>
    Task<List<DocumentClassification>> GetClassificationHistoryAsync(string documentId, CancellationToken cancellationToken = default);
}
