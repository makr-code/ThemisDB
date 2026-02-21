/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationHandlers.cs                          ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     489                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using MediatR;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Application.Classification.Commands;
using Themis.DocumentManager.Application.Classification.Queries;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Domain.Classification;
using Themis.DocumentManager.Services.Classification;

namespace Themis.DocumentManager.Application.Classification.Handlers;

/// <summary>
/// Handler für ClassifyDocumentCommand.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class ClassifyDocumentHandler : IRequestHandler<ClassifyDocumentCommand, Result<DocumentClassification>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<ClassifyDocumentHandler> _logger;

    public ClassifyDocumentHandler(
        IClassificationService classificationService,
        ILogger<ClassifyDocumentHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<DocumentClassification>> Handle(
        ClassifyDocumentCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling ClassifyDocumentCommand for {DocumentId}", request.DocumentId);

            // In real implementation, fetch document content from document service
            var content = request.ContentOverride ?? "Sample document content";

            var classification = await _classificationService.ClassifyDocumentAsync(
                request.DocumentId,
                content,
                cancellationToken);

            return Result<DocumentClassification>.Success(classification);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to classify document {DocumentId}", request.DocumentId);
            return Result<DocumentClassification>.Failure($"Classification failed: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für TrainClassificationModelCommand.
/// </summary>
public class TrainClassificationModelHandler : IRequestHandler<TrainClassificationModelCommand, Result<ClassificationModel>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<TrainClassificationModelHandler> _logger;

    public TrainClassificationModelHandler(
        IClassificationService classificationService,
        ILogger<TrainClassificationModelHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<ClassificationModel>> Handle(
        TrainClassificationModelCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling TrainClassificationModelCommand for {ModelName}", request.ModelName);

            var model = await _classificationService.TrainModelAsync(
                request.ModelName,
                request.Description,
                request.TrainingDataIds,
                cancellationToken);

            return Result<ClassificationModel>.Success(model);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to train model {ModelName}", request.ModelName);
            return Result<ClassificationModel>.Failure($"Model training failed: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für ExtractMetadataCommand.
/// </summary>
public class ExtractMetadataHandler : IRequestHandler<ExtractMetadataCommand, Result<ExtractedMetadata>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<ExtractMetadataHandler> _logger;

    public ExtractMetadataHandler(
        IClassificationService classificationService,
        ILogger<ExtractMetadataHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<ExtractedMetadata>> Handle(
        ExtractMetadataCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling ExtractMetadataCommand for {DocumentId}", request.DocumentId);

            // In real implementation, fetch document content from document service
            var content = "Sample document content with entities";

            var metadata = await _classificationService.ExtractMetadataAsync(
                request.DocumentId,
                content,
                cancellationToken);

            return Result<ExtractedMetadata>.Success(metadata);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to extract metadata from {DocumentId}", request.DocumentId);
            return Result<ExtractedMetadata>.Failure($"Metadata extraction failed: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für AddTrainingDataCommand.
/// </summary>
public class AddTrainingDataHandler : IRequestHandler<AddTrainingDataCommand, Result<TrainingData>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<AddTrainingDataHandler> _logger;

    public AddTrainingDataHandler(
        IClassificationService classificationService,
        ILogger<AddTrainingDataHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<TrainingData>> Handle(
        AddTrainingDataCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling AddTrainingDataCommand with label {Label}", request.Label);

            var trainingData = new TrainingData
            {
                Content = request.Content,
                Label = request.Label,
                DocumentId = request.DocumentId,
                Usage = request.Usage,
                IsVerified = request.IsVerified,
                Source = "Manual"
            };

            var result = await _classificationService.AddTrainingDataAsync(trainingData, cancellationToken);

            return Result<TrainingData>.Success(result);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to add training data");
            return Result<TrainingData>.Failure($"Failed to add training data: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für ConfirmClassificationCommand.
/// </summary>
public class ConfirmClassificationHandler : IRequestHandler<ConfirmClassificationCommand, Result<bool>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<ConfirmClassificationHandler> _logger;

    public ConfirmClassificationHandler(
        IClassificationService classificationService,
        ILogger<ConfirmClassificationHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<bool>> Handle(
        ConfirmClassificationCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling ConfirmClassificationCommand for {ClassificationId}", request.ClassificationId);

            var success = await _classificationService.ConfirmClassificationAsync(
                request.ClassificationId,
                request.ActualCategory,
                request.UserFeedback,
                cancellationToken);

            return Result<bool>.Success(success);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to confirm classification {ClassificationId}", request.ClassificationId);
            return Result<bool>.Failure($"Failed to confirm classification: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für ActivateModelCommand.
/// </summary>
public class ActivateModelHandler : IRequestHandler<ActivateModelCommand, Result<bool>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<ActivateModelHandler> _logger;

    public ActivateModelHandler(
        IClassificationService classificationService,
        ILogger<ActivateModelHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<bool>> Handle(
        ActivateModelCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling ActivateModelCommand for {ModelId}", request.ModelId);

            var success = await _classificationService.ActivateModelAsync(request.ModelId, cancellationToken);

            return Result<bool>.Success(success);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to activate model {ModelId}", request.ModelId);
            return Result<bool>.Failure($"Failed to activate model: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für BatchClassifyDocumentsCommand.
/// </summary>
public class BatchClassifyDocumentsHandler : IRequestHandler<BatchClassifyDocumentsCommand, Result<System.Collections.Generic.List<DocumentClassification>>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<BatchClassifyDocumentsHandler> _logger;

    public BatchClassifyDocumentsHandler(
        IClassificationService classificationService,
        ILogger<BatchClassifyDocumentsHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<System.Collections.Generic.List<DocumentClassification>>> Handle(
        BatchClassifyDocumentsCommand request,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Handling BatchClassifyDocumentsCommand for {Count} documents", request.DocumentIds.Count);

            var classifications = new System.Collections.Generic.List<DocumentClassification>();

            foreach (var documentId in request.DocumentIds)
            {
                var content = "Sample content"; // Fetch real content in production
                var classification = await _classificationService.ClassifyDocumentAsync(documentId, content, cancellationToken);
                classifications.Add(classification);
            }

            return Result<System.Collections.Generic.List<DocumentClassification>>.Success(classifications);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to batch classify documents");
            return Result<System.Collections.Generic.List<DocumentClassification>>.Failure($"Batch classification failed: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für GetClassificationPredictionQuery.
/// </summary>
public class GetClassificationPredictionHandler : IRequestHandler<GetClassificationPredictionQuery, Result<DocumentClassification?>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<GetClassificationPredictionHandler> _logger;

    public GetClassificationPredictionHandler(
        IClassificationService classificationService,
        ILogger<GetClassificationPredictionHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<DocumentClassification?>> Handle(
        GetClassificationPredictionQuery request,
        CancellationToken cancellationToken)
    {
        try
        {
            var history = await _classificationService.GetClassificationHistoryAsync(request.DocumentId, cancellationToken);
            var latest = history.FirstOrDefault();

            return Result<DocumentClassification?>.Success(latest);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get classification prediction for {DocumentId}", request.DocumentId);
            return Result<DocumentClassification?>.Failure($"Failed to get prediction: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für GetTrainingDataQuery.
/// </summary>
public class GetTrainingDataHandler : IRequestHandler<GetTrainingDataQuery, Result<TrainingDataCollection>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<GetTrainingDataHandler> _logger;

    public GetTrainingDataHandler(
        IClassificationService classificationService,
        ILogger<GetTrainingDataHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<TrainingDataCollection>> Handle(
        GetTrainingDataQuery request,
        CancellationToken cancellationToken)
    {
        try
        {
            var allData = await _classificationService.GetTrainingDataAsync(request.Usage, cancellationToken);

            // Apply filters
            if (!string.IsNullOrEmpty(request.Label))
            {
                allData = allData.Where(d => d.Label == request.Label).ToList();
            }

            if (request.VerifiedOnly)
            {
                allData = allData.Where(d => d.IsVerified).ToList();
            }

            // Pagination
            var skip = (request.PageNumber - 1) * request.PageSize;
            var pagedData = allData.Skip(skip).Take(request.PageSize).ToList();

            var collection = new TrainingDataCollection
            {
                Data = pagedData,
                TotalCount = allData.Count,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return Result<TrainingDataCollection>.Success(collection);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get training data");
            return Result<TrainingDataCollection>.Failure($"Failed to get training data: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für GetClassificationModelsQuery.
/// </summary>
public class GetClassificationModelsHandler : IRequestHandler<GetClassificationModelsQuery, Result<System.Collections.Generic.List<ClassificationModel>>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<GetClassificationModelsHandler> _logger;

    public GetClassificationModelsHandler(
        IClassificationService classificationService,
        ILogger<GetClassificationModelsHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<System.Collections.Generic.List<ClassificationModel>>> Handle(
        GetClassificationModelsQuery request,
        CancellationToken cancellationToken)
    {
        try
        {
            var models = await _classificationService.GetModelsAsync(request.ActiveOnly, cancellationToken);

            return Result<System.Collections.Generic.List<ClassificationModel>>.Success(models);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get models");
            return Result<System.Collections.Generic.List<ClassificationModel>>.Failure($"Failed to get models: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für GetClassificationHistoryQuery.
/// </summary>
public class GetClassificationHistoryHandler : IRequestHandler<GetClassificationHistoryQuery, Result<System.Collections.Generic.List<DocumentClassification>>>
{
    private readonly IClassificationService _classificationService;
    private readonly ILogger<GetClassificationHistoryHandler> _logger;

    public GetClassificationHistoryHandler(
        IClassificationService classificationService,
        ILogger<GetClassificationHistoryHandler> logger)
    {
        _classificationService = classificationService;
        _logger = logger;
    }

    public async Task<Result<System.Collections.Generic.List<DocumentClassification>>> Handle(
        GetClassificationHistoryQuery request,
        CancellationToken cancellationToken)
    {
        try
        {
            var history = await _classificationService.GetClassificationHistoryAsync(request.DocumentId, cancellationToken);

            return Result<System.Collections.Generic.List<DocumentClassification>>.Success(history);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get classification history for {DocumentId}", request.DocumentId);
            return Result<System.Collections.Generic.List<DocumentClassification>>.Failure($"Failed to get history: {ex.Message}");
        }
    }
}
