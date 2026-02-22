/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommandHandler.cs              ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
