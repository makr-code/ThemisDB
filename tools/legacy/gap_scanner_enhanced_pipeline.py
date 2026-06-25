#!/usr/bin/env python3
"""
Integrated Gap Scanner + Enhanced Issue Creator Pipeline

Pipeline:
  1. Run gap scanners (Phase 1-5)
  2. Generate aggregate results
  3. Create enhanced issue templates (AI-agent ready)
  4. Create GitHub issues with detailed remediation guidance
  5. Generate execution report

Usage:
  python tools/gap_scanner_enhanced_pipeline.py [--github] [--dry-run]
"""

import json
import subprocess
import sys
from pathlib import Path
from typing import Dict, List
from datetime import datetime
import logging

logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)-8s] %(message)s'
)
logger = logging.getLogger(__name__)


class EnhancedGapScannerPipeline:
    """Orchestrate gap scanning with enhanced issue templates"""
    
    def __init__(self, repo_root: str = '.', output_dir: str = 'ai_working'):
        self.repo_root = Path(repo_root)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def run_enhanced_pipeline(self, create_github_issues: bool = False, dry_run: bool = False) -> int:
        """Execute full pipeline with enhanced templates"""
        
        print("\n" + "=" * 100)
        print("ThemisDB Gap Scanner + Enhanced Issues Pipeline")
        print("=" * 100)
        
        steps = [
            ('Load Aggregate Gap Results', self._step_load_aggregate),
            ('Generate Enhanced Issue Templates', self._step_generate_enhanced_templates),
            ('Prepare GitHub Issues', self._step_prepare_github_issues),
        ]
        
        if create_github_issues:
            steps.append(('Create GitHub Issues', lambda: self._step_create_github_issues(dry_run)))
        
        for step_name, step_func in steps:
            try:
                print(f"\n[...] {step_name}")
                step_func()
                print(f"[OK] {step_name} complete")
            except Exception as e:
                logger.error(f"[ABORT] {step_name} failed: {e}")
                import traceback
                traceback.print_exc()
                return 1
        
        print("\n" + "=" * 100)
        print("Pipeline Complete ✓")
        print("=" * 100)
        return 0
    
    def _step_load_aggregate(self):
        """Load aggregate gap results"""
        aggregate_file = self.output_dir / 'gap_scan_v3_aggregate.json'
        
        if not aggregate_file.exists():
            raise FileNotFoundError(f"Gap analysis file not found: {aggregate_file}")
        
        with open(aggregate_file, 'r') as f:
            self.aggregate = json.load(f)
        
        # Calculate summary
        total_gaps = sum(data.get('total', 0) for data in self.aggregate.values())
        total_critical = sum(data.get('severity_critical', 0) for data in self.aggregate.values())
        total_high = sum(data.get('severity_high', 0) for data in self.aggregate.values())
        
        print(f"  Loaded: {len(self.aggregate)} modules")
        print(f"  Total gaps: {total_gaps} (🔴 {total_critical}, 🟠 {total_high})")
    
    def _step_generate_enhanced_templates(self):
        """Generate enhanced issue templates using specialized generator"""
        
        # Import enhanced template generator
        sys.path.insert(0, str(self.repo_root / 'tools'))
        
        try:
            from gap_issue_enhanced_template_generator import generate_all_enhanced_templates
            
            aggregate_file = self.output_dir / 'gap_scan_v3_aggregate.json'
            generate_all_enhanced_templates(str(aggregate_file), str(self.output_dir))
            
            enhanced_dir = self.output_dir / 'enhanced_issues'
            template_count = len(list(enhanced_dir.glob('*.md')))
            print(f"  Generated: {template_count} enhanced issue templates")
            
        except ImportError as e:
            logger.warning(f"Enhanced template generator not available: {e}")
            logger.info("Falling back to standard templates")
    
    def _step_prepare_github_issues(self):
        """Prepare GitHub issues from enhanced templates"""
        
        enhanced_dir = self.output_dir / 'enhanced_issues'
        
        if not enhanced_dir.exists():
            logger.warning(f"No enhanced issue templates found in {enhanced_dir}")
            return
        
        # List all generated templates
        template_files = list(enhanced_dir.glob('*_enhanced_issues.md'))
        
        print(f"  Prepared: {len(template_files)} GitHub issue templates")
        for template_file in template_files:
            module = template_file.stem.replace('_enhanced_issues', '')
            print(f"    - {module}: {template_file.stat().st_size:,} bytes")
    
    def _step_create_github_issues(self, dry_run: bool = False):
        """Create GitHub issues from enhanced templates"""
        
        sys.path.insert(0, str(self.repo_root / 'tools'))
        
        try:
            from github_issue_creator import GitHubIssueCreator, GitHubIssue
            
            creator = GitHubIssueCreator(repo="makr-code/ThemisDB", dry_run=dry_run)
            
            enhanced_dir = self.output_dir / 'enhanced_issues'
            template_files = list(enhanced_dir.glob('*_enhanced_issues.md'))
            
            created_count = 0
            for template_file in template_files:
                with open(template_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Extract title from first heading
                first_line = content.split('\n')[0]
                title = first_line.lstrip('# ').strip()
                
                # Determine labels
                labels = []
                if '🔴 CRITICAL' in content:
                    labels.append('P0-critical')
                elif '🟠 HIGH' in content:
                    labels.append('P1-high')
                else:
                    labels.append('P2-medium')
                
                labels.extend(['gap-scanner', 'ai-agent-ready'])
                
                # Extract module name
                module = template_file.stem.replace('_enhanced_issues', '')
                labels.append(f'module-{module}')
                
                # Create issue
                issue = GitHubIssue(
                    title=title,
                    body=content,
                    labels=labels,
                    milestone='v1.5.0' if 'HIGH' in content or 'CRITICAL' in content else 'Backlog'
                )
                
                result = creator._create_github_issue(issue)
                if result:
                    created_count += 1
                    print(f"  Created: {module} → {result}")
            
            print(f"\n  Total: {created_count} issues created")
            
        except ImportError as e:
            logger.error(f"Failed to create GitHub issues: {e}")
            raise


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Gap Scanner + Enhanced Issues Pipeline')
    parser.add_argument('--github', action='store_true', help='Create issues on GitHub')
    parser.add_argument('--dry-run', action='store_true', help='Dry run (no GitHub changes)')
    
    args = parser.parse_args()
    
    pipeline = EnhancedGapScannerPipeline()
    return pipeline.run_enhanced_pipeline(
        create_github_issues=args.github,
        dry_run=args.dry_run
    )


if __name__ == '__main__':
    sys.exit(main())
