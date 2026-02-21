/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NetworkBottleneckAnalyzer.cs                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     459                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Services.Network;

/// <summary>
/// Network Bottleneck Analyzer - Identifies capacity constraints and critical paths
/// Sprint 1, US-1.2: Bottleneck Analysis
/// </summary>
public class NetworkBottleneckAnalyzer
{
    private readonly RailwayNetworkAnalyzer _network;
    
    public NetworkBottleneckAnalyzer(RailwayNetworkAnalyzer network)
    {
        _network = network ?? throw new ArgumentNullException(nameof(network));
    }
    
    /// <summary>
    /// Find all bottlenecks in the network (capacity > 85%)
    /// </summary>
    public List<NetworkBottleneck> FindBottlenecks(double utilizationThreshold = 0.85)
    {
        var bottlenecks = new List<NetworkBottleneck>();
        
        foreach (var station in _network.GetAllStations())
        {
            var edges = _network.GetConnectedEdges(station.Id).ToList();
            
            foreach (var edge in edges)
            {
                var utilization = CalculateUtilization(edge);
                
                if (utilization >= utilizationThreshold)
                {
                    bottlenecks.Add(new NetworkBottleneck
                    {
                        EdgeId = edge.Id,
                        FromStation = edge.From,
                        ToStation = edge.To,
                        CurrentUtilization = utilization,
                        DailyTrains = edge.DailyTrains,
                        MaxCapacity = edge.MaxCapacityTrainsPerHour * 24,
                        CapacityReserve = (edge.MaxCapacityTrainsPerHour * 24) - edge.DailyTrains,
                        Severity = ClassifySeverity(utilization),
                        Type = DetermineBottleneckType(edge, utilization)
                    });
                }
            }
        }
        
        return bottlenecks.OrderByDescending(b => b.CurrentUtilization).ToList();
    }
    
    /// <summary>
    /// Find critical paths (single point of failure - no alternative routes)
    /// </summary>
    public List<CriticalPath> FindCriticalPaths()
    {
        var criticalPaths = new List<CriticalPath>();
        var stations = _network.GetAllStations().ToList();
        
        foreach (var station in stations)
        {
            var neighbors = _network.GetNeighbors(station.Id).ToList();
            
            // If station has only one connection, it's a potential critical path
            if (neighbors.Count == 1)
            {
                var edge = _network.GetConnectedEdges(station.Id).First();
                
                criticalPaths.Add(new CriticalPath
                {
                    StationId = station.Id,
                    StationName = station.Name,
                    SingleConnectionTo = neighbors[0].Name,
                    EdgeId = edge.Id,
                    PassengersAffected = station.PassengersPerDay,
                    AlternativeRoutesAvailable = false,
                    RiskLevel = station.PassengersPerDay > 100000 ? RiskLevel.Critical : RiskLevel.High
                });
            }
        }
        
        // Also find bridges in graph (edges whose removal disconnects the graph)
        criticalPaths.AddRange(FindBridgeEdges());
        
        return criticalPaths.OrderByDescending(cp => cp.PassengersAffected).ToList();
    }
    
    /// <summary>
    /// Calculate capacity reserves for all edges
    /// </summary>
    public Dictionary<string, CapacityReserve> CalculateCapacityReserves()
    {
        var reserves = new Dictionary<string, CapacityReserve>();
        
        foreach (var station in _network.GetAllStations())
        {
            foreach (var edge in _network.GetConnectedEdges(station.Id))
            {
                if (reserves.ContainsKey(edge.Id))
                    continue;
                
                var maxDaily = edge.MaxCapacityTrainsPerHour * 24;
                var reserve = maxDaily - edge.DailyTrains;
                var reservePercent = maxDaily > 0 ? (double)reserve / maxDaily * 100 : 0;
                
                reserves[edge.Id] = new CapacityReserve
                {
                    EdgeId = edge.Id,
                    Route = $"{edge.From?.Name} → {edge.To?.Name}",
                    CurrentTrainsPerDay = edge.DailyTrains,
                    MaxCapacityPerDay = maxDaily,
                    ReserveTrains = reserve,
                    ReservePercent = reservePercent,
                    Status = reservePercent > 30 ? CapacityStatus.Good :
                            reservePercent > 15 ? CapacityStatus.Warning :
                            CapacityStatus.Critical
                };
            }
        }
        
        return reserves;
    }
    
    /// <summary>
    /// Generate bottleneck report with recommendations
    /// </summary>
    public BottleneckReport GenerateReport()
    {
        var bottlenecks = FindBottlenecks();
        var criticalPaths = FindCriticalPaths();
        var reserves = CalculateCapacityReserves();
        
        var report = new BottleneckReport
        {
            GeneratedAt = DateTime.Now,
            TotalBottlenecks = bottlenecks.Count,
            CriticalBottlenecks = bottlenecks.Count(b => b.Severity == BottleneckSeverity.Critical),
            HighBottlenecks = bottlenecks.Count(b => b.Severity == BottleneckSeverity.High),
            MediumBottlenecks = bottlenecks.Count(b => b.Severity == BottleneckSeverity.Medium),
            
            Bottlenecks = bottlenecks,
            CriticalPaths = criticalPaths,
            CapacityReserves = reserves.Values.ToList(),
            
            Recommendations = GenerateRecommendations(bottlenecks, criticalPaths)
        };
        
        return report;
    }
    
    /// <summary>
    /// Calculate edge utilization (0.0 - 1.0)
    /// </summary>
    private double CalculateUtilization(RailwayEdge edge)
    {
        if (edge.MaxCapacityTrainsPerHour == 0)
            return 0;
        
        var maxDaily = edge.MaxCapacityTrainsPerHour * 24;
        return maxDaily > 0 ? (double)edge.DailyTrains / maxDaily : 0;
    }
    
    /// <summary>
    /// Classify bottleneck severity
    /// </summary>
    private BottleneckSeverity ClassifySeverity(double utilization)
    {
        if (utilization >= 0.95)
            return BottleneckSeverity.Critical;
        if (utilization >= 0.90)
            return BottleneckSeverity.High;
        if (utilization >= 0.85)
            return BottleneckSeverity.Medium;
        return BottleneckSeverity.Low;
    }
    
    /// <summary>
    /// Determine bottleneck type based on infrastructure
    /// </summary>
    private BottleneckType DetermineBottleneckType(RailwayEdge edge, double utilization)
    {
        if (edge.TrackCount == 1)
            return BottleneckType.SingleTrack;
        
        if (!edge.HasETCS && edge.MaxSpeedKmh > 160)
            return BottleneckType.SignalingLimited;
        
        if (edge.IsHighSpeed && utilization > 0.90)
            return BottleneckType.HighSpeedCapacity;
        
        return BottleneckType.GeneralCapacity;
    }
    
    /// <summary>
    /// Find bridge edges (critical connections)
    /// Uses DFS to detect edges whose removal disconnects the graph
    /// </summary>
    private List<CriticalPath> FindBridgeEdges()
    {
        var bridges = new List<CriticalPath>();
        var visited = new HashSet<Station>();
        var discoveryTime = new Dictionary<Station, int>();
        var lowTime = new Dictionary<Station, int>();
        var parent = new Dictionary<Station, Station?>();
        var time = 0;
        
        void DFS(Station u)
        {
            visited.Add(u);
            discoveryTime[u] = lowTime[u] = ++time;
            
            foreach (var v in _network.GetNeighbors(u.Id))
            {
                if (!visited.Contains(v))
                {
                    parent[v] = u;
                    DFS(v);
                    
                    lowTime[u] = Math.Min(lowTime[u], lowTime[v]);
                    
                    // If low[v] > disc[u], then u-v is a bridge
                    if (lowTime[v] > discoveryTime[u])
                    {
                        var edge = _network.GetConnectedEdges(u.Id)
                            .FirstOrDefault(e => e.To.Id == v.Id);
                        
                        if (edge != null)
                        {
                            bridges.Add(new CriticalPath
                            {
                                StationId = u.Id,
                                StationName = u.Name,
                                SingleConnectionTo = v.Name,
                                EdgeId = edge.Id,
                                PassengersAffected = u.PassengersPerDay + v.PassengersPerDay,
                                AlternativeRoutesAvailable = false,
                                RiskLevel = RiskLevel.Critical,
                                IsBridgeEdge = true
                            });
                        }
                    }
                }
                else if (parent.GetValueOrDefault(u) != v)
                {
                    lowTime[u] = Math.Min(lowTime[u], discoveryTime[v]);
                }
            }
        }
        
        // Run DFS from first station
        var firstStation = _network.GetAllStations().FirstOrDefault();
        if (firstStation != null)
        {
            DFS(firstStation);
        }
        
        return bridges;
    }
    
    /// <summary>
    /// Generate recommendations based on analysis
    /// </summary>
    private List<string> GenerateRecommendations(List<NetworkBottleneck> bottlenecks, List<CriticalPath> criticalPaths)
    {
        var recommendations = new List<string>();
        
        // Critical bottlenecks
        var critical = bottlenecks.Where(b => b.Severity == BottleneckSeverity.Critical).ToList();
        if (critical.Any())
        {
            recommendations.Add($"🔴 KRITISCH: {critical.Count} Streckenabschnitte mit >95% Auslastung gefunden");
            foreach (var b in critical.Take(3))
            {
                recommendations.Add($"   → {b.FromStation?.Name} - {b.ToStation?.Name}: {b.CurrentUtilization:P1} Auslastung");
                
                if (b.Type == BottleneckType.SingleTrack)
                    recommendations.Add($"      Empfehlung: Zweigleisiger Ausbau (+50% Kapazität)");
                else if (b.Type == BottleneckType.SignalingLimited)
                    recommendations.Add($"      Empfehlung: ETCS Level 2 Installation (+20% Kapazität)");
                else
                    recommendations.Add($"      Empfehlung: Zusätzliche Überholgleise oder Paralleltrassierung");
            }
        }
        
        // Critical paths
        if (criticalPaths.Any())
        {
            var highRisk = criticalPaths.Where(cp => cp.RiskLevel == RiskLevel.Critical).ToList();
            if (highRisk.Any())
            {
                recommendations.Add($"⚠️ RISIKO: {highRisk.Count} Single-Point-of-Failure identifiziert");
                foreach (var cp in highRisk.Take(3))
                {
                    recommendations.Add($"   → {cp.StationName}: {cp.PassengersAffected:N0} Passagiere/Tag betroffen");
                    recommendations.Add($"      Empfehlung: Alternative Route oder Redundanz schaffen");
                }
            }
        }
        
        // Capacity reserves
        var lowReserve = bottlenecks.Where(b => b.CapacityReserve < 20).ToList();
        if (lowReserve.Any())
        {
            recommendations.Add($"📊 INFO: {lowReserve.Count} Strecken mit <20 Züge/Tag Reservekapazität");
            recommendations.Add($"   → Mittelfristige Kapazitätserweiterung planen");
        }
        
        if (!recommendations.Any())
        {
            recommendations.Add("✅ Netzwerk in gutem Zustand - keine kritischen Engpässe identifiziert");
        }
        
        return recommendations;
    }
}

// ============================================================================
// Data Models
// ============================================================================

/// <summary>
/// Network bottleneck (capacity constraint)
/// </summary>
public class NetworkBottleneck
{
    public string EdgeId { get; set; } = "";
    public Station? FromStation { get; set; }
    public Station? ToStation { get; set; }
    public double CurrentUtilization { get; set; } // 0.0 - 1.0
    public int DailyTrains { get; set; }
    public int MaxCapacity { get; set; }
    public int CapacityReserve { get; set; }
    public BottleneckSeverity Severity { get; set; }
    public BottleneckType Type { get; set; }
    
    public string GetDescription() =>
        $"{FromStation?.Name} → {ToStation?.Name}: {CurrentUtilization:P1} Auslastung ({DailyTrains}/{MaxCapacity} Züge/Tag)";
}

/// <summary>
/// Critical path (single point of failure)
/// </summary>
public class CriticalPath
{
    public string StationId { get; set; } = "";
    public string StationName { get; set; } = "";
    public string SingleConnectionTo { get; set; } = "";
    public string EdgeId { get; set; } = "";
    public int PassengersAffected { get; set; }
    public bool AlternativeRoutesAvailable { get; set; }
    public RiskLevel RiskLevel { get; set; }
    public bool IsBridgeEdge { get; set; } // Graph theory bridge
    
    public string GetDescription() =>
        IsBridgeEdge 
            ? $"Bridge-Kante: {StationName} - {SingleConnectionTo} (Kritische Verbindung)"
            : $"Endpunkt: {StationName} → {SingleConnectionTo} ({PassengersAffected:N0} Passagiere/Tag)";
}

/// <summary>
/// Capacity reserve information
/// </summary>
public class CapacityReserve
{
    public string EdgeId { get; set; } = "";
    public string Route { get; set; } = "";
    public int CurrentTrainsPerDay { get; set; }
    public int MaxCapacityPerDay { get; set; }
    public int ReserveTrains { get; set; }
    public double ReservePercent { get; set; }
    public CapacityStatus Status { get; set; }
}

/// <summary>
/// Complete bottleneck analysis report
/// </summary>
public class BottleneckReport
{
    public DateTime GeneratedAt { get; set; }
    
    // Summary statistics
    public int TotalBottlenecks { get; set; }
    public int CriticalBottlenecks { get; set; }
    public int HighBottlenecks { get; set; }
    public int MediumBottlenecks { get; set; }
    
    // Detailed findings
    public List<NetworkBottleneck> Bottlenecks { get; set; } = new();
    public List<CriticalPath> CriticalPaths { get; set; } = new();
    public List<CapacityReserve> CapacityReserves { get; set; } = new();
    
    // Recommendations
    public List<string> Recommendations { get; set; } = new();
    
    public override string ToString()
    {
        var report = $"=== NETZWERK-ENGPASS-ANALYSE ===" + Environment.NewLine;
        report += $"Erstellt: {GeneratedAt:yyyy-MM-dd HH:mm}" + Environment.NewLine;
        report += $"Engpässe gefunden: {TotalBottlenecks} (Kritisch: {CriticalBottlenecks}, Hoch: {HighBottlenecks}, Mittel: {MediumBottlenecks})" + Environment.NewLine;
        report += $"Kritische Pfade: {CriticalPaths.Count}" + Environment.NewLine;
        report += Environment.NewLine;
        
        report += "EMPFEHLUNGEN:" + Environment.NewLine;
        foreach (var rec in Recommendations)
        {
            report += rec + Environment.NewLine;
        }
        
        return report;
    }
}

/// <summary>
/// Bottleneck severity classification
/// </summary>
public enum BottleneckSeverity
{
    Low,        // 85-90% utilization
    Medium,     // 90-95% utilization
    High,       // 95-98% utilization
    Critical    // >98% utilization
}

/// <summary>
/// Bottleneck type classification
/// </summary>
public enum BottleneckType
{
    SingleTrack,        // Single-track section
    SignalingLimited,   // Limited by signaling system
    HighSpeedCapacity,  // High-speed line at capacity
    GeneralCapacity     // General capacity constraint
}

/// <summary>
/// Risk level for critical paths
/// </summary>
public enum RiskLevel
{
    Low,
    Medium,
    High,
    Critical
}

/// <summary>
/// Capacity status
/// </summary>
public enum CapacityStatus
{
    Good,       // >30% reserve
    Warning,    // 15-30% reserve
    Critical    // <15% reserve
}
