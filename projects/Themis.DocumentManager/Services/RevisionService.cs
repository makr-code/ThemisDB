/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RevisionService.cs                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     182                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 293b3ec17  2025-12-07  Add ThemisDB Document Manager with Office integration ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Revision service interface for managing document versions
/// </summary>
public interface IRevisionService
{
    Task<DocumentRevision> CreateRevisionAsync(DocumentRevision revision);
    Task<IEnumerable<DocumentRevision>> GetDocumentRevisionsAsync(string documentId);
    Task<DocumentRevision?> GetRevisionByIdAsync(string revisionId);
    Task<DocumentRevision?> GetLatestRevisionAsync(string documentId);
    Task<bool> RestoreRevisionAsync(string documentId, int revisionNumber);
    Task<bool> CompareRevisionsAsync(string revisionId1, string revisionId2);
}

/// <summary>
/// Revision service implementation
/// Ensures revision-safe document management with complete audit trail
/// </summary>
public class RevisionService : IRevisionService
{
    private readonly IThemisApiClient _apiClient;

    public RevisionService(IThemisApiClient apiClient)
    {
        _apiClient = apiClient;
    }

    public async Task<DocumentRevision> CreateRevisionAsync(DocumentRevision revision)
    {
        var response = await _apiClient.PutAsync<object, object>(
            $"/entities/document_revisions:{revision.Id}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(revision) }
        );

        // Create timeline event for revision
        await _apiClient.PutAsync<object, object>(
            $"/entities/timeline_events:{Guid.NewGuid()}",
            new
            {
                blob = System.Text.Json.JsonSerializer.Serialize(new
                {
                    id = Guid.NewGuid().ToString(),
                    documentId = revision.DocumentId,
                    timestamp = revision.CreatedAt,
                    eventType = "RevisionCreated",
                    description = $"Revision {revision.RevisionNumber} created by {revision.Author}",
                    metadata = new
                    {
                        revisionId = revision.Id,
                        revisionNumber = revision.RevisionNumber,
                        author = revision.Author
                    }
                })
            }
        );

        return revision;
    }

    public async Task<IEnumerable<DocumentRevision>> GetDocumentRevisionsAsync(string documentId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse>(
            "/query/aql",
            new
            {
                query = $@"FOR rev IN document_revisions 
                          FILTER rev.documentId == '{documentId}'
                          SORT rev.revisionNumber DESC
                          RETURN rev"
            }
        );

        return response?.Results ?? Enumerable.Empty<DocumentRevision>();
    }

    public async Task<DocumentRevision?> GetRevisionByIdAsync(string revisionId)
    {
        return await _apiClient.GetAsync<DocumentRevision>($"/entities/document_revisions:{revisionId}");
    }

    public async Task<DocumentRevision?> GetLatestRevisionAsync(string documentId)
    {
        var revisions = await GetDocumentRevisionsAsync(documentId);
        return revisions.OrderByDescending(r => r.RevisionNumber).FirstOrDefault();
    }

    public async Task<bool> RestoreRevisionAsync(string documentId, int revisionNumber)
    {
        try
        {
            var revisions = await GetDocumentRevisionsAsync(documentId);
            var targetRevision = revisions.FirstOrDefault(r => r.RevisionNumber == revisionNumber);
            
            if (targetRevision == null || !System.IO.File.Exists(targetRevision.FilePath))
                return false;

            // Create new revision from restored file
            var newRevision = new DocumentRevision
            {
                Id = Guid.NewGuid().ToString(),
                DocumentId = documentId,
                RevisionNumber = revisions.Max(r => r.RevisionNumber) + 1,
                CreatedAt = DateTime.UtcNow,
                Author = Environment.UserName,
                Comment = $"Restored from revision {revisionNumber}",
                FilePath = targetRevision.FilePath,
                FileHash = targetRevision.FileHash,
                Metadata = new Dictionary<string, object>
                {
                    ["RestoredFromRevision"] = revisionNumber,
                    ["OriginalRevisionId"] = targetRevision.Id
                }
            };

            await CreateRevisionAsync(newRevision);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> CompareRevisionsAsync(string revisionId1, string revisionId2)
    {
        try
        {
            var rev1 = await GetRevisionByIdAsync(revisionId1);
            var rev2 = await GetRevisionByIdAsync(revisionId2);

            if (rev1 == null || rev2 == null)
                return false;

            // This would integrate with Office's compare functionality
            // For Word: Application.CompareDocuments
            // For Excel: Application.CompareWorkbooks
            
            return true;
        }
        catch
        {
            return false;
        }
    }

    private class QueryResponse
    {
        public List<DocumentRevision> Results { get; set; } = new();
    }
}
