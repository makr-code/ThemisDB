"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analyze_workflows.py                               ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     550                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Workflow Inventory Analyzer for ThemisDB CI/CD Workflows

This script analyzes all GitHub Actions workflow files in .github/workflows/
and generates a comprehensive inventory document with detailed metadata.

Usage:
    python3 tools/ci/analyze_workflows.py

Output:
    - docs/ci-cd/workflows-inventory.md (inventory document)
"""

import os
import sys
import yaml
import re
from pathlib import Path
from typing import Dict, List, Any, Set
from collections import defaultdict


class WorkflowAnalyzer:
    """Analyzes GitHub Actions workflow files and generates inventory."""
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.workflows_dir = repo_root / ".github" / "workflows"
        self.workflows = []
        
    def find_workflows(self) -> List[Path]:
        """Find all workflow files."""
        patterns = ["*.yml", "*.yaml"]
        files = []
        for pattern in patterns:
            files.extend(self.workflows_dir.glob(pattern))
        return sorted(files)
    
    def extract_triggers(self, on_config: Any) -> Dict[str, Any]:
        """Extract trigger information from 'on:' configuration."""
        triggers = {
            'events': [],
            'branches': [],
            'tags': [],
            'paths': [],
            'schedule': [],
            'inputs': {},
            'types': []
        }
        
        if isinstance(on_config, str):
            triggers['events'].append(on_config)
        elif isinstance(on_config, list):
            triggers['events'].extend(on_config)
        elif isinstance(on_config, dict):
            for event, config in on_config.items():
                triggers['events'].append(event)
                
                if isinstance(config, dict):
                    if 'branches' in config:
                        branches = config['branches']
                        if isinstance(branches, list):
                            triggers['branches'].extend(branches)
                        elif isinstance(branches, str):
                            triggers['branches'].append(branches)
                    
                    if 'tags' in config:
                        tags = config['tags']
                        if isinstance(tags, list):
                            triggers['tags'].extend(tags)
                        elif isinstance(tags, str):
                            triggers['tags'].append(tags)
                    
                    if 'paths' in config:
                        paths = config['paths']
                        if isinstance(paths, list):
                            triggers['paths'].extend(paths)
                    
                    if 'types' in config:
                        types = config['types']
                        if isinstance(types, list):
                            triggers['types'].extend(types)
                    
                    if 'inputs' in config:
                        triggers['inputs'] = config['inputs']
                
                if event == 'schedule':
                    if isinstance(config, list):
                        for sched in config:
                            if isinstance(sched, dict) and 'cron' in sched:
                                triggers['schedule'].append(sched['cron'])
        
        return triggers
    
    def extract_jobs_info(self, jobs: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Extract job information."""
        job_list = []
        
        for job_id, job_config in jobs.items():
            if not isinstance(job_config, dict):
                continue
                
            job_info = {
                'id': job_id,
                'name': job_config.get('name', job_id),
                'runs_on': self.extract_runs_on(job_config.get('runs-on')),
                'if': job_config.get('if', ''),
                'needs': job_config.get('needs', []),
                'strategy': job_config.get('strategy', {}),
                'uses': job_config.get('uses', ''),
                'steps_count': len(job_config.get('steps', [])) if 'steps' in job_config else 0
            }
            job_list.append(job_info)
        
        return job_list
    
    def extract_runs_on(self, runs_on: Any) -> List[str]:
        """Extract runner information."""
        if isinstance(runs_on, str):
            return [runs_on]
        elif isinstance(runs_on, list):
            return runs_on
        elif isinstance(runs_on, dict):
            # Handle matrix or other complex configurations
            return ['complex-matrix']
        return []
    
    def extract_actions_used(self, jobs: Dict[str, Any]) -> Set[str]:
        """Extract all GitHub Actions used in workflow."""
        actions = set()
        
        for job_id, job_config in jobs.items():
            if not isinstance(job_config, dict):
                continue
            
            # Check if it's a reusable workflow
            if 'uses' in job_config:
                actions.add(f"workflow:{job_config['uses']}")
            
            steps = job_config.get('steps', [])
            for step in steps:
                if isinstance(step, dict) and 'uses' in step:
                    action = step['uses'].split('@')[0]  # Remove version
                    actions.add(action)
        
        return actions
    
    def extract_permissions(self, workflow: Dict[str, Any]) -> Dict[str, Any]:
        """Extract permissions configuration."""
        permissions = workflow.get('permissions', {})
        if isinstance(permissions, str):
            return {'all': permissions}
        return permissions
    
    def extract_concurrency(self, workflow: Dict[str, Any]) -> Dict[str, Any]:
        """Extract concurrency configuration."""
        concurrency = workflow.get('concurrency', {})
        if isinstance(concurrency, str):
            return {'group': concurrency}
        return concurrency
    
    def extract_env_vars(self, workflow: Dict[str, Any]) -> Dict[str, str]:
        """Extract environment variables."""
        return workflow.get('env', {})
    
    def detect_technologies(self, workflow_data: Dict[str, Any], actions: Set[str]) -> List[str]:
        """Detect technologies and tools used in workflow."""
        technologies = set()
        
        # Check workflow content as string
        workflow_str = str(workflow_data).lower()
        
        # Language detection
        if 'cmake' in workflow_str or 'CMakeLists' in workflow_str:
            technologies.add('CMake')
        if 'vcpkg' in workflow_str:
            technologies.add('vcpkg')
        if 'python' in workflow_str or 'actions/setup-python' in actions:
            technologies.add('Python')
        if 'node' in workflow_str or 'npm' in workflow_str or 'actions/setup-node' in actions:
            technologies.add('Node.js')
        if 'dotnet' in workflow_str or '.net' in workflow_str or 'actions/setup-dotnet' in actions:
            technologies.add('.NET')
        if 'go' in workflow_str or 'actions/setup-go' in actions:
            technologies.add('Go')
        if 'java' in workflow_str or 'actions/setup-java' in actions:
            technologies.add('Java')
        if 'ruby' in workflow_str or 'actions/setup-ruby' in actions:
            technologies.add('Ruby')
        if 'rust' in workflow_str or 'cargo' in workflow_str:
            technologies.add('Rust')
        if 'swift' in workflow_str:
            technologies.add('Swift')
        if 'php' in workflow_str:
            technologies.add('PHP')
        
        # Tools detection
        if 'docker' in workflow_str or 'docker/build-push-action' in actions:
            technologies.add('Docker')
        if 'helm' in workflow_str:
            technologies.add('Helm')
        if 'mkdocs' in workflow_str:
            technologies.add('MkDocs')
        if 'fuzzing' in workflow_str or 'libfuzzer' in workflow_str:
            technologies.add('Fuzzing')
        if 'sanitizer' in workflow_str or 'asan' in workflow_str:
            technologies.add('Sanitizers')
        if 'codeql' in workflow_str or 'github/codeql-action' in actions:
            technologies.add('CodeQL')
        if 'owasp' in workflow_str or 'zap' in workflow_str:
            technologies.add('OWASP ZAP')
        if 'sbom' in workflow_str or 'anchore' in workflow_str:
            technologies.add('SBOM/Anchore')
        if 'lora' in workflow_str or 'llama' in workflow_str:
            technologies.add('LoRA/LLM')
        
        return sorted(list(technologies))
    
    def analyze_workflow(self, workflow_path: Path) -> Dict[str, Any]:
        """Analyze a single workflow file."""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                workflow = yaml.safe_load(f)
            
            if not isinstance(workflow, dict):
                return None
            
            # Extract basic information
            name = workflow.get('name', workflow_path.stem)
            on_config = workflow.get('on', workflow.get(True, {}))
            
            triggers = self.extract_triggers(on_config)
            jobs = workflow.get('jobs', {})
            jobs_info = self.extract_jobs_info(jobs)
            actions = self.extract_actions_used(jobs)
            permissions = self.extract_permissions(workflow)
            concurrency = self.extract_concurrency(workflow)
            env_vars = self.extract_env_vars(workflow)
            technologies = self.detect_technologies(workflow, actions)
            
            return {
                'filepath': str(workflow_path.relative_to(self.repo_root)),
                'filename': workflow_path.name,
                'name': name,
                'triggers': triggers,
                'jobs': jobs_info,
                'actions': sorted(list(actions)),
                'permissions': permissions,
                'concurrency': concurrency,
                'env': env_vars,
                'technologies': technologies,
                'job_count': len(jobs_info)
            }
        except Exception as e:
            print(f"Error analyzing {workflow_path}: {e}", file=sys.stderr)
            return None
    
    def analyze_all(self) -> List[Dict[str, Any]]:
        """Analyze all workflows."""
        workflow_files = self.find_workflows()
        print(f"Found {len(workflow_files)} workflow files")
        
        for workflow_path in workflow_files:
            print(f"Analyzing {workflow_path.name}...")
            result = self.analyze_workflow(workflow_path)
            if result:
                self.workflows.append(result)
        
        return self.workflows
    
    def categorize_workflows(self) -> Dict[str, List[Dict[str, Any]]]:
        """Categorize workflows by their purpose."""
        categories = defaultdict(list)
        
        for workflow in self.workflows:
            name_lower = workflow['name'].lower()
            filename_lower = workflow['filename'].lower()
            triggers = workflow['triggers']['events']
            
            # Categorization logic
            if 'release' in filename_lower or 'release' in name_lower:
                categories['Release'].append(workflow)
            elif 'maturity' in filename_lower or 'versioning' in filename_lower:
                categories['Code Quality & Versioning'].append(workflow)
            elif 'security' in filename_lower or 'owasp' in filename_lower or 'audit' in filename_lower:
                categories['Security & Compliance'].append(workflow)
            elif 'docs' in filename_lower or 'documentation' in filename_lower or 'wiki' in filename_lower:
                categories['Documentation'].append(workflow)
            elif 'sdk' in filename_lower or any(lang in filename_lower for lang in ['python', 'java', 'go', 'csharp', 'ruby', 'rust', 'swift', 'php', 'javascript']):
                categories['SDK Testing'].append(workflow)
            elif 'lora' in filename_lower or 'llama' in filename_lower:
                categories['LoRA/LLM'].append(workflow)
            elif 'docker' in filename_lower or 'helm' in filename_lower:
                categories['Container & Deployment'].append(workflow)
            elif 'fuzzing' in filename_lower or 'fuzz' in filename_lower:
                categories['Fuzzing'].append(workflow)
            elif 'sanitizer' in filename_lower or 'asan' in filename_lower:
                categories['Sanitizers'].append(workflow)
            elif 'arm' in filename_lower or 'cross' in filename_lower:
                categories['Cross-Compilation'].append(workflow)
            elif 'benchmark' in filename_lower or 'performance' in filename_lower:
                categories['Performance & Benchmarking'].append(workflow)
            elif 'test' in filename_lower or 'chaos' in filename_lower or 'durability' in filename_lower:
                categories['Testing'].append(workflow)
            elif 'ci' in filename_lower and ('pr' in triggers or 'pull_request' in triggers):
                categories['PR CI'].append(workflow)
            elif 'main' in filename_lower or 'develop' in filename_lower:
                categories['Branch CI'].append(workflow)
            elif any(event in triggers for event in ['schedule', 'workflow_dispatch']):
                categories['Scheduled & Manual'].append(workflow)
            else:
                categories['Other'].append(workflow)
        
        return categories
    
    def generate_inventory_markdown(self, output_path: Path):
        """Generate markdown inventory document."""
        categories = self.categorize_workflows()
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write("# GitHub Actions Workflows Inventory\n\n")
            f.write(f"**Generated:** {self._get_timestamp()}\n\n")
            f.write(f"**Total Workflows:** {len(self.workflows)}\n\n")
            
            # Table of Contents
            f.write("## Table of Contents\n\n")
            f.write("1. [Summary Statistics](#summary-statistics)\n")
            f.write("2. [Workflows by Category](#workflows-by-category)\n")
            f.write("3. [Detailed Workflow Inventory](#detailed-workflow-inventory)\n")
            f.write("4. [Common Patterns Analysis](#common-patterns-analysis)\n")
            f.write("5. [Actions Usage Summary](#actions-usage-summary)\n\n")
            
            # Summary Statistics
            f.write("## Summary Statistics\n\n")
            f.write(f"- **Total Workflows:** {len(self.workflows)}\n")
            f.write(f"- **Total Jobs:** {sum(w['job_count'] for w in self.workflows)}\n")
            
            all_actions = set()
            for w in self.workflows:
                all_actions.update(w['actions'])
            f.write(f"- **Unique Actions Used:** {len(all_actions)}\n\n")
            
            # Category breakdown
            f.write("### Workflows by Category\n\n")
            for category in sorted(categories.keys()):
                f.write(f"- **{category}:** {len(categories[category])} workflows\n")
            f.write("\n")
            
            # Detailed categories
            f.write("## Workflows by Category\n\n")
            for category in sorted(categories.keys()):
                f.write(f"### {category}\n\n")
                for workflow in sorted(categories[category], key=lambda x: x['filename']):
                    f.write(f"- **{workflow['filename']}** - {workflow['name']}\n")
                f.write("\n")
            
            # Detailed Inventory
            f.write("## Detailed Workflow Inventory\n\n")
            for workflow in sorted(self.workflows, key=lambda x: x['filename']):
                self._write_workflow_details(f, workflow)
            
            # Common Patterns
            f.write("## Common Patterns Analysis\n\n")
            self._write_patterns_analysis(f)
            
            # Actions Usage
            f.write("## Actions Usage Summary\n\n")
            self._write_actions_summary(f)
    
    def _write_workflow_details(self, f, workflow: Dict[str, Any]):
        """Write detailed information for a single workflow."""
        f.write(f"### {workflow['filename']}\n\n")
        f.write(f"**Name:** {workflow['name']}\n\n")
        f.write(f"**Path:** `{workflow['filepath']}`\n\n")
        
        # Triggers
        f.write("**Triggers:**\n")
        events = workflow['triggers']['events']
        if events:
            f.write(f"- Events: {', '.join(events)}\n")
        if workflow['triggers']['branches']:
            f.write(f"- Branches: {', '.join(workflow['triggers']['branches'])}\n")
        if workflow['triggers']['tags']:
            f.write(f"- Tags: {', '.join(workflow['triggers']['tags'])}\n")
        if workflow['triggers']['paths']:
            f.write(f"- Paths: {len(workflow['triggers']['paths'])} path filters\n")
        if workflow['triggers']['schedule']:
            f.write(f"- Schedule: {', '.join(workflow['triggers']['schedule'])}\n")
        if workflow['triggers']['types']:
            f.write(f"- Types: {', '.join(workflow['triggers']['types'])}\n")
        f.write("\n")
        
        # Jobs
        f.write(f"**Jobs ({len(workflow['jobs'])}):**\n")
        for job in workflow['jobs']:
            runner = ', '.join(job['runs_on']) if job['runs_on'] else 'N/A'
            f.write(f"- `{job['id']}`: {job['name']} (runs-on: {runner})\n")
            if job['uses']:
                f.write(f"  - Reusable workflow: `{job['uses']}`\n")
        f.write("\n")
        
        # Technologies
        if workflow['technologies']:
            f.write(f"**Technologies:** {', '.join(workflow['technologies'])}\n\n")
        
        # Permissions
        if workflow['permissions']:
            f.write("**Permissions:**\n")
            if isinstance(workflow['permissions'], dict):
                for perm, value in workflow['permissions'].items():
                    f.write(f"- {perm}: {value}\n")
            f.write("\n")
        
        # Concurrency
        if workflow['concurrency']:
            f.write("**Concurrency:**\n")
            if isinstance(workflow['concurrency'], dict):
                for key, value in workflow['concurrency'].items():
                    f.write(f"- {key}: {value}\n")
            f.write("\n")
        
        # Actions
        if workflow['actions']:
            f.write(f"**Actions Used ({len(workflow['actions'])}):**\n")
            for action in sorted(workflow['actions'])[:10]:  # Limit to first 10
                f.write(f"- `{action}`\n")
            if len(workflow['actions']) > 10:
                f.write(f"- ... and {len(workflow['actions']) - 10} more\n")
            f.write("\n")
        
        f.write("---\n\n")
    
    def _write_patterns_analysis(self, f):
        """Write common patterns analysis."""
        # Count action usage
        action_count = defaultdict(int)
        for workflow in self.workflows:
            for action in workflow['actions']:
                action_count[action] += 1
        
        f.write("### Most Used Actions\n\n")
        sorted_actions = sorted(action_count.items(), key=lambda x: x[1], reverse=True)
        for action, count in sorted_actions[:15]:
            f.write(f"- `{action}`: used in {count} workflows\n")
        f.write("\n")
        
        # Technology distribution
        tech_count = defaultdict(int)
        for workflow in self.workflows:
            for tech in workflow['technologies']:
                tech_count[tech] += 1
        
        f.write("### Technology Distribution\n\n")
        for tech, count in sorted(tech_count.items(), key=lambda x: x[1], reverse=True):
            f.write(f"- {tech}: {count} workflows\n")
        f.write("\n")
        
        # Runner distribution
        runner_count = defaultdict(int)
        for workflow in self.workflows:
            for job in workflow['jobs']:
                for runner in job['runs_on']:
                    runner_count[runner] += 1
        
        f.write("### Runner Distribution\n\n")
        for runner, count in sorted(runner_count.items(), key=lambda x: x[1], reverse=True):
            f.write(f"- {runner}: {count} jobs\n")
        f.write("\n")
    
    def _write_actions_summary(self, f):
        """Write actions usage summary."""
        all_actions = set()
        for workflow in self.workflows:
            all_actions.update(workflow['actions'])
        
        # Group by source
        github_actions = [a for a in all_actions if a.startswith('actions/')]
        third_party = [a for a in all_actions if not a.startswith('actions/') and not a.startswith('workflow:')]
        reusable_workflows = [a for a in all_actions if a.startswith('workflow:')]
        
        f.write(f"**Total Unique Actions:** {len(all_actions)}\n\n")
        f.write(f"- GitHub Official Actions: {len(github_actions)}\n")
        f.write(f"- Third-Party Actions: {len(third_party)}\n")
        f.write(f"- Reusable Workflows: {len(reusable_workflows)}\n\n")
        
        if github_actions:
            f.write("### GitHub Official Actions\n\n")
            for action in sorted(github_actions):
                f.write(f"- `{action}`\n")
            f.write("\n")
        
        if reusable_workflows:
            f.write("### Reusable Workflows\n\n")
            for workflow in sorted(reusable_workflows):
                f.write(f"- `{workflow}`\n")
            f.write("\n")
    
    def _get_timestamp(self):
        """Get current timestamp."""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def main():
    """Main entry point."""
    repo_root = Path(__file__).resolve().parent.parent.parent
    
    print("Starting workflow analysis...")
    analyzer = WorkflowAnalyzer(repo_root)
    
    # Analyze all workflows
    workflows = analyzer.analyze_all()
    print(f"\nAnalyzed {len(workflows)} workflows")
    
    # Generate inventory
    output_path = repo_root / "docs" / "ci-cd" / "workflows-inventory.md"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Generating inventory document: {output_path}")
    analyzer.generate_inventory_markdown(output_path)
    
    print("\n✓ Workflow inventory generated successfully!")
    print(f"  Output: {output_path}")


if __name__ == "__main__":
    main()
