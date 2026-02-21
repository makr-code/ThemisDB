/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NodeImpact.cs                                      ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
