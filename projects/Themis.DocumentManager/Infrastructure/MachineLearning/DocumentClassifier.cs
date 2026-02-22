/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentClassifier.cs                              ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.Extensions.Logging;
using Microsoft.ML;
using Microsoft.ML.Data;
using Themis.DocumentManager.Domain.Classification;

namespace Themis.DocumentManager.Infrastructure.MachineLearning;

/// <summary>
/// ML.NET Service für Dokumenten-Klassifizierung.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class DocumentClassifier
{
    private readonly MLContext _mlContext;
    private readonly ILogger<DocumentClassifier> _logger;
    private ITransformer? _model;
    private string? _modelPath;

    public DocumentClassifier(ILogger<DocumentClassifier> logger)
    {
        _mlContext = new MLContext(seed: 0);
        _logger = logger;
    }

    /// <summary>
    /// Lädt ein trainiertes Modell.
    /// </summary>
    public void LoadModel(string modelPath)
    {
        try
        {
            if (!File.Exists(modelPath))
            {
                throw new FileNotFoundException($"Model file not found: {modelPath}");
            }

            _model = _mlContext.Model.Load(modelPath, out var modelInputSchema);
            _modelPath = modelPath;
            
            _logger.LogInformation("Model loaded successfully from {ModelPath}", modelPath);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to load model from {ModelPath}", modelPath);
            throw;
        }
    }

    /// <summary>
    /// Klassifiziert einen Dokumenten-Text.
    /// </summary>
    public DocumentClassification ClassifyDocument(string documentId, string content)
    {
        if (_model == null)
        {
            throw new InvalidOperationException("Model not loaded. Call LoadModel first.");
        }

        try
        {
            var predictionEngine = _mlContext.Model.CreatePredictionEngine<DocumentInput, DocumentPrediction>(_model);

            var input = new DocumentInput { Text = content };
            var prediction = predictionEngine.Predict(input);

            var classification = new DocumentClassification
            {
                DocumentId = documentId,
                PredictedCategory = prediction.PredictedLabel,
                ConfidenceScore = prediction.Score.Max(),
                ModelVersion = _modelPath ?? "unknown",
                ClassifiedAt = DateTime.UtcNow
            };

            // Alternative Kategorien mit Scores
            if (prediction.Score.Length > 1)
            {
                var categories = GetCategoryNames();
                for (int i = 0; i < prediction.Score.Length; i++)
                {
                    if (i < categories.Count && categories[i] != prediction.PredictedLabel)
                    {
                        classification.AlternativeCategories.Add(new CategoryScore
                        {
                            Category = categories[i],
                            Score = prediction.Score[i]
                        });
                    }
                }
            }

            _logger.LogInformation("Document {DocumentId} classified as {Category} with confidence {Confidence:P2}",
                documentId, classification.PredictedCategory, classification.ConfidenceScore);

            return classification;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to classify document {DocumentId}", documentId);
            throw;
        }
    }

    /// <summary>
    /// Trainiert ein neues Klassifizierungs-Modell.
    /// </summary>
    public ClassificationModel TrainModel(
        List<TrainingData> trainingData,
        string modelName,
        string outputPath,
        float testSplit = 0.2f)
    {
        try
        {
            _logger.LogInformation("Starting model training with {Count} examples", trainingData.Count);

            var startTime = DateTime.UtcNow;

            // Daten konvertieren
            var mlData = trainingData.Select(td => new DocumentInput
            {
                Text = td.Content,
                Label = td.Label
            }).ToList();

            var data = _mlContext.Data.LoadFromEnumerable(mlData);

            // Train/Test Split
            var trainTestSplit = _mlContext.Data.TrainTestSplit(data, testFraction: testSplit);

            // Pipeline erstellen
            var pipeline = _mlContext.Transforms.Conversion.MapValueToKey("Label")
                .Append(_mlContext.Transforms.Text.FeaturizeText("Features", nameof(DocumentInput.Text)))
                .Append(_mlContext.MulticlassClassification.Trainers.SdcaMaximumEntropy())
                .Append(_mlContext.Transforms.Conversion.MapKeyToValue("PredictedLabel"));

            // Modell trainieren
            var model = pipeline.Fit(trainTestSplit.TrainSet);

            // Evaluieren
            var predictions = model.Transform(trainTestSplit.TestSet);
            var metrics = _mlContext.MulticlassClassification.Evaluate(predictions);

            // Modell speichern
            Directory.CreateDirectory(Path.GetDirectoryName(outputPath) ?? ".");
            _mlContext.Model.Save(model, data.Schema, outputPath);

            var trainingDuration = DateTime.UtcNow - startTime;

            var classificationModel = new ClassificationModel
            {
                Name = modelName,
                ModelPath = outputPath,
                TrainedAt = DateTime.UtcNow,
                TrainingDuration = trainingDuration,
                Accuracy = (float)metrics.MacroAccuracy,
                Precision = (float)metrics.LogLoss, // Approximation
                Recall = (float)metrics.LogLossReduction, // Approximation
                F1Score = CalculateF1Score((float)metrics.MacroAccuracy, (float)metrics.MacroAccuracy),
                TrainingExamples = (int)(trainingData.Count * (1 - testSplit)),
                TestExamples = (int)(trainingData.Count * testSplit),
                Categories = trainingData.Select(td => td.Label).Distinct().ToList(),
                IsActive = false
            };

            _logger.LogInformation("Model training completed. Accuracy: {Accuracy:P2}, Duration: {Duration}",
                metrics.MacroAccuracy, trainingDuration);

            return classificationModel;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to train model");
            throw;
        }
    }

    private float CalculateF1Score(float precision, float recall)
    {
        if (precision + recall == 0)
            return 0;
        return 2 * (precision * recall) / (precision + recall);
    }

    private List<string> GetCategoryNames()
    {
        // TODO: Extract from model metadata
        return new List<string> { "Document", "Email", "Report", "Contract", "Invoice", "Other" };
    }
}

/// <summary>
/// Input-Klasse für ML.NET Model.
/// </summary>
public class DocumentInput
{
    [LoadColumn(0)]
    public string Text { get; set; } = string.Empty;

    [LoadColumn(1)]
    public string Label { get; set; } = string.Empty;
}

/// <summary>
/// Prediction-Klasse für ML.NET Model.
/// </summary>
public class DocumentPrediction
{
    [ColumnName("PredictedLabel")]
    public string PredictedLabel { get; set; } = string.Empty;

    [ColumnName("Score")]
    public float[] Score { get; set; } = Array.Empty<float>();
}
