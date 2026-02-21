/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationService.cs                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     495                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Domain.Classification;
using Themis.DocumentManager.Infrastructure.MachineLearning;
using ThemisDB.Client;

namespace Themis.DocumentManager.Services.Classification;

/// <summary>
/// Service für ML-basierte Dokumenten-Klassifizierung.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class ClassificationService : IClassificationService
{
    private readonly IThemisDBClient _apiClient;
    private readonly DocumentClassifier _classifier;
    private readonly MetadataExtractor _metadataExtractor;
    private readonly ILogger<ClassificationService> _logger;

    // In-Memory Cache
    private readonly Dictionary<string, DocumentClassification> _classificationCache = new();
    private readonly Dictionary<string, ExtractedMetadata> _metadataCache = new();
    private readonly Dictionary<string, ClassificationModel> _models = new();
    private readonly List<TrainingData> _trainingDataCache = new();

    private string? _activeModelId;

    public ClassificationService(
        IThemisDBClient apiClient,
        DocumentClassifier classifier,
        MetadataExtractor metadataExtractor,
        ILogger<ClassificationService> logger)
    {
        _apiClient = apiClient;
        _classifier = classifier;
        _metadataExtractor = metadataExtractor;
        _logger = logger;

        // Initialize with default model if exists
        InitializeDefaultModelAsync().ConfigureAwait(false);
    }

    private async Task InitializeDefaultModelAsync()
    {
        try
        {
            // Load active model from database
            var aql = "FOR model IN classification_models FILTER model.isActive == true LIMIT 1 RETURN model";
            var result = await _apiClient.ExecuteAqlAsync<ClassificationModel>(aql, null);
            
            if (result?.Any() == true)
            {
                var activeModel = result.First();
                _activeModelId = activeModel.Id;
                _models[activeModel.Id] = activeModel;
                
                if (File.Exists(activeModel.ModelPath))
                {
                    _classifier.LoadModel(activeModel.ModelPath);
                    _logger.LogInformation("Active model {ModelId} loaded successfully", activeModel.Id);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Failed to load active model on initialization");
        }
    }

    public async Task<DocumentClassification> ClassifyDocumentAsync(
        string documentId,
        string content,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Classifying document {DocumentId}", documentId);

            // Check cache first
            if (_classificationCache.TryGetValue(documentId, out var cached))
            {
                _logger.LogDebug("Classification found in cache for {DocumentId}", documentId);
                return cached;
            }

            // Classify using ML.NET
            var classification = _classifier.ClassifyDocument(documentId, content);

            // Cache result
            _classificationCache[documentId] = classification;

            // Persist to database
            await PersistClassificationAsync(classification, cancellationToken);

            return classification;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to classify document {DocumentId}", documentId);
            throw;
        }
    }

    public async Task<ExtractedMetadata> ExtractMetadataAsync(
        string documentId,
        string content,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Extracting metadata from document {DocumentId}", documentId);

            // Check cache first
            if (_metadataCache.TryGetValue(documentId, out var cached))
            {
                _logger.LogDebug("Metadata found in cache for {DocumentId}", documentId);
                return cached;
            }

            // Extract metadata
            var metadata = _metadataExtractor.ExtractMetadata(documentId, content);

            // Cache result
            _metadataCache[documentId] = metadata;

            // Persist to database
            await PersistMetadataAsync(metadata, cancellationToken);

            return metadata;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to extract metadata from document {DocumentId}", documentId);
            throw;
        }
    }

    public async Task<ClassificationModel> TrainModelAsync(
        string modelName,
        string description,
        List<string> trainingDataIds,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Starting model training: {ModelName}", modelName);

            // Load training data
            var trainingData = await LoadTrainingDataByIdsAsync(trainingDataIds, cancellationToken);

            if (trainingData.Count == 0)
            {
                throw new InvalidOperationException("No training data found");
            }

            // Define model output path
            var modelsDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Models");
            Directory.CreateDirectory(modelsDir);
            var modelPath = Path.Combine(modelsDir, $"{modelName}_{DateTime.UtcNow:yyyyMMddHHmmss}.zip");

            // Train model
            var model = _classifier.TrainModel(trainingData, modelName, modelPath);
            model.Description = description;

            // Cache model
            _models[model.Id] = model;

            // Persist to database
            await PersistModelAsync(model, cancellationToken);

            _logger.LogInformation("Model {ModelName} trained successfully with accuracy {Accuracy:P2}",
                modelName, model.Accuracy);

            return model;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to train model {ModelName}", modelName);
            throw;
        }
    }

    public async Task<TrainingData> AddTrainingDataAsync(
        TrainingData data,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Adding training data with label {Label}", data.Label);

            // Add to cache
            _trainingDataCache.Add(data);

            // Persist to database
            var aql = "INSERT @data INTO training_data RETURN NEW";
            var bindVars = new Dictionary<string, object>
            {
                { "data", new
                    {
                        _key = data.Id,
                        documentId = data.DocumentId,
                        content = data.Content,
                        label = data.Label,
                        metadata = data.Metadata,
                        isVerified = data.IsVerified,
                        createdAt = data.CreatedAt,
                        source = data.Source,
                        usage = data.Usage.ToString()
                    }
                }
            };

            await _apiClient.ExecuteAqlAsync<object>(aql, bindVars);

            _logger.LogInformation("Training data {Id} added successfully", data.Id);

            return data;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to add training data");
            throw;
        }
    }

    public async Task<List<TrainingData>> GetTrainingDataAsync(
        DataUsage? usage = null,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var aql = usage.HasValue
                ? "FOR data IN training_data FILTER data.usage == @usage RETURN data"
                : "FOR data IN training_data RETURN data";

            var bindVars = usage.HasValue
                ? new Dictionary<string, object> { { "usage", usage.Value.ToString() } }
                : null;

            var result = await _apiClient.ExecuteAqlAsync<dynamic>(aql, bindVars);

            var trainingData = new List<TrainingData>();
            foreach (var item in result ?? Enumerable.Empty<dynamic>())
            {
                trainingData.Add(MapToTrainingData(item));
            }

            return trainingData;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get training data");
            throw;
        }
    }

    public async Task<List<ClassificationModel>> GetModelsAsync(
        bool activeOnly = false,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var aql = activeOnly
                ? "FOR model IN classification_models FILTER model.isActive == true RETURN model"
                : "FOR model IN classification_models RETURN model";

            var result = await _apiClient.ExecuteAqlAsync<ClassificationModel>(aql, null);

            return result?.ToList() ?? new List<ClassificationModel>();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get models");
            throw;
        }
    }

    public async Task<bool> ActivateModelAsync(
        string modelId,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Activating model {ModelId}", modelId);

            // Deactivate all models first
            var deactivateAql = "FOR model IN classification_models UPDATE model WITH { isActive: false } IN classification_models";
            await _apiClient.ExecuteAqlAsync<object>(deactivateAql, null);

            // Activate the specified model
            var activateAql = "FOR model IN classification_models FILTER model._key == @modelId UPDATE model WITH { isActive: true } IN classification_models RETURN NEW";
            var bindVars = new Dictionary<string, object> { { "modelId", modelId } };
            
            var result = await _apiClient.ExecuteAqlAsync<ClassificationModel>(activateAql, bindVars);
            var activatedModel = result?.FirstOrDefault();

            if (activatedModel != null)
            {
                _activeModelId = modelId;
                
                // Load the model into classifier
                if (File.Exists(activatedModel.ModelPath))
                {
                    _classifier.LoadModel(activatedModel.ModelPath);
                    _logger.LogInformation("Model {ModelId} activated and loaded", modelId);
                }

                return true;
            }

            return false;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to activate model {ModelId}", modelId);
            throw;
        }
    }

    public async Task<bool> ConfirmClassificationAsync(
        string classificationId,
        string actualCategory,
        string? feedback = null,
        CancellationToken cancellationToken = default)
    {
        try
        {
            _logger.LogInformation("Confirming classification {ClassificationId}", classificationId);

            var aql = @"
                FOR classification IN document_classifications 
                FILTER classification._key == @classificationId 
                UPDATE classification WITH { 
                    isConfirmed: true, 
                    actualCategory: @actualCategory,
                    userFeedback: @feedback
                } IN document_classifications 
                RETURN NEW";

            var bindVars = new Dictionary<string, object>
            {
                { "classificationId", classificationId },
                { "actualCategory", actualCategory },
                { "feedback", feedback ?? string.Empty }
            };

            var result = await _apiClient.ExecuteAqlAsync<object>(aql, bindVars);

            // Update cache if exists
            if (_classificationCache.Values.FirstOrDefault(c => c.Id == classificationId) is var cached && cached != null)
            {
                cached.IsConfirmed = true;
                cached.ActualCategory = actualCategory;
                cached.UserFeedback = feedback;
            }

            return result?.Any() == true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to confirm classification {ClassificationId}", classificationId);
            throw;
        }
    }

    public async Task<List<DocumentClassification>> GetClassificationHistoryAsync(
        string documentId,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var aql = "FOR classification IN document_classifications FILTER classification.documentId == @documentId SORT classification.classifiedAt DESC RETURN classification";
            var bindVars = new Dictionary<string, object> { { "documentId", documentId } };

            var result = await _apiClient.ExecuteAqlAsync<DocumentClassification>(aql, bindVars);

            return result?.ToList() ?? new List<DocumentClassification>();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to get classification history for {DocumentId}", documentId);
            throw;
        }
    }

    // Helper methods

    private async Task PersistClassificationAsync(DocumentClassification classification, CancellationToken cancellationToken)
    {
        var aql = "INSERT @classification INTO document_classifications RETURN NEW";
        var bindVars = new Dictionary<string, object>
        {
            { "classification", new
                {
                    _key = classification.Id,
                    documentId = classification.DocumentId,
                    predictedCategory = classification.PredictedCategory,
                    confidenceScore = classification.ConfidenceScore,
                    alternativeCategories = classification.AlternativeCategories.Select(c => new { category = c.Category, score = c.Score }),
                    classifiedAt = classification.ClassifiedAt,
                    modelVersion = classification.ModelVersion,
                    isConfirmed = classification.IsConfirmed
                }
            }
        };

        await _apiClient.ExecuteAqlAsync<object>(aql, bindVars);
    }

    private async Task PersistMetadataAsync(ExtractedMetadata metadata, CancellationToken cancellationToken)
    {
        var aql = "INSERT @metadata INTO extracted_metadata RETURN NEW";
        var bindVars = new Dictionary<string, object>
        {
            { "metadata", new
                {
                    _key = metadata.Id,
                    documentId = metadata.DocumentId,
                    entities = metadata.Entities.Select(e => new { text = e.Text, type = e.Type.ToString(), confidence = e.Confidence }),
                    dates = metadata.Dates,
                    tags = metadata.Tags,
                    keyPhrases = metadata.KeyPhrases.Select(kp => new { phrase = kp.Phrase, relevance = kp.Relevance }),
                    extractedAt = metadata.ExtractedAt,
                    modelVersion = metadata.ModelVersion
                }
            }
        };

        await _apiClient.ExecuteAqlAsync<object>(aql, bindVars);
    }

    private async Task PersistModelAsync(ClassificationModel model, CancellationToken cancellationToken)
    {
        var aql = "INSERT @model INTO classification_models RETURN NEW";
        var bindVars = new Dictionary<string, object>
        {
            { "model", new
                {
                    _key = model.Id,
                    name = model.Name,
                    description = model.Description,
                    modelPath = model.ModelPath,
                    trainedAt = model.TrainedAt,
                    trainingDuration = model.TrainingDuration.TotalSeconds,
                    accuracy = model.Accuracy,
                    precision = model.Precision,
                    recall = model.Recall,
                    f1Score = model.F1Score,
                    trainingExamples = model.TrainingExamples,
                    testExamples = model.TestExamples,
                    categories = model.Categories,
                    isActive = model.IsActive
                }
            }
        };

        await _apiClient.ExecuteAqlAsync<object>(aql, bindVars);
    }

    private async Task<List<TrainingData>> LoadTrainingDataByIdsAsync(List<string> ids, CancellationToken cancellationToken)
    {
        var aql = "FOR data IN training_data FILTER data._key IN @ids RETURN data";
        var bindVars = new Dictionary<string, object> { { "ids", ids } };

        var result = await _apiClient.ExecuteAqlAsync<dynamic>(aql, bindVars);

        var trainingData = new List<TrainingData>();
        foreach (var item in result ?? Enumerable.Empty<dynamic>())
        {
            trainingData.Add(MapToTrainingData(item));
        }

        return trainingData;
    }

    private TrainingData MapToTrainingData(dynamic item)
    {
        return new TrainingData
        {
            Id = item._key?.ToString() ?? Guid.NewGuid().ToString(),
            DocumentId = item.documentId?.ToString(),
            Content = item.content?.ToString() ?? string.Empty,
            Label = item.label?.ToString() ?? string.Empty,
            IsVerified = item.isVerified ?? false,
            CreatedAt = item.createdAt ?? DateTime.UtcNow,
            Source = item.source?.ToString() ?? "Unknown",
            Usage = Enum.TryParse<DataUsage>(item.usage?.ToString(), out var usage) ? usage : DataUsage.Training
        };
    }
}
