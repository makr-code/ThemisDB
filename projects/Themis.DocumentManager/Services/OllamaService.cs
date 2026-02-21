/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OllamaService.cs                                   ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Ollama LLM Integration Service - Lokale KI-Unterstützung
/// </summary>
public interface IOllamaService
{
    /// <summary>
    /// Chat mit LLM (Streaming)
    /// </summary>
    Task<string> ChatAsync(string prompt, string model = "llama2", CancellationToken cancellationToken = default);

    /// <summary>
    /// Chat mit Kontext-History
    /// </summary>
    Task<string> ChatWithHistoryAsync(List<OllamaMessage> messages, string model = "llama2", CancellationToken cancellationToken = default);

    /// <summary>
    /// Dokument-Zusammenfassung generieren
    /// </summary>
    Task<string> SummarizeDocumentAsync(string documentText, CancellationToken cancellationToken = default);

    /// <summary>
    /// Metadaten aus Text extrahieren
    /// </summary>
    Task<Dictionary<string, string>> ExtractMetadataAsync(string documentText, CancellationToken cancellationToken = default);

    /// <summary>
    /// Verfügbare Modelle abrufen
    /// </summary>
    Task<List<string>> GetAvailableModelsAsync(CancellationToken cancellationToken = default);
}

public class OllamaService : IOllamaService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;

    public OllamaService()
    {
        _baseUrl = "http://localhost:11434"; // Default Ollama port
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromMinutes(5) // LLM kann länger dauern
        };
    }

    public OllamaService(string baseUrl)
    {
        _baseUrl = baseUrl;
        _httpClient = new HttpClient
        {
            BaseAddress = new Uri(_baseUrl),
            Timeout = TimeSpan.FromMinutes(5)
        };
    }

    public async Task<string> ChatAsync(string prompt, string model = "llama2", CancellationToken cancellationToken = default)
    {
        try
        {
            var request = new OllamaRequest
            {
                Model = model,
                Prompt = prompt,
                Stream = false
            };

            var response = await _httpClient.PostAsJsonAsync("/api/generate", request, cancellationToken);
            response.EnsureSuccessStatusCode();

            var result = await response.Content.ReadFromJsonAsync<OllamaResponse>(cancellationToken: cancellationToken);
            return result?.Response ?? string.Empty;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[Ollama] Chat failed: {ex.Message}");
            return $"Error: {ex.Message}";
        }
    }

    public async Task<string> ChatWithHistoryAsync(List<OllamaMessage> messages, string model = "llama2", CancellationToken cancellationToken = default)
    {
        try
        {
            var request = new OllamaChatRequest
            {
                Model = model,
                Messages = messages,
                Stream = false
            };

            var response = await _httpClient.PostAsJsonAsync("/api/chat", request, cancellationToken);
            response.EnsureSuccessStatusCode();

            var result = await response.Content.ReadFromJsonAsync<OllamaChatResponse>(cancellationToken: cancellationToken);
            return result?.Message?.Content ?? string.Empty;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[Ollama] Chat with history failed: {ex.Message}");
            return $"Error: {ex.Message}";
        }
    }

    public async Task<string> SummarizeDocumentAsync(string documentText, CancellationToken cancellationToken = default)
    {
        var prompt = $@"Bitte fasse das folgende Dokument in 3-5 Sätzen zusammen:

{documentText}

Zusammenfassung:";

        return await ChatAsync(prompt, "llama2", cancellationToken);
    }

    public async Task<Dictionary<string, string>> ExtractMetadataAsync(string documentText, CancellationToken cancellationToken = default)
    {
        var prompt = $@"Extrahiere folgende Metadaten aus dem Dokument (Ausgabe als JSON):
- Dokumentart (type)
- Datum (date)
- Absender/Autor (sender)
- Aktenzeichen (fileReference)
- Betreff/Thema (subject)
- Schlagworte (keywords)

Dokument:
{documentText.Substring(0, Math.Min(2000, documentText.Length))}

JSON:";

        var jsonResponse = await ChatAsync(prompt, "llama2", cancellationToken);
        
        try
        {
            // Parse JSON-Response
            var metadata = JsonSerializer.Deserialize<Dictionary<string, string>>(jsonResponse);
            return metadata ?? new Dictionary<string, string>();
        }
        catch
        {
            // Fallback: Leere Metadaten
            return new Dictionary<string, string>();
        }
    }

    public async Task<List<string>> GetAvailableModelsAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/tags", cancellationToken);
            response.EnsureSuccessStatusCode();

            var result = await response.Content.ReadFromJsonAsync<OllamaModelsResponse>(cancellationToken: cancellationToken);
            
            var models = new List<string>();
            if (result?.Models != null)
            {
                foreach (var model in result.Models)
                {
                    models.Add(model.Name ?? "unknown");
                }
            }

            return models;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[Ollama] Failed to get models: {ex.Message}");
            return new List<string> { "llama2" }; // Fallback
        }
    }
}

#region DTOs

public class OllamaRequest
{
    public string Model { get; set; } = "llama2";
    public string Prompt { get; set; } = string.Empty;
    public bool Stream { get; set; } = false;
}

public class OllamaResponse
{
    public string? Model { get; set; }
    public string? Response { get; set; }
    public bool Done { get; set; }
}

public class OllamaChatRequest
{
    public string Model { get; set; } = "llama2";
    public List<OllamaMessage> Messages { get; set; } = new();
    public bool Stream { get; set; } = false;
}

public class OllamaChatResponse
{
    public string? Model { get; set; }
    public OllamaMessage? Message { get; set; }
    public bool Done { get; set; }
}

public class OllamaMessage
{
    public string Role { get; set; } = "user"; // user, assistant, system
    public string Content { get; set; } = string.Empty;
}

public class OllamaModelsResponse
{
    public List<OllamaModelInfo>? Models { get; set; }
}

public class OllamaModelInfo
{
    public string? Name { get; set; }
    public long Size { get; set; }
    public DateTime ModifiedAt { get; set; }
}

#endregion
