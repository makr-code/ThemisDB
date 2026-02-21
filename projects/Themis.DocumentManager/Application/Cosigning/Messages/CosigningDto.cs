/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CosigningDto.cs                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
