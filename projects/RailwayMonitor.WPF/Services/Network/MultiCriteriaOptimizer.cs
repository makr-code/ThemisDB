/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MultiCriteriaOptimizer.cs                          ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     770                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Services.Network;

/// <summary>
/// Multi-Criteria Optimization using NSGA-II (Non-dominated Sorting Genetic Algorithm II)
/// Sprint 1, US-1.3: Multi-Criteria Optimization Framework
/// 
/// Optimizes railway routes considering multiple objectives:
/// - Cost minimization
/// - Travel time minimization
/// - Environmental impact minimization
/// </summary>
public class MultiCriteriaOptimizer
{
    private readonly RailwayNetworkAnalyzer _network;
    private readonly Random _random;
    private readonly List<IObjectiveFunction> _objectives;
    
    public MultiCriteriaOptimizer(RailwayNetworkAnalyzer network, int? seed = null)
    {
        _network = network ?? throw new ArgumentNullException(nameof(network));
        _random = seed.HasValue ? new Random(seed.Value) : new Random();
        _objectives = new List<IObjectiveFunction>();
    }
    
    /// <summary>
    /// Add objective function to optimize
    /// </summary>
    public void AddObjective(IObjectiveFunction objective)
    {
        _objectives.Add(objective);
    }
    
    /// <summary>
    /// Run NSGA-II optimization
    /// </summary>
    public OptimizationResult Optimize(OptimizationConfig config)
    {
        if (_objectives.Count == 0)
            throw new InvalidOperationException("At least one objective must be added");
        
        // Initialize population
        var population = InitializePopulation(config.PopulationSize, config.StartStation, config.EndStation);
        
        var result = new OptimizationResult
        {
            StartedAt = DateTime.Now,
            Config = config
        };
        
        // Evolution loop
        for (int generation = 0; generation < config.MaxGenerations; generation++)
        {
            // Evaluate fitness
            EvaluatePopulation(population);
            
            // Non-dominated sorting
            var fronts = NonDominatedSort(population);
            
            // Calculate crowding distance
            foreach (var front in fronts)
            {
                CalculateCrowdingDistance(front);
            }
            
            // Generate offspring
            var offspring = GenerateOffspring(population, config.PopulationSize);
            
            // Combine parent and offspring
            var combined = population.Concat(offspring).ToList();
            EvaluatePopulation(combined);
            
            // Select next generation
            population = SelectNextGeneration(combined, config.PopulationSize);
            
            // Track progress
            if (generation % 10 == 0)
            {
                result.GenerationSnapshots.Add(new GenerationSnapshot
                {
                    Generation = generation,
                    ParetoFrontSize = fronts[0].Count,
                    BestCost = fronts[0].Min(s => s.Objectives[0]), // Assuming first is cost
                    Hypervolume = CalculateHypervolume(fronts[0])
                });
            }
        }
        
        // Extract final Pareto front
        var finalFronts = NonDominatedSort(population);
        result.ParetoFront = finalFronts[0].Select(s => new RouteSolution
        {
            Route = s.Route,
            Objectives = s.Objectives.ToList(),
            ObjectiveNames = _objectives.Select(o => o.Name).ToList()
        }).ToList();
        
        result.CompletedAt = DateTime.Now;
        result.TotalSolutions = population.Count;
        result.ParetoOptimalSolutions = result.ParetoFront.Count;
        
        return result;
    }
    
    /// <summary>
    /// Initialize random population of routes
    /// </summary>
    private List<Solution> InitializePopulation(int size, string startStationId, string endStationId)
    {
        var population = new List<Solution>();
        
        for (int i = 0; i < size; i++)
        {
            var route = GenerateRandomRoute(startStationId, endStationId);
            if (route != null && route.Any())
            {
                population.Add(new Solution { Route = route });
            }
        }
        
        // Ensure we have enough solutions
        while (population.Count < size / 2)
        {
            var route = GenerateRandomRoute(startStationId, endStationId);
            if (route != null && route.Any())
            {
                population.Add(new Solution { Route = route });
            }
        }
        
        return population;
    }
    
    /// <summary>
    /// Generate a random route using random walk
    /// </summary>
    private List<Station> GenerateRandomRoute(string startId, string endId)
    {
        var start = _network.GetStation(startId);
        var end = _network.GetStation(endId);
        
        if (start == null || end == null)
            return new List<Station>();
        
        var route = new List<Station> { start };
        var current = start;
        var visited = new HashSet<string> { start.Id };
        var maxSteps = 50; // Prevent infinite loops
        
        for (int step = 0; step < maxSteps; step++)
        {
            if (current.Id == endId)
                return route;
            
            var neighbors = _network.GetNeighbors(current.Id)
                .Where(n => !visited.Contains(n.Id))
                .ToList();
            
            if (!neighbors.Any())
            {
                // Backtrack or restart
                if (route.Count > 1)
                {
                    route.RemoveAt(route.Count - 1);
                    visited.Remove(current.Id);
                    current = route.Last();
                }
                else
                {
                    break; // Can't find route
                }
            }
            else
            {
                // Prefer neighbors closer to goal (biased random walk)
                var next = SelectBiasedNeighbor(neighbors, end);
                route.Add(next);
                visited.Add(next.Id);
                current = next;
            }
        }
        
        return new List<Station>(); // Failed to find route
    }
    
    /// <summary>
    /// Select neighbor biased towards goal
    /// </summary>
    private Station SelectBiasedNeighbor(List<Station> neighbors, Station goal)
    {
        if (!neighbors.Any())
            throw new ArgumentException("No neighbors available");
        
        // Calculate distances to goal
        var distances = neighbors.Select(n => new
        {
            Station = n,
            Distance = CalculateDistance(n, goal)
        }).ToList();
        
        // Softmax selection (prefer closer but allow randomness)
        var weights = distances.Select(d => Math.Exp(-d.Distance / 100.0)).ToList();
        var totalWeight = weights.Sum();
        
        var rand = _random.NextDouble() * totalWeight;
        var cumulative = 0.0;
        
        for (int i = 0; i < neighbors.Count; i++)
        {
            cumulative += weights[i];
            if (rand <= cumulative)
                return neighbors[i];
        }
        
        return neighbors.Last();
    }
    
    /// <summary>
    /// Calculate Euclidean distance between stations
    /// </summary>
    private double CalculateDistance(Station a, Station b)
    {
        var dLat = (b.Latitude - a.Latitude);
        var dLon = (b.Longitude - a.Longitude);
        return Math.Sqrt(dLat * dLat + dLon * dLon) * 111.0; // Approximate km
    }
    
    /// <summary>
    /// Evaluate all objectives for population
    /// </summary>
    private void EvaluatePopulation(List<Solution> population)
    {
        foreach (var solution in population)
        {
            if (solution.Objectives == null || solution.Objectives.Length == 0)
            {
                solution.Objectives = new double[_objectives.Count];
                
                for (int i = 0; i < _objectives.Count; i++)
                {
                    solution.Objectives[i] = _objectives[i].Evaluate(solution.Route, _network);
                }
            }
        }
    }
    
    /// <summary>
    /// Non-dominated sorting (Fast non-dominated sort)
    /// </summary>
    private List<List<Solution>> NonDominatedSort(List<Solution> population)
    {
        var fronts = new List<List<Solution>>();
        
        // Calculate domination for each solution
        foreach (var p in population)
        {
            p.DominatedSolutions.Clear();
            p.DominationCount = 0;
            
            foreach (var q in population)
            {
                if (p == q) continue;
                
                if (Dominates(p, q))
                {
                    p.DominatedSolutions.Add(q);
                }
                else if (Dominates(q, p))
                {
                    p.DominationCount++;
                }
            }
            
            if (p.DominationCount == 0)
            {
                p.Rank = 0;
                if (fronts.Count == 0)
                    fronts.Add(new List<Solution>());
                fronts[0].Add(p);
            }
        }
        
        // Build subsequent fronts
        int i = 0;
        while (i < fronts.Count && fronts[i].Any())
        {
            var nextFront = new List<Solution>();
            
            foreach (var p in fronts[i])
            {
                foreach (var q in p.DominatedSolutions)
                {
                    q.DominationCount--;
                    if (q.DominationCount == 0)
                    {
                        q.Rank = i + 1;
                        nextFront.Add(q);
                    }
                }
            }
            
            if (nextFront.Any())
            {
                fronts.Add(nextFront);
            }
            i++;
        }
        
        return fronts;
    }
    
    /// <summary>
    /// Check if solution p dominates solution q
    /// </summary>
    private bool Dominates(Solution p, Solution q)
    {
        bool atLeastOneBetter = false;
        
        for (int i = 0; i < p.Objectives.Length; i++)
        {
            if (p.Objectives[i] > q.Objectives[i])
                return false; // Worse in at least one objective
            
            if (p.Objectives[i] < q.Objectives[i])
                atLeastOneBetter = true;
        }
        
        return atLeastOneBetter;
    }
    
    /// <summary>
    /// Calculate crowding distance for a front
    /// </summary>
    private void CalculateCrowdingDistance(List<Solution> front)
    {
        if (front.Count == 0)
            return;
        
        int objectiveCount = front[0].Objectives.Length;
        
        // Initialize distances
        foreach (var sol in front)
            sol.CrowdingDistance = 0;
        
        // For each objective
        for (int m = 0; m < objectiveCount; m++)
        {
            // Sort by objective m
            var sorted = front.OrderBy(s => s.Objectives[m]).ToList();
            
            // Boundary solutions have infinite distance
            sorted[0].CrowdingDistance = double.MaxValue;
            sorted[sorted.Count - 1].CrowdingDistance = double.MaxValue;
            
            // Calculate range
            var range = sorted[sorted.Count - 1].Objectives[m] - sorted[0].Objectives[m];
            
            if (range > 0)
            {
                for (int i = 1; i < sorted.Count - 1; i++)
                {
                    sorted[i].CrowdingDistance +=
                        (sorted[i + 1].Objectives[m] - sorted[i - 1].Objectives[m]) / range;
                }
            }
        }
    }
    
    /// <summary>
    /// Generate offspring through crossover and mutation
    /// </summary>
    private List<Solution> GenerateOffspring(List<Solution> population, int count)
    {
        var offspring = new List<Solution>();
        
        while (offspring.Count < count)
        {
            // Tournament selection
            var parent1 = TournamentSelect(population);
            var parent2 = TournamentSelect(population);
            
            // Crossover
            var child = Crossover(parent1, parent2);
            
            // Mutation
            if (_random.NextDouble() < 0.1) // 10% mutation rate
            {
                child = Mutate(child);
            }
            
            if (child.Route.Any())
            {
                offspring.Add(child);
            }
        }
        
        return offspring;
    }
    
    /// <summary>
    /// Tournament selection
    /// </summary>
    private Solution TournamentSelect(List<Solution> population, int tournamentSize = 3)
    {
        var tournament = new List<Solution>();
        
        for (int i = 0; i < tournamentSize; i++)
        {
            tournament.Add(population[_random.Next(population.Count)]);
        }
        
        // Select best (lowest rank, or highest crowding distance if same rank)
        return tournament.OrderBy(s => s.Rank)
            .ThenByDescending(s => s.CrowdingDistance)
            .First();
    }
    
    /// <summary>
    /// Crossover two routes
    /// </summary>
    private Solution Crossover(Solution parent1, Solution parent2)
    {
        if (!parent1.Route.Any() || !parent2.Route.Any())
            return new Solution { Route = parent1.Route.ToList() };
        
        // Find common stations
        var common = parent1.Route.Select(s => s.Id)
            .Intersect(parent2.Route.Select(s => s.Id))
            .ToList();
        
        if (common.Count < 2)
            return new Solution { Route = parent1.Route.ToList() };
        
        // Select random common station as crossover point
        var crossPoint = common[_random.Next(common.Count)];
        
        var idx1 = parent1.Route.FindIndex(s => s.Id == crossPoint);
        var idx2 = parent2.Route.FindIndex(s => s.Id == crossPoint);
        
        // Combine: first part from parent1, second from parent2
        var childRoute = parent1.Route.Take(idx1 + 1)
            .Concat(parent2.Route.Skip(idx2 + 1))
            .ToList();
        
        return new Solution { Route = childRoute };
    }
    
    /// <summary>
    /// Mutate a solution by replacing a segment
    /// </summary>
    private Solution Mutate(Solution solution)
    {
        if (solution.Route.Count < 3)
            return solution;
        
        // Select random segment to replace
        var start = _random.Next(solution.Route.Count - 2);
        var end = _random.Next(start + 1, solution.Route.Count);
        
        // Generate new segment
        var newSegment = GenerateRandomRoute(
            solution.Route[start].Id,
            solution.Route[end].Id
        );
        
        if (newSegment.Any())
        {
            var mutated = solution.Route.Take(start)
                .Concat(newSegment)
                .Concat(solution.Route.Skip(end))
                .ToList();
            
            return new Solution { Route = mutated };
        }
        
        return solution;
    }
    
    /// <summary>
    /// Select next generation using elitism
    /// </summary>
    private List<Solution> SelectNextGeneration(List<Solution> combined, int size)
    {
        var fronts = NonDominatedSort(combined);
        var nextGen = new List<Solution>();
        
        // Add fronts until size is reached
        int i = 0;
        while (i < fronts.Count && nextGen.Count + fronts[i].Count <= size)
        {
            CalculateCrowdingDistance(fronts[i]);
            nextGen.AddRange(fronts[i]);
            i++;
        }
        
        // Fill remaining slots from next front based on crowding distance
        if (i < fronts.Count && nextGen.Count < size)
        {
            CalculateCrowdingDistance(fronts[i]);
            var sorted = fronts[i].OrderByDescending(s => s.CrowdingDistance).ToList();
            nextGen.AddRange(sorted.Take(size - nextGen.Count));
        }
        
        return nextGen;
    }
    
    /// <summary>
    /// Calculate hypervolume indicator (quality metric)
    /// </summary>
    private double CalculateHypervolume(List<Solution> front)
    {
        if (!front.Any())
            return 0;
        
        // Simplified 2D hypervolume (for first two objectives)
        if (front[0].Objectives.Length < 2)
            return 0;
        
        var sorted = front.OrderBy(s => s.Objectives[0]).ToList();
        double volume = 0;
        
        for (int i = 0; i < sorted.Count - 1; i++)
        {
            var width = sorted[i + 1].Objectives[0] - sorted[i].Objectives[0];
            var height = sorted[i].Objectives[1];
            volume += width * height;
        }
        
        return volume;
    }
}

// ============================================================================
// Supporting Classes
// ============================================================================

/// <summary>
/// Solution in the population
/// </summary>
public class Solution
{
    public List<Station> Route { get; set; } = new();
    public double[] Objectives { get; set; } = Array.Empty<double>();
    
    // NSGA-II specific
    public int Rank { get; set; }
    public double CrowdingDistance { get; set; }
    public int DominationCount { get; set; }
    public List<Solution> DominatedSolutions { get; set; } = new();
}

/// <summary>
/// Objective function interface
/// </summary>
public interface IObjectiveFunction
{
    string Name { get; }
    double Evaluate(List<Station> route, RailwayNetworkAnalyzer network);
}

/// <summary>
/// Cost minimization objective
/// </summary>
public class CostObjective : IObjectiveFunction
{
    public string Name => "Total Cost (€)";
    
    public double Evaluate(List<Station> route, RailwayNetworkAnalyzer network)
    {
        if (route.Count < 2)
            return double.MaxValue;
        
        double totalCost = 0;
        
        for (int i = 0; i < route.Count - 1; i++)
        {
            var edges = network.GetConnectedEdges(route[i].Id);
            var edge = edges.FirstOrDefault(e => e.To.Id == route[i + 1].Id);
            
            if (edge != null)
            {
                // Simplified cost: 10M € per km base
                totalCost += edge.LengthKm * 10_000_000;
                
                // Higher cost for high-speed
                if (edge.IsHighSpeed)
                    totalCost += edge.LengthKm * 5_000_000;
            }
            else
            {
                return double.MaxValue; // Invalid route
            }
        }
        
        return totalCost;
    }
}

/// <summary>
/// Travel time minimization objective
/// </summary>
public class TravelTimeObjective : IObjectiveFunction
{
    public string Name => "Travel Time (min)";
    
    public double Evaluate(List<Station> route, RailwayNetworkAnalyzer network)
    {
        if (route.Count < 2)
            return double.MaxValue;
        
        double totalTime = 0;
        
        for (int i = 0; i < route.Count - 1; i++)
        {
            var edges = network.GetConnectedEdges(route[i].Id);
            var edge = edges.FirstOrDefault(e => e.To.Id == route[i + 1].Id);
            
            if (edge != null)
            {
                // Time = Distance / Speed
                totalTime += (edge.LengthKm / edge.MaxSpeedKmh) * 60; // minutes
                
                // Add station stop time
                totalTime += 3; // 3 minutes per station
            }
            else
            {
                return double.MaxValue;
            }
        }
        
        return totalTime;
    }
}

/// <summary>
/// Environmental impact minimization objective
/// </summary>
public class EnvironmentalObjective : IObjectiveFunction
{
    public string Name => "Environmental Impact (CO₂ tons)";
    
    public double Evaluate(List<Station> route, RailwayNetworkAnalyzer network)
    {
        if (route.Count < 2)
            return double.MaxValue;
        
        double totalImpact = 0;
        
        for (int i = 0; i < route.Count - 1; i++)
        {
            var edges = network.GetConnectedEdges(route[i].Id);
            var edge = edges.FirstOrDefault(e => e.To.Id == route[i + 1].Id);
            
            if (edge != null)
            {
                // Simplified CO₂: construction impact
                totalImpact += edge.LengthKm * 100; // 100 tons CO₂ per km
                
                // Penalty for non-electrified
                if (!edge.IsElectrified)
                    totalImpact += edge.LengthKm * 50;
            }
            else
            {
                return double.MaxValue;
            }
        }
        
        return totalImpact;
    }
}

/// <summary>
/// Optimization configuration
/// </summary>
public class OptimizationConfig
{
    public string StartStation { get; set; } = "";
    public string EndStation { get; set; } = "";
    public int PopulationSize { get; set; } = 100;
    public int MaxGenerations { get; set; } = 100;
    public double CrossoverRate { get; set; } = 0.9;
    public double MutationRate { get; set; } = 0.1;
}

/// <summary>
/// Optimization result with Pareto front
/// </summary>
public class OptimizationResult
{
    public DateTime StartedAt { get; set; }
    public DateTime CompletedAt { get; set; }
    public OptimizationConfig Config { get; set; } = new();
    
    public List<RouteSolution> ParetoFront { get; set; } = new();
    public int TotalSolutions { get; set; }
    public int ParetoOptimalSolutions { get; set; }
    
    public List<GenerationSnapshot> GenerationSnapshots { get; set; } = new();
    
    public TimeSpan Duration => CompletedAt - StartedAt;
    
    public override string ToString()
    {
        return $"Optimization completed in {Duration.TotalSeconds:F1}s\n" +
               $"Pareto-optimal solutions: {ParetoOptimalSolutions}\n" +
               $"Best cost: {ParetoFront.FirstOrDefault()?.Objectives[0]:N0} €\n" +
               $"Best time: {ParetoFront.LastOrDefault()?.Objectives[1]:F1} min";
    }
}

/// <summary>
/// Route solution with objective values
/// </summary>
public class RouteSolution
{
    public List<Station> Route { get; set; } = new();
    public List<double> Objectives { get; set; } = new();
    public List<string> ObjectiveNames { get; set; } = new();
    
    public string GetRouteDescription()
    {
        if (!Route.Any())
            return "Empty route";
        
        return $"{Route.First().Name} → {Route.Last().Name} ({Route.Count} stations)";
    }
}

/// <summary>
/// Snapshot of generation progress
/// </summary>
public class GenerationSnapshot
{
    public int Generation { get; set; }
    public int ParetoFrontSize { get; set; }
    public double BestCost { get; set; }
    public double Hypervolume { get; set; }
}
