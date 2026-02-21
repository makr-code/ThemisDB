/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDocumentsQueryHandler.cs                        ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     49                                             ║
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
