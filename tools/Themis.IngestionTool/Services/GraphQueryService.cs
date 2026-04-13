/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GraphQueryService.cs                               ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     484                                            ║
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
    /// Interface for graph-based queries and relationship traversal.
    /// Supports path finding, relationship traversal, and community detection.
    /// </summary>
    public interface IGraphQueryService
    {
        /// <summary>
        /// Traverse relationships from a source entity to find related entities.
        /// </summary>
        Task<GraphTraversalResult> TraverseRelationshipsAsync(
            string sourceEntityId,
            int maxDepth = 3,
            string? relationshipType = null);

        /// <summary>
        /// Find shortest path between two entities.
        /// </summary>
        Task<PathFindingResult> FindPathAsync(
            string sourceEntityId,
            string targetEntityId,
            int maxDepth = 5);

        /// <summary>
        /// Get all entities within a certain relationship distance.
        /// </summary>
        Task<NeighborhoodResult> GetNeighborhoodAsync(
            string entityId,
            int distance = 2);

        /// <summary>
        /// Detect communities/clusters in the relationship graph.
        /// </summary>
        Task<CommunityDetectionResult> DetectCommunitiesAsync(int minCommunitySize = 3);

        /// <summary>
        /// Get relationship statistics for an entity.
        /// </summary>
        Task<RelationshipStatistics> GetRelationshipStatsAsync(string entityId);
    }

    public class GraphQueryService : IGraphQueryService
    {
        private readonly HttpClient _httpClient;
        private readonly ISettingsService _settingsService;
        private readonly ILoggerService _loggerService;
        private readonly IHttpResilienceService _resilienceService;

        public GraphQueryService(
            HttpClient httpClient,
            ISettingsService settingsService,
            ILoggerService loggerService,
            IHttpResilienceService resilienceService)
        {
            _httpClient = httpClient;
            _settingsService = settingsService;
            _loggerService = loggerService;
            _resilienceService = resilienceService;
        }

        public async Task<GraphTraversalResult> TraverseRelationshipsAsync(
            string sourceEntityId,
            int maxDepth = 3,
            string? relationshipType = null)
        {
            try
            {
                _loggerService.LogInfo($"Starting graph traversal from entity {sourceEntityId}, maxDepth={maxDepth}");

                var request = new GraphTraversalRequest
                {
                    SourceEntityId = sourceEntityId,
                    MaxDepth = maxDepth,
                    RelationshipType = relationshipType,
                    IncludeMetadata = true
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/graph/traverse";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<GraphTraversalResult>(content);
                    if (result == null)
                    {
                        return new GraphTraversalResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
                    }
                    _loggerService.LogInfo($"Graph traversal complete: {result.Entities.Count} entities found");
                    return result;
                }

                _loggerService.LogError($"Graph traversal failed: {response.StatusCode}");
                return new GraphTraversalResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during graph traversal: {ex.Message}");
                return new GraphTraversalResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
            }
        }

        public async Task<PathFindingResult> FindPathAsync(
            string sourceEntityId,
            string targetEntityId,
            int maxDepth = 5)
        {
            try
            {
                _loggerService.LogInfo($"Finding path from {sourceEntityId} to {targetEntityId}");

                var request = new PathFindingRequest
                {
                    SourceEntityId = sourceEntityId,
                    TargetEntityId = targetEntityId,
                    MaxDepth = maxDepth
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/graph/find-path";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<PathFindingResult>(content);
                    if (result == null)
                    {
                        return new PathFindingResult { PathFound = false, Path = new List<string>(), PathLength = 0 };
                    }
                    _loggerService.LogInfo($"Path found: {result.PathLength} hops, {(result.PathFound ? "Success" : "Not found")}");
                    return result;
                }

                _loggerService.LogError($"Path finding failed: {response.StatusCode}");
                return new PathFindingResult { PathFound = false, Path = new List<string>(), PathLength = 0 };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during path finding: {ex.Message}");
                return new PathFindingResult { PathFound = false, Path = new List<string>(), PathLength = 0 };
            }
        }

        public async Task<NeighborhoodResult> GetNeighborhoodAsync(string entityId, int distance = 2)
        {
            try
            {
                _loggerService.LogInfo($"Getting neighborhood for entity {entityId}, distance={distance}");

                var request = new NeighborhoodRequest
                {
                    EntityId = entityId,
                    Distance = distance
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/graph/neighborhood";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<NeighborhoodResult>(content);
                    if (result == null)
                    {
                        return new NeighborhoodResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
                    }
                    _loggerService.LogInfo($"Neighborhood retrieved: {result.Entities.Count} entities, {result.Relationships.Count} relationships");
                    return result;
                }

                _loggerService.LogError($"Neighborhood retrieval failed: {response.StatusCode}");
                return new NeighborhoodResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error getting neighborhood: {ex.Message}");
                return new NeighborhoodResult { Entities = new List<EntityNode>(), Relationships = new List<RelationshipEdge>() };
            }
        }

        public async Task<CommunityDetectionResult> DetectCommunitiesAsync(int minCommunitySize = 3)
        {
            try
            {
                _loggerService.LogInfo($"Starting community detection with minSize={minCommunitySize}");

                var request = new CommunityDetectionRequest
                {
                    MinCommunitySize = minCommunitySize,
                    Algorithm = "louvain"  // Default algorithm
                };

                var url = $"{_settingsService.GetThemisApiUrl()}/graph/detect-communities";
                var response = await _resilienceService.PostWithResilienceAsync(
                    url,
                    new StringContent(
                        JsonConvert.SerializeObject(request),
                        Encoding.UTF8,
                        "application/json"));

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<CommunityDetectionResult>(content);
                    if (result == null)
                    {
                        return new CommunityDetectionResult { Communities = new List<Community>(), Modularity = 0 };
                    }
                    _loggerService.LogInfo($"Community detection complete: {result.Communities.Count} communities found");
                    return result;
                }

                _loggerService.LogError($"Community detection failed: {response.StatusCode}");
                return new CommunityDetectionResult { Communities = new List<Community>(), Modularity = 0 };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error during community detection: {ex.Message}");
                return new CommunityDetectionResult { Communities = new List<Community>(), Modularity = 0 };
            }
        }

        public async Task<RelationshipStatistics> GetRelationshipStatsAsync(string entityId)
        {
            try
            {
                _loggerService.LogInfo($"Getting relationship statistics for entity {entityId}");

                var url = $"{_settingsService.GetThemisApiUrl()}/graph/entity/{entityId}/stats";
                var response = await _resilienceService.GetWithResilienceAsync(url);

                if (response.IsSuccessStatusCode)
                {
                    var content = await response.Content.ReadAsStringAsync();
                    var result = JsonConvert.DeserializeObject<RelationshipStatistics>(content);
                    if (result != null)
                    {
                        _loggerService.LogInfo($"Statistics retrieved: {result.InDegree} in-edges, {result.OutDegree} out-edges");
                        return result;
                    }
                }

                _loggerService.LogError($"Statistics retrieval failed: {response.StatusCode}");
                return new RelationshipStatistics { EntityId = entityId, InDegree = 0, OutDegree = 0 };
            }
            catch (Exception ex)
            {
                _loggerService.LogError($"Error getting statistics: {ex.Message}");
                return new RelationshipStatistics { EntityId = entityId, InDegree = 0, OutDegree = 0 };
            }
        }
    }

    #region DTOs

    public class GraphTraversalRequest
    {
        [JsonProperty("source_entity_id")]
        public string SourceEntityId { get; set; } = string.Empty;

        [JsonProperty("max_depth")]
        public int MaxDepth { get; set; }

        [JsonProperty("relationship_type")]
        public string? RelationshipType { get; set; }

        [JsonProperty("include_metadata")]
        public bool IncludeMetadata { get; set; }
    }

    public class GraphTraversalResult
    {
        [JsonProperty("entities")]
        public List<EntityNode> Entities { get; set; } = new List<EntityNode>();

        [JsonProperty("relationships")]
        public List<RelationshipEdge> Relationships { get; set; } = new List<RelationshipEdge>();

        [JsonProperty("traversal_depth")]
        public int TraversalDepth { get; set; }

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class EntityNode
    {
        [JsonProperty("entity_id")]
        public string EntityId { get; set; } = string.Empty;

        [JsonProperty("entity_type")]
        public string EntityType { get; set; } = string.Empty;

        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        [JsonProperty("depth")]
        public int Depth { get; set; }

        [JsonProperty("metadata")]
        public Dictionary<string, object> Metadata { get; set; } = new Dictionary<string, object>();
    }

    public class RelationshipEdge
    {
        [JsonProperty("source_id")]
        public string SourceId { get; set; } = string.Empty;

        [JsonProperty("target_id")]
        public string TargetId { get; set; } = string.Empty;

        [JsonProperty("relationship_type")]
        public string RelationshipType { get; set; } = string.Empty;

        [JsonProperty("weight")]
        public double Weight { get; set; }

        [JsonProperty("metadata")]
        public Dictionary<string, object> Metadata { get; set; } = new Dictionary<string, object>();
    }

    public class PathFindingRequest
    {
        [JsonProperty("source_entity_id")]
        public string SourceEntityId { get; set; } = string.Empty;

        [JsonProperty("target_entity_id")]
        public string TargetEntityId { get; set; } = string.Empty;

        [JsonProperty("max_depth")]
        public int MaxDepth { get; set; }
    }

    public class PathFindingResult
    {
        [JsonProperty("path_found")]
        public bool PathFound { get; set; }

        [JsonProperty("path")]
        public List<string> Path { get; set; } = new List<string>();

        [JsonProperty("path_length")]
        public int PathLength { get; set; }

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class NeighborhoodRequest
    {
        [JsonProperty("entity_id")]
        public string EntityId { get; set; } = string.Empty;

        [JsonProperty("distance")]
        public int Distance { get; set; }
    }

    public class NeighborhoodResult
    {
        [JsonProperty("center_entity_id")]
        public string CenterEntityId { get; set; } = string.Empty;

        [JsonProperty("entities")]
        public List<EntityNode> Entities { get; set; } = new List<EntityNode>();

        [JsonProperty("relationships")]
        public List<RelationshipEdge> Relationships { get; set; } = new List<RelationshipEdge>();

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class CommunityDetectionRequest
    {
        [JsonProperty("min_community_size")]
        public int MinCommunitySize { get; set; }

        [JsonProperty("algorithm")]
        public string Algorithm { get; set; } = "louvain";
    }

    public class CommunityDetectionResult
    {
        [JsonProperty("communities")]
        public List<Community> Communities { get; set; } = new List<Community>();

        [JsonProperty("modularity")]
        public double Modularity { get; set; }

        [JsonProperty("execution_time_ms")]
        public long ExecutionTimeMs { get; set; }
    }

    public class Community
    {
        [JsonProperty("community_id")]
        public int CommunityId { get; set; }

        [JsonProperty("entity_ids")]
        public List<string> EntityIds { get; set; } = new List<string>();

        [JsonProperty("size")]
        public int Size { get; set; }

        [JsonProperty("density")]
        public double Density { get; set; }

        [JsonProperty("internal_edges")]
        public int InternalEdges { get; set; }
    }

    public class RelationshipStatistics
    {
        [JsonProperty("entity_id")]
        public string EntityId { get; set; } = string.Empty;

        [JsonProperty("in_degree")]
        public int InDegree { get; set; }

        [JsonProperty("out_degree")]
        public int OutDegree { get; set; }

        [JsonProperty("total_degree")]
        public int TotalDegree => InDegree + OutDegree;

        [JsonProperty("relationship_types")]
        public Dictionary<string, int> RelationshipTypes { get; set; } = new Dictionary<string, int>();

        [JsonProperty("clustering_coefficient")]
        public double ClusteringCoefficient { get; set; }

        [JsonProperty("betweenness_centrality")]
        public double BetweennessCentrality { get; set; }
    }

    #endregion
}
