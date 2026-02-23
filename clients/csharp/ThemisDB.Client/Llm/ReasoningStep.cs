/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReasoningStep.cs                                   ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text.Json.Serialization;

namespace ThemisDB.Client.Llm;

/// <summary>
/// Represents a reasoning step in LLM interaction
/// </summary>
public class ReasoningStep
{
    /// <summary>
    /// Type of reasoning (e.g., "chain_of_thought")
    /// </summary>
    [JsonPropertyName("type")]
    public string Type { get; set; } = string.Empty;

    /// <summary>
    /// Content of the reasoning step
    /// </summary>
    [JsonPropertyName("content")]
    public List<string> Content { get; set; } = new();

    public ReasoningStep()
    {
    }

    public ReasoningStep(string type, List<string> content)
    {
        Type = type;
        Content = content;
    }
}
