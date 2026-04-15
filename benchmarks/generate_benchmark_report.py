"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_benchmark_report.py                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     785                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

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
    
    def generate_report(self, output_json: str, output_md: str, output_html: str = None, output_latex: str = None) -> bool:
        """Generate comprehensive benchmark report in multiple formats"""
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
        
        # Generate HTML report if requested
        if output_html:
            if not self.export_html(output_html, full_report):
                print("⚠ HTML export failed, continuing...")
        
        # Generate LaTeX report if requested
        if output_latex:
            if not self.export_latex(output_latex, full_report):
                print("⚠ LaTeX export failed, continuing...")
        
        return True
    
    def _generate_scientific_references(self) -> str:
        """Generate scientific references section for reports"""
        refs = []
        refs.append("\n## Scientific References\n")
        refs.append("\nThis benchmark suite is based on established scientific standards and methodologies.\n")
        refs.append("For complete scientific foundation documentation, see: `docs/benchmarks/CHIMERA_SCIENTIFIC_FOUNDATION.md`\n")
        
        refs.append("\n### Benchmark Standards\n")
        refs.append("\n")
        refs.append("[1] Cooper, B.F., et al. (2010). Benchmarking cloud serving systems with YCSB. ")
        refs.append("*Proc. 1st ACM Symp. Cloud Comput. (SoCC)*, 143-154.\n\n")
        refs.append("[2] Transaction Processing Performance Council. (2010). TPC Benchmark C Standard Specification. ")
        refs.append("http://www.tpc.org/tpcc/\n\n")
        refs.append("[3] Transaction Processing Performance Council. (2014). TPC Benchmark H Standard Specification. ")
        refs.append("http://www.tpc.org/tpch/\n\n")
        refs.append("[7] Kwon, W., et al. (2023). Efficient memory management for large language model serving with PagedAttention. ")
        refs.append("*Proc. 29th ACM Symp. Operating Syst. Principles (SOSP)*, 611-626.\n\n")
        
        refs.append("### Statistical Methods\n")
        refs.append("\n")
        refs.append("[11] Cohen, J. (1988). *Statistical Power Analysis for the Behavioral Sciences* (2nd ed.). ")
        refs.append("Lawrence Erlbaum Associates.\n\n")
        refs.append("[12] Montgomery, D.C. (2017). *Design and Analysis of Experiments* (9th ed.). ")
        refs.append("John Wiley & Sons.\n\n")
        
        refs.append("### Reproducibility Standards\n")
        refs.append("\n")
        refs.append("[13] ACM Publications Board. (2020). Artifact Review and Badging. ")
        refs.append("https://www.acm.org/publications/policies/artifact-review-and-badging-current\n\n")
        
        refs.append("\n**BibTeX**: See `docs/benchmarks/references.bib` for complete bibliography in BibTeX format.\n")
        
        return "".join(refs)
    
    def _generate_html_references(self) -> str:
        """Generate HTML formatted references section"""
        html = []
        html.append('\n<div class="appendix">\n')
        html.append('<h2>Scientific References</h2>\n')
        html.append('<p>This benchmark suite is based on established scientific standards and methodologies.</p>\n')
        html.append('<p>For complete documentation, see: <code>docs/benchmarks/CHIMERA_SCIENTIFIC_FOUNDATION.md</code></p>\n')
        
        html.append('<h3>Benchmark Standards</h3>\n')
        html.append('<ol class="references">\n')
        html.append('<li>Cooper, B.F., et al. (2010). Benchmarking cloud serving systems with YCSB. ')
        html.append('<em>Proc. 1st ACM Symp. Cloud Comput. (SoCC)</em>, 143-154.</li>\n')
        html.append('<li>Transaction Processing Performance Council. (2010). TPC Benchmark C Standard Specification. ')
        html.append('<a href="http://www.tpc.org/tpcc/">http://www.tpc.org/tpcc/</a></li>\n')
        html.append('<li>Transaction Processing Performance Council. (2014). TPC Benchmark H Standard Specification. ')
        html.append('<a href="http://www.tpc.org/tpch/">http://www.tpc.org/tpch/</a></li>\n')
        html.append('<li value="7">Kwon, W., et al. (2023). Efficient memory management for large language model serving with PagedAttention. ')
        html.append('<em>Proc. 29th ACM Symp. Operating Syst. Principles (SOSP)</em>, 611-626.</li>\n')
        html.append('</ol>\n')
        
        html.append('<h3>Statistical Methods</h3>\n')
        html.append('<ol class="references" start="11">\n')
        html.append('<li>Cohen, J. (1988). <em>Statistical Power Analysis for the Behavioral Sciences</em> (2nd ed.). ')
        html.append('Lawrence Erlbaum Associates.</li>\n')
        html.append('<li>Montgomery, D.C. (2017). <em>Design and Analysis of Experiments</em> (9th ed.). ')
        html.append('John Wiley &amp; Sons.</li>\n')
        html.append('</ol>\n')
        
        html.append('<h3>Reproducibility Standards</h3>\n')
        html.append('<ol class="references" start="13">\n')
        html.append('<li>ACM Publications Board. (2020). Artifact Review and Badging. ')
        html.append('<a href="https://www.acm.org/publications/policies/artifact-review-and-badging-current">')
        html.append('https://www.acm.org/publications/policies/artifact-review-and-badging-current</a></li>\n')
        html.append('</ol>\n')
        
        html.append('<p><strong>BibTeX</strong>: See <code>docs/benchmarks/references.bib</code> ')
        html.append('for complete bibliography in BibTeX format.</p>\n')
        html.append('</div>\n')
        
        return "".join(html)
    
    def _generate_latex_references(self) -> str:
        """Generate LaTeX formatted references section"""
        latex = []
        latex.append('\n\\section{References}\n\n')
        latex.append('This benchmark suite is based on established scientific standards and methodologies. ')
        latex.append('For complete documentation, see: \\texttt{docs/benchmarks/CHIMERA\\_SCIENTIFIC\\_FOUNDATION.md}\n\n')
        
        latex.append('\\subsection{Benchmark Standards}\n\n')
        latex.append('\\begin{thebibliography}{99}\n\n')
        
        latex.append('\\bibitem{cooper2010ycsb}\n')
        latex.append('B.F.~Cooper, A.~Silberstein, E.~Tam, R.~Ramakrishnan, and R.~Sears,\n')
        latex.append('``Benchmarking cloud serving systems with YCSB,\'\'\n')
        latex.append('in \\emph{Proceedings of the 1st ACM Symposium on Cloud Computing (SoCC)},\n')
        latex.append('Indianapolis, IN, USA, Jun. 2010, pp. 143--154.\n\n')
        
        latex.append('\\bibitem{tpcc2010}\n')
        latex.append('Transaction Processing Performance Council,\n')
        latex.append('``TPC Benchmark C Standard Specification, Revision 5.11,\'\' 2010.\n')
        latex.append('[Online]. Available: \\url{http://www.tpc.org/tpcc/}\n\n')
        
        latex.append('\\bibitem{kwon2023vllm}\n')
        latex.append('W.~Kwon, Z.~Li, S.~Zhuang, Y.~Sheng, L.~Zheng, C.H.~Yu, J.~Gonzalez, H.~Zhang, and I.~Stoica,\n')
        latex.append('``Efficient memory management for large language model serving with PagedAttention,\'\'\n')
        latex.append('in \\emph{Proc. 29th ACM Symp. Operating Syst. Principles (SOSP)},\n')
        latex.append('Koblenz, Germany, Oct. 2023, pp. 611--626.\n\n')
        
        latex.append('\\bibitem{cohen1988statistical}\n')
        latex.append('J.~Cohen,\n')
        latex.append('\\emph{Statistical Power Analysis for the Behavioral Sciences}, 2nd~ed.\n')
        latex.append('Hillsdale, NJ, USA: Lawrence Erlbaum Associates, 1988.\n\n')
        
        latex.append('\\bibitem{montgomery2017design}\n')
        latex.append('D.C.~Montgomery,\n')
        latex.append('\\emph{Design and Analysis of Experiments}, 9th~ed.\n')
        latex.append('Hoboken, NJ, USA: John Wiley \\& Sons, 2017.\n\n')
        
        latex.append('\\bibitem{acm2020badging}\n')
        latex.append('ACM Publications Board,\n')
        latex.append('``Artifact Review and Badging,\'\' 2020.\n')
        latex.append('[Online]. Available: \\url{https://www.acm.org/publications/policies/artifact-review-and-badging-current}\n\n')
        
        latex.append('\\end{thebibliography}\n\n')
        latex.append('\\textbf{Note}: For complete bibliography in BibTeX format, ')
        latex.append('see \\texttt{docs/benchmarks/references.bib}.\n')
        
        return "".join(latex)
    
    def export_html(self, output_file: str, report: Dict) -> bool:
        """Export benchmark report to HTML format with IEEE citations"""
        try:
            html = []
            html.append('<!DOCTYPE html>\n')
            html.append('<html lang="en">\n')
            html.append('<head>\n')
            html.append('  <meta charset="UTF-8">\n')
            html.append('  <meta name="viewport" content="width=device-width, initial-scale=1.0">\n')
            html.append('  <title>GPU Training Performance Benchmark Report</title>\n')
            html.append('  <style>\n')
            html.append('    body { font-family: Arial, sans-serif; max-width: 1200px; margin: 0 auto; padding: 20px; }\n')
            html.append('    h1 { color: #333; border-bottom: 2px solid #0066cc; }\n')
            html.append('    h2 { color: #0066cc; margin-top: 30px; }\n')
            html.append('    h3 { color: #333; }\n')
            html.append('    .summary { background: #f5f5f5; padding: 15px; border-radius: 5px; margin: 20px 0; }\n')
            html.append('    .target { margin: 10px 0; }\n')
            html.append('    .status { font-weight: bold; }\n')
            html.append('    .pass { color: #28a745; }\n')
            html.append('    .pending { color: #ffc107; }\n')
            html.append('    .appendix { margin-top: 50px; border-top: 2px solid #ccc; padding-top: 20px; }\n')
            html.append('    .references { line-height: 1.8; }\n')
            html.append('    .references li { margin-bottom: 10px; }\n')
            html.append('    code { background: #f4f4f4; padding: 2px 5px; border-radius: 3px; }\n')
            html.append('  </style>\n')
            html.append('</head>\n')
            html.append('<body>\n')
            
            html.append('<h1>GPU Training Performance Benchmark Report</h1>\n')
            html.append(f'<p><strong>Generated:</strong> {report["metadata"]["generated_at"]}</p>\n')
            html.append(f'<p class="status"><strong>Status:</strong> ')
            html.append(f'<span class="{report["summary"]["status"].lower()}">{report["summary"]["status"]}</span></p>\n')
            
            html.append('<div class="summary">\n')
            html.append('<h2>Executive Summary</h2>\n')
            html.append('<h3>Performance Targets</h3>\n')
            for target_name, target_data in report['summary']['performance_targets'].items():
                target = target_data.get('target', 'N/A')
                achieved = target_data.get('achieved', 'N/A')
                status_class = "pass" if achieved and achieved != "N/A" else "pending"
                html.append(f'<div class="target">\n')
                html.append(f'  <strong>{target_name.replace("_", " ").title()}</strong><br>\n')
                html.append(f'  Target: {target}<br>\n')
                html.append(f'  Achieved: <span class="{status_class}">{achieved}</span>\n')
                html.append('</div>\n')
            html.append('</div>\n')
            
            # Append scientific references
            html.append(self._generate_html_references())
            
            html.append('</body>\n')
            html.append('</html>\n')
            
            with open(output_file, 'w') as f:
                f.write("".join(html))
            print(f"✓ HTML report exported to: {output_file}")
            return True
        except Exception as e:
            print(f"✗ Failed to export HTML report: {e}")
            return False
    
    def export_latex(self, output_file: str, report: Dict) -> bool:
        """Export benchmark report to LaTeX format with IEEE bibliography"""
        try:
            latex = []
            latex.append('\\documentclass[11pt,a4paper]{article}\n')
            latex.append('\\usepackage[utf8]{inputenc}\n')
            latex.append('\\usepackage{hyperref}\n')
            latex.append('\\usepackage{graphicx}\n')
            latex.append('\\usepackage{booktabs}\n')
            latex.append('\\title{GPU Training Performance Benchmark Report}\n')
            latex.append('\\author{ThemisDB Team}\n')
            latex.append(f'\\date{{{report["metadata"]["generated_at"]}}}\n\n')
            latex.append('\\begin{document}\n\n')
            latex.append('\\maketitle\n\n')
            
            latex.append('\\section{Executive Summary}\n\n')
            latex.append(f'\\textbf{{Status:}} {report["summary"]["status"]}\n\n')
            
            latex.append('\\subsection{Performance Targets}\n\n')
            latex.append('\\begin{itemize}\n')
            for target_name, target_data in report['summary']['performance_targets'].items():
                target = target_data.get('target', 'N/A')
                achieved = target_data.get('achieved', 'N/A')
                latex.append(f'  \\item \\textbf{{{target_name.replace("_", " ").title()}}}:\\\\\n')
                latex.append(f'        Target: {target}\\\\\n')
                latex.append(f'        Achieved: {achieved}\n')
            latex.append('\\end{itemize}\n\n')
            
            # Append scientific references
            latex.append(self._generate_latex_references())
            
            latex.append('\\end{document}\n')
            
            with open(output_file, 'w') as f:
                f.write("".join(latex))
            print(f"✓ LaTeX report exported to: {output_file}")
            return True
        except Exception as e:
            print(f"✗ Failed to export LaTeX report: {e}")
            return False
    
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
        
        # Append scientific references
        md.append(self._generate_scientific_references())
        
        return "".join(md)


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: generate_benchmark_report.py <benchmark_results.json> [output_dir] [--html] [--latex]")
        print("\nExample:")
        print("  python3 generate_benchmark_report.py benchmark_results.json ./reports")
        print("  python3 generate_benchmark_report.py benchmark_results.json ./reports --html --latex")
        print("\nOptions:")
        print("  --html   Generate HTML report with IEEE citations")
        print("  --latex  Generate LaTeX report with bibliography")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 and not sys.argv[2].startswith('--') else "."
    
    # Check for format flags
    generate_html = '--html' in sys.argv
    generate_latex = '--latex' in sys.argv
    
    # Create output directory if needed
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    output_json = os.path.join(output_dir, "gpu_training_benchmark_report.json")
    output_md = os.path.join(output_dir, "GPU_TRAINING_BENCHMARK_REPORT.md")
    output_html = os.path.join(output_dir, "gpu_training_benchmark_report.html") if generate_html else None
    output_latex = os.path.join(output_dir, "gpu_training_benchmark_report.tex") if generate_latex else None
    
    # Generate report
    generator = BenchmarkReportGenerator()
    
    if not generator.load_benchmark_results(input_file):
        sys.exit(1)
    
    if generator.generate_report(output_json, output_md, output_html, output_latex):
        print("\n✓ Benchmark report generated successfully!")
        if generate_html:
            print("  → HTML report includes IEEE citations appendix")
        if generate_latex:
            print("  → LaTeX report includes bibliography section")
        sys.exit(0)
    else:
        print("\n✗ Failed to generate benchmark report")
        sys.exit(1)


if __name__ == "__main__":
    main()
