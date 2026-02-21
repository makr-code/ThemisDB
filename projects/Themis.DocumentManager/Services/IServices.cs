/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IServices.cs                                       ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     162                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// ThemisDB API client interface
/// </summary>
public interface IThemisApiClient
{
    Task<T?> GetAsync<T>(string endpoint, CancellationToken cancellationToken = default);
    Task<TResponse?> PostAsync<TRequest, TResponse>(string endpoint, TRequest data, CancellationToken cancellationToken = default);
    Task<TResponse?> PutAsync<TRequest, TResponse>(string endpoint, TRequest data, CancellationToken cancellationToken = default);
    Task<bool> DeleteAsync(string endpoint, CancellationToken cancellationToken = default);
    Task<List<T>> ExecuteAqlAsync<T>(string query, object? bindVars = null, CancellationToken cancellationToken = default);
    void SetAuthToken(string? token);
    Task<bool> CheckHealthAsync(CancellationToken cancellationToken = default);
}

/// <summary>
/// Document management service interface
/// </summary>
public interface IDocumentService
{
    Task<IEnumerable<Document>> GetAllDocumentsAsync();
    Task<Document?> GetDocumentAsync(string id);
    Task<Document?> GetDocumentByIdAsync(string id);
    Task<Document> CreateDocumentAsync(Document document);
    Task<Document> UpdateDocumentAsync(Document document);
    Task<bool> DeleteDocumentAsync(string id);
    Task<IEnumerable<DocumentChunk>> GetDocumentChunksAsync(string documentId);
}

/// <summary>
/// Search service interface
/// </summary>
public interface ISearchService
{
    Task<IEnumerable<SearchResult>> SearchAsync(string query, int limit = 10);
    Task<IEnumerable<SearchResult>> FullTextSearchAsync(string query, int limit = 10);
    Task<IEnumerable<SearchResult>> VectorSearchAsync(float[] queryVector, int limit = 10);
    Task<IEnumerable<SearchResult>> HybridSearchAsync(string query, float[] queryVector, int limit = 10);
    Task<IEnumerable<SearchResult>> FacetedSearchAsync(Dictionary<string, object> facets, int limit = 10);
}

/// <summary>
/// Metadata management service interface
/// </summary>
public interface IMetadataService
{
    Task<Dictionary<string, object>> GetMetadataAsync(string documentId);
    Task<bool> UpdateMetadataAsync(string documentId, Dictionary<string, object> metadata);
    Task<IEnumerable<string>> GetAllTagsAsync();
    Task<IEnumerable<string>> GetCategoriesAsync();
}

/// <summary>
/// Geo service interface
/// </summary>
public interface IGeoService
{
    Task<IEnumerable<Document>> GetDocumentsByLocationAsync(double latitude, double longitude, double radiusKm);
    Task<IEnumerable<Document>> GetDocumentsByRegionAsync(double minLat, double minLon, double maxLat, double maxLon);
    Task<bool> AddLocationToDocumentAsync(string documentId, GeoLocation location);
}

/// <summary>
/// Timeline service interface
/// </summary>
public interface ITimelineService
{
    Task<IEnumerable<TimelineEvent>> GetEventsAsync(DateTime startDate, DateTime endDate);
    Task<IEnumerable<TimelineEvent>> GetDocumentEventsAsync(string documentId);
    Task<TimelineEvent> CreateEventAsync(TimelineEvent timelineEvent, CancellationToken cancellationToken = default);
}

/// <summary>
/// Vector service interface
/// </summary>
public interface IVectorService
{
    Task<float[]?> GenerateEmbeddingAsync(string text);
    Task<IEnumerable<Document>> FindSimilarDocumentsAsync(string documentId, int limit = 10);
    Task<IEnumerable<DocumentChunk>> FindSimilarChunksAsync(float[] queryVector, int limit = 10);
}

/// <summary>
/// Graph service interface
/// </summary>
public interface IGraphService
{
    Task<IEnumerable<DocumentRelation>> GetDocumentRelationsAsync(string documentId);
    Task<DocumentRelation> CreateRelationAsync(DocumentRelation relation);
    Task<bool> DeleteRelationAsync(string relationId);
    Task<IEnumerable<Document>> TraverseGraphAsync(string startDocumentId, int maxDepth = 3);
    Task<IEnumerable<Document>> FindShortestPathAsync(string fromDocumentId, string toDocumentId);
}

/// <summary>
/// Lightweight client abstraction for direct database access (Arango style)
/// </summary>
public interface IThemisDbClient
{
    Task<IAsyncEnumerable<T>> QueryAsync<T>(string query, IDictionary<string, object>? bindVars = null, CancellationToken cancellationToken = default);
    Task ExecuteAsync(string query, IDictionary<string, object>? bindVars = null, CancellationToken cancellationToken = default);
    Task InsertAsync<T>(string collection, T document, CancellationToken cancellationToken = default);
}

/// <summary>
/// Higher-level DB service used by compliance/AI helpers
/// </summary>
public interface IThemisDBService
{
    Task<List<T>> ExecuteQueryAsync<T>(string query, object? bindVars = null, CancellationToken cancellationToken = default);
    Task ExecuteCommandAsync(string query, object? bindVars = null, CancellationToken cancellationToken = default);
    Task<IAsyncEnumerable<T>> QueryAsync<T>(string query, IDictionary<string, object>? bindVars = null, CancellationToken cancellationToken = default);
    Task ExecuteAsync(string query, IDictionary<string, object>? bindVars = null, CancellationToken cancellationToken = default);
}

/// <summary>
/// Placeholder process abstraction used by AI assistant
/// </summary>
public interface IProcessService
{
}

/// <summary>
/// Enhanced notification delivery (toast, email, sms, in-app)
/// </summary>
public interface IEnhancedNotificationService
{
    Task SendAsync(EnhancedNotification notification, CancellationToken cancellationToken = default);
}
