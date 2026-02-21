/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DeleteDocumentCommandHandler.cs                    ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Domain.Events;

namespace Themis.DocumentManager.Application.Documents.Commands.DeleteDocument;

/// <summary>
/// Handler for DeleteDocumentCommand
/// </summary>
public class DeleteDocumentCommandHandler : IRequestHandler<DeleteDocumentCommand, Result>
{
    private readonly IThemisRepository _repository;
    private readonly IMediator _mediator;

    public DeleteDocumentCommandHandler(IThemisRepository repository, IMediator mediator)
    {
        _repository = repository;
        _mediator = mediator;
    }

    public async Task<Result> Handle(DeleteDocumentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var document = await _repository.GetDocumentAsync(request.Id, cancellationToken);
            if (document == null)
            {
                return Result.Fail($"Dokument mit ID '{request.Id}' wurde nicht gefunden");
            }

            var success = await _repository.DeleteDocumentAsync(request.Id, cancellationToken);
            if (!success)
            {
                return Result.Fail("Fehler beim Löschen des Dokuments");
            }

            // Publish domain event
            await _mediator.Publish(new DocumentDeletedDomainEvent(request.Id, document.Title, DateTime.UtcNow), cancellationToken);

            return Result.Ok();
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Löschen des Dokuments: {ex.Message}");
        }
    }
}
