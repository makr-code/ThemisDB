/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommand.cs                          ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:55:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Commands.UpdateCosigning;

public record UpdateCosigningCommand : IRequest<Result<CosigningDto>>
{
    public string Id { get; init; } = string.Empty;
    public CosigningStatus? Status { get; init; }
    public string? SignatureData { get; init; }
    public string? Comment { get; init; }
    public string? RejectionReason { get; init; }
    public int? SignOrder { get; init; }
}
