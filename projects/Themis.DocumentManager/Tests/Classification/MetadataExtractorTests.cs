/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataExtractorTests.cs                          ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Linq;
using Microsoft.Extensions.Logging;
using Moq;
using Themis.DocumentManager.Domain.Classification;
using Themis.DocumentManager.Infrastructure.MachineLearning;
using Xunit;

namespace Themis.DocumentManager.Tests.Classification;

/// <summary>
/// Tests für MetadataExtractor.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class MetadataExtractorTests
{
    private readonly Mock<ILogger<MetadataExtractor>> _mockLogger;
    private readonly MetadataExtractor _extractor;

    public MetadataExtractorTests()
    {
        _mockLogger = new Mock<ILogger<MetadataExtractor>>();
        _extractor = new MetadataExtractor(_mockLogger.Object);
    }

    [Fact]
    public void ExtractMetadata_WithEmailAddresses_ExtractsCorrectly()
    {
        // Arrange
        var content = "Contact us at support@example.com or sales@company.org for more information.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        Assert.Equal("doc123", metadata.DocumentId);
        Assert.True(metadata.Entities.Any(e => e.Text.Contains("@")));
        Assert.Contains(metadata.Entities, e => e.Text == "support@example.com");
        Assert.Contains(metadata.Entities, e => e.Text == "sales@company.org");
    }

    [Fact]
    public void ExtractMetadata_WithPhoneNumbers_ExtractsCorrectly()
    {
        // Arrange
        var content = "Call us at 555-123-4567 or +1 (800) 555-0199";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        Assert.True(metadata.Entities.Count > 0);
        // Phone pattern should match
        var phoneEntities = metadata.Entities.Where(e => e.Type == EntityType.Custom).ToList();
        Assert.NotEmpty(phoneEntities);
    }

    [Fact]
    public void ExtractMetadata_WithMoneyAmounts_ExtractsCorrectly()
    {
        // Arrange
        var content = "The total cost is $1,500.00 USD and additional fee of €250 EUR.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        var moneyEntities = metadata.Entities.Where(e => e.Type == EntityType.Money).ToList();
        Assert.NotEmpty(moneyEntities);
    }

    [Fact]
    public void ExtractMetadata_WithPercentages_ExtractsCorrectly()
    {
        // Arrange
        var content = "The interest rate is 5.5% and the success rate is 95%.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        var percentageEntities = metadata.Entities.Where(e => e.Type == EntityType.Percentage).ToList();
        Assert.True(percentageEntities.Count >= 2);
        Assert.Contains(percentageEntities, e => e.Text == "5.5%");
        Assert.Contains(percentageEntities, e => e.Text == "95%");
    }

    [Fact]
    public void ExtractMetadata_WithDates_ExtractsCorrectly()
    {
        // Arrange
        var content = "The meeting is scheduled for 12/31/2024 and the deadline is 01-15-2025.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content, extractDates: true);

        // Assert
        Assert.NotNull(metadata);
        Assert.NotEmpty(metadata.Dates);
        // Dates should be parsed
        Assert.True(metadata.Dates.Count >= 1);
    }

    [Fact]
    public void ExtractMetadata_WithPersonNames_DetectsBasicNames()
    {
        // Arrange
        var content = "John Smith and Mary Johnson attended the meeting with Dr. Robert Brown.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        var personEntities = metadata.Entities.Where(e => e.Type == EntityType.Person).ToList();
        // Heuristic-based person name detection should find some names
        Assert.NotEmpty(personEntities);
    }

    [Fact]
    public void ExtractMetadata_WithKeyPhrases_ExtractsRelevantTerms()
    {
        // Arrange
        var content = "Machine learning artificial intelligence deep learning neural networks data science";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content, extractKeyPhrases: true);

        // Assert
        Assert.NotNull(metadata);
        Assert.NotEmpty(metadata.KeyPhrases);
        // Should extract frequent/important terms
        Assert.True(metadata.KeyPhrases.Any(kp => kp.Relevance > 0));
    }

    [Fact]
    public void ExtractMetadata_WithTags_GeneratesRelevantTags()
    {
        // Arrange
        var content = "Email address test@example.com with 50% discount and $100 payment.";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content, generateTags: true);

        // Assert
        Assert.NotNull(metadata);
        Assert.NotEmpty(metadata.Tags);
        // Tags should be generated from entity types and frequent words
        Assert.Contains(metadata.Tags, t => t == "money" || t == "percentage" || t == "custom");
    }

    [Fact]
    public void ExtractMetadata_WithDisabledFeatures_ReturnsEmptyCollections()
    {
        // Arrange
        var content = "Some content with email@test.com and 25%";

        // Act
        var metadata = _extractor.ExtractMetadata(
            "doc123", 
            content,
            extractEntities: false,
            extractDates: false,
            extractKeyPhrases: false,
            generateTags: false);

        // Assert
        Assert.NotNull(metadata);
        Assert.Empty(metadata.Entities);
        Assert.Empty(metadata.Dates);
        Assert.Empty(metadata.KeyPhrases);
        Assert.Empty(metadata.Tags);
    }

    [Fact]
    public void ExtractMetadata_SetsMetadataFields()
    {
        // Arrange
        var content = "Test content";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.Equal("doc123", metadata.DocumentId);
        Assert.NotEqual(default(DateTime), metadata.ExtractedAt);
        Assert.NotEmpty(metadata.ModelVersion);
        Assert.True(metadata.ExtractedAt <= DateTime.UtcNow);
    }

    [Fact]
    public void ExtractMetadata_WithComplexDocument_ExtractsMultipleEntities()
    {
        // Arrange
        var content = @"
            Dear John Smith,
            
            Please contact us at support@company.com or call +1-555-1234.
            Your invoice total is $1,250.50 USD with a 10% discount.
            The payment is due by 12/31/2024.
            
            Best regards,
            Mary Johnson
            Sales Department
        ";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        
        // Should have various entity types
        Assert.True(metadata.Entities.Count > 0, "Should extract entities");
        
        // Should have different entity types
        var entityTypes = metadata.Entities.Select(e => e.Type).Distinct().ToList();
        Assert.True(entityTypes.Count > 1, "Should have multiple entity types");
        
        // Dates should be extracted
        Assert.NotEmpty(metadata.Dates);
        
        // Tags should be generated
        Assert.NotEmpty(metadata.Tags);
    }

    [Fact]
    public void ExtractMetadata_WithEmptyContent_ReturnsMetadataWithEmptyCollections()
    {
        // Arrange
        var content = "";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        Assert.Equal("doc123", metadata.DocumentId);
        Assert.Empty(metadata.Entities);
        Assert.Empty(metadata.Dates);
        Assert.Empty(metadata.KeyPhrases);
        Assert.Empty(metadata.Tags);
    }

    [Fact]
    public void ExtractMetadata_EntityConfidenceScores_AreValid()
    {
        // Arrange
        var content = "Email: test@example.com, Amount: $500, Percentage: 25%";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content);

        // Assert
        Assert.NotNull(metadata);
        Assert.All(metadata.Entities, entity =>
        {
            Assert.InRange(entity.Confidence, 0f, 1f);
        });
    }

    [Fact]
    public void ExtractMetadata_KeyPhraseRelevance_IsValid()
    {
        // Arrange
        var content = "important critical essential significant vital crucial fundamental necessary";

        // Act
        var metadata = _extractor.ExtractMetadata("doc123", content, extractKeyPhrases: true);

        // Assert
        Assert.NotNull(metadata);
        Assert.All(metadata.KeyPhrases, phrase =>
        {
            Assert.InRange(phrase.Relevance, 0f, 1f);
        });
    }
}
