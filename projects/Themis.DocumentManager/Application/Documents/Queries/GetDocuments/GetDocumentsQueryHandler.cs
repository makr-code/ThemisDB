/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDocumentsQueryHandler.cs                        ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     42                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
