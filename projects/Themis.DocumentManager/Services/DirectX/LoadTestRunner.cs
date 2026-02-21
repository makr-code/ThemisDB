/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LoadTestRunner.cs                                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     373                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
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
    /// Automated load test runner for comprehensive performance analysis
    /// </summary>
    public class LoadTestRunner
    {
        private readonly IDirectX3DGraphRenderer _renderer;
        private int _testsPassed = 0;
        private int _testsFailed = 0;
        private List<PerformanceMetrics> _allResults = new();

        public LoadTestRunner(IDirectX3DGraphRenderer renderer)
        {
            _renderer = renderer ?? throw new ArgumentNullException(nameof(renderer));
        }

        /// <summary>
        /// Run a single performance test with specified parameters
        /// </summary>
        public async Task<PerformanceMetrics> RunTest(
            int nodeCount, 
            int frameCount = 300, 
            string testName = "")
        {
            if (string.IsNullOrEmpty(testName))
                testName = $"{nodeCount}-node-test";

            Console.WriteLine($"\n┌─────────────────────────────────────────────────┐");
            Console.WriteLine($"│ Starting Load Test: {testName,-30} │");
            Console.WriteLine($"└─────────────────────────────────────────────────┘");

            using (var profiler = new PerformanceProfiler())
            {
                profiler.StartTest();

                // Generate test graph
                var graph = LoadTestGenerator.GenerateTestGraph(nodeCount);
                var edgeCount = graph.Edges.Count;

                Console.WriteLine($"  Graph: {nodeCount} nodes, {edgeCount} edges");
                Console.WriteLine($"  Target: {frameCount} frames @ 60 FPS");

                // Initialize renderer
                _renderer.Initialize(IntPtr.Zero, 1024, 768);

                // Run frames
                for (int frame = 0; frame < frameCount; frame++)
                {
                    var frameTimer = Stopwatch.StartNew();
                    
                    // Simulate frame rendering
                    await Task.Run(() =>
                    {
                        _renderer.Render(graph);
                    });
                    
                    frameTimer.Stop();
                    var frameTimeMs = frameTimer.Elapsed.TotalMilliseconds;
                    var fps = frameTimeMs > 0 ? 1000.0 / frameTimeMs : 0;

                    var frameData = new FrameTimingData
                    {
                        FrameNumber = frame,
                        FrameTimeMs = frameTimeMs,
                        FPS = fps,
                        MemoryUsageBytes = GC.GetTotalMemory(false),
                        CommandsProcessed = Math.Min(10000, nodeCount + edgeCount),
                        TrianglesRendered = nodeCount * 144 + edgeCount * 24,  // Approx
                        DrawCalls = nodeCount + edgeCount
                    };

                    profiler.RecordFrameMetrics(frameData);

                    // Progress indicator
                    if ((frame + 1) % 50 == 0)
                        Console.WriteLine($"    Frame {frame + 1}/{frameCount}...");
                }

                profiler.EndTest();

                var metrics = profiler.GenerateReport(testName, nodeCount, edgeCount);
                _allResults.Add(metrics);

                // Display results
                Console.WriteLine(metrics.ToString());

                // Determine test status
                bool passed = metrics.AverageFrameTime <= 20.0 && metrics.StandardDeviation < 8.0;
                if (passed)
                {
                    _testsPassed++;
                    Console.WriteLine("✅ TEST PASSED");
                }
                else
                {
                    _testsFailed++;
                    Console.WriteLine("❌ TEST FAILED");
                }

                return metrics;
            }
        }

        /// <summary>
        /// Run comprehensive load test suite
        /// </summary>
        public async Task<List<PerformanceMetrics>> RunLoadTestSuite()
        {
            Console.WriteLine("\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine("║ COMPREHENSIVE LOAD TEST SUITE                               ║");
            Console.WriteLine("╚═════════════════════════════════════════════════════════════╝");

            var testCases = LoadTestGenerator.GenerateLoadTestSuite();
            var results = new List<PerformanceMetrics>();

            foreach (var (nodes, edges, name) in testCases)
            {
                try
                {
                    var result = await RunTest(nodes, 200, $"{name} ({nodes} nodes)");
                    results.Add(result);

                    // Brief pause between tests
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    await Task.Delay(500);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ Test failed with exception: {ex.Message}");
                    _testsFailed++;
                }
            }

            PrintSummaryReport(results);
            return results;
        }

        /// <summary>
        /// Run stress test with progressive load
        /// </summary>
        public async Task<List<PerformanceMetrics>> RunStressTest()
        {
            Console.WriteLine("\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine("║ STRESS TEST - PROGRESSIVE LOAD INCREASE                    ║");
            Console.WriteLine("╚═════════════════════════════════════════════════════════════╝");

            var results = new List<PerformanceMetrics>();
            int currentLoad = 100;
            int maxLoad = 5000;

            while (currentLoad <= maxLoad)
            {
                try
                {
                    Console.WriteLine($"\n⚡ Testing at {currentLoad} nodes...");
                    var result = await RunTest(currentLoad, 100, $"Stress-{currentLoad}");
                    results.Add(result);

                    // Check if we should continue
                    if (result.AverageFPS < 30)
                    {
                        Console.WriteLine($"\n⚠️ FPS dropped below 30, stopping stress test at {currentLoad} nodes");
                        break;
                    }

                    currentLoad = (int)(currentLoad * 1.5);  // 50% increase
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ Stress test failed at {currentLoad} nodes: {ex.Message}");
                    break;
                }
            }

            PrintStressTestReport(results);
            return results;
        }

        /// <summary>
        /// Run memory leak detection test
        /// </summary>
        public async Task<MemoryProfilerReport> RunMemoryLeakTest(int nodeCount = 500, int iterations = 100)
        {
            Console.WriteLine($"\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine($"║ MEMORY LEAK DETECTION TEST - {nodeCount} nodes            ║");
            Console.WriteLine($"╚═════════════════════════════════════════════════════════════╝");

            var memProfiler = new MemoryProfiler();
            memProfiler.StartProfiling();

            var graph = LoadTestGenerator.GenerateTestGraph(nodeCount);
            _renderer.Initialize(IntPtr.Zero, 1024, 768);

            for (int i = 0; i < iterations; i++)
            {
                memProfiler.TakeSnapshot();
                
                await Task.Run(() => _renderer.Render(graph));
                
                if ((i + 1) % 10 == 0)
                    Console.WriteLine($"  Iteration {i + 1}/{iterations}...");
            }

            var report = memProfiler.GetReport();
            
            Console.WriteLine(report.ToString());

            // Analyze growth
            if (report.TotalAllocated > report.PeakMemory * 0.5)
            {
                Console.WriteLine("⚠️ WARNING: Significant memory growth detected - potential memory leak!");
                _testsFailed++;
            }
            else
            {
                Console.WriteLine("✅ Memory usage stable - no memory leak detected");
                _testsPassed++;
            }

            return report;
        }

        /// <summary>
        /// Print summary report of all tests
        /// </summary>
        public void PrintSummaryReport(List<PerformanceMetrics> results)
        {
            Console.WriteLine($"\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine($"║ LOAD TEST SUMMARY REPORT                                    ║");
            Console.WriteLine($"╚═════════════════════════════════════════════════════════════╝\n");

            Console.WriteLine($"Tests Run:    {results.Count}");
            Console.WriteLine($"Passed:       {results.Count(r => r.AverageFrameTime <= 20.0 && r.StandardDeviation < 8.0)} ✅");
            Console.WriteLine($"Failed:       {results.Count(r => r.AverageFrameTime > 20.0 || r.StandardDeviation >= 8.0)} ❌");

            Console.WriteLine($"\n{"Test Name",-25} {"Nodes",-10} {"Avg FPS",-12} {"Frame Time",-15} {"Status",-10}");
            Console.WriteLine(new string('─', 72));

            foreach (var result in results)
            {
                var status = result.AverageFrameTime <= 20.0 && result.StandardDeviation < 8.0 ? "✅ PASS" : "❌ FAIL";
                var avgFps = result.AverageFPS > 0 ? $"{result.AverageFPS:F1}" : "N/A";
                var frameTime = $"{result.AverageFrameTime:F2}ms";
                
                Console.WriteLine($"{result.TestName,-25} {result.NodeCount,-10} {avgFps,-12} {frameTime,-15} {status,-10}");
            }

            // Identify scalability limits
            Console.WriteLine($"\n📊 SCALABILITY ANALYSIS:");
            var sortedByNodes = results.OrderBy(r => r.NodeCount).ToList();
            
            for (int i = 0; i < sortedByNodes.Count - 1; i++)
            {
                var current = sortedByNodes[i];
                var next = sortedByNodes[i + 1];
                
                var nodeIncrease = next.NodeCount / (double)current.NodeCount;
                var fpsDecrease = current.AverageFPS / (next.AverageFPS > 0 ? next.AverageFPS : 1.0);
                var scaleFactor = fpsDecrease / nodeIncrease;
                
                Console.WriteLine($"  {current.NodeCount} → {next.NodeCount} nodes: FPS reduction {fpsDecrease:F2}x for {nodeIncrease:F1}x nodes (scale: {scaleFactor:F3})");
            }

            // Performance recommendations
            PrintRecommendations(results);
        }

        private void PrintStressTestReport(List<PerformanceMetrics> results)
        {
            Console.WriteLine($"\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine($"║ STRESS TEST REPORT                                          ║");
            Console.WriteLine($"╚═════════════════════════════════════════════════════════════╝\n");

            if (results.Count == 0)
            {
                Console.WriteLine("No results to report");
                return;
            }

            var maxNodeCount = results.Max(r => r.NodeCount);
            var minFPS = results.Min(r => r.AverageFPS);
            var maxFrameTime = results.Max(r => r.AverageFrameTime);

            Console.WriteLine($"Max Sustainable Load:  {maxNodeCount} nodes");
            Console.WriteLine($"Worst FPS:             {minFPS:F1} FPS");
            Console.WriteLine($"Worst Frame Time:      {maxFrameTime:F2} ms");

            Console.WriteLine($"\n📈 Load vs Performance:");
            foreach (var result in results.OrderBy(r => r.NodeCount))
            {
                var loadBar = new string('█', Math.Min((int)(result.AverageFPS / 6), 10));
                Console.WriteLine($"  {result.NodeCount,5} nodes: {loadBar,-10} {result.AverageFPS:F1} FPS ({result.AverageFrameTime:F2}ms)");
            }
        }

        private void PrintRecommendations(List<PerformanceMetrics> results)
        {
            Console.WriteLine($"\n💡 PERFORMANCE RECOMMENDATIONS:\n");

            foreach (var result in results)
            {
                if (result.BottleneckIdentified.Count > 0)
                {
                    Console.WriteLine($"{result.TestName}:");
                    foreach (var bottleneck in result.BottleneckIdentified)
                        Console.WriteLine($"  • {bottleneck}");
                }
            }

            // General recommendations
            var avgGPUUtil = results.Average(r => r.GPUUtilizationEstimate);
            if (avgGPUUtil > 80)
                Console.WriteLine($"\n⚡ GPU Optimization Needed: Average utilization {avgGPUUtil:F1}%");
            
            var avgMemGrowth = results.Average(r => r.MemoryGrowthRate);
            if (avgMemGrowth > 500)
                Console.WriteLine($"\n💾 Memory Optimization Needed: Growth rate {avgMemGrowth:F0} bytes/frame");
            
            var avgCacheHit = results.Average(r => r.CacheHitRate);
            if (avgCacheHit < 0.95)
                Console.WriteLine($"\n🗂️ Cache Optimization Needed: Hit rate {avgCacheHit * 100:F1}%");
        }

        public void PrintFinalReport()
        {
            Console.WriteLine($"\n╔═════════════════════════════════════════════════════════════╗");
            Console.WriteLine($"║ FINAL TEST REPORT                                           ║");
            Console.WriteLine($"╚═════════════════════════════════════════════════════════════╝\n");

            Console.WriteLine($"Total Tests:     {_testsPassed + _testsFailed}");
            Console.WriteLine($"Passed:          {_testsPassed} ✅");
            Console.WriteLine($"Failed:          {_testsFailed} ❌");
            Console.WriteLine($"Success Rate:    {(_testsPassed * 100.0 / (_testsPassed + _testsFailed)):F1}%");

            if (_allResults.Count > 0)
            {
                Console.WriteLine($"\nPerformance Summary:");
                Console.WriteLine($"  Best FPS:      {_allResults.Max(r => r.AverageFPS):F1}");
                Console.WriteLine($"  Worst FPS:     {_allResults.Min(r => r.AverageFPS):F1}");
                Console.WriteLine($"  Avg Frame:     {_allResults.Average(r => r.AverageFrameTime):F2} ms");
                Console.WriteLine($"  Peak Memory:   {_allResults.Max(r => r.PeakMemoryUsage) / 1024 / 1024:F2} MB");
            }
        }
    }
}
