#!/usr/bin/env python3
"""Phase 3: AI-assisted code generation - STORAGE Module"""

import json
import sys
import time
from pathlib import Path
from typing import Dict

if sys.platform.startswith('win'):
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        sys.stderr.reconfigure(encoding='utf-8', errors='replace')
    except:
        pass

class Phase3CodeGen:
    def __init__(self, module: str, max_tasks: int = 5):
        self.module = module
        self.max_tasks = max_tasks
        self.model = None
        self.detect_model()

    def detect_model(self):
        try:
            import requests
            resp = requests.get("http://localhost:11434/api/tags", timeout=5)
            models = [m['name'] for m in resp.json().get('models', [])]
            for preferred in ['deepseek-coder-v2:16b', 'codellama:latest', 'qwen2.5-coder:1.5b-base']:
                if preferred in models:
                    self.model = preferred
                    return
            self.model = models[0] if models else 'codellama:latest'
        except:
            self.model = 'codellama:latest'

    def check_ollama(self) -> bool:
        try:
            import requests
            requests.get("http://localhost:11434/api/tags", timeout=5)
            return True
        except:
            print(f"[ERROR] Ollama not running")
            return False

    def load_phase2_plan(self) -> Dict:
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        return batch[self.module]

    def generate_code(self, task: Dict) -> str:
        try:
            import requests
            response = requests.post(
                "http://localhost:11434/api/generate",
                json={
                    'model': self.model,
                    'prompt': f"""Generate C++20 production code for ThemisDB.

MODULE: {self.module}
TASK: {task['id']} ({task['priority']})
GAPS: {task['gap_range']}
EFFORT: {task['effort_hours']:.0f}h

Requirements:
- Modern C++20
- Exception-safe
- RAII patterns
- Doxygen docs
- No raw new/delete

Output ONLY code.""",
                    'stream': False,
                    'temperature': 0.2,
                    'num_predict': 2048
                },
                timeout=120
            )
            return response.json()['response'] if response.status_code == 200 else f"// ERROR"
        except Exception as e:
            return f"// ERROR: {e}"

    def validate_code(self, code: str) -> Dict:
        return {
            'syntax_ok': code.count('{') == code.count('}'),
            'has_comments': '///' in code or '/**' in code,
            'is_cpp': '#include' in code,
        }

    def execute(self):
        print("=" * 80)
        print(f"PHASE 3 CODE GENERATION - {self.module.upper()}")
        print("=" * 80)
        
        if not self.check_ollama():
            return
        
        print(f"\n[OK] Ollama: {self.model}")
        
        plan = self.load_phase2_plan()
        print(f"\n[*] Module: {self.module}")
        print(f"    Gaps: {plan['total_gaps']:,} | Effort: {plan['effort_estimate']['total_hours']:.0f}h | Tasks: {len(plan['task_breakdown'])}")
        
        tasks = plan['task_breakdown'][:self.max_tasks]
        print(f"\n[*] Generating code for {len(tasks)} tasks...\n")
        
        results = []
        start = time.time()
        
        for idx, task in enumerate(tasks, 1):
            print(f"  {idx:2d}. {task['id']:8s} ({task['priority']:8s}) ", end='', flush=True)
            code = self.generate_code(task)
            validation = self.validate_code(code)
            status = "OK" if validation['syntax_ok'] else "!"
            print(f"[{status}] ({len(code)} chars)")
            
            results.append({
                'task_id': task['id'],
                'priority': task['priority'],
                'effort_hours': task['effort_hours'],
                'code_length': len(code),
                'syntax_ok': validation['syntax_ok'],
                'code': code[:500]
            })
        
        elapsed = time.time() - start
        
        output = {
            'module': self.module,
            'model': self.model,
            'execution_time': elapsed,
            'tasks_total': len(tasks),
            'tasks_generated': len(results),
            'syntax_ok': sum(1 for r in results if r['syntax_ok']),
            'results': results
        }
        
        output_file = Path(f'ai_working/phase3_{self.module}_results.json')
        with open(output_file, 'w') as f:
            json.dump(output, f, indent=2)
        
        print(f"""
{"=" * 80}
PHASE 3 COMPLETE - {self.module.upper()}
{"=" * 80}

Tasks Generated:  {len(results)}
Syntax Valid:     {output['syntax_ok']}/{len(results)}
Time:             {elapsed:.1f}s
Output:           {output_file}

STATUS: READY
""")

if __name__ == '__main__':
    gen = Phase3CodeGen(module='storage', max_tasks=5)
    gen.execute()
