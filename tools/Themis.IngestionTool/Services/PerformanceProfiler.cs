/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            PerformanceProfiler.cs                             ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Linq;

namespace Themis.IngestionTool.Services
{
    /// <summary>
    /// Performance Profiling - CPU, Memory, I/O metrics
    /// </summary>
    public interface IPerformanceProfiler
    {
        void StartProfiling();
        void StopProfiling();
        PerformanceProfile GetProfile();
        void ResetMetrics();
    }

    public class PerformanceMetric
    {
        public string Name { get; set; } = string.Empty;
        public long StartMemoryMB { get; set; }
        public long EndMemoryMB { get; set; }
        public double ElapsedSeconds { get; set; }
        public double CPUUsagePercent { get; set; }

        public long MemoryDeltaMB => EndMemoryMB - StartMemoryMB;
    }

    public class PerformanceProfile
    {
        public DateTime StartTime { get; set; }
        public DateTime EndTime { get; set; }
        public TimeSpan Duration => EndTime - StartTime;

        public long MemoryStartMB { get; set; }
        public long MemoryPeakMB { get; set; }
        public long MemoryEndMB { get; set; }
        public long MemoryDeltaMB => MemoryEndMB - MemoryStartMB;

        public double CPUAveragePercent { get; set; }
        public double CPUPeakPercent { get; set; }

        public int ThreadCount { get; set; }

        public List<PerformanceMetric> Metrics { get; set; } = new();

        public override string ToString()
        {
            return $@"
╔═══════════════════════════════════════════════════════════════╗
║                  Performance Profile Report                    ║
╚═══════════════════════════════════════════════════════════════╝

Duration:           {Duration.TotalSeconds:F2} seconds ({Duration.TotalMilliseconds:F0}ms)

Memory Metrics:
  Start:            {MemoryStartMB} MB
  Peak:             {MemoryPeakMB} MB
  End:              {MemoryEndMB} MB
  Delta:            {MemoryDeltaMB:+0;-#} MB

CPU Metrics:
  Average:          {CPUAveragePercent:F2}%
  Peak:             {CPUPeakPercent:F2}%
  Threads:          {ThreadCount}

Operation Metrics:
{(Metrics.Any() ? string.Join("\n", Metrics.Select(m => 
    $"  {m.Name,-30} {m.ElapsedSeconds,6:F2}s  Memory: {m.MemoryDeltaMB:+0;-#}MB  CPU: {m.CPUUsagePercent,6:F1}%")) 
    : "  No metrics recorded")}

╔═══════════════════════════════════════════════════════════════╗
";
        }
    }

    public class PerformanceProfiler : IPerformanceProfiler
    {
        private readonly Process _currentProcess;
        private readonly PerformanceCounter? _cpuCounter;
        private readonly PerformanceCounter? _ramCounter;

        private PerformanceProfile? _currentProfile;
        private DateTime _profilingStartTime;
        private List<double> _cpuReadings = new();

        public PerformanceProfiler()
        {
            _currentProcess = Process.GetCurrentProcess();

            try
            {
                _cpuCounter = new PerformanceCounter("Processor", "% Processor Time", "_Total", true);
                _ramCounter = new PerformanceCounter("Memory", "Available MBytes");
            }
            catch
            {
                // Performance counters may not be available
            }
        }

        public void StartProfiling()
        {
            _cpuReadings.Clear();
            _profilingStartTime = DateTime.Now;

            _currentProfile = new PerformanceProfile
            {
                StartTime = DateTime.Now,
                MemoryStartMB = _currentProcess.WorkingSet64 / (1024 * 1024),
                ThreadCount = _currentProcess.Threads.Count
            };
        }

        public void StopProfiling()
        {
            if (_currentProfile == null) return;

            _currentProfile.EndTime = DateTime.Now;
            _currentProfile.MemoryEndMB = _currentProcess.WorkingSet64 / (1024 * 1024);
            _currentProfile.MemoryPeakMB = _currentProcess.PeakWorkingSet64 / (1024 * 1024);

            if (_cpuReadings.Any())
            {
                _currentProfile.CPUAveragePercent = _cpuReadings.Average();
                _currentProfile.CPUPeakPercent = _cpuReadings.Max();
            }
        }

        public PerformanceProfile GetProfile()
        {
            if (_currentProfile == null)
            {
                StartProfiling();
            }

            // Take CPU reading
            if (_cpuCounter != null)
            {
                try
                {
                    var cpuValue = _cpuCounter.NextValue();
                    _cpuReadings.Add(cpuValue);
                }
                catch { }
            }

            // Update memory peak
            long currentMemory = 0;
            if (_currentProcess != null && _currentProcess.WorkingSet64 > 0)
            {
                currentMemory = (_currentProcess.WorkingSet64 / (1024 * 1024));
            }
            if (currentMemory > _currentProfile.MemoryPeakMB)
            {
                _currentProfile.MemoryPeakMB = currentMemory;
            }

            return _currentProfile;
        }

        public void ResetMetrics()
        {
            _cpuReadings.Clear();
            _currentProfile = null;
        }
    }

    /// <summary>
    /// CPU Time Tracker - Measure individual operation times
    /// </summary>
    public class OperationTimer : IDisposable
    {
        private readonly IPerformanceProfiler _profiler;
        private readonly string _operationName;
        private readonly Stopwatch _stopwatch;
        private readonly long _startMemory;

        public OperationTimer(IPerformanceProfiler profiler, string operationName)
        {
            _profiler = profiler;
            _operationName = operationName;
            _stopwatch = Stopwatch.StartNew();
            _startMemory = GC.GetTotalMemory(false);
        }

        public void Dispose()
        {
            _stopwatch.Stop();
            var endMemory = GC.GetTotalMemory(false);

            var profile = _profiler.GetProfile();
            profile.Metrics.Add(new PerformanceMetric
            {
                Name = _operationName,
                StartMemoryMB = _startMemory / (1024 * 1024),
                EndMemoryMB = endMemory / (1024 * 1024),
                ElapsedSeconds = _stopwatch.Elapsed.TotalSeconds,
                CPUUsagePercent = 0  // Would need more detailed tracking
            });
        }
    }
}
