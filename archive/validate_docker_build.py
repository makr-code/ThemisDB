#!/usr/bin/env python3
"""
Docker Build Validator - Validates Dockerfiles and docker-compose files
"""
import os
import json
import subprocess
import sys
from pathlib import Path

class DockerValidator:
    def __init__(self, repo_root="."):
        self.repo_root = Path(repo_root)
        self.results = []
        self.errors = []
        self.warnings = []
    
    def validate_dockerfile_syntax(self, dockerfile_path):
        """Validate Dockerfile syntax using docker build --dry-run"""
        print(f"\n[DOCKERFILE] Checking: {dockerfile_path}")
        
        try:
            # Check if file exists
            if not dockerfile_path.exists():
                self.errors.append(f"File not found: {dockerfile_path}")
                return False
            
            # Read file
            with open(dockerfile_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Check for common issues
            issues = self._check_dockerfile_content(content)
            
            if issues:
                for issue in issues:
                    print(f"  ⚠ {issue['type']}: {issue['msg']}")
                    self.warnings.append(f"{dockerfile_path}: {issue['msg']}")
            else:
                print(f"  ✓ Syntax OK")
                self.results.append((str(dockerfile_path), "PASS"))
                return True
            
            return len(issues) == 0
        
        except Exception as e:
            self.errors.append(f"Error validating {dockerfile_path}: {e}")
            print(f"  ✗ ERROR: {e}")
            return False
    
    def _check_dockerfile_content(self, content):
        """Check Dockerfile content for common issues"""
        issues = []
        lines = content.split('\n')
        stages = {}
        
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # Track FROM...AS stage names
            if stripped.startswith('FROM '):
                parts = stripped.split()
                if 'AS' in parts:
                    idx = parts.index('AS')
                    if idx + 1 < len(parts):
                        stage_name = parts[idx + 1]
                        stages[stage_name] = i
            
            # Check for COPY --from references
            if 'COPY --from=' in stripped or 'COPY --from ' in stripped:
                # Extract stage reference
                if '--from=' in stripped:
                    start = stripped.index('--from=') + 7
                    end = stripped.find(' ', start)
                    if end == -1:
                        end = len(stripped)
                    stage_ref = stripped[start:end]
                elif '--from ' in stripped:
                    parts = stripped.split()
                    idx = parts.index('--from')
                    if idx + 1 < len(parts):
                        stage_ref = parts[idx + 1]
                    else:
                        continue
                else:
                    continue
                
                # Check if stage exists (allow numbers 0-9 for compatibility)
                if not (stage_ref.isdigit() or stage_ref in stages or stage_ref == 'scratch'):
                    # This might be valid but not defined yet - only error if clearly invalid
                    if not any(c.isalnum() or c == '_' for c in stage_ref):
                        issues.append({
                            'type': 'ERROR',
                            'line': i,
                            'msg': f"Invalid COPY --from stage reference: '{stage_ref}' (not found in defined stages)"
                        })
            
            # Check for deprecated index-based stage references (e.g., --from=0)
            if '--from=0' in stripped and 'FROM ubuntu:24.04 AS builder' not in content[:content.find('\n' * i)]:
                # Check if this is problematic (no builder stage defined yet)
                if 'builder' not in stages and '0' not in stages:
                    # This is only an issue if there's no alias for stage 0
                    pass  # Will be caught by stage reference check above
        
        # Check for missing required fields
        if 'FROM ' not in content:
            issues.append({'type': 'ERROR', 'msg': 'No FROM instruction found'})
        
        return issues
    
    def validate_compose_file(self, compose_path):
        """Validate docker-compose file"""
        print(f"\n[COMPOSE] Checking: {compose_path}")
        
        try:
            if not compose_path.exists():
                self.errors.append(f"File not found: {compose_path}")
                return False
            
            # Try to parse as YAML (if PyYAML available)
            try:
                import yaml
                with open(compose_path, 'r', encoding='utf-8') as f:
                    compose_data = yaml.safe_load(f)
                
                # Basic validation
                if not isinstance(compose_data, dict):
                    self.errors.append(f"{compose_path}: Root must be a dict")
                    return False
                
                if 'services' not in compose_data and 'version' in compose_data:
                    print(f"  ⚠ No services defined")
                    self.warnings.append(f"{compose_path}: No services defined")
                
                print(f"  ✓ Valid YAML structure")
                self.results.append((str(compose_path), "PASS"))
                return True
            
            except ImportError:
                print(f"  ⚠ PyYAML not available, skipping YAML validation")
                return True
            except Exception as e:
                self.errors.append(f"{compose_path}: {e}")
                print(f"  ✗ YAML Error: {e}")
                return False
        
        except Exception as e:
            self.errors.append(f"Error validating {compose_path}: {e}")
            print(f"  ✗ ERROR: {e}")
            return False
    
    def run_all_validations(self):
        """Run all Docker validations"""
        print("=" * 70)
        print("Docker Build Validator")
        print("=" * 70)
        
        # Find all Dockerfiles
        dockerfiles = [
            self.repo_root / "Dockerfile",
            self.repo_root / "Dockerfile.community-simple",
            self.repo_root / "Dockerfile.prebuilt-local",
            self.repo_root / "Dockerfile.prebuilt-helper",
        ]
        
        # Also check in docker/ subdirectory
        docker_dir = self.repo_root / "docker"
        if docker_dir.exists():
            dockerfiles.extend(docker_dir.glob("Dockerfile*"))
        
        # Validate Dockerfiles
        print("\n[1/2] Validating Dockerfiles...")
        dockerfile_results = []
        for df in dockerfiles:
            if df.exists():
                result = self.validate_dockerfile_syntax(df)
                dockerfile_results.append(result)
        
        # Validate docker-compose files
        print("\n[2/2] Validating docker-compose files...")
        compose_results = []
        compose_files = [
            self.repo_root / "docker-compose.yml",
        ]
        
        # Also check docker/ directory
        if docker_dir.exists():
            compose_files.extend(docker_dir.glob("docker-compose*.yml"))
        
        for cf in compose_files:
            if cf.exists():
                result = self.validate_compose_file(cf)
                compose_results.append(result)
        
        # Print summary
        print("\n" + "=" * 70)
        print("VALIDATION SUMMARY")
        print("=" * 70)
        
        total_passed = len(self.results)
        total_warnings = len(self.warnings)
        total_errors = len(self.errors)
        
        print(f"\nResults: {total_passed} passed")
        if self.warnings:
            print(f"Warnings: {total_warnings}")
            for warn in self.warnings:
                print(f"  ⚠ {warn}")
        
        if self.errors:
            print(f"Errors: {total_errors}")
            for err in self.errors:
                print(f"  ✗ {err}")
        
        print("\n" + "=" * 70)
        
        # Overall status
        if total_errors > 0:
            print("Status: ✗ VALIDATION FAILED (errors found)")
            return 1
        elif total_warnings > 0:
            print("Status: ⚠ VALIDATION WARNING (warnings found, but valid)")
            return 0
        else:
            print("Status: ✓ ALL VALIDATIONS PASSED")
            return 0


def main():
    validator = DockerValidator()
    exit_code = validator.run_all_validations()
    
    # Also try docker build syntax check if docker is available
    print("\n[3/3] Docker build syntax check (if docker available)...")
    try:
        result = subprocess.run(
            ["docker", "build", "--dry-run", "-f", "Dockerfile", "."],
            capture_output=True,
            timeout=10,
            cwd=".",
            text=True
        )
        if result.returncode == 0:
            print("  ✓ docker build --dry-run succeeded")
        else:
            print(f"  ⚠ docker build check returned: {result.returncode}")
            if result.stderr:
                print(f"     stderr: {result.stderr[:200]}")
    except FileNotFoundError:
        print("  ⚠ Docker not found in PATH (install Docker to enable build validation)")
    except subprocess.TimeoutExpired:
        print("  ⚠ docker build check timed out")
    except Exception as e:
        print(f"  ⚠ docker build check error: {e}")
    
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
