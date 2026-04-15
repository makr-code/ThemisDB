/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisApiService.cs                                ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:19:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     461                                            ║
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
    /// ThemisDB Multi-Model API Service
    /// Implementiert Entity, Graph, Vector, und TimeSeries Storage
    /// </summary>
    public class ThemisApiService : IThemisApiService
    {
        private readonly HttpClient _httpClient;
        private readonly string _baseUrl;
        private readonly ILoggerService _loggerService;
        private readonly IEmbeddingService _embeddingService;
        private readonly ICacheService _cacheService;
        private readonly ISettingsService _settingsService;

        // Request/Response DTOs
        private class EntityRequest
        {
            [JsonPropertyName("key")]
            public string Key { get; set; } = string.Empty;

            [JsonPropertyName("data")]
            public Dictionary<string, object> Data { get; set; } = new();
        }

        private class RelationshipRequest
        {
            [JsonPropertyName("from")]
            public string From { get; set; } = string.Empty;

            [JsonPropertyName("to")]
            public string To { get; set; } = string.Empty;

            [JsonPropertyName("type")]
            public string Type { get; set; } = string.Empty;

            [JsonPropertyName("properties")]
            public Dictionary<string, object> Properties { get; set; } = new();
        }

        private class VectorRequest
        {
            [JsonPropertyName("object_name")]
            public string ObjectName { get; set; } = string.Empty;

            [JsonPropertyName("key")]
            public string Key { get; set; } = string.Empty;

            [JsonPropertyName("vector")]
            public double[] Vector { get; set; } = Array.Empty<double>();

            [JsonPropertyName("metadata")]
            public Dictionary<string, object> Metadata { get; set; } = new();
        }

        private class TimeSeriesRequest
        {
            [JsonPropertyName("key")]
            public string Key { get; set; } = string.Empty;

            [JsonPropertyName("timestamp")]
            public long Timestamp { get; set; }

            [JsonPropertyName("value")]
            public double Value { get; set; }

            [JsonPropertyName("tags")]
            public Dictionary<string, string> Tags { get; set; } = new();
        }

        public ThemisApiService(
            ISettingsService settingsService, 
            ILoggerService loggerService,
            IEmbeddingService embeddingService,
            ICacheService cacheService)
        {
            _loggerService = loggerService;
            _embeddingService = embeddingService;
            _cacheService = cacheService;
            _settingsService = settingsService;
            var settings = settingsService.LoadSettings();
            
            _baseUrl = $"http://{settings.ThemisHost}:{settings.ThemisPort}";
            
            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(15),
                BaseAddress = new Uri(_baseUrl)
            };

            _loggerService.LogInfo($"ThemisApiService initialized: {_baseUrl}");
        }

        /// <summary>
        /// Speichert eine Datei-Entity mit Metadaten
        /// </summary>
        public async Task<bool> StoreEntityAsync(FileAnalysisResult result)
        {
            try
            {
                var key = $"file:{result.ContentHash}";
                var entityData = new Dictionary<string, object>
                {
                    { "filename", result.FileName },
                    { "filepath", result.FilePath },
                    { "filesize", result.FileSize },
                    { "filetype", result.FileType },
                    { "hash", result.ContentHash },
                    { "relevance_score", result.RelevanceScore },
                    { "impact_score", result.ImpactScore },
                    { "quality_score", result.QualityScore },
                    { "language", result.Language },
                    { "analysis_timestamp", result.AnalysisTimestamp.ToUniversalTime() },
                    { "processing_time_ms", (long)result.ProcessingTime.TotalMilliseconds },
                    { "graph_nodes", result.GraphNodeCount },
                    { "relationships", result.RelationshipCount }
                };

                // Optional: Add extracted data
                if (result.Summary != null)
                    entityData["summary"] = result.Summary;

                if (result.Keywords?.Any() == true)
                    entityData["keywords"] = result.Keywords;

                if (result.Topics?.Any() == true)
                    entityData["topics"] = result.Topics;

                if (result.ExtractedEntities?.Any() == true)
                    entityData["entities"] = result.ExtractedEntities;

                var request = new EntityRequest
                {
                    Key = key,
                    Data = entityData
                };

                var response = await PostAsync("/entities", request);
                
                if (response.IsSuccessStatusCode)
                {
                    _loggerService.LogInfo($"Entity stored: {key}");
                    return true;
                }
                else
                {
                    _loggerService.LogWarning($"Failed to store entity: {key} - {response.StatusCode}");
                    return false;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"StoreEntityAsync failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Erstellt Graph-Beziehungen zwischen Dateien
        /// </summary>
        public async Task<bool> CreateRelationshipAsync(string fromKey, string toKey, string relationshipType, Dictionary<string, object>? properties = null)
        {
            try
            {
                var request = new RelationshipRequest
                {
                    From = fromKey,
                    To = toKey,
                    Type = relationshipType,
                    Properties = properties ?? new Dictionary<string, object>()
                };

                var response = await PostAsync("/graph/relationship", request);
                
                if (response.IsSuccessStatusCode)
                {
                    _loggerService.LogInfo($"Relationship created: {fromKey} -[{relationshipType}]-> {toKey}");
                    return true;
                }
                else
                {
                    _loggerService.LogWarning($"Failed to create relationship: {response.StatusCode}");
                    return false;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"CreateRelationshipAsync failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Speichert Vector-Embeddings für Semantic Search
        /// </summary>
        public async Task<bool> StoreVectorAsync(string key, double[] embedding, Dictionary<string, object> metadata)
        {
            try
            {
                var request = new VectorRequest
                {
                    ObjectName = "documents",
                    Key = key,
                    Vector = embedding,
                    Metadata = metadata
                };

                var response = await PostAsync("/vector/store", request);

                if (response.IsSuccessStatusCode)
                {
                    _loggerService.LogInfo($"Vector stored: {key}");
                    return true;
                }
                else
                {
                    _loggerService.LogWarning($"Failed to store vector: {key} - {response.StatusCode}");
                    return false;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"StoreVectorAsync failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Speichert TimeSeries-Daten für Performance-Tracking
        /// </summary>
        public async Task<bool> StoreTimeSeriesAsync(string key, double value, DateTime timestamp, Dictionary<string, string>? tags = null)
        {
            try
            {
                var request = new TimeSeriesRequest
                {
                    Key = key,
                    Timestamp = new DateTimeOffset(timestamp.ToUniversalTime()).ToUnixTimeSeconds(),
                    Value = value,
                    Tags = tags ?? new Dictionary<string, string>()
                };

                var response = await PostAsync("/ts/put", request);

                if (response.IsSuccessStatusCode)
                {
                    _loggerService.LogInfo($"TimeSeries stored: {key}");
                    return true;
                }
                else
                {
                    _loggerService.LogWarning($"Failed to store timeseries: {response.StatusCode}");
                    return false;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"StoreTimeSeriesAsync failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Führt eine Batch-Transaction aus für Atomic Operations
        /// </summary>
        public async Task<bool> ExecuteTransactionAsync(List<FileAnalysisResult> results, IProgress<string>? progress = null)
        {
            try
            {
                var transactionOps = new List<object>();

                foreach (var result in results)
                {
                    progress?.Report($"Preparing transaction for: {result.FileName}");

                    var key = $"file:{result.ContentHash}";

                    // Entity operation
                    transactionOps.Add(new
                    {
                        op = "entity",
                        key = key,
                        data = CreateEntityData(result)
                    });

                    // Graph relationships
                    if (result.ExtractedEntities?.Any() == true)
                    {
                        foreach (var entity in result.ExtractedEntities.Take(5))
                        {
                            transactionOps.Add(new
                            {
                                op = "relationship",
                                from = key,
                                to = $"entity:{entity}",
                                type = "CONTAINS",
                                properties = new { source = "analysis" }
                            });
                        }
                    }

                    // TimeSeries for metrics
                    transactionOps.Add(new
                    {
                        op = "timeseries",
                        key = $"metric:relevance:{key}",
                        timestamp = result.AnalysisTimestamp.ToUniversalTime(),
                        value = result.RelevanceScore,
                        tags = new { file = result.FileName, hash = result.ContentHash }
                    });
                }

                var transactionRequest = new { operations = transactionOps };
                var response = await PostAsync("/transaction", transactionRequest);

                if (response.IsSuccessStatusCode)
                {
                    _loggerService.LogInfo($"Transaction executed: {results.Count} operations");
                    return true;
                }
                else
                {
                    _loggerService.LogWarning($"Transaction failed: {response.StatusCode}");
                    return false;
                }
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"ExecuteTransactionAsync failed: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Generiert Embeddings für Vector Search mit Cache-Optimierung
        /// </summary>
        public double[] GenerateEmbedding(string text, int dimension = 1536)
        {
            if (string.IsNullOrWhiteSpace(text))
                return new double[dimension];

            // Prüfe Cache zuerst
            if (_cacheService.TryGetEmbedding(text, out var cachedEmbedding))
            {
                _loggerService.LogInfo($"Embedding cache hit for text (length={text.Length})");
                return cachedEmbedding ?? new double[dimension];
            }

            // Generiere echtes Embedding via Ollama/HuggingFace
            var embeddingTask = _embeddingService.GenerateEmbeddingAsync(text);
            embeddingTask.Wait(); // Synchron für diesen Use-Case

            var embedding = embeddingTask.Result;
            if (embedding != null && embedding.Length > 0)
            {
                // Speichere im Cache
                _cacheService.SetEmbedding(text, embedding);
                _loggerService.LogInfo($"Generated and cached embedding (dim={embedding.Length})");
                return embedding;
            }

            // Fallback: Deterministic hash-based embedding wenn Service nicht verfügbar
            _loggerService.LogWarning("Embedding service unavailable, using fallback hash-based embedding");
            return GenerateFallbackEmbedding(text, dimension);
        }

        private double[] GenerateFallbackEmbedding(string text, int dimension)
        {
            // Fallback: Einfache Hash-basierte Embeddings
            var random = new Random(text.GetHashCode());
            return Enumerable.Range(0, dimension)
                .Select(_ => random.NextDouble() * 2 - 1)
                .ToArray();
        }

        private Dictionary<string, object> CreateEntityData(FileAnalysisResult result)
        {
            return new Dictionary<string, object>
            {
                { "filename", result.FileName },
                { "filepath", result.FilePath },
                { "filesize", result.FileSize },
                { "filetype", result.FileType },
                { "hash", result.ContentHash },
                { "relevance_score", result.RelevanceScore },
                { "impact_score", result.ImpactScore },
                { "quality_score", result.QualityScore },
                { "language", result.Language },
                { "graph_nodes", result.GraphNodeCount },
                { "relationships", result.RelationshipCount }
            };
        }

        private async Task<HttpResponseMessage> PostAsync<T>(string endpoint, T data)
        {
            try
            {
                var jsonContent = new StringContent(
                    JsonSerializer.Serialize(data),
                    System.Text.Encoding.UTF8,
                    "application/json");

                return await _httpClient.PostAsync(endpoint, jsonContent);
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"PostAsync failed: {ex.Message}");
                throw;
            }
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
        }
    }

    /// <summary>
    /// Interface für ThemisDB API Service
    /// </summary>
    public interface IThemisApiService
    {
        Task<bool> StoreEntityAsync(FileAnalysisResult result);
        Task<bool> CreateRelationshipAsync(string fromKey, string toKey, string relationshipType, Dictionary<string, object>? properties = null);
        Task<bool> StoreVectorAsync(string key, double[] embedding, Dictionary<string, object> metadata);
        Task<bool> StoreTimeSeriesAsync(string key, double value, DateTime timestamp, Dictionary<string, string>? tags = null);
        Task<bool> ExecuteTransactionAsync(List<FileAnalysisResult> results, IProgress<string>? progress = null);
        double[] GenerateEmbedding(string text, int dimension = 1536);
    }
}
