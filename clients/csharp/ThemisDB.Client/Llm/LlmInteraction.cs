/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmInteraction.cs                                  ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     73                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text.Json.Serialization;

namespace ThemisDB.Client.Llm;

/// <summary>
/// Represents a stored LLM interaction
/// </summary>
public class LlmInteraction
{
    /// <summary>
    /// Unique identifier for the interaction
    /// </summary>
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    /// <summary>
    /// Timestamp when the interaction was created
    /// </summary>
    [JsonPropertyName("created_at")]
    public string CreatedAt { get; set; } = string.Empty;

    /// <summary>
    /// LLM model used (e.g., "gpt-4o", "llama-3.1")
    /// </summary>
    [JsonPropertyName("model")]
    public string Model { get; set; } = string.Empty;

    /// <summary>
    /// Conversation messages
    /// </summary>
    [JsonPropertyName("messages")]
    public List<LlmMessage> Messages { get; set; } = new();

    /// <summary>
    /// Optional reasoning steps
    /// </summary>
    [JsonPropertyName("reasoning_steps")]
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public List<ReasoningStep>? ReasoningSteps { get; set; }

    /// <summary>
    /// Optional metadata
    /// </summary>
    [JsonPropertyName("metadata")]
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public Dictionary<string, object>? Metadata { get; set; }
}
