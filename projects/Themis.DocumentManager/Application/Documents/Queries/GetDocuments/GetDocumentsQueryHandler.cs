/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDocumentsQueryHandler.cs                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 77d774278  2025-12-10  Phase 1 Sprint 1: Clean Architecture Foundation mit CQRS ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Queries.GetDocuments;

/// <summary>
/// Handler for GetDocumentsQuery
/// </summary>
public class GetDocumentsQueryHandler : IRequestHandler<GetDocumentsQuery, List<Document>>
{
    private readonly IThemisRepository _repository;

    public GetDocumentsQueryHandler(IThemisRepository repository)
    {
        _repository = repository;
    }

    public async Task<List<Document>> Handle(GetDocumentsQuery request, CancellationToken cancellationToken)
    {
        return await _repository.GetDocumentsAsync(request.Page, request.PageSize, cancellationToken);
    }
}
