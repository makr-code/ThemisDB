/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NodeImpact.cs                                      ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     54                                             ║
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
/// Represents a node affected by an impact analysis
/// </summary>
public class NodeImpact
{
    public string NodeId { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public double ImpactScore { get; set; }
    public int PropagationDepth { get; set; }
    public string NodeType { get; set; } = string.Empty;
    
    // Layer information
    public string Layer { get; set; } = string.Empty;
    public LayerMetadata LayerMetadata { get; set; } = new();
    public List<string> CrossedLayers { get; set; } = new();
    public bool IsCrossLayerImpact { get; set; }
    
    // Visualization properties
    public double X { get; set; }  // 2D/3D position
    public double Y { get; set; }
    public double Z { get; set; }
    public double Size { get; set; } = 1.0;
    public string Color { get; set; } = "#808080";
    
    // Additional metadata
    public Dictionary<string, object> Metadata { get; set; } = new();
    public bool IsSourceNode { get; set; }
    public bool IsHighImpact { get; set; }
    public double Criticality { get; set; }
}
