"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            verify_benchmark_protocol.py                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Benchmark Protocol Verification & Comparison Tool
Verifies current system against locked protocol and detects changes
"""

import json
import subprocess
import platform
from pathlib import Path
from datetime import datetime


class ProtocolVerifier:
    def __init__(self, protocol_file: str = "benchmark_protocols/benchmark_protocol_schema.json"):
        self.protocol_file = Path(protocol_file)
        self.locked_protocol = self._load_protocol()
        self.current_system = self._capture_current_system()
        self.deviations = []
    
    def _load_protocol(self) -> dict:
        """Load locked protocol from JSON"""
        if self.protocol_file.exists():
            with open(self.protocol_file, 'r') as f:
                return json.load(f)
        return {}
    
    def _capture_current_system(self) -> dict:
        """Capture current system state"""
        return {
            "cpu": self._get_cpu_info(),
            "memory": self._get_memory_info(),
            "storage": self._get_storage_info(),
            "os": self._get_os_info(),
            "docker": self._get_docker_info(),
            "python": self._get_python_info()
        }
    
    def _get_cpu_info(self) -> dict:
        try:
            cpu_info = subprocess.run(
                ["powershell", "-Command",
                 "Get-CimInstance Win32_Processor | Select-Object -First 1 | ConvertTo-Json"],
                capture_output=True, text=True, check=True
            ).stdout
            return json.loads(cpu_info)
        except:
            return {}
    
    def _get_memory_info(self) -> dict:
        try:
            mem_info = subprocess.run(
                ["powershell", "-Command",
                 "Get-CimInstance Win32_OperatingSystem | Select-Object TotalVisibleMemorySize | ConvertTo-Json"],
                capture_output=True, text=True, check=True
            ).stdout
            data = json.loads(mem_info)
            return {"total_gb": round(data.get("TotalVisibleMemorySize", 0) / (1024**2), 2)}
        except:
            return {}
    
    def _get_storage_info(self) -> dict:
        try:
            storage_info = subprocess.run(
                ["powershell", "-Command",
                 "Get-CimInstance Win32_LogicalDisk -Filter \"DeviceID='C:'\" | ConvertTo-Json"],
                capture_output=True, text=True, check=True
            ).stdout
            data = json.loads(storage_info)
            return {
                "total_gb": round(data.get("Size", 0) / (1024**3), 2),
                "free_gb": round(data.get("FreeSpace", 0) / (1024**3), 2)
            }
        except:
            return {}
    
    def _get_os_info(self) -> dict:
        return {
            "name": platform.system(),
            "version": platform.version(),
            "architecture": platform.architecture()[0]
        }
    
    def _get_docker_info(self) -> dict:
        try:
            docker_version = subprocess.run(
                ["docker", "--version"],
                capture_output=True, text=True, check=True
            ).stdout.strip()
            return {"docker": docker_version}
        except:
            return {"docker": "not installed"}
    
    def _get_python_info(self) -> dict:
        return {
            "version": platform.python_version()
        }
    
    def verify(self) -> bool:
        """Verify current system matches locked protocol"""
        print("\n" + "="*70)
        print("BENCHMARK PROTOCOL VERIFICATION")
        print("="*70 + "\n")
        
        if not self.locked_protocol:
            print("[!] No locked protocol found - cannot verify")
            return False
        
        locked = self.locked_protocol.get("benchmark_protocol", {})
        
        # Check CPU
        print("[*] Checking CPU...")
        locked_cpu = locked.get("hardware", {}).get("cpu", {})
        if locked_cpu:
            if self.current_system["cpu"].get("NumberOfCores") != locked_cpu.get("physical_cores"):
                self.deviations.append(f"CPU cores: {self.current_system['cpu'].get('NumberOfCores')} vs {locked_cpu.get('physical_cores')}")
                print(f"[!] CPU core count changed!")
            else:
                print(f"[✓] CPU: {locked_cpu.get('model')} - OK")
        
        # Check Memory
        print("[*] Checking Memory...")
        if self.current_system["memory"].get("total_gb"):
            if self.current_system["memory"]["total_gb"] < locked.get("benchmark_environment", {}).get("docker_resources", {}).get("memory", {}).get("available_gb", 64):
                self.deviations.append(f"Memory: {self.current_system['memory']['total_gb']}GB vs {64}GB")
                print(f"[!] Memory size changed!")
            else:
                print(f"[✓] Memory: {self.current_system['memory']['total_gb']}GB - OK")
        
        # Check Storage
        print("[*] Checking Storage...")
        if self.current_system["storage"].get("free_gb"):
            required_gb = 80
            if self.current_system["storage"]["free_gb"] < required_gb:
                self.deviations.append(f"Storage: {self.current_system['storage']['free_gb']}GB free vs {required_gb}GB required")
                print(f"[!] Storage space is insufficient! ({self.current_system['storage']['free_gb']}GB < {required_gb}GB)")
            else:
                print(f"[✓] Storage: {self.current_system['storage']['free_gb']}GB free - OK")
        
        # Check OS
        print("[*] Checking OS...")
        locked_os = locked.get("software", {}).get("os", {})
        if locked_os.get("build") in self.current_system["os"].get("version", ""):
            print(f"[✓] OS: Windows Build {locked_os.get('build')} - OK")
        else:
            self.deviations.append(f"OS build may have changed")
            print(f"[!] OS build may have changed (current: {self.current_system['os'].get('version')})")
        
        # Check Docker
        print("[*] Checking Docker...")
        if "29.0.1" in self.current_system["docker"].get("docker", ""):
            print(f"[✓] Docker: 29.0.1 - OK")
        else:
            self.deviations.append(f"Docker version changed: {self.current_system['docker'].get('docker', 'unknown')}")
            print(f"[!] Docker version may have changed")
        
        # Check Python
        print("[*] Checking Python...")
        if self.current_system["python"]["version"].startswith("3.13"):
            print(f"[✓] Python: 3.13.6 - OK")
        else:
            self.deviations.append(f"Python version: {self.current_system['python']['version']}")
            print(f"[!] Python version changed to {self.current_system['python']['version']}")
        
        # Results
        print("\n" + "="*70)
        if self.deviations:
            print("[!] DEVIATIONS DETECTED FROM LOCKED PROTOCOL:")
            for i, dev in enumerate(self.deviations, 1):
                print(f"  {i}. {dev}")
            print("\n[WARNING] Document any deviations in benchmark results!")
            print("="*70 + "\n")
            return False
        else:
            print("[✓] SYSTEM MATCHES LOCKED PROTOCOL - ALL CHECKS PASSED")
            print("="*70 + "\n")
            return True
    
    def print_summary(self):
        """Print current system summary"""
        print("\nCURRENT SYSTEM CONFIGURATION:")
        print("-" * 70)
        print(f"CPU Cores: {self.current_system['cpu'].get('NumberOfCores', 'N/A')}")
        print(f"RAM: {self.current_system['memory'].get('total_gb', 'N/A')} GB")
        print(f"Storage Free: {self.current_system['storage'].get('free_gb', 'N/A')} GB")
        print(f"OS: {self.current_system['os'].get('name', 'N/A')} {self.current_system['os'].get('version', 'N/A')}")
        print(f"Docker: {self.current_system['docker'].get('docker', 'Not installed')}")
        print(f"Python: {self.current_system['python'].get('version', 'N/A')}")
        print("-" * 70 + "\n")


if __name__ == "__main__":
    verifier = ProtocolVerifier()
    verifier.print_summary()
    success = verifier.verify()
    
    exit(0 if success else 1)
