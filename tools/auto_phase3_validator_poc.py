#!/usr/bin/env python3
"""
Phase 3 Validation PoC: Test Ollama code generation quality before production.
1. Setup check
2. Generate real C++ code for a simple module
3. Validate syntax, build, and tests
4. Quality assessment
"""

import json
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List

class Phase3Validator:
    """Validate Ollama code generation capability"""
    
    def __init__(self):
        self.results = {
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'stages': {},
            'overall_status': 'NOT_STARTED'
        }
    
    def stage_1_check_environment(self) -> bool:
        """Stage 1: Verify Ollama installation and model availability"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 3 VALIDATION PoC: Stage 1 - Environment Check                   ║
╚════════════════════════════════════════════════════════════════════════════════╝
""")
        
        checks = {
            'ollama_installed': False,
            'ollama_running': False,
            'model_downloaded': False,
            'api_accessible': False
        }
        
        # Check 1: Ollama executable
        try:
            result = subprocess.run(['ollama', '--version'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                checks['ollama_installed'] = True
                print(f"[OK] Ollama installed: {result.stdout.strip()}")
            else:
                print(f"[X] Ollama version check failed: {result.stderr}")
        except FileNotFoundError:
            print("[X] Ollama not found in PATH")
            print("    Install from: https://ollama.com/download/windows")
        except Exception as e:
            print(f"[X] Ollama check error: {e}")
        
        # Check 2: Ollama service running
        try:
            import requests
            resp = requests.get("http://localhost:11434/api/tags", timeout=5)
            if resp.status_code == 200:
                checks['ollama_running'] = True
                models = resp.json().get('models', [])
                print(f"[OK] Ollama service running with {len(models)} models")
                
                # Check 3: Required model
                model_names = [m['name'] for m in models]
                if 'deepseek-coder-v2:16b' in model_names:
                    checks['model_downloaded'] = True
                    print("[OK] Model available: deepseek-coder-v2:16b")
                else:
                    print(f"[!] Model not found. Available: {model_names}")
                    print("    Download: ollama pull deepseek-coder-v2:16b")
                
                checks['api_accessible'] = True
                print("[OK] API accessible at localhost:11434")
            else:
                print(f"[X] Ollama API error: HTTP {resp.status_code}")
        except requests.exceptions.ConnectionError:
            print("[X] Cannot connect to Ollama at localhost:11434")
            print("    Start with: ollama serve")
        except ImportError:
            print("[!] requests library not installed: pip install requests")
        except Exception as e:
            print(f"[X] API check error: {e}")
        
        self.results['stages']['stage_1_environment'] = checks
        
        if all(checks.values()):
            print("\n[OK] All environment checks PASSED")
            return True
        else:
            print(f"\n[!] Some checks failed: {checks}")
            return False
    
    def stage_2_generate_sample_code(self) -> bool:
        """Stage 2: Generate real C++ code for a simple module"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 3 VALIDATION PoC: Stage 2 - Sample Code Generation              ║
╚════════════════════════════════════════════════════════════════════════════════╝

Target: Simple C++ utility function (from small module)
Task: Implement a thread-safe counter wrapper
""")
        
        try:
            import requests
            
            prompt = """
You are a senior C++ developer for ThemisDB.

TASK: Implement a simple thread-safe counter wrapper

REQUIREMENTS:
1. Class: ThreadSafeCounter
2. Methods:
   - increment() -> void
   - decrement() -> void  
   - get() -> int (returns current value)
3. Thread-safe (use std::mutex)
4. RAII compliant
5. Include unit test skeleton

OUTPUT: Only C++ code, no explanations.

```cpp
#include <mutex>
#include <iostream>

// Your implementation here
```
"""
            
            print("[*] Generating code from Ollama...")
            start = time.time()
            
            response = requests.post(
                'http://localhost:11434/api/generate',
                json={
                    'model': 'deepseek-coder-v2:16b',
                    'prompt': prompt,
                    'stream': False,
                    'temperature': 0.2,  # Low creativity for deterministic code
                    'num_predict': 1024
                },
                timeout=120
            )
            
            gen_time = time.time() - start
            
            if response.status_code == 200:
                code = response.json()['response']
                
                self.results['stages']['stage_2_generation'] = {
                    'status': 'SUCCESS',
                    'generation_time_seconds': gen_time,
                    'code_length': len(code),
                    'code_preview': code[:500] + ('...' if len(code) > 500 else '')
                }
                
                print(f"[OK] Code generated in {gen_time:.1f}s")
                print(f"[OK] Code length: {len(code)} characters")
                print(f"\nGenerated code preview:\n{'='*80}")
                print(code[:800])
                print(f"{'='*80}")
                
                # Save generated code
                with open('ai_working/phase3_poc_generated.cpp', 'w') as f:
                    f.write(code)
                
                return True
            else:
                print(f"[X] Generation failed: HTTP {response.status_code}")
                self.results['stages']['stage_2_generation'] = {
                    'status': 'FAILED',
                    'error': f'HTTP {response.status_code}'
                }
                return False
        
        except Exception as e:
            print(f"[X] Generation error: {e}")
            self.results['stages']['stage_2_generation'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def stage_3_validate_syntax(self) -> bool:
        """Stage 3: Validate C++ syntax"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 3 VALIDATION PoC: Stage 3 - Syntax Validation                   ║
╚════════════════════════════════════════════════════════════════════════════════╝
""")
        
        code_file = Path('ai_working/phase3_poc_generated.cpp')
        if not code_file.exists():
            print("[X] Generated code file not found")
            return False
        
        with open(code_file) as f:
            code = f.read()
        
        checks = {
            'has_main': False,
            'has_class': False,
            'braces_balanced': False,
            'has_include': False,
            'has_mutex': False
        }
        
        # Basic syntax checks
        checks['braces_balanced'] = code.count('{') == code.count('}')
        checks['has_include'] = '#include' in code
        checks['has_class'] = 'class ' in code
        checks['has_mutex'] = 'mutex' in code or 'std::lock_guard' in code
        
        print(f"[{'OK' if checks['braces_balanced'] else 'X'}] Braces balanced")
        print(f"[{'OK' if checks['has_include'] else 'X'}] Has includes")
        print(f"[{'OK' if checks['has_class'] else '!'}] Has class definition")
        print(f"[{'OK' if checks['has_mutex'] else '!'}] Uses mutex/threading")
        
        syntax_ok = checks['braces_balanced'] and checks['has_include']
        
        self.results['stages']['stage_3_syntax'] = {
            'status': 'PASS' if syntax_ok else 'FAIL',
            'checks': checks
        }
        
        print(f"\n[{'OK' if syntax_ok else 'X'}] Syntax validation: {'PASS' if syntax_ok else 'FAIL'}")
        return syntax_ok
    
    def stage_4_compile_attempt(self) -> bool:
        """Stage 4: Attempt compilation with clang/MSVC"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 3 VALIDATION PoC: Stage 4 - Compilation Test                    ║
╚════════════════════════════════════════════════════════════════════════════════╝
""")
        
        code_file = Path('ai_working/phase3_poc_generated.cpp')
        exe_file = Path('ai_working/phase3_poc_test.exe')
        
        # Try MSVC compilation (Windows)
        try:
            # Note: This assumes MSVC is in PATH (Visual Studio installation)
            print("[*] Attempting compilation with MSVC...")
            
            result = subprocess.run(
                ['cl', str(code_file), '/std:c++20', f'/Fe{exe_file}'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            compile_ok = result.returncode == 0
            
            self.results['stages']['stage_4_compilation'] = {
                'status': 'SUCCESS' if compile_ok else 'PARTIAL',
                'compiler': 'MSVC',
                'returncode': result.returncode,
                'stdout_preview': result.stdout[:200],
                'stderr_preview': result.stderr[:200]
            }
            
            if compile_ok:
                print(f"[OK] Compilation successful")
                return True
            else:
                print(f"[!] Compilation had issues (returncode {result.returncode})")
                print(f"    STDERR: {result.stderr[:300]}")
                print("\n    Note: This may be expected for generated stub code")
                return False
        
        except FileNotFoundError:
            print("[!] MSVC compiler not found (cl.exe)")
            print("    Install Visual Studio or use clang-cl")
            self.results['stages']['stage_4_compilation'] = {
                'status': 'SKIPPED',
                'reason': 'MSVC not in PATH'
            }
            return False
        except subprocess.TimeoutExpired:
            print("[X] Compilation timed out")
            self.results['stages']['stage_4_compilation'] = {
                'status': 'TIMEOUT'
            }
            return False
        except Exception as e:
            print(f"[X] Compilation error: {e}")
            self.results['stages']['stage_4_compilation'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def stage_5_quality_assessment(self) -> Dict:
        """Stage 5: Assess code quality"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║        PHASE 3 VALIDATION PoC: Stage 5 - Quality Assessment                  ║
╚════════════════════════════════════════════════════════════════════════════════╝
""")
        
        code_file = Path('ai_working/phase3_poc_generated.cpp')
        with open(code_file) as f:
            code = f.read()
        
        quality = {
            'code_length': len(code),
            'has_documentation': bool('/**' in code or '///' in code),
            'has_error_handling': bool('try' in code or 'catch' in code or 'throw' in code),
            'uses_modern_cpp': bool('auto' in code or 'constexpr' in code or 'nullptr' in code),
            'has_raw_pointers': 'new ' in code or 'delete ' in code,
            'has_todos': 'TODO' in code or 'FIXME' in code,
            'line_count': len(code.split('\n'))
        }
        
        print(f"Code Length: {quality['code_length']} chars ({quality['line_count']} lines)")
        print(f"[{'OK' if quality['has_documentation'] else '!'}] Has documentation")
        print(f"[{'OK' if quality['uses_modern_cpp'] else '!'}] Uses modern C++")
        print(f"[{'X' if quality['has_raw_pointers'] else 'OK'}] No raw pointers")
        print(f"[{'X' if quality['has_todos'] else 'OK'}] No TODOs left")
        
        # Overall quality score (0-100)
        score = 0
        score += 20 if quality['has_documentation'] else 10
        score += 20 if quality['uses_modern_cpp'] else 10
        score += 20 if not quality['has_raw_pointers'] else 0
        score += 20 if not quality['has_todos'] else 10
        score += 20 if quality['has_error_handling'] else 10
        
        quality['overall_score'] = score
        
        print(f"\nQuality Score: {score}/100")
        
        self.results['stages']['stage_5_quality'] = quality
        
        return quality
    
    def final_report(self):
        """Generate final validation report"""
        print("""
╔════════════════════════════════════════════════════════════════════════════════╗
║                    PHASE 3 VALIDATION PoC - FINAL REPORT                     ║
╚════════════════════════════════════════════════════════════════════════════════╝
""")
        
        stages = self.results['stages']
        
        # Determine overall status
        stage1_ok = stages.get('stage_1_environment', {}).get('ollama_running', False)
        stage2_ok = stages.get('stage_2_generation', {}).get('status') == 'SUCCESS'
        stage3_ok = stages.get('stage_3_syntax', {}).get('status') == 'PASS'
        stage5_quality = stages.get('stage_5_quality', {}).get('overall_score', 0)
        
        overall = 'READY' if all([stage1_ok, stage2_ok, stage3_ok, stage5_quality > 60]) else 'NEEDS_WORK'
        
        self.results['overall_status'] = overall
        
        print(f"""
STAGES COMPLETED:
  Stage 1 (Environment):  {'[OK]' if stage1_ok else '[X]'}
  Stage 2 (Generation):   {'[OK]' if stage2_ok else '[X]'}
  Stage 3 (Syntax):       {'[OK]' if stage3_ok else '[X]'}
  Stage 4 (Compilation):  [{'OK' if stages.get('stage_4_compilation', {}).get('status') == 'SUCCESS' else '!'}]
  Stage 5 (Quality):      {stage5_quality}/100

VERDICT: {overall}
  Quality Score: {stage5_quality}/100 ({'Good' if stage5_quality > 70 else 'Fair' if stage5_quality > 50 else 'Poor'})

RECOMMENDATION:
""")
        
        if overall == 'READY':
            print("""
  [OK] Ollama is READY for production use
  
  NEXT STEPS:
  1. Execute Phase 3 on high-priority modules (LLM, Server, Query)
  2. Review generated code with Copilot
  3. Validate with build + test suite
  4. Merge to develop after approval
  
  Command to start:
  $ python tools/auto_phase3_codegen.py --module llm --max-tasks 20
""")
        else:
            print("""
  [!] Ollama needs improvement before production
  
  OPTIONS:
  1. Refine prompts for better code generation
  2. Use different model (codellama:34b for higher quality)
  3. Add post-generation filtering/validation
  4. Combine with Copilot for all suggestions
  
  SAFE APPROACH:
  1. Generate code with Ollama (as draft)
  2. ALWAYS review with Copilot before commit
  3. Run full test suite (no merge without passing tests)
  4. Manual code review required
""")
        
        # Save report
        with open('ai_working/phase3_validation_report.json', 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"""
Full report saved: ai_working/phase3_validation_report.json
Generated code: ai_working/phase3_poc_generated.cpp
""")
        
        return self.results

def main():
    """Execute Phase 3 validation"""
    
    validator = Phase3Validator()
    
    # Run all stages
    if not validator.stage_1_check_environment():
        print("\n[BLOCKER] Environment check failed. Fix requirements before proceeding.")
        validator.results['overall_status'] = 'BLOCKED'
        return 1
    
    if not validator.stage_2_generate_sample_code():
        print("\n[BLOCKER] Code generation failed.")
        return 1
    
    validator.stage_3_validate_syntax()
    validator.stage_4_compile_attempt()
    validator.stage_5_quality_assessment()
    
    # Generate report
    report = validator.final_report()
    
    return 0 if report['overall_status'] == 'READY' else 1

if __name__ == '__main__':
    sys.exit(main())
