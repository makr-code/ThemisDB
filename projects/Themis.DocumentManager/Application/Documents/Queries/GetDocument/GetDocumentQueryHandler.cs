/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDocumentQueryHandler.cs                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     42                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Queries.GetDocument;

/// <summary>
/// Handler for GetDocumentQuery
/// </summary>
public class GetDocumentQueryHandler : IRequestHandler<GetDocumentQuery, Document?>
{
    private readonly IThemisRepository _repository;

    public GetDocumentQueryHandler(IThemisRepository repository)
    {
        _repository = repository;
    }

    public async Task<Document?> Handle(GetDocumentQuery request, CancellationToken cancellationToken)
    {
        return await _repository.GetDocumentAsync(request.DocumentId, cancellationToken);
    }
}
