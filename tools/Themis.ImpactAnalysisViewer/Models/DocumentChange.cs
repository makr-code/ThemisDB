/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentChange.cs                                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
