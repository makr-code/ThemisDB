/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RobustnessAnalyzer.cs                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     601                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;

namespace RailwayMonitor.WPF.Services.Timetabling
{
    /// <summary>
    /// Robustness analyzer for timetables using Monte Carlo simulation
    /// Analyzes delay propagation and buffer time optimization
    /// </summary>
    public class RobustnessAnalyzer
    {
        private readonly Random _random;

        public RobustnessAnalyzer(int? seed = null)
        {
            _random = seed.HasValue ? new Random(seed.Value) : new Random();
        }

        /// <summary>
        /// Analyzes timetable robustness using Monte Carlo simulation
        /// </summary>
        public RobustnessResult AnalyzeRobustness(RobustnessInput input)
        {
            ValidateInput(input);

            var results = new List<SimulationRun>();

            for (int i = 0; i < input.SimulationCount; i++)
            {
                var run = SimulateDelayPropagation(input);
                results.Add(run);
            }

            return AggregateResults(results, input);
        }

        private void ValidateInput(RobustnessInput input)
        {
            if (input == null)
                throw new ArgumentNullException(nameof(input));
            if (input.Trains == null || !input.Trains.Any())
                throw new ArgumentException("At least one train required", nameof(input.Trains));
            if (input.SimulationCount < 100)
                throw new ArgumentException("Minimum 100 simulations required for statistical validity", nameof(input.SimulationCount));
            if (input.MaxInitialDelayMinutes < 0)
                throw new ArgumentException("Max initial delay must be non-negative", nameof(input.MaxInitialDelayMinutes));
        }

        private SimulationRun SimulateDelayPropagation(RobustnessInput input)
        {
            var run = new SimulationRun
            {
                TrainDelays = new Dictionary<string, List<DelayEvent>>()
            };

            // Initialize train states
            var trainStates = new Dictionary<string, TrainState>();
            foreach (var train in input.Trains)
            {
                trainStates[train.Id] = new TrainState
                {
                    CurrentDelay = 0,
                    Position = train.StartPosition,
                    DepartureTime = train.ScheduledDeparture
                };
                run.TrainDelays[train.Id] = new List<DelayEvent>();
            }

            // Apply initial delays (Monte Carlo)
            ApplyInitialDelays(input, trainStates, run);

            // Simulate delay propagation through network
            PropagateDelays(input, trainStates, run);

            // Calculate metrics
            run.TotalDelayMinutes = run.TrainDelays.Sum(kvp => kvp.Value.Sum(d => d.DelayMinutes));
            run.AffectedTrains = run.TrainDelays.Count(kvp => kvp.Value.Any(d => d.DelayMinutes > 0));
            run.MaxDelay = run.TrainDelays.SelectMany(kvp => kvp.Value).Max(d => d.DelayMinutes);
            run.MissedConnections = CalculateMissedConnections(input, trainStates);

            return run;
        }

        private void ApplyInitialDelays(RobustnessInput input, Dictionary<string, TrainState> trainStates, SimulationRun run)
        {
            foreach (var train in input.Trains)
            {
                // Exponential distribution for delays (realistic model)
                var lambda = 1.0 / (input.MaxInitialDelayMinutes / 3.0); // Mean = 1/3 of max
                var delay = -Math.Log(1 - _random.NextDouble()) / lambda;
                delay = Math.Min(delay, input.MaxInitialDelayMinutes);

                if (delay >= 0.5) // Only track delays >= 30 seconds
                {
                    trainStates[train.Id].CurrentDelay = delay;
                    run.TrainDelays[train.Id].Add(new DelayEvent
                    {
                        TrainId = train.Id,
                        Timestamp = train.ScheduledDeparture,
                        DelayMinutes = delay,
                        Cause = "Initial delay",
                        Type = DelayType.Primary
                    });
                }
            }
        }

        private void PropagateDelays(RobustnessInput input, Dictionary<string, TrainState> trainStates, SimulationRun run)
        {
            // Sort trains by departure time
            var orderedTrains = input.Trains.OrderBy(t => t.ScheduledDeparture).ToList();

            foreach (var train in orderedTrains)
            {
                var state = trainStates[train.Id];

                // Check for connection delays
                if (train.IncomingConnections != null)
                {
                    foreach (var connection in train.IncomingConnections)
                    {
                        var feedingTrainState = trainStates[connection.FromTrainId];
                        var arrivalDelay = feedingTrainState.CurrentDelay;

                        if (arrivalDelay > connection.MinTransferTime)
                        {
                            // Connection delayed
                            var propagatedDelay = arrivalDelay - connection.MinTransferTime;
                            
                            // Apply propagation factor (delays attenuate)
                            propagatedDelay *= input.DelayPropagationFactor;

                            if (propagatedDelay >= 0.5)
                            {
                                state.CurrentDelay = Math.Max(state.CurrentDelay, propagatedDelay);
                                run.TrainDelays[train.Id].Add(new DelayEvent
                                {
                                    TrainId = train.Id,
                                    Timestamp = train.ScheduledDeparture,
                                    DelayMinutes = propagatedDelay,
                                    Cause = $"Waiting for {connection.FromTrainId}",
                                    Type = DelayType.Secondary
                                });
                            }
                        }
                    }
                }

                // Check for track conflicts (simplified)
                foreach (var otherTrain in orderedTrains)
                {
                    if (otherTrain.Id == train.Id) continue;

                    var otherState = trainStates[otherTrain.Id];
                    
                    // If trains share track segment and temporal overlap
                    if (SharesTrackSegment(train, otherTrain) && 
                        TemporalOverlap(train, state, otherTrain, otherState, input.MinHeadwayMinutes))
                    {
                        // Apply headway delay
                        var headwayDelay = CalculateHeadwayDelay(train, state, otherTrain, otherState, input.MinHeadwayMinutes);
                        
                        if (headwayDelay >= 0.5)
                        {
                            state.CurrentDelay += headwayDelay;
                            run.TrainDelays[train.Id].Add(new DelayEvent
                            {
                                TrainId = train.Id,
                                Timestamp = train.ScheduledDeparture.AddMinutes(state.CurrentDelay),
                                DelayMinutes = headwayDelay,
                                Cause = $"Headway conflict with {otherTrain.Id}",
                                Type = DelayType.Tertiary
                            });
                        }
                    }
                }
            }
        }

        private bool SharesTrackSegment(TrainSchedule train1, TrainSchedule train2)
        {
            // Simplified: check if routes overlap
            if (train1.Route == null || train2.Route == null)
                return false;

            return train1.Route.Intersect(train2.Route).Any();
        }

        private bool TemporalOverlap(TrainSchedule train1, TrainState state1, TrainSchedule train2, TrainState state2, double minHeadway)
        {
            var train1Start = train1.ScheduledDeparture.AddMinutes(state1.CurrentDelay);
            var train1End = train1.ScheduledArrival.AddMinutes(state1.CurrentDelay);
            
            var train2Start = train2.ScheduledDeparture.AddMinutes(state2.CurrentDelay);
            var train2End = train2.ScheduledArrival.AddMinutes(state2.CurrentDelay);

            // Check if time windows overlap within headway tolerance
            return train1Start < train2End.AddMinutes(minHeadway) && 
                   train2Start < train1End.AddMinutes(minHeadway);
        }

        private double CalculateHeadwayDelay(TrainSchedule train1, TrainState state1, TrainSchedule train2, TrainState state2, double minHeadway)
        {
            var train1Start = train1.ScheduledDeparture.AddMinutes(state1.CurrentDelay);
            var train2Start = train2.ScheduledDeparture.AddMinutes(state2.CurrentDelay);

            if (train1Start > train2Start)
            {
                // Train1 is later, needs to wait for train2 + headway
                var train2Occupancy = train2.ScheduledArrival.AddMinutes(state2.CurrentDelay);
                var requiredGap = (train2Occupancy - train1Start).TotalMinutes + minHeadway;
                return Math.Max(0, requiredGap);
            }

            return 0;
        }

        private int CalculateMissedConnections(RobustnessInput input, Dictionary<string, TrainState> trainStates)
        {
            int missed = 0;

            foreach (var train in input.Trains)
            {
                if (train.IncomingConnections == null) continue;

                foreach (var connection in train.IncomingConnections)
                {
                    var feedingTrainState = trainStates[connection.FromTrainId];
                    var thisTrainState = trainStates[train.Id];

                    var feedingArrival = connection.ArrivalTime.AddMinutes(feedingTrainState.CurrentDelay);
                    var thisDeparture = train.ScheduledDeparture.AddMinutes(thisTrainState.CurrentDelay);

                    if ((thisDeparture - feedingArrival).TotalMinutes < connection.MinTransferTime)
                    {
                        missed++;
                    }
                }
            }

            return missed;
        }

        private RobustnessResult AggregateResults(List<SimulationRun> runs, RobustnessInput input)
        {
            var result = new RobustnessResult
            {
                SimulationCount = runs.Count,
                AverageDelayMinutes = runs.Average(r => r.TotalDelayMinutes / input.Trains.Count),
                MaxDelayMinutes = runs.Max(r => r.MaxDelay),
                MinDelayMinutes = runs.Min(r => r.TotalDelayMinutes / input.Trains.Count),
                StdDevDelayMinutes = CalculateStdDev(runs.Select(r => r.TotalDelayMinutes / (double)input.Trains.Count).ToList()),
                AverageAffectedTrains = runs.Average(r => r.AffectedTrains),
                MissedConnectionsAverage = runs.Average(r => r.MissedConnections),
                MissedConnectionsMax = runs.Max(r => r.MissedConnections),
                Percentile95 = CalculatePercentile(runs.Select(r => r.TotalDelayMinutes / (double)input.Trains.Count).ToList(), 0.95),
                Percentile99 = CalculatePercentile(runs.Select(r => r.TotalDelayMinutes / (double)input.Trains.Count).ToList(), 0.99)
            };

            // Calculate robustness score (0-100)
            result.RobustnessScore = CalculateRobustnessScore(result, input);

            // Generate recommendations
            result.Recommendations = GenerateRecommendations(result, input);

            return result;
        }

        private double CalculateStdDev(List<double> values)
        {
            var mean = values.Average();
            var variance = values.Select(v => Math.Pow(v - mean, 2)).Average();
            return Math.Sqrt(variance);
        }

        private double CalculatePercentile(List<double> values, double percentile)
        {
            var sorted = values.OrderBy(v => v).ToList();
            var index = (int)Math.Ceiling(percentile * sorted.Count) - 1;
            return sorted[Math.Max(0, Math.Min(index, sorted.Count - 1))];
        }

        private double CalculateRobustnessScore(RobustnessResult result, RobustnessInput input)
        {
            // Score based on:
            // - Average delay (lower is better)
            // - Missed connections (lower is better)
            // - Delay variability (lower is better)

            var delayScore = Math.Max(0, 100 - (result.AverageDelayMinutes / input.MaxInitialDelayMinutes) * 100);
            var connectionScore = Math.Max(0, 100 - (result.MissedConnectionsAverage / Math.Max(1, input.Trains.Sum(t => t.IncomingConnections?.Count ?? 0))) * 100);
            var variabilityScore = Math.Max(0, 100 - (result.StdDevDelayMinutes / result.AverageDelayMinutes) * 50);

            return (delayScore * 0.4 + connectionScore * 0.4 + variabilityScore * 0.2);
        }

        private List<string> GenerateRecommendations(RobustnessResult result, RobustnessInput input)
        {
            var recommendations = new List<string>();

            if (result.RobustnessScore < 50)
            {
                recommendations.Add("🔴 KRITISCH: Fahrplan nicht robust gegen Verspätungen");
                recommendations.Add("→ Buffer-Zeiten um mindestens 50% erhöhen");
            }
            else if (result.RobustnessScore < 70)
            {
                recommendations.Add("⚠️ WARNUNG: Mäßige Robustheit");
                recommendations.Add("→ Buffer-Zeiten um 20-30% erhöhen");
            }
            else if (result.RobustnessScore < 85)
            {
                recommendations.Add("✓ Akzeptable Robustheit");
                recommendations.Add("→ Minimale Anpassungen empfohlen");
            }
            else
            {
                recommendations.Add("✓✓ Exzellente Robustheit");
                recommendations.Add("→ Fahrplan optimal gegen Verspätungen geschützt");
            }

            if (result.MissedConnectionsAverage > 0.1)
            {
                recommendations.Add($"→ {result.MissedConnectionsAverage:F1} Anschlüsse im Durchschnitt verpasst");
                recommendations.Add("→ Umsteigezeiten verlängern oder Taktung anpassen");
            }

            if (result.Percentile95 > input.MaxInitialDelayMinutes * 2)
            {
                recommendations.Add("→ 95. Perzentil zeigt starke Verspätungskaskaden");
                recommendations.Add("→ Kritische Abhängigkeiten im Fahrplan reduzieren");
            }

            if (result.StdDevDelayMinutes > result.AverageDelayMinutes)
            {
                recommendations.Add("→ Hohe Variabilität der Verspätungen");
                recommendations.Add("→ Stabilisierung durch gleichmäßigere Buffer-Verteilung");
            }

            return recommendations;
        }

        /// <summary>
        /// Optimizes buffer times to improve robustness
        /// </summary>
        public BufferOptimizationResult OptimizeBuffers(RobustnessInput input, double targetRobustness = 80.0)
        {
            var iterations = new List<BufferOptimizationIteration>();
            var currentBufferFactor = 1.0;
            var bestResult = (RobustnessResult: (RobustnessResult)null, BufferFactor: 1.0);

            // Binary search for optimal buffer factor
            var lowerBound = 0.5;
            var upperBound = 3.0;
            var tolerance = 0.05;

            while (upperBound - lowerBound > tolerance)
            {
                currentBufferFactor = (lowerBound + upperBound) / 2.0;

                // Apply buffer factor
                var adjustedInput = ApplyBufferFactor(input, currentBufferFactor);
                var robustness = AnalyzeRobustness(adjustedInput);

                iterations.Add(new BufferOptimizationIteration
                {
                    BufferFactor = currentBufferFactor,
                    RobustnessScore = robustness.RobustnessScore,
                    AverageDelay = robustness.AverageDelayMinutes,
                    MissedConnections = robustness.MissedConnectionsAverage
                });

                if (bestResult.RobustnessResult == null || 
                    Math.Abs(robustness.RobustnessScore - targetRobustness) < Math.Abs(bestResult.RobustnessResult.RobustnessScore - targetRobustness))
                {
                    bestResult = (robustness, currentBufferFactor);
                }

                if (robustness.RobustnessScore < targetRobustness)
                {
                    lowerBound = currentBufferFactor; // Need more buffer
                }
                else
                {
                    upperBound = currentBufferFactor; // Can reduce buffer
                }
            }

            return new BufferOptimizationResult
            {
                OptimalBufferFactor = bestResult.BufferFactor,
                AchievedRobustness = bestResult.RobustnessResult.RobustnessScore,
                TargetRobustness = targetRobustness,
                Iterations = iterations,
                FinalResult = bestResult.RobustnessResult
            };
        }

        private RobustnessInput ApplyBufferFactor(RobustnessInput original, double bufferFactor)
        {
            // Create copy with adjusted buffer times
            var adjusted = new RobustnessInput
            {
                Trains = original.Trains.Select(t => new TrainSchedule
                {
                    Id = t.Id,
                    ScheduledDeparture = t.ScheduledDeparture,
                    ScheduledArrival = t.ScheduledArrival.AddMinutes((bufferFactor - 1.0) * (t.ScheduledArrival - t.ScheduledDeparture).TotalMinutes * 0.1),
                    StartPosition = t.StartPosition,
                    EndPosition = t.EndPosition,
                    Route = t.Route,
                    IncomingConnections = t.IncomingConnections?.Select(c => new Connection
                    {
                        FromTrainId = c.FromTrainId,
                        ToTrainId = c.ToTrainId,
                        StationId = c.StationId,
                        ArrivalTime = c.ArrivalTime,
                        MinTransferTime = c.MinTransferTime * bufferFactor
                    }).ToList()
                }).ToList(),
                SimulationCount = original.SimulationCount,
                MaxInitialDelayMinutes = original.MaxInitialDelayMinutes,
                DelayPropagationFactor = original.DelayPropagationFactor,
                MinHeadwayMinutes = original.MinHeadwayMinutes
            };

            return adjusted;
        }
    }

    #region Data Models

    public class RobustnessInput
    {
        public List<TrainSchedule> Trains { get; set; }
        public int SimulationCount { get; set; } = 10000;
        public double MaxInitialDelayMinutes { get; set; } = 15.0;
        public double DelayPropagationFactor { get; set; } = 0.7; // Delays attenuate by 30%
        public double MinHeadwayMinutes { get; set; } = 2.0;
    }

    public class TrainSchedule
    {
        public string Id { get; set; }
        public DateTime ScheduledDeparture { get; set; }
        public DateTime ScheduledArrival { get; set; }
        public double StartPosition { get; set; }
        public double EndPosition { get; set; }
        public List<string> Route { get; set; }
        public List<Connection> IncomingConnections { get; set; }
    }

    public class Connection
    {
        public string FromTrainId { get; set; }
        public string ToTrainId { get; set; }
        public string StationId { get; set; }
        public DateTime ArrivalTime { get; set; }
        public double MinTransferTime { get; set; } // minutes
    }

    public class TrainState
    {
        public double CurrentDelay { get; set; }
        public double Position { get; set; }
        public DateTime DepartureTime { get; set; }
    }

    public class SimulationRun
    {
        public Dictionary<string, List<DelayEvent>> TrainDelays { get; set; }
        public double TotalDelayMinutes { get; set; }
        public int AffectedTrains { get; set; }
        public double MaxDelay { get; set; }
        public int MissedConnections { get; set; }
    }

    public class DelayEvent
    {
        public string TrainId { get; set; }
        public DateTime Timestamp { get; set; }
        public double DelayMinutes { get; set; }
        public string Cause { get; set; }
        public DelayType Type { get; set; }
    }

    public enum DelayType
    {
        Primary,    // Initial/operational delay
        Secondary,  // Connection propagation
        Tertiary    // Track conflict
    }

    public class RobustnessResult
    {
        public int SimulationCount { get; set; }
        public double AverageDelayMinutes { get; set; }
        public double MaxDelayMinutes { get; set; }
        public double MinDelayMinutes { get; set; }
        public double StdDevDelayMinutes { get; set; }
        public double AverageAffectedTrains { get; set; }
        public double MissedConnectionsAverage { get; set; }
        public int MissedConnectionsMax { get; set; }
        public double Percentile95 { get; set; }
        public double Percentile99 { get; set; }
        public double RobustnessScore { get; set; } // 0-100
        public List<string> Recommendations { get; set; }

        public string GetSummary()
        {
            var summary = $@"=== ROBUSTNESS ANALYSIS ===

Simulationen: {SimulationCount:N0}

Verspätungen:
  Durchschnitt: {AverageDelayMinutes:F1} min
  Maximum: {MaxDelayMinutes:F1} min
  Minimum: {MinDelayMinutes:F1} min
  Standardabweichung: {StdDevDelayMinutes:F1} min
  95. Perzentil: {Percentile95:F1} min
  99. Perzentil: {Percentile99:F1} min

Auswirkungen:
  Betroffene Züge (Ø): {AverageAffectedTrains:F1}
  Verpasste Anschlüsse (Ø): {MissedConnectionsAverage:F1}
  Verpasste Anschlüsse (Max): {MissedConnectionsMax}

Robustheit:
  Score: {RobustnessScore:F1}/100
  Status: {GetRobustnessStatus()}

EMPFEHLUNGEN:
{string.Join("\n", Recommendations)}";

            return summary;
        }

        private string GetRobustnessStatus()
        {
            if (RobustnessScore >= 85) return "✓✓ Exzellent";
            if (RobustnessScore >= 70) return "✓ Gut";
            if (RobustnessScore >= 50) return "⚠️ Mäßig";
            return "🔴 Kritisch";
        }
    }

    public class BufferOptimizationResult
    {
        public double OptimalBufferFactor { get; set; }
        public double AchievedRobustness { get; set; }
        public double TargetRobustness { get; set; }
        public List<BufferOptimizationIteration> Iterations { get; set; }
        public RobustnessResult FinalResult { get; set; }

        public string GetSummary()
        {
            return $@"=== BUFFER OPTIMIZATION ===

Ergebnis:
  Optimaler Buffer-Faktor: {OptimalBufferFactor:F2}x
  Erreichte Robustheit: {AchievedRobustness:F1}/100
  Ziel-Robustheit: {TargetRobustness:F1}/100
  Iterationen: {Iterations.Count}

Empfehlung:
  → Buffer-Zeiten um {(OptimalBufferFactor - 1.0) * 100:F0}% anpassen
  → Umsteigezeiten: ×{OptimalBufferFactor:F2}
  → Fahrzeit-Puffer: +{(OptimalBufferFactor - 1.0) * 10:F1}%";
        }
    }

    public class BufferOptimizationIteration
    {
        public double BufferFactor { get; set; }
        public double RobustnessScore { get; set; }
        public double AverageDelay { get; set; }
        public double MissedConnections { get; set; }
    }

    #endregion
}
