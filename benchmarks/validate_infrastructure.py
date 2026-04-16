"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_infrastructure.py                         ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Multi-Shard RAID Benchmark - Infrastructure Validator

Überprüft alle notwendigen Komponenten für Benchmark-Ausführung.
"""

import sys
import subprocess
import json
from pathlib import Path
from typing import Tuple, List

class BenchmarkValidator:
    """Validiert Benchmark-Infrastruktur"""
    
    def __init__(self):
        self.passed_checks = 0
        self.failed_checks = 0
        self.warnings = []
        self.benchmarks_dir = Path(__file__).parent
    
    def print_header(self):
        """Druckt Kopfzeile"""
        print("""
╔════════════════════════════════════════════════════════════════════════════╗
║    ThemisDB Multi-Shard RAID Benchmark - Infrastructure Validation         ║
╚════════════════════════════════════════════════════════════════════════════╝
        """)
    
    def check(self, name: str, condition: bool, details: str = "") -> bool:
        """Führt einen Check durch und protokolliert das Ergebnis"""
        if condition:
            print(f"✓ {name}")
            if details:
                print(f"  → {details}")
            self.passed_checks += 1
            return True
        else:
            print(f"✗ {name}")
            if details:
                print(f"  → {details}")
            self.failed_checks += 1
            return False
    
    def warning(self, message: str):
        """Protokolliert eine Warnung"""
        print(f"⚠ {message}")
        self.warnings.append(message)
    
    def run_command(self, cmd: List[str]) -> Tuple[bool, str]:
        """Führt einen Befehl aus und gibt Erfolg und Ausgabe zurück"""
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            return result.returncode == 0, result.stdout.strip()
        except subprocess.TimeoutExpired:
            return False, "Timeout"
        except Exception as e:
            return False, str(e)
    
    def validate_python(self) -> None:
        """Überprüft Python-Installation"""
        print("\n[Python Environment]")
        
        success, output = self.run_command([sys.executable, "--version"])
        self.check("Python Installation", success, output)
        
        # Überprüfe erforderliche Module
        required_modules = ["asyncio", "aiohttp", "json"]
        for module in required_modules:
            try:
                __import__(module)
                self.check(f"Module '{module}' verfügbar", True)
            except ImportError:
                self.check(f"Module '{module}' verfügbar", False, "pip install erforderlich")
    
    def validate_docker(self) -> None:
        """Überprüft Docker-Installation"""
        print("\n[Docker Environment]")
        
        # Docker
        success, output = self.run_command(["docker", "--version"])
        self.check("Docker Installation", success, output)
        
        # Docker Daemon
        success, output = self.run_command(["docker", "ps"])
        self.check("Docker Daemon läuft", success)
        
        # Docker Compose
        success, output = self.run_command(["docker", "compose", "version"])
        if not success:
            success, output = self.run_command(["docker-compose", "--version"])
        self.check("Docker Compose Installation", success, output)
    
    def validate_files(self) -> None:
        """Überprüft erforderliche Dateien"""
        print("\n[Required Files]")
        
        files = {
            "Docker Compose": "docker-compose.multi-shard-raid.yml",
            "Benchmark Script": "run_multi_shard_raid_benchmark.py",
            "PowerShell Runner": "run_benchmark.ps1",
            "Data Loader": "scripts/load_test_data.py",
            "Prometheus Config": "monitoring/prometheus.yml",
            "Analyzer": "analyze_results.py",
            "Benchmark Plan": "MULTI_SHARD_RAID_BENCHMARK_PLAN.md",
            "Quick Start": "MULTI_SHARD_RAID_QUICKSTART.md",
        }
        
        for name, filename in files.items():
            filepath = self.benchmarks_dir / filename
            exists = filepath.exists()
            self.check(f"{name}", exists, str(filepath))
            
            if exists and filename.endswith(".py"):
                # Überprüfe Python-Syntax
                success, _ = self.run_command([sys.executable, "-m", "py_compile", str(filepath)])
                if not success:
                    self.warning(f"Syntax-Fehler in {filename}")
    
    def validate_directories(self) -> None:
        """Überprüft erforderliche Verzeichnisse"""
        print("\n[Directories]")
        
        directories = ["scripts", "monitoring", "monitoring/grafana"]
        
        for dirname in directories:
            dirpath = self.benchmarks_dir / dirname
            exists = dirpath.is_dir()
            self.check(f"Verzeichnis '{dirname}' vorhanden", exists)
    
    def validate_docker_images(self) -> None:
        """Überprüft verfügbare Docker-Images"""
        print("\n[Docker Images]")
        
        # Überprüfe ob Docker Images verfügbar sind
        success, output = self.run_command(["docker", "images", "themisdb/themisdb"])
        has_themis = success and "themisdb" in output
        
        if has_themis:
            self.check("ThemisDB Image verfügbar", True)
        else:
            self.warning("ThemisDB Image nicht lokal verfügbar (wird beim Start heruntergeladen)")
    
    def validate_ports(self) -> None:
        """Überprüft verfügbare Ports"""
        print("\n[Network Ports]")
        
        ports_info = [
            ("Grafana", 3000),
            ("Prometheus", 9090),
            ("ThemisDB Shard 0", 8080),
            ("ThemisDB Shard 1", 8081),
        ]
        
        all_available = True
        for name, port in ports_info:
            try:
                import socket
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                result = sock.connect_ex(('localhost', port))
                sock.close()
                
                if result == 0:
                    self.warning(f"Port {port} ({name}) wird bereits verwendet")
                    all_available = False
                else:
                    self.check(f"Port {port} ({name}) verfügbar", True)
            except Exception as e:
                self.warning(f"Fehler beim Prüfen von Port {port}: {e}")
                all_available = False
        
        if not all_available:
            self.warning("Einige Ports sind belegt - Docker Compose kann Fehler verursachen")
    
    def validate_disk_space(self) -> None:
        """Überprüft verfügbaren Speicherplatz"""
        print("\n[Disk Space]")
        
        try:
            import shutil
            stat = shutil.disk_usage(str(self.benchmarks_dir))
            free_gb = stat.free / (1024**3)
            
            required_gb = 500  # Minimum für Tests
            
            if free_gb >= required_gb:
                self.check(f"Speicherplatz ausreichend", True, f"{free_gb:.1f}GB verfügbar")
            else:
                self.warning(f"Nur {free_gb:.1f}GB verfügbar (empfohlen: {required_gb}GB)")
        except Exception as e:
            self.warning(f"Fehler beim Prüfen des Speicherplatzes: {e}")
    
    def validate_system_resources(self) -> None:
        """Überprüft Systemressourcen"""
        print("\n[System Resources]")
        
        try:
            import psutil
            
            # CPU-Kerne
            cpu_count = psutil.cpu_count()
            self.check(f"CPU-Kerne", cpu_count >= 8, f"{cpu_count} Kerne verfügbar")
            if cpu_count < 16:
                self.warning(f"Weniger als 16 CPU-Kerne - Performance kann begrenzt sein")
            
            # RAM
            ram_gb = psutil.virtual_memory().total / (1024**3)
            self.check(f"RAM verfügbar", ram_gb >= 16, f"{ram_gb:.1f}GB verfügbar")
            if ram_gb < 64:
                self.warning(f"Weniger als 64GB RAM - größere Tests können fehlschlagen")
        
        except ImportError:
            self.warning("psutil nicht installiert - kann Systemressourcen nicht prüfen")
        except Exception as e:
            self.warning(f"Fehler bei Ressourcen-Prüfung: {e}")
    
    def print_summary(self) -> None:
        """Druckt Zusammenfassung"""
        print("\n" + "="*80)
        print("VALIDIERUNGSERGEBNIS")
        print("="*80)
        
        total = self.passed_checks + self.failed_checks
        print(f"\nBestanden: {self.passed_checks}/{total}")
        print(f"Fehlgeschlagen: {self.failed_checks}/{total}")
        
        if self.warnings:
            print(f"\nWarnungen: {len(self.warnings)}")
            for warning in self.warnings:
                print(f"  ⚠ {warning}")
        
        print("\n" + "="*80)
        
        if self.failed_checks == 0:
            print("✓ Alle Überprüfungen bestanden!")
            print("\nNächste Schritte:")
            print("  1. Benchmark ausführen:")
            print("     ./run_benchmark.ps1 -Scenario S1 -RaidLevel RAID10 -NumShards 3")
            print("  2. Grafana öffnen: http://localhost:3000")
            print("  3. Ergebnisse analysieren: python analyze_results.py")
            return 0
        else:
            print("✗ Einige Überprüfungen fehlgeschlagen!")
            print("Bitte beheben Sie die oben genannten Fehler, bevor Sie Benchmarks starten.")
            return 1
    
    def run(self) -> int:
        """Führt alle Validierungen durch"""
        self.print_header()
        
        self.validate_python()
        self.validate_docker()
        self.validate_files()
        self.validate_directories()
        self.validate_docker_images()
        self.validate_ports()
        self.validate_disk_space()
        self.validate_system_resources()
        
        return self.print_summary()

def main():
    validator = BenchmarkValidator()
    return validator.run()

if __name__ == "__main__":
    sys.exit(main())
