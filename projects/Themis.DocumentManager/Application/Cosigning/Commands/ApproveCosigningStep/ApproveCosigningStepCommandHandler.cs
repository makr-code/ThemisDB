/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommandHandler.cs              ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     52                                             ║
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
