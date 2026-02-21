/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllDocumentsQueryHandler.cs                     ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     126                                            ║
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

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Documents.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Queries.GetAllDocuments;

/// <summary>
/// Handler for GetAllDocumentsQuery
/// </summary>
public class GetAllDocumentsQueryHandler : IRequestHandler<GetAllDocumentsQuery, Result<PagedResult<DocumentDto>>>
{
    private readonly IThemisRepository _repository;

    public GetAllDocumentsQueryHandler(IThemisRepository repository)
    {
        _repository = repository;
    }

    public async Task<Result<PagedResult<DocumentDto>>> Handle(GetAllDocumentsQuery request, CancellationToken cancellationToken)
    {
        try
        {
            var documents = await _repository.GetDocumentsAsync(request.PageNumber, request.PageSize, cancellationToken);
            
            // Apply search filter
            var filtered = documents;
            if (!string.IsNullOrWhiteSpace(request.SearchTerm))
            {
                filtered = filtered.Where(d => 
                    d.Title.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase) ||
                    d.Description.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase)
                ).ToList();
            }

            // Apply additional filters
            if (request.Filters != null)
            {
                if (request.Filters.TryGetValue("Category", out var category))
                {
                    filtered = filtered.Where(d => d.Category == category?.ToString()).ToList();
                }
                if (request.Filters.TryGetValue("Classification", out var classification))
                {
                    filtered = filtered.Where(d => d.Classification == classification?.ToString()).ToList();
                }
            }

            var totalCount = filtered.Count;
            var dtos = filtered.Select(MapToDto).ToList();

            var pagedResult = new PagedResult<DocumentDto>
            {
                Items = dtos,
                TotalCount = totalCount,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return Result<PagedResult<DocumentDto>>.Ok(pagedResult);
        }
        catch (Exception ex)
        {
            return Result<PagedResult<DocumentDto>>.Fail($"Fehler beim Abrufen der Dokumente: {ex.Message}");
        }
    }

    private static DocumentDto MapToDto(Document doc)
    {
        return new DocumentDto
        {
            Id = doc.Id,
            Title = doc.Title,
            Description = doc.Description,
            MimeType = doc.MimeType,
            Filename = doc.Filename,
            SizeBytes = doc.SizeBytes,
            Author = doc.Author,
            Metadata = doc.Metadata,
            Tags = doc.Tags,
            Location = doc.Location != null ? new GeoLocationDto
            {
                Latitude = doc.Location.Latitude,
                Longitude = doc.Location.Longitude,
                Address = doc.Location.Address,
                Country = doc.Location.Country,
                City = doc.Location.City
            } : null,
            Classification = doc.Classification,
            Category = doc.Category,
            ContentPreview = doc.ContentPreview,
            BlobPath = doc.BlobPath,
            CreatedAt = doc.CreatedAt,
            CreatedBy = doc.Author,
            UpdatedAt = doc.ModifiedAt
        };
    }
}
