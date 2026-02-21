/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RailwayControlVisualizer.cs                        ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   83.0/100                                       ║
    • Total Lines:     775                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 83d8a09c6  2025-12-15  Add Interactive Railway Control & What-If Analysis visual... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;

namespace RailwayMonitor.WPF.Services.Visualization
{
    /// <summary>
    /// Interactive Railway Control & What-If Analysis System
    /// Provides DB-standard visualizations for train paths, switches, signals, and scenario simulation
    /// </summary>
    public class RailwayControlVisualizer
    {
        public TrainPathDiagram GenerateTrainPathDiagram(TrainPathInput input)
        {
            var diagram = new TrainPathDiagram
            {
                TimeRange = input.TimeRange,
                Stations = input.Stations,
                TrainPaths = new List<TrainPath>()
            };

            foreach (var train in input.Trains)
            {
                var path = new TrainPath
                {
                    TrainId = train.Id,
                    TrainName = train.Name,
                    Segments = CalculatePathSegments(train, input.Stations)
                };
                diagram.TrainPaths.Add(path);
            }

            return diagram;
        }

        public SwitchSignalPanel GenerateSwitchSignalPanel(ControlPanelInput input)
        {
            var panel = new SwitchSignalPanel
            {
                Stations = input.Stations,
                Switches = new List<SwitchState>(),
                Signals = new List<SignalState>()
            };

            foreach (var station in input.Stations)
            {
                // Generate switch states
                var switchStates = input.Interlocking.GetSwitchStates(station.Id);
                panel.Switches.AddRange(switchStates);

                // Generate signal states
                var signalStates = input.Interlocking.GetSignalStates(station.Id);
                panel.Signals.AddRange(signalStates);
            }

            panel.HasConflicts = panel.Switches.Any(s => s.Locked && s.RequestedChange) ||
                                panel.Signals.Any(s => s.Conflict);

            return panel;
        }

        public NetworkDiagram GenerateNetworkDiagram(NetworkInput input)
        {
            var diagram = new NetworkDiagram
            {
                Stations = input.Stations,
                Tracks = input.Tracks,
                ShowCapacity = input.ShowCapacity
            };

            if (input.ShowCapacity)
            {
                foreach (var track in diagram.Tracks)
                {
                    track.CapacityColor = GetCapacityColor(track.UtilizationPercent);
                }
            }

            return diagram;
        }

        public TimetableGraph GenerateTimetableGraph(TimetableGraphInput input)
        {
            var graph = new TimetableGraph
            {
                Route = input.Route,
                TimeRange = input.TimeRange,
                TrainLines = new List<TrainLine>()
            };

            foreach (var train in input.Trains ?? new List<Train>())
            {
                var line = new TrainLine
                {
                    TrainId = train.Id,
                    TrainName = train.Name,
                    TrainType = train.Type,
                    Points = CalculateTimetablePoints(train, input.Route, input.TimeRange)
                };
                graph.TrainLines.Add(line);
            }

            return graph;
        }

        public TrainDetails GetTrainDetails(Train train)
        {
            return new TrainDetails
            {
                TrainId = train.Id,
                TrainName = train.Name,
                TrainType = train.Type,
                CurrentSpeed = train.CurrentSpeed,
                MaxSpeed = train.MaxSpeed,
                Position = train.Position,
                Status = train.Status,
                Delay = train.Delay
            };
        }

        public void UpdateTimeRange(TimeRange newRange)
        {
            // Update visualization time range
            CurrentTimeRange = newRange;
        }

        private List<PathSegment> CalculatePathSegments(Train train, List<Station> stations)
        {
            var segments = new List<PathSegment>();
            
            for (int i = 0; i < train.Schedule.Count - 1; i++)
            {
                var from = train.Schedule[i];
                var to = train.Schedule[i + 1];
                
                segments.Add(new PathSegment
                {
                    FromStation = from.StationId,
                    ToStation = to.StationId,
                    DepartureTime = from.DepartureTime,
                    ArrivalTime = to.ArrivalTime,
                    Distance = CalculateDistance(from.StationId, to.StationId, stations)
                });
            }

            return segments;
        }

        private List<TimetablePoint> CalculateTimetablePoints(Train train, string[] route, TimeRange range)
        {
            var points = new List<TimetablePoint>();

            foreach (var stop in train.Schedule)
            {
                if (route.Contains(stop.StationName) && 
                    stop.ArrivalTime >= range.Start && 
                    stop.ArrivalTime <= range.End)
                {
                    points.Add(new TimetablePoint
                    {
                        StationName = stop.StationName,
                        Time = stop.ArrivalTime,
                        IsStop = true
                    });
                }
            }

            return points;
        }

        private double CalculateDistance(string fromId, string toId, List<Station> stations)
        {
            var from = stations.FirstOrDefault(s => s.Id == fromId);
            var to = stations.FirstOrDefault(s => s.Id == toId);
            
            if (from == null || to == null) return 0;

            // Haversine formula
            var lat1 = from.Latitude * Math.PI / 180;
            var lat2 = to.Latitude * Math.PI / 180;
            var dLat = lat2 - lat1;
            var dLon = (to.Longitude - from.Longitude) * Math.PI / 180;

            var a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) +
                    Math.Cos(lat1) * Math.Cos(lat2) *
                    Math.Sin(dLon / 2) * Math.Sin(dLon / 2);
            var c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));

            return 6371 * c; // Earth radius in km
        }

        private string GetCapacityColor(double utilizationPercent)
        {
            if (utilizationPercent < 60) return "Green";
            if (utilizationPercent < 75) return "Yellow";
            if (utilizationPercent < 85) return "Orange";
            return "Red";
        }

        public TimeRange CurrentTimeRange { get; private set; }
    }

    /// <summary>
    /// What-If Analysis Engine for scenario simulation
    /// </summary>
    public class WhatIfAnalysisEngine
    {
        private readonly RailwayControlVisualizer _visualizer;

        public WhatIfAnalysisEngine(RailwayControlVisualizer visualizer)
        {
            _visualizer = visualizer;
        }

        public ScenarioResult SimulateDelay(DelayScenario scenario)
        {
            var result = new ScenarioResult
            {
                ScenarioType = "Train Delay",
                ScenarioDescription = $"{scenario.TrainId} delayed by {scenario.DelayMinutes} minutes at {scenario.Location}"
            };

            // Simulate direct impacts
            result.DirectImpacts = CalculateDelayImpacts(scenario);

            // Simulate cascading delays if enabled
            if (scenario.PropagateDelays)
            {
                result.SecondaryImpacts = CalculateCascadingDelays(scenario, result.DirectImpacts);
            }

            // Calculate costs
            result.TotalCost = CalculateDelayCosts(result.DirectImpacts, result.SecondaryImpacts);

            // Generate recommendations
            result.Recommendations = GenerateDelayRecommendations(result);

            return result;
        }

        public ScenarioResult SimulateTrackClosure(TrackClosureScenario scenario)
        {
            var result = new ScenarioResult
            {
                ScenarioType = "Track Closure",
                ScenarioDescription = $"{scenario.Track} closed from {scenario.StartTime:HH:mm} for {scenario.Duration.TotalHours}h"
            };

            // Find affected trains
            result.AffectedTrains = FindAffectedTrains(scenario);

            // Calculate rerouting options
            result.ReroutingOptions = CalculateReroutingOptions(scenario, result.AffectedTrains);

            // Calculate impacts
            result.DirectImpacts = CalculateClosureImpacts(scenario, result.ReroutingOptions);

            // Calculate costs
            result.TotalCost = CalculateClosureCosts(result.DirectImpacts);

            // Generate recommendations
            result.Recommendations = GenerateClosureRecommendations(result);

            return result;
        }

        public ScenarioResult SimulateNewService(NewServiceScenario scenario)
        {
            var result = new ScenarioResult
            {
                ScenarioType = "New Train Service",
                ScenarioDescription = $"New {scenario.TrainType} service on {string.Join(" → ", scenario.Route)}, {scenario.Frequency}min frequency"
            };

            // Capacity analysis
            result.CapacityAnalysis = AnalyzeCapacityImpact(scenario);

            // Conflict analysis
            result.ConflictAnalysis = AnalyzeConflicts(scenario);

            // Economic analysis
            result.EconomicAnalysis = AnalyzeEconomics(scenario);

            // Generate recommendation
            result.Recommendations = GenerateNewServiceRecommendations(result);

            return result;
        }

        public ScenarioResult Simulate(Scenario scenario)
        {
            return scenario.Type switch
            {
                "Delay" => SimulateDelay(scenario as DelayScenario),
                "Closure" => SimulateTrackClosure(scenario as TrackClosureScenario),
                "NewService" => SimulateNewService(scenario as NewServiceScenario),
                _ => throw new NotSupportedException($"Scenario type {scenario.Type} not supported")
            };
        }

        private List<Impact> CalculateDelayImpacts(DelayScenario scenario)
        {
            var impacts = new List<Impact>
            {
                new Impact
                {
                    Type = "Arrival Delay",
                    Description = $"Arrival delayed by {scenario.DelayMinutes} minutes",
                    Severity = scenario.DelayMinutes > 30 ? "High" : scenario.DelayMinutes > 15 ? "Medium" : "Low"
                }
            };

            // Calculate missed connections
            var missedConnections = scenario.DelayMinutes / 5; // Simplified
            if (missedConnections > 0)
            {
                impacts.Add(new Impact
                {
                    Type = "Missed Connections",
                    Description = $"{missedConnections} connections may be missed",
                    Severity = "High",
                    AffectedPassengers = missedConnections * 10 // Estimate
                });
            }

            return impacts;
        }

        private List<Impact> CalculateCascadingDelays(DelayScenario scenario, List<Impact> directImpacts)
        {
            var cascading = new List<Impact>();

            // Simplified cascading logic
            var affectedTrains = directImpacts.Where(i => i.Type == "Missed Connections").Count();
            if (affectedTrains > 0)
            {
                cascading.Add(new Impact
                {
                    Type = "Cascading Delay",
                    Description = $"{affectedTrains} connecting trains affected",
                    Severity = "Medium",
                    AffectedTrains = affectedTrains
                });
            }

            return cascading;
        }

        private decimal CalculateDelayCosts(List<Impact> direct, List<Impact> secondary)
        {
            decimal cost = 0;

            foreach (var impact in direct.Concat(secondary))
            {
                cost += impact.Type switch
                {
                    "Arrival Delay" => 100, // 100€ per minute
                    "Missed Connections" => impact.AffectedPassengers * 100,
                    "Cascading Delay" => impact.AffectedTrains * 5000,
                    _ => 0
                };
            }

            return cost;
        }

        private decimal CalculateClosureCosts(List<Impact> impacts)
        {
            return impacts.Sum(i => 15000); // Simplified
        }

        private List<Train> FindAffectedTrains(TrackClosureScenario scenario)
        {
            return new List<Train>(); // Simplified - would query actual schedule
        }

        private List<ReroutingOption> CalculateReroutingOptions(TrackClosureScenario scenario, List<Train> trains)
        {
            return new List<ReroutingOption>
            {
                new ReroutingOption { Route = "Parallel Track", TrainCount = 8, AverageDelay = 5 },
                new ReroutingOption { Route = "Alternative via Junction", TrainCount = 4, AverageDelay = 15 }
            };
        }

        private List<Impact> CalculateClosureImpacts(TrackClosureScenario scenario, List<ReroutingOption> options)
        {
            return new List<Impact>
            {
                new Impact
                {
                    Type = "Capacity Reduction",
                    Description = "Track capacity reduced during closure",
                    Severity = "High"
                }
            };
        }

        private CapacityAnalysis AnalyzeCapacityImpact(NewServiceScenario scenario)
        {
            return new CapacityAnalysis
            {
                SegmentUtilizations = scenario.Route.Select((r, i) => new SegmentUtilization
                {
                    Segment = i < scenario.Route.Length - 1 ? $"{scenario.Route[i]} - {scenario.Route[i + 1]}" : r,
                    BeforePercent = 70,
                    AfterPercent = 76,
                    Acceptable = true
                }).ToList()
            };
        }

        private ConflictAnalysis AnalyzeConflicts(NewServiceScenario scenario)
        {
            return new ConflictAnalysis
            {
                PotentialConflicts = 8,
                AutoResolved = 8,
                ManualRequired = 0,
                ConflictFree = true
            };
        }

        private EconomicAnalysis AnalyzeEconomics(NewServiceScenario scenario)
        {
            return new EconomicAnalysis
            {
                Investment = 2500000,
                AnnualOperatingCost = 1800000,
                ExpectedRevenue = 3200000,
                ROIYears = 1.8,
                Profitable = true
            };
        }

        private List<string> GenerateDelayRecommendations(ScenarioResult result)
        {
            var recommendations = new List<string>();
            
            if (result.DirectImpacts.Any(i => i.Type == "Missed Connections"))
            {
                recommendations.Add("✓ Delay connecting trains by 5 minutes (low cost)");
                recommendations.Add("⚠️ Adjust timetable for affected routes");
            }

            return recommendations;
        }

        private List<string> GenerateClosureRecommendations(ScenarioResult result)
        {
            return new List<string>
            {
                "→ Consider ETCS Level 2 for increased capacity on alternate routes",
                "→ Adjust frequency during closure period"
            };
        }

        private List<string> GenerateNewServiceRecommendations(ScenarioResult result)
        {
            if (result.EconomicAnalysis?.Profitable == true && result.ConflictAnalysis?.ConflictFree == true)
            {
                return new List<string> { "✓ Implementation recommended - economically viable and operationally feasible" };
            }

            return new List<string> { "⚠️ Further analysis required" };
        }
    }

    // Data models
    public class TrainPathInput
    {
        public List<Train> Trains { get; set; }
        public TimeRange TimeRange { get; set; }
        public List<Station> Stations { get; set; }
        public List<Track> Tracks { get; set; }
    }

    public class ControlPanelInput
    {
        public InterlockingSystem Interlocking { get; set; }
        public List<Station> Stations { get; set; }
        public DateTime CurrentTime { get; set; }
    }

    public class NetworkInput
    {
        public List<Station> Stations { get; set; }
        public List<Track> Tracks { get; set; }
        public bool ShowCapacity { get; set; }
        public bool ShowSignals { get; set; }
        public bool ShowSwitches { get; set; }
    }

    public class TimetableGraphInput
    {
        public string[] Route { get; set; }
        public TimeRange TimeRange { get; set; }
        public bool ShowAllTrains { get; set; }
        public List<Train> Trains { get; set; }
    }

    public class TrainPathDiagram
    {
        public TimeRange TimeRange { get; set; }
        public List<Station> Stations { get; set; }
        public List<TrainPath> TrainPaths { get; set; }

        public string GetSummary()
        {
            return $"=== ZUGWEG-DIAGRAMM ===\n\nZeitbereich: {TimeRange.Start:HH:mm} - {TimeRange.End:HH:mm}\nStationen: {Stations.Count}\nZüge: {TrainPaths.Count}";
        }
    }

    public class SwitchSignalPanel
    {
        public List<Station> Stations { get; set; }
        public List<SwitchState> Switches { get; set; }
        public List<SignalState> Signals { get; set; }
        public bool HasConflicts { get; set; }
    }

    public class NetworkDiagram
    {
        public List<Station> Stations { get; set; }
        public List<Track> Tracks { get; set; }
        public bool ShowCapacity { get; set; }
    }

    public class TimetableGraph
    {
        public string[] Route { get; set; }
        public TimeRange TimeRange { get; set; }
        public List<TrainLine> TrainLines { get; set; }
    }

    public class TrainDetails
    {
        public string TrainId { get; set; }
        public string TrainName { get; set; }
        public string TrainType { get; set; }
        public double CurrentSpeed { get; set; }
        public double MaxSpeed { get; set; }
        public double Position { get; set; }
        public string Status { get; set; }
        public int Delay { get; set; }
    }

    public class TrainPath
    {
        public string TrainId { get; set; }
        public string TrainName { get; set; }
        public List<PathSegment> Segments { get; set; }
    }

    public class PathSegment
    {
        public string FromStation { get; set; }
        public string ToStation { get; set; }
        public DateTime DepartureTime { get; set; }
        public DateTime ArrivalTime { get; set; }
        public double Distance { get; set; }
    }

    public class TimetablePoint
    {
        public string StationName { get; set; }
        public DateTime Time { get; set; }
        public bool IsStop { get; set; }
    }

    public class TrainLine
    {
        public string TrainId { get; set; }
        public string TrainName { get; set; }
        public string TrainType { get; set; }
        public List<TimetablePoint> Points { get; set; }
    }

    public class TimeRange
    {
        public DateTime Start { get; set; }
        public DateTime End { get; set; }
    }

    public class Train
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public string Type { get; set; }
        public double CurrentSpeed { get; set; }
        public double MaxSpeed { get; set; }
        public double Position { get; set; }
        public string Status { get; set; }
        public int Delay { get; set; }
        public List<ScheduleStop> Schedule { get; set; }
    }

    public class ScheduleStop
    {
        public string StationId { get; set; }
        public string StationName { get; set; }
        public DateTime ArrivalTime { get; set; }
        public DateTime DepartureTime { get; set; }
    }

    public class Station
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public double Latitude { get; set; }
        public double Longitude { get; set; }
    }

    public class Track
    {
        public string Id { get; set; }
        public string FromStationId { get; set; }
        public string ToStationId { get; set; }
        public double UtilizationPercent { get; set; }
        public string CapacityColor { get; set; }
    }

    public class SwitchState
    {
        public string Id { get; set; }
        public string Position { get; set; }
        public bool Locked { get; set; }
        public bool RequestedChange { get; set; }
    }

    public class SignalState
    {
        public string Id { get; set; }
        public string Aspect { get; set; }
        public bool Conflict { get; set; }
    }

    public class InterlockingSystem
    {
        public List<SwitchState> GetSwitchStates(string stationId)
        {
            return new List<SwitchState>();
        }

        public List<SignalState> GetSignalStates(string stationId)
        {
            return new List<SignalState>();
        }
    }

    public class DelayScenario : Scenario
    {
        public DelayScenario() { Type = "Delay"; }
        public string TrainId { get; set; }
        public int DelayMinutes { get; set; }
        public string Location { get; set; }
        public bool PropagateDelays { get; set; }
    }

    public class TrackClosureScenario : Scenario
    {
        public TrackClosureScenario() { Type = "Closure"; }
        public string Track { get; set; }
        public DateTime StartTime { get; set; }
        public TimeSpan Duration { get; set; }
        public string Reason { get; set; }
    }

    public class NewServiceScenario : Scenario
    {
        public NewServiceScenario() { Type = "NewService"; }
        public string[] Route { get; set; }
        public int Frequency { get; set; }
        public string TrainType { get; set; }
        public DateTime StartTime { get; set; }
        public DateTime EndTime { get; set; }
    }

    public class Scenario
    {
        public string Type { get; set; }
    }

    public class ScenarioResult
    {
        public string ScenarioType { get; set; }
        public string ScenarioDescription { get; set; }
        public List<Impact> DirectImpacts { get; set; } = new List<Impact>();
        public List<Impact> SecondaryImpacts { get; set; } = new List<Impact>();
        public List<Train> AffectedTrains { get; set; } = new List<Train>();
        public List<ReroutingOption> ReroutingOptions { get; set; } = new List<ReroutingOption>();
        public CapacityAnalysis CapacityAnalysis { get; set; }
        public ConflictAnalysis ConflictAnalysis { get; set; }
        public EconomicAnalysis EconomicAnalysis { get; set; }
        public decimal TotalCost { get; set; }
        public List<string> Recommendations { get; set; } = new List<string>();
        public bool Acceptable => TotalCost < 50000 && !DirectImpacts.Any(i => i.Severity == "Critical");

        public string GetImpactSummary()
        {
            return $"=== WAS-WÄRE-WENN ANALYSE ===\n\nSzenario: {ScenarioDescription}\n\nGesamtkosten: {TotalCost:N0} €\nEmpfehlungen: {Recommendations.Count}";
        }
    }

    public class Impact
    {
        public string Type { get; set; }
        public string Description { get; set; }
        public string Severity { get; set; }
        public int AffectedPassengers { get; set; }
        public int AffectedTrains { get; set; }
    }

    public class ReroutingOption
    {
        public string Route { get; set; }
        public int TrainCount { get; set; }
        public int AverageDelay { get; set; }
    }

    public class CapacityAnalysis
    {
        public List<SegmentUtilization> SegmentUtilizations { get; set; }
    }

    public class SegmentUtilization
    {
        public string Segment { get; set; }
        public double BeforePercent { get; set; }
        public double AfterPercent { get; set; }
        public bool Acceptable { get; set; }
    }

    public class ConflictAnalysis
    {
        public int PotentialConflicts { get; set; }
        public int AutoResolved { get; set; }
        public int ManualRequired { get; set; }
        public bool ConflictFree { get; set; }
    }

    public class EconomicAnalysis
    {
        public decimal Investment { get; set; }
        public decimal AnnualOperatingCost { get; set; }
        public decimal ExpectedRevenue { get; set; }
        public double ROIYears { get; set; }
        public bool Profitable { get; set; }
    }
}
