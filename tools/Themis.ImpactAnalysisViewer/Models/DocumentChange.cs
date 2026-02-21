/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentChange.cs                                  ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     47                                             ║
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
