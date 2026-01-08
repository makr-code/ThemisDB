using System.Text.Json.Serialization;

namespace ThemisDB.Client.Llm;

/// <summary>
/// Represents a message in an LLM conversation
/// </summary>
public class LlmMessage
{
    /// <summary>
    /// Role of the message sender (e.g., "user", "assistant", "system")
    /// </summary>
    [JsonPropertyName("role")]
    public string Role { get; set; } = string.Empty;

    /// <summary>
    /// Content of the message
    /// </summary>
    [JsonPropertyName("content")]
    public string Content { get; set; } = string.Empty;

    /// <summary>
    /// Optional image URL for vision models
    /// </summary>
    [JsonPropertyName("image_url")]
    [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
    public string? ImageUrl { get; set; }

    public LlmMessage()
    {
    }

    public LlmMessage(string role, string content, string? imageUrl = null)
    {
        Role = role;
        Content = content;
        ImageUrl = imageUrl;
    }
}
