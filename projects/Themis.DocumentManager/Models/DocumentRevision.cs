/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentRevision.cs                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     41                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 293b3ec17  2025-12-07  Add ThemisDB Document Manager with Office integration ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Document revision for audit trail and version control
/// </summary>
public class DocumentRevision
{
    public string Id { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public int RevisionNumber { get; set; }
    public DateTime CreatedAt { get; set; }
    public string Author { get; set; } = string.Empty;
    public string Comment { get; set; } = string.Empty;
    public string FilePath { get; set; } = string.Empty;
    public string? FileHash { get; set; }
    public long FileSize { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}
