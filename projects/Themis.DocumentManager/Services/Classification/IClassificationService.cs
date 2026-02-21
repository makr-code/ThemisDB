/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IClassificationService.cs                          ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
