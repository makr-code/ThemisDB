/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCosigningCommandHandler.cs                   ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

public class CreateCosigningCommandHandler : IRequestHandler<CreateCosigningCommand, Result<CosigningDto>>
{
    internal static readonly Dictionary<string, CosigningItem> Cosignings = new();

    public async Task<Result<CosigningDto>> Handle(CreateCosigningCommand request, CancellationToken cancellationToken)
    {
        var id = Guid.NewGuid().ToString();
        var now = DateTime.UtcNow;

        var cosigning = new CosigningItem
        {
            Id = id,
            DocumentId = request.DocumentId,
            DocumentName = request.DocumentName,
            SignerId = request.SignerId,
            SignerName = request.SignerName,
            SignerEmail = request.SignerEmail,
            Status = CosigningStatus.Pending,
            SignOrder = request.SignOrder,
            RequestedAt = now,
            RequiresComment = request.RequiresComment,
            Type = request.Type,
            Metadata = request.Metadata,
            CreatedAt = now,
            CreatedBy = "System"
        };

        Cosignings[id] = cosigning;

        var dto = new CosigningDto
        {
            Id = cosigning.Id,
            DocumentId = cosigning.DocumentId,
            DocumentName = cosigning.DocumentName,
            SignerId = cosigning.SignerId,
            SignerName = cosigning.SignerName,
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
            Metadata = cosigning.Metadata,
            CreatedAt = cosigning.CreatedAt,
            CreatedBy = cosigning.CreatedBy,
            UpdatedAt = cosigning.UpdatedAt,
            UpdatedBy = cosigning.UpdatedBy
        };

        return await Task.FromResult(Result<CosigningDto>.Ok(dto));
    }

    internal class CosigningItem
    {
        public string Id { get; set; } = string.Empty;
        public string DocumentId { get; set; } = string.Empty;
        public string DocumentName { get; set; } = string.Empty;
        public string SignerId { get; set; } = string.Empty;
        public string SignerName { get; set; } = string.Empty;
        public string? SignerEmail { get; set; }
        public CosigningStatus Status { get; set; }
        public int SignOrder { get; set; }
        public string? SignatureData { get; set; }
        public DateTime? SignedAt { get; set; }
        public DateTime? RequestedAt { get; set; }
        public DateTime? ReminderSentAt { get; set; }
        public string? Comment { get; set; }
        public string? RejectionReason { get; set; }
        public bool RequiresComment { get; set; }
        public CosigningType Type { get; set; }
        public Dictionary<string, object> Metadata { get; set; } = new();
        public DateTime CreatedAt { get; set; }
        public string? CreatedBy { get; set; }
        public DateTime? UpdatedAt { get; set; }
        public string? UpdatedBy { get; set; }
    }
}
