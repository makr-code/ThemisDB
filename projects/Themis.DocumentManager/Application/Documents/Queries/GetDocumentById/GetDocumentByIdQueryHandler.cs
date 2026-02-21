/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDocumentByIdQueryHandler.cs                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Application.Documents.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Queries.GetDocumentById;

/// <summary>
/// Handler for GetDocumentByIdQuery
/// </summary>
public class GetDocumentByIdQueryHandler : IRequestHandler<GetDocumentByIdQuery, Result<DocumentDto>>
{
    private readonly IThemisRepository _repository;

    public GetDocumentByIdQueryHandler(IThemisRepository repository)
    {
        _repository = repository;
    }

    public async Task<Result<DocumentDto>> Handle(GetDocumentByIdQuery request, CancellationToken cancellationToken)
    {
        try
        {
            var document = await _repository.GetDocumentAsync(request.Id, cancellationToken);
            if (document == null)
            {
                return Result<DocumentDto>.Fail($"Dokument mit ID '{request.Id}' wurde nicht gefunden");
            }

            var dto = MapToDto(document);
            return Result<DocumentDto>.Ok(dto);
        }
        catch (Exception ex)
        {
            return Result<DocumentDto>.Fail($"Fehler beim Abrufen des Dokuments: {ex.Message}");
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
