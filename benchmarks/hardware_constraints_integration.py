"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hardware_constraints_integration.py                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     421                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Hardware Constraints Integration & ThemisDB Performance Validation
====================================================================

Integriert die Hardware-Constraint-Analyse mit den bestehenden Benchmarks
und validiert ThemisDB gegen RocksDB/TBB-Erwartungen.

Features:
- Hardware-profiling (automatisch vom System)
- Performance-Vergleich (ThemisDB vs RocksDB baseline)
- Scaling-Analyse (TBB-style Effizienz)
- Bottleneck-Identifikation
- Optimierungs-Empfehlungen
- JSON/CSV Export

Author: ThemisDB Team
Date: 2025-12-04
"""

import asyncio
import json
import csv
import os
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict, field
from datetime import datetime
from pathlib import Path

try:
    import psutil
except ImportError:
    psutil = None

from hardware_constraints_analyzer import (
    HardwareLimits,
    RocksDBPerformanceModel,
    TBBPerformanceModel,
    HardwareConstraintAnalysis,
    ThemisCompliance,
)


# ============================================================================
# HARDWARE PROFILING
# ============================================================================

class HardwareProfiler:
    """Detect and profile system hardware"""
    
    @staticmethod
    def detect_system() -> HardwareLimits:
        """Detect system hardware and return HardwareLimits"""
        
        if psutil is None:
            # Fallback to typical system
            print("⚠️  psutil not available - using typical system profile")
            return HardwareLimits.typical_system()
        
        # Detect CPU
        cpu_count = psutil.cpu_count(logical=False)  # Physical cores
        if cpu_count is None:
            cpu_count = 8
        
        # Approximate CPU frequency (GHz)
        cpu_freq = psutil.cpu_freq()
        cpu_freq_ghz = cpu_freq.max / 1000 if cpu_freq else 3.4
        
        # Detect Memory
        memory_total_gb = psutil.virtual_memory().total / (1024**3)
        
        # Calculate memory bandwidth (estimate)
        # Typical: 50GB/sec per core for DDR4
        memory_bandwidth = min(50 * cpu_count / 4, 100)  # Cap at 100GB/sec
        
        # Storage IOPS (estimate)
        # SSD: ~100k-500k IOPS, HDD: ~100-1k IOPS
        storage_iops = 100000  # Default to SSD
        
        return HardwareLimits(
            cpu_cores=cpu_count,
            cpu_freq_ghz=cpu_freq_ghz,
            memory_total_gb=memory_total_gb,
            memory_bandwidth_gb_sec=memory_bandwidth,
            storage_iops_random=storage_iops,
        )


# ============================================================================
# INTEGRATED COMPLIANCE ANALYZER
# ============================================================================

@dataclass
class ComplianceResult:
    """Result of compliance analysis"""
    
    database: str
    hardware: Dict[str, Any]
    timestamp: str
    
    # Performance Metrics
    read_performance: Dict[str, Any]
    write_performance: Dict[str, Any]
    scan_performance: Dict[str, Any]
    
    # Scaling Analysis
    scaling_efficiency: Dict[int, Dict[str, Any]]
    
    # Hardware Constraint Analysis
    constraint_analysis: Dict[str, Any]
    
    # Compliance Summary
    overall_compliance_pct: float
    primary_bottleneck: str
    recommendations: List[str]
    
    # Hardware Context
    hardware_limits: str  # Description


class HardwareConstraintsIntegration:
    """Integrate hardware constraints with existing benchmarks"""
    
    def __init__(self, database_name: str = "ThemisDB"):
        self.database_name = database_name
        self.hardware = HardwareProfiler.detect_system()
        self.validator = ThemisCompliance(database_name)
        self.rocksdb_model = RocksDBPerformanceModel()
        self.tbb_model = TBBPerformanceModel()
    
    async def run_compliance_analysis(self,
                                     themis_metrics: Dict[str, float],
                                     scaling_metrics: Optional[Dict[int, float]] = None) -> ComplianceResult:
        """
        Run complete compliance analysis
        
        Args:
            themis_metrics: {read_ops_sec, write_ops_sec, scan_mb_sec, latencies...}
            scaling_metrics: {thread_count: throughput, ...}
        """
        
        print(f"\n{'='*80}")
        print(f"THEMIS COMPLIANCE ANALYSIS")
        print(f"Database: {self.database_name}")
        print(f"Timestamp: {datetime.now().isoformat()}")
        print(f"{'='*80}\n")
        
        # Phase 1: Hardware Detection
        print("📊 Phase 1: Hardware Detection")
        print(f"  CPU Cores:        {self.hardware.cpu_cores}")
        print(f"  CPU Frequency:    {self.hardware.cpu_freq_ghz:.1f} GHz")
        print(f"  Memory:           {self.hardware.memory_total_gb:.1f} GB")
        print(f"  Max Throughput:   {self.hardware.max_throughput_ops_sec():,.0f} ops/sec")
        print()
        
        # Phase 2: RocksDB Comparison (8 threads)
        print("📊 Phase 2: RocksDB Comparison (8 threads)")
        rocksdb_report = await self.validator.validate_against_rocksdb(
            themis_metrics, thread_count=8
        )
        
        # Phase 3: Scaling Analysis (if provided)
        scaling_report = None
        if scaling_metrics:
            print("\n📊 Phase 3: Scaling Efficiency Analysis")
            scaling_report = await self.validator.validate_scaling_efficiency(
                themis_metrics.get("read_ops_sec", 0),
                scaling_metrics
            )
        
        # Phase 4: Hardware Constraint Impact
        print("\n📊 Phase 4: Hardware Constraint Impact Analysis")
        constraint_report = await self.validator.hardware_constraint_impact(
            self.hardware, themis_metrics
        )
        
        # Phase 5: Compile Results
        print("\n📊 Phase 5: Compiling Results...")
        
        # Calculate overall compliance
        compliance_scores = []
        for comparison in rocksdb_report["comparisons"].values():
            if isinstance(comparison, dict) and "ratio" in comparison:
                ratio = comparison.get("ratio", 0)
                compliance_scores.append(min(ratio * 100, 100))
        
        overall_compliance = sum(compliance_scores) / len(compliance_scores) if compliance_scores else 0
        
        # Compile recommendations
        recommendations = []
        
        for comparison in rocksdb_report["comparisons"].values():
            if isinstance(comparison, dict):
                if not comparison.get("meets_expectation"):
                    metric = comparison.get("metric", "Unknown")
                    ratio = comparison.get("ratio", 0)
                    
                    if ratio < 0.5:
                        recommendations.append(f"🔴 CRITICAL: {metric} only {ratio:.1%} of RocksDB")
                    elif ratio < 0.8:
                        recommendations.append(f"🟠 WARNING: {metric} only {ratio:.1%} of RocksDB")
        
        if constraint_report.get("primary_bottleneck"):
            bottleneck = constraint_report["primary_bottleneck"]
            severity = constraint_report.get("bottleneck_severity_pct", 0)
            recommendations.append(f"💡 Primary Bottleneck: {bottleneck} ({severity:.0f}% loss)")
        
        # Build result
        result = ComplianceResult(
            database=self.database_name,
            hardware=asdict(self.hardware),
            timestamp=datetime.now().isoformat(),
            read_performance=rocksdb_report["comparisons"].get("read", {}),
            write_performance=rocksdb_report["comparisons"].get("write", {}),
            scan_performance=rocksdb_report["comparisons"].get("scan", {}),
            scaling_efficiency=scaling_report.get("scaling_results", {}) if scaling_report else {},
            constraint_analysis=constraint_report,
            overall_compliance_pct=overall_compliance,
            primary_bottleneck=constraint_report.get("primary_bottleneck", "Unknown"),
            recommendations=recommendations,
            hardware_limits=self._describe_hardware(),
        )
        
        return result
    
    def _describe_hardware(self) -> str:
        """Describe hardware characteristics"""
        
        hardware_class = "High-Performance"
        if self.hardware.cpu_cores <= 4:
            hardware_class = "Entry-Level"
        elif self.hardware.cpu_cores <= 8:
            hardware_class = "Mid-Range"
        elif self.hardware.cpu_cores <= 16:
            hardware_class = "High-Performance"
        else:
            hardware_class = "Enterprise"
        
        return (f"{hardware_class} System: "
                f"{self.hardware.cpu_cores} cores @ {self.hardware.cpu_freq_ghz:.1f} GHz, "
                f"{self.hardware.memory_total_gb:.0f}GB RAM")
    
    async def export_results(self, result: ComplianceResult, 
                            output_dir: str = "benchmark_results") -> Dict[str, str]:
        """Export compliance results to files"""
        
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # JSON export
        json_file = f"{output_dir}/compliance_{self.database_name.lower()}_{timestamp}.json"
        with open(json_file, "w") as f:
            json.dump(asdict(result), f, indent=2, default=str)
        
        # CSV export
        csv_file = f"{output_dir}/compliance_{self.database_name.lower()}_{timestamp}.csv"
        with open(csv_file, "w", newline="") as f:
            writer = csv.writer(f)
            
            # Header
            writer.writerow(["Metric", "Value"])
            
            # Hardware
            writer.writerow(["HARDWARE CONFIGURATION", ""])
            writer.writerow(["CPU Cores", result.hardware["cpu_cores"]])
            writer.writerow(["CPU Frequency (GHz)", result.hardware["cpu_freq_ghz"]])
            writer.writerow(["Memory (GB)", result.hardware["memory_total_gb"]])
            
            # Performance
            writer.writerow(["", ""])
            writer.writerow(["PERFORMANCE METRICS", ""])
            
            if result.read_performance:
                writer.writerow(["Read Throughput (ThemisDB)", result.read_performance.get("themis", "N/A")])
                writer.writerow(["Read Throughput (RocksDB Baseline)", result.read_performance.get("rocksdb_baseline", "N/A")])
                writer.writerow(["Read Efficiency", f"{result.read_performance.get('ratio', 0):.2%}"])
            
            if result.write_performance:
                writer.writerow(["Write Throughput (ThemisDB)", result.write_performance.get("themis", "N/A")])
                writer.writerow(["Write Throughput (RocksDB Baseline)", result.write_performance.get("rocksdb_baseline", "N/A")])
                writer.writerow(["Write Efficiency", f"{result.write_performance.get('ratio', 0):.2%}"])
            
            if result.scan_performance:
                writer.writerow(["Scan Throughput (ThemisDB)", result.scan_performance.get("themis", "N/A")])
                writer.writerow(["Scan Throughput (RocksDB Baseline)", result.scan_performance.get("rocksdb_baseline", "N/A")])
                writer.writerow(["Scan Efficiency", f"{result.scan_performance.get('ratio', 0):.2%}"])
            
            # Compliance
            writer.writerow(["", ""])
            writer.writerow(["COMPLIANCE SUMMARY", ""])
            writer.writerow(["Overall Compliance %", f"{result.overall_compliance_pct:.1f}%"])
            writer.writerow(["Primary Bottleneck", result.primary_bottleneck])
            writer.writerow(["Hardware Class", result.hardware_limits])
            
            # Recommendations
            if result.recommendations:
                writer.writerow(["", ""])
                writer.writerow(["RECOMMENDATIONS", ""])
                for rec in result.recommendations:
                    writer.writerow([rec])
        
        # Text report
        txt_file = f"{output_dir}/compliance_{self.database_name.lower()}_{timestamp}.txt"
        with open(txt_file, "w") as f:
            f.write("="*80 + "\n")
            f.write("THEMIS COMPLIANCE ANALYSIS REPORT\n")
            f.write("="*80 + "\n\n")
            
            f.write(f"Database: {result.database}\n")
            f.write(f"Timestamp: {result.timestamp}\n")
            f.write(f"Hardware: {result.hardware_limits}\n\n")
            
            f.write("PERFORMANCE COMPARISON (vs RocksDB Baseline)\n")
            f.write("-"*80 + "\n")
            
            for metric_name, metric in [("Read", result.read_performance),
                                       ("Write", result.write_performance),
                                       ("Scan", result.scan_performance)]:
                if metric:
                    f.write(f"\n{metric_name}:\n")
                    f.write(f"  ThemisDB:     {metric.get('themis', 'N/A')}\n")
                    f.write(f"  RocksDB:      {metric.get('rocksdb_baseline', 'N/A')}\n")
                    f.write(f"  Efficiency:   {metric.get('ratio', 0):.2%}\n")
                    f.write(f"  Grade:        {metric.get('grade', 'N/A')}\n")
            
            f.write(f"\nOVERALL COMPLIANCE: {result.overall_compliance_pct:.1f}%\n")
            f.write(f"Primary Bottleneck: {result.primary_bottleneck}\n\n")
            
            if result.recommendations:
                f.write("RECOMMENDATIONS:\n")
                for rec in result.recommendations:
                    f.write(f"  {rec}\n")
        
        return {
            "json": json_file,
            "csv": csv_file,
            "txt": txt_file,
        }


# ============================================================================
# EXAMPLE USAGE
# ============================================================================

async def example_integrated_analysis():
    """Example integrated compliance analysis"""
    
    # Create integrator
    integrator = HardwareConstraintsIntegration("ThemisDB")
    
    # Example ThemisDB metrics (would come from actual benchmarks)
    themis_metrics = {
        "read_ops_sec": 1200000,
        "write_ops_sec": 450000,
        "scan_mb_sec": 1800,
        "read_latency_p50_us": 6.0,
        "read_latency_p99_us": 55.0,
        "read_latency_p999_us": 550.0,
        "write_latency_p50_us": 11.0,
        "write_latency_p99_us": 110.0,
        "write_latency_p999_us": 1100.0,
    }
    
    # Scaling metrics
    scaling_metrics = {
        8: 7200000,
        16: 12800000,
        32: 21600000,
    }
    
    # Run analysis
    result = await integrator.run_compliance_analysis(themis_metrics, scaling_metrics)
    
    # Export results
    files = await integrator.export_results(result)
    
    print(f"\n✅ Analysis Complete!")
    print(f"\nExported files:")
    for file_type, file_path in files.items():
        print(f"  {file_type.upper()}: {file_path}")
    
    # Print summary
    print(f"\n{'='*80}")
    print(f"COMPLIANCE SUMMARY")
    print(f"{'='*80}")
    print(f"Database:            {result.database}")
    print(f"Hardware:            {result.hardware_limits}")
    print(f"Overall Compliance:  {result.overall_compliance_pct:.1f}%")
    print(f"Primary Bottleneck:  {result.primary_bottleneck}")
    
    if result.recommendations:
        print(f"\nTop Recommendations:")
        for rec in result.recommendations[:3]:
            print(f"  • {rec}")


if __name__ == "__main__":
    asyncio.run(example_integrated_analysis())
