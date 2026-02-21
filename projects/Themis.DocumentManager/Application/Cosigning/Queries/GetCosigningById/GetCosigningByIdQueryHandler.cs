/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetCosigningByIdQueryHandler.cs                    ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
