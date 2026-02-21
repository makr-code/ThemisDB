/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateDocumentCommandHandler.cs                    ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     121                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Application.Documents.Messages;
using Themis.DocumentManager.Domain.Events;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Commands.CreateDocument;

/// <summary>
/// Handler for CreateDocumentCommand
/// </summary>
public class CreateDocumentCommandHandler : IRequestHandler<CreateDocumentCommand, Result<DocumentDto>>
{
    private readonly IThemisRepository _repository;
    private readonly IMediator _mediator;

    public CreateDocumentCommandHandler(IThemisRepository repository, IMediator mediator)
    {
        _repository = repository;
        _mediator = mediator;
    }

    public async Task<Result<DocumentDto>> Handle(CreateDocumentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var document = new Document
            {
                Id = Guid.NewGuid().ToString(),
                Title = request.Title,
                Description = request.Description,
                MimeType = request.MimeType,
                Filename = request.Filename,
                SizeBytes = request.SizeBytes,
                Author = request.Author,
                ContentPreview = request.ContentPreview,
                Category = request.Category,
                Classification = request.Classification,
                BlobPath = request.BlobPath,
                CreatedAt = DateTime.UtcNow,
                ModifiedAt = DateTime.UtcNow,
                Metadata = request.Metadata ?? new Dictionary<string, object>(),
                Tags = request.Tags ?? new List<string>(),
                Location = request.Location != null ? new GeoLocation
                {
                    Latitude = request.Location.Latitude,
                    Longitude = request.Location.Longitude,
                    Address = request.Location.Address,
                    Country = request.Location.Country,
                    City = request.Location.City
                } : null
            };

            var documentId = await _repository.CreateDocumentAsync(document, cancellationToken);

            // Update ID with returned documentId
            document.Id = documentId;

            // Publish domain event
            await _mediator.Publish(new DocumentCreatedEvent(documentId, document.Title, document.CreatedAt), cancellationToken);

            var dto = MapToDto(document, request.Author);
            return Result<DocumentDto>.Ok(dto);
        }
        catch (Exception ex)
        {
            return Result<DocumentDto>.Fail($"Fehler beim Erstellen des Dokuments: {ex.Message}");
        }
    }

    private static DocumentDto MapToDto(Document doc, string createdBy)
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
            CreatedBy = createdBy,
            UpdatedAt = doc.ModifiedAt
        };
    }
}
