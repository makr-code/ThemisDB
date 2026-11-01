import importlib.util
import json
import os
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "runtime_payload", ROOT / "scripts/release/validate_runtime_payload.py"
)
PAYLOAD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PAYLOAD)


def create_payload(root):
    database = root / "data/docs.db"
    database.mkdir(parents=True)
    (database / "CURRENT").write_text("MANIFEST-000001\n")
    (database / "MANIFEST-000001").write_bytes(b"manifest fixture")
    (database / "000002.sst").write_bytes(b"document fixture")
    models = root / "models"
    models.mkdir()
    with (models / "tinyllama.gguf").open("wb") as model:
        model.write(b"GGUF")
        model.truncate(100_000_001)


class ReleaseRuntimePayloadTests(unittest.TestCase):
    def test_requires_both_database_and_complete_model(self):
        for missing in ["data/docs.db/CURRENT", "data/docs.db/MANIFEST-000001",
                        "data/docs.db/000002.sst", "models/tinyllama.gguf"]:
            with self.subTest(missing=missing), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                create_payload(root)
                PAYLOAD.validate(root)
                (root / missing).unlink()
                with self.assertRaises(ValueError):
                    PAYLOAD.validate(root)

    def test_rejects_truncated_model_invalid_magic_and_invalid_manifest_pointer(self):
        for case in ["truncated", "magic", "pointer"]:
            with self.subTest(case=case), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                create_payload(root)
                if case == "truncated":
                    (root / "models/tinyllama.gguf").write_bytes(b"GGUF")
                elif case == "magic":
                    with (root / "models/tinyllama.gguf").open("r+b") as model:
                        model.write(b"HTML")
                else:
                    (root / "data/docs.db/CURRENT").write_text("../MANIFEST-000001")
                with self.assertRaises(ValueError):
                    PAYLOAD.validate(root)

    def test_each_archive_is_checked_not_just_union_of_archives(self):
        for extension in ["zip", "tar.gz"]:
            with self.subTest(extension=extension), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                source = root / "source"
                create_payload(source)
                packages = root / "packages"
                packages.mkdir()
                archive = packages / f"complete.{extension}"
                if extension == "zip":
                    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as output:
                        for file in source.rglob("*"):
                            if file.is_file():
                                output.write(file, "package/" + str(file.relative_to(source)))
                else:
                    with tarfile.open(archive, "w:gz") as output:
                        output.add(source, arcname="package")
                PAYLOAD.validate_packages(packages)
                incomplete = packages / "incomplete.zip"
                with zipfile.ZipFile(incomplete, "w") as output:
                    output.writestr("package/bin/themis_server", b"binary")
                with self.assertRaisesRegex(ValueError, "no compiled"):
                    PAYLOAD.validate_packages(packages)

    @unittest.skipUnless(shutil.which("dpkg-deb"), "dpkg-deb required")
    def test_debian_payload_is_checked_inside_actual_package(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tree = root / "tree"
            create_payload(tree / "usr/share/themisdb")
            control = tree / "DEBIAN"
            control.mkdir()
            (control / "control").write_text(
                "Package: themis-payload-test\nVersion: 1.0\nArchitecture: all\n"
                "Maintainer: Test <test@example.invalid>\nDescription: Runtime payload fixture\n"
            )
            package = root / "package.deb"
            subprocess.run(["dpkg-deb", "--build", str(tree), str(package)],
                           check=True, capture_output=True)
            PAYLOAD.validate_package(package)

    @unittest.skipUnless(shutil.which("g++") and Path("/usr/include/rocksdb/db.h").exists(),
                         "RocksDB C++ development dependencies required")
    def test_generated_importer_produces_packagable_compiled_database(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_payload(root)
            shutil.rmtree(root / "data/docs.db")
            source = root / "documentation"
            source.mkdir()
            (source / "README.md").write_text("# Packaging\nOffline documentation for the release.\n")
            data = root / "data/docs_database.json"
            subprocess.run(
                ["python3", str(ROOT / "scripts/generate_docs_database.py"),
                 "--input-dir", str(source), "--output", str(data)],
                check=True, capture_output=True,
            )
            self.assertGreater(len(json.loads(data.read_text())["documents"]), 0)
            subprocess.run(
                ["python3", str(ROOT / "scripts/generate_docs_rocksdb.py"),
                 "--input", str(data), "--output", str(root / "data/docs.db"), "--method", "cpp"],
                check=True, capture_output=True,
            )
            executable = root / "importer"
            subprocess.run(
                ["g++", "-std=c++17", str(root / "data/import_docs_rocksdb.cpp"),
                 "-o", str(executable), "-lrocksdb"],
                check=True, capture_output=True,
            )
            subprocess.run([str(executable), str(data), str(root / "data/docs.db")],
                           check=True, capture_output=True)
            PAYLOAD.validate(root)

    def test_all_release_lanes_build_and_validate_both_payloads(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/release-build-matrix.yml").read_text())
        script = workflow["jobs"]["prepare-release-matrix"]["steps"][0]["run"]
        script = script.replace(' >> "$GITHUB_OUTPUT"', "")
        process = subprocess.run(
            ["bash", "-euo", "pipefail", "-c", script], check=True, capture_output=True, text=True,
            env={**os.environ, "BUILD_MATRIX": "all", "OS_SELECTOR": "all", "ARCH_SELECTOR": "all",
                 "EDITION": "community", "BUILD_TYPE": "Release", "PACKAGING": "auto",
                 "DISTRIBUTION": "all", "RUN_NUMBER": "1"},
        )
        matrix = json.loads(process.stdout.split("matrix=", 1)[1])["include"]
        self.assertEqual(len(matrix), 4)
        for lane in matrix:
            with self.subTest(lane=lane["id"]):
                self.assertIn("download-tinyllama-hf.sh", lane["setup_command"])
                self.assertIn("-DTHEMIS_DOCS_DB_MODE=BUILD", lane["configure_command"])
                self.assertIn("-DTHEMIS_MODELS_MODE=PACKAGE", lane["configure_command"])
                self.assertIn("--target docs_database", lane["validation_command"])
                self.assertIn("--component documentation", lane["validation_command"])
                self.assertIn("--component models", lane["validation_command"])
                self.assertIn('"${PACKAGES_DIR}" --packages', lane["validation_command"])
                self.assertFalse(lane["validation_continue_on_error"])
                self.assertLess(lane["validation_command"].index("--target docs_database"),
                                lane["validation_command"].index("cpack --config"))

    def test_docker_copies_models_out_of_cache_mount_and_includes_database_in_both_images(self):
        dockerfile = (ROOT / "docker/Dockerfile.unified").read_text()
        self.assertNotIn("-DTHEMIS_DOCS_DB_MODE=OFF", dockerfile)
        self.assertIn("--target \"${THEMIS_BUILD_TARGET}\" docs_database", dockerfile)
        self.assertIn('cp "/opt/themis-models/${MODEL_FILE}" /opt/bundled-models/', dockerfile)
        self.assertIn("cp /opt/themis-models/tinyllama.gguf /opt/bundled-models/tinyllama.gguf", dockerfile)
        self.assertIn("validate_runtime_payload.py /opt/themis-runtime", dockerfile)
        provisioning = dockerfile.split(
            "RUN --mount=type=cache,id=llm-model-cache-${TARGETARCH},target=/opt/themis-models \\\n", 1
        )[1].split("\n\n", 1)[0].replace("\\\n", "\n")
        subprocess.run(["sh", "-n"], input=provisioning, text=True, check=True, capture_output=True)
        for stage in ["runtime", "debug"]:
            body = dockerfile.split(f"FROM ubuntu:24.04 AS {stage}", 1)[1].split("\nFROM ", 1)[0]
            self.assertIn("/opt/themis-runtime/data/docs.db /app/data/docs.db", body)
            self.assertIn("/opt/bundled-models/ /models/", body)


if __name__ == "__main__":
    unittest.main()
