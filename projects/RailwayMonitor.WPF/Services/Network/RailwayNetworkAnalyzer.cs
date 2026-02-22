/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RailwayNetworkAnalyzer.cs                          ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     342                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
/// Railway Network Analyzer - Graph-based network analysis for German railway system
/// Sprint 1, US-1.1: Network Graph Structure
/// </summary>
public class RailwayNetworkAnalyzer
{
    private readonly Graph<Station, RailwayEdge> _network;
    private readonly Dictionary<string, Station> _stationIndex;
    
    public RailwayNetworkAnalyzer()
    {
        _network = new Graph<Station, RailwayEdge>();
        _stationIndex = new Dictionary<string, Station>();
    }
    
    /// <summary>
    /// Get the network graph
    /// </summary>
    public Graph<Station, RailwayEdge> Network => _network;
    
    /// <summary>
    /// Add station to network
    /// </summary>
    public void AddStation(Station station)
    {
        if (station == null)
            throw new ArgumentNullException(nameof(station));
            
        _network.AddNode(station);
        _stationIndex[station.Id] = station;
    }
    
    /// <summary>
    /// Add railway connection between two stations
    /// </summary>
    public void AddConnection(string fromStationId, string toStationId, RailwayEdge edge)
    {
        if (!_stationIndex.TryGetValue(fromStationId, out var fromStation))
            throw new ArgumentException($"Station {fromStationId} not found");
            
        if (!_stationIndex.TryGetValue(toStationId, out var toStation))
            throw new ArgumentException($"Station {toStationId} not found");
            
        edge.From = fromStation;
        edge.To = toStation;
        
        _network.AddEdge(fromStation, toStation, edge);
    }
    
    /// <summary>
    /// Get station by ID
    /// </summary>
    public Station? GetStation(string stationId)
    {
        return _stationIndex.GetValueOrDefault(stationId);
    }
    
    /// <summary>
    /// Get all stations
    /// </summary>
    public IEnumerable<Station> GetAllStations()
    {
        return _stationIndex.Values;
    }
    
    /// <summary>
    /// Get edges connected to a station
    /// </summary>
    public IEnumerable<RailwayEdge> GetConnectedEdges(string stationId)
    {
        if (!_stationIndex.TryGetValue(stationId, out var station))
            return Enumerable.Empty<RailwayEdge>();
            
        return _network.GetEdges(station);
    }
    
    /// <summary>
    /// Get neighbor stations
    /// </summary>
    public IEnumerable<Station> GetNeighbors(string stationId)
    {
        if (!_stationIndex.TryGetValue(stationId, out var station))
            return Enumerable.Empty<Station>();
            
        return _network.GetNeighbors(station);
    }
    
    /// <summary>
    /// Get network statistics
    /// </summary>
    public NetworkStatistics GetStatistics()
    {
        var stats = new NetworkStatistics
        {
            StationCount = _stationIndex.Count,
            EdgeCount = _network.EdgeCount,
            AverageConnections = _stationIndex.Count > 0 
                ? (double)_network.EdgeCount / _stationIndex.Count 
                : 0
        };
        
        // Count by station type
        foreach (var station in _stationIndex.Values)
        {
            switch (station.Type)
            {
                case StationType.Hauptbahnhof:
                    stats.HauptbahnhofCount++;
                    break;
                case StationType.Regionalbahnhof:
                    stats.RegionalbahnhofCount++;
                    break;
                case StationType.Haltepunkt:
                    stats.HaltepunktCount++;
                    break;
            }
        }
        
        return stats;
    }
}

/// <summary>
/// Generic Graph data structure
/// </summary>
public class Graph<TNode, TEdge> where TNode : class where TEdge : class
{
    private readonly Dictionary<TNode, List<TEdge>> _adjacencyList;
    private readonly HashSet<TNode> _nodes;
    private int _edgeCount;
    
    public Graph()
    {
        _adjacencyList = new Dictionary<TNode, List<TEdge>>();
        _nodes = new HashSet<TNode>();
        _edgeCount = 0;
    }
    
    public int NodeCount => _nodes.Count;
    public int EdgeCount => _edgeCount;
    
    /// <summary>
    /// Add node to graph
    /// </summary>
    public void AddNode(TNode node)
    {
        if (!_nodes.Contains(node))
        {
            _nodes.Add(node);
            _adjacencyList[node] = new List<TEdge>();
        }
    }
    
    /// <summary>
    /// Add edge between nodes
    /// </summary>
    public void AddEdge(TNode from, TNode to, TEdge edge)
    {
        if (!_nodes.Contains(from))
            AddNode(from);
        if (!_nodes.Contains(to))
            AddNode(to);
            
        _adjacencyList[from].Add(edge);
        _edgeCount++;
    }
    
    /// <summary>
    /// Get all edges from a node
    /// </summary>
    public IEnumerable<TEdge> GetEdges(TNode node)
    {
        if (_adjacencyList.TryGetValue(node, out var edges))
            return edges;
        return Enumerable.Empty<TEdge>();
    }
    
    /// <summary>
    /// Get neighbor nodes
    /// </summary>
    public IEnumerable<TNode> GetNeighbors(TNode node)
    {
        if (_adjacencyList.TryGetValue(node, out var edges))
        {
            foreach (var edge in edges)
            {
                if (edge is IEdge<TNode> graphEdge)
                {
                    yield return graphEdge.To;
                }
            }
        }
    }
    
    /// <summary>
    /// Get all nodes
    /// </summary>
    public IEnumerable<TNode> GetAllNodes() => _nodes;
    
    /// <summary>
    /// Check if node exists
    /// </summary>
    public bool ContainsNode(TNode node) => _nodes.Contains(node);
}

/// <summary>
/// Interface for graph edges
/// </summary>
public interface IEdge<TNode> where TNode : class
{
    TNode From { get; set; }
    TNode To { get; set; }
}

/// <summary>
/// Railway station node
/// </summary>
public class Station
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string Code { get; set; } = ""; // DS100 code (e.g., "FF" for Frankfurt Hbf)
    public StationType Type { get; set; }
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public int TrackCount { get; set; }
    public int PlatformCount { get; set; }
    public bool HasElevator { get; set; }
    public bool HasParkingRide { get; set; }
    public int PassengersPerDay { get; set; } // Average daily passengers
    public List<string> ServingLines { get; set; } = new(); // Line numbers (e.g., "ICE 1", "RE 5")
    
    public override string ToString() => $"{Name} ({Code})";
    
    public override bool Equals(object? obj)
    {
        if (obj is Station other)
            return Id == other.Id;
        return false;
    }
    
    public override int GetHashCode() => Id.GetHashCode();
}

/// <summary>
/// Railway edge (connection between stations)
/// </summary>
public class RailwayEdge : IEdge<Station>
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public Station From { get; set; } = null!;
    public Station To { get; set; } = null!;
    
    public double LengthKm { get; set; }
    public double MaxSpeedKmh { get; set; } = 160; // Default regional speed
    public int TrackCount { get; set; } = 2; // Number of parallel tracks
    public bool IsElectrified { get; set; } = true;
    public bool IsHighSpeed { get; set; } // ICE line
    public TrackType Type { get; set; } = TrackType.MainLine;
    
    // Capacity metrics
    public int DailyTrains { get; set; }
    public int MaxCapacityTrainsPerHour { get; set; } = 12; // Theoretical max
    
    // Infrastructure
    public bool HasETCS { get; set; } // European Train Control System
    public bool HasATB { get; set; } // Automatic Train Protection
    
    public override string ToString() => $"{From?.Name} → {To?.Name} ({LengthKm:F1} km)";
}

/// <summary>
/// Station type classification
/// </summary>
public enum StationType
{
    Hauptbahnhof,       // Main station (Category 1-2)
    Regionalbahnhof,    // Regional station (Category 3-4)
    Haltepunkt,         // Stop point (Category 5-7)
    Güterbahnhof,       // Freight station
    Betriebsbahnhof     // Operation station
}

/// <summary>
/// Track type classification
/// </summary>
public enum TrackType
{
    HighSpeed,          // Neubaustrecke (>250 km/h)
    MainLine,           // Hauptbahn (100-200 km/h)
    BranchLine,         // Nebenbahn (50-100 km/h)
    IndustrialSpur,     // Industrieanschluss
    MuseumLine          // Museumsbahn
}

/// <summary>
/// Network statistics
/// </summary>
public class NetworkStatistics
{
    public int StationCount { get; set; }
    public int EdgeCount { get; set; }
    public double AverageConnections { get; set; }
    
    // Station breakdown
    public int HauptbahnhofCount { get; set; }
    public int RegionalbahnhofCount { get; set; }
    public int HaltepunktCount { get; set; }
    
    public override string ToString()
    {
        return $"Network: {StationCount} stations, {EdgeCount} connections\n" +
               $"Hauptbahnhöfe: {HauptbahnhofCount}, Regional: {RegionalbahnhofCount}, Haltepunkte: {HaltepunktCount}\n" +
               $"Avg connections per station: {AverageConnections:F2}";
    }
}
