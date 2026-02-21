/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ConflictDetector.cs                                ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     601                                            ║
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
    /// Detects and resolves conflicts in railway operations including train movements,
    /// signal interlocking, and resource allocation.
    /// </summary>
    public class ConflictDetector
    {
        /// <summary>
        /// Detects all conflicts in a given train schedule.
        /// </summary>
        public ConflictDetectionResult DetectConflicts(ConflictDetectionInput input)
        {
            var conflicts = new List<Conflict>();

            // Detect collision conflicts
            conflicts.AddRange(DetectCollisionConflicts(input.TrainMovements));

            // Detect signal interlocking conflicts
            conflicts.AddRange(DetectSignalConflicts(input.TrainMovements, input.Signals));

            // Detect platform conflicts
            if (input.Stations != null)
            {
                conflicts.AddRange(DetectPlatformConflicts(input.TrainMovements, input.Stations));
            }

            // Detect track section conflicts
            conflicts.AddRange(DetectTrackSectionConflicts(input.TrainMovements));

            // Generate resolution strategies
            var resolutions = GenerateResolutions(conflicts, input.TrainMovements);

            return new ConflictDetectionResult
            {
                Conflicts = conflicts,
                ResolutionStrategies = resolutions,
                TotalConflicts = conflicts.Count,
                CriticalConflicts = conflicts.Count(c => c.Severity == ConflictSeverity.Critical),
                HighConflicts = conflicts.Count(c => c.Severity == ConflictSeverity.High),
                MediumConflicts = conflicts.Count(c => c.Severity == ConflictSeverity.Medium),
                LowConflicts = conflicts.Count(c => c.Severity == ConflictSeverity.Low)
            };
        }

        /// <summary>
        /// Detects collision conflicts between trains.
        /// </summary>
        private List<Conflict> DetectCollisionConflicts(List<TrainMovement> movements)
        {
            var conflicts = new List<Conflict>();

            for (int i = 0; i < movements.Count; i++)
            {
                for (int j = i + 1; j < movements.Count; j++)
                {
                    var conflict = CheckCollision(movements[i], movements[j]);
                    if (conflict != null)
                    {
                        conflicts.Add(conflict);
                    }
                }
            }

            return conflicts;
        }

        /// <summary>
        /// Checks if two train movements collide.
        /// </summary>
        private Conflict? CheckCollision(TrainMovement train1, TrainMovement train2)
        {
            // Check spatial overlap
            var spatialOverlap = CheckSpatialOverlap(train1, train2);
            if (!spatialOverlap) return null;

            // Check temporal overlap
            var temporalOverlap = CheckTemporalOverlap(train1, train2);
            if (!temporalOverlap) return null;

            // Calculate time gap (safety margin)
            var timeGap = CalculateTimeGap(train1, train2);
            var minSafetyGap = 120.0; // 2 minutes minimum

            if (timeGap < minSafetyGap)
            {
                var severity = timeGap < 30 ? ConflictSeverity.Critical :
                              timeGap < 60 ? ConflictSeverity.High :
                              ConflictSeverity.Medium;

                return new Conflict
                {
                    Type = ConflictType.Collision,
                    Severity = severity,
                    Train1Id = train1.TrainId,
                    Train2Id = train2.TrainId,
                    Position = train1.StartPosition,
                    TimeOfConflict = train1.DepartureTime,
                    TimeGapSeconds = timeGap,
                    Description = $"Collision risk: {train1.TrainId} and {train2.TrainId} at position {train1.StartPosition}m with only {timeGap:F1}s gap"
                };
            }

            return null;
        }

        /// <summary>
        /// Checks spatial overlap between two train movements.
        /// </summary>
        private bool CheckSpatialOverlap(TrainMovement train1, TrainMovement train2)
        {
            // Check if routes overlap
            var start1 = Math.Min(train1.StartPosition, train1.EndPosition);
            var end1 = Math.Max(train1.StartPosition, train1.EndPosition);
            var start2 = Math.Min(train2.StartPosition, train2.EndPosition);
            var end2 = Math.Max(train2.StartPosition, train2.EndPosition);

            return !(end1 < start2 || end2 < start1);
        }

        /// <summary>
        /// Checks temporal overlap between two train movements.
        /// </summary>
        private bool CheckTemporalOverlap(TrainMovement train1, TrainMovement train2)
        {
            var start1 = train1.DepartureTime;
            var end1 = train1.ArrivalTime;
            var start2 = train2.DepartureTime;
            var end2 = train2.ArrivalTime;

            return !(end1 < start2 || end2 < start1);
        }

        /// <summary>
        /// Calculates time gap between two trains.
        /// </summary>
        private double CalculateTimeGap(TrainMovement train1, TrainMovement train2)
        {
            // Calculate when each train occupies the overlap point
            var overlapStart = Math.Max(
                Math.Min(train1.StartPosition, train1.EndPosition),
                Math.Min(train2.StartPosition, train2.EndPosition)
            );

            var time1 = CalculateTimeAtPosition(train1, overlapStart);
            var time2 = CalculateTimeAtPosition(train2, overlapStart);

            return Math.Abs((time2 - time1).TotalSeconds);
        }

        /// <summary>
        /// Calculates when a train reaches a specific position.
        /// </summary>
        private DateTime CalculateTimeAtPosition(TrainMovement movement, double position)
        {
            var distance = Math.Abs(position - movement.StartPosition);
            var totalDistance = Math.Abs(movement.EndPosition - movement.StartPosition);
            var totalTime = (movement.ArrivalTime - movement.DepartureTime).TotalSeconds;
            
            var timeToPosition = (distance / totalDistance) * totalTime;
            return movement.DepartureTime.AddSeconds(timeToPosition);
        }

        /// <summary>
        /// Detects signal interlocking conflicts.
        /// </summary>
        private List<Conflict> DetectSignalConflicts(List<TrainMovement> movements, List<Signal>? signals)
        {
            var conflicts = new List<Conflict>();
            if (signals == null) return conflicts;

            foreach (var movement in movements)
            {
                foreach (var signal in signals)
                {
                    // Check if train passes signal when it's red
                    if (signal.Aspect == SignalAspect.Red || signal.Aspect == SignalAspect.Stop)
                    {
                        var trainPassesSignal = movement.StartPosition <= signal.Position && 
                                               movement.EndPosition >= signal.Position;

                        if (trainPassesSignal)
                        {
                            conflicts.Add(new Conflict
                            {
                                Type = ConflictType.SignalViolation,
                                Severity = ConflictSeverity.Critical,
                                Train1Id = movement.TrainId,
                                Position = signal.Position,
                                TimeOfConflict = CalculateTimeAtPosition(movement, signal.Position),
                                Description = $"Signal violation: {movement.TrainId} passes red signal at {signal.Position}m"
                            });
                        }
                    }
                }
            }

            return conflicts;
        }

        /// <summary>
        /// Detects platform conflicts (multiple trains at same platform).
        /// </summary>
        private List<Conflict> DetectPlatformConflicts(List<TrainMovement> movements, List<Station> stations)
        {
            var conflicts = new List<Conflict>();

            foreach (var station in stations)
            {
                // Group movements by platform
                var platformOccupancy = new Dictionary<int, List<(TrainMovement movement, DateTime start, DateTime end)>>();

                foreach (var movement in movements)
                {
                    if (movement.StationStops == null) continue;

                    foreach (var stop in movement.StationStops)
                    {
                        if (stop.StationId != station.Id) continue;

                        if (!platformOccupancy.ContainsKey(stop.PlatformNumber))
                        {
                            platformOccupancy[stop.PlatformNumber] = new List<(TrainMovement, DateTime, DateTime)>();
                        }

                        platformOccupancy[stop.PlatformNumber].Add((movement, stop.ArrivalTime, stop.DepartureTime));
                    }
                }

                // Check for overlaps on each platform
                foreach (var platform in platformOccupancy)
                {
                    var occupancies = platform.Value.OrderBy(o => o.start).ToList();
                    
                    for (int i = 0; i < occupancies.Count - 1; i++)
                    {
                        var current = occupancies[i];
                        var next = occupancies[i + 1];

                        // Check temporal overlap
                        if (current.end > next.start)
                        {
                            var overlapSeconds = (current.end - next.start).TotalSeconds;
                            
                            conflicts.Add(new Conflict
                            {
                                Type = ConflictType.PlatformOccupancy,
                                Severity = ConflictSeverity.High,
                                Train1Id = current.movement.TrainId,
                                Train2Id = next.movement.TrainId,
                                Position = station.Position,
                                TimeOfConflict = next.start,
                                Description = $"Platform conflict: {current.movement.TrainId} and {next.movement.TrainId} at station {station.Name}, platform {platform.Key} (overlap: {overlapSeconds:F0}s)"
                            });
                        }
                    }
                }
            }

            return conflicts;
        }

        /// <summary>
        /// Detects track section conflicts (single-track sections).
        /// </summary>
        private List<Conflict> DetectTrackSectionConflicts(List<TrainMovement> movements)
        {
            var conflicts = new List<Conflict>();

            // Group movements by track section
            var sectionOccupancy = new Dictionary<string, List<TrainMovement>>();

            foreach (var movement in movements)
            {
                if (movement.TrackSectionId == null) continue;

                if (!sectionOccupancy.ContainsKey(movement.TrackSectionId))
                {
                    sectionOccupancy[movement.TrackSectionId] = new List<TrainMovement>();
                }

                sectionOccupancy[movement.TrackSectionId].Add(movement);
            }

            // Check for opposite direction conflicts on single-track sections
            foreach (var section in sectionOccupancy)
            {
                var sectionMovements = section.Value;

                for (int i = 0; i < sectionMovements.Count; i++)
                {
                    for (int j = i + 1; j < sectionMovements.Count; j++)
                    {
                        var m1 = sectionMovements[i];
                        var m2 = sectionMovements[j];

                        // Check if trains move in opposite directions
                        var dir1 = Math.Sign(m1.EndPosition - m1.StartPosition);
                        var dir2 = Math.Sign(m2.EndPosition - m2.StartPosition);

                        if (dir1 != dir2 && CheckTemporalOverlap(m1, m2))
                        {
                            conflicts.Add(new Conflict
                            {
                                Type = ConflictType.OpposingTraffic,
                                Severity = ConflictSeverity.Critical,
                                Train1Id = m1.TrainId,
                                Train2Id = m2.TrainId,
                                Position = m1.StartPosition,
                                TimeOfConflict = m1.DepartureTime,
                                Description = $"Opposing traffic: {m1.TrainId} and {m2.TrainId} on single-track section {section.Key}"
                            });
                        }
                    }
                }
            }

            return conflicts;
        }

        /// <summary>
        /// Generates resolution strategies for detected conflicts.
        /// </summary>
        private List<ResolutionStrategy> GenerateResolutions(List<Conflict> conflicts, List<TrainMovement> movements)
        {
            var resolutions = new List<ResolutionStrategy>();

            foreach (var conflict in conflicts)
            {
                switch (conflict.Type)
                {
                    case ConflictType.Collision:
                        resolutions.Add(GenerateCollisionResolution(conflict, movements));
                        break;

                    case ConflictType.SignalViolation:
                        resolutions.Add(new ResolutionStrategy
                        {
                            ConflictId = conflict.GetHashCode(),
                            Type = ResolutionType.WaitAtSignal,
                            Description = $"Hold {conflict.Train1Id} at signal until clear",
                            EstimatedDelaySeconds = 180,
                            Cost = 5000 // Delay cost
                        });
                        break;

                    case ConflictType.PlatformOccupancy:
                        resolutions.Add(new ResolutionStrategy
                        {
                            ConflictId = conflict.GetHashCode(),
                            Type = ResolutionType.AlternatePlatform,
                            Description = $"Assign {conflict.Train2Id} to alternate platform",
                            EstimatedDelaySeconds = 60,
                            Cost = 2000
                        });
                        break;

                    case ConflictType.OpposingTraffic:
                        resolutions.Add(new ResolutionStrategy
                        {
                            ConflictId = conflict.GetHashCode(),
                            Type = ResolutionType.CreateMeetingPoint,
                            Description = $"Create meeting point: hold {conflict.Train1Id} at siding",
                            EstimatedDelaySeconds = 300,
                            Cost = 15000
                        });
                        break;
                }
            }

            return resolutions;
        }

        /// <summary>
        /// Generates resolution for collision conflicts.
        /// </summary>
        private ResolutionStrategy GenerateCollisionResolution(Conflict conflict, List<TrainMovement> movements)
        {
            // Find the trains involved
            var train1 = movements.FirstOrDefault(m => m.TrainId == conflict.Train1Id);
            var train2 = movements.FirstOrDefault(m => m.TrainId == conflict.Train2Id);

            if (train1 == null || train2 == null)
            {
                return new ResolutionStrategy
                {
                    ConflictId = conflict.GetHashCode(),
                    Type = ResolutionType.DelayDeparture,
                    Description = "Delay later train",
                    EstimatedDelaySeconds = 120,
                    Cost = 10000
                };
            }

            // Determine which train should be delayed (prefer delaying later train)
            var delayTrain = train1.DepartureTime > train2.DepartureTime ? train1 : train2;
            var requiredDelay = Math.Max(120, conflict.TimeGapSeconds * 2); // At least 2 minutes

            return new ResolutionStrategy
            {
                ConflictId = conflict.GetHashCode(),
                Type = ResolutionType.DelayDeparture,
                Description = $"Delay {delayTrain.TrainId} departure by {requiredDelay:F0}s",
                EstimatedDelaySeconds = requiredDelay,
                Cost = requiredDelay * 100, // 100€ per second delay
                AffectedTrainIds = new List<string> { delayTrain.TrainId }
            };
        }
    }

    #region Data Models

    public class ConflictDetectionInput
    {
        public List<TrainMovement> TrainMovements { get; set; } = new();
        public List<Signal>? Signals { get; set; }
        public List<Station>? Stations { get; set; }
    }

    public class ConflictDetectionResult
    {
        public List<Conflict> Conflicts { get; set; } = new();
        public List<ResolutionStrategy> ResolutionStrategies { get; set; } = new();
        public int TotalConflicts { get; set; }
        public int CriticalConflicts { get; set; }
        public int HighConflicts { get; set; }
        public int MediumConflicts { get; set; }
        public int LowConflicts { get; set; }

        public string GetSummary()
        {
            return $@"=== KONFLIKT-DETEKTION ===

Gefundene Konflikte: {TotalConflicts}
  - Kritisch: {CriticalConflicts}
  - Hoch: {HighConflicts}
  - Mittel: {MediumConflicts}
  - Niedrig: {LowConflicts}

Lösungsstrategien: {ResolutionStrategies.Count}

Konflikte:
{string.Join("\n", Conflicts.Take(10).Select(c => $"  • {c.Description}"))}
{(Conflicts.Count > 10 ? $"\n  ... und {Conflicts.Count - 10} weitere" : "")}

EMPFEHLUNGEN:
{GetRecommendations()}";
        }

        private string GetRecommendations()
        {
            var recommendations = new List<string>();

            if (CriticalConflicts > 0)
            {
                recommendations.Add($"🔴 KRITISCH: {CriticalConflicts} kritische Konflikte erfordern sofortige Maßnahmen");
            }

            if (HighConflicts > 3)
            {
                recommendations.Add($"⚠️ WARNUNG: {HighConflicts} Konflikte mit hoher Priorität");
                recommendations.Add("→ Fahrplan-Optimierung empfohlen");
            }

            if (Conflicts.Any(c => c.Type == ConflictType.OpposingTraffic))
            {
                recommendations.Add("→ Zweigleisiger Ausbau für betroffene Abschnitte prüfen");
            }

            if (Conflicts.Count(c => c.Type == ConflictType.PlatformOccupancy) > 2)
            {
                recommendations.Add("→ Zusätzliche Bahnsteige oder optimierte Gleiszuordnung");
            }

            if (recommendations.Count == 0)
            {
                recommendations.Add("✓ Keine kritischen Konflikte - Fahrplan ist durchführbar");
            }

            return string.Join("\n", recommendations);
        }
    }

    public class Conflict
    {
        public ConflictType Type { get; set; }
        public ConflictSeverity Severity { get; set; }
        public string Train1Id { get; set; } = "";
        public string? Train2Id { get; set; }
        public double Position { get; set; }
        public DateTime TimeOfConflict { get; set; }
        public double TimeGapSeconds { get; set; }
        public string Description { get; set; } = "";
    }

    public class ResolutionStrategy
    {
        public int ConflictId { get; set; }
        public ResolutionType Type { get; set; }
        public string Description { get; set; } = "";
        public double EstimatedDelaySeconds { get; set; }
        public double Cost { get; set; } // €
        public List<string> AffectedTrainIds { get; set; } = new();
    }

    public class TrainMovement
    {
        public string TrainId { get; set; } = "";
        public double StartPosition { get; set; } // meters
        public double EndPosition { get; set; } // meters
        public DateTime DepartureTime { get; set; }
        public DateTime ArrivalTime { get; set; }
        public double AverageSpeedKmh { get; set; }
        public string? TrackSectionId { get; set; }
        public List<StationStop>? StationStops { get; set; }
    }

    public class StationStop
    {
        public string StationId { get; set; } = "";
        public int PlatformNumber { get; set; }
        public DateTime ArrivalTime { get; set; }
        public DateTime DepartureTime { get; set; }
    }

    public class Signal
    {
        public string Id { get; set; } = "";
        public double Position { get; set; }
        public SignalAspect Aspect { get; set; }
    }

    public class Station
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public double Position { get; set; }
        public int PlatformCount { get; set; }
    }

    public enum ConflictType
    {
        Collision,
        SignalViolation,
        PlatformOccupancy,
        OpposingTraffic,
        ResourceContention
    }

    public enum ConflictSeverity
    {
        Low,
        Medium,
        High,
        Critical
    }

    public enum SignalAspect
    {
        Green,
        Yellow,
        DoubleYellow,
        Red,
        Stop
    }

    public enum ResolutionType
    {
        DelayDeparture,
        WaitAtSignal,
        AlternatePlatform,
        AlternateRoute,
        CreateMeetingPoint,
        SpeedAdjustment
    }

    #endregion
}
