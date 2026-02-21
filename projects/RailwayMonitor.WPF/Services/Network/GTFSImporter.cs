/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GTFSImporter.cs                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     365                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Services.Network;

/// <summary>
/// GTFS (General Transit Feed Specification) Parser for DB data
/// Sprint 1, US-1.1: Data Import
/// </summary>
public class GTFSImporter
{
    private readonly HttpClient _httpClient;
    
    public GTFSImporter(HttpClient httpClient)
    {
        _httpClient = httpClient;
    }
    
    /// <summary>
    /// Import network from GTFS data
    /// </summary>
    public async Task<RailwayNetworkAnalyzer> ImportFromGTFSAsync(string gtfsPath)
    {
        var analyzer = new RailwayNetworkAnalyzer();
        
        // GTFS consists of multiple CSV files
        var stopsPath = Path.Combine(gtfsPath, "stops.txt");
        var routesPath = Path.Combine(gtfsPath, "routes.txt");
        
        if (!File.Exists(stopsPath))
            throw new FileNotFoundException($"GTFS stops.txt not found at {stopsPath}");
        
        // Parse stops (stations)
        var stations = await ParseStopsAsync(stopsPath);
        foreach (var station in stations)
        {
            analyzer.AddStation(station);
        }
        
        // Parse routes and create edges
        // Note: GTFS doesn't directly provide station-to-station edges
        // We need to infer them from stop_times.txt
        var stopTimesPath = Path.Combine(gtfsPath, "stop_times.txt");
        if (File.Exists(stopTimesPath))
        {
            var edges = await ParseRoutesFromStopTimesAsync(stopTimesPath, stations);
            foreach (var edge in edges)
            {
                try
                {
                    analyzer.AddConnection(edge.From.Id, edge.To.Id, edge);
                }
                catch
                {
                    // Skip invalid connections
                }
            }
        }
        
        return analyzer;
    }
    
    /// <summary>
    /// Parse stops.txt file
    /// </summary>
    private async Task<List<Station>> ParseStopsAsync(string filePath)
    {
        var stations = new List<Station>();
        var lines = await File.ReadAllLinesAsync(filePath);
        
        if (lines.Length < 2)
            return stations;
        
        // Parse header
        var header = lines[0].Split(',');
        var stopIdIdx = Array.IndexOf(header, "stop_id");
        var stopNameIdx = Array.IndexOf(header, "stop_name");
        var stopLatIdx = Array.IndexOf(header, "stop_lat");
        var stopLonIdx = Array.IndexOf(header, "stop_lon");
        var stopCodeIdx = Array.IndexOf(header, "stop_code");
        
        // Parse data rows
        for (int i = 1; i < lines.Length; i++)
        {
            var parts = ParseCSVLine(lines[i]);
            
            if (parts.Length <= Math.Max(stopIdIdx, stopNameIdx))
                continue;
            
            var station = new Station
            {
                Id = parts[stopIdIdx].Trim(),
                Name = parts[stopNameIdx].Trim().Replace("\"", ""),
                Code = stopCodeIdx >= 0 && parts.Length > stopCodeIdx ? parts[stopCodeIdx].Trim() : "",
                Latitude = stopLatIdx >= 0 && parts.Length > stopLatIdx && double.TryParse(parts[stopLatIdx], out var lat) ? lat : 0,
                Longitude = stopLonIdx >= 0 && parts.Length > stopLonIdx && double.TryParse(parts[stopLonIdx], out var lon) ? lon : 0,
                Type = ClassifyStationType(parts[stopNameIdx])
            };
            
            stations.Add(station);
        }
        
        return stations;
    }
    
    /// <summary>
    /// Parse stop_times.txt to infer edges
    /// </summary>
    private async Task<List<RailwayEdge>> ParseRoutesFromStopTimesAsync(string filePath, List<Station> stations)
    {
        var edges = new List<RailwayEdge>();
        var stationDict = stations.ToDictionary(s => s.Id);
        
        var lines = await File.ReadAllLinesAsync(filePath);
        if (lines.Length < 2)
            return edges;
        
        // Parse header
        var header = lines[0].Split(',');
        var tripIdIdx = Array.IndexOf(header, "trip_id");
        var stopIdIdx = Array.IndexOf(header, "stop_id");
        var stopSequenceIdx = Array.IndexOf(header, "stop_sequence");
        
        // Group by trip to find consecutive stops
        var tripStops = new Dictionary<string, List<(int sequence, string stopId)>>();
        
        for (int i = 1; i < Math.Min(lines.Length, 10000); i++) // Limit for performance
        {
            var parts = ParseCSVLine(lines[i]);
            
            if (parts.Length <= Math.Max(tripIdIdx, stopIdIdx))
                continue;
            
            var tripId = parts[tripIdIdx].Trim();
            var stopId = parts[stopIdIdx].Trim();
            var sequence = stopSequenceIdx >= 0 && parts.Length > stopSequenceIdx && int.TryParse(parts[stopSequenceIdx], out var seq) ? seq : 0;
            
            if (!tripStops.ContainsKey(tripId))
                tripStops[tripId] = new List<(int, string)>();
                
            tripStops[tripId].Add((sequence, stopId));
        }
        
        // Create edges from consecutive stops
        var edgeSet = new HashSet<string>(); // Avoid duplicates
        
        foreach (var trip in tripStops.Values)
        {
            var sorted = trip.OrderBy(t => t.sequence).ToList();
            
            for (int i = 0; i < sorted.Count - 1; i++)
            {
                var fromId = sorted[i].stopId;
                var toId = sorted[i + 1].stopId;
                
                if (!stationDict.ContainsKey(fromId) || !stationDict.ContainsKey(toId))
                    continue;
                
                var edgeKey = $"{fromId}_{toId}";
                if (edgeSet.Contains(edgeKey))
                    continue;
                
                edgeSet.Add(edgeKey);
                
                var fromStation = stationDict[fromId];
                var toStation = stationDict[toId];
                
                var distance = CalculateDistance(
                    fromStation.Latitude, fromStation.Longitude,
                    toStation.Latitude, toStation.Longitude
                );
                
                edges.Add(new RailwayEdge
                {
                    From = fromStation,
                    To = toStation,
                    LengthKm = distance,
                    MaxSpeedKmh = 160, // Default, would need additional data
                    TrackCount = 2,
                    IsElectrified = true
                });
            }
        }
        
        return edges;
    }
    
    /// <summary>
    /// Classify station type based on name
    /// </summary>
    private StationType ClassifyStationType(string name)
    {
        name = name.ToLower();
        
        if (name.Contains("hbf") || name.Contains("hauptbahnhof"))
            return StationType.Hauptbahnhof;
        
        if (name.Contains("bf") || name.Contains("bahnhof"))
            return StationType.Regionalbahnhof;
        
        return StationType.Haltepunkt;
    }
    
    /// <summary>
    /// Calculate distance between two coordinates (Haversine formula)
    /// </summary>
    private double CalculateDistance(double lat1, double lon1, double lat2, double lon2)
    {
        const double R = 6371; // Earth radius in km
        
        var dLat = (lat2 - lat1) * Math.PI / 180.0;
        var dLon = (lon2 - lon1) * Math.PI / 180.0;
        
        var a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) +
                Math.Cos(lat1 * Math.PI / 180.0) * Math.Cos(lat2 * Math.PI / 180.0) *
                Math.Sin(dLon / 2) * Math.Sin(dLon / 2);
        
        var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
        
        return R * c;
    }
    
    /// <summary>
    /// Parse CSV line handling quoted fields
    /// </summary>
    private string[] ParseCSVLine(string line)
    {
        var result = new List<string>();
        var current = "";
        var inQuotes = false;
        
        for (int i = 0; i < line.Length; i++)
        {
            var c = line[i];
            
            if (c == '"')
            {
                inQuotes = !inQuotes;
            }
            else if (c == ',' && !inQuotes)
            {
                result.Add(current);
                current = "";
            }
            else
            {
                current += c;
            }
        }
        
        result.Add(current);
        return result.ToArray();
    }
}

/// <summary>
/// Sample data generator for testing
/// Generates a simplified network of major German cities
/// </summary>
public class SampleNetworkGenerator
{
    public static RailwayNetworkAnalyzer GenerateSampleNetwork()
    {
        var analyzer = new RailwayNetworkAnalyzer();
        
        // Major German stations
        var stations = new[]
        {
            new Station { Id = "8000105", Code = "FF", Name = "Frankfurt (Main) Hbf", Latitude = 50.1069, Longitude = 8.6623, Type = StationType.Hauptbahnhof, TrackCount = 24, PlatformCount = 15, PassengersPerDay = 450000 },
            new Station { Id = "8000260", Code = "M", Name = "München Hbf", Latitude = 48.1408, Longitude = 11.5556, Type = StationType.Hauptbahnhof, TrackCount = 32, PlatformCount = 16, PassengersPerDay = 413000 },
            new Station { Id = "8098160", Code = "BL", Name = "Berlin Hbf", Latitude = 52.5250, Longitude = 13.3694, Type = StationType.Hauptbahnhof, TrackCount = 14, PlatformCount = 8, PassengersPerDay = 300000 },
            new Station { Id = "8000191", Code = "HH", Name = "Hamburg Hbf", Latitude = 53.5528, Longitude = 10.0067, Type = StationType.Hauptbahnhof, TrackCount = 14, PlatformCount = 8, PassengersPerDay = 537000 },
            new Station { Id = "8000085", Code = "EE", Name = "Essen Hbf", Latitude = 51.4508, Longitude = 7.0131, Type = StationType.Hauptbahnhof, TrackCount = 12, PlatformCount = 7, PassengersPerDay = 220000 },
            new Station { Id = "8000207", Code = "KK", Name = "Köln Hbf", Latitude = 50.9429, Longitude = 6.9589, Type = StationType.Hauptbahnhof, TrackCount = 11, PlatformCount = 9, PassengersPerDay = 280000 },
            new Station { Id = "8000096", Code = "SSTU", Name = "Stuttgart Hbf", Latitude = 48.7839, Longitude = 9.1818, Type = StationType.Hauptbahnhof, TrackCount = 16, PlatformCount = 9, PassengersPerDay = 255000 },
            new Station { Id = "8010205", Code = "LH", Name = "Hannover Hbf", Latitude = 52.3769, Longitude = 9.7417, Type = StationType.Hauptbahnhof, TrackCount = 12, PlatformCount = 6, PassengersPerDay = 250000 },
        };
        
        foreach (var station in stations)
        {
            analyzer.AddStation(station);
        }
        
        // Major connections (ICE routes)
        var connections = new[]
        {
            // Frankfurt-München (ICE 1)
            ("8000105", "8000260", 393.0, 300),
            // Frankfurt-Köln (ICE 10)
            ("8000105", "8000207", 177.0, 300),
            // Köln-Berlin (ICE 10)
            ("8000207", "8098160", 577.0, 300),
            // Hamburg-Berlin (ICE 11)
            ("8000191", "8098160", 287.0, 230),
            // Hamburg-Köln (ICE 26)
            ("8000191", "8000207", 426.0, 230),
            // Frankfurt-Stuttgart (ICE 11)
            ("8000105", "8000096", 152.0, 250),
            // Stuttgart-München (ICE 11)
            ("8000096", "8000260", 250.0, 230),
            // Hannover-Berlin (ICE 10)
            ("8010205", "8098160", 286.0, 230),
            // Essen-Köln
            ("8000085", "8000207", 81.0, 200),
        };
        
        foreach (var (from, to, length, speed) in connections)
        {
            // Add bi-directional edges
            analyzer.AddConnection(from, to, new RailwayEdge
            {
                LengthKm = length,
                MaxSpeedKmh = speed,
                IsHighSpeed = speed >= 250,
                TrackCount = 2,
                IsElectrified = true,
                HasETCS = speed >= 250,
                MaxCapacityTrainsPerHour = 12
            });
            
            analyzer.AddConnection(to, from, new RailwayEdge
            {
                LengthKm = length,
                MaxSpeedKmh = speed,
                IsHighSpeed = speed >= 250,
                TrackCount = 2,
                IsElectrified = true,
                HasETCS = speed >= 250,
                MaxCapacityTrainsPerHour = 12
            });
        }
        
        return analyzer;
    }
}
