/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationDto.cs                               ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Classification.Messages;

public record ClassificationDto : BaseEntityDto
{
    public string Name { get; init; } = string.Empty;
    public string? Description { get; init; }
    public string? Code { get; init; }
    public ClassificationLevel Level { get; init; }
    public string? Color { get; init; }
    public int SortOrder { get; init; }
    public bool IsActive { get; init; }
    public string? ParentId { get; init; }
    public List<string> AllowedRoles { get; init; } = new();
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum ClassificationLevel
{
    Public,
    Internal,
    Confidential,
    Secret,
    TopSecret
}
