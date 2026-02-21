/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            PESPTimetableOptimizer.cs                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     433                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using Google.OrTools.Sat;

namespace RailwayMonitor.WPF.Services.Timetabling
{
    /// <summary>
    /// PESP (Periodic Event Scheduling Problem) Timetable Optimizer
    /// Implements ILP-based periodic timetable optimization for Deutschland-Takt
    /// </summary>
    public class PESPTimetableOptimizer
    {
        public PESPResult OptimizeTimetable(PESPInput input)
        {
            var validation = ValidateInput(input);
            if (!validation.IsValid)
            {
                return new PESPResult
                {
                    Status = SolverStatus.Invalid,
                    ValidationErrors = validation.Errors
                };
            }

            var model = new CpModel();
            var events = BuildEventGraph(input);
            var times = CreateDecisionVariables(model, events, input.Period);
            
            AddRuntimeConstraints(model, times, events, input);
            AddTransferConstraints(model, times, events, input);
            AddSymmetryConstraints(model, times, events, input);
            
            var objective = BuildObjective(model, times, events, input);
            model.Minimize(objective);

            var solver = new CpSolver();
            solver.StringParameters = "max_time_in_seconds:120.0";
            var status = solver.Solve(model);

            return BuildResult(status, solver, times, events, input);
        }

        private List<Event> BuildEventGraph(PESPInput input)
        {
            var events = new List<Event>();
            int eventId = 0;

            foreach (var connection in input.Connections)
            {
                events.Add(new Event
                {
                    Id = eventId++,
                    Type = EventType.Departure,
                    StationId = connection.FromStation,
                    LineId = connection.LineId
                });

                events.Add(new Event
                {
                    Id = eventId++,
                    Type = EventType.Arrival,
                    StationId = connection.ToStation,
                    LineId = connection.LineId
                });
            }

            return events;
        }

        private Dictionary<int, IntVar> CreateDecisionVariables(CpModel model, List<Event> events, int period)
        {
            var times = new Dictionary<int, IntVar>();
            foreach (var evt in events)
            {
                times[evt.Id] = model.NewIntVar(0, period - 1, $"time_{evt.Id}");
            }
            return times;
        }

        private void AddRuntimeConstraints(CpModel model, Dictionary<int, IntVar> times, List<Event> events, PESPInput input)
        {
            foreach (var connection in input.Connections)
            {
                var depEvent = events.First(e => e.Type == EventType.Departure && 
                                                 e.StationId == connection.FromStation && 
                                                 e.LineId == connection.LineId);
                var arrEvent = events.First(e => e.Type == EventType.Arrival && 
                                                 e.StationId == connection.ToStation && 
                                                 e.LineId == connection.LineId);

                var runtime = connection.RuntimeMinutes;
                
                // (arrival - departure) mod period = runtime
                var diff = model.NewIntVar(0, input.Period - 1, $"runtime_{connection.LineId}");
                model.Add(diff == times[arrEvent.Id] - times[depEvent.Id]).OnlyEnforceIf(
                    model.NewBoolVar($"runtime_pos_{connection.LineId}"));
                model.Add(diff == times[arrEvent.Id] - times[depEvent.Id] + input.Period).OnlyEnforceIf(
                    model.NewBoolVar($"runtime_neg_{connection.LineId}").Not());
                model.Add(diff == runtime);
            }
        }

        private void AddTransferConstraints(CpModel model, Dictionary<int, IntVar> times, List<Event> events, PESPInput input)
        {
            foreach (var transfer in input.TransferRequirements)
            {
                var arrEvent = events.FirstOrDefault(e => e.Type == EventType.Arrival && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.FromLine);
                var depEvent = events.FirstOrDefault(e => e.Type == EventType.Departure && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.ToLine);

                if (arrEvent == null || depEvent == null) continue;

                var transferTime = model.NewIntVar(transfer.MinTransferTime, transfer.MaxTransferTime, 
                                                    $"transfer_{transfer.Station}_{transfer.FromLine}_{transfer.ToLine}");
                
                model.Add(transferTime == times[depEvent.Id] - times[arrEvent.Id]).OnlyEnforceIf(
                    model.NewBoolVar($"transfer_pos").Not());
                model.Add(transferTime == times[depEvent.Id] - times[arrEvent.Id] + input.Period).OnlyEnforceIf(
                    model.NewBoolVar($"transfer_neg"));
            }
        }

        private void AddSymmetryConstraints(CpModel model, Dictionary<int, IntVar> times, List<Event> events, PESPInput input)
        {
            if (input.SymmetryMinutes == null || input.SymmetryMinutes.Count == 0)
                return;

            var symmetryMinute = input.SymmetryMinutes[0]; // e.g., 0 or 30

            foreach (var station in input.SymmetryStations)
            {
                var stationEvents = events.Where(e => e.StationId == station && e.Type == EventType.Departure).ToList();
                
                foreach (var evt in stationEvents)
                {
                    var deviation = model.NewIntVar(0, input.Period / 2, $"symmetry_dev_{evt.Id}");
                    model.AddAbsEquality(deviation, times[evt.Id] - symmetryMinute);
                }
            }
        }

        private LinearExpr BuildObjective(CpModel model, Dictionary<int, IntVar> times, List<Event> events, PESPInput input)
        {
            var terms = new List<LinearExpr>();

            // Transfer time minimization (high priority)
            foreach (var transfer in input.TransferRequirements)
            {
                var arrEvent = events.FirstOrDefault(e => e.Type == EventType.Arrival && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.FromLine);
                var depEvent = events.FirstOrDefault(e => e.Type == EventType.Departure && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.ToLine);

                if (arrEvent == null || depEvent == null) continue;

                var transferTime = model.NewIntVar(0, input.Period, $"obj_transfer_{transfer.Station}");
                model.Add(transferTime >= times[depEvent.Id] - times[arrEvent.Id]);
                model.Add(transferTime >= times[depEvent.Id] - times[arrEvent.Id] + input.Period);
                
                var weight = transfer.PassengerCount * 1000;
                terms.Add(transferTime * weight);
            }

            // Symmetry deviation minimization (medium priority)
            if (input.SymmetryMinutes != null && input.SymmetryMinutes.Count > 0)
            {
                var symmetryMinute = input.SymmetryMinutes[0];
                
                foreach (var station in input.SymmetryStations)
                {
                    var stationEvents = events.Where(e => e.StationId == station && e.Type == EventType.Departure).ToList();
                    
                    foreach (var evt in stationEvents)
                    {
                        var deviation = model.NewIntVar(0, input.Period, $"obj_symmetry_{evt.Id}");
                        model.AddAbsEquality(deviation, times[evt.Id] - symmetryMinute);
                        terms.Add(deviation * 100);
                    }
                }
            }

            return LinearExpr.Sum(terms);
        }

        private PESPResult BuildResult(CpSolverStatus status, CpSolver solver, Dictionary<int, IntVar> times, 
                                       List<Event> events, PESPInput input)
        {
            var result = new PESPResult
            {
                Status = status == CpSolverStatus.Optimal ? SolverStatus.Optimal :
                        status == CpSolverStatus.Feasible ? SolverStatus.Feasible :
                        SolverStatus.Infeasible,
                SolveTimeSeconds = solver.WallTime(),
                Period = input.Period
            };

            if (status == CpSolverStatus.Optimal || status == CpSolverStatus.Feasible)
            {
                result.ScheduledTimes = new Dictionary<int, int>();
                foreach (var kvp in times)
                {
                    result.ScheduledTimes[kvp.Key] = (int)solver.Value(kvp.Value);
                }

                result.SymmetryScore = CalculateSymmetryScore(result, events, input);
                result.TransferQuality = CalculateTransferQuality(result, events, input);
            }

            return result;
        }

        private double CalculateSymmetryScore(PESPResult result, List<Event> events, PESPInput input)
        {
            if (input.SymmetryMinutes == null || input.SymmetryMinutes.Count == 0)
                return 100.0;

            var symmetryMinute = input.SymmetryMinutes[0];
            var deviations = new List<int>();

            foreach (var station in input.SymmetryStations)
            {
                var stationEvents = events.Where(e => e.StationId == station && e.Type == EventType.Departure).ToList();
                
                foreach (var evt in stationEvents)
                {
                    if (result.ScheduledTimes.TryGetValue(evt.Id, out var time))
                    {
                        var deviation = Math.Abs(time - symmetryMinute);
                        deviation = Math.Min(deviation, input.Period - deviation);
                        deviations.Add(deviation);
                    }
                }
            }

            if (deviations.Count == 0) return 100.0;

            var avgDeviation = deviations.Average();
            var maxAcceptableDeviation = input.Period / 4.0;
            var score = Math.Max(0, 100.0 * (1.0 - avgDeviation / maxAcceptableDeviation));
            return score;
        }

        private double CalculateTransferQuality(PESPResult result, List<Event> events, PESPInput input)
        {
            var goodTransfers = 0;
            var totalTransfers = 0;

            foreach (var transfer in input.TransferRequirements)
            {
                var arrEvent = events.FirstOrDefault(e => e.Type == EventType.Arrival && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.FromLine);
                var depEvent = events.FirstOrDefault(e => e.Type == EventType.Departure && 
                                                          e.StationId == transfer.Station && 
                                                          e.LineId == transfer.ToLine);

                if (arrEvent == null || depEvent == null) continue;

                totalTransfers++;

                if (result.ScheduledTimes.TryGetValue(arrEvent.Id, out var arrTime) &&
                    result.ScheduledTimes.TryGetValue(depEvent.Id, out var depTime))
                {
                    var transferTime = (depTime - arrTime + input.Period) % input.Period;
                    if (transferTime <= 10) // Good transfer if ≤10 minutes
                    {
                        goodTransfers++;
                    }
                }
            }

            return totalTransfers > 0 ? (100.0 * goodTransfers / totalTransfers) : 100.0;
        }

        public ValidationResult ValidateInput(PESPInput input)
        {
            var errors = new List<string>();

            if (input.Period <= 0)
                errors.Add("Period must be positive");

            if (input.Period > 240)
                errors.Add("Period must be ≤240 minutes");

            if (input.Stations == null || input.Stations.Count == 0)
                errors.Add("At least one station required");

            if (input.Connections == null || input.Connections.Count == 0)
                errors.Add("At least one connection required");

            foreach (var connection in input.Connections ?? new List<Connection>())
            {
                if (connection.RuntimeMinutes <= 0)
                    errors.Add($"Runtime must be positive for {connection.LineId}");

                if (connection.RuntimeMinutes >= input.Period)
                    errors.Add($"Runtime must be < period for {connection.LineId}");
            }

            return new ValidationResult
            {
                IsValid = errors.Count == 0,
                Errors = errors
            };
        }
    }

    // Data Models

    public class PESPInput
    {
        public int Period { get; set; } = 60; // minutes
        public List<Station> Stations { get; set; } = new();
        public List<Connection> Connections { get; set; } = new();
        public List<TransferRequirement> TransferRequirements { get; set; } = new();
        public List<string> SymmetryStations { get; set; } = new();
        public List<int> SymmetryMinutes { get; set; } = new() { 0, 30 };
        public int HorizonPeriods { get; set; } = 1;
        public DateTime? StartTime { get; set; }
    }

    public class Station
    {
        public string Id { get; set; } = string.Empty;
        public string Name { get; set; } = string.Empty;
        public bool IsSymmetryPoint { get; set; }
    }

    public class Connection
    {
        public string LineId { get; set; } = string.Empty;
        public string FromStation { get; set; } = string.Empty;
        public string ToStation { get; set; } = string.Empty;
        public int RuntimeMinutes { get; set; }
        public int Frequency { get; set; } = 1; // trains per period
    }

    public class TransferRequirement
    {
        public string Station { get; set; } = string.Empty;
        public string FromLine { get; set; } = string.Empty;
        public string ToLine { get; set; } = string.Empty;
        public int MinTransferTime { get; set; } = 3;
        public int MaxTransferTime { get; set; } = 15;
        public int PassengerCount { get; set; } = 100;
    }

    public class PESPResult
    {
        public SolverStatus Status { get; set; }
        public double SolveTimeSeconds { get; set; }
        public int Period { get; set; }
        public Dictionary<int, int> ScheduledTimes { get; set; } = new();
        public double SymmetryScore { get; set; }
        public double TransferQuality { get; set; }
        public List<string> ValidationErrors { get; set; } = new();

        public string GetSummary()
        {
            var summary = $@"=== PESP FAHRPLAN-OPTIMIERUNG ===

Ergebnis:
  Status: {Status}
  Berechnungszeit: {SolveTimeSeconds:F2}s
  Periode: {Period} min
  
Qualität:
  Symmetrie-Score: {SymmetryScore:F1}%
  Anschluss-Qualität: {TransferQuality:F1}%
  
Geplante Zeiten: {ScheduledTimes.Count} Events
";

            if (ValidationErrors.Count > 0)
            {
                summary += "\nFehler:\n";
                foreach (var error in ValidationErrors)
                {
                    summary += $"  ❌ {error}\n";
                }
            }

            if (Status == SolverStatus.Optimal)
            {
                summary += "\n✓ Optimale Lösung gefunden";
            }
            else if (Status == SolverStatus.Feasible)
            {
                summary += "\n⚠️ Zulässige Lösung gefunden (nicht optimal)";
            }
            else
            {
                summary += "\n🔴 Keine Lösung gefunden";
            }

            return summary;
        }
    }

    public class Event
    {
        public int Id { get; set; }
        public EventType Type { get; set; }
        public string StationId { get; set; } = string.Empty;
        public string LineId { get; set; } = string.Empty;
    }

    public enum EventType
    {
        Departure,
        Arrival
    }

    public enum SolverStatus
    {
        Optimal,
        Feasible,
        Infeasible,
        Invalid
    }

    public class ValidationResult
    {
        public bool IsValid { get; set; }
        public List<string> Errors { get; set; } = new();
    }
}
