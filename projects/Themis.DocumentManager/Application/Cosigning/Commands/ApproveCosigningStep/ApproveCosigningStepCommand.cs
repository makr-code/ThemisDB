/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommand.cs                     ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Cosigning.Commands.ApproveCosigningStep;

/// <summary>
/// Command to approve a cosigning step (Mitzeichnung genehmigen)
/// Based on PDV VIS Mitzeichnung requirements
/// </summary>
public record ApproveCosigningStepCommand : IRequest<bool>
{
    public string CosigningId { get; init; } = string.Empty;
    public string CosignerId { get; init; } = string.Empty;
    public string? Comment { get; init; }
}
