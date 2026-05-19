#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Phase 3 Code Generation: AI-assisted implementation from Phase 2 task breakdown.
Uses Ollama for local code generation + validation.
"""

import sys
import json
import argparse
import subprocess
import time
from pathlib import Path
from typing import List, Dict

# Force UTF-8 output on Windows
if sys.platform.startswith('win'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

class Phase3CodeGen:
    """AI-assisted code generation from Phase 2 plans"""
    
    def __init__(self, module: str, max_tasks: int = 10, model: str = "deepseek-coder-v2:16b"):
        self.module = module
        self.max_tasks = max_tasks
        self.model = model
        self.ollama_endpoint = "http://localhost:11434/api/generate"
        self.results = []
        
    def check_ollama(self) -> bool:
        """Verify Ollama is running and model is available"""
        try:
            import requests
            resp = requests.get("http://localhost:11434/api/tags", timeout=5)
            models = [m['name'] for m in resp.json().get('models', [])]
            if self.model in models:
                print(f"[OK] Ollama found with model: {self.model}")
                return True
            else:
                print(f"[ERROR] Model {self.model} not found. Available: {models}")
                print(f"       Pull it with: ollama pull {self.model}")
                return False
        except Exception as e:
            print(f"[ERROR] Ollama not running: {e}")
            print(f"       Start with: ollama serve")
            return False
    
    def load_phase2_plan(self) -> Dict:
        """Load Phase 2 plan from batch results"""
        plan_file = Path('ai_working/phase2_batch_results.json')
        if not plan_file.exists():
            raise FileNotFoundError(f"Phase 2 results not found: {plan_file}")
        
        with open(plan_file) as f:
            batch = json.load(f)
        
        if self.module not in batch:
            raise ValueError(f"Module {self.module} not in Phase 2 results")
        
        return batch[self.module]
    
    def extract_tasks(self, plan: Dict) -> List[Dict]:
        """Extract implementation tasks from Phase 2 plan"""
        tasks = plan.get('task_breakdown', [])
        
        # Limit to max_tasks for PoC
        limited_tasks = tasks[:self.max_tasks]
        
        print(f"\n[*] Extracted {len(limited_tasks)}/{len(tasks)} tasks for {self.module}")
        return limited_tasks
    
    def generate_code_for_task(self, task: Dict) -> Dict:
        """Generate code for a single task using Ollama"""
        
        prompt = f"""
You are a senior C++ developer for ThemisDB. Generate production-quality C++ code.

MODULE: {self.module}
TASK: {task['id']} - {task['priority']} priority
GAP RANGE: {task['gap_range']} ({task['gap_count']} gaps)
ESTIMATED EFFORT: {task['effort_hours']} hours

REQUIREMENTS:
1. Modern C++20 code (concepts, ranges)
2. RAII for resource management
3. Exception-safe
4. Const-correct
5. No raw new/delete
6. Thread-safe if needed
7. Include unit tests
8. Doxygen comments

OUTPUT:
- Function signature
- Implementation
- Unit test stub
- API documentation

Generate ONLY code. No explanations.
"""

        try:
            import requests
            
            response = requests.post(
                self.ollama_endpoint,
                json={{
                    'model': self.model,
                    'prompt': prompt,
                    'stream': False,
                    'temperature': 0.3,  # Low for deterministic code
                    'num_predict': 2048  # Max tokens
                }},
                timeout=120
            )
            
            if response.status_code == 200:
                code = response.json()['response']
                return {
                    'task_id': task['id'],
                    'status': 'GENERATED',
                    'code': code,
                    'effort_hours': task['effort_hours']
                }
            else:
                return {
                    'task_id': task['id'],
                    'status': 'FAILED',
                    'error': f"HTTP {response.status_code}",
                    'effort_hours': task['effort_hours']
                }
        
        except Exception as e:
            return {
                'task_id': task['id'],
                'status': 'ERROR',
                'error': str(e),
                'effort_hours': task['effort_hours']
            }
    
    def validate_code(self, code: str) -> Dict:
        """Validate generated code"""
        results = {
            'syntax_ok': False,
            'warnings': []
        }
        
        # Check for basic C++ syntax
        if code.count('{') != code.count('}'):
            results['warnings'].append("Mismatched braces")
        
        if 'TODO' in code:
            results['warnings'].append("Contains TODO comments")
        
        if 'TODO' not in code and code.count('{') == code.count('}'):
            results['syntax_ok'] = True
        
        return results
    
    def execute(self) -> Dict:
        """Execute Phase 3 code generation"""
        
        print(f"""
================================================================================
        PHASE 3 CODE GENERATION: {self.module.upper():30s}
================================================================================
""")
        
        # Check Ollama
        if not self.check_ollama():
            return {
                'module': self.module,
                'status': 'OLLAMA_NOT_READY',
                'tasks_generated': 0,
                'tasks_validated': 0
            }
        
        # Load Phase 2 plan
        print(f"\n[*] Loading Phase 2 plan for {self.module}...")
        plan = self.load_phase2_plan()
        
        print(f"    - Total gaps: {plan['total_gaps']:,}")
        print(f"    - CRITICAL: {plan['gap_distribution']['CRITICAL']}")
        print(f"    - HIGH: {plan['gap_distribution']['HIGH']}")
        print(f"    - MEDIUM: {plan['gap_distribution']['MEDIUM']}")
        print(f"    - Estimated effort: {plan['effort_estimate']['total_hours']:.0f} hours")
        
        # Extract tasks
        tasks = self.extract_tasks(plan)
        
        # Generate code for each task
        print(f"\n[*] Generating code ({len(tasks)} tasks)...")
        print()
        
        start_time = time.time()
        generated_count = 0
        validated_count = 0
        
        for idx, task in enumerate(tasks, 1):
            result = self.generate_code_for_task(task)
            self.results.append(result)
            
            if result['status'] == 'GENERATED':
                # Validate code
                validation = self.validate_code(result['code'])
                result['validation'] = validation
                
                if validation['syntax_ok']:
                    validated_count += 1
                
                generated_count += 1
                status = "[OK]" if validation['syntax_ok'] else "[!]"
            else:
                status = "[X]"
            
            print(f"    {idx:2d}. {task['id']:8s} {task['priority']:8s} {status} ({task['effort_hours']:.0f}h)")
        
        execution_time = time.time() - start_time
        
        # Save results
        output = {
            'module': self.module,
            'execution_time': execution_time,
            'tasks_total': len(tasks),
            'tasks_generated': generated_count,
            'tasks_validated': validated_count,
            'success_rate': generated_count / len(tasks) if tasks else 0,
            'results': self.results
        }
        
        output_file = Path(f'ai_working/phase3_{self.module}_codegen.json')
        with open(output_file, 'w') as f:
            json.dump(output, f, indent=2)
        
        # Print summary
        print(f"""

{"="*80}
                       PHASE 3 GENERATION COMPLETE
{"="*80}

Module:          {self.module}
Execution Time:  {execution_time:.1f} seconds
Tasks Generated: {generated_count}/{len(tasks)} ({100*generated_count/len(tasks):.0f}% success)
Tasks Validated: {validated_count}/{generated_count}
Success Rate:    {100*output['success_rate']:.0f}%

Output: {output_file}

NEXT STEPS:
  1. Review generated code: ai_working/phase3_{self.module}_codegen.json
  2. Refine with Copilot feedback
  3. Create patches for testing
  4. Run Phase 4 validation: cmake --build + ctest

STATUS: Ready for Phase 4 Testing
""")

        
        return output

def main():
    parser = argparse.ArgumentParser(
        description='Phase 3: AI-Assisted Code Generation'
    )
    parser.add_argument('--module', default='llm', help='Module to generate code for')
    parser.add_argument('--max-tasks', type=int, default=10, help='Max tasks to generate')
    parser.add_argument('--model', default='deepseek-coder-v2:16b', help='Ollama model')
    parser.add_argument('--check-ollama', action='store_true', help='Check Ollama status only')
    
    args = parser.parse_args()
    
    codegen = Phase3CodeGen(
        module=args.module,
        max_tasks=args.max_tasks,
        model=args.model
    )
    
    if args.check_ollama:
        codegen.check_ollama()
        return
    
    codegen.execute()

if __name__ == '__main__':
    main()
