/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommandHandler.cs              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5de9cc853  2025-12-10  Integrate VIS features (Inbox, Reminders, Cosigning) into... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Cosigning.Commands.ApproveCosigningStep;

/// <summary>
/// Handler for ApproveCosigningStepCommand
/// Implements VIS Mitzeichnung functionality
/// </summary>
public class ApproveCosigningStepCommandHandler : IRequestHandler<ApproveCosigningStepCommand, bool>
{
    private readonly ICosigningService _cosigningService;

    public ApproveCosigningStepCommandHandler(ICosigningService cosigningService)
    {
        _cosigningService = cosigningService;
    }

    public async Task<bool> Handle(ApproveCosigningStepCommand request, CancellationToken cancellationToken)
    {
        return await _cosigningService.ApproveCosigningStepAsync(
            request.CosigningId,
            request.CosignerId,
            request.Comment ?? string.Empty);
    }
}
