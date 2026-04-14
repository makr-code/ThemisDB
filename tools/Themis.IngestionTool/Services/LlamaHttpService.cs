/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlamaHttpService.cs                                ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// HTTP-basierter LLM Service für echte llama.cpp Integration
    /// Replaces simulation with actual LLM calls via HTTP
    /// </summary>
    public class LlamaHttpService : ILlamaService
    {
        private readonly HttpClient _httpClient;
        private readonly string _endpoint;
        private readonly string _model;
        private readonly int _maxTokens;
        private readonly double _temperature;
        private readonly ILoggerService _loggerService;
        private bool _isAvailable;

        // llama.cpp HTTP Request/Response DTOs
        private class LlamaRequest
        {
            [JsonPropertyName("prompt")]
            public string Prompt { get; set; } = string.Empty;

            [JsonPropertyName("n_predict")]
            public int MaxTokens { get; set; }

            [JsonPropertyName("temperature")]
            public double Temperature { get; set; }

            [JsonPropertyName("top_p")]
            public double TopP { get; set; } = 0.9;

            [JsonPropertyName("top_k")]
            public int TopK { get; set; } = 40;

            [JsonPropertyName("stop")]
            public List<string> StopSequences { get; set; } = new() { "\n\n" };
        }

        private class LlamaResponse
        {
            [JsonPropertyName("content")]
            public string Content { get; set; } = string.Empty;

            [JsonPropertyName("stop")]
            public bool Stop { get; set; }

            [JsonPropertyName("tokens_predicted")]
            public int TokensPredicted { get; set; }

            [JsonPropertyName("model")]
            public string Model { get; set; } = string.Empty;
        }

        public LlamaHttpService(ISettingsService settingsService, ILoggerService loggerService)
        {
            _loggerService = loggerService;
            
            // Konfiguration aus Settings
            var settings = settingsService.LoadSettings();
            _endpoint = !string.IsNullOrEmpty(settings.LlamaEndpoint) ? settings.LlamaEndpoint : "http://localhost:11434/api/generate";
            _model = !string.IsNullOrEmpty(settings.LlamaModel) ? settings.LlamaModel : "llama2";
            _maxTokens = settings.LlamaMaxTokens > 0 ? settings.LlamaMaxTokens : 200;
            _temperature = settings.LlamaTemperature > 0 ? settings.LlamaTemperature : 0.7;

            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(30),
                BaseAddress = new Uri(_endpoint.Substring(0, _endpoint.LastIndexOf('/')))
            };

            _loggerService.LogInfo($"LlamaHttpService initialized: {_endpoint}");
        }

        public async Task<bool> IsAvailableAsync()
        {
            if (_isAvailable)
                return true;

            try
            {
                // Test LLM availability with minimal prompt
                var testPrompt = new LlamaRequest
                {
                    Prompt = "Test",
                    MaxTokens = 10,
                    Temperature = 0.1
                };

                using (var content = new StringContent(
                    JsonSerializer.Serialize(testPrompt), 
                    System.Text.Encoding.UTF8, 
                    "application/json"))
                {
                    using (var response = await _httpClient.PostAsync("/api/generate", content))
                    {
                        _isAvailable = response.IsSuccessStatusCode;
                        if (_isAvailable)
                            _loggerService.LogInfo("LlamaHttpService: LLM is available");
                        else
                            _loggerService.LogWarning($"LlamaHttpService: LLM not available - {response.StatusCode}");
                    }
                }
            }
            catch (Exception ex)
            {
                _isAvailable = false;
                _loggerService.LogWarning($"LlamaHttpService: Availability check failed - {ex.Message}");
            }

            return _isAvailable;
        }

        public async Task<string> GenerateSummaryAsync(string content)
        {
            if (!await IsAvailableAsync())
                return string.Empty;

            try
            {
                // Truncate content if too long
                var truncatedContent = TruncateContent(content, maxLength: 2000);

                var prompt = $@"Erstelle eine kurze, prägnante Zusammenfassung (max 100 Wörter) des folgenden Codes oder Dokumentation:

{truncatedContent}

Zusammenfassung:";

                var result = await CallLlamaAsync(prompt, maxTokens: 150);
                return result.Trim();
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"GenerateSummaryAsync failed: {ex.Message}");
                return string.Empty;
            }
        }

        public async Task<List<string>> ExtractKeywordsAsync(string content)
        {
            if (!await IsAvailableAsync())
                return new List<string>();

            try
            {
                var truncatedContent = TruncateContent(content, maxLength: 1000);

                var prompt = $@"Extrahiere die 5-10 wichtigsten Keywords aus diesem Text als komma-separierte Liste:

{truncatedContent}

Keywords:";

                var result = await CallLlamaAsync(prompt, maxTokens: 100);
                
                return result
                    .Split(',')
                    .Select(k => k.Trim())
                    .Where(k => !string.IsNullOrEmpty(k))
                    .Distinct()
                    .ToList();
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"ExtractKeywordsAsync failed: {ex.Message}");
                return new List<string>();
            }
        }

        public async Task<List<string>> ExtractEntitiesAsync(string content)
        {
            if (!await IsAvailableAsync())
                return new List<string>();

            try
            {
                var truncatedContent = TruncateContent(content, maxLength: 1500);

                var prompt = $@"Extrahiere Named Entities (Klassen, Funktionen, Variablen, APIs) als komma-separierte Liste:

{truncatedContent}

Entities:";

                var result = await CallLlamaAsync(prompt, maxTokens: 150);

                return result
                    .Split(',')
                    .Select(e => e.Trim())
                    .Where(e => !string.IsNullOrEmpty(e) && e.Length > 2)
                    .Distinct()
                    .ToList();
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"ExtractEntitiesAsync failed: {ex.Message}");
                return new List<string>();
            }
        }

        public async Task<double> CalculateRelevanceScoreAsync(string content)
        {
            if (!await IsAvailableAsync())
                return 0.5; // Default score

            try
            {
                var truncatedContent = TruncateContent(content, maxLength: 1500);

                var prompt = $@"Bewerte die Relevanz dieses Code/Dokument für ein Entwicklungsprojekt auf einer Skala von 0.0 bis 1.0:
- 0.0-0.3: Nicht relevant (Config, Konstanten)
- 0.3-0.6: Bedingt relevant (Utils, Tests)
- 0.6-0.8: Relevant (Core-Features)
- 0.8-1.0: Sehr relevant (Kritische Komponenten)

{truncatedContent}

Gib nur die numerische Score als Dezimalzahl zurück (z.B. 0.75):";

                var result = await CallLlamaAsync(prompt, maxTokens: 20);
                
                if (double.TryParse(result.Trim(), out var score))
                    return Math.Clamp(score, 0.0, 1.0);

                return 0.5;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"CalculateRelevanceScoreAsync failed: {ex.Message}");
                return 0.5;
            }
        }

        private async Task<string> CallLlamaAsync(string prompt, int maxTokens)
        {
            try
            {
                var request = new LlamaRequest
                {
                    Prompt = prompt,
                    MaxTokens = maxTokens,
                    Temperature = _temperature
                };

                var jsonContent = new StringContent(
                    JsonSerializer.Serialize(request),
                    System.Text.Encoding.UTF8,
                    "application/json");

                using (var response = await _httpClient.PostAsync("/api/generate", jsonContent))
                {
                    if (!response.IsSuccessStatusCode)
                    {
                        _loggerService.LogWarning($"LLM call failed: {response.StatusCode}");
                        return string.Empty;
                    }

                    var responseContent = await response.Content.ReadAsStringAsync();
                    var llamaResponse = JsonSerializer.Deserialize<LlamaResponse>(responseContent);

                    return llamaResponse?.Content ?? string.Empty;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"CallLlamaAsync failed: {ex.Message}");
                return string.Empty;
            }
        }

        private string TruncateContent(string content, int maxLength)
        {
            if (content.Length <= maxLength)
                return content;

            return content.Substring(0, maxLength) + "...";
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }
}
