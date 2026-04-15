"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quickstart.py                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Quick Start Helper für Multi-Shard RAID Benchmarks

Hilft beim Starten der Cluster und Benchmarks.
"""

import subprocess
import sys
import os
from pathlib import Path

class BenchmarkHelper:
    """Hilfsfunktion für Benchmark-Start"""
    
    def __init__(self):
        self.benchmarks_dir = Path(__file__).parent
        os.chdir(self.benchmarks_dir)
    
    def run_command(self, cmd, description=""):
        """Führt einen Befehl aus"""
        if description:
            print(f"\n{'='*60}")
            print(f"[*] {description}")
            print(f"{'='*60}")
        
        print(f"  Befehl: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
        
        try:
            if isinstance(cmd, list):
                result = subprocess.run(cmd, cwd=str(self.benchmarks_dir))
            else:
                result = subprocess.run(cmd, shell=True, cwd=str(self.benchmarks_dir))
            
            return result.returncode == 0
        except Exception as e:
            print(f"  ✗ Fehler: {e}")
            return False
    
    def start_cluster(self, num_shards=3):
        """Startet einen Docker Cluster"""
        profile = f"{num_shards}-shards"
        
        cmd = [
            "docker-compose",
            "-f", "docker-compose.multi-shard-raid.yml",
            "--profile", profile,
            "up", "-d"
        ]
        
        return self.run_command(cmd, f"Starte Cluster mit {num_shards} Shards")
    
    def stop_cluster(self, num_shards=3):
        """Stoppt einen Docker Cluster"""
        profile = f"{num_shards}-shards"
        
        cmd = [
            "docker-compose",
            "-f", "docker-compose.multi-shard-raid.yml",
            "--profile", profile,
            "down", "-v"
        ]
        
        return self.run_command(cmd, f"Stoppe Cluster")
    
    def check_health(self, num_shards=3):
        """Überprüft die Gesundheit der Shards"""
        print(f"\n[*] Health Check ({num_shards} Shards):")
        
        healthy = 0
        for i in range(num_shards):
            port = 8080 + i
            cmd = f"curl -s http://localhost:{port}/health"
            
            try:
                result = subprocess.run(cmd, shell=True, capture_output=True, timeout=5)
                if result.returncode == 0:
                    print(f"  ✓ Shard {i} (Port {port}): OK")
                    healthy += 1
                else:
                    print(f"  ✗ Shard {i} (Port {port}): Not Ready")
            except Exception as e:
                print(f"  ✗ Shard {i} (Port {port}): Error - {e}")
        
        print(f"  → {healthy}/{num_shards} Shards healthy")
        return healthy == num_shards
    
    def show_menu(self):
        """Zeigt Menü"""
        print("""
╔════════════════════════════════════════════════════════════════════════════╗
║    ThemisDB Multi-Shard RAID Benchmark - Quick Start Helper                ║
╚════════════════════════════════════════════════════════════════════════════╝

SZENARIEN:
  1. S1 - Baseline (RAID0, 3 Shards, OLTP) → 1 Stunde
  2. S3 - Balanced (RAID5, 3 Shards, OLTP) → 2 Stunden
  3. S4 - Production (RAID10, 6 Shards, Mixed) → 12 Stunden
  4. S5 - Data Warehouse (RAID6, 12 Shards, OLAP) → 18 Stunden

MANAGEMENT:
  5. Cluster starten (3 Shards)
  6. Cluster starten (6 Shards)
  7. Cluster starten (12 Shards)
  8. Cluster stoppen
  9. Health Check
 10. Infrastruktur validieren

ANDERE:
  0. Beenden

Wähle eine Option [0-10]:
        """)

def main():
    """Hauptfunktion"""
    helper = BenchmarkHelper()
    
    while True:
        helper.show_menu()
        choice = input(">> ").strip()
        
        if choice == "0":
            print("Auf Wiedersehen!")
            sys.exit(0)
        
        elif choice == "1":
            cmd = [
                sys.executable, "run_multi_shard_raid_benchmark.py",
                "--scenario", "S1",
                "--shards", "3",
                "--raid", "RAID10",
                "--workload", "OLTP",
                "--duration", "3600"
            ]
            helper.run_command(cmd, "Starte S1 Benchmark (1 Stunde)")
        
        elif choice == "5":
            if helper.start_cluster(3):
                print("✓ Cluster gestartet")
                input("Drücke Enter um fortzufahren...")
            else:
                print("✗ Fehler beim Starten des Clusters")
        
        elif choice == "8":
            helper.stop_cluster(3)
            print("✓ Cluster gestoppt")
        
        elif choice == "9":
            helper.check_health(3)
            input("Drücke Enter um fortzufahren...")
        
        else:
            print("✗ Ungültige Auswahl")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nAbgebrochen durch Benutzer.")
        sys.exit(0)
    except Exception as e:
        print(f"Fehler: {e}", file=sys.stderr)
        sys.exit(1)
