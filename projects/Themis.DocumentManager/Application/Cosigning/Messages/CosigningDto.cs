/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CosigningDto.cs                                    ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Messages;

public record CosigningDto : BaseEntityDto
{
    public string DocumentId { get; init; } = string.Empty;
    public string DocumentName { get; init; } = string.Empty;
    public string SignerId { get; init; } = string.Empty;
    public string SignerName { get; init; } = string.Empty;
    public string? SignerEmail { get; init; }
    public CosigningStatus Status { get; init; }
    public int SignOrder { get; init; }
    public string? SignatureData { get; init; }
    public DateTime? SignedAt { get; init; }
    public DateTime? RequestedAt { get; init; }
    public DateTime? ReminderSentAt { get; init; }
    public string? Comment { get; init; }
    public string? RejectionReason { get; init; }
    public bool RequiresComment { get; init; }
    public CosigningType Type { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum CosigningStatus
{
    Pending,
    Signed,
    Rejected,
    Expired,
    Cancelled
}

public enum CosigningType
{
    Sequential,
    Parallel,
    Single
}
