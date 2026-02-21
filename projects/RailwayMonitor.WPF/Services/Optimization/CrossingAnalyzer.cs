/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CrossingAnalyzer.cs                                ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     418                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Services.Optimization;

/// <summary>
/// Analyzes crossings with other traffic infrastructure (roads, railways, waterways)
/// and determines optimal crossing strategies (level crossing, bridge, tunnel).
/// </summary>
public class CrossingAnalyzer
{
    private readonly HttpClient _httpClient;
    private readonly string _overpassUrl = "https://overpass-api.de/api/interpreter";

    public CrossingAnalyzer(HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    /// <summary>
    /// Detects all crossings along a planned route.
    /// </summary>
    public async Task<List<RoadCrossing>> DetectRoadCrossingsAsync(
        List<GeoPoint> routePath,
        double bufferMeters = 100)
    {
        var crossings = new List<RoadCrossing>();
        
        // Build bounding box around route
        var bbox = CalculateBoundingBox(routePath, bufferMeters);
        
        // Query Overpass API for roads
        var roads = await QueryRoadsAsync(bbox);
        
        // Find intersections
        foreach (var road in roads)
        {
            var intersections = FindIntersections(routePath, road.Geometry);
            
            foreach (var point in intersections)
            {
                var roadType = ClassifyRoad(road.Highway);
                var crossingType = DetermineBestCrossing(roadType, road);
                
                crossings.Add(new RoadCrossing
                {
                    Location = point,
                    RoadId = road.Id,
                    RoadName = road.Name ?? "Unnamed Road",
                    RoadType = roadType,
                    Highway = road.Highway,
                    CrossingType = crossingType,
                    EstimatedCost = CalculateCrossingCost(crossingType, roadType),
                    RequiresPermit = roadType >= RoadType.StateRoad,
                    TrafficVolume = EstimateTrafficVolume(road),
                    MaxHeight = road.MaxHeight,
                    Lanes = road.Lanes
                });
            }
        }
        
        return crossings;
    }

    /// <summary>
    /// Queries Overpass API for roads in bounding box.
    /// </summary>
    private async Task<List<OsmRoad>> QueryRoadsAsync(BoundingBox bbox)
    {
        var query = $@"
[out:json][timeout:25];
(
  way[""highway""~""motorway|trunk|primary|secondary|tertiary|residential|unclassified""]
    ({bbox.MinLat},{bbox.MinLon},{bbox.MaxLat},{bbox.MaxLon});
  way[""waterway""~""river|canal""]
    ({bbox.MinLat},{bbox.MinLon},{bbox.MaxLat},{bbox.MaxLon});
  way[""railway""=""rail""]
    ({bbox.MinLat},{bbox.MinLon},{bbox.MaxLat},{bbox.MaxLon});
);
out geom;
";

        var content = new FormUrlEncodedContent(new[]
        {
            new KeyValuePair<string, string>("data", query)
        });

        try
        {
            var response = await _httpClient.PostAsync(_overpassUrl, content);
            response.EnsureSuccessStatusCode();
            
            var json = await response.Content.ReadAsStringAsync();
            var result = JsonSerializer.Deserialize<OverpassResult>(json);
            
            return result?.Elements
                .Where(e => e.Type == "way")
                .Select(e => new OsmRoad
                {
                    Id = e.Id,
                    Name = e.Tags.GetValueOrDefault("name"),
                    Highway = e.Tags.GetValueOrDefault("highway"),
                    Waterway = e.Tags.GetValueOrDefault("waterway"),
                    Railway = e.Tags.GetValueOrDefault("railway"),
                    Lanes = ParseInt(e.Tags.GetValueOrDefault("lanes")),
                    MaxHeight = ParseDouble(e.Tags.GetValueOrDefault("maxheight")),
                    Geometry = e.Geometry.Select(g => new GeoPoint(g.Lat, g.Lon)).ToList()
                }).ToList() ?? new List<OsmRoad>();
        }
        catch
        {
            // Fallback: Return empty list if API fails
            return new List<OsmRoad>();
        }
    }

    /// <summary>
    /// Classifies road type based on OSM highway tag.
    /// </summary>
    private RoadType ClassifyRoad(string? highway)
    {
        return highway switch
        {
            "motorway" or "motorway_link" => RoadType.Autobahn,
            "trunk" or "trunk_link" => RoadType.FederalRoad,
            "primary" or "primary_link" => RoadType.StateRoad,
            "secondary" or "secondary_link" => RoadType.CountyRoad,
            "tertiary" or "tertiary_link" => RoadType.CountyRoad,
            "residential" or "unclassified" => RoadType.LocalRoad,
            "service" or "track" => RoadType.Footpath,
            _ => RoadType.LocalRoad
        };
    }

    /// <summary>
    /// Determines the best crossing strategy.
    /// </summary>
    private CrossingType DetermineBestCrossing(RoadType roadType, OsmRoad road)
    {
        // Autobahn: Always bridge or tunnel
        if (roadType == RoadType.Autobahn)
        {
            return CrossingType.Bridge; // Default to bridge (cheaper than tunnel)
        }

        // Federal roads: Prefer bridge
        if (roadType >= RoadType.FederalRoad)
        {
            return CrossingType.Bridge;
        }

        // High traffic volume: Bridge
        var traffic = EstimateTrafficVolume(road);
        if (traffic > 5000) // vehicles per day
        {
            return CrossingType.Bridge;
        }

        // Low traffic: Level crossing
        if (traffic < 1000)
        {
            return CrossingType.LevelCrossing;
        }

        // Medium traffic: Bridge for safety
        return CrossingType.Bridge;
    }

    /// <summary>
    /// Calculates crossing cost based on type and road classification.
    /// </summary>
    private decimal CalculateCrossingCost(CrossingType type, RoadType road)
    {
        return (type, road) switch
        {
            // Level Crossings (Bahnübergänge)
            (CrossingType.LevelCrossing, RoadType.Footpath) => 50_000m,
            (CrossingType.LevelCrossing, RoadType.LocalRoad) => 100_000m,
            (CrossingType.LevelCrossing, RoadType.CountyRoad) => 200_000m,
            (CrossingType.LevelCrossing, RoadType.StateRoad) => 300_000m, // Rarely allowed

            // Bridges (Railway over road)
            (CrossingType.Bridge, RoadType.Footpath) => 500_000m,
            (CrossingType.Bridge, RoadType.LocalRoad) => 1_000_000m,
            (CrossingType.Bridge, RoadType.CountyRoad) => 1_500_000m,
            (CrossingType.Bridge, RoadType.StateRoad) => 2_000_000m,
            (CrossingType.Bridge, RoadType.FederalRoad) => 3_000_000m,
            (CrossingType.Bridge, RoadType.Autobahn) => 5_000_000m,

            // Tunnels (Railway under road)
            (CrossingType.Tunnel, RoadType.LocalRoad) => 8_000_000m,
            (CrossingType.Tunnel, RoadType.CountyRoad) => 10_000_000m,
            (CrossingType.Tunnel, RoadType.StateRoad) => 12_000_000m,
            (CrossingType.Tunnel, RoadType.FederalRoad) => 15_000_000m,
            (CrossingType.Tunnel, RoadType.Autobahn) => 25_000_000m,

            // Underpass (Road under railway)
            (CrossingType.Underpass, _) => 2_000_000m,

            _ => 1_000_000m // Default
        };
    }

    /// <summary>
    /// Estimates traffic volume based on road classification.
    /// </summary>
    private int EstimateTrafficVolume(OsmRoad road)
    {
        var roadType = ClassifyRoad(road.Highway);
        
        // Base traffic by road type (vehicles per day)
        var baseTraffic = roadType switch
        {
            RoadType.Autobahn => 50000,
            RoadType.FederalRoad => 15000,
            RoadType.StateRoad => 5000,
            RoadType.CountyRoad => 2000,
            RoadType.LocalRoad => 500,
            RoadType.Footpath => 100,
            _ => 1000
        };

        // Adjust by lanes
        if (road.Lanes.HasValue && road.Lanes.Value > 2)
        {
            baseTraffic *= road.Lanes.Value / 2;
        }

        return baseTraffic;
    }

    /// <summary>
    /// Finds intersection points between railway route and road.
    /// </summary>
    private List<GeoPoint> FindIntersections(List<GeoPoint> route, List<GeoPoint> road)
    {
        var intersections = new List<GeoPoint>();

        for (int i = 0; i < route.Count - 1; i++)
        {
            var r1 = route[i];
            var r2 = route[i + 1];

            for (int j = 0; j < road.Count - 1; j++)
            {
                var rd1 = road[j];
                var rd2 = road[j + 1];

                var intersection = LineIntersection(r1, r2, rd1, rd2);
                if (intersection != null)
                {
                    intersections.Add(intersection);
                }
            }
        }

        return intersections;
    }

    /// <summary>
    /// Calculates line-line intersection.
    /// </summary>
    private GeoPoint? LineIntersection(GeoPoint p1, GeoPoint p2, GeoPoint p3, GeoPoint p4)
    {
        var x1 = p1.Longitude;
        var y1 = p1.Latitude;
        var x2 = p2.Longitude;
        var y2 = p2.Latitude;
        var x3 = p3.Longitude;
        var y3 = p3.Latitude;
        var x4 = p4.Longitude;
        var y4 = p4.Latitude;

        var denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (Math.Abs(denom) < 1e-10)
            return null; // Parallel lines

        var t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        var u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

        if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
        {
            return new GeoPoint(
                y1 + t * (y2 - y1),
                x1 + t * (x2 - x1)
            );
        }

        return null;
    }

    private BoundingBox CalculateBoundingBox(List<GeoPoint> points, double bufferMeters)
    {
        var minLat = points.Min(p => p.Latitude);
        var maxLat = points.Max(p => p.Latitude);
        var minLon = points.Min(p => p.Longitude);
        var maxLon = points.Max(p => p.Longitude);

        // Add buffer (approximate: 1 degree ≈ 111 km)
        var bufferDegrees = bufferMeters / 111000.0;

        return new BoundingBox
        {
            MinLat = minLat - bufferDegrees,
            MaxLat = maxLat + bufferDegrees,
            MinLon = minLon - bufferDegrees,
            MaxLon = maxLon + bufferDegrees
        };
    }

    private int? ParseInt(string? value) =>
        int.TryParse(value, out var result) ? result : null;

    private double? ParseDouble(string? value) =>
        double.TryParse(value, out var result) ? result : null;
}

// ============================================================================
// Data Models
// ============================================================================

public enum RoadType
{
    Footpath = 0,       // Feldweg
    LocalRoad = 1,      // Gemeindestraße
    CountyRoad = 2,     // Kreisstraße
    StateRoad = 3,      // Landesstraße
    FederalRoad = 4,    // Bundesstraße
    Autobahn = 5        // Autobahn
}

public enum CrossingType
{
    LevelCrossing,      // Bahnübergang (schienengleich)
    Bridge,             // Brücke (Bahn über Straße)
    Tunnel,             // Tunnel (Bahn unter Straße)
    Underpass           // Unterführung (Straße unter Bahn)
}

public class RoadCrossing
{
    public GeoPoint Location { get; set; } = null!;
    public long RoadId { get; set; }
    public string RoadName { get; set; } = "";
    public RoadType RoadType { get; set; }
    public string? Highway { get; set; }
    public CrossingType CrossingType { get; set; }
    public decimal EstimatedCost { get; set; }
    public bool RequiresPermit { get; set; }
    public int TrafficVolume { get; set; }
    public double? MaxHeight { get; set; }
    public int? Lanes { get; set; }
}

public class OsmRoad
{
    public long Id { get; set; }
    public string? Name { get; set; }
    public string? Highway { get; set; }
    public string? Waterway { get; set; }
    public string? Railway { get; set; }
    public int? Lanes { get; set; }
    public double? MaxHeight { get; set; }
    public List<GeoPoint> Geometry { get; set; } = new();
}

public class BoundingBox
{
    public double MinLat { get; set; }
    public double MaxLat { get; set; }
    public double MinLon { get; set; }
    public double MaxLon { get; set; }
}

public record GeoPoint(double Latitude, double Longitude);

// Overpass API response models
internal class OverpassResult
{
    public List<OverpassElement> Elements { get; set; } = new();
}

internal class OverpassElement
{
    public string Type { get; set; } = "";
    public long Id { get; set; }
    public Dictionary<string, string> Tags { get; set; } = new();
    public List<OverpassGeo> Geometry { get; set; } = new();
}

internal class OverpassGeo
{
    public double Lat { get; set; }
    public double Lon { get; set; }
}
