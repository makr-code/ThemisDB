#!/usr/bin/env python3
"""Capture local benchmark artifact hashes and a limited host fingerprint.

The output is a self-description of files and host properties observed locally.
It is not attestation, a baseline, or a performance decision.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import sys

BENCHMARKS_DIR = Path(__file__).resolve().parents[2] / "benchmarks"
sys.path.insert(0, str(BENCHMARKS_DIR / "cmake"))
from benchmark_run_manifest import load_workload_links, resolve_workload_id


UNKNOWN = {"", "unknown", "unset", "n/a", "na", "none", "not available", "not_available"}
SAFE_OS_ID = re.compile(r"[A-Za-z0-9._+-]{1,128}\Z")
SAFE_CPU_MODEL = re.compile(r"[A-Za-z0-9][A-Za-z0-9 .,_()+@-]{0,255}\Z")


def _clean_text(value, label, maximum=256):
    if (not isinstance(value, str) or not value or value != value.strip()
            or value.casefold() in UNKNOWN or len(value) > maximum
            or any(ord(char) < 32 or ord(char) == 127 for char in value)):
        raise ValueError(f"{label}: missing or invalid")
    return value


def _clean_cpu_model(value):
    value = _clean_text(value, "hardware CPU model")
    if not SAFE_CPU_MODEL.fullmatch(value):
        raise ValueError("hardware fingerprint: CPU model contains unsupported characters")
    return value


def _read_regular_file(path, label):
    path = Path(path)
    if not path.is_file():
        raise ValueError(f"{label}: not a regular file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            before = os.fstat(stream.fileno())
            while chunk := stream.read(1024 * 1024):
                digest.update(chunk)
            after = os.fstat(stream.fileno())
    except OSError as exc:
        raise ValueError(f"{label}: cannot be read") from exc
    identity_before = (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns)
    identity_after = (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns)
    if identity_before != identity_after:
        raise ValueError(f"{label}: changed while being hashed")
    return digest.hexdigest()


def _read_stable_bytes(path, label):
    path = Path(path)
    if not path.is_file():
        raise ValueError(f"{label}: not a regular file")
    try:
        with path.open("rb") as stream:
            before = os.fstat(stream.fileno())
            content = stream.read()
            after = os.fstat(stream.fileno())
    except OSError as exc:
        raise ValueError(f"{label}: cannot be read") from exc
    identity_before = (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns)
    identity_after = (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns)
    if identity_before != identity_after or len(content) != after.st_size:
        raise ValueError(f"{label}: changed while being read")
    return content


def _unique_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _read_workload_id(result_path, target):
    try:
        raw_result = _read_stable_bytes(result_path, "raw result")
        result = json.loads(
            raw_result.decode("utf-8-sig"),
            object_pairs_hook=_unique_json_object,
            parse_constant=lambda value: (_ for _ in ()).throw(
                ValueError(f"invalid JSON number: {value}")),
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError("raw result: cannot be parsed as JSON") from exc
    rows = result.get("benchmarks") if isinstance(result, dict) else None
    if not isinstance(rows, list) or not rows:
        raise ValueError("raw result: missing benchmark cases")

    links = load_workload_links(BENCHMARKS_DIR)
    if not links:
        raise ValueError("workload links are unavailable")
    workload_ids = set()
    observed = 0
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("raw result: invalid benchmark row")
        run_type = row.get("run_type", "iteration")
        if run_type not in ("iteration", "aggregate"):
            raise ValueError("raw result: invalid benchmark run type")
        if run_type == "aggregate":
            continue
        name = row.get("name")
        if not isinstance(name, str) or not name:
            raise ValueError("raw result: benchmark case name is missing")
        workload_id = resolve_workload_id(target, name, links)
        if workload_id is None:
            raise ValueError(f"raw result case has no unique catalog link: {name}")
        workload_ids.add(workload_id)
        observed += 1
    if observed == 0 or len(workload_ids) != 1:
        raise ValueError("raw result must resolve to one source-described workload")

    workload_id = next(iter(workload_ids))
    try:
        catalog_bytes = (BENCHMARKS_DIR / "workload_profiles.json").read_bytes()
        catalog = json.loads(catalog_bytes.decode("utf-8-sig"),
                             object_pairs_hook=_unique_json_object)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError("workload catalog is unavailable or invalid") from exc
    profiles = (catalog.get("profiles") if isinstance(catalog, dict)
                and type(catalog.get("schema_version")) is int
                and catalog["schema_version"] == 1 else None)
    matches = ([profile for profile in profiles
                if isinstance(profile, dict) and profile.get("workload_id") == workload_id]
               if isinstance(profiles, list) else [])
    if len(matches) != 1 or matches[0].get("status") != "source-described":
        raise ValueError("workload is not uniquely source-described in the catalog")
    return (workload_id, hashlib.sha256(catalog_bytes).hexdigest(),
            hashlib.sha256(raw_result).hexdigest())


def _linux_cpu_model():
    try:
        content = Path("/proc/cpuinfo").read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise ValueError("hardware fingerprint: CPU model unavailable") from exc
    candidates = []
    for block in re.split(r"\n\s*\n", content):
        fields = {}
        for line in block.splitlines():
            if ":" in line:
                key, value = line.split(":", 1)
                fields[key.strip().casefold()] = value.strip()
        model = fields.get("model name") or fields.get("hardware")
        if model:
            candidates.append(model)
    unique = {_clean_cpu_model(value) for value in candidates}
    if len(unique) != 1:
        raise ValueError("hardware fingerprint: CPU model is missing or heterogeneous")
    return next(iter(unique))


def _linux_memory_bytes():
    try:
        lines = Path("/proc/meminfo").read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeError) as exc:
        raise ValueError("hardware fingerprint: memory size unavailable") from exc
    values = [match.group(1) for line in lines
              if (match := re.fullmatch(r"MemTotal:\s+([0-9]+)\s+kB", line))]
    if len(values) != 1 or int(values[0]) <= 0:
        raise ValueError("hardware fingerprint: memory size unavailable")
    return int(values[0]) * 1024


def _linux_os():
    try:
        lines = Path("/etc/os-release").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as exc:
        raise ValueError("OS fingerprint: /etc/os-release unavailable") from exc
    values = {}
    for line in lines:
        if not line or line.startswith("#"):
            continue
        key, separator, raw_value = line.partition("=")
        if not separator or key not in ("ID", "VERSION_ID"):
            continue
        if key in values:
            raise ValueError(f"OS fingerprint: duplicate {key} in /etc/os-release")
        try:
            parsed = shlex.split(raw_value, comments=True, posix=True)
        except ValueError as exc:
            raise ValueError("OS fingerprint: malformed /etc/os-release") from exc
        if len(parsed) != 1:
            raise ValueError("OS fingerprint: malformed /etc/os-release")
        values[key] = parsed[0]
    distribution = values.get("ID")
    version = values.get("VERSION_ID")
    if not isinstance(distribution, str) or not SAFE_OS_ID.fullmatch(distribution):
        raise ValueError("OS fingerprint: distribution ID unavailable")
    if not isinstance(version, str) or not SAFE_OS_ID.fullmatch(version):
        raise ValueError("OS fingerprint: distribution version unavailable")
    return {"system": "Linux", "release": _clean_text(platform.release(), "OS release"),
            "distribution": distribution, "distribution_version": version}


def _windows_cpu_model():
    import winreg

    key_path = r"HARDWARE\DESCRIPTION\System\CentralProcessor"
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path) as key:
            values = []
            for index in range(winreg.QueryInfoKey(key)[0]):
                with winreg.OpenKey(key, str(index)) as processor:
                    value, _ = winreg.QueryValueEx(processor, "ProcessorNameString")
                    values.append(_clean_cpu_model(value))
    except OSError as exc:
        raise ValueError("hardware fingerprint: CPU model unavailable") from exc
    unique = set(values)
    if len(unique) != 1:
        raise ValueError("hardware fingerprint: CPU model is missing or heterogeneous")
    return next(iter(unique))


def _windows_memory_bytes():
    import ctypes

    class MemoryStatus(ctypes.Structure):
        _fields_ = [
            ("dwLength", ctypes.c_ulong),
            ("dwMemoryLoad", ctypes.c_ulong),
            ("ullTotalPhys", ctypes.c_ulonglong),
            ("ullAvailPhys", ctypes.c_ulonglong),
            ("ullTotalPageFile", ctypes.c_ulonglong),
            ("ullAvailPageFile", ctypes.c_ulonglong),
            ("ullTotalVirtual", ctypes.c_ulonglong),
            ("ullAvailVirtual", ctypes.c_ulonglong),
            ("ullAvailExtendedVirtual", ctypes.c_ulonglong),
        ]

    status = MemoryStatus()
    status.dwLength = ctypes.sizeof(status)
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    if not kernel32.GlobalMemoryStatusEx(ctypes.byref(status)) or status.ullTotalPhys <= 0:
        raise ValueError("hardware fingerprint: memory size unavailable")
    return int(status.ullTotalPhys)


def collect_host_fingerprint():
    system = platform.system()
    if system == "Windows":
        os_fingerprint = {
            "system": "Windows",
            "release": _clean_text(platform.release(), "OS release"),
            "distribution": None,
            "distribution_version": _clean_text(platform.version(), "OS version"),
        }
        cpu_model = _windows_cpu_model()
        memory_bytes = _windows_memory_bytes()
    elif system == "Linux":
        os_fingerprint = _linux_os()
        cpu_model = _linux_cpu_model()
        memory_bytes = _linux_memory_bytes()
    else:
        raise ValueError(f"OS fingerprint: unsupported platform {system or 'unknown'}")
    logical_processors = os.cpu_count()
    hardware = {
        "architecture": _clean_text(platform.machine(), "hardware architecture"),
        "cpu_model": _clean_cpu_model(cpu_model),
        "logical_processors": logical_processors,
        "memory_bytes": memory_bytes,
    }
    validate_host_fingerprint(os_fingerprint, hardware)
    return {"os": os_fingerprint, "hardware": hardware}


def validate_host_fingerprint(os_fingerprint, hardware):
    if (not isinstance(os_fingerprint, dict)
            or set(os_fingerprint) != {
                "system", "release", "distribution", "distribution_version"}):
        raise ValueError("OS fingerprint: missing required identity")
    if os_fingerprint["system"] not in ("Windows", "Linux"):
        raise ValueError("OS fingerprint: unsupported or missing system")
    _clean_text(os_fingerprint["release"], "OS release")
    if os_fingerprint["system"] == "Linux":
        _clean_text(os_fingerprint["distribution"], "OS distribution")
        _clean_text(os_fingerprint["distribution_version"], "OS distribution version")
    elif os_fingerprint["distribution"] is not None:
        raise ValueError("OS fingerprint: unexpected Windows distribution")
    else:
        _clean_text(os_fingerprint["distribution_version"], "OS version")
    if (not isinstance(hardware, dict)
            or set(hardware) != {
                "architecture", "cpu_model", "logical_processors", "memory_bytes"}):
        raise ValueError("hardware fingerprint: missing required identity")
    _clean_text(hardware["architecture"], "hardware architecture")
    _clean_cpu_model(hardware["cpu_model"])
    for key in ("logical_processors", "memory_bytes"):
        value = hardware[key]
        if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
            raise ValueError(f"hardware fingerprint: {key} unavailable")


def capture(executable, runner, result, configuration, target):
    if not isinstance(target, str) or Path(executable).stem != target:
        raise ValueError("target must match the executable filename stem exactly")
    workload_id, catalog_hash, result_hash = _read_workload_id(result, target)
    host = collect_host_fingerprint()
    validate_host_fingerprint(host.get("os"), host.get("hardware"))
    return {
        "schema_version": 1,
        "identity_scope": "local-observation-not-attestation",
        "captured_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "host_fingerprint_scope": (
            "OS, CPU model, architecture, logical processor count, and reported memory; "
            "storage and accelerators are not observed"
        ),
        "workload": {
            "workload_id": workload_id,
            "catalog_sha256": catalog_hash,
        },
        "artifacts": {
            "executable_sha256": _read_regular_file(executable, "executable"),
            "runner_sha256": _read_regular_file(runner, "runner"),
            "raw_result_sha256": result_hash,
            "configuration_sha256": _read_regular_file(configuration, "configuration"),
        },
        "host": host,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("executable", "runner", "result", "configuration", "target", "output"):
        parser.add_argument("--" + option, required=True)
    args = parser.parse_args()
    try:
        inputs = [Path(getattr(args, name)).resolve(strict=True)
                  for name in ("executable", "runner", "result", "configuration")]
        output = Path(args.output).resolve()
        if output in inputs:
            raise ValueError("output must not replace an input artifact")
        record = capture(args.executable, args.runner, args.result,
                         args.configuration, args.target)
        if output.exists():
            raise ValueError("output already exists")
        if not output.parent.is_dir():
            raise ValueError("output parent directory does not exist")
        created = False
        try:
            with output.open("x", encoding="utf-8") as stream:
                created = True
                json.dump(record, stream, indent=2, allow_nan=False)
                stream.write("\n")
        except BaseException:
            if created:
                output.unlink(missing_ok=True)
            raise
    except (OSError, UnicodeError, ValueError, OverflowError) as exc:
        print(f"Identity capture failed: {exc}", file=sys.stderr)
        return 1
    print("Local identity captured; this is not attestation or a baseline.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
