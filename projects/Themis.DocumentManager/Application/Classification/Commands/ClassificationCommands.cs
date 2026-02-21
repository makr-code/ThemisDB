/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationCommands.cs                          ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     95                                             ║
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

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Domain.Classification;

namespace Themis.DocumentManager.Application.Classification.Commands;

/// <summary>
/// Command zum Klassifizieren eines Dokuments mit ML.NET.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public record ClassifyDocumentCommand(
    string DocumentId,
    string? ContentOverride = null
) : IRequest<Result<DocumentClassification>>;

/// <summary>
/// Command zum Trainieren eines neuen Classification-Modells.
/// </summary>
public record TrainClassificationModelCommand(
    string ModelName,
    string Description,
    List<string> TrainingDataIds,
    float TestDataSplit = 0.2f
) : IRequest<Result<ClassificationModel>>;

/// <summary>
/// Command zum Extrahieren von Metadaten aus einem Dokument.
/// </summary>
public record ExtractMetadataCommand(
    string DocumentId,
    bool ExtractEntities = true,
    bool ExtractDates = true,
    bool ExtractKeyPhrases = true,
    bool GenerateTags = true
) : IRequest<Result<ExtractedMetadata>>;

/// <summary>
/// Command zum Hinzufügen von Trainingsdaten.
/// </summary>
public record AddTrainingDataCommand(
    string Content,
    string Label,
    string? DocumentId = null,
    DataUsage Usage = DataUsage.Training,
    bool IsVerified = false
) : IRequest<Result<TrainingData>>;

/// <summary>
/// Command zum Bestätigen/Korrigieren einer Klassifizierung.
/// </summary>
public record ConfirmClassificationCommand(
    string ClassificationId,
    string ActualCategory,
    string? UserFeedback = null
) : IRequest<Result<bool>>;

/// <summary>
/// Command zum Aktivieren eines Modells als Produktiv-Modell.
/// </summary>
public record ActivateModelCommand(
    string ModelId
) : IRequest<Result<bool>>;

/// <summary>
/// Command zum Batch-Classification mehrerer Dokumente.
/// </summary>
public record BatchClassifyDocumentsCommand(
    List<string> DocumentIds
) : IRequest<Result<List<DocumentClassification>>>;
