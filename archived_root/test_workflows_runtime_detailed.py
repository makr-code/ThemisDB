#!/usr/bin/env python3
"""
GitHub Actions Workflow Runtime Test Suite
Testet Workflows lokal mit act und detaillierter Validierung
"""

import os
import sys
import json
import yaml
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple

class WorkflowRuntimeTester:
    def __init__(self, workflow_dir: str = '.github/workflows'):
        self.workflow_dir = Path(workflow_dir)
        self.results = []
        self.start_time = datetime.now()
        
    def log(self, level: str, message: str):
        """Schreibe Testmeldung"""
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
        
    def run_command(self, cmd: List[str], timeout: int = 30) -> Tuple[int, str, str]:
        """Führe Shell-Kommando aus"""
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", f"Timeout nach {timeout}s"
        except Exception as e:
            return -1, "", str(e)
    
    def test_yaml_syntax(self, workflow_path: Path) -> Tuple[bool, str]:
        """Validiere YAML-Syntax"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                yaml.safe_load(f)
            return True, "YAML-Syntax korrekt"
        except yaml.YAMLError as e:
            return False, f"YAML-Fehler: {str(e)[:100]}"
        except Exception as e:
            return False, f"Fehler beim Laden: {str(e)[:100]}"
    
    def test_act_list(self, workflow_path: Path) -> Tuple[bool, List[str], str]:
        """Extrahiere Jobs mit act list"""
        code, stdout, stderr = self.run_command(
            ['act', 'list', '-W', str(workflow_path)],
            timeout=10
        )
        
        if code != 0:
            return False, [], f"act list fehlgeschlagen: {stderr[:100]}"
        
        # Parse Job-Namen aus stdout
        jobs = []
        for line in stdout.split('\n'):
            line = line.strip()
            if line and not line.startswith('ID') and not line.startswith('---'):
                parts = line.split()
                if parts:
                    jobs.append(parts[0])
        
        return True, jobs, f"{len(jobs)} Jobs gefunden"
    
    def test_act_dry_run(self, workflow_path: Path) -> Tuple[bool, str]:
        """Führe act dry-run durch"""
        code, stdout, stderr = self.run_command(
            ['act', '--dry-run', '-W', str(workflow_path)],
            timeout=15
        )
        
        if code == 0:
            return True, "Dry-run erfolgreich"
        else:
            # Dry-run kann fehlschlagen, wenn Secrets/Inputs fehlen - das ist ok
            if 'secrets' in stderr.lower() or 'input' in stderr.lower():
                return True, f"Dry-run (mit Secrets-Warnung)"
            return False, f"Dry-run Fehler: {stderr[:100]}"
    
    def test_workflow_triggers(self, workflow_path: Path) -> Tuple[bool, List[str], str]:
        """Extrahiere Trigger aus Workflow"""
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
            
            return True, triggers, f"Trigger: {', '.join(str(t) for t in triggers) if triggers else 'keine'}"
        except Exception as e:
            return False, [], str(e)[:100]
    
    def test_workflow_jobs(self, workflow_path: Path) -> Tuple[bool, Dict, str]:
        """Extrahiere Job-Struktur"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            jobs = data.get('jobs', {})
            job_info = {}
            
            for job_name, job_config in jobs.items():
                steps_count = len(job_config.get('steps', [])) if job_config else 0
                job_info[job_name] = {
                    'steps': steps_count,
                    'runs-on': job_config.get('runs-on', 'unknown') if job_config else 'unknown'
                }
            
            return True, job_info, f"{len(jobs)} Jobs mit {sum(j['steps'] for j in job_info.values())} Steps total"
        except Exception as e:
            return False, {}, str(e)[:100]
    
    def test_workflow_permissions(self, workflow_path: Path) -> Tuple[bool, Dict, str]:
        """Prüfe Permissions"""
        try:
            with open(workflow_path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
            
            perms = data.get('permissions', {})
            if isinstance(perms, str):
                return True, {'mode': perms}, f"Permissions: {perms}"
            
            perm_str = ', '.join(f"{k}:{v}" for k, v in perms.items()) if perms else "default"
            return True, perms, f"Permissions: {perm_str if perm_str else 'default'}"
        except Exception as e:
            return False, {}, str(e)[:100]
    
    def test_workflow_file(self, workflow_path: Path) -> Dict:
        """Führe alle Tests für eine Workflow-Datei durch"""
        workflow_name = workflow_path.name
        self.log('INFO', f"Teste: {workflow_name}")
        
        tests = {
            'file': workflow_name,
            'passed': 0,
            'failed': 0,
            'warnings': 0,
            'details': {}
        }
        
        # Test 1: Datei existiert
        if not workflow_path.exists():
            self.log('FAIL', f"  Datei nicht gefunden")
            tests['failed'] += 1
            return tests
        
        # Test 2: YAML-Syntax
        success, msg = self.test_yaml_syntax(workflow_path)
        if success:
            self.log('PASS', f"  ✓ {msg}")
            tests['passed'] += 1
        else:
            self.log('FAIL', f"  ✗ {msg}")
            tests['failed'] += 1
        tests['details']['yaml'] = msg
        
        # Test 3: Job-Parsing mit act
        success, jobs, msg = self.test_act_list(workflow_path)
        if success:
            self.log('PASS', f"  ✓ act-Parsing: {msg}")
            tests['passed'] += 1
            tests['details']['act_jobs'] = jobs
        else:
            self.log('WARN', f"  ⚠ act-Parsing: {msg}")
            tests['warnings'] += 1
        
        # Test 4: Trigger-Validierung
        success, triggers, msg = self.test_workflow_triggers(workflow_path)
        if success:
            self.log('PASS', f"  ✓ {msg}")
            tests['passed'] += 1
            tests['details']['triggers'] = triggers
        else:
            self.log('FAIL', f"  ✗ {msg}")
            tests['failed'] += 1
        
        # Test 5: Job-Struktur
        success, job_info, msg = self.test_workflow_jobs(workflow_path)
        if success:
            self.log('PASS', f"  ✓ Job-Struktur: {msg}")
            tests['passed'] += 1
            tests['details']['jobs'] = job_info
        else:
            self.log('FAIL', f"  ✗ Job-Struktur: {msg}")
            tests['failed'] += 1
        
        # Test 6: Permissions
        success, perms, msg = self.test_workflow_permissions(workflow_path)
        if success:
            self.log('PASS', f"  ✓ {msg}")
            tests['passed'] += 1
            tests['details']['permissions'] = perms
        else:
            self.log('WARN', f"  ⚠ {msg}")
            tests['warnings'] += 1
        
        return tests
    
    def run_all_tests(self) -> int:
        """Führe alle Workflow-Tests durch"""
        self.log('INFO', '=' * 70)
        self.log('INFO', 'GitHub Actions Workflow Runtime Test Suite')
        self.log('INFO', f'Zeitstempel: {self.start_time.strftime("%Y-%m-%d %H:%M:%S")}')
        self.log('INFO', '=' * 70)
        self.log('INFO', '')
        
        if not self.workflow_dir.exists():
            self.log('FAIL', f"Workflow-Verzeichnis nicht gefunden: {self.workflow_dir}")
            return 1
        
        # Sammle alle Workflow-Dateien
        workflow_files = sorted(self.workflow_dir.glob('*.yml'))
        
        if not workflow_files:
            self.log('FAIL', "Keine Workflow-Dateien gefunden")
            return 1
        
        self.log('INFO', f"Gefundene Workflows: {len(workflow_files)}")
        self.log('INFO', '')
        
        # Teste alle Workflows
        test_results = []
        total_passed = 0
        total_failed = 0
        total_warnings = 0
        
        for workflow_path in workflow_files:
            result = self.test_workflow_file(workflow_path)
            test_results.append(result)
            total_passed += result['passed']
            total_failed += result['failed']
            total_warnings += result['warnings']
            self.log('INFO', '')
        
        # Zusammenfassung
        self.log('INFO', '=' * 70)
        self.log('INFO', 'ZUSAMMENFASSUNG')
        self.log('INFO', '-' * 70)
        self.log('PASS', f"Bestanden: {total_passed}")
        self.log('FAIL', f"Fehlgeschlagen: {total_failed}")
        self.log('WARN', f"Warnungen: {total_warnings}")
        self.log('INFO', f"Workflows getestet: {len(workflow_files)}")
        
        duration = (datetime.now() - self.start_time).total_seconds()
        self.log('INFO', f"Dauer: {duration:.2f}s")
        
        if total_failed == 0:
            self.log('PASS', "[OK] ALLE TESTS BESTANDEN")
            status = 0
        else:
            self.log('FAIL', f"[XX] {total_failed} Test(s) fehlgeschlagen")
            status = 1
        
        self.log('INFO', '=' * 70)
        
        # Speichere Report
        self.save_report(test_results)
        
        return status
    
    def save_report(self, test_results: List[Dict]):
        """Speichere detaillierten Report"""
        report_file = Path('WORKFLOW_RUNTIME_TEST_REPORT_DETAILED.txt')
        
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(self.results))
            f.write('\n\n')
            f.write('═' * 70 + '\n')
            f.write('DETAILLIERTE TEST-ERGEBNISSE\n')
            f.write('═' * 70 + '\n\n')
            
            for result in test_results:
                f.write(f"Workflow: {result['file']}\n")
                f.write(f"  Bestanden: {result['passed']}\n")
                f.write(f"  Fehlgeschlagen: {result['failed']}\n")
                f.write(f"  Warnungen: {result['warnings']}\n")
                f.write(f"  Details:\n")
                
                for key, value in result['details'].items():
                    if isinstance(value, dict):
                        f.write(f"    {key}:\n")
                        for k, v in value.items():
                            f.write(f"      - {k}: {v}\n")
                    elif isinstance(value, list):
                        f.write(f"    {key}: {', '.join(value) if value else 'keine'}\n")
                    else:
                        f.write(f"    {key}: {value}\n")
                
                f.write('\n')
        
        print(f"\n📄 Report gespeichert: {report_file}")

def main():
    tester = WorkflowRuntimeTester()
    sys.exit(tester.run_all_tests())

if __name__ == '__main__':
    main()
