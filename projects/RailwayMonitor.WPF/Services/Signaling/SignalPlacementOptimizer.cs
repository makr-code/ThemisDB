/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SignalPlacementOptimizer.cs                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     505                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using Google.OrTools.LinearSolver;

namespace RailwayMonitor.WPF.Services.Signaling
{
    /// <summary>
    /// Optimizes signal placement on railway tracks using Integer Linear Programming (ILP).
    /// Minimizes total cost while ensuring safety constraints and capacity requirements.
    /// Uses Google OR-Tools SCIP solver for mixed-integer optimization.
    /// </summary>
    public class SignalPlacementOptimizer
    {
        private const double SIGNAL_COST_MAIN = 150000.0; // Main signal cost in EUR
        private const double SIGNAL_COST_DISTANT = 75000.0; // Distant signal cost in EUR
        private const double SIGNAL_COST_ETCS = 250000.0; // ETCS beacon/balise group in EUR
        private const double INSTALLATION_COST = 50000.0; // Installation per signal in EUR
        private const double MAINTENANCE_COST_ANNUAL = 5000.0; // Annual maintenance per signal in EUR
        
        private readonly ETCSLevel2Simulator _etcsSimulator;

        public SignalPlacementOptimizer()
        {
            _etcsSimulator = new ETCSLevel2Simulator();
        }

        /// <summary>
        /// Optimizes signal placement for a track section.
        /// </summary>
        public SignalPlacementResult OptimizeSignalPlacement(SignalPlacementInput input)
        {
            var validation = ValidateInput(input);
            if (!validation.IsValid)
            {
                return new SignalPlacementResult
                {
                    Success = false,
                    ErrorMessage = string.Join("; ", validation.Errors)
                };
            }

            // Create ILP solver (SCIP is free, open-source)
            var solver = Solver.CreateSolver("SCIP");
            if (solver == null)
            {
                return new SignalPlacementResult
                {
                    Success = false,
                    ErrorMessage = "OR-Tools SCIP solver not available"
                };
            }

            var result = SolveILP(solver, input);
            return result;
        }

        private SignalPlacementResult SolveILP(Solver solver, SignalPlacementInput input)
        {
            var positions = GenerateCandidatePositions(input);
            int n = positions.Count;

            // Decision variables: x[i] = 1 if signal placed at position i
            var x = new Variable[n];
            for (int i = 0; i < n; i++)
            {
                x[i] = solver.MakeBoolVar($"signal_{i}");
            }

            // Objective: Minimize total cost
            var objective = solver.Objective();
            for (int i = 0; i < n; i++)
            {
                double cost = CalculateSignalCost(positions[i], input);
                objective.SetCoefficient(x[i], cost);
            }
            objective.SetMinimization();

            // Constraint 1: Maximum spacing between signals
            double maxSpacing = CalculateMaxSpacing(input);
            for (int i = 0; i < n - 1; i++)
            {
                // Ensure at least one signal within maxSpacing
                int j = i + 1;
                while (j < n && positions[j] - positions[i] <= maxSpacing)
                {
                    j++;
                }
                
                if (j > i + 1)
                {
                    var constraint = solver.MakeConstraint(1, double.PositiveInfinity, $"spacing_{i}");
                    for (int k = i; k < j && k < n; k++)
                    {
                        constraint.SetCoefficient(x[k], 1);
                    }
                }
            }

            // Constraint 2: Minimum spacing between signals
            double minSpacing = CalculateMinSpacing(input);
            for (int i = 0; i < n - 1; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    if (positions[j] - positions[i] < minSpacing)
                    {
                        // Can't place both signals i and j
                        var constraint = solver.MakeConstraint(0, 1, $"min_spacing_{i}_{j}");
                        constraint.SetCoefficient(x[i], 1);
                        constraint.SetCoefficient(x[j], 1);
                    }
                    else
                    {
                        break; // Positions are sorted
                    }
                }
            }

            // Constraint 3: Required signals at specific locations (stations, junctions)
            foreach (var requiredPos in input.RequiredSignalPositions)
            {
                int idx = FindNearestPosition(positions, requiredPos);
                if (idx >= 0)
                {
                    var constraint = solver.MakeConstraint(1, 1, $"required_{idx}");
                    constraint.SetCoefficient(x[idx], 1);
                }
            }

            // Constraint 4: Capacity requirements (minimum number of signals)
            if (input.MinSignalCount > 0)
            {
                var constraint = solver.MakeConstraint(input.MinSignalCount, double.PositiveInfinity, "min_signals");
                for (int i = 0; i < n; i++)
                {
                    constraint.SetCoefficient(x[i], 1);
                }
            }

            // Solve
            var resultStatus = solver.Solve();

            if (resultStatus != Solver.ResultStatus.OPTIMAL && resultStatus != Solver.ResultStatus.FEASIBLE)
            {
                return new SignalPlacementResult
                {
                    Success = false,
                    ErrorMessage = $"No feasible solution found. Status: {resultStatus}"
                };
            }

            // Extract solution
            var signals = new List<PlacedSignal>();
            for (int i = 0; i < n; i++)
            {
                if (x[i].SolutionValue() > 0.5) // Boolean variable
                {
                    signals.Add(new PlacedSignal
                    {
                        Position = positions[i],
                        Type = DetermineSignalType(positions[i], input),
                        Cost = CalculateSignalCost(positions[i], input),
                        SpeedLimit = input.MaxSpeedKmh,
                        IsETCS = input.HasETCS
                    });
                }
            }

            // Calculate metrics
            double totalCost = objective.Value();
            double totalInstallation = signals.Count * INSTALLATION_COST;
            double totalMaintenance = signals.Count * MAINTENANCE_COST_ANNUAL;
            
            var capacityBenefit = CalculateCapacityBenefit(signals, input);

            return new SignalPlacementResult
            {
                Success = true,
                Signals = signals,
                TotalCost = totalCost,
                InstallationCost = totalInstallation,
                AnnualMaintenanceCost = totalMaintenance,
                SignalCount = signals.Count,
                AverageSpacing = signals.Count > 1 
                    ? input.TrackLengthKm * 1000 / (signals.Count - 1) 
                    : 0,
                CapacityImprovement = capacityBenefit,
                SolverStatus = resultStatus.ToString(),
                ComputationTimeMs = solver.WallTime()
            };
        }

        private List<double> GenerateCandidatePositions(SignalPlacementInput input)
        {
            var positions = new List<double>();
            
            // Start and end
            positions.Add(0);
            positions.Add(input.TrackLengthKm * 1000);

            // Regular intervals (potential signal locations)
            double spacing = 500; // Every 500m candidate
            for (double pos = spacing; pos < input.TrackLengthKm * 1000; pos += spacing)
            {
                positions.Add(pos);
            }

            // Add required positions
            positions.AddRange(input.RequiredSignalPositions);

            // Add station positions
            positions.AddRange(input.StationPositions);

            // Sort and remove duplicates
            positions = positions.Distinct().OrderBy(p => p).ToList();

            return positions;
        }

        private double CalculateMaxSpacing(SignalPlacementInput input)
        {
            if (input.HasETCS)
            {
                // ETCS allows dynamic blocks, but still need signals for backup
                // Typically 3-5 km for high-speed lines
                return Math.Min(5000, input.TrackLengthKm * 1000 / 3);
            }
            else
            {
                // Traditional signaling: block length based on braking distance
                var brakingDist = _etcsSimulator.CalculateBrakingDistance(
                    input.MaxSpeedKmh, 
                    ETCSLevel2Simulator.BrakeType.Emergency);
                
                // Safety factor of 1.5
                return Math.Min(2500, brakingDist * 1.5);
            }
        }

        private double CalculateMinSpacing(SignalPlacementInput input)
        {
            // Minimum spacing to avoid signal clutter
            // Typically 300-500m
            return input.HasETCS ? 500 : 300;
        }

        private double CalculateSignalCost(double position, SignalPlacementInput input)
        {
            double baseCost = SIGNAL_COST_MAIN;

            if (input.HasETCS)
            {
                baseCost += SIGNAL_COST_ETCS;
            }

            // Higher cost near difficult terrain
            foreach (var difficult in input.DifficultTerrainSections)
            {
                if (position >= difficult.StartPosition && position <= difficult.EndPosition)
                {
                    baseCost *= 1.5; // 50% cost increase
                    break;
                }
            }

            return baseCost + INSTALLATION_COST;
        }

        private SignalType DetermineSignalType(double position, SignalPlacementInput input)
        {
            // Check if near station
            foreach (var stationPos in input.StationPositions)
            {
                if (Math.Abs(position - stationPos) < 100)
                {
                    return SignalType.Station;
                }
            }

            // Check if at junction
            foreach (var junctionPos in input.RequiredSignalPositions)
            {
                if (Math.Abs(position - junctionPos) < 50)
                {
                    return SignalType.Junction;
                }
            }

            return SignalType.Block;
        }

        private int FindNearestPosition(List<double> positions, double target)
        {
            int nearest = -1;
            double minDist = double.MaxValue;

            for (int i = 0; i < positions.Count; i++)
            {
                double dist = Math.Abs(positions[i] - target);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = i;
                }
            }

            return nearest;
        }

        private double CalculateCapacityBenefit(List<PlacedSignal> signals, SignalPlacementInput input)
        {
            if (signals.Count < 2) return 0;

            double avgSpacing = input.TrackLengthKm * 1000 / (signals.Count - 1);
            
            // Capacity inversely proportional to spacing
            double baselineSpacing = 2000; // 2km blocks
            double capacityRatio = baselineSpacing / avgSpacing;

            return (capacityRatio - 1.0) * 100; // Percentage improvement
        }

        private ValidationResult ValidateInput(SignalPlacementInput input)
        {
            var errors = new List<string>();

            if (input.TrackLengthKm <= 0)
                errors.Add("Track length must be positive");

            if (input.MaxSpeedKmh <= 0 || input.MaxSpeedKmh > 350)
                errors.Add("Max speed must be between 0 and 350 km/h");

            if (input.MinSignalCount < 0)
                errors.Add("Minimum signal count cannot be negative");

            return new ValidationResult
            {
                IsValid = errors.Count == 0,
                Errors = errors
            };
        }
    }

    #region Data Models

    public class SignalPlacementInput
    {
        public double TrackLengthKm { get; set; }
        public double MaxSpeedKmh { get; set; }
        public bool HasETCS { get; set; }
        public int MinSignalCount { get; set; }
        public List<double> RequiredSignalPositions { get; set; } = new();
        public List<double> StationPositions { get; set; } = new();
        public List<TerrainSection> DifficultTerrainSections { get; set; } = new();
        public double AverageBlockLengthKm { get; set; } = 2.0;
    }

    public class TerrainSection
    {
        public double StartPosition { get; set; }
        public double EndPosition { get; set; }
        public string Reason { get; set; } = "";
    }

    public class SignalPlacementResult
    {
        public bool Success { get; set; }
        public string ErrorMessage { get; set; } = "";
        public List<PlacedSignal> Signals { get; set; } = new();
        public double TotalCost { get; set; }
        public double InstallationCost { get; set; }
        public double AnnualMaintenanceCost { get; set; }
        public int SignalCount { get; set; }
        public double AverageSpacing { get; set; }
        public double CapacityImprovement { get; set; }
        public string SolverStatus { get; set; } = "";
        public double ComputationTimeMs { get; set; }

        public string GetSummary()
        {
            if (!Success)
            {
                return $"❌ Optimization Failed: {ErrorMessage}";
            }

            return $@"
=== SIGNAL PLACEMENT OPTIMIZATION ===

Ergebnis:
  Anzahl Signale: {SignalCount}
  Durchschnittlicher Abstand: {AverageSpacing:F0}m
  Solver Status: {SolverStatus}
  Berechnungszeit: {ComputationTimeMs:F1}ms

Kosten:
  Total: {TotalCost:N0} €
  Installation: {InstallationCost:N0} €
  Wartung (jährlich): {AnnualMaintenanceCost:N0} €

Kapazität:
  Verbesserung: +{CapacityImprovement:F1}%

Signale:
{string.Join("\n", Signals.Take(10).Select(s => 
    $"  {s.Position / 1000:F1} km - {s.Type} ({s.Cost:N0} €)"))}
{(SignalCount > 10 ? $"  ... und {SignalCount - 10} weitere" : "")}

EMPFEHLUNGEN:
{GetRecommendations()}
";
        }

        private string GetRecommendations()
        {
            var recommendations = new List<string>();

            if (AverageSpacing > 2500)
            {
                recommendations.Add("⚠️ Hoher durchschnittlicher Signalabstand");
                recommendations.Add("→ Erwägen Sie zusätzliche Signale für höhere Kapazität");
            }

            if (AverageSpacing < 800)
            {
                recommendations.Add("⚠️ Sehr geringer Signalabstand");
                recommendations.Add("→ Prüfen Sie, ob alle Signale notwendig sind");
            }

            if (CapacityImprovement > 30)
            {
                recommendations.Add("✓ Signifikante Kapazitätssteigerung erreicht");
            }

            if (!Signals.Any(s => s.IsETCS))
            {
                recommendations.Add("💡 ETCS Level 2 Installation für +20-30% Kapazität erwägen");
            }

            if (recommendations.Count == 0)
            {
                recommendations.Add("✓ Optimale Signalplatzierung erreicht");
            }

            return string.Join("\n", recommendations);
        }
    }

    public class PlacedSignal
    {
        public double Position { get; set; } // meters
        public SignalType Type { get; set; }
        public double Cost { get; set; }
        public double SpeedLimit { get; set; }
        public bool IsETCS { get; set; }

        public string GetDescription()
        {
            return $"Signal at {Position / 1000:F1}km - {Type} " +
                   $"(Speed: {SpeedLimit:F0} km/h, {(IsETCS ? "ETCS" : "Conv.")})";
        }
    }

    public enum SignalType
    {
        Block,      // Regular block signal
        Station,    // Station signal
        Junction,   // Junction/switch signal
        Distant     // Distant (pre-warning) signal
    }

    public class ValidationResult
    {
        public bool IsValid { get; set; }
        public List<string> Errors { get; set; } = new();
    }

    #endregion
}
