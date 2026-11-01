#!/usr/bin/env python3
"""Fail packaging when the installed runtime lacks compiled docs.db or TinyLlama."""

import argparse
import os
import subprocess
import tarfile
import tempfile
import zipfile
from pathlib import Path


def validate(root: Path, models: Path | None = None) -> None:
    database = root / "data/docs.db"
    current = database / "CURRENT"
    if not current.is_file() or not current.stat().st_size:
        raise ValueError(f"Compiled documentation database CURRENT is missing: {database}")
    manifest = current.read_text(encoding="ascii").strip()
    if not manifest.startswith("MANIFEST-") or Path(manifest).name != manifest:
        raise ValueError(f"Invalid RocksDB CURRENT pointer: {current}")
    if not (database / manifest).is_file() or not (database / manifest).stat().st_size:
        raise ValueError(f"RocksDB manifest is missing or empty: {database / manifest}")
    if not any(file.stat().st_size for pattern in ("*.sst", "*.log") for file in database.glob(pattern)):
        raise ValueError(f"Compiled documentation database has no populated SST or WAL file: {database}")
    model_dir = models if models is not None else root / "models"
    candidates = [file for file in model_dir.glob("*.gguf") if "tinyllama" in file.name.lower()]
    if not candidates:
        raise ValueError(f"TinyLlama GGUF is missing: {model_dir}")
    for model in candidates:
        if model.stat().st_size <= 100_000_000:
            raise ValueError(f"TinyLlama GGUF is truncated or too small: {model}")
        with model.open("rb") as stream:
            if stream.read(4) not in (b"GGUF", b"FUGG"):
                raise ValueError(f"Invalid TinyLlama GGUF header: {model}")


def validate_extracted(root: Path) -> None:
    databases = list(root.rglob("data/docs.db/CURRENT"))
    if not databases:
        raise ValueError(f"Package has no compiled data/docs.db: {root}")
    for current in databases:
        validate(current.parents[2])


def validate_package(package: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="themis-payload-") as temporary:
        extracted = Path(temporary)
        if zipfile.is_zipfile(package):
            with zipfile.ZipFile(package) as archive:
                for member in archive.infolist():
                    if "/data/docs.db/" in "/" + member.filename or "/models/" in "/" + member.filename:
                        target = (extracted / member.filename).resolve()
                        if not target.is_relative_to(extracted.resolve()):
                            raise ValueError(f"Unsafe package path: {member.filename}")
                        archive.extract(member, extracted)
        elif package.name.endswith((".tar.gz", ".tgz")):
            with tarfile.open(package) as archive:
                members = [member for member in archive.getmembers()
                           if "/data/docs.db/" in "/" + member.name or "/models/" in "/" + member.name]
                archive.extractall(extracted, members=members, filter="data")
        elif package.suffix == ".deb":
            subprocess.run(["dpkg-deb", "-x", str(package), str(extracted)], check=True)
        elif package.suffix == ".rpm":
            subprocess.run(["bsdtar", "-xf", str(package), "-C", str(extracted)], check=True)
        elif package.suffix == ".msi":
            if os.name == "nt":
                subprocess.run(["msiexec", "/a", str(package.resolve()), "/qn",
                                f"TARGETDIR={extracted}"], check=True)
            else:
                subprocess.run(["msiextract", "-C", str(extracted), str(package)], check=True)
        elif package.suffix == ".dmg":
            mount = extracted / "mount"
            subprocess.run(["hdiutil", "attach", str(package), "-readonly", "-nobrowse",
                            "-mountpoint", str(mount)], check=True, stdout=subprocess.DEVNULL)
            try:
                validate_extracted(mount)
            finally:
                subprocess.run(["hdiutil", "detach", str(mount)], check=True, stdout=subprocess.DEVNULL)
            return
        else:
            raise ValueError(f"Unsupported release package format: {package}")
        validate_extracted(extracted)
    print(f"Package payload verified: {package.name}")


def validate_packages(directory: Path) -> None:
    packages = [file for file in directory.iterdir() if file.is_file() and
                file.name.endswith((".tar.gz", ".tgz", ".zip", ".deb", ".rpm", ".msi", ".dmg"))]
    if not packages:
        raise ValueError(f"No distributable packages found: {directory}")
    for package in packages:
        validate_package(package)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path, help="Installed package tree containing data/docs.db and models/")
    parser.add_argument("--models", type=Path, help="Separate model directory (Docker build stages)")
    parser.add_argument("--packages", action="store_true", help="Validate every package in the directory")
    args = parser.parse_args()
    try:
        if args.root.is_file():
            validate_package(args.root)
        elif args.packages:
            validate_packages(args.root)
        else:
            validate(args.root, args.models)
    except (ValueError, OSError, subprocess.CalledProcessError, tarfile.TarError, zipfile.BadZipFile) as error:
        parser.exit(1, f"::error::{error}\n")
    print(f"Runtime payload verified: {args.root}")


if __name__ == "__main__":
    main()
