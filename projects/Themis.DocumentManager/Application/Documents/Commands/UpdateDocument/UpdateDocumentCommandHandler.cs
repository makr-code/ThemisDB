/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateDocumentCommandHandler.cs                    ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Domain.Events;

namespace Themis.DocumentManager.Application.Documents.Commands.UpdateDocument;

/// <summary>
/// Handler for UpdateDocumentCommand
/// </summary>
public class UpdateDocumentCommandHandler : IRequestHandler<UpdateDocumentCommand, Result>
{
    private readonly IThemisRepository _repository;
    private readonly IMediator _mediator;

    public UpdateDocumentCommandHandler(IThemisRepository repository, IMediator mediator)
    {
        _repository = repository;
        _mediator = mediator;
    }

    public async Task<Result> Handle(UpdateDocumentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var document = await _repository.GetDocumentAsync(request.Id, cancellationToken);
            if (document == null)
            {
                return Result.Fail($"Dokument mit ID '{request.Id}' wurde nicht gefunden");
            }

            // Update only provided fields
            if (request.Title != null) document.Title = request.Title;
            if (request.Description != null) document.Description = request.Description;
            if (request.Category != null) document.Category = request.Category;
            if (request.Classification != null) document.Classification = request.Classification;
            if (request.Metadata != null) document.Metadata = request.Metadata;
            if (request.Tags != null) document.Tags = request.Tags;
            
            document.ModifiedAt = DateTime.UtcNow;

            var success = await _repository.UpdateDocumentAsync(document, cancellationToken);
            if (!success)
            {
                return Result.Fail("Fehler beim Speichern des Dokuments");
            }

            // Publish domain event
            await _mediator.Publish(new DocumentUpdatedDomainEvent(document.Id, document.Title, DateTime.UtcNow), cancellationToken);

            return Result.Ok();
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Aktualisieren des Dokuments: {ex.Message}");
        }
    }
}
