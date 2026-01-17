#!/usr/bin/env python3
"""
GPU Training Benchmark Report Generator

Generates comprehensive JSON and Markdown reports from benchmark results.
Validates performance targets and computes speedup metrics.
"""

import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Any
from datetime import datetime


class BenchmarkReportGenerator:
    """Generate reports from GPU training benchmark results"""
    
    def __init__(self):
        self.results = {}
        self.summary = {
            "generated_at": datetime.now().isoformat(),
            "benchmarks": [],
            "performance_targets": {
                "gpu_vs_cpu_speedup": {"target": "2-4x", "achieved": None},
                "multi_gpu_scaling_efficiency": {"target": "80-95%", "achieved": None},
                "mixed_precision_speedup": {"target": "2x", "achieved": None},
                "mixed_precision_memory_reduction": {"target": "50%", "achieved": None},
            },
            "status": "PENDING"
        }
    
    def load_benchmark_results(self, json_file: str) -> bool:
        """Load benchmark results from JSON file"""
        try:
            with open(json_file, 'r') as f:
                self.results = json.load(f)
            print(f"✓ Loaded benchmark results from {json_file}")
            return True
        except FileNotFoundError:
            print(f"✗ Benchmark results file not found: {json_file}")
            return False
        except json.JSONDecodeError as e:
            print(f"✗ Invalid JSON in benchmark results: {e}")
            return False
    
    def analyze_training_cycle(self) -> Dict[str, Any]:
        """Analyze end-to-end training cycle benchmarks"""
        analysis = {
            "name": "End-to-End Training Cycle",
            "tests": [],
            "speedup_summary": {}
        }
        
        if "benchmarks" not in self.results:
            return analysis
        
        cpu_baseline = {}
        gpu_results = {}
        
        for bench in self.results.get("benchmarks", []):
            name = bench.get("name", "")
            
            if "TrainingCycle_CPU_Baseline" in name:
                # Extract batch size and seq len from name
                batch_size = bench.get("batch_size", "unknown")
                seq_len = bench.get("seq_len", "unknown")
                key = f"{batch_size}_{seq_len}"
                cpu_baseline[key] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0)
                }
            
            elif "TrainingCycle_CUDA" in name or "TrainingCycle_HIP" in name:
                batch_size = bench.get("batch_size", "unknown")
                seq_len = bench.get("seq_len", "unknown")
                backend = "CUDA" if "CUDA" in name else "HIP"
                key = f"{batch_size}_{seq_len}"
                
                if key not in gpu_results:
                    gpu_results[key] = {}
                
                gpu_results[key][backend] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0)
                }
        
        # Calculate speedups
        speedups = []
        for key in cpu_baseline:
            if key in gpu_results:
                cpu_time = cpu_baseline[key]["time_ms"]
                
                for backend, gpu_data in gpu_results[key].items():
                    gpu_time = gpu_data["time_ms"]
                    if gpu_time > 0:
                        speedup = cpu_time / gpu_time
                        speedups.append(speedup)
                        
                        analysis["tests"].append({
                            "config": key,
                            "backend": backend,
                            "cpu_time_ms": cpu_time,
                            "gpu_time_ms": gpu_time,
                            "speedup": f"{speedup:.2f}x"
                        })
        
        if speedups:
            avg_speedup = sum(speedups) / len(speedups)
            analysis["speedup_summary"] = {
                "average": f"{avg_speedup:.2f}x",
                "min": f"{min(speedups):.2f}x",
                "max": f"{max(speedups):.2f}x",
                "target_met": avg_speedup >= 2.0
            }
            
            # Update summary
            self.summary["performance_targets"]["gpu_vs_cpu_speedup"]["achieved"] = f"{avg_speedup:.2f}x"
        
        return analysis
    
    def analyze_multi_gpu_scaling(self) -> Dict[str, Any]:
        """Analyze multi-GPU scaling efficiency"""
        analysis = {
            "name": "Multi-GPU Scaling",
            "tests": [],
            "scaling_efficiency": {}
        }
        
        if "benchmarks" not in self.results:
            return analysis
        
        single_gpu_baseline = {}
        multi_gpu_results = {}
        
        for bench in self.results.get("benchmarks", []):
            name = bench.get("name", "")
            
            if "SingleGPU_TrainingStep" in name:
                batch_size = bench.get("batch_size", "unknown")
                single_gpu_baseline[batch_size] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0)
                }
            
            elif "TwoGPU_DataParallel" in name or "FourGPU_DataParallel" in name:
                batch_size = bench.get("batch_size", "unknown")
                num_gpus = 2 if "TwoGPU" in name else 4
                
                if batch_size not in multi_gpu_results:
                    multi_gpu_results[batch_size] = {}
                
                multi_gpu_results[batch_size][num_gpus] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0)
                }
        
        # Calculate scaling efficiency
        efficiencies = []
        for batch_size in single_gpu_baseline:
            if batch_size in multi_gpu_results:
                baseline_time = single_gpu_baseline[batch_size]["time_ms"]
                
                for num_gpus, gpu_data in multi_gpu_results[batch_size].items():
                    multi_gpu_time = gpu_data["time_ms"]
                    if multi_gpu_time > 0:
                        actual_speedup = baseline_time / multi_gpu_time
                        ideal_speedup = num_gpus
                        efficiency = (actual_speedup / ideal_speedup) * 100
                        efficiencies.append(efficiency)
                        
                        analysis["tests"].append({
                            "batch_size": batch_size,
                            "num_gpus": num_gpus,
                            "baseline_time_ms": baseline_time,
                            "multi_gpu_time_ms": multi_gpu_time,
                            "speedup": f"{actual_speedup:.2f}x",
                            "scaling_efficiency": f"{efficiency:.1f}%"
                        })
        
        if efficiencies:
            avg_efficiency = sum(efficiencies) / len(efficiencies)
            analysis["scaling_efficiency"] = {
                "average": f"{avg_efficiency:.1f}%",
                "min": f"{min(efficiencies):.1f}%",
                "max": f"{max(efficiencies):.1f}%",
                "target_met": avg_efficiency >= 80.0
            }
            
            # Update summary
            self.summary["performance_targets"]["multi_gpu_scaling_efficiency"]["achieved"] = f"{avg_efficiency:.1f}%"
        
        return analysis
    
    def analyze_mixed_precision(self) -> Dict[str, Any]:
        """Analyze mixed precision performance"""
        analysis = {
            "name": "Mixed Precision Performance",
            "tests": [],
            "speedup_summary": {},
            "memory_summary": {}
        }
        
        if "benchmarks" not in self.results:
            return analysis
        
        fp32_baseline = {}
        fp16_results = {}
        memory_fp32 = {}
        memory_fp16 = {}
        
        for bench in self.results.get("benchmarks", []):
            name = bench.get("name", "")
            
            if "Training_FP32" in name:
                batch_size = bench.get("batch_size", "unknown")
                fp32_baseline[batch_size] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0),
                    "memory_mb": bench.get("memory_MB", 0)
                }
            
            elif "Training_FP16" in name:
                batch_size = bench.get("batch_size", "unknown")
                fp16_results[batch_size] = {
                    "time_ms": bench.get("real_time", 0),
                    "samples_per_sec": bench.get("samples/sec", 0),
                    "memory_mb": bench.get("memory_MB", 0)
                }
            
            elif "Memory_FP32_vs_FP16" in name:
                precision = bench.get("precision", 32)
                memory_mb = bench.get("memory_MB", 0)
                if precision == 32:
                    memory_fp32["total"] = memory_mb
                else:
                    memory_fp16["total"] = memory_mb
        
        # Calculate speedups
        speedups = []
        for batch_size in fp32_baseline:
            if batch_size in fp16_results:
                fp32_time = fp32_baseline[batch_size]["time_ms"]
                fp16_time = fp16_results[batch_size]["time_ms"]
                
                if fp16_time > 0:
                    speedup = fp32_time / fp16_time
                    speedups.append(speedup)
                    
                    analysis["tests"].append({
                        "batch_size": batch_size,
                        "fp32_time_ms": fp32_time,
                        "fp16_time_ms": fp16_time,
                        "speedup": f"{speedup:.2f}x"
                    })
        
        if speedups:
            avg_speedup = sum(speedups) / len(speedups)
            analysis["speedup_summary"] = {
                "average": f"{avg_speedup:.2f}x",
                "min": f"{min(speedups):.2f}x",
                "max": f"{max(speedups):.2f}x",
                "target_met": avg_speedup >= 2.0
            }
            
            # Update summary
            self.summary["performance_targets"]["mixed_precision_speedup"]["achieved"] = f"{avg_speedup:.2f}x"
        
        # Calculate memory reduction
        if memory_fp32.get("total") and memory_fp16.get("total"):
            fp32_mem = memory_fp32["total"]
            fp16_mem = memory_fp16["total"]
            reduction = ((fp32_mem - fp16_mem) / fp32_mem) * 100
            
            analysis["memory_summary"] = {
                "fp32_memory_mb": fp32_mem,
                "fp16_memory_mb": fp16_mem,
                "reduction": f"{reduction:.1f}%",
                "target_met": reduction >= 50.0
            }
            
            # Update summary
            self.summary["performance_targets"]["mixed_precision_memory_reduction"]["achieved"] = f"{reduction:.1f}%"
        
        return analysis
    
    def analyze_data_transfer(self) -> Dict[str, Any]:
        """Analyze data transfer performance"""
        analysis = {
            "name": "Data Transfer & Memory Bandwidth",
            "cpu_to_gpu": [],
            "gpu_to_cpu": [],
            "prefetch_effectiveness": []
        }
        
        if "benchmarks" not in self.results:
            return analysis
        
        for bench in self.results.get("benchmarks", []):
            name = bench.get("name", "")
            
            if "CPUtoGPU_Transfer" in name:
                analysis["cpu_to_gpu"].append({
                    "size_mb": bench.get("size_MB", 0),
                    "bandwidth_gbps": bench.get("bandwidth_GB/s", 0)
                })
            
            elif "GPUtoCPU_Transfer" in name:
                analysis["gpu_to_cpu"].append({
                    "size_mb": bench.get("size_MB", 0),
                    "bandwidth_gbps": bench.get("bandwidth_GB/s", 0)
                })
            
            elif "DataLoader_WithPrefetch" in name:
                prefetch = bench.get("prefetch", 0) == 1
                analysis["prefetch_effectiveness"].append({
                    "batch_size": bench.get("batch_size", 0),
                    "prefetch_enabled": prefetch,
                    "samples_per_sec": bench.get("samples/sec", 0)
                })
        
        return analysis
    
    def analyze_backend_comparison(self) -> Dict[str, Any]:
        """Analyze backend performance comparison"""
        analysis = {
            "name": "Backend Comparison",
            "backends": [],
            "initialization_costs": []
        }
        
        if "benchmarks" not in self.results:
            return analysis
        
        for bench in self.results.get("benchmarks", []):
            name = bench.get("name", "")
            
            if "Backend_CPU" in name or "Backend_CUDA" in name or "Backend_HIP" in name or "Backend_Vulkan" in name:
                backend = "CPU"
                if "CUDA" in name:
                    backend = "CUDA"
                elif "HIP" in name:
                    backend = "HIP"
                elif "Vulkan" in name:
                    backend = "Vulkan"
                
                analysis["backends"].append({
                    "backend": backend,
                    "batch_size": bench.get("batch_size", 0),
                    "samples_per_sec": bench.get("samples/sec", 0),
                    "time_ms": bench.get("real_time", 0)
                })
            
            elif "Backend_Init" in name:
                backend = name.split("_")[-1]
                analysis["initialization_costs"].append({
                    "backend": backend,
                    "init_time_ms": bench.get("real_time", 0)
                })
        
        return analysis
    
    def generate_report(self, output_json: str, output_md: str) -> bool:
        """Generate comprehensive benchmark report"""
        print("\n=== Analyzing Benchmark Results ===\n")
        
        # Analyze all benchmark categories
        training_cycle = self.analyze_training_cycle()
        multi_gpu = self.analyze_multi_gpu_scaling()
        mixed_precision = self.analyze_mixed_precision()
        data_transfer = self.analyze_data_transfer()
        backend_comp = self.analyze_backend_comparison()
        
        # Compile full report
        full_report = {
            "metadata": {
                "generated_at": self.summary["generated_at"],
                "benchmark_suite": "GPU Training Performance",
                "version": "1.0.0"
            },
            "summary": self.summary,
            "detailed_results": {
                "training_cycle": training_cycle,
                "multi_gpu_scaling": multi_gpu,
                "mixed_precision": mixed_precision,
                "data_transfer": data_transfer,
                "backend_comparison": backend_comp
            }
        }
        
        # Determine overall status
        targets = self.summary["performance_targets"]
        all_met = all(
            target.get("achieved") and target.get("achieved") != "N/A"
            for target in targets.values()
        )
        self.summary["status"] = "PASS" if all_met else "PARTIAL"
        
        # Write JSON report
        try:
            with open(output_json, 'w') as f:
                json.dump(full_report, f, indent=2)
            print(f"✓ JSON report saved to: {output_json}")
        except Exception as e:
            print(f"✗ Failed to save JSON report: {e}")
            return False
        
        # Generate Markdown report
        md_content = self._generate_markdown(full_report)
        try:
            with open(output_md, 'w') as f:
                f.write(md_content)
            print(f"✓ Markdown report saved to: {output_md}")
        except Exception as e:
            print(f"✗ Failed to save Markdown report: {e}")
            return False
        
        return True
    
    def _generate_markdown(self, report: Dict) -> str:
        """Generate Markdown report from benchmark results"""
        md = []
        md.append("# GPU Training Performance Benchmark Report\n")
        md.append(f"**Generated:** {report['metadata']['generated_at']}\n")
        md.append(f"**Status:** {report['summary']['status']}\n")
        
        md.append("\n## Executive Summary\n")
        md.append("### Performance Targets\n")
        
        for target_name, target_data in report['summary']['performance_targets'].items():
            target = target_data.get('target', 'N/A')
            achieved = target_data.get('achieved', 'N/A')
            status = "✓" if achieved and achieved != "N/A" else "⏳"
            
            md.append(f"- **{target_name.replace('_', ' ').title()}**\n")
            md.append(f"  - Target: {target}\n")
            md.append(f"  - Achieved: {achieved} {status}\n")
        
        # Detailed results sections
        md.append("\n## Detailed Results\n")
        
        # Training cycle
        tc = report['detailed_results']['training_cycle']
        md.append(f"\n### {tc['name']}\n")
        if tc.get('speedup_summary'):
            ss = tc['speedup_summary']
            md.append(f"- Average Speedup: {ss.get('average', 'N/A')}\n")
            md.append(f"- Range: {ss.get('min', 'N/A')} - {ss.get('max', 'N/A')}\n")
            md.append(f"- Target Met: {'Yes ✓' if ss.get('target_met') else 'No ✗'}\n")
        
        # Multi-GPU
        mg = report['detailed_results']['multi_gpu_scaling']
        md.append(f"\n### {mg['name']}\n")
        if mg.get('scaling_efficiency'):
            se = mg['scaling_efficiency']
            md.append(f"- Average Efficiency: {se.get('average', 'N/A')}\n")
            md.append(f"- Range: {se.get('min', 'N/A')} - {se.get('max', 'N/A')}\n")
            md.append(f"- Target Met: {'Yes ✓' if se.get('target_met') else 'No ✗'}\n")
        
        # Mixed precision
        mp = report['detailed_results']['mixed_precision']
        md.append(f"\n### {mp['name']}\n")
        if mp.get('speedup_summary'):
            ss = mp['speedup_summary']
            md.append(f"- Average Speedup: {ss.get('average', 'N/A')}\n")
            md.append(f"- Target Met: {'Yes ✓' if ss.get('target_met') else 'No ✗'}\n")
        if mp.get('memory_summary'):
            ms = mp['memory_summary']
            md.append(f"- Memory Reduction: {ms.get('reduction', 'N/A')}\n")
            md.append(f"- Target Met: {'Yes ✓' if ms.get('target_met') else 'No ✗'}\n")
        
        md.append("\n---\n")
        md.append("*Generated by GPU Training Benchmark Suite*\n")
        
        return "".join(md)


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: generate_benchmark_report.py <benchmark_results.json> [output_dir]")
        print("\nExample:")
        print("  python3 generate_benchmark_report.py benchmark_results.json ./reports")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    
    # Create output directory if needed
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    output_json = os.path.join(output_dir, "gpu_training_benchmark_report.json")
    output_md = os.path.join(output_dir, "GPU_TRAINING_BENCHMARK_REPORT.md")
    
    # Generate report
    generator = BenchmarkReportGenerator()
    
    if not generator.load_benchmark_results(input_file):
        sys.exit(1)
    
    if generator.generate_report(output_json, output_md):
        print("\n✓ Benchmark report generated successfully!")
        sys.exit(0)
    else:
        print("\n✗ Failed to generate benchmark report")
        sys.exit(1)


if __name__ == "__main__":
    main()
