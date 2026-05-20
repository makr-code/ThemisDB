#!/usr/bin/env python3
"""
ThemisDB Gap Scanner → GitHub Issues Automation Pipeline

Single command execution:
  python tools/gap_scanner_and_issues.py

Pipeline:
  1. Run Phase 1-4 gap scanner (Security, Memory, Reliability, Concurrency, RAII, Container, Platform, Performance)
  2. Aggregate results
  3. Cluster into actionable issues
  4. Generate GitHub issue templates
  5. Create issues on GitHub (with --github flag)
  6. Generate status report

Phase 1-4 Coverage:
  - Security: unsafe functions, hardcoded secrets, SQL injection
  - Memory: new/delete, pointer arithmetic, bounds checks
  - Reliability: retry logic, timeouts, exception handling
  - Concurrency: data races, lock ordering, deadlocks
  - RAII: resource leaks, unsafe cleanup
  - Container: std:: misuse, O(n²) patterns
  - Platform: portability, ifdef gaps
  - Performance: string concat loops, allocation patterns
"""

import json
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Tuple
from datetime import datetime
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)-8s] %(message)s'
)
logger = logging.getLogger(__name__)


class GapScannerPipeline:
    """Orchestrate complete gap scanning and GitHub issue creation workflow"""
    
    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.start_time = datetime.now()
        self.results = {}
    
    def run_complete_pipeline(self, create_issues: bool = False) -> int:
        """Execute full pipeline"""
        
        print("\n" + "=" * 100)
        print("ThemisDB Gap Scanner >> GitHub Issues Automation Pipeline")
        print("=" * 100)
        
        steps = [
            ('Phase 1-4 Gap Scanner (All 8 Categories)', 
             self._step_run_scanner),
            ('Aggregate Gap Results', 
             self._step_aggregate_results),
            ('Cluster Gaps into Issues', 
             self._step_cluster_gaps),
            ('Generate Issue Templates', 
             self._step_generate_templates),
        ]
        
        if create_issues:
            steps.append(('Create GitHub Issues', self._step_create_github_issues))
        
        for step_name, step_func in steps:
            success = self._run_step(step_name, step_func)
            if not success:
                logger.error(f"[ABORT] Pipeline failed at step: {step_name}")
                return 1
        
        # Generate final report
        self._step_generate_report()
        
        print("\n" + "=" * 100)
        print("Pipeline Complete")
        print("=" * 100)
        return 0
    
    def _run_step(self, step_name: str, step_func) -> bool:
        """Run single pipeline step with error handling"""
        print(f"\n[...] {step_name}")
        print("-" * 100)
        
        try:
            step_func()
            print(f"[OK] {step_name} complete")
            return True
        except Exception as e:
            logger.error(f"Step failed: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def _step_run_scanner(self):
        """Step 1: Run Phase 1 gap scanner"""
        # Import scanners
        sys.path.insert(0, str(self.repo_root / 'tools'))
        try:
            from gap_scanner_v3 import UnifiedGapScannerV3
            
            scanner = UnifiedGapScannerV3(str(self.repo_root), str(self.output_dir))
            aggregate = scanner.run_complete_scan()
            self.results['aggregate'] = aggregate
            
        except ImportError as e:
            logger.error(f"Failed to import gap scanner: {e}")
            raise
    
    def _step_aggregate_results(self):
        """Step 2: Aggregate and validate results"""
        if 'aggregate' not in self.results:
            raise ValueError("No scanner results found")
        
        aggregate = self.results['aggregate']
        
        # Calculate metrics
        total_gaps = sum(m.get('total', 0) for m in aggregate.values())
        critical = sum(m.get('severity_critical', 0) for m in aggregate.values())
        high = sum(m.get('severity_high', 0) for m in aggregate.values())
        
        metrics = {
            'scan_date': datetime.now().isoformat(),
            'total_gaps': total_gaps,
            'severity_critical': critical,
            'severity_high': high,
            'modules_scanned': len(aggregate),
            'actionable_gaps': critical + high
        }
        
        self.results['metrics'] = metrics
        
        # Save metrics
        with open(self.output_dir / 'pipeline_metrics.json', 'w') as f:
            json.dump(metrics, f, indent=2)
        
        logger.info(f"Total gaps: {total_gaps}")
        logger.info(f"Critical: {critical}, High: {high}")
        logger.info(f"Modules: {len(aggregate)}")
    
    def _step_cluster_gaps(self):
        """Step 3: Cluster gaps into logical issues"""
        if 'aggregate' not in self.results:
            raise ValueError("No scanner results")
        
        aggregate = self.results['aggregate']
        cluster_dir = self.output_dir / 'clustered_issues'
        cluster_dir.mkdir(exist_ok=True)
        
        clusters = self._create_clusters(aggregate)
        self.results['clusters'] = clusters
        
        # Save cluster definitions
        cluster_index = {}
        for cluster_name, gaps in clusters.items():
            cluster_index[cluster_name] = {
                'total_gaps': len(gaps),
                'categories': list(set(g.get('type', 'unknown') for g in gaps))
            }
        
        logger.info(f"Created {len(clusters)} clusters")
    
    def _create_clusters(self, aggregate: Dict) -> Dict[str, List]:
        """Group gaps by module and severity"""
        clusters = {}
        
        for module, data in aggregate.items():
            total = data.get('total', 0)
            critical = data.get('severity_critical', 0)
            
            if total == 0:
                continue
            
            # Create cluster name
            if critical >= 10:
                priority = 'P0-critical'
            elif data.get('severity_high', 0) >= 5:
                priority = 'P1-high'
            else:
                priority = 'P2-medium'
            
            cluster_name = f"{priority}_{module}"
            
            # Collect gaps for this module
            gaps_list = []
            for file_path, file_gaps in data.get('gaps_by_file', {}).items():
                gaps_list.extend(file_gaps)
            
            clusters[cluster_name] = gaps_list
        
        return clusters
    
    def _step_generate_templates(self):
        """Step 4: Generate GitHub issue templates from clusters"""
        if 'clusters' not in self.results:
            raise ValueError("No clusters found")
        
        if 'aggregate' not in self.results:
            raise ValueError("No aggregate data")
        
        aggregate = self.results['aggregate']
        cluster_dir = self.output_dir / 'clustered_issues'
        cluster_dir.mkdir(exist_ok=True)
        
        # Generate markdown template for each module
        template_count = 0
        for module, data in aggregate.items():
            total = data.get('total', 0)
            if total == 0:
                continue
            
            template = self._generate_issue_template(module, data)
            
            # Save template
            template_file = cluster_dir / f"{module}_gaps.md"
            with open(template_file, 'w', encoding='utf-8') as f:
                f.write(template)
            
            template_count += 1
        
        logger.info(f"Generated {template_count} issue templates")
    
    def _generate_issue_template(self, module: str, data: Dict) -> str:
        """Generate markdown issue template"""
        
        critical = data.get('severity_critical', 0)
        high = data.get('severity_high', 0)
        medium = data.get('severity_medium', 0)
        
        # Determine priority
        if critical >= 10:
            priority = '[CRITICAL] P0'
        elif high >= 5:
            priority = '[HIGH] P1'
        else:
            priority = '[MEDIUM] P2'
        
        template = f"""# {priority} — {module.upper()} Module Gap Analysis

## Summary

**Module:** `{module}`  
**Total Gaps:** {data.get('total', 0)}  
**CRITICAL:** {critical} | **HIGH:** {high} | **MEDIUM:** {medium}

## Breakdown by Category

"""
        
        for cat, count in sorted(data.get('by_category', {}).items(), 
                                key=lambda x: x[1], reverse=True):
            template += f"- **{cat}:** {count} gaps\n"
        
        # Top files
        template += "\n## Top Files by Gap Density\n\n"
        
        files_by_gap = sorted(
            data.get('gaps_by_file', {}).items(),
            key=lambda x: len(x[1]),
            reverse=True
        )[:10]
        
        for file_path, gaps_in_file in files_by_gap:
            severity_str = ", ".join([
                f"C:{sum(1 for g in gaps_in_file if g.get('severity') == 'CRITICAL')}",
                f"H:{sum(1 for g in gaps_in_file if g.get('severity') == 'HIGH')}",
                f"M:{sum(1 for g in gaps_in_file if g.get('severity') == 'MEDIUM')}"
            ])
            template += f"- `{file_path}`: {len(gaps_in_file)} gaps ({severity_str})\n"
        
        # Implementation guide
        template += f"""

## Implementation Guide

### Phase 1: Critical Fixes
Focus on CRITICAL severity gaps (data safety, security):
"""
        
        for file_path, gaps_in_file in files_by_gap[:5]:
            critical_gaps = [g for g in gaps_in_file if g.get('severity') == 'CRITICAL']
            if critical_gaps:
                template += f"- [ ] {file_path} ({len(critical_gaps)} critical gaps)\n"
        
        template += f"""

### Phase 2: High Priority Fixes
Address HIGH severity gaps (performance, reliability):
"""
        
        for file_path, gaps_in_file in files_by_gap[:5]:
            high_gaps = [g for g in gaps_in_file if g.get('severity') == 'HIGH']
            if high_gaps:
                template += f"- [ ] {file_path} ({len(high_gaps)} high gaps)\n"
        
        template += f"""

## Related Documentation

- [Module Gap Documentation](../../src/{module}/MODULE_GAPS.md)
- [Gap Scanner Report](../../ai_working/gap_scan_v3_{module}.json)
- [Full Gap Index](../../ai_working/module_gaps/MODULE_GAPS_INDEX.md)

## Metrics & Tracking

| Metric | Value |
|--------|-------|
| Total Gaps | {data.get('total', 0)} |
| CRITICAL | {critical} |
| HIGH | {high} |
| MEDIUM | {medium} |
| Estimated Effort | {self._estimate_effort(critical, high)} |

## Acceptance Criteria

- [ ] All CRITICAL gaps addressed
- [ ] All HIGH gaps reviewed and prioritized
- [ ] Documentation updated (ARCHITECTURE.md, ROADMAP.md)
- [ ] Tests added for gap fixes
- [ ] Code review completed

---
*Generated: {datetime.now().isoformat()}*  
*Gap Scanner: v3 Phase 1 (Security, Memory, Reliability)*
"""
        
        return template
    
    def _estimate_effort(self, critical: int, high: int) -> str:
        """Estimate development effort"""
        # Rough estimates: 2 hours per critical, 1 hour per high
        hours = (critical * 2) + (high * 1)
        
        if hours < 8:
            return f"{hours}h (1 dev-day)"
        elif hours < 40:
            days = hours / 8
            return f"{days:.1f} dev-days"
        else:
            weeks = hours / 40
            return f"{weeks:.1f} weeks"
    
    def _step_create_github_issues(self):
        """Step 5: Create issues on GitHub"""
        sys.path.insert(0, str(self.repo_root / 'tools'))
        
        try:
            from github_issue_creator import GitHubIssueCreator
            
            creator = GitHubIssueCreator(dry_run=False)
            cluster_dir = self.output_dir / 'clustered_issues'
            
            results = creator.batch_create_from_cluster_dir(cluster_dir)
            creator.save_creation_log()
            creator.print_summary()
            
            self.results['github_issues'] = results
            
        except ImportError as e:
            logger.error(f"Failed to import issue creator: {e}")
            raise
    
    def _step_generate_report(self):
        """Generate final status report"""
        
        report = f"""
# Gap Scanner Pipeline — Execution Report

**Execution Time:** {datetime.now().isoformat()}  
**Duration:** {(datetime.now() - self.start_time).total_seconds():.1f}s  
**Status:** [OK] Complete

## Metrics

"""
        
        if 'metrics' in self.results:
            metrics = self.results['metrics']
            report += f"""
- **Total Gaps Found:** {metrics['total_gaps']}
- **CRITICAL Severity:** {metrics['severity_critical']}
- **HIGH Severity:** {metrics['severity_high']}
- **Actionable Gaps:** {metrics['actionable_gaps']}
- **Modules Scanned:** {metrics['modules_scanned']}
"""
        
        report += f"""

## Artifacts Generated

- `gap_scan_v3_aggregate.json` — Complete gap analysis by module
- `gap_scan_v3_*.json` — Per-module detailed reports
- `gap_scan_v3_summary.json` — Summary statistics
- `clustered_issues/` — GitHub issue templates (ready to create)
- `github_issues_log.csv` — Created issues tracking log
- `pipeline_metrics.json` — Pipeline execution metrics

## Next Actions

1. **Review Issues:** Browse ai_working/clustered_issues/
2. **Create on GitHub:** Run with --github flag (requires gh CLI)
3. **Assign Team:** Add assignees and set priorities
4. **Track Progress:** Use GitHub Project board
5. **Implement Fixes:** Follow Phase 1/2/3 roadmap

## Recommendations

- Phase 1: Focus on CRITICAL gaps (data safety, security)
- Phase 2: Implement reliability fixes (retry logic, timeouts)
- Phase 3: Address performance and code quality

---
*Report generated by: gap_scanner_and_issues.py*
*Gap Scanner Version: 3.0 Phase 1*
"""
        
        report_file = self.output_dir / 'pipeline_report.md'
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(report)
        
        logger.info(f"Report saved: {report_file}")
        print(report)


def main():
    """CLI entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Gap Scanner → GitHub Issues Automation Pipeline'
    )
    parser.add_argument('--repo-root', default='.',
                       help='Repository root directory')
    parser.add_argument('--output-dir', default='ai_working',
                       help='Output directory for reports')
    parser.add_argument('--github', action='store_true',
                       help='Create issues on GitHub (requires gh CLI authentication)')
    
    args = parser.parse_args()
    
    pipeline = GapScannerPipeline(args.repo_root, args.output_dir)
    return pipeline.run_complete_pipeline(create_issues=args.github)


if __name__ == '__main__':
    sys.exit(main())
