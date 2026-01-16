#!/usr/bin/env python3
"""
ThemisDB Static Initialization Crash Analyzer
Isolate crashing static initializer through binary inspection
"""

import subprocess
import sys
import os
import re
from pathlib import Path

def run_cmd(cmd, cwd=None):
    """Run command and return stdout"""
    try:
        result = subprocess.run(
            cmd, 
            cwd=cwd, 
            capture_output=True, 
            text=True,
            shell=True
        )
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        return -1, "", str(e)

def analyze_binary_symbols(binary_path):
    """Extract static initializers from binary using dumpbin"""
    print(f"[*] Analyzing binary symbols: {binary_path}")
    
    # Use dumpbin.exe to get imports and exports
    dumpbin_path = r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\..\..\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\dumpbin.exe"
    
    if not os.path.exists(dumpbin_path):
        print(f"[!] dumpbin.exe not found at {dumpbin_path}")
        return []
    
    code, stdout, stderr = run_cmd(f'"{dumpbin_path}" /exports "{binary_path}" 2>&1 | findstr /R "^[ ]*[0-9]"')
    
    # Look for initialization function patterns
    init_functions = []
    for line in stdout.split('\n'):
        if any(pattern in line for pattern in ['_GLOBAL_', 'dynamic initializer', 'atexit', '__init', 'CRT']):
            init_functions.append(line.strip())
    
    return init_functions

def find_problematic_includes():
    """Find .cpp files that load files or do I/O in static context"""
    print("[*] Scanning for problematic static I/O...")
    
    themis_root = Path(r"C:\VCC\themis")
    suspicious_patterns = [
        (r"static.*YAML::|static.*YAML\.Load", "YAML file loading in static context"),
        (r"static.*std::ifstream|static.*std::fstream", "File stream in static context"),
        (r"static.*std::filesystem::exists|static.*std::filesystem::read", "Filesystem ops in static context"),
    ]
    
    results = []
    
    for cpp_file in themis_root.glob("src/**/*.cpp"):
        try:
            content = cpp_file.read_text(errors='ignore')
            for pattern, description in suspicious_patterns:
                if re.search(pattern, content):
                    results.append((cpp_file.relative_to(themis_root), description))
        except:
            pass
    
    return results

def test_minimal_binary():
    """Create a minimal test binary to isolate problem"""
    print("\n[*] Creating minimal test binary...")
    
    test_code = r"""
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        std::cout << "Minimal binary test" << std::endl;
        if (argc > 1) {
            std::string arg = argv[1];
            if (arg == "--test") {
                std::cout << "Test successful" << std::endl;
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 2;
    }
}
"""
    
    test_cpp = r"C:\VCC\themis\test_minimal.cpp"
    test_exe = r"C:\VCC\themis\build-vs\cmake\Release\test_minimal.exe"
    
    # Write test file
    with open(test_cpp, 'w') as f:
        f.write(test_code)
    
    # Compile
    vsdevcmd = r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    compile_cmd = f'cmd /c "{vsdevcmd}" -arch=x64 && cl.exe /EHsc "{test_cpp}" /link /OUT:"{test_exe}"'
    
    code, stdout, stderr = run_cmd(compile_cmd)
    if code == 0:
        print(f"[+] Compiled test binary: {test_exe}")
        
        # Test it
        code, out, err = run_cmd(f'"{test_exe}" --test')
        print(f"[*] Test binary exit code: {code}")
        if code == 0:
            print("[+] Test binary works - problem is specific to themis_server initialization")
        return True
    else:
        print(f"[-] Compilation failed: {stderr}")
        return False

def main():
    binary_path = r"C:\VCC\themis\build-vs\cmake\Release\themis_server.exe"
    
    print("="*60)
    print("ThemisDB Static Initialization Crash Analyzer")
    print("="*60)
    
    if not os.path.exists(binary_path):
        print(f"[!] Binary not found: {binary_path}")
        return 1
    
    # Get file info
    size_mb = os.path.getsize(binary_path) / (1024*1024)
    print(f"[*] Binary: {binary_path}")
    print(f"[*] Size: {size_mb:.1f} MB")
    
    # Try minimal binary
    test_minimal_binary()
    
    # Analyze for problematic includes
    print("\n" + "="*60)
    print("Scanning for problematic static initialization patterns...")
    print("="*60)
    problematic = find_problematic_includes()
    if problematic:
        for file, desc in problematic[:10]:
            print(f"  {file}: {desc}")
    else:
        print("  [!] No obvious patterns found")
    
    # Analysis results
    print("\n" + "="*60)
    print("ANALYSIS SUMMARY")
    print("="*60)
    print("""
The -1073741502 (0xC0000142) crash occurs during CRT initialization,
before main() executes. This indicates:

1. Static global constructor is throwing an unhandled exception
2. OR: Dynamic initialization of static storage is failing
3. OR: DLL entrypoint is failing during CRT setup

NEXT STEPS:
1. Use Visual Studio Debugger with themis_server.exe
2. Set breakpoint at entrypoint  
3. Step through DLL initialization
4. Identify which module/function fails first

RECOMMENDED DEBUGGING SESSION:
  1. devenv C:\\VCC\\themis\\build-vs\\Themis.sln
  2. Debug > Start Debugging (F5)
  3. Catch first exception: Debug > Exceptions
     - Enable all C++ exceptions
     - Set to "Break"
  4. Run and observe which static constructor fails
""")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
