/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmInteraction.cs                                  ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
