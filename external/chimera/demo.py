"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            demo.py                                            ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Demo script for CHIMERA vendor-neutral reporting framework.

Generates example reports with mock data from multiple database systems.
"""

import sys
import numpy as np
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from chimera import ChimeraReporter


def generate_mock_data(mean: float, std_dev: float, n: int = 30) -> list:
    """
    Generate mock benchmark data with realistic characteristics.
    
    Args:
        mean: Mean value
        std_dev: Standard deviation
        n: Number of samples
        
    Returns:
        List of sample values
    """
    # Generate data with normal distribution
    data = np.random.normal(mean, std_dev, n)
    
    # Add a few outliers (5%)
    n_outliers = max(1, int(n * 0.05))
    outlier_indices = np.random.choice(n, n_outliers, replace=False)
    for idx in outlier_indices:
        # Outliers are 3-5 std devs away
        data[idx] = mean + np.random.choice([-1, 1]) * std_dev * np.random.uniform(3, 5)
    
    # Ensure all values are positive
    data = np.abs(data)
    
    return data.tolist()


def demo_basic_report():
    """Generate a basic benchmark comparison report"""
    print("=" * 60)
    print("CHIMERA Demo: Basic Benchmark Comparison")
    print("=" * 60)
    
    reporter = ChimeraReporter(significance_level=0.05)
    
    # Add results from different systems
    # Scenario: Query throughput benchmark (queries per second)
    
    print("\n📊 Generating mock data for 3 database systems...")
    
    # System A: High performance, low variance
    system_a_data = generate_mock_data(mean=15000, std_dev=800, n=30)
    reporter.add_system_results(
        system_name="System Alpha",
        metric_name="Query Throughput",
        metric_unit="queries/sec",
        data=system_a_data,
        metadata={'version': '3.2.1', 'hardware': 'Standard VM'}
    )
    print("  ✓ System Alpha: mean ≈ 15,000 q/s")
    
    # System B: Medium performance, medium variance
    system_b_data = generate_mock_data(mean=12500, std_dev=1200, n=30)
    reporter.add_system_results(
        system_name="System Beta",
        metric_name="Query Throughput",
        metric_unit="queries/sec",
        data=system_b_data,
        metadata={'version': '2.8.0', 'hardware': 'Standard VM'}
    )
    print("  ✓ System Beta: mean ≈ 12,500 q/s")
    
    # System C: Lower performance, higher variance
    system_c_data = generate_mock_data(mean=10000, std_dev=1500, n=30)
    reporter.add_system_results(
        system_name="System Gamma",
        metric_name="Query Throughput",
        metric_unit="queries/sec",
        data=system_c_data,
        metadata={'version': '1.5.4', 'hardware': 'Standard VM'}
    )
    print("  ✓ System Gamma: mean ≈ 10,000 q/s")
    
    # Generate reports
    output_dir = Path(__file__).parent / "demo_reports"
    output_dir.mkdir(exist_ok=True)
    
    print(f"\n📁 Output directory: {output_dir}")
    print("\n🔄 Generating reports...")
    
    # CSV report
    csv_path = output_dir / "benchmark_comparison.csv"
    reporter.generate_csv_report(str(csv_path), sort_by='alphabetical')
    
    # HTML report (alphabetical sort)
    html_alpha_path = output_dir / "benchmark_report_alphabetical.html"
    reporter.generate_html_report(str(html_alpha_path), sort_by='alphabetical')
    
    # HTML report (metric sort)
    html_metric_path = output_dir / "benchmark_report_by_metric.html"
    reporter.generate_html_report(str(html_metric_path), sort_by='metric')
    
    print("\n✅ Demo completed successfully!")
    print(f"\n📄 Generated files:")
    print(f"  • {csv_path}")
    print(f"  • {html_alpha_path}")
    print(f"  • {html_metric_path}")
    print(f"\n💡 Open the HTML files in a web browser to view the reports.")


def demo_advanced_report():
    """Generate an advanced report with more systems and closer performance"""
    print("\n" + "=" * 60)
    print("CHIMERA Demo: Advanced Multi-System Comparison")
    print("=" * 60)
    
    reporter = ChimeraReporter(significance_level=0.05)
    
    # Scenario: Vector search latency (lower is better)
    print("\n📊 Generating mock data for 5 vector database systems...")
    
    systems = [
        ("System Aurora", 8.5, 1.2),
        ("System Nexus", 9.2, 1.5),
        ("System Quantum", 7.8, 0.9),
        ("System Vertex", 8.9, 1.3),
        ("System Zenith", 9.5, 1.8)
    ]
    
    for system_name, mean_latency, std_dev in systems:
        data = generate_mock_data(mean=mean_latency, std_dev=std_dev, n=50)
        reporter.add_system_results(
            system_name=system_name,
            metric_name="Vector Search Latency (P95)",
            metric_unit="milliseconds",
            data=data,
            metadata={'dataset': 'SIFT-1M', 'dimensions': 128}
        )
        print(f"  ✓ {system_name}: mean ≈ {mean_latency} ms")
    
    # Generate reports
    output_dir = Path(__file__).parent / "demo_reports"
    output_dir.mkdir(exist_ok=True)
    
    print(f"\n📁 Output directory: {output_dir}")
    print("\n🔄 Generating advanced reports...")
    
    # CSV report
    csv_path = output_dir / "vector_search_comparison.csv"
    reporter.generate_csv_report(str(csv_path), sort_by='alphabetical')
    
    # HTML report
    html_path = output_dir / "vector_search_report.html"
    reporter.generate_html_report(str(html_path), sort_by='metric')
    
    print("\n✅ Advanced demo completed!")
    print(f"\n📄 Generated files:")
    print(f"  • {csv_path}")
    print(f"  • {html_path}")


def main():
    """Run all demos"""
    print("\n🔬 CHIMERA Vendor-Neutral Reporting Framework - Demo Suite\n")
    
    try:
        # Set random seed for reproducibility
        np.random.seed(42)
        
        # Run demos
        demo_basic_report()
        demo_advanced_report()
        
        print("\n" + "=" * 60)
        print("All demos completed successfully!")
        print("=" * 60)
        print("\n📚 Next steps:")
        print("  1. Review the generated HTML reports in demo_reports/")
        print("  2. Check the CSV files for raw statistics")
        print("  3. Verify color-blind friendliness of visualizations")
        print("  4. Review neutrality seal and methodology disclosure")
        print("  5. Examine IEEE citations in the reports")
        print("\n✨ Thank you for using CHIMERA!\n")
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
