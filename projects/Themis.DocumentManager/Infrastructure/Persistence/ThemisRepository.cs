/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisRepository.cs                                ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     133                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Infrastructure.Persistence;

/// <summary>
/// ThemisDB Repository implementation using ThemisApiClient
/// </summary>
public class ThemisRepository : IThemisRepository
{
    private readonly IThemisApiClient _apiClient;

    public ThemisRepository(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }

    public async Task<string> CreateDocumentAsync(Document document, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(document);

        var payload = new
        {
            blob = System.Text.Json.JsonSerializer.Serialize(document)
        };

        var response = await _apiClient.PutAsync<object, CreateDocumentResponse>(
            $"/entities/documents:{document.Id}",
            payload,
            cancellationToken);

        return response?.Id ?? document.Id;
    }

    public async Task<Document?> GetDocumentAsync(string id, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(id);

        var response = await _apiClient.GetAsync<EntityResponse>($"/entities/documents:{id}");
        
        if (response?.Blob == null)
            return null;

        return System.Text.Json.JsonSerializer.Deserialize<Document>(response.Blob);
    }

    public async Task<List<Document>> GetDocumentsAsync(int page, int pageSize, CancellationToken cancellationToken = default)
    {
        var query = new
        {
            table = "documents",
            @return = "entities"
        };

        var response = await _apiClient.PostAsync<object, QueryResponse>("/query", query);
        
        if (response?.Entities == null)
            return new List<Document>();

        var documents = new List<Document>();
        foreach (var entity in response.Entities)
        {
            try
            {
                var doc = System.Text.Json.JsonSerializer.Deserialize<Document>(entity);
                if (doc != null)
                    documents.Add(doc);
            }
            catch
            {
                // Skip invalid entities
            }
        }

        return documents;
    }

    public async Task<bool> UpdateDocumentAsync(Document document, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(document);

        var payload = new
        {
            blob = System.Text.Json.JsonSerializer.Serialize(document)
        };

        var response = await _apiClient.PutAsync<object, CreateDocumentResponse>(
            $"/entities/documents:{document.Id}",
            payload,
            cancellationToken);

        return response != null;
    }

    public async Task<bool> DeleteDocumentAsync(string id, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(id);

        return await _apiClient.DeleteAsync($"/entities/documents:{id}");
    }

    // Response DTOs
    private record CreateDocumentResponse(string Id);
    private record EntityResponse(string Blob);
    private record QueryResponse(List<string> Entities);
}
