/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetCosigningByIdQueryHandler.cs                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4b4dd1ff7  2025-12-29  Add performance benchmark results and update benchmarking... ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Queries.GetCosigningById;

public class GetCosigningByIdQueryHandler : IRequestHandler<GetCosigningByIdQuery, Result<CosigningDto>>
{
    private static readonly Dictionary<string, CreateCosigningCommandHandler.CosigningItem> _cosignings = CreateCosigningCommandHandler.Cosignings;

    public async Task<Result<CosigningDto>> Handle(GetCosigningByIdQuery request, CancellationToken cancellationToken)
    {
        if (!_cosignings.TryGetValue(request.Id, out var cosigning))
        {
            return await Task.FromResult(Result<CosigningDto>.Fail("Mitunterzeichnung nicht gefunden"));
        }

        var dto = new CosigningDto
        {
            Id = cosigning.Id ?? string.Empty,
            DocumentId = cosigning.DocumentId ?? string.Empty,
            DocumentName = cosigning.DocumentName ?? string.Empty,
            SignerId = cosigning.SignerId ?? string.Empty,
            SignerName = cosigning.SignerName ?? string.Empty,
            SignerEmail = cosigning.SignerEmail,
            Status = cosigning.Status,
            SignOrder = cosigning.SignOrder,
            SignatureData = cosigning.SignatureData,
            SignedAt = cosigning.SignedAt,
            RequestedAt = cosigning.RequestedAt,
            ReminderSentAt = cosigning.ReminderSentAt,
            Comment = cosigning.Comment,
            RejectionReason = cosigning.RejectionReason,
            RequiresComment = cosigning.RequiresComment,
            Type = cosigning.Type,
            Metadata = cosigning.Metadata ?? new(),
            CreatedAt = cosigning.CreatedAt,
            CreatedBy = cosigning.CreatedBy ?? "System",
            UpdatedAt = cosigning.UpdatedAt ?? cosigning.CreatedAt,
            UpdatedBy = cosigning.UpdatedBy ?? cosigning.CreatedBy ?? "System"
        };

        return await Task.FromResult(Result<CosigningDto>.Ok(dto));
    }
}
