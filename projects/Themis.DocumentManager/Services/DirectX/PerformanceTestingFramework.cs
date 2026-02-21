/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            PerformanceTestingFramework.cs                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     486                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services.DirectX
{
    /// <summary>
    /// Comprehensive performance testing framework for DirectX 3D rendering
    /// </summary>
    public class PerformanceMetrics
    {
        public string TestName { get; set; } = "Test";
        public int NodeCount { get; set; }
        public int EdgeCount { get; set; }
        public int FrameCount { get; set; }
        
        public double AverageFrameTime { get; set; }          // ms
        public double MinFrameTime { get; set; }              // ms
        public double MaxFrameTime { get; set; }              // ms
        public double StandardDeviation { get; set; }         // ms
        public double AverageFPS { get; set; }
        public double MinFPS { get; set; }
        public double MaxFPS { get; set; }
        
        public long PeakMemoryUsage { get; set; }             // bytes
        public long AverageMemoryUsage { get; set; }          // bytes
        public double MemoryGrowthRate { get; set; }          // bytes/frame
        
        public long TotalGPUBufferSize { get; set; }          // bytes
        public int MeshCacheHits { get; set; }
        public int MeshCacheMisses { get; set; }
        public double CacheHitRate { get; set; }
        
        public long TotalRenderTime { get; set; }             // ms
        public double GPUUtilizationEstimate { get; set; }    // 0-100%
        
        public DateTime TestStartTime { get; set; }
        public DateTime TestEndTime { get; set; }
        public double TestDurationSeconds { get; set; }
        
        public List<string> Warnings { get; set; } = new();
        public List<string> BottleneckIdentified { get; set; } = new();
        
        public override string ToString()
        {
            return $@"
╔════════════════════════════════════════════════════════════╗
║ PERFORMANCE TEST RESULTS: {TestName,-30} ║
╚════════════════════════════════════════════════════════════╝

Graph Configuration:
  Nodes: {NodeCount} | Edges: {EdgeCount}
  Frames Rendered: {FrameCount}
  Total Duration: {TestDurationSeconds:F2} seconds

Frame Timing:
  Average Frame Time: {AverageFrameTime:F3} ms
  Min Frame Time:     {MinFrameTime:F3} ms
  Max Frame Time:     {MaxFrameTime:F3} ms
  Std Deviation:      {StandardDeviation:F3} ms

FPS Performance:
  Average FPS:        {AverageFPS:F1} FPS
  Min FPS:            {MinFPS:F1} FPS
  Max FPS:            {MaxFPS:F1} FPS
  
Memory Usage:
  Peak Memory:        {PeakMemoryUsage / 1024 / 1024:F2} MB
  Avg Memory:         {AverageMemoryUsage / 1024 / 1024:F2} MB
  Growth Rate:        {MemoryGrowthRate:F2} bytes/frame

GPU Resources:
  Buffer Size:        {TotalGPUBufferSize / 1024 / 1024:F2} MB
  Cache Hit Rate:     {CacheHitRate * 100:F1}%
  Mesh Hits:          {MeshCacheHits} | Misses: {MeshCacheMisses}

GPU Utilization Estimate: {GPUUtilizationEstimate:F1}%

{(Warnings.Count > 0 ? "⚠️ WARNINGS:\n" + string.Join("\n", Warnings.Select(w => "  • " + w)) : "")}

{(BottleneckIdentified.Count > 0 ? "🔴 IDENTIFIED BOTTLENECKS:\n" + string.Join("\n", BottleneckIdentified.Select(b => "  • " + b)) : "")}
";
        }
    }

    /// <summary>
    /// Frame timing measurement
    /// </summary>
    public class FrameTimingData
    {
        public long FrameNumber { get; set; }
        public double FrameTimeMs { get; set; }
        public double FPS { get; set; }
        public long MemoryUsageBytes { get; set; }
        public int CommandsProcessed { get; set; }
        public int TrianglesRendered { get; set; }
        public int DrawCalls { get; set; }
        public Dictionary<string, double> StageTimings { get; set; } = new();
    }

    /// <summary>
    /// Performance profiler with detailed timing breakdown
    /// </summary>
    public class PerformanceProfiler : IDisposable
    {
        private Dictionary<string, List<double>> _timingMap = new();
        private Dictionary<string, Stopwatch> _activeTimers = new();
        private List<FrameTimingData> _frameData = new();
        private Stopwatch _overallTimer = new();
        private long _initialMemory = 0;
        private long _peakMemory = 0;
        private long _totalMemory = 0;
        private int _memoryMeasurements = 0;
        
        private int _meshCacheHits = 0;
        private int _meshCacheMisses = 0;
        private int _totalCommandsProcessed = 0;
        private int _totalTrianglesRendered = 0;

        public void StartTest()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            _initialMemory = GC.GetTotalMemory(true);
            _overallTimer.Restart();
        }

        public void EndTest()
        {
            _overallTimer.Stop();
        }

        public void RecordFrameTiming(string stageName, double timeMs)
        {
            if (!_timingMap.ContainsKey(stageName))
                _timingMap[stageName] = new();
            
            _timingMap[stageName].Add(timeMs);
        }

        public void StartStage(string stageName)
        {
            if (!_activeTimers.ContainsKey(stageName))
                _activeTimers[stageName] = new();
            
            _activeTimers[stageName].Restart();
        }

        public void EndStage(string stageName)
        {
            if (_activeTimers.TryGetValue(stageName, out var timer))
            {
                timer.Stop();
                RecordFrameTiming(stageName, timer.ElapsedMilliseconds + timer.ElapsedTicks / 10000.0);
            }
        }

        public void RecordFrameMetrics(FrameTimingData frameData)
        {
            _frameData.Add(frameData);
            
            var currentMemory = GC.GetTotalMemory(false);
            _peakMemory = Math.Max(_peakMemory, currentMemory);
            _totalMemory += currentMemory;
            _memoryMeasurements++;
            
            _totalCommandsProcessed += frameData.CommandsProcessed;
            _totalTrianglesRendered += frameData.TrianglesRendered;
        }

        public void RecordCacheHit()
        {
            _meshCacheHits++;
        }

        public void RecordCacheMiss()
        {
            _meshCacheMisses++;
        }

        public PerformanceMetrics GenerateReport(string testName, int nodeCount, int edgeCount)
        {
            var frameTimes = _frameData.Select(f => f.FrameTimeMs).ToList();
            var fpsList = _frameData.Select(f => f.FPS).ToList();
            
            var metrics = new PerformanceMetrics
            {
                TestName = testName,
                NodeCount = nodeCount,
                EdgeCount = edgeCount,
                FrameCount = _frameData.Count,
                
                AverageFrameTime = frameTimes.Count > 0 ? frameTimes.Average() : 0,
                MinFrameTime = frameTimes.Count > 0 ? frameTimes.Min() : 0,
                MaxFrameTime = frameTimes.Count > 0 ? frameTimes.Max() : 0,
                StandardDeviation = CalculateStdDev(frameTimes),
                AverageFPS = fpsList.Count > 0 ? fpsList.Average() : 0,
                MinFPS = fpsList.Count > 0 ? fpsList.Min() : 0,
                MaxFPS = fpsList.Count > 0 ? fpsList.Max() : 0,
                
                PeakMemoryUsage = _peakMemory,
                AverageMemoryUsage = _memoryMeasurements > 0 ? _totalMemory / _memoryMeasurements : 0,
                MemoryGrowthRate = frameTimes.Count > 1 ? (_peakMemory - _initialMemory) / (double)frameTimes.Count : 0,
                
                MeshCacheHits = _meshCacheHits,
                MeshCacheMisses = _meshCacheMisses,
                CacheHitRate = (_meshCacheHits + _meshCacheMisses) > 0 ? _meshCacheHits / (double)(_meshCacheHits + _meshCacheMisses) : 0,
                
                TotalRenderTime = _overallTimer.ElapsedMilliseconds,
                GPUUtilizationEstimate = CalculateGPUUtilization(frameTimes),
                
                TestStartTime = DateTime.Now.AddSeconds(-_overallTimer.Elapsed.TotalSeconds),
                TestEndTime = DateTime.Now,
                TestDurationSeconds = _overallTimer.Elapsed.TotalSeconds
            };
            
            // Identify bottlenecks
            IdentifyBottlenecks(metrics, frameTimes);
            
            return metrics;
        }

        private double CalculateStdDev(List<double> values)
        {
            if (values.Count <= 1) return 0;
            
            var avg = values.Average();
            var sumOfSquares = values.Sum(x => Math.Pow(x - avg, 2));
            return Math.Sqrt(sumOfSquares / values.Count);
        }

        private double CalculateGPUUtilization(List<double> frameTimes)
        {
            var avgTime = frameTimes.Count > 0 ? frameTimes.Average() : 0;
            return Math.Min(100.0, (avgTime / 16.67) * 100.0);  // Relative to 60 FPS target
        }

        private void IdentifyBottlenecks(PerformanceMetrics metrics, List<double> frameTimes)
        {
            // Frame time analysis
            if (metrics.AverageFrameTime > 16.67)
                metrics.BottleneckIdentified.Add(
                    $"Frame time exceeds 60 FPS target ({metrics.AverageFrameTime:F2}ms > 16.67ms)");
            
            if (metrics.StandardDeviation > 5.0)
                metrics.BottleneckIdentified.Add(
                    $"High frame time variance (StdDev: {metrics.StandardDeviation:F2}ms) - inconsistent performance");
            
            // Memory analysis
            if (metrics.MemoryGrowthRate > 1000)
                metrics.BottleneckIdentified.Add(
                    $"High memory growth rate ({metrics.MemoryGrowthRate:F0} bytes/frame) - potential memory leak");
            
            // Cache analysis
            if (metrics.CacheHitRate < 0.9)
                metrics.Warnings.Add(
                    $"Low mesh cache hit rate ({metrics.CacheHitRate * 100:F1}%) - improve caching strategy");
            
            // GPU utilization
            if (metrics.GPUUtilizationEstimate > 90)
                metrics.BottleneckIdentified.Add(
                    $"GPU utilization high ({metrics.GPUUtilizationEstimate:F1}%) - GPU-bound performance");
            
            // Stage timing analysis
            foreach (var stage in metrics.BottleneckIdentified)
            {
                if (stage.Contains("GPU"))
                    metrics.BottleneckIdentified.Add("Recommendation: Optimize shaders or reduce draw calls");
                else if (stage.Contains("memory"))
                    metrics.BottleneckIdentified.Add("Recommendation: Review allocation patterns, add pooling");
            }
        }

        public Dictionary<string, double> GetStageSummary()
        {
            var summary = new Dictionary<string, double>();
            
            foreach (var kvp in _timingMap)
            {
                if (kvp.Value.Count > 0)
                {
                    summary[kvp.Key] = kvp.Value.Average();
                }
            }
            
            return summary;
        }

        public void PrintStageSummary()
        {
            Console.WriteLine("\n╔════════════════════════════════════════════╗");
            Console.WriteLine("║ STAGE TIMING BREAKDOWN                     ║");
            Console.WriteLine("╚════════════════════════════════════════════╝\n");
            
            foreach (var kvp in GetStageSummary().OrderByDescending(x => x.Value))
            {
                var percentage = (kvp.Value / 16.67) * 100;
                var barLength = (int)(percentage / 5);
                var bar = new string('█', Math.Min(barLength, 20));
                
                Console.WriteLine($"{kvp.Key,-30} {kvp.Value:F3}ms {percentage:F1}% [{bar,-20}]");
            }
        }

        public void Dispose()
        {
            _timingMap.Clear();
            _frameData.Clear();
        }
    }

    /// <summary>
    /// Load test generator for creating graphs of various sizes
    /// </summary>
    public class LoadTestGenerator
    {
        /// <summary>
        /// Generate a test graph with specified node/edge count
        /// </summary>
        public static Graph GenerateTestGraph(int nodeCount, string layoutType = "random")
        {
            var graph = new Graph { Nodes = new List<GraphNode>(), Edges = new List<GraphEdge>() };
            var random = new Random(42);  // Fixed seed for reproducibility
            
            // Create nodes
            for (int i = 0; i < nodeCount; i++)
            {
                var color = i < nodeCount * 0.1 ? "#0000FF" : "#00FF00";  // 10% blue hubs
                var posVec = layoutType == "random"
                    ? new System.Numerics.Vector3(
                        (float)(random.NextDouble() - 0.5) * 10,
                        (float)(random.NextDouble() - 0.5) * 10,
                        (float)(random.NextDouble() - 0.5) * 10)
                    : GenerateGridPosition(i, nodeCount);
                
                var position = new Models.Vector3D { X = posVec.X, Y = posVec.Y, Z = posVec.Z };
                
                graph.Nodes.Add(new GraphNode
                {
                    Id = $"node_{i}",
                    Label = $"Node {i}",
                    Position = position,
                    Color = color
                });
            }
            
            // Create edges (hub-and-spoke pattern scaled)
            var hubCount = Math.Max(1, (int)(nodeCount * 0.1));
            for (int hub = 0; hub < hubCount; hub++)
            {
                for (int spoke = 0; spoke < Math.Min(5, nodeCount - 1); spoke++)
                {
                    var targetNode = (hub * 5 + spoke + hubCount) % nodeCount;
                    if (targetNode != hub)
                    {
                        graph.Edges.Add(new GraphEdge
                        {
                            SourceNodeId = $"node_{hub}",
                            TargetNodeId = $"node_{targetNode}"
                        });
                    }
                }
            }
            
            return graph;
        }

        private static System.Numerics.Vector3 GenerateGridPosition(int index, int total)
        {
            var gridSize = (int)Math.Ceiling(Math.Cbrt(total));
            var x = (index % gridSize) - gridSize / 2;
            var y = ((index / gridSize) % gridSize) - gridSize / 2;
            var z = (index / (gridSize * gridSize)) - gridSize / 2;
            return new System.Numerics.Vector3(x, y, z);
        }

        /// <summary>
        /// Generate a series of test cases with increasing graph sizes
        /// </summary>
        public static List<(int Nodes, int Edges, string Name)> GenerateLoadTestSuite()
        {
            return new()
            {
                (10, 18, "Tiny"),
                (50, 100, "Small"),
                (100, 250, "Medium"),
                (500, 1000, "Large"),
                (1000, 2000, "XL"),
                (2000, 4000, "XXL")
            };
        }
    }

    /// <summary>
    /// Memory profiler for tracking allocations
    /// </summary>
    public class MemoryProfiler
    {
        private List<long> _memorySnapshots = new();
        private Stopwatch _profilerTimer = new();

        public void StartProfiling()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            _profilerTimer.Restart();
        }

        public void TakeSnapshot()
        {
            _memorySnapshots.Add(GC.GetTotalMemory(false));
        }

        public MemoryProfilerReport GetReport()
        {
            _profilerTimer.Stop();
            
            if (_memorySnapshots.Count < 2)
                return new MemoryProfilerReport { SnapshotCount = _memorySnapshots.Count };
            
            var report = new MemoryProfilerReport
            {
                SnapshotCount = _memorySnapshots.Count,
                InitialMemory = _memorySnapshots.First(),
                PeakMemory = _memorySnapshots.Max(),
                FinalMemory = _memorySnapshots.Last(),
                TotalAllocated = _memorySnapshots.Last() - _memorySnapshots.First(),
                Duration = _profilerTimer.Elapsed
            };
            
            return report;
        }
    }

    public class MemoryProfilerReport
    {
        public int SnapshotCount { get; set; }
        public long InitialMemory { get; set; }
        public long PeakMemory { get; set; }
        public long FinalMemory { get; set; }
        public long TotalAllocated { get; set; }
        public TimeSpan Duration { get; set; }

        public override string ToString()
        {
            return $@"
Memory Profiling Report:
  Snapshots:        {SnapshotCount}
  Initial Memory:   {InitialMemory / 1024 / 1024:F2} MB
  Peak Memory:      {PeakMemory / 1024 / 1024:F2} MB
  Final Memory:     {FinalMemory / 1024 / 1024:F2} MB
  Total Allocated:  {TotalAllocated / 1024 / 1024:F2} MB
  Duration:         {Duration.TotalSeconds:F2}s
";
        }
    }
}
