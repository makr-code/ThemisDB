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
