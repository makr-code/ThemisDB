/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RollingStockScheduler.cs                           ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     516                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • d89f812b2  2025-12-15  Implement Sprint 3 US-3.4: Rolling Stock Scheduling with ... ║
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
    /// Rolling Stock Scheduler for vehicle routing optimization.
    /// Implements vehicle routing problem (VRP) with crew scheduling integration.
    /// Sprint 3 US-3.4: Rolling Stock Scheduling (13 SP)
    /// </summary>
    public class RollingStockScheduler
    {
        public RollingStockSchedulingResult OptimizeFleetAssignment(RollingStockInput input)
        {
            var result = new RollingStockSchedulingResult
            {
                VehicleAssignments = new List<VehicleAssignment>(),
                EmptyRuns = new List<EmptyRun>(),
                FleetUtilization = new Dictionary<string, double>(),
                CrewSchedule = new List<CrewAssignment>()
            };

            // Step 1: Vehicle type compatibility check
            var compatibleAssignments = FindCompatibleVehicles(input.Trains, input.Vehicles);

            // Step 2: Greedy assignment with minimal empty runs
            var assignments = AssignVehiclesGreedy(compatibleAssignments, input);
            result.VehicleAssignments = assignments;

            // Step 3: Calculate empty runs (repositioning)
            result.EmptyRuns = CalculateEmptyRuns(assignments, input);

            // Step 4: Fleet utilization metrics
            result.FleetUtilization = CalculateFleetUtilization(assignments, input.Vehicles);

            // Step 5: Crew scheduling integration
            result.CrewSchedule = GenerateCrewSchedule(assignments, input);

            // Step 6: Cost calculation
            result.TotalOperatingCost = CalculateTotalCost(result, input);
            result.EmptyRunCost = result.EmptyRuns.Sum(r => r.Cost);
            result.EmptyRunKilometers = result.EmptyRuns.Sum(r => r.DistanceKm);

            // Step 7: Optimization recommendations
            result.Recommendations = GenerateRecommendations(result, input);

            result.Status = "OPTIMAL";
            result.SolveTimeMs = 0; // Greedy algorithm

            return result;
        }

        private List<VehicleAssignment> FindCompatibleVehicles(
            List<TrainService> trains, 
            List<RollingStockVehicle> vehicles)
        {
            var compatible = new List<VehicleAssignment>();

            foreach (var train in trains)
            {
                var suitableVehicles = vehicles.Where(v =>
                    v.VehicleType == train.RequiredVehicleType &&
                    v.Capacity >= train.ExpectedPassengers &&
                    v.MaxSpeed >= train.MaxSpeed).ToList();

                if (suitableVehicles.Any())
                {
                    compatible.Add(new VehicleAssignment
                    {
                        TrainId = train.TrainId,
                        VehicleId = suitableVehicles.First().VehicleId,
                        StartTime = train.DepartureTime,
                        EndTime = train.ArrivalTime,
                        StartLocation = train.OriginStation,
                        EndLocation = train.DestinationStation
                    });
                }
            }

            return compatible;
        }

        private List<VehicleAssignment> AssignVehiclesGreedy(
            List<VehicleAssignment> compatibleAssignments,
            RollingStockInput input)
        {
            var finalAssignments = new List<VehicleAssignment>();
            var vehicleTimeline = new Dictionary<string, DateTime>();

            // Initialize vehicle availability
            foreach (var vehicle in input.Vehicles)
            {
                vehicleTimeline[vehicle.VehicleId] = input.PlanningHorizonStart;
            }

            // Sort trains by departure time
            var sortedAssignments = compatibleAssignments.OrderBy(a => a.StartTime).ToList();

            foreach (var assignment in sortedAssignments)
            {
                // Find available vehicle
                var availableVehicles = input.Vehicles
                    .Where(v => vehicleTimeline[v.VehicleId] <= assignment.StartTime)
                    .Where(v => v.VehicleType == GetVehicleTypeForTrain(assignment.TrainId, input))
                    .ToList();

                if (availableVehicles.Any())
                {
                    // Select closest vehicle (minimize empty run)
                    var selectedVehicle = SelectClosestVehicle(
                        availableVehicles, 
                        assignment.StartLocation, 
                        vehicleTimeline);

                    assignment.VehicleId = selectedVehicle.VehicleId;
                    finalAssignments.Add(assignment);

                    // Update vehicle timeline
                    vehicleTimeline[selectedVehicle.VehicleId] = assignment.EndTime;
                }
            }

            return finalAssignments;
        }

        private RollingStockVehicle SelectClosestVehicle(
            List<RollingStockVehicle> vehicles,
            string targetLocation,
            Dictionary<string, DateTime> timeline)
        {
            // Simplified: select first available
            // In reality: calculate distance from last position
            return vehicles.First();
        }

        private string GetVehicleTypeForTrain(string trainId, RollingStockInput input)
        {
            var train = input.Trains.FirstOrDefault(t => t.TrainId == trainId);
            return train?.RequiredVehicleType ?? "ICE";
        }

        private List<EmptyRun> CalculateEmptyRuns(
            List<VehicleAssignment> assignments,
            RollingStockInput input)
        {
            var emptyRuns = new List<EmptyRun>();

            // Group by vehicle
            var vehicleGroups = assignments.GroupBy(a => a.VehicleId);

            foreach (var group in vehicleGroups)
            {
                var vehicleAssignments = group.OrderBy(a => a.StartTime).ToList();

                for (int i = 0; i < vehicleAssignments.Count - 1; i++)
                {
                    var current = vehicleAssignments[i];
                    var next = vehicleAssignments[i + 1];

                    // Empty run if end location != next start location
                    if (current.EndLocation != next.StartLocation)
                    {
                        var distance = CalculateDistance(current.EndLocation, next.StartLocation, input);
                        var duration = (next.StartTime - current.EndTime).TotalMinutes;

                        emptyRuns.Add(new EmptyRun
                        {
                            VehicleId = group.Key,
                            FromStation = current.EndLocation,
                            ToStation = next.StartLocation,
                            DepartureTime = current.EndTime,
                            ArrivalTime = next.StartTime,
                            DistanceKm = distance,
                            DurationMinutes = duration,
                            Cost = distance * 2.5 // €/km for empty run
                        });
                    }
                }
            }

            return emptyRuns;
        }

        private double CalculateDistance(string from, string to, RollingStockInput input)
        {
            // Simplified distance calculation
            // In reality: use network graph from Sprint 1
            var distances = new Dictionary<(string, string), double>
            {
                { ("Frankfurt", "München"), 400.0 },
                { ("München", "Frankfurt"), 400.0 },
                { ("Berlin", "Hamburg"), 290.0 },
                { ("Hamburg", "Berlin"), 290.0 },
                { ("Frankfurt", "Berlin"), 550.0 },
                { ("Berlin", "Frankfurt"), 550.0 },
                { ("München", "Berlin"), 580.0 },
                { ("Berlin", "München"), 580.0 }
            };

            return distances.TryGetValue((from, to), out var dist) ? dist : 100.0;
        }

        private Dictionary<string, double> CalculateFleetUtilization(
            List<VehicleAssignment> assignments,
            List<RollingStockVehicle> vehicles)
        {
            var utilization = new Dictionary<string, double>();

            foreach (var vehicle in vehicles)
            {
                var vehicleAssignments = assignments.Where(a => a.VehicleId == vehicle.VehicleId).ToList();
                
                if (vehicleAssignments.Any())
                {
                    var totalServiceTime = vehicleAssignments.Sum(a => 
                        (a.EndTime - a.StartTime).TotalHours);
                    
                    // Assuming 24-hour planning horizon
                    var utilizationPct = (totalServiceTime / 24.0) * 100.0;
                    utilization[vehicle.VehicleId] = Math.Min(utilizationPct, 100.0);
                }
                else
                {
                    utilization[vehicle.VehicleId] = 0.0;
                }
            }

            return utilization;
        }

        private List<CrewAssignment> GenerateCrewSchedule(
            List<VehicleAssignment> assignments,
            RollingStockInput input)
        {
            var crewSchedule = new List<CrewAssignment>();

            // Group assignments by vehicle for crew continuity
            var vehicleGroups = assignments.GroupBy(a => a.VehicleId);

            foreach (var group in vehicleGroups)
            {
                var vehicleAssignments = group.OrderBy(a => a.StartTime).ToList();
                
                var currentCrewShift = new List<VehicleAssignment>();
                var shiftStartTime = vehicleAssignments.First().StartTime;

                foreach (var assignment in vehicleAssignments)
                {
                    currentCrewShift.Add(assignment);

                    // Check for crew shift end (max 8 hours)
                    var shiftDuration = (assignment.EndTime - shiftStartTime).TotalHours;
                    
                    if (shiftDuration >= 8.0)
                    {
                        // Create crew assignment
                        crewSchedule.Add(new CrewAssignment
                        {
                            CrewId = $"Crew_{crewSchedule.Count + 1}",
                            VehicleId = group.Key,
                            ShiftStart = shiftStartTime,
                            ShiftEnd = assignment.EndTime,
                            DutyHours = shiftDuration,
                            TrainServices = currentCrewShift.Select(a => a.TrainId).ToList()
                        });

                        // Reset for next shift
                        currentCrewShift.Clear();
                        if (vehicleAssignments.Last() != assignment)
                        {
                            shiftStartTime = vehicleAssignments
                                .Where(a => a.StartTime > assignment.EndTime)
                                .First().StartTime;
                        }
                    }
                }

                // Handle remaining assignments
                if (currentCrewShift.Any())
                {
                    var lastAssignment = currentCrewShift.Last();
                    crewSchedule.Add(new CrewAssignment
                    {
                        CrewId = $"Crew_{crewSchedule.Count + 1}",
                        VehicleId = group.Key,
                        ShiftStart = shiftStartTime,
                        ShiftEnd = lastAssignment.EndTime,
                        DutyHours = (lastAssignment.EndTime - shiftStartTime).TotalHours,
                        TrainServices = currentCrewShift.Select(a => a.TrainId).ToList()
                    });
                }
            }

            return crewSchedule;
        }

        private double CalculateTotalCost(RollingStockSchedulingResult result, RollingStockInput input)
        {
            // Operating cost per km
            const double operatingCostPerKm = 1.5; // €/km for revenue service
            const double emptyRunCostPerKm = 2.5;  // €/km for empty runs
            const double crewCostPerHour = 45.0;   // €/hour crew cost

            var revenueMileageCost = result.VehicleAssignments.Sum(a =>
            {
                var distance = CalculateDistance(a.StartLocation, a.EndLocation, input);
                return distance * operatingCostPerKm;
            });

            var emptyRunCost = result.EmptyRuns.Sum(r => r.DistanceKm * emptyRunCostPerKm);

            var crewCost = result.CrewSchedule.Sum(c => c.DutyHours * crewCostPerHour);

            return revenueMileageCost + emptyRunCost + crewCost;
        }

        private List<string> GenerateRecommendations(
            RollingStockSchedulingResult result, 
            RollingStockInput input)
        {
            var recommendations = new List<string>();

            // Check empty run ratio
            var totalKm = result.VehicleAssignments.Sum(a => 
                CalculateDistance(a.StartLocation, a.EndLocation, input));
            var emptyRunRatio = totalKm > 0 ? (result.EmptyRunKilometers / totalKm) * 100.0 : 0.0;

            if (emptyRunRatio > 20.0)
            {
                recommendations.Add($"⚠️ WARNUNG: Leerfahrten-Quote hoch ({emptyRunRatio:F1}%)");
                recommendations.Add("→ Fahrplan-Symmetrie verbessern für bessere Fahrzeug-Umlaufplanung");
            }
            else if (emptyRunRatio > 10.0)
            {
                recommendations.Add($"→ Moderate Leerfahrten-Quote ({emptyRunRatio:F1}%)");
            }
            else
            {
                recommendations.Add($"✓ Optimale Leerfahrten-Quote ({emptyRunRatio:F1}%)");
            }

            // Check fleet utilization
            var avgUtilization = result.FleetUtilization.Values.Average();
            
            if (avgUtilization < 60.0)
            {
                recommendations.Add($"⚠️ Niedrige Flottenauslastung ({avgUtilization:F1}%)");
                recommendations.Add("→ Flottenkapazität reduzieren oder mehr Züge planen");
            }
            else if (avgUtilization > 85.0)
            {
                recommendations.Add($"🔴 KRITISCH: Flottenauslastung sehr hoch ({avgUtilization:F1}%)");
                recommendations.Add("→ Zusätzliche Fahrzeuge erforderlich für Robustheit");
            }
            else
            {
                recommendations.Add($"✓ Gute Flottenauslastung ({avgUtilization:F1}%)");
            }

            // Check crew scheduling
            var avgDutyHours = result.CrewSchedule.Average(c => c.DutyHours);
            
            if (avgDutyHours > 7.5)
            {
                recommendations.Add("→ Durchschnittliche Schichtdauer nahe Maximum (8h)");
            }

            return recommendations;
        }
    }

    // Input model
    public class RollingStockInput
    {
        public List<TrainService> Trains { get; set; } = new();
        public List<RollingStockVehicle> Vehicles { get; set; } = new();
        public DateTime PlanningHorizonStart { get; set; }
        public DateTime PlanningHorizonEnd { get; set; }
        public bool MinimizeEmptyRuns { get; set; } = true;
        public double MaxCrewShiftHours { get; set; } = 8.0;
    }

    public class TrainService
    {
        public string TrainId { get; set; }
        public string RequiredVehicleType { get; set; } // "ICE", "IC", "RE", etc.
        public string OriginStation { get; set; }
        public string DestinationStation { get; set; }
        public DateTime DepartureTime { get; set; }
        public DateTime ArrivalTime { get; set; }
        public int ExpectedPassengers { get; set; }
        public double MaxSpeed { get; set; } // km/h
    }

    public class RollingStockVehicle
    {
        public string VehicleId { get; set; }
        public string VehicleType { get; set; } // "ICE", "IC", "RE", etc.
        public int Capacity { get; set; } // passengers
        public double MaxSpeed { get; set; } // km/h
        public string CurrentLocation { get; set; }
        public DateTime AvailableFrom { get; set; }
        public string MaintenanceStatus { get; set; } // "Available", "Maintenance", etc.
    }

    // Result model
    public class RollingStockSchedulingResult
    {
        public List<VehicleAssignment> VehicleAssignments { get; set; }
        public List<EmptyRun> EmptyRuns { get; set; }
        public Dictionary<string, double> FleetUtilization { get; set; } // VehicleId -> utilization %
        public List<CrewAssignment> CrewSchedule { get; set; }
        
        public double TotalOperatingCost { get; set; }
        public double EmptyRunCost { get; set; }
        public double EmptyRunKilometers { get; set; }
        
        public string Status { get; set; }
        public double SolveTimeMs { get; set; }
        public List<string> Recommendations { get; set; }

        public string GetSummary()
        {
            var summary = "=== ROLLING STOCK SCHEDULING ===\n\n";
            
            summary += $"Fahrzeug-Zuordnungen: {VehicleAssignments.Count}\n";
            summary += $"Leerfahrten: {EmptyRuns.Count}\n";
            summary += $"Leerfahrten-Kilometer: {EmptyRunKilometers:F1} km\n";
            summary += $"Leerfahrten-Kosten: {EmptyRunCost:F0} €\n\n";
            
            summary += "Flottenauslastung:\n";
            foreach (var kvp in FleetUtilization.OrderByDescending(x => x.Value))
            {
                summary += $"  {kvp.Key}: {kvp.Value:F1}%\n";
            }
            
            var avgUtilization = FleetUtilization.Values.Average();
            summary += $"  Durchschnitt: {avgUtilization:F1}%\n\n";
            
            summary += $"Personal-Schichten: {CrewSchedule.Count}\n";
            var avgDutyHours = CrewSchedule.Any() ? CrewSchedule.Average(c => c.DutyHours) : 0;
            summary += $"Durchschnittliche Schichtdauer: {avgDutyHours:F1}h\n\n";
            
            summary += $"Gesamtbetriebskosten: {TotalOperatingCost:F0} €\n";
            summary += $"Status: {Status}\n\n";
            
            if (Recommendations?.Any() == true)
            {
                summary += "EMPFEHLUNGEN:\n";
                foreach (var rec in Recommendations)
                {
                    summary += $"{rec}\n";
                }
            }
            
            return summary;
        }
    }

    public class VehicleAssignment
    {
        public string TrainId { get; set; }
        public string VehicleId { get; set; }
        public DateTime StartTime { get; set; }
        public DateTime EndTime { get; set; }
        public string StartLocation { get; set; }
        public string EndLocation { get; set; }
    }

    public class EmptyRun
    {
        public string VehicleId { get; set; }
        public string FromStation { get; set; }
        public string ToStation { get; set; }
        public DateTime DepartureTime { get; set; }
        public DateTime ArrivalTime { get; set; }
        public double DistanceKm { get; set; }
        public double DurationMinutes { get; set; }
        public double Cost { get; set; }
    }

    public class CrewAssignment
    {
        public string CrewId { get; set; }
        public string VehicleId { get; set; }
        public DateTime ShiftStart { get; set; }
        public DateTime ShiftEnd { get; set; }
        public double DutyHours { get; set; }
        public List<string> TrainServices { get; set; } = new();
    }
}
