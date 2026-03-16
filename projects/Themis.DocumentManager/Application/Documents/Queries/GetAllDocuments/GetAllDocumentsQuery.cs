/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllDocumentsQuery.cs                            ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Documents.Messages;

namespace Themis.DocumentManager.Application.Documents.Queries.GetAllDocuments;

/// <summary>
/// Query to get all documents with pagination
/// </summary>
public record GetAllDocumentsQuery : IGetAllQuery<DocumentDto>
{
    public int PageNumber { get; init; } = 1;
    public int PageSize { get; init; } = 50;
    public string? SearchTerm { get; init; }
    public Dictionary<string, object>? Filters { get; init; }
}
