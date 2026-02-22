/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataExtractor.cs                               ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     299                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Domain.Classification;

namespace Themis.DocumentManager.Infrastructure.MachineLearning;

/// <summary>
/// Service für Metadaten-Extraktion aus Dokumenten.
/// Phase 2 Sprint 7-8 - AI/ML Integration.
/// </summary>
public class MetadataExtractor
{
    private readonly ILogger<MetadataExtractor> _logger;

    // Regex-Patterns für verschiedene Entities
    private static readonly Regex EmailPattern = new(@"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b");
    private static readonly Regex PhonePattern = new(@"\b(\+?\d{1,3}[-.\s]?)?\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4}\b");
    private static readonly Regex DatePattern = new(@"\b\d{1,2}[./\-]\d{1,2}[./\-]\d{2,4}\b");
    private static readonly Regex MoneyPattern = new(@"\b\$?\d{1,3}(,\d{3})*(\.\d{2})?\s*(USD|EUR|GBP|CHF)?\b");
    private static readonly Regex PercentagePattern = new(@"\b\d+(\.\d+)?%\b");

    public MetadataExtractor(ILogger<MetadataExtractor> logger)
    {
        _logger = logger;
    }

    /// <summary>
    /// Extrahiert Metadaten aus einem Text.
    /// </summary>
    public ExtractedMetadata ExtractMetadata(
        string documentId,
        string content,
        bool extractEntities = true,
        bool extractDates = true,
        bool extractKeyPhrases = true,
        bool generateTags = true)
    {
        try
        {
            _logger.LogInformation("Extracting metadata from document {DocumentId}", documentId);

            var metadata = new ExtractedMetadata
            {
                DocumentId = documentId,
                ExtractedAt = DateTime.UtcNow,
                ModelVersion = "1.0.0"
            };

            if (extractEntities)
            {
                metadata.Entities = ExtractNamedEntities(content);
            }

            if (extractDates)
            {
                metadata.Dates = ExtractDates(content);
            }

            if (extractKeyPhrases)
            {
                metadata.KeyPhrases = ExtractKeyPhrases(content);
            }

            if (generateTags)
            {
                metadata.Tags = GenerateTags(content, metadata.Entities);
            }

            _logger.LogInformation("Metadata extraction completed. Found {EntityCount} entities, {DateCount} dates, {KeyPhraseCount} key phrases",
                metadata.Entities.Count, metadata.Dates.Count, metadata.KeyPhrases.Count);

            return metadata;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to extract metadata from document {DocumentId}", documentId);
            throw;
        }
    }

    /// <summary>
    /// Extrahiert Named Entities (Personen, Organisationen, etc.)
    /// </summary>
    private List<NamedEntity> ExtractNamedEntities(string content)
    {
        var entities = new List<NamedEntity>();

        // Email-Adressen
        var emails = EmailPattern.Matches(content);
        foreach (Match match in emails)
        {
            entities.Add(new NamedEntity
            {
                Text = match.Value,
                Type = EntityType.Custom,
                Confidence = 0.95f,
                StartPosition = match.Index,
                EndPosition = match.Index + match.Length
            });
        }

        // Telefonnummern
        var phones = PhonePattern.Matches(content);
        foreach (Match match in phones)
        {
            entities.Add(new NamedEntity
            {
                Text = match.Value,
                Type = EntityType.Custom,
                Confidence = 0.85f,
                StartPosition = match.Index,
                EndPosition = match.Index + match.Length
            });
        }

        // Geldbeträge
        var money = MoneyPattern.Matches(content);
        foreach (Match match in money)
        {
            entities.Add(new NamedEntity
            {
                Text = match.Value,
                Type = EntityType.Money,
                Confidence = 0.9f,
                StartPosition = match.Index,
                EndPosition = match.Index + match.Length
            });
        }

        // Prozentwerte
        var percentages = PercentagePattern.Matches(content);
        foreach (Match match in percentages)
        {
            entities.Add(new NamedEntity
            {
                Text = match.Value,
                Type = EntityType.Percentage,
                Confidence = 0.95f,
                StartPosition = match.Index,
                EndPosition = match.Index + match.Length
            });
        }

        // Einfache Heuristik für Personennamen (Großbuchstaben-Wörter)
        var words = content.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        for (int i = 0; i < words.Length - 1; i++)
        {
            if (char.IsUpper(words[i][0]) && char.IsUpper(words[i + 1][0]) &&
                words[i].Length > 1 && words[i + 1].Length > 1 &&
                !words[i].Contains('.') && !words[i + 1].Contains('.'))
            {
                var fullName = $"{words[i]} {words[i + 1]}";
                var startPos = content.IndexOf(fullName);
                if (startPos >= 0)
                {
                    entities.Add(new NamedEntity
                    {
                        Text = fullName,
                        Type = EntityType.Person,
                        Confidence = 0.6f,
                        StartPosition = startPos,
                        EndPosition = startPos + fullName.Length
                    });
                }
            }
        }

        return entities;
    }

    /// <summary>
    /// Extrahiert Datumsangaben.
    /// </summary>
    private List<DateTime> ExtractDates(string content)
    {
        var dates = new List<DateTime>();
        var matches = DatePattern.Matches(content);

        foreach (Match match in matches)
        {
            if (DateTime.TryParse(match.Value, out var date))
            {
                dates.Add(date);
            }
        }

        return dates.Distinct().ToList();
    }

    /// <summary>
    /// Extrahiert Schlüsselphrasen.
    /// </summary>
    private List<KeyPhrase> ExtractKeyPhrases(string content)
    {
        var keyPhrases = new List<KeyPhrase>();

        // Einfache Heuristik: Häufige Substantiv-Phrasen
        var sentences = content.Split(new[] { '.', '!', '?' }, StringSplitOptions.RemoveEmptyEntries);
        var wordFrequency = new Dictionary<string, int>();

        foreach (var sentence in sentences)
        {
            var words = sentence.Split(' ', StringSplitOptions.RemoveEmptyEntries)
                .Select(w => w.Trim().ToLower())
                .Where(w => w.Length > 3 && !IsStopWord(w));

            foreach (var word in words)
            {
                wordFrequency[word] = wordFrequency.GetValueOrDefault(word, 0) + 1;
            }
        }

        // Top N Wörter als Key Phrases
        var topPhrases = wordFrequency
            .OrderByDescending(kvp => kvp.Value)
            .Take(10);

        foreach (var phrase in topPhrases)
        {
            keyPhrases.Add(new KeyPhrase
            {
                Phrase = phrase.Key,
                Relevance = Math.Min(phrase.Value / 10.0f, 1.0f)
            });
        }

        return keyPhrases;
    }

    /// <summary>
    /// Generiert Tags basierend auf Inhalt und Entities.
    /// </summary>
    private List<string> GenerateTags(string content, List<NamedEntity> entities)
    {
        var tags = new HashSet<string>();

        // Tags aus Entity-Typen
        foreach (var entity in entities)
        {
            tags.Add(entity.Type.ToString().ToLower());
        }

        // Tags aus häufigen Wörtern
        var words = content.Split(' ', StringSplitOptions.RemoveEmptyEntries)
            .Select(w => w.Trim().ToLower())
            .Where(w => w.Length > 4 && !IsStopWord(w));

        var wordCounts = words
            .GroupBy(w => w)
            .Select(g => new { Word = g.Key, Count = g.Count() })
            .OrderByDescending(x => x.Count)
            .Take(5);

        foreach (var word in wordCounts)
        {
            tags.Add(word.Word);
        }

        return tags.ToList();
    }

    /// <summary>
    /// Prüft ob ein Wort ein Stop-Word ist.
    /// </summary>
    private bool IsStopWord(string word)
    {
        var stopWords = new HashSet<string>
        {
            "the", "and", "for", "that", "this", "with", "from", "have", "has", "had",
            "are", "was", "were", "been", "being", "will", "would", "could", "should",
            "can", "may", "must", "der", "die", "das", "und", "für", "mit", "von",
            "auch", "nicht", "ist", "sind", "war", "werden", "kann", "soll"
        };

        return stopWords.Contains(word);
    }
}
