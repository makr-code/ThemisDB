/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoSpatialAnalyzer.cs                              ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     802                                            ║
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

using System.Collections.Concurrent;
using NetTopologySuite.Geometries;
using NetTopologySuite.Operation.Distance;

namespace RailwayMonitor.WPF.Services.Optimization;

/// <summary>
/// Cost-Based A* Algorithm für Railway Route Planning
/// Ähnlich Railway Empire - berücksichtigt Terrain, Steigung, Baukosten, Distanz
/// </summary>
public interface IGeoSpatialAnalyzer
{
    Task<OptimalPath?> FindOptimalPathAsync(string origin, string destination, PathCriteria criteria);
    Task<List<PathResult>> FindAllPathsAsync(string origin, string destination, int maxAlternatives);
    Task<List<RouteSegment>> SubdivideRouteAsync(RouteSegment route, double segmentLengthKm);
    Task<TerrainInfo> GetTerrainInfoAsync(double lat, double lon);
    Task<double> CalculateElevationChangeAsync(double lat1, double lon1, double lat2, double lon2);
}

/// <summary>
/// Geo-Spatial Analyzer mit Railway Empire-ähnlichem A* für Routenplanung
/// Integriert CrossingAnalyzer und SettlementAnalyzer für realistische Kostenberechnung
/// </summary>
public class GeoSpatialAnalyzer : IGeoSpatialAnalyzer
{
    private readonly ITerrainDataProvider _terrainProvider;
    private readonly CrossingAnalyzer? _crossingAnalyzer;
    private readonly SettlementAnalyzer? _settlementAnalyzer;
    private readonly Dictionary<string, GeoNode> _nodeCache = new();
    private readonly ConcurrentDictionary<string, TerrainInfo> _terrainCache = new();
    
    // Kosten-Konstanten (wie in Railway Empire)
    private const double COST_PER_KM_FLAT = 10_000_000;      // 10 Mio € pro km ebenes Gelände
    private const double COST_PER_KM_HILL = 15_000_000;      // 15 Mio € pro km hügeliges Gelände
    private const double COST_PER_KM_MOUNTAIN = 25_000_000;  // 25 Mio € pro km bergiges Gelände
    private const double COST_PER_KM_TUNNEL = 50_000_000;    // 50 Mio € pro km Tunnel
    private const double COST_PER_KM_BRIDGE = 20_000_000;    // 20 Mio € pro km Brücke
    private const double COST_PER_KM_WATER = 30_000_000;     // 30 Mio € pro km Wasserüberquerung
    private const double COST_GRADIENT_MULTIPLIER = 2.0;     // Steigungszuschlag
    private const double COST_CURVE_MULTIPLIER = 1.5;        // Kurvenzuschlag
    private const double COST_PROTECTED_AREA = 100_000_000;  // Schutzgebiet-Zuschlag

    public GeoSpatialAnalyzer(
        ITerrainDataProvider terrainProvider,
        CrossingAnalyzer? crossingAnalyzer = null,
        SettlementAnalyzer? settlementAnalyzer = null)
    {
        _terrainProvider = terrainProvider;
        _crossingAnalyzer = crossingAnalyzer;
        _settlementAnalyzer = settlementAnalyzer;
    }

    /// <summary>
    /// A* Algorithmus mit Geo-Kosten für optimale Routenplanung
    /// </summary>
    public async Task<OptimalPath?> FindOptimalPathAsync(string origin, string destination, PathCriteria criteria)
    {
        Console.WriteLine($"[A*] Planning route from {origin} to {destination}...");
        
        // Hole Start- und Zielknoten
        var startNode = await GetOrCreateNodeAsync(origin);
        var goalNode = await GetOrCreateNodeAsync(destination);
        
        if (startNode == null || goalNode == null)
            return null;

        // A* Collections
        var openSet = new PriorityQueue<GeoNode, double>();
        var closedSet = new HashSet<string>();
        var cameFrom = new Dictionary<string, GeoNode>();
        var gScore = new Dictionary<string, double> { [startNode.Id] = 0 };
        var fScore = new Dictionary<string, double> { [startNode.Id] = HeuristicCost(startNode, goalNode) };
        
        openSet.Enqueue(startNode, fScore[startNode.Id]);
        
        int iterations = 0;
        const int MAX_ITERATIONS = 10000;
        
        while (openSet.Count > 0 && iterations < MAX_ITERATIONS)
        {
            iterations++;
            
            // Hole Knoten mit niedrigsten f-Score
            var current = openSet.Dequeue();
            
            // Ziel erreicht?
            if (current.Id == goalNode.Id)
            {
                Console.WriteLine($"[A*] Path found in {iterations} iterations!");
                return await ReconstructPathAsync(cameFrom, current, startNode, criteria);
            }
            
            closedSet.Add(current.Id);
            
            // Expandiere Nachbarn
            var neighbors = await GetNeighborsAsync(current, goalNode, criteria);
            
            foreach (var neighbor in neighbors)
            {
                if (closedSet.Contains(neighbor.Id))
                    continue;
                
                // Berechne Kosten für diese Route
                var moveCost = await CalculateMoveCostAsync(current, neighbor, criteria);
                var tentativeGScore = gScore.GetValueOrDefault(current.Id, double.MaxValue) + moveCost;
                
                if (!gScore.ContainsKey(neighbor.Id) || tentativeGScore < gScore[neighbor.Id])
                {
                    // Besserer Weg gefunden
                    cameFrom[neighbor.Id] = current;
                    gScore[neighbor.Id] = tentativeGScore;
                    fScore[neighbor.Id] = tentativeGScore + HeuristicCost(neighbor, goalNode);
                    
                    openSet.Enqueue(neighbor, fScore[neighbor.Id]);
                }
            }
        }
        
        Console.WriteLine($"[A*] No path found after {iterations} iterations");
        return null;
    }

    /// <summary>
    /// Heuristische Kostenfunktion (Luftlinie mit geschätzten Baukosten)
    /// </summary>
    private double HeuristicCost(GeoNode from, GeoNode to)
    {
        // Haversine-Distanz
        var distance = CalculateDistance(from.Latitude, from.Longitude, to.Latitude, to.Longitude);
        
        // Geschätzte durchschnittliche Baukosten pro km
        var estimatedCostPerKm = COST_PER_KM_FLAT * 1.3; // 30% Aufschlag für Unwägbarkeiten
        
        return distance * estimatedCostPerKm;
    }

    /// <summary>
    /// Berechne tatsächliche Bewegungskosten zwischen zwei Knoten
    /// Berücksichtigt: Terrain, Steigung, Kurven, Schutzgebiete
    /// </summary>
    private async Task<double> CalculateMoveCostAsync(GeoNode from, GeoNode to, PathCriteria criteria)
    {
        var distance = CalculateDistance(from.Latitude, from.Longitude, to.Latitude, to.Longitude);
        
        // 1. Basis-Terrain-Kosten
        var terrainFrom = await GetTerrainInfoAsync(from.Latitude, from.Longitude);
        var terrainTo = await GetTerrainInfoAsync(to.Latitude, to.Longitude);
        var avgTerrain = (terrainFrom.Elevation + terrainTo.Elevation) / 2;
        
        double baseCost = terrainFrom.Type switch
        {
            TerrainType.Flat => COST_PER_KM_FLAT,
            TerrainType.Hill => COST_PER_KM_HILL,
            TerrainType.Mountain => COST_PER_KM_MOUNTAIN,
            TerrainType.Water => COST_PER_KM_WATER,
            _ => COST_PER_KM_FLAT
        };
        
        // 2. Steigungskosten (kritisch für Züge!)
        var elevationChange = Math.Abs(terrainTo.Elevation - terrainFrom.Elevation);
        var gradient = distance > 0 ? (elevationChange / (distance * 1000)) * 100 : 0; // in %
        
        double gradientMultiplier = 1.0;
        if (gradient > 2.5) // Max 2.5% für Hochgeschwindigkeitszüge
        {
            // Tunnel erforderlich
            baseCost = COST_PER_KM_TUNNEL;
            gradientMultiplier = 1.0;
        }
        else if (gradient > 1.5)
        {
            gradientMultiplier = COST_GRADIENT_MULTIPLIER;
        }
        else if (gradient > 0.5)
        {
            gradientMultiplier = 1.3;
        }
        
        // 3. Brücken-Kosten (bei Wasserüberquerung oder tiefen Tälern)
        double bridgeCost = 0;
        if (terrainFrom.Type == TerrainType.Water || terrainTo.Type == TerrainType.Water)
        {
            bridgeCost = distance * COST_PER_KM_BRIDGE;
        }
        else if (elevationChange > 50 && terrainFrom.Elevation > terrainTo.Elevation)
        {
            // Brücke über Tal
            bridgeCost = distance * COST_PER_KM_BRIDGE * 0.5;
        }
        
        // 4. Kurven-Kosten (Richtungsänderung)
        double curveCost = 0;
        if (from.PreviousNode != null)
        {
            var angleDiff = CalculateAngleDifference(from.PreviousNode, from, to);
            if (angleDiff > 30) // Kurve > 30°
            {
                curveCost = baseCost * (COST_CURVE_MULTIPLIER - 1.0);
            }
        }
        
        // 5. Schutzgebiete
        double protectedAreaCost = 0;
        if (terrainFrom.IsProtectedArea || terrainTo.IsProtectedArea)
        {
            if (criteria.AvoidDifficultTerrain)
            {
                // Stark verteuern wenn Vermeidung gewünscht
                protectedAreaCost = COST_PROTECTED_AREA;
            }
            else
            {
                protectedAreaCost = COST_PROTECTED_AREA * 0.3; // Teilzuschlag
            }
        }
        
        // 6. Bebauung (Enteignungskosten) - ENHANCED with SettlementAnalyzer
        double urbanCost = 0;
        if (_settlementAnalyzer != null)
        {
            // Use SettlementAnalyzer for precise urban constraint analysis
            var midPoint = new GeoPoint(
                (from.Latitude + to.Latitude) / 2,
                (from.Longitude + to.Longitude) / 2
            );
            
            var settlement = await _settlementAnalyzer.AnalyzeSettlementAsync(midPoint, distance / 2);
            
            // Add urban penalty based on settlement type
            urbanCost = (double)settlement.EstimatedPenaltyCost;
            
            // If tunnel is required for this settlement type, force tunnel costs
            if (settlement.RequiresTunnel && !criteria.AllowTunnels)
            {
                // Make route prohibitively expensive if tunnels not allowed but required
                urbanCost += COST_PER_KM_TUNNEL * distance * 2;
            }
        }
        else if (terrainFrom.UrbanDensity > 0.5 || terrainTo.UrbanDensity > 0.5)
        {
            // Fallback to simple calculation if SettlementAnalyzer not available
            urbanCost = distance * COST_PER_KM_FLAT * 2.0 * terrainFrom.UrbanDensity;
        }
        
        // 7. Road/Railway Crossings - NEW with CrossingAnalyzer
        double crossingCost = 0;
        if (_crossingAnalyzer != null)
        {
            // Detect crossings along this segment
            var routeSegment = new List<GeoPoint>
            {
                new GeoPoint(from.Latitude, from.Longitude),
                new GeoPoint(to.Latitude, to.Longitude)
            };
            
            var crossings = await _crossingAnalyzer.DetectRoadCrossingsAsync(routeSegment, 50);
            
            // Sum up all crossing costs
            foreach (var crossing in crossings)
            {
                crossingCost += (double)crossing.EstimatedCost;
                
                // Additional penalty for crossings requiring permits
                if (crossing.RequiresPermit)
                {
                    crossingCost += 500_000; // Administrative costs
                }
            }
        }
        
        // 8. Existierende Korridore bevorzugen (billiger!)
        double corridorDiscount = 0;
        if (criteria.PreferExistingCorridors)
        {
            if (terrainFrom.HasExistingRailway || terrainTo.HasExistingRailway)
            {
                corridorDiscount = baseCost * 0.4; // 40% Rabatt bei existierender Infrastruktur
            }
        }
        
        // Gesamtkosten - NOW includes crossing costs
        var totalCost = (baseCost * distance * gradientMultiplier) + 
                       bridgeCost + 
                       curveCost + 
                       protectedAreaCost + 
                       urbanCost + 
                       crossingCost - 
                       corridorDiscount;
        
        // Kriterien-basierte Adjustierung
        if (criteria.MinimizeDistance)
        {
            // Distanz stärker gewichten
            totalCost *= (1 + distance / 100.0);
        }
        
        if (criteria.AvoidDifficultTerrain && (gradient > 1.5 || terrainFrom.Type == TerrainType.Mountain))
        {
            // Schwieriges Terrain stark verteuern
            totalCost *= 2.0;
        }
        
        return totalCost;
    }

    /// <summary>
    /// Generiere Nachbarknoten für A* Expansion
    /// </summary>
    private async Task<List<GeoNode>> GetNeighborsAsync(GeoNode current, GeoNode goal, PathCriteria criteria)
    {
        var neighbors = new List<GeoNode>();
        
        // Verschiedene Richtungen (8-directional)
        var directions = new[]
        {
            (0.0, 0.01),    // Nord
            (0.01, 0.01),   // Nordost
            (0.01, 0.0),    // Ost
            (0.01, -0.01),  // Südost
            (0.0, -0.01),   // Süd
            (-0.01, -0.01), // Südwest
            (-0.01, 0.0),   // West
            (-0.01, 0.01)   // Nordwest
        };
        
        // Adaptive Schrittweite basierend auf Distanz zum Ziel
        var distanceToGoal = CalculateDistance(current.Latitude, current.Longitude, goal.Latitude, goal.Longitude);
        var stepMultiplier = distanceToGoal > 100 ? 5.0 : (distanceToGoal > 50 ? 2.0 : 1.0);
        
        foreach (var (latDiff, lonDiff) in directions)
        {
            var newLat = current.Latitude + latDiff * stepMultiplier;
            var newLon = current.Longitude + lonDiff * stepMultiplier;
            
            var neighbor = new GeoNode
            {
                Id = $"{newLat:F4}_{newLon:F4}",
                Latitude = newLat,
                Longitude = newLon,
                PreviousNode = current
            };
            
            neighbors.Add(neighbor);
        }
        
        return neighbors;
    }

    /// <summary>
    /// Rekonstruiere den gefundenen Pfad
    /// </summary>
    private async Task<OptimalPath> ReconstructPathAsync(
        Dictionary<string, GeoNode> cameFrom,
        GeoNode current,
        GeoNode start,
        PathCriteria criteria)
    {
        var path = new List<GeoNode>();
        var totalCost = 0.0;
        var totalDistance = 0.0;
        var totalTunnel = 0.0;
        var totalBridge = 0.0;
        var maxGradient = 0.0;
        var protectedAreasKm = 0.0;
        
        // Rückwärts durch cameFrom
        path.Add(current);
        while (cameFrom.ContainsKey(current.Id))
        {
            current = cameFrom[current.Id];
            path.Add(current);
            
            if (path.Count > 1)
            {
                var prev = path[path.Count - 2];
                var distance = CalculateDistance(current.Latitude, current.Longitude, prev.Latitude, prev.Longitude);
                totalDistance += distance;
                
                var cost = await CalculateMoveCostAsync(current, prev, criteria);
                totalCost += cost;
                
                // Tunnel/Brücken/Gradient erfassen
                var terrainCurrent = await GetTerrainInfoAsync(current.Latitude, current.Longitude);
                var terrainPrev = await GetTerrainInfoAsync(prev.Latitude, prev.Longitude);
                
                var elevationChange = Math.Abs(terrainPrev.Elevation - terrainCurrent.Elevation);
                var gradient = distance > 0 ? (elevationChange / (distance * 1000)) * 100 : 0;
                maxGradient = Math.Max(maxGradient, gradient);
                
                if (gradient > 2.5)
                    totalTunnel += distance;
                
                if (terrainCurrent.Type == TerrainType.Water)
                    totalBridge += distance;
                
                if (terrainCurrent.IsProtectedArea)
                    protectedAreasKm += distance;
            }
        }
        
        path.Reverse();
        
        // Konvertiere zu RouteSegments
        var segments = new List<RouteSegment>();
        for (int i = 0; i < path.Count - 1; i++)
        {
            var from = path[i];
            var to = path[i + 1];
            
            segments.Add(new RouteSegment
            {
                RouteId = $"segment_{i}",
                Name = $"Segment {i}",
                Latitude = (from.Latitude + to.Latitude) / 2,
                Longitude = (from.Longitude + to.Longitude) / 2,
                LengthKm = CalculateDistance(from.Latitude, from.Longitude, to.Latitude, to.Longitude),
                StartKm = segments.Sum(s => s.LengthKm),
                EndKm = segments.Sum(s => s.LengthKm) + CalculateDistance(from.Latitude, from.Longitude, to.Latitude, to.Longitude)
            });
        }
        
        var avgSpeed = maxGradient < 1.5 ? 250.0 : (maxGradient < 2.5 ? 200.0 : 160.0); // km/h
        var travelTime = (int)(totalDistance / avgSpeed * 60);
        
        return new OptimalPath
        {
            Segments = segments,
            TotalLengthKm = totalDistance,
            TotalCost = totalCost,
            TunnelLengthKm = totalTunnel,
            BridgeLengthKm = totalBridge,
            MaxGradient = maxGradient,
            ProtectedAreasKm = protectedAreasKm,
            DifficultTerrainPercent = (totalTunnel + totalBridge) / totalDistance * 100,
            EstimatedTravelTimeMinutes = travelTime,
            Waypoints = path.Select(p => new GeoLocation { Latitude = p.Latitude, Longitude = p.Longitude }).ToList()
        };
    }

    /// <summary>
    /// Finde mehrere alternative Routen
    /// </summary>
    public async Task<List<PathResult>> FindAllPathsAsync(string origin, string destination, int maxAlternatives)
    {
        var paths = new List<PathResult>();
        
        // Verschiedene Kriterien für verschiedene Alternativen
        var criteriaList = new[]
        {
            new PathCriteria { MinimizeDistance = true, PreferExistingCorridors = false }, // Kürzeste
            new PathCriteria { MinimizeDistance = false, AvoidDifficultTerrain = true },  // Einfachste
            new PathCriteria { MinimizeDistance = false, PreferExistingCorridors = true }, // Entlang Korridoren
            new PathCriteria { MinimizeDistance = false, MaxGradient = 1.0 }              // Flachste
        };
        
        foreach (var criteria in criteriaList.Take(maxAlternatives))
        {
            var path = await FindOptimalPathAsync(origin, destination, criteria);
            if (path != null)
            {
                paths.Add(new PathResult
                {
                    Segments = path.Segments,
                    TotalCost = path.TotalCost,
                    TotalLengthKm = path.TotalLengthKm
                });
            }
        }
        
        return paths.DistinctBy(p => p.TotalLengthKm).ToList();
    }

    /// <summary>
    /// Hole Terrain-Informationen (aus Cache oder Provider)
    /// </summary>
    public async Task<TerrainInfo> GetTerrainInfoAsync(double lat, double lon)
    {
        var key = $"{lat:F4}_{lon:F4}";
        
        if (_terrainCache.TryGetValue(key, out var cached))
            return cached;
        
        var terrain = await _terrainProvider.GetTerrainDataAsync(lat, lon);
        _terrainCache[key] = terrain;
        
        return terrain;
    }

    public async Task<double> CalculateElevationChangeAsync(double lat1, double lon1, double lat2, double lon2)
    {
        var terrain1 = await GetTerrainInfoAsync(lat1, lon1);
        var terrain2 = await GetTerrainInfoAsync(lat2, lon2);
        
        return Math.Abs(terrain2.Elevation - terrain1.Elevation);
    }

    public async Task<List<RouteSegment>> SubdivideRouteAsync(RouteSegment route, double segmentLengthKm)
    {
        var segments = new List<RouteSegment>();
        var numSegments = (int)Math.Ceiling(route.LengthKm / segmentLengthKm);
        
        for (int i = 0; i < numSegments; i++)
        {
            segments.Add(new RouteSegment
            {
                RouteId = $"{route.RouteId}_seg{i}",
                Name = $"{route.Name} Segment {i}",
                StartKm = i * segmentLengthKm,
                EndKm = Math.Min((i + 1) * segmentLengthKm, route.LengthKm),
                LengthKm = Math.Min(segmentLengthKm, route.LengthKm - i * segmentLengthKm),
                Latitude = route.Latitude, // Vereinfacht
                Longitude = route.Longitude
            });
        }
        
        return segments;
    }

    // Helper Methods
    
    private async Task<GeoNode?> GetOrCreateNodeAsync(string locationName)
    {
        if (_nodeCache.TryGetValue(locationName, out var node))
            return node;
        
        // In Produktion: Geocoding API
        var coords = await GeocodeLocationAsync(locationName);
        if (coords == null)
            return null;
        
        node = new GeoNode
        {
            Id = locationName,
            Latitude = coords.Value.Lat,
            Longitude = coords.Value.Lon
        };
        
        _nodeCache[locationName] = node;
        return node;
    }

    private async Task<(double Lat, double Lon)?> GeocodeLocationAsync(string locationName)
    {
        // Mock geocoding - in Produktion: Nominatim oder Google Maps API
        var mockLocations = new Dictionary<string, (double, double)>
        {
            ["Berlin"] = (52.5200, 13.4050),
            ["München"] = (48.1351, 11.5820),
            ["Hamburg"] = (53.5511, 9.9937),
            ["Frankfurt"] = (50.1109, 8.6821),
            ["Köln"] = (50.9375, 6.9603),
            ["Stuttgart"] = (48.7758, 9.1829),
            ["Dresden"] = (51.0504, 13.7373),
            ["Leipzig"] = (51.3397, 12.3731)
        };
        
        if (mockLocations.TryGetValue(locationName, out var coords))
            return coords;
        
        return null;
    }

    private double CalculateDistance(double lat1, double lon1, double lat2, double lon2)
    {
        // Haversine formula
        var R = 6371; // Earth radius in km
        var dLat = ToRadians(lat2 - lat1);
        var dLon = ToRadians(lon2 - lon1);
        
        var a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) +
                Math.Cos(ToRadians(lat1)) * Math.Cos(ToRadians(lat2)) *
                Math.Sin(dLon / 2) * Math.Sin(dLon / 2);
        
        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
        
        return R * c;
    }

    private double ToRadians(double degrees) => degrees * Math.PI / 180.0;

    private double CalculateAngleDifference(GeoNode prev, GeoNode current, GeoNode next)
    {
        // Bearing zwischen prev->current und current->next
        var bearing1 = CalculateBearing(prev.Latitude, prev.Longitude, current.Latitude, current.Longitude);
        var bearing2 = CalculateBearing(current.Latitude, current.Longitude, next.Latitude, next.Longitude);
        
        var diff = Math.Abs(bearing2 - bearing1);
        if (diff > 180)
            diff = 360 - diff;
        
        return diff;
    }

    private double CalculateBearing(double lat1, double lon1, double lat2, double lon2)
    {
        var dLon = ToRadians(lon2 - lon1);
        var lat1Rad = ToRadians(lat1);
        var lat2Rad = ToRadians(lat2);
        
        var y = Math.Sin(dLon) * Math.Cos(lat2Rad);
        var x = Math.Cos(lat1Rad) * Math.Sin(lat2Rad) -
                Math.Sin(lat1Rad) * Math.Cos(lat2Rad) * Math.Cos(dLon);
        
        var bearing = Math.Atan2(y, x);
        return (ToDegrees(bearing) + 360) % 360;
    }

    private double ToDegrees(double radians) => radians * 180.0 / Math.PI;
}

// Supporting Classes

public class GeoNode
{
    public string Id { get; set; } = "";
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public GeoNode? PreviousNode { get; set; }
}

public class PathCriteria
{
    public bool MinimizeDistance { get; set; }
    public bool AvoidDifficultTerrain { get; set; }
    public bool PreferExistingCorridors { get; set; }
    public bool AllowTunnels { get; set; } = true; // Allow tunnels by default
    public double MaxGradient { get; set; } = 2.5; // % Steigung
}

public class OptimalPath
{
    public List<RouteSegment> Segments { get; set; } = new();
    public double TotalLengthKm { get; set; }
    public double TotalCost { get; set; }
    public double TunnelLengthKm { get; set; }
    public double BridgeLengthKm { get; set; }
    public double MaxGradient { get; set; }
    public double ProtectedAreasKm { get; set; }
    public double DifficultTerrainPercent { get; set; }
    public int EstimatedTravelTimeMinutes { get; set; }
    public List<GeoLocation> Waypoints { get; set; } = new();
    
    // NEW: Crossing and Settlement Analysis Results
    public List<RoadCrossing> RoadCrossings { get; set; } = new();
    public List<SettlementInfo> SettlementsAffected { get; set; } = new();
    public double TotalCrossingCost { get; set; }
    public double TotalUrbanPenaltyCost { get; set; }
    public int SettlementsRequiringTunnels { get; set; }
}

public class PathResult
{
    public List<RouteSegment> Segments { get; set; } = new();
    public double TotalCost { get; set; }
    public double TotalLengthKm { get; set; }
}

public class GeoLocation
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
}

public enum TerrainType
{
    Flat,
    Hill,
    Mountain,
    Water,
    Urban
}

public class TerrainInfo
{
    public TerrainType Type { get; set; }
    public double Elevation { get; set; } // in meters
    public bool IsProtectedArea { get; set; }
    public double UrbanDensity { get; set; } // 0.0 - 1.0
    public bool HasExistingRailway { get; set; }
    public string SoilType { get; set; } = "normal"; // rock, sand, clay, etc.
}

public class RouteSegment
{
    public string RouteId { get; set; } = "";
    public string Name { get; set; } = "";
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public double LengthKm { get; set; }
    public double StartKm { get; set; }
    public double EndKm { get; set; }
    public int NumberOfTracks { get; set; } = 1;
    public double MaxSpeedKmh { get; set; } = 160;
    public double AverageSignalDistanceM { get; set; } = 2000;
}

/// <summary>
/// Terrain Data Provider Interface (kann echte DEM-Daten laden)
/// </summary>
public interface ITerrainDataProvider
{
    Task<TerrainInfo> GetTerrainDataAsync(double lat, double lon);
}

/// <summary>
/// Mock Terrain Provider (in Produktion: SRTM, ASTER GDEM, etc.)
/// </summary>
public class MockTerrainDataProvider : ITerrainDataProvider
{
    public async Task<TerrainInfo> GetTerrainDataAsync(double lat, double lon)
    {
        await Task.CompletedTask;
        
        // Vereinfachte Terrain-Klassifizierung basierend auf Deutschland
        var elevation = CalculateElevation(lat, lon);
        
        return new TerrainInfo
        {
            Type = elevation switch
            {
                < 200 => TerrainType.Flat,
                < 500 => TerrainType.Hill,
                _ => TerrainType.Mountain
            },
            Elevation = elevation,
            IsProtectedArea = IsInProtectedArea(lat, lon),
            UrbanDensity = CalculateUrbanDensity(lat, lon),
            HasExistingRailway = HasNearbyRailway(lat, lon),
            SoilType = "normal"
        };
    }

    private double CalculateElevation(double lat, double lon)
    {
        // Mock: Alpen im Süden, flach im Norden
        if (lat < 48.0) // Süddeutschland
            return 500 + (48.0 - lat) * 200; // Bis zu 1000m
        else
            return 50 + Math.Abs(lat - 52.0) * 30; // Flachland
    }

    private bool IsInProtectedArea(double lat, double lon)
    {
        // Mock: Einige Schutzgebiete
        return (lat > 47.5 && lat < 48.0) || // Alpenrand
               (lat > 51.0 && lat < 51.5 && lon > 13.0); // Lausitz
    }

    private double CalculateUrbanDensity(double lat, double lon)
    {
        // Mock: Großstädte haben hohe Dichte
        var cities = new[]
        {
            (52.52, 13.40), // Berlin
            (48.13, 11.58), // München
            (53.55, 9.99),  // Hamburg
            (50.11, 8.68)   // Frankfurt
        };
        
        foreach (var (cityLat, cityLon) in cities)
        {
            var distance = Math.Sqrt(Math.Pow(lat - cityLat, 2) + Math.Pow(lon - cityLon, 2));
            if (distance < 0.5) // ~50km
                return 0.9 - distance;
        }
        
        return 0.1; // Ländlich
    }

    private bool HasNearbyRailway(double lat, double lon)
    {
        // Mock: Hauptachsen haben existierende Schienen
        return (lat > 48.0 && lat < 52.0) || // Nord-Süd Achse
               (lon > 8.0 && lon < 14.0);    // Ost-West Achse
    }
}
