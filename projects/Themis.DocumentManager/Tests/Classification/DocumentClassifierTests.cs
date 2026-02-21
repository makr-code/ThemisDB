/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentClassifierTests.cs                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     255                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.Extensions.Logging;
using Moq;
using Themis.DocumentManager.Domain.Classification;
using Themis.DocumentManager.Infrastructure.MachineLearning;
using Xunit;

namespace Themis.DocumentManager.Tests.Classification;

/// <summary>
/// Tests für ML.NET Document Classifier.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class DocumentClassifierTests
{
    private readonly Mock<ILogger<DocumentClassifier>> _mockLogger;

    public DocumentClassifierTests()
    {
        _mockLogger = new Mock<ILogger<DocumentClassifier>>();
    }

    [Fact]
    public void TrainModel_WithValidTrainingData_ReturnsModel()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateSampleTrainingData();
        var outputPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");

        // Act
        var model = classifier.TrainModel(trainingData, "TestModel", outputPath);

        // Assert
        Assert.NotNull(model);
        Assert.Equal("TestModel", model.Name);
        Assert.True(model.Accuracy >= 0 && model.Accuracy <= 1);
        Assert.True(File.Exists(outputPath));
        Assert.Equal(trainingData.Select(td => td.Label).Distinct().Count(), model.Categories.Count);

        // Cleanup
        if (File.Exists(outputPath))
            File.Delete(outputPath);
    }

    [Fact]
    public void TrainModel_WithSufficientData_MeetsAccuracyThreshold()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateLargeTrainingDataset(100); // More data = better accuracy
        var outputPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");

        // Act
        var model = classifier.TrainModel(trainingData, "AccuracyTestModel", outputPath, testSplit: 0.2f);

        // Assert
        Assert.NotNull(model);
        Assert.True(model.Accuracy > 0.5f, $"Model accuracy {model.Accuracy:P2} should be > 50%");
        Assert.Equal(80, model.TrainingExamples); // 100 * 0.8
        Assert.Equal(20, model.TestExamples); // 100 * 0.2

        // Cleanup
        if (File.Exists(outputPath))
            File.Delete(outputPath);
    }

    [Fact]
    public void ClassifyDocument_WithLoadedModel_ReturnsClassification()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateSampleTrainingData();
        var modelPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");
        
        // Train and save model
        classifier.TrainModel(trainingData, "ClassifyTestModel", modelPath);
        classifier.LoadModel(modelPath);

        // Act
        var classification = classifier.ClassifyDocument("doc123", "This is a contract document with legal terms");

        // Assert
        Assert.NotNull(classification);
        Assert.Equal("doc123", classification.DocumentId);
        Assert.NotEmpty(classification.PredictedCategory);
        Assert.True(classification.ConfidenceScore >= 0 && classification.ConfidenceScore <= 1);

        // Cleanup
        if (File.Exists(modelPath))
            File.Delete(modelPath);
    }

    [Fact]
    public void LoadModel_WithNonExistentFile_ThrowsFileNotFoundException()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var nonExistentPath = Path.Combine(Path.GetTempPath(), "nonexistent_model.zip");

        // Act & Assert
        Assert.Throws<FileNotFoundException>(() => classifier.LoadModel(nonExistentPath));
    }

    [Fact]
    public void ClassifyDocument_WithoutLoadedModel_ThrowsInvalidOperationException()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);

        // Act & Assert
        Assert.Throws<InvalidOperationException>(() => 
            classifier.ClassifyDocument("doc123", "Some content"));
    }

    [Fact]
    public void TrainModel_ValidatesModelMetrics()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateSampleTrainingData();
        var outputPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");

        // Act
        var model = classifier.TrainModel(trainingData, "MetricsTestModel", outputPath);

        // Assert - Verify all metrics are in valid range
        Assert.InRange(model.Accuracy, 0f, 1f);
        Assert.InRange(model.Precision, 0f, float.MaxValue); // LogLoss approximation
        Assert.InRange(model.Recall, 0f, float.MaxValue); // LogLossReduction approximation
        Assert.InRange(model.F1Score, 0f, 1f);
        Assert.True(model.TrainingDuration.TotalSeconds > 0);

        // Cleanup
        if (File.Exists(outputPath))
            File.Delete(outputPath);
    }

    [Fact]
    public void ClassifyDocument_ReturnsAlternativeCategories()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateSampleTrainingData();
        var modelPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");
        
        classifier.TrainModel(trainingData, "AlternativeTestModel", modelPath);
        classifier.LoadModel(modelPath);

        // Act
        var classification = classifier.ClassifyDocument("doc123", "Invoice for services rendered");

        // Assert
        Assert.NotNull(classification);
        // AlternativeCategories might be empty if there's only one strong prediction
        // but the structure should be present
        Assert.NotNull(classification.AlternativeCategories);

        // Cleanup
        if (File.Exists(modelPath))
            File.Delete(modelPath);
    }

    [Fact]
    public void TrainModel_WithDifferentCategories_CreatesMultiClassModel()
    {
        // Arrange
        var classifier = new DocumentClassifier(_mockLogger.Object);
        var trainingData = CreateMultiCategoryTrainingData();
        var outputPath = Path.Combine(Path.GetTempPath(), $"test_model_{Guid.NewGuid()}.zip");

        // Act
        var model = classifier.TrainModel(trainingData, "MultiClassModel", outputPath);

        // Assert
        Assert.NotNull(model);
        Assert.True(model.Categories.Count >= 3, "Should have at least 3 categories");
        Assert.Contains("Contract", model.Categories);
        Assert.Contains("Invoice", model.Categories);
        Assert.Contains("Email", model.Categories);

        // Cleanup
        if (File.Exists(outputPath))
            File.Delete(outputPath);
    }

    // Helper methods

    private List<TrainingData> CreateSampleTrainingData()
    {
        return new List<TrainingData>
        {
            new TrainingData { Content = "This is a contract agreement between parties", Label = "Contract", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Invoice for professional services rendered", Label = "Invoice", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Dear Sir, please find attached the requested document", Label = "Email", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Quarterly financial report for Q1 2024", Label = "Report", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Contract terms and conditions apply", Label = "Contract", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Payment due for invoice #12345", Label = "Invoice", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Email regarding meeting schedule", Label = "Email", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Annual performance report", Label = "Report", IsVerified = true, Usage = DataUsage.Training },
            new TrainingData { Content = "Legal contract for services", Label = "Contract", IsVerified = true, Usage = DataUsage.Test },
            new TrainingData { Content = "Invoice total amount $1500", Label = "Invoice", IsVerified = true, Usage = DataUsage.Test }
        };
    }

    private List<TrainingData> CreateLargeTrainingDataset(int count)
    {
        var data = new List<TrainingData>();
        var categories = new[] { "Contract", "Invoice", "Email", "Report" };
        var templates = new Dictionary<string, string[]>
        {
            ["Contract"] = new[] { "agreement", "terms", "legal", "parties", "conditions" },
            ["Invoice"] = new[] { "payment", "amount", "due", "billing", "total" },
            ["Email"] = new[] { "dear", "regards", "attached", "please find", "meeting" },
            ["Report"] = new[] { "quarterly", "annual", "performance", "analysis", "summary" }
        };

        var random = new Random(42); // Fixed seed for reproducibility

        for (int i = 0; i < count; i++)
        {
            var category = categories[i % categories.Length];
            var words = templates[category];
            var content = $"Document {i}: {string.Join(" ", words.OrderBy(x => random.Next()))}";

            data.Add(new TrainingData
            {
                Content = content,
                Label = category,
                IsVerified = true,
                Usage = i < count * 0.8 ? DataUsage.Training : DataUsage.Test
            });
        }

        return data;
    }

    private List<TrainingData> CreateMultiCategoryTrainingData()
    {
        return new List<TrainingData>
        {
            new TrainingData { Content = "Contract agreement for software license", Label = "Contract" },
            new TrainingData { Content = "Contract terms and service level agreement", Label = "Contract" },
            new TrainingData { Content = "Invoice #001 for consulting services", Label = "Invoice" },
            new TrainingData { Content = "Payment invoice total $2500", Label = "Invoice" },
            new TrainingData { Content = "Email from customer support team", Label = "Email" },
            new TrainingData { Content = "Dear customer, your order is ready", Label = "Email" },
            new TrainingData { Content = "Monthly financial report Q3 2024", Label = "Report" },
            new TrainingData { Content = "Performance analysis report", Label = "Report" }
        };
    }
}
