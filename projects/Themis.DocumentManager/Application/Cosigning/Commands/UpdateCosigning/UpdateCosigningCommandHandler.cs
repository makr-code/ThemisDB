/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommandHandler.cs                   ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Commands.UpdateCosigning;

public class UpdateCosigningCommandHandler : IRequestHandler<UpdateCosigningCommand, Result<CosigningDto>>
{
    private static readonly Dictionary<string, CreateCosigningCommandHandler.CosigningItem> _cosignings = CreateCosigningCommandHandler.Cosignings;

    public async Task<Result<CosigningDto>> Handle(UpdateCosigningCommand request, CancellationToken cancellationToken)
    {
        if (!_cosignings.TryGetValue(request.Id, out var cosigning))
        {
            return await Task.FromResult(Result<CosigningDto>.Fail("Mitunterzeichnung nicht gefunden"));
        }

        var now = DateTime.UtcNow;

        if (request.Status.HasValue)
        {
            cosigning.Status = request.Status.Value;
            if (request.Status == CosigningStatus.Signed)
            {
                cosigning.SignedAt = now;
            }
        }

        if (!string.IsNullOrWhiteSpace(request.SignatureData))
            cosigning.SignatureData = request.SignatureData;

        if (!string.IsNullOrWhiteSpace(request.Comment))
            cosigning.Comment = request.Comment;

        if (!string.IsNullOrWhiteSpace(request.RejectionReason))
            cosigning.RejectionReason = request.RejectionReason;

        if (request.SignOrder.HasValue)
            cosigning.SignOrder = request.SignOrder.Value;

        cosigning.UpdatedAt = now;
        cosigning.UpdatedBy = "System";

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
