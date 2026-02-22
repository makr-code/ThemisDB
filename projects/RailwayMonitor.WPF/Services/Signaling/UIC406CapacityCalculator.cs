/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UIC406CapacityCalculator.cs                        ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     467                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;

namespace RailwayMonitor.WPF.Services.Signaling
{
    /// <summary>
    /// UIC 406 Capacity Calculator - Standard method for railway capacity calculation
    /// Implements Compression and Deduction methods per UIC Code 406
    /// </summary>
    public class UIC406CapacityCalculator
    {
        /// <summary>
        /// Calculate track capacity using UIC 406 methodology
        /// </summary>
        public CapacityResult CalculateCapacity(CapacityInput input)
        {
            // Step 1: Calculate consumption time (compression method)
            var consumption = CalculateConsumptionTime(input);

            // Step 2: Apply deduction factors
            var deductions = CalculateDeductions(input);

            // Step 3: Calculate available capacity
            var availableMinutes = 1440.0; // 24 hours in minutes
            var usedMinutes = consumption.TotalConsumptionMinutes;
            var deductedMinutes = deductions.TotalDeductionMinutes;

            var capacityUsed = (usedMinutes + deductedMinutes) / availableMinutes;
            var capacityReserve = 1.0 - capacityUsed;

            // Step 4: Calculate maximum trains per day
            var avgConsumptionPerTrain = consumption.TotalConsumptionMinutes / input.TrainSchedule.Count;
            var maxTrainsPerDay = (int)((availableMinutes * 0.85) / avgConsumptionPerTrain); // 85% utilization limit

            return new CapacityResult
            {
                ConsumptionTime = consumption,
                Deductions = deductions,
                TotalUsedMinutes = usedMinutes + deductedMinutes,
                AvailableMinutes = availableMinutes,
                CapacityUsed = capacityUsed,
                CapacityReserve = capacityReserve,
                CurrentTrainsPerDay = input.TrainSchedule.Count,
                MaxTrainsPerDay = maxTrainsPerDay,
                Status = GetCapacityStatus(capacityUsed),
                RecommendedActions = GenerateRecommendations(capacityUsed, input)
            };
        }

        /// <summary>
        /// Compression method - Calculate consumption time for all trains
        /// </summary>
        private ConsumptionTime CalculateConsumptionTime(CapacityInput input)
        {
            var totalMinutes = 0.0;
            var details = new List<TrainConsumption>();

            foreach (var train in input.TrainSchedule)
            {
                // UIC 406 formula: t_c = t_r + t_a + (L/v) * 60
                // t_r = runtime, t_a = additional time (stops, acceleration), L/v = section occupation

                var runtime = (input.SectionLengthKm / train.AverageSpeedKmh) * 60.0; // minutes
                var additionalTime = train.StopCount * 2.0; // 2 min per stop
                var occupationTime = CalculateOccupationTime(input.SectionLengthKm, train, input);

                var consumption = runtime + additionalTime + occupationTime;
                totalMinutes += consumption;

                details.Add(new TrainConsumption
                {
                    TrainId = train.TrainId,
                    TrainType = train.TrainType,
                    Runtime = runtime,
                    AdditionalTime = additionalTime,
                    OccupationTime = occupationTime,
                    TotalConsumption = consumption
                });
            }

            return new ConsumptionTime
            {
                TotalConsumptionMinutes = totalMinutes,
                TrainDetails = details,
                AverageConsumptionPerTrain = totalMinutes / input.TrainSchedule.Count
            };
        }

        /// <summary>
        /// Calculate occupation time based on signaling system and train characteristics
        /// </summary>
        private double CalculateOccupationTime(double sectionLengthKm, TrainSchedule train, CapacityInput input)
        {
            // Block occupation depends on signaling system
            if (input.HasETCS)
            {
                // ETCS: Dynamic blocks based on braking distance
                var brakingDistanceKm = CalculateBrakingDistance(train.MaxSpeedKmh) / 1000.0;
                var occupationLengthKm = sectionLengthKm + brakingDistanceKm;
                return (occupationLengthKm / train.AverageSpeedKmh) * 60.0;
            }
            else
            {
                // Fixed blocks: Typically 2km for main lines
                var blockLengthKm = input.AverageBlockLengthKm;
                var blocksNeeded = Math.Ceiling(sectionLengthKm / blockLengthKm) + 1; // +1 for safety
                var occupationLengthKm = blocksNeeded * blockLengthKm;
                return (occupationLengthKm / train.AverageSpeedKmh) * 60.0;
            }
        }

        /// <summary>
        /// Deduction method - Calculate capacity reductions due to various factors
        /// </summary>
        private DeductionFactors CalculateDeductions(CapacityInput input)
        {
            var totalDeductions = 0.0;
            var factors = new Dictionary<string, double>();

            // 1. Maintenance windows (typically 4-6 hours/day)
            var maintenanceMinutes = input.MaintenanceHoursPerDay * 60.0;
            factors["Maintenance"] = maintenanceMinutes;
            totalDeductions += maintenanceMinutes;

            // 2. Train mix heterogeneity (slower trains reduce capacity)
            var speedRange = input.TrainSchedule.Max(t => t.MaxSpeedKmh) - input.TrainSchedule.Min(t => t.MaxSpeedKmh);
            var heterogeneityFactor = speedRange / 100.0; // ~1% reduction per 10 km/h difference
            var heterogeneityMinutes = input.TrainSchedule.Count * heterogeneityFactor * 0.5; // 0.5 min per train
            factors["TrainMixHeterogeneity"] = heterogeneityMinutes;
            totalDeductions += heterogeneityMinutes;

            // 3. Buffer time for delays (UIC recommends 10-15%)
            var bufferMinutes = input.TrainSchedule.Count * 1.5; // 1.5 min buffer per train
            factors["DelayBuffer"] = bufferMinutes;
            totalDeductions += bufferMinutes;

            // 4. Station dwell time variations
            var dwellVariation = input.TrainSchedule.Sum(t => t.StopCount) * 0.5; // 0.5 min variation per stop
            factors["DwellTimeVariation"] = dwellVariation;
            totalDeductions += dwellVariation;

            // 5. Single-track sections (if applicable)
            if (input.IsSingleTrack)
            {
                var singleTrackPenalty = input.TrainSchedule.Count * 3.0; // 3 min penalty per train
                factors["SingleTrackPenalty"] = singleTrackPenalty;
                totalDeductions += singleTrackPenalty;
            }

            return new DeductionFactors
            {
                TotalDeductionMinutes = totalDeductions,
                Factors = factors
            };
        }

        /// <summary>
        /// Calculate braking distance (simplified from ETCS simulator)
        /// </summary>
        private double CalculateBrakingDistance(double speedKmh)
        {
            var speedMs = speedKmh / 3.6;
            var deceleration = 1.0; // m/s² (average service brake)
            var distance = (speedMs * speedMs) / (2.0 * deceleration);
            return distance + 50.0; // +50m safety margin
        }

        /// <summary>
        /// Determine capacity status based on utilization
        /// </summary>
        private CapacityStatus GetCapacityStatus(double capacityUsed)
        {
            if (capacityUsed >= 0.85) return CapacityStatus.Critical;
            if (capacityUsed >= 0.75) return CapacityStatus.High;
            if (capacityUsed >= 0.60) return CapacityStatus.Medium;
            return CapacityStatus.Low;
        }

        /// <summary>
        /// Generate recommendations based on capacity analysis
        /// </summary>
        private List<string> GenerateRecommendations(double capacityUsed, CapacityInput input)
        {
            var recommendations = new List<string>();

            if (capacityUsed >= 0.85)
            {
                recommendations.Add("🔴 KRITISCH: Kapazitätsgrenze erreicht (>85%)");
                
                if (!input.HasETCS)
                {
                    recommendations.Add("→ ETCS Level 2 Installation empfohlen (+20-30% Kapazität)");
                }
                
                if (input.TrackCount == 1)
                {
                    recommendations.Add("→ Zweigleisiger Ausbau empfohlen (+100% Kapazität)");
                }
                else
                {
                    recommendations.Add("→ Zusätzliches Gleis oder Paralleltrassierung prüfen");
                }
                
                var speedRange = input.TrainSchedule.Max(t => t.MaxSpeedKmh) - input.TrainSchedule.Min(t => t.MaxSpeedKmh);
                if (speedRange > 50)
                {
                    recommendations.Add("→ Überholgleise für homogenere Zugfolge (+10-15% Kapazität)");
                }
            }
            else if (capacityUsed >= 0.75)
            {
                recommendations.Add("⚠️ WARNUNG: Hohe Auslastung (>75%)");
                recommendations.Add("→ Kapazitätsreserven für Wachstum begrenzt");
                
                if (!input.HasETCS)
                {
                    recommendations.Add("→ ETCS Level 2 für zusätzliche Kapazität prüfen");
                }
            }
            else if (capacityUsed >= 0.60)
            {
                recommendations.Add("✓ Moderate Auslastung (60-75%)");
                recommendations.Add("→ Kapazitätsreserven vorhanden für moderates Wachstum");
            }
            else
            {
                recommendations.Add("✓ Gute Kapazitätsreserven (<60% Auslastung)");
                recommendations.Add("→ Ausreichend Puffer für Verspätungen und Wachstum");
            }

            return recommendations;
        }

        /// <summary>
        /// Calculate train mix factor (heterogeneity of train speeds)
        /// </summary>
        public double CalculateTrainMixFactor(List<TrainSchedule> trains)
        {
            if (trains.Count == 0) return 1.0;

            var avgSpeed = trains.Average(t => t.AverageSpeedKmh);
            var speedVariance = trains.Sum(t => Math.Pow(t.AverageSpeedKmh - avgSpeed, 2)) / trains.Count;
            var speedStdDev = Math.Sqrt(speedVariance);

            // UIC 406: Mix factor = 1 + (stddev / mean) * 0.5
            var mixFactor = 1.0 + (speedStdDev / avgSpeed) * 0.5;
            return Math.Min(mixFactor, 2.0); // Cap at 2.0
        }

        /// <summary>
        /// Validate capacity input
        /// </summary>
        public ValidationResult ValidateInput(CapacityInput input)
        {
            var errors = new List<string>();

            if (input.SectionLengthKm <= 0)
                errors.Add("Section length must be positive");

            if (input.TrainSchedule == null || input.TrainSchedule.Count == 0)
                errors.Add("At least one train must be scheduled");

            if (input.TrackCount < 1 || input.TrackCount > 4)
                errors.Add("Track count must be between 1 and 4");

            if (input.MaintenanceHoursPerDay < 0 || input.MaintenanceHoursPerDay > 24)
                errors.Add("Maintenance hours must be between 0 and 24");

            foreach (var train in input.TrainSchedule ?? new List<TrainSchedule>())
            {
                if (train.AverageSpeedKmh <= 0 || train.AverageSpeedKmh > 350)
                    errors.Add($"Invalid speed for train {train.TrainId}: {train.AverageSpeedKmh} km/h");

                if (train.MaxSpeedKmh < train.AverageSpeedKmh)
                    errors.Add($"Max speed must be >= average speed for train {train.TrainId}");
            }

            return new ValidationResult
            {
                IsValid = errors.Count == 0,
                Errors = errors
            };
        }
    }

    #region Data Models

    /// <summary>
    /// Input parameters for UIC 406 capacity calculation
    /// </summary>
    public class CapacityInput
    {
        public double SectionLengthKm { get; set; }
        public List<TrainSchedule> TrainSchedule { get; set; } = new();
        public int TrackCount { get; set; } = 2;
        public bool HasETCS { get; set; }
        public double AverageBlockLengthKm { get; set; } = 2.0;
        public double MaintenanceHoursPerDay { get; set; } = 4.0;
        public bool IsSingleTrack => TrackCount == 1;
    }

    /// <summary>
    /// Train schedule entry
    /// </summary>
    public class TrainSchedule
    {
        public string TrainId { get; set; } = "";
        public TrainType TrainType { get; set; }
        public double AverageSpeedKmh { get; set; }
        public double MaxSpeedKmh { get; set; }
        public int StopCount { get; set; }
    }

    /// <summary>
    /// Train types for classification
    /// </summary>
    public enum TrainType
    {
        ICE,            // High-speed (>250 km/h)
        IC_EC,          // InterCity/EuroCity (160-200 km/h)
        RegionalExpress, // RE (120-160 km/h)
        Regional,       // RB (80-120 km/h)
        SBahn,          // S-Bahn (60-100 km/h)
        Freight         // Güterverkehr (80-120 km/h)
    }

    /// <summary>
    /// Consumption time breakdown (Compression method)
    /// </summary>
    public class ConsumptionTime
    {
        public double TotalConsumptionMinutes { get; set; }
        public List<TrainConsumption> TrainDetails { get; set; } = new();
        public double AverageConsumptionPerTrain { get; set; }
    }

    /// <summary>
    /// Individual train consumption
    /// </summary>
    public class TrainConsumption
    {
        public string TrainId { get; set; } = "";
        public TrainType TrainType { get; set; }
        public double Runtime { get; set; }
        public double AdditionalTime { get; set; }
        public double OccupationTime { get; set; }
        public double TotalConsumption { get; set; }

        public string GetSummary() => 
            $"{TrainId} ({TrainType}): {TotalConsumption:F1} min " +
            $"(Runtime: {Runtime:F1}, Additional: {AdditionalTime:F1}, Occupation: {OccupationTime:F1})";
    }

    /// <summary>
    /// Deduction factors (Deduction method)
    /// </summary>
    public class DeductionFactors
    {
        public double TotalDeductionMinutes { get; set; }
        public Dictionary<string, double> Factors { get; set; } = new();

        public string GetSummary()
        {
            var summary = $"Total Deductions: {TotalDeductionMinutes:F1} min\n";
            foreach (var factor in Factors.OrderByDescending(f => f.Value))
            {
                summary += $"  - {factor.Key}: {factor.Value:F1} min\n";
            }
            return summary;
        }
    }

    /// <summary>
    /// Complete capacity calculation result
    /// </summary>
    public class CapacityResult
    {
        public ConsumptionTime ConsumptionTime { get; set; } = new();
        public DeductionFactors Deductions { get; set; } = new();
        public double TotalUsedMinutes { get; set; }
        public double AvailableMinutes { get; set; }
        public double CapacityUsed { get; set; }
        public double CapacityReserve { get; set; }
        public int CurrentTrainsPerDay { get; set; }
        public int MaxTrainsPerDay { get; set; }
        public CapacityStatus Status { get; set; }
        public List<string> RecommendedActions { get; set; } = new();

        public string GetSummary()
        {
            var summary = "=== UIC 406 KAPAZITÄTSANALYSE ===\n\n";
            summary += $"Verbrauch (Compression Method):\n";
            summary += $"  Total: {ConsumptionTime.TotalConsumptionMinutes:F1} min\n";
            summary += $"  Avg/Train: {ConsumptionTime.AverageConsumptionPerTrain:F1} min\n\n";
            
            summary += $"Abzüge (Deduction Method):\n";
            summary += $"  Total: {Deductions.TotalDeductionMinutes:F1} min\n";
            foreach (var factor in Deductions.Factors.OrderByDescending(f => f.Value).Take(3))
            {
                summary += $"  - {factor.Key}: {factor.Value:F1} min\n";
            }
            summary += "\n";
            
            summary += $"Kapazitätsbilanz:\n";
            summary += $"  Verfügbar: {AvailableMinutes:F0} min/Tag (24h)\n";
            summary += $"  Verbraucht: {TotalUsedMinutes:F0} min/Tag\n";
            summary += $"  Auslastung: {CapacityUsed:P1}\n";
            summary += $"  Reserve: {CapacityReserve:P1}\n";
            summary += $"  Status: {Status}\n\n";
            
            summary += $"Zugzahlen:\n";
            summary += $"  Aktuell: {CurrentTrainsPerDay} Züge/Tag\n";
            summary += $"  Maximum (85%): {MaxTrainsPerDay} Züge/Tag\n";
            summary += $"  Wachstumspotenzial: +{MaxTrainsPerDay - CurrentTrainsPerDay} Züge/Tag\n\n";
            
            summary += "EMPFEHLUNGEN:\n";
            foreach (var action in RecommendedActions)
            {
                summary += $"{action}\n";
            }
            
            return summary;
        }
    }

    /// <summary>
    /// Capacity status classification
    /// </summary>
    public enum CapacityStatus
    {
        Low,        // <60% utilization
        Medium,     // 60-75%
        High,       // 75-85%
        Critical    // >85%
    }

    /// <summary>
    /// Validation result
    /// </summary>
    public class ValidationResult
    {
        public bool IsValid { get; set; }
        public List<string> Errors { get; set; } = new();
    }

    #endregion
}
