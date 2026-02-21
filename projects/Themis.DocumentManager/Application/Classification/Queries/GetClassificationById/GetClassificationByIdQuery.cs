/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetClassificationByIdQuery.cs                      ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     36                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Classification.Messages;

namespace Themis.DocumentManager.Application.Classification.Queries.GetClassificationById;

public record GetClassificationByIdQuery : IGetByIdQuery<ClassificationDto>
{
    public string Id { get; init; } = string.Empty;
}
