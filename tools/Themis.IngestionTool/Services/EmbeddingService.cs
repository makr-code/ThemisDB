/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            EmbeddingService.cs                                ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 19:10:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     332                                            ║
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

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Real Embedding Service mit Ollama Integration
    /// Ersetzt Placeholder-Embeddings mit echten LLM-Embeddings
    /// </summary>
    public interface IEmbeddingService
    {
        Task<double[]?> GenerateEmbeddingAsync(string text);
        Task<List<double[]>> GenerateEmbeddingsBatchAsync(List<string> texts);
        Task<bool> IsAvailableAsync();
        int EmbeddingDimension { get; }
    }

    public class OllamaEmbeddingService : IEmbeddingService
    {
        private readonly HttpClient _httpClient;
        private readonly string _baseUrl;
        private readonly string _model;
        private readonly ILoggerService _loggerService;
        private bool _isAvailable;
        private int _cachedDimension = -1;

        // Ollama Embedding Request/Response
        private class OllamaEmbeddingRequest
        {
            [JsonPropertyName("model")]
            public string Model { get; set; } = string.Empty;

            [JsonPropertyName("prompt")]
            public string Prompt { get; set; } = string.Empty;
        }

        private class OllamaEmbeddingResponse
        {
            [JsonPropertyName("embedding")]
            public double[] Embedding { get; set; } = Array.Empty<double>();

            [JsonPropertyName("model")]
            public string Model { get; set; } = string.Empty;
        }

        public int EmbeddingDimension => _cachedDimension > 0 ? _cachedDimension : 384; // nomic-embed-text default

        public OllamaEmbeddingService(ISettingsService settingsService, ILoggerService loggerService)
        {
            _loggerService = loggerService;
            var settings = settingsService.LoadSettings();

            // Ollama Server Konfiguration
            var ollamaHost = settings.OllamaHost ?? "localhost";
            var ollamaPort = settings.OllamaPort > 0 ? settings.OllamaPort : 11434;
            _baseUrl = $"http://{ollamaHost}:{ollamaPort}";
            _model = settings.EmbeddingModel ?? "nomic-embed-text"; // Empfehlungen: nomic-embed-text, all-minilm

            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(30),
                BaseAddress = new Uri(_baseUrl)
            };

            _loggerService.LogInfo($"OllamaEmbeddingService initialized: {_baseUrl}, model: {_model}");
        }

        public async Task<bool> IsAvailableAsync()
        {
            if (_isAvailable && _cachedDimension > 0)
                return true;

            try
            {
                // Test mit kleinem Embedding
                var testEmbedding = await GenerateEmbeddingAsync("test");
                _isAvailable = testEmbedding != null && testEmbedding.Length > 0;

                if (_isAvailable && _cachedDimension <= 0)
                {
                    _cachedDimension = testEmbedding!.Length;
                    _loggerService.LogInfo($"Embedding dimension detected: {_cachedDimension}");
                }

                if (_isAvailable)
                    _loggerService.LogInfo("OllamaEmbeddingService: Embedding service is available");
                else
                    _loggerService.LogWarning("OllamaEmbeddingService: Embedding service test failed");
            }
            catch (Exception ex)
            {
                _isAvailable = false;
                _loggerService.LogWarning($"OllamaEmbeddingService availability check failed: {ex.Message}");
            }

            return _isAvailable;
        }

        public async Task<double[]?> GenerateEmbeddingAsync(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return null;

            try
            {
                // Text normalisieren
                var normalizedText = text.Length > 512 ? text.Substring(0, 512) : text;

                var request = new OllamaEmbeddingRequest
                {
                    Model = _model,
                    Prompt = normalizedText
                };

                var jsonContent = new StringContent(
                    JsonSerializer.Serialize(request),
                    System.Text.Encoding.UTF8,
                    "application/json");

                using (var response = await _httpClient.PostAsync("/api/embeddings", jsonContent))
                {
                    if (!response.IsSuccessStatusCode)
                    {
                        _loggerService.LogWarning($"Embedding failed: {response.StatusCode}");
                        return null;
                    }

                    var responseContent = await response.Content.ReadAsStringAsync();
                    var ollamaResponse = JsonSerializer.Deserialize<OllamaEmbeddingResponse>(responseContent);

                    if (ollamaResponse?.Embedding != null && ollamaResponse.Embedding.Length > 0)
                    {
                        _cachedDimension = ollamaResponse.Embedding.Length;
                        return ollamaResponse.Embedding;
                    }
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"GenerateEmbeddingAsync failed: {ex.Message}");
            }

            return null;
        }

        /// <summary>
        /// Batch-Generierung von Embeddings (Performance-Optimierung)
        /// </summary>
        public async Task<List<double[]>> GenerateEmbeddingsBatchAsync(List<string> texts)
        {
            var results = new List<double[]>();

            if (texts == null || texts.Count == 0)
                return results;

            try
            {
                // Batch in Chunks verarbeiten (max 10 parallel)
                var chunks = texts
                    .Select((text, idx) => new { text, idx })
                    .GroupBy(x => x.idx / 10)
                    .Select(g => g.Select(x => x.text).ToList())
                    .ToList();

                foreach (var chunk in chunks)
                {
                    var tasks = chunk.Select(text => GenerateEmbeddingAsync(text)).ToList();
                    var embeddings = await Task.WhenAll(tasks);

                    foreach (var embedding in embeddings)
                    {
                        if (embedding != null)
                            results.Add(embedding);
                    }
                }

                _loggerService.LogInfo($"Batch embedding generated: {results.Count}/{texts.Count}");
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"GenerateEmbeddingsBatchAsync failed: {ex.Message}");
            }

            return results;
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }

    /// <summary>
    /// Alternative: HuggingFace Inference API Integration
    /// Für Cloud-basierte Embeddings ohne lokalen Ollama Server
    /// </summary>
    public class HuggingFaceEmbeddingService : IEmbeddingService
    {
        private readonly HttpClient _httpClient;
        private readonly string _apiKey;
        private readonly string _model;
        private readonly ILoggerService _loggerService;
        private bool _isAvailable;

        private class HFRequest
        {
            [JsonPropertyName("inputs")]
            public string Inputs { get; set; } = string.Empty;
        }

        public int EmbeddingDimension => 384; // all-MiniLM-L6-v2 default

        public HuggingFaceEmbeddingService(ISettingsService settingsService, ILoggerService loggerService)
        {
            _loggerService = loggerService;
            var settings = settingsService.LoadSettings();

            _apiKey = settings.HuggingFaceApiKey ?? "";
            _model = settings.EmbeddingModel ?? "sentence-transformers/all-MiniLM-L6-v2";

            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(30),
                DefaultRequestHeaders =
                {
                    { "Authorization", $"Bearer {_apiKey}" }
                }
            };

            _loggerService.LogInfo($"HuggingFaceEmbeddingService initialized: {_model}");
        }

        public async Task<bool> IsAvailableAsync()
        {
            if (string.IsNullOrEmpty(_apiKey))
            {
                _loggerService.LogWarning("HuggingFaceEmbeddingService: API key not configured");
                return false;
            }

            try
            {
                var testEmbedding = await GenerateEmbeddingAsync("test");
                _isAvailable = testEmbedding != null;
            }
            catch (Exception ex)
            {
                _loggerService.LogWarning($"HuggingFaceEmbeddingService availability check failed: {ex.Message}");
            }

            return _isAvailable;
        }

        public async Task<double[]?> GenerateEmbeddingAsync(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return null;

            try
            {
                var request = new HFRequest { Inputs = text };
                var jsonContent = new StringContent(
                    JsonSerializer.Serialize(request),
                    System.Text.Encoding.UTF8,
                    "application/json");

                var url = $"https://api-inference.huggingface.co/models/{_model}";
                using (var response = await _httpClient.PostAsync(url, jsonContent))
                {
                    if (!response.IsSuccessStatusCode)
                        return null;

                    var responseContent = await response.Content.ReadAsStringAsync();
                    var embeddings = JsonSerializer.Deserialize<double[][]>(responseContent);
                    return embeddings?[0];
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"GenerateEmbeddingAsync failed: {ex.Message}");
            }

            return null;
        }

        public async Task<List<double[]>> GenerateEmbeddingsBatchAsync(List<string> texts)
        {
            var results = new List<double[]>();

            foreach (var text in texts)
            {
                var embedding = await GenerateEmbeddingAsync(text);
                if (embedding != null)
                    results.Add(embedding);
            }

            return results;
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }
}
