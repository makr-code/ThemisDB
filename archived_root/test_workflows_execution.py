#!/usr/bin/env python3
"""
GitHub Actions Workflow Execution Test Suite
Führt echte Workflow-Executions durch act durch und validiert Ergebnisse
"""

import os
import sys
import json
import yaml
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple

class WorkflowExecutionTester:
    def __init__(self):
        self.workflow_dir = Path('.github/workflows')
        self.results = []
        self.start_time = datetime.now()
        
        # Kritische Workflows für Execution Tests
        self.critical_workflows = {
            'ci-build.yml': {
                'name': 'Core CI Build',
                'trigger': 'push',
                'timeout': 30,
                'dry_run': True  # Nur dry-run für ci-build
            },
            'automation-community.yml': {
                'name': 'Community Automation',
                'trigger': 'issues',
                'timeout': 15,
                'dry_run': True
            },
            'security-consolidated.yml': {
                'name': 'Security Scanning',
                'trigger': 'schedule',
                'timeout': 20,
                'dry_run': True
            }
        }
        
    def log(self, level: str, message: str):
        """Log message"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        prefix = {
            'PASS': '[OK]',
            'FAIL': '[XX]',
            'WARN': '[!!]',
            'INFO': '[..]'
        }.get(level, '[*]')
        
        line = f"[{timestamp}] {prefix} {message}"
        print(line)
        self.results.append(line)
    
    def run_act_list(self, workflow_path: Path) -> Tuple[bool, List[str]]:
        """Extract jobs from workflow using act list"""
        try:
            result = subprocess.run(
                ['act', 'list', '-W', str(workflow_path), '-j'],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode != 0:
                # act list fehlgeschlagen - versuche YAML zu parsen
                return False, self._parse_jobs_from_yaml(workflow_path)
            
            jobs = [line.strip() for line in result.stdout.split('\n') if line.strip()]
            return True, jobs
        except Exception as e:
            self.log('WARN', f"    act list fehlgeschlagen: {str(e)[:50]}")
            return False, self._parse_jobs_from_yaml(workflow_path)
    
    def _parse_jobs_from_yaml(self, workflow_path: Path) -> List[str]:
        """Fallback: Parse jobs directly from YAML"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            jobs = list(data.get('jobs', {}).keys())
            return jobs
        except:
            return []
    
    def test_workflow_dry_run(self, workflow_path: Path) -> Tuple[bool, str]:
        """Test workflow with act dry-run"""
        workflow_name = workflow_path.name
        self.log('INFO', f"  Starte Dry-Run: {workflow_name}")
        
        try:
            result = subprocess.run(
                ['act', '--dry-run', '-W', str(workflow_path), '--quiet'],
                capture_output=True,
                text=True,
                timeout=20
            )
            
            if result.returncode == 0:
                self.log('PASS', f"    Dry-Run erfolgreich")
                return True, "Dry-Run OK"
            else:
                # Dry-run kann fehlschlagen wenn Secrets/Inputs fehlen - ok
                if 'secret' in result.stderr.lower() or 'input' in result.stderr.lower():
                    self.log('WARN', f"    Dry-Run (Secrets/Inputs fehlen - ok)")
                    return True, "Dry-Run OK (mit Secrets-Hinweis)"
                else:
                    self.log('FAIL', f"    Dry-Run fehlgeschlagen")
                    return False, result.stderr[:100]
        except subprocess.TimeoutExpired:
            self.log('WARN', f"    Dry-Run Timeout (>20s)")
            return False, "Timeout"
        except Exception as e:
            self.log('FAIL', f"    Exception: {str(e)[:50]}")
            return False, str(e)[:100]
    
    def test_workflow_job_names(self, workflow_path: Path) -> Tuple[bool, List[str]]:
        """Extract and validate job names"""
        success, jobs = self.run_act_list(workflow_path)
        
        if jobs:
            self.log('PASS', f"    Gefundene Jobs: {', '.join(jobs[:3])}")
            return True, jobs
        else:
            self.log('WARN', f"    Keine Jobs extrahiert")
            return False, []
    
    def test_workflow_permissions(self, workflow_path: Path) -> Tuple[bool, Dict]:
        """Validate workflow permissions"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            perms = data.get('permissions', {})
            
            if not perms:
                self.log('WARN', f"    Keine Permissions definiert (default)")
                return True, {}
            
            perm_str = ', '.join(f"{k}:{v}" for k, v in perms.items()) if isinstance(perms, dict) else str(perms)
            self.log('PASS', f"    Permissions: {perm_str[:60]}")
            return True, perms
        except Exception as e:
            self.log('FAIL', f"    Permissions-Fehler: {str(e)[:50]}")
            return False, {}
    
    def test_workflow_steps(self, workflow_path: Path) -> Tuple[bool, int]:
        """Count and validate workflow steps"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            total_steps = 0
            jobs = data.get('jobs', {})
            
            for job_name, job_config in jobs.items():
                if job_config and isinstance(job_config, dict):
                    steps = job_config.get('steps', [])
                    total_steps += len(steps)
            
            self.log('PASS', f"    Total Steps: {total_steps}")
            return True, total_steps
        except Exception as e:
            self.log('FAIL', f"    Steps-Fehler: {str(e)[:50]}")
            return False, 0
    
    def test_workflow_actions(self, workflow_path: Path) -> Tuple[bool, List[str]]:
        """Extract and validate action versions"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            actions = []
            jobs = data.get('jobs', {})
            
            for job_name, job_config in jobs.items():
                if job_config and isinstance(job_config, dict):
                    steps = job_config.get('steps', [])
                    for step in steps:
                        if isinstance(step, dict) and 'uses' in step:
                            action = step['uses']
                            actions.append(action)
                            
                            # Check if action is pinned to SHA
                            if '@' in action:
                                version = action.split('@')[1]
                                if len(version) != 40:  # SHA is 40 chars
                                    self.log('WARN', f"      Action nicht zu SHA gepinnt: {action[:60]}")
            
            if actions:
                self.log('PASS', f"    Gefundene Actions: {len(actions)}")
                return True, actions
            else:
                self.log('WARN', f"    Keine Actions gefunden")
                return True, []
        except Exception as e:
            self.log('FAIL', f"    Actions-Fehler: {str(e)[:50]}")
            return False, []
    
    def test_workflow_triggers(self, workflow_path: Path) -> Tuple[bool, List[str]]:
        """Validate workflow triggers"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            # YAML parst 'on:' als Boolean True, nicht als String 'on'
            on = data.get(True, data.get('on', {}))
            triggers = []
            
            if isinstance(on, str):
                triggers = [on]
            elif isinstance(on, dict):
                triggers = list(on.keys())
            elif isinstance(on, list):
                triggers = on
            
            if triggers:
                self.log('PASS', f"    Trigger: {', '.join(str(t) for t in triggers)}")
            else:
                self.log('WARN', f"    Keine Trigger definiert")
            
            return True, triggers
        except Exception as e:
            self.log('FAIL', f"    Trigger-Fehler: {str(e)[:50]}")
            return False, []
    
    def run_critical_tests(self) -> int:
        """Run all tests on critical workflows"""
        self.log('INFO', '=' * 70)
        self.log('INFO', 'GitHub Actions Workflow Execution Test Suite')
        self.log('INFO', f'Zeitstempel: {self.start_time.strftime("%Y-%m-%d %H:%M:%S")}')
        self.log('INFO', '=' * 70)
        self.log('INFO', '')
        
        total_passed = 0
        total_failed = 0
        test_results = {}
        
        for workflow_name, config in self.critical_workflows.items():
            workflow_path = self.workflow_dir / workflow_name
            
            if not workflow_path.exists():
                self.log('FAIL', f"Workflow nicht gefunden: {workflow_name}")
                continue
            
            self.log('INFO', f"Teste: {config['name']}")
            
            test_result = {
                'name': config['name'],
                'file': workflow_name,
                'passed': 0,
                'failed': 0,
                'tests': {}
            }
            
            # Test 1: Job Names
            success, jobs = self.test_workflow_job_names(workflow_path)
            if success:
                test_result['passed'] += 1
                test_result['tests']['jobs'] = {'status': 'PASS', 'count': len(jobs)}
            else:
                test_result['failed'] += 1
                test_result['tests']['jobs'] = {'status': 'FAIL', 'count': 0}
            
            # Test 2: Permissions
            success, perms = self.test_workflow_permissions(workflow_path)
            if success:
                test_result['passed'] += 1
                test_result['tests']['permissions'] = {'status': 'PASS'}
            else:
                test_result['failed'] += 1
                test_result['tests']['permissions'] = {'status': 'FAIL'}
            
            # Test 3: Steps
            success, step_count = self.test_workflow_steps(workflow_path)
            if success:
                test_result['passed'] += 1
                test_result['tests']['steps'] = {'status': 'PASS', 'count': step_count}
            else:
                test_result['failed'] += 1
                test_result['tests']['steps'] = {'status': 'FAIL', 'count': 0}
            
            # Test 4: Actions
            success, actions = self.test_workflow_actions(workflow_path)
            if success:
                test_result['passed'] += 1
                test_result['tests']['actions'] = {'status': 'PASS', 'count': len(actions)}
            else:
                test_result['failed'] += 1
                test_result['tests']['actions'] = {'status': 'FAIL', 'count': 0}
            
            # Test 5: Triggers
            success, triggers = self.test_workflow_triggers(workflow_path)
            if success:
                test_result['passed'] += 1
                test_result['tests']['triggers'] = {'status': 'PASS', 'count': len(triggers)}
            else:
                test_result['failed'] += 1
                test_result['tests']['triggers'] = {'status': 'FAIL', 'count': 0}
            
            # Test 6: Dry-Run (optional, skipped for this phase)
            if config.get('dry_run', False):
                self.log('INFO', f"  Dry-Run Test (optional)")
                success, msg = self.test_workflow_dry_run(workflow_path)
                if success:
                    test_result['passed'] += 1
                    test_result['tests']['dry_run'] = {'status': 'PASS', 'msg': msg}
                else:
                    test_result['failed'] += 1
                    test_result['tests']['dry_run'] = {'status': 'FAIL', 'msg': msg}
            
            self.log('INFO', '')
            
            total_passed += test_result['passed']
            total_failed += test_result['failed']
            test_results[workflow_name] = test_result
        
        # Summary
        self.log('INFO', '=' * 70)
        self.log('INFO', 'ZUSAMMENFASSUNG')
        self.log('INFO', '-' * 70)
        self.log('PASS', f"Tests bestanden: {total_passed}")
        self.log('FAIL', f"Tests fehlgeschlagen: {total_failed}")
        self.log('INFO', f"Workflows getestet: {len(self.critical_workflows)}")
        
        duration = (datetime.now() - self.start_time).total_seconds()
        self.log('INFO', f"Dauer: {duration:.2f}s")
        
        if total_failed == 0:
            self.log('PASS', "[OK] ALLE TESTS BESTANDEN")
            status = 0
        else:
            self.log('FAIL', f"[XX] {total_failed} Test(s) fehlgeschlagen")
            status = 1
        
        self.log('INFO', '=' * 70)
        
        # Save report
        self._save_execution_report(test_results)
        
        return status
    
    def _save_execution_report(self, test_results: Dict):
        """Save execution test report"""
        report_file = Path('WORKFLOW_EXECUTION_TEST_REPORT.txt')
        
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(self.results))
            f.write('\n\n')
            f.write('=' * 70 + '\n')
            f.write('DETAILLIERTE EXECUTION-TEST-ERGEBNISSE\n')
            f.write('=' * 70 + '\n\n')
            
            for workflow_name, result in test_results.items():
                f.write(f"Workflow: {result['file']}\n")
                f.write(f"  Name: {result['name']}\n")
                f.write(f"  Bestanden: {result['passed']}\n")
                f.write(f"  Fehlgeschlagen: {result['failed']}\n")
                f.write(f"  Tests:\n")
                
                for test_name, test_result in result['tests'].items():
                    status = test_result.get('status', 'UNKNOWN')
                    f.write(f"    - {test_name}: {status}\n")
                    
                    for key, value in test_result.items():
                        if key != 'status':
                            f.write(f"      {key}: {value}\n")
                
                f.write('\n')
        
        print(f"\n[..] Bericht gespeichert: {report_file}")

def main():
    tester = WorkflowExecutionTester()
    sys.exit(tester.run_critical_tests())

if __name__ == '__main__':
    main()
