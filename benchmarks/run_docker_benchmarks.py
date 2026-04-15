"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_docker_benchmarks.py                           ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     216                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Benchmark Runner für ThemisDB Docker
Wartet auf Build-Abschluss und führt dann Benchmarks aus
"""

import subprocess
import time
import sys
import json
from datetime import datetime

def wait_for_docker_build(timeout=3600):
    """Warte auf Docker Build Abschluss"""
    print("Warte auf Docker Build-Abschluss...")
    start_time = time.time()
    
    while time.time() - start_time < timeout:
        try:
            # Prüfe ob Image existiert
            result = subprocess.run(
                ['docker', 'images', '-q', 'themisdb/themisdb:latest'],
                capture_output=True,
                text=True
            )
            
            if result.stdout.strip():
                print(f"✓ Docker Image gefunden: themisdb/themisdb:latest")
                return True
            
            time.sleep(10)
            elapsed = int(time.time() - start_time)
            print(f"  Warte... ({elapsed}s / {timeout}s)")
            
        except Exception as e:
            print(f"Fehler beim Prüfen: {e}")
            time.sleep(10)
    
    print("✗ Timeout beim Warten auf Docker Build")
    return False

def start_themis_container():
    """Starte ThemisDB Container"""
    print("\nStarte ThemisDB Container...")
    
    try:
        # Stoppe alten Container
        subprocess.run(['docker-compose', '-f', 'docker-compose.benchmark.yml', 'down'],
                      capture_output=True)
        
        # Starte neuen Container
        result = subprocess.run(
            ['docker-compose', '-f', 'docker-compose.benchmark.yml', 'up', '-d'],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"✗ Fehler beim Starten: {result.stderr}")
            return False
        
        # Warte auf Health Check
        print("Warte auf Health Check...")
        for i in range(30):
            result = subprocess.run(
                ['docker', 'exec', 'themis-benchmark', 'curl', '-f', 'http://localhost:8765/health'],
                capture_output=True
            )
            if result.returncode == 0:
                print(f"✓ ThemisDB ist bereit!")
                return True
            time.sleep(2)
            print(f"  Warte... ({i*2}s)")
        
        print("✗ Health Check Timeout")
        return False
        
    except Exception as e:
        print(f"✗ Fehler: {e}")
        return False

def run_benchmarks():
    """Führe Benchmarks aus"""
    print("\n" + "="*80)
    print("STARTE BENCHMARKS")
    print("="*80)
    
    benchmarks = [
        {
            'name': 'Docker Comparative Benchmarks',
            'script': 'docker_benchmarks_unified.py',
            'args': ['--workload', 'all', '--duration', '180']
        },
        {
            'name': 'CRUD Performance Test',
            'script': 'comprehensive_crud_benchmark.py',
            'args': ['--host', 'localhost', '--port', '8765', '--duration', '60', '--protocols', 'http']
        }
    ]
    
    results = {}
    
    for bench in benchmarks:
        print(f"\n{'='*80}")
        print(f"Benchmark: {bench['name']}")
        print(f"{'='*80}")
        
        cmd = ['python', bench['script']] + bench['args']
        
        try:
            start = time.time()
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=600
            )
            duration = time.time() - start
            
            results[bench['name']] = {
                'success': result.returncode == 0,
                'duration': duration,
                'stdout': result.stdout[-1000:] if len(result.stdout) > 1000 else result.stdout,
                'stderr': result.stderr[-500:] if len(result.stderr) > 500 else result.stderr
            }
            
            if result.returncode == 0:
                print(f"✓ Erfolgreich ({duration:.1f}s)")
            else:
                print(f"✗ Fehlgeschlagen ({duration:.1f}s)")
                print(f"Fehler: {result.stderr[-200:]}")
                
        except subprocess.TimeoutExpired:
            print(f"✗ Timeout nach 600s")
            results[bench['name']] = {'success': False, 'error': 'timeout'}
        except Exception as e:
            print(f"✗ Fehler: {e}")
            results[bench['name']] = {'success': False, 'error': str(e)}
    
    # Speichere Ergebnisse
    results_file = f"benchmark_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✓ Ergebnisse gespeichert: {results_file}")
    
    # Zusammenfassung
    print("\n" + "="*80)
    print("ZUSAMMENFASSUNG")
    print("="*80)
    
    for name, result in results.items():
        status = "✓ ERFOLG" if result['success'] else "✗ FEHLER"
        print(f"{status}: {name}")
    
    return results

def main():
    print("="*80)
    print("ThemisDB Benchmark Automation")
    print("="*80)
    print(f"Start: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Schritt 1: Warte auf Build
    if not wait_for_docker_build():
        print("\n✗ Build nicht rechtzeitig abgeschlossen")
        sys.exit(1)
    
    # Schritt 2: Starte Container
    if not start_themis_container():
        print("\n✗ Container konnte nicht gestartet werden")
        sys.exit(1)
    
    # Schritt 3: Führe Benchmarks aus
    results = run_benchmarks()
    
    # Schritt 4: Cleanup
    print("\nStoppe Container...")
    subprocess.run(['docker-compose', '-f', 'docker-compose.benchmark.yml', 'down'],
                  capture_output=True)
    
    # Erfolg prüfen
    success_count = sum(1 for r in results.values() if r['success'])
    total_count = len(results)
    
    print(f"\n{'='*80}")
    print(f"FERTIG: {success_count}/{total_count} Benchmarks erfolgreich")
    print(f"Ende: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"{'='*80}")
    
    sys.exit(0 if success_count == total_count else 1)

if __name__ == '__main__':
    main()
