#!/usr/bin/env python3
"""Phase 3: AI-assisted code generation using Ollama"""

import json
import sys
import time
from pathlib import Path
from typing import Dict, List

# Windows UTF-8 fix
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

    def detect_model(self) -> str:
        """Auto-detect available Ollama coding model"""
        try:
            import requests
            resp = requests.get("http://localhost:11434/api/tags", timeout=5)
            models = [m['name'] for m in resp.json().get('models', [])]
            
            for preferred in ['deepseek-coder-v2:16b', 'codellama:latest', 'qwen2.5-coder:1.5b-base']:
                if preferred in models:
                    self.model = preferred
                    return preferred
            
            # Fallback to first available
            if models:
                self.model = models[0]
                return models[0]
        except:
            pass
        
        self.model = 'codellama:latest'
        return self.model

    def check_ollama(self) -> bool:
        """Verify Ollama is running"""
        try:
            import requests
            resp = requests.get("http://localhost:11434/api/tags", timeout=5)
            models = [m['name'] for m in resp.json().get('models', [])]
            
            if self.model not in models:
                print(f"[WARN] Model {self.model} not found. Available: {models}")
                if models:
                    self.detect_model()
                    print(f"[INFO] Using fallback: {self.model}")
            
            return True
        except Exception as e:
            print(f"[ERROR] Ollama not running: {e}")
            return False

    def load_phase2_plan(self) -> Dict:
        """Load Phase 2 planning data"""
        plan_file = Path('ai_working/phase2_batch_results.json')
        with open(plan_file) as f:
            batch = json.load(f)
        
        if self.module not in batch:
            raise ValueError(f"Module {self.module} not in Phase 2 results")
        
        return batch[self.module]

    def generate_code(self, task: Dict) -> str:
        """Generate code for a single task using Ollama"""
        
        prompt = f"""You are a C++ expert. Generate production code for ThemisDB.

MODULE: {self.module}
TASK: {task['id']} ({task['priority']})
GAPS: {task['gap_range']} ({task['gap_count']} items)
EFFORT: {task['effort_hours']:.0f} hours

Requirements:
- Modern C++20 (concepts, ranges, RAII)
- Exception-safe, const-correct
- Doxygen comments
- No raw new/delete
- Include unit tests

Output ONLY code, no explanations."""

        try:
            import requests
            
            response = requests.post(
                "http://localhost:11434/api/generate",
                json={
                    'model': self.model,
                    'prompt': prompt,
                    'stream': False,
                    'temperature': 0.2,
                    'num_predict': 2048
                },
                timeout=120
            )
            
            if response.status_code == 200:
                return response.json()['response']
            else:
                return f"// HTTP {response.status_code}: {response.text}"
        except Exception as e:
            return f"// ERROR: {e}"

    def validate_code(self, code: str) -> Dict:
        """Validate generated code syntax"""
        return {
            'syntax_ok': code.count('{') == code.count('}'),
            'has_comments': '///' in code or '/**' in code,
            'is_cpp': '#include' in code,
            'warnings': [] if code.count('{') == code.count('}') else ["Unbalanced braces"]
        }

    def execute(self):
        """Run Phase 3 code generation"""
        
        print("=" * 80)
        print(f"PHASE 3 CODE GENERATION - {self.module.upper()}")
        print("=" * 80)
        
        # Check Ollama
        print(f"\n[*] Checking Ollama...")
        if not self.check_ollama():
            print("[ERROR] Ollama not available. Start with: ollama serve")
            return
        
        print(f"[OK] Ollama running with model: {self.model}")
        
        # Load phase 2 plan
        print(f"\n[*] Loading Phase 2 plan...")
        plan = self.load_phase2_plan()
        
        print(f"[OK] Module: {self.module}")
        print(f"     Total gaps: {plan['total_gaps']:,}")
        print(f"     Effort: {plan['effort_estimate']['total_hours']:.0f} hours")
        print(f"     Tasks: {len(plan['task_breakdown'])}")
        
        # Generate code for first N tasks
        tasks = plan['task_breakdown'][:self.max_tasks]
        
        print(f"\n[*] Generating code for {len(tasks)} tasks...")
        print()
        
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
                'has_comments': validation['has_comments'],
                'code': code[:500]  # First 500 chars preview
            })
        
        elapsed = time.time() - start
        
        # Save results
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
        
        # Summary
        print(f"""
{"=" * 80}
PHASE 3 COMPLETE
{"=" * 80}

Module:           {self.module}
Model:            {self.model}
Execution Time:   {elapsed:.1f} seconds
Tasks Generated:  {len(results)}
Syntax Valid:     {output['syntax_ok']}/{len(results)}
Output File:      {output_file}

Next Steps:
  1. Review: ai_working/phase3_{self.module}_results.json
  2. Copilot review + refinement
  3. Patch generation (Phase 4)
  4. Build + test

STATUS: READY
""")

if __name__ == '__main__':
    gen = Phase3CodeGen(module='index', max_tasks=5)
    gen.execute()
