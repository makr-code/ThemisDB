/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LLMModels.cs                                       ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     538                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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

namespace Themis.DocumentManager.Models;

/// <summary>
/// Native LLM-Unterstützung für ThemisDB Document Manager
/// Vollständige KI-Integration für intelligente Dokumentenverwaltung
/// </summary>

#region LLM Configuration

/// <summary>
/// LLM Provider Konfiguration
/// </summary>
public class LLMConfiguration
{
    public string Id { get; set; } = string.Empty;
    public LLMProvider Provider { get; set; }
    public string ApiKey { get; set; } = string.Empty;
    public string ApiEndpoint { get; set; } = string.Empty;
    public string ModelName { get; set; } = string.Empty;
    public int MaxTokens { get; set; } = 4096;
    public double Temperature { get; set; } = 0.7;
    public bool IsActive { get; set; } = true;
}

public enum LLMProvider
{
    OpenAI,         // OpenAI GPT-4, GPT-3.5
    AzureOpenAI,    // Azure OpenAI Service
    Anthropic,      // Claude
    Ollama,         // Local Ollama
    HuggingFace,    // HuggingFace Models
    Custom          // Custom Endpoint
}

#endregion

#region Document Analysis

/// <summary>
/// LLM-basierte Dokumentenanalyse
/// URN: urn:themis:llm:analysis:{id}
/// </summary>
public class DocumentAnalysis
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:analysis:{Id}";
    
    public string DocumentId { get; set; } = string.Empty;
    public DateTime AnalyzedAt { get; set; }
    public string ModelUsed { get; set; } = string.Empty;
    
    // Zusammenfassung
    public string Summary { get; set; } = string.Empty;
    public string ShortSummary { get; set; } = string.Empty; // 1-2 Sätze
    public string ExecutiveSummary { get; set; } = string.Empty; // Führungsebene
    
    // Extraktion
    public List<string> KeyPoints { get; set; } = new();
    public List<string> Keywords { get; set; } = new();
    public List<NamedEntity> NamedEntities { get; set; } = new();
    public List<ExtractedDate> ExtractedDates { get; set; } = new();
    public List<ExtractedPerson> ExtractedPersons { get; set; } = new();
    public List<ExtractedOrganization> ExtractedOrganizations { get; set; } = new();
    
    // Klassifizierung
    public string SuggestedCategory { get; set; } = string.Empty;
    public double CategoryConfidence { get; set; }
    public List<string> SuggestedTags { get; set; } = new();
    public DocumentSentiment Sentiment { get; set; } = new();
    
    // Verwaltungsrelevant
    public bool RequiresAction { get; set; }
    public List<ActionItem> ActionItems { get; set; } = new();
    public DateTime? SuggestedDeadline { get; set; }
    public string SuggestedFileNumber { get; set; } = string.Empty;
    public SecurityClassification SuggestedSecurityLevel { get; set; }
    
    // Qualität
    public double AnalysisConfidence { get; set; }
    public int TokensUsed { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public class NamedEntity
{
    public string Text { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty; // Person, Organization, Location, Date, etc.
    public double Confidence { get; set; }
    public int StartPosition { get; set; }
    public int EndPosition { get; set; }
}

public class ExtractedDate
{
    public DateTime Date { get; set; }
    public string Context { get; set; } = string.Empty;
    public DateType Type { get; set; }
}

public enum DateType
{
    Deadline,
    EventDate,
    DocumentDate,
    EffectiveDate,
    ExpirationDate,
    Other
}

public class ExtractedPerson
{
    public string Name { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
    public string Organization { get; set; } = string.Empty;
    public string Context { get; set; } = string.Empty;
}

public class ExtractedOrganization
{
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty;
    public string Context { get; set; } = string.Empty;
}

public class ActionItem
{
    public string Description { get; set; } = string.Empty;
    public string AssignedTo { get; set; } = string.Empty;
    public DateTime? DueDate { get; set; }
    public ActionPriority Priority { get; set; }
}

public enum ActionPriority
{
    Low,
    Normal,
    High,
    Urgent
}

public class DocumentSentiment
{
    public SentimentType Type { get; set; }
    public double PositiveScore { get; set; }
    public double NeutralScore { get; set; }
    public double NegativeScore { get; set; }
}

public enum SentimentType
{
    Positive,
    Neutral,
    Negative,
    Mixed
}

#endregion

#region Semantic Search

/// <summary>
/// Semantische Suchanfrage mit LLM
/// </summary>
public class SemanticQuery
{
    public string Id { get; set; } = string.Empty;
    public string Query { get; set; } = string.Empty;
    public string UserId { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    
    // LLM-erweiterte Suche
    public string ExpandedQuery { get; set; } = string.Empty;
    public List<string> Synonyms { get; set; } = new();
    public List<string> RelatedTerms { get; set; } = new();
    
    // Ergebnisse
    public List<SemanticSearchResult> Results { get; set; } = new();
}

public class SemanticSearchResult
{
    public string DocumentId { get; set; } = string.Empty;
    public double SemanticScore { get; set; }
    public double RelevanceScore { get; set; }
    public string MatchedContent { get; set; } = string.Empty;
    public string Explanation { get; set; } = string.Empty; // LLM erklärt Relevanz
}

#endregion

#region Chat Assistant

/// <summary>
/// LLM-basierter Chat-Assistent für Vorgänge
/// URN: urn:themis:llm:chat:{id}
/// </summary>
public class ChatConversation
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:chat:{Id}";
    
    public string UserId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    
    public DateTime CreatedAt { get; set; }
    public DateTime? LastMessageAt { get; set; }
    
    public string Title { get; set; } = string.Empty;
    public List<ChatMessage> Messages { get; set; } = new();
    
    public Dictionary<string, object> Context { get; set; } = new();
}

public class ChatMessage
{
    public string Id { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; }
    public ChatRole Role { get; set; }
    public string Content { get; set; } = string.Empty;
    
    // Erweiterte Informationen
    public List<string> ReferencedDocuments { get; set; } = new();
    public List<ActionItem> SuggestedActions { get; set; } = new();
    public int TokensUsed { get; set; }
}

public enum ChatRole
{
    User,
    Assistant,
    System
}

#endregion

#region Document Generation

/// <summary>
/// LLM-basierte Dokumentengenerierung
/// </summary>
public class DocumentGenerationRequest
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:generation:{Id}";
    
    public DocumentType DocumentType { get; set; }
    public string Template { get; set; } = string.Empty;
    public Dictionary<string, string> Parameters { get; set; } = new();
    
    public string Purpose { get; set; } = string.Empty;
    public string Tone { get; set; } = "formal"; // formal, informal, neutral
    public string Language { get; set; } = "de";
    
    public string GeneratedContent { get; set; } = string.Empty;
    public DateTime GeneratedAt { get; set; }
    public string ModelUsed { get; set; } = string.Empty;
    public int TokensUsed { get; set; }
}

#endregion

#region Document Comparison

/// <summary>
/// LLM-basierter Dokumentenvergleich
/// </summary>
public class DocumentComparison
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:comparison:{Id}";
    
    public string Document1Id { get; set; } = string.Empty;
    public string Document2Id { get; set; } = string.Empty;
    
    public DateTime ComparedAt { get; set; }
    public string ModelUsed { get; set; } = string.Empty;
    
    // Ergebnisse
    public double SimilarityScore { get; set; }
    public string Summary { get; set; } = string.Empty;
    public List<ContentDifference> Differences { get; set; } = new();
    public List<string> CommonPoints { get; set; } = new();
    public List<string> UniqueToDocument1 { get; set; } = new();
    public List<string> UniqueToDocument2 { get; set; } = new();
}

public class ContentDifference
{
    public string Section { get; set; } = string.Empty;
    public string Content1 { get; set; } = string.Empty;
    public string Content2 { get; set; } = string.Empty;
    public DifferenceType Type { get; set; }
    public string Explanation { get; set; } = string.Empty;
}

public enum DifferenceType
{
    Addition,
    Deletion,
    Modification,
    Reordering
}

#endregion

#region Compliance Check

/// <summary>
/// LLM-basierte Compliance-Prüfung
/// </summary>
public class ComplianceCheck
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:compliance:{Id}";
    
    public string DocumentId { get; set; } = string.Empty;
    public DateTime CheckedAt { get; set; }
    
    public List<string> ApplicableRegulations { get; set; } = new();
    public ComplianceStatus Status { get; set; }
    
    public List<ComplianceIssue> Issues { get; set; } = new();
    public List<ComplianceRecommendation> Recommendations { get; set; } = new();
    
    public double ComplianceScore { get; set; }
    public string Summary { get; set; } = string.Empty;
}

public class ComplianceIssue
{
    public string Regulation { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public ComplianceSeverity Severity { get; set; }
    public string Location { get; set; } = string.Empty;
}

public class ComplianceRecommendation
{
    public string Description { get; set; } = string.Empty;
    public string Reasoning { get; set; } = string.Empty;
    public ActionPriority Priority { get; set; }
}

public enum ComplianceStatus
{
    Compliant,
    PartiallyCompliant,
    NonCompliant,
    RequiresReview
}

public enum ComplianceSeverity
{
    Low,
    Medium,
    High,
    Critical
}

#endregion

#region Translation

/// <summary>
/// LLM-basierte Übersetzung
/// </summary>
public class DocumentTranslation
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:llm:translation:{Id}";
    
    public string SourceDocumentId { get; set; } = string.Empty;
    public string SourceLanguage { get; set; } = string.Empty;
    public string TargetLanguage { get; set; } = string.Empty;
    
    public string TranslatedContent { get; set; } = string.Empty;
    public DateTime TranslatedAt { get; set; }
    
    public bool PreserveFormatting { get; set; } = true;
    public bool PreserveLegalTerms { get; set; } = true;
    
    public double QualityScore { get; set; }
    public string ModelUsed { get; set; } = string.Empty;
}

#endregion

#region Auto-Classification

/// <summary>
/// Automatische Klassifizierung mit LLM
/// </summary>
public class AutoClassification
{
    public string DocumentId { get; set; } = string.Empty;
    public DateTime ClassifiedAt { get; set; }
    
    // Vorschläge
    public string SuggestedAuthority { get; set; } = string.Empty;
    public string SuggestedFiling { get; set; } = string.Empty;
    public string SuggestedFilingPlanNode { get; set; } = string.Empty;
    public ProcessType SuggestedProcessType { get; set; }
    
    // Metadaten-Vorschläge
    public Dictionary<string, string> SuggestedMetadata { get; set; } = new();
    public List<string> SuggestedTags { get; set; } = new();
    public SecurityClassification SuggestedSecurityLevel { get; set; }
    public int SuggestedRetentionYears { get; set; }
    
    // Konfidenz
    public Dictionary<string, double> ConfidenceScores { get; set; } = new();
    
    public string Reasoning { get; set; } = string.Empty;
}

#endregion

#region Redaction

/// <summary>
/// LLM-basierte Schwärzung sensibler Daten
/// </summary>
public class AutoRedaction
{
    public string DocumentId { get; set; } = string.Empty;
    public DateTime RedactedAt { get; set; }
    
    public List<RedactionItem> Redactions { get; set; } = new();
    public string RedactedContent { get; set; } = string.Empty;
    
    public RedactionPolicy Policy { get; set; } = RedactionPolicy.GDPR;
}

public class RedactionItem
{
    public int StartPosition { get; set; }
    public int EndPosition { get; set; }
    public string OriginalText { get; set; } = string.Empty;
    public RedactionType Type { get; set; }
    public string Reason { get; set; } = string.Empty;
}

public enum RedactionType
{
    PersonalData,
    FinancialInfo,
    HealthInfo,
    LegalPrivilege,
    ClassifiedInfo,
    TradeSecret
}

public enum RedactionPolicy
{
    GDPR,
    HIPAA,
    Custom
}

#endregion

#region Quality Assurance

/// <summary>
/// LLM-basierte Qualitätssicherung
/// </summary>
public class QualityAssessment
{
    public string DocumentId { get; set; } = string.Empty;
    public DateTime AssessedAt { get; set; }
    
    public double OverallQuality { get; set; }
    
    // Einzelbewertungen
    public double ClarityScore { get; set; }
    public double CompletenessScore { get; set; }
    public double ConsistencyScore { get; set; }
    public double GrammarScore { get; set; }
    public double StructureScore { get; set; }
    
    public List<QualityIssue> Issues { get; set; } = new();
    public List<QualityImprovement> Suggestions { get; set; } = new();
}

public class QualityIssue
{
    public string Description { get; set; } = string.Empty;
    public string Location { get; set; } = string.Empty;
    public QualitySeverity Severity { get; set; }
    public string Suggestion { get; set; } = string.Empty;
}

public class QualityImprovement
{
    public string Description { get; set; } = string.Empty;
    public string Reasoning { get; set; } = string.Empty;
    public string Example { get; set; } = string.Empty;
}

public enum QualitySeverity
{
    Suggestion,
    Warning,
    Error,
    Critical
}

#endregion
