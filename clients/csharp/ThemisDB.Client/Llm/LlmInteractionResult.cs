using System.Text.Json.Serialization;

namespace ThemisDB.Client.Llm;

/// <summary>
/// Result of creating an LLM interaction
/// </summary>
public class LlmInteractionResult
{
    /// <summary>
    /// Unique identifier for the created interaction
    /// </summary>
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    /// <summary>
    /// Indicates whether the interaction was created successfully
    /// </summary>
    [JsonPropertyName("success")]
    public bool Success { get; set; }
}
