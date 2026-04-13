/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            VectorQueryService.cs                              ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     468                                            ║
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
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Themis.IngestionTool.Models;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Interface for semantic/vector-based similarity search.
    /// Supports k-NN search, distance-based queries, and semantic clustering.
    /// </summary>
    public interface IVectorQueryService
    {
        /// <summary>
        /// Search for semantically similar entities using embedding vectors.
        /// </summary>
        Task<VectorSearchResult> SearchSimilarAsync(
            string query,
            int topK = 10,
            double distanceThreshold = 0.7);

        /// <summary>
        /// Search using pre-computed embedding vector.
        /// </summary>
        Task<VectorSearchResult> SearchByEmbeddingAsync(
            double[] embedding,
            int topK = 10,
            double distanceThreshold = 0.7);

        /// <summary>
        /// Find entities in a geographic/vector space radius.
        /// </summary>
        Task<VectorRadiusResult> SearchByRadiusAsync(
            double[] centerPoint,
            double radius,
            int maxResults = 100);

        /// <summary>
        /// Compute similarity between two entities.
        /// </summary>
        Task<SimilarityResult> ComputeSimilarityAsync(
            string entityId1,
            string entityId2);

        /// <summary>
        /// Get vector statistics (dimensionality, distribution, etc).
        /// </summary>
        Task<VectorStatistics> GetVectorStatsAsync();

        /// <summary>
        /// Batch search for multiple queries.
        /// </summary>
        Task<List<VectorSearchResult>> SearchBatchAsync(
            List<string> queries,
            int topK = 10);
    }

    public class VectorQueryService : IVectorQueryService
    {
        private readonly HttpClient _httpClient;
        private readonly ISettingsService _settingsService;
        private readonly ILoggerService _loggerService;
        private readonly IHttpResilienceService _resilienceService;
        private readonly ICacheService _cacheService;
        private readonly IEmbeddingService _embeddingService;

        public VectorQueryService(
            HttpClient httpClient,
            ISettingsService settingsService,
            ILoggerService loggerService,
            IHttpResilienceService resilienceService,
            ICacheService cacheService,
            IEmbeddingService embeddingService)
        {
            _httpClient = httpClient;
            _settingsService = settingsService;
            _loggerService = loggerService;
            _resilienceService = resilienceService;
            _cacheService = cacheService;
            _embeddingService = embeddingService;
        }

        public async Task<VectorSearchResult> SearchSimilarAsync(
            string query,
            int topK = 10,
            double distanceThreshold = 0.7)
        {
            try
            {
                _loggerService.LogInfo($"Vector search for query: '{query}' (topK={topK})");

                // Generate embedding for query
                var queryEmbedding = await _embeddingService.GenerateEmbeddingAsync(query);
                if (queryEmbedding == null)
                {
                    _loggerService.LogError("Failed to generate embedding for query");
                    return new VectorSearchResult { Results = new List<VectorSearchMatch>() };
                }

                return await SearchByEmbeddingAsync(queryEmbedding, topK, distanceThreshold);
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during vector search: {ex.Message}");
                return new VectorSearchResult { Results = new List<VectorSearchMatch>() };
            }
        }

        public async Task<VectorSearchResult> SearchByEmbeddingAsync(
            double[] embedding,
            int topK = 10,
            double distanceThreshold = 0.7)
        {
            try
            {
                _loggerService.LogInfo($"Vector search by embedding (topK={topK}, threshold={distanceThreshold})");

                var request = new VectorSearchRequest
                {
                    Embedding = embedding,
                    TopK = topK,
                    DistanceThreshold = distanceThreshold,
                    Metric = "cosine"  // Cosine similarity
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/vector/search";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<VectorSearchResult>(content);
                    if (result == null)
                    {
                        return new VectorSearchResult { Results = new List<VectorSearchMatch>() };
                    }
                    _loggerService.LogInfo($"Vector search returned {result.Results.Count} results");
                    return result;
                }

                _loggerService.LogError($"Vector search failed: {response.StatusCode}");
                return new VectorSearchResult { Results = new List<VectorSearchMatch>() };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during vector search by embedding: {ex.Message}");
                return new VectorSearchResult { Results = new List<VectorSearchMatch>() };
            }
        }

        public async Task<VectorRadiusResult> SearchByRadiusAsync(
            double[] centerPoint,
            double radius,
            int maxResults = 100)
        {
            try
            {
                _loggerService.LogInfo($"Vector radius search (radius={radius}, maxResults={maxResults})");

                var request = new VectorRadiusRequest
                {
                    CenterPoint = centerPoint,
                    Radius = radius,
                    MaxResults = maxResults,
                    Metric = "cosine"
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/vector/radius-search";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<VectorRadiusResult>(content);
                    if (result == null)
                    {
                        return new VectorRadiusResult { Matches = new List<VectorSearchMatch>() };
                    }
                    _loggerService.LogInfo($"Radius search returned {result.Matches.Count} matches");
                    return result;
                }

                _loggerService.LogError($"Radius search failed: {response.StatusCode}");
                return new VectorRadiusResult { Matches = new List<VectorSearchMatch>() };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during radius search: {ex.Message}");
                return new VectorRadiusResult { Matches = new List<VectorSearchMatch>() };
            }
        }

        public async Task<SimilarityResult> ComputeSimilarityAsync(
            string entityId1,
            string entityId2)
        {
            try
            {
                _loggerService.LogInfo($"Computing similarity between {entityId1} and {entityId2}");

                var request = new SimilarityRequest
                {
                    EntityId1 = entityId1,
                    EntityId2 = entityId2,
                    Metric = "cosine"
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/vector/similarity";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<SimilarityResult>(content);
                    if (result == null)
                    {
                        return new SimilarityResult { SimilarityScore = 0 };
                    }
                    _loggerService.LogInfo($"Similarity score: {result.SimilarityScore:F4}");
                    return result;
                }

                _loggerService.LogError($"Similarity computation failed: {response.StatusCode}");
                return new SimilarityResult { SimilarityScore = 0 };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error computing similarity: {ex.Message}");
                return new SimilarityResult { SimilarityScore = 0 };
            }
        }

        public async Task<VectorStatistics> GetVectorStatsAsync()
        {
            try
            {
                _loggerService.LogInfo("Fetching vector statistics");

                var url = $"{_settingsService.GetThemisApiUrl()}/vector/stats";
                var response = await _resilienceService.GetWithResilienceAsync(url);

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<VectorStatistics>(content);
                    if (result != null)
                    {
                        _loggerService.LogInfo($"Vector stats: {result.TotalVectors} vectors, dimension {result.VectorDimension}");
                        return result;
                    }
                }

                _loggerService.LogError($"Vector stats retrieval failed: {response.StatusCode}");
                return new VectorStatistics();
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error fetching vector stats: {ex.Message}");
                return new VectorStatistics();
            }
        }

        public async Task<List<VectorSearchResult>> SearchBatchAsync(
            List<string> queries,
            int topK = 10)
        {
            try
            {
                _loggerService.LogInfo($"Batch vector search for {queries.Count} queries");

                var results = new List<VectorSearchResult>();
                var chunkedQueries = queries
                    .Select((q, i) => new { query = q, index = i })
                    .GroupBy(x => x.index / 5)  // Group by 5s
                    .ToList();

                foreach (var chunk in chunkedQueries)
                {
                    var tasks = chunk.Select(q => SearchSimilarAsync(q.query, topK)).ToList();
                    var chunkResults = await Task.WhenAll(tasks);
                    results.AddRange(chunkResults);
                }

                _loggerService.LogInfo($"Batch search completed: {results.Count} result sets");
                return results;
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during batch search: {ex.Message}");
                return new List<VectorSearchResult>();
            }
        }
    }

    #region DTOs

    public class VectorSearchRequest
    {
        [JsonProperty("embedding")]
        public double[] Embedding { get; set; } = Array.Empty<double>();

        [JsonProperty("top_k")]
        public int TopK { get; set; }

        [JsonProperty("distance_threshold")]
        public double DistanceThreshold { get; set; }

        [JsonProperty("metric")]
        public string Metric { get; set; } = "cosine";  // cosine, euclidean, manhattan
    }

    public class VectorSearchResult
    {
        [JsonProperty("query_embedding_dimension")]
        public int QueryEmbeddingDimension { get; set; }

        [JsonProperty("results")]
        public List<VectorSearchMatch> Results { get; set; } = new List<VectorSearchMatch>();

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class VectorSearchMatch
    {
        [JsonProperty("entity_id")]
        public string EntityId { get; set; } = string.Empty;

        [JsonProperty("entity_type")]
        public string EntityType { get; set; } = string.Empty;

        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        [JsonProperty("similarity_score")]
        public double SimilarityScore { get; set; }

        [JsonProperty("distance")]
        public double Distance { get; set; }

        [JsonProperty("rank")]
        public int Rank { get; set; }

        [JsonProperty("metadata")]
        public Dictionary<string, object> Metadata { get; set; } = new Dictionary<string, object>();
    }

    public class VectorRadiusRequest
    {
        [JsonProperty("center_point")]
        public double[] CenterPoint { get; set; } = Array.Empty<double>();

        [JsonProperty("radius")]
        public double Radius { get; set; }

        [JsonProperty("max_results")]
        public int MaxResults { get; set; }

        [JsonProperty("metric")]
        public string Metric { get; set; } = "cosine";
    }

    public class VectorRadiusResult
    {
        [JsonProperty("center_point_dimension")]
        public int CenterPointDimension { get; set; }

        [JsonProperty("radius")]
        public double Radius { get; set; }

        [JsonProperty("matches")]
        public List<VectorSearchMatch> Matches { get; set; } = new List<VectorSearchMatch>();

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class SimilarityRequest
    {
        [JsonProperty("entity_id_1")]
        public string EntityId1 { get; set; } = string.Empty;

        [JsonProperty("entity_id_2")]
        public string EntityId2 { get; set; } = string.Empty;

        [JsonProperty("metric")]
        public string Metric { get; set; } = "cosine";
    }

    public class SimilarityResult
    {
        [JsonProperty("entity_id_1")]
        public string EntityId1 { get; set; } = string.Empty;

        [JsonProperty("entity_id_2")]
        public string EntityId2 { get; set; } = string.Empty;

        [JsonProperty("similarity_score")]
        public double SimilarityScore { get; set; }

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class VectorStatistics
    {
        [JsonProperty("total_vectors")]
        public long TotalVectors { get; set; }

        [JsonProperty("vector_dimension")]
        public int VectorDimension { get; set; }

        [JsonProperty("index_type")]
        public string IndexType { get; set; } = "HNSW";

        [JsonProperty("avg_query_time_ms")]
        public double AvgQueryTimeMs { get; set; }

        [JsonProperty("index_size_mb")]
        public double IndexSizeMb { get; set; }

        [JsonProperty("last_updated")]
        public DateTime LastUpdated { get; set; }
    }

    #endregion
}
