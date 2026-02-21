/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DeleteDocumentCommandHandler.cs                    ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
