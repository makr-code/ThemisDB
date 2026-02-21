/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ImpactAnalysisResult.cs                            ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     73                                             ║
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
/// Represents the result of an impact analysis operation
/// </summary>
public class ImpactAnalysisResult
{
    public string AnalysisId { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public DocumentChange SourceChange { get; set; } = new();
    
    // Overall statistics
    public int TotalAffectedCount { get; set; }
    public double MaxImpactScore { get; set; }
    public double AverageImpactScore { get; set; }
    public int TraversalDepth { get; set; }
    
    // Node and edge data
    public List<NodeImpact> AffectedNodes { get; set; } = new();
    public List<ImpactEdge> Edges { get; set; } = new();
    
    // Layer-specific statistics
    public Dictionary<string, int> AffectedNodesPerLayer { get; set; } = new();
    public Dictionary<string, double> MaxImpactPerLayer { get; set; } = new();
    public int CrossLayerTransitions { get; set; }
    public List<string> LayersInvolved { get; set; } = new();
    
    // Performance metrics
    public long ComputationTimeMs { get; set; }
    public long MemoryUsedBytes { get; set; }
    public bool UsedGpuAcceleration { get; set; }
    public bool UsedHybridSearch { get; set; }
}

/// <summary>
/// Represents an edge in the impact graph
/// </summary>
public class ImpactEdge
{
    public string From { get; set; } = string.Empty;
    public string To { get; set; } = string.Empty;
    public string EdgeType { get; set; } = string.Empty;
    public double Weight { get; set; }
    public bool IsCrossLayer { get; set; }
    public string SourceLayer { get; set; } = string.Empty;
    public string TargetLayer { get; set; } = string.Empty;
}
