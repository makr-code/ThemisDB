/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentChange.cs                                  ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.ImpactAnalysisViewer.Models;

/// <summary>
/// Represents a document change that triggers impact analysis
/// </summary>
public class DocumentChange
{
    public string DocumentId { get; set; } = string.Empty;
    public string ChangeType { get; set; } = string.Empty;
    public double Magnitude { get; set; }
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    
    // Layer information
    public string SourceLayer { get; set; } = string.Empty;
    public LayerMetadata LayerMetadata { get; set; } = new();
    
    // Change details
    public Dictionary<string, object> ChangeDetails { get; set; } = new();
    public string Description { get; set; } = string.Empty;
    public string Author { get; set; } = string.Empty;
}
