/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ImpactAnalysisResult.cs                            ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:33:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     69                                             ║
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
