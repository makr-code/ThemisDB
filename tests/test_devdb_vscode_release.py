import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import yaml


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / ".github/scripts/devdb_vscode_release.py"
spec = importlib.util.spec_from_file_location("devdb_vscode_release", SCRIPT)
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)

REQUEST = {
    "EVENT_NAME": "workflow_dispatch",
    "REPOSITORY": "makr-code/ThemisDB",
    "REQUEST_PUBLISH": "true",
    "REF_PROTECTED": "true",
    "REF": "refs/tags/devdb-v1.2.3",
}
MANIFEST = {
    "publisher": "makr-code",
    "name": "devdb",
    "version": "1.2.3",
    "scripts": {"build": "build", "test": "test"},
}


class DevdbReleaseTests(unittest.TestCase):
    def test_request_rejects_untrusted_events_and_refs(self):
        release.validate_request(REQUEST)
        for change in (
            {"EVENT_NAME": "pull_request"},
            {"EVENT_NAME": "push"},
            {"REPOSITORY": "fork/ThemisDB"},
            {"REF_PROTECTED": "false"},
            {"REF": "refs/heads/develop"},
            {"REF": "refs/tags/devdb-v1.2.3-rc1"},
            {"REQUEST_PUBLISH": ""},
        ):
            with self.subTest(change=change), self.assertRaises(ValueError):
                release.validate_request({**REQUEST, **change})

    def test_preview_does_not_require_a_release_tag(self):
        release.validate_request({**REQUEST, "REQUEST_PUBLISH": "false", "REF": "refs/heads/develop"})

    def test_missing_extension_and_missing_scripts_fail(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(release, "EXTENSION", Path(tmp)):
            with self.assertRaisesRegex(ValueError, "prerequisite missing"):
                release.read_source(REQUEST)
            (Path(tmp) / "package.json").write_text(json.dumps({**MANIFEST, "scripts": {}}))
            (Path(tmp) / "package-lock.json").write_text("{}")
            with self.assertRaisesRegex(ValueError, "build and focused test"):
                release.read_source(REQUEST)

    def test_identity_and_version_mismatches_fail(self):
        for change in (
            {"publisher": "other"},
            {"version": "1.2.4"},
            {"version": "1.2.3-rc1"},
            {"name": "../bad"},
        ):
            with self.subTest(change=change), self.assertRaises(ValueError):
                release.validate_manifest({**MANIFEST, **change}, REQUEST)

    def test_packaged_manifest_must_match_source(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(release, "EXTENSION", Path(tmp)):
            root = Path(tmp)
            (root / "package.json").write_text(json.dumps(MANIFEST))
            (root / "package-lock.json").write_text("{}")
            vsix = root / "extension.vsix"
            for name, valid in (("devdb", True), ("other", False)):
                with zipfile.ZipFile(vsix, "w") as archive:
                    archive.writestr("extension/package.json", json.dumps({**MANIFEST, "name": name}))
                if valid:
                    release.validate_artifact(vsix, REQUEST)
                else:
                    with self.assertRaisesRegex(ValueError, "identity differs"):
                        release.validate_artifact(vsix, REQUEST)

    def test_workflow_publish_is_downstream_and_oidc_is_isolated(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/release-devdb-vscode.yml").read_text())
        triggers = workflow.get(True, workflow.get("on"))
        self.assertEqual(set(triggers), {"workflow_dispatch"})
        self.assertFalse(triggers["workflow_dispatch"]["inputs"]["publish"]["default"])
        build, publish = (workflow["jobs"][name] for name in ("build", "publish"))
        self.assertEqual(publish["needs"], ["build"])
        self.assertNotIn("id-token", build["permissions"])
        self.assertEqual(publish["permissions"]["id-token"], "write")
        for guard in ("workflow_dispatch", "makr-code/ThemisDB", "inputs.publish == true", "github.ref_type == 'tag'", "github.ref_protected == true"):
            self.assertIn(guard, publish["if"])
        self.assertEqual(publish["environment"], "devdb-marketplace-production")
        self.assertFalse(any("continue-on-error" in step for step in build["steps"]))
        build_commands = "\n".join(step.get("run", "") for step in build["steps"])
        publish_commands = "\n".join(step.get("run", "") for step in publish["steps"])
        for command in ("request", "source", "npm run build", "npm test", "vsce", "artifact"):
            self.assertIn(command, build_commands)
        self.assertNotIn("publish --oidc", build_commands)
        self.assertIn("sha256sum --check", publish_commands)
        self.assertIn("publish --oidc --packagePath", publish_commands)
        self.assertNotIn("--skip-duplicate", publish_commands)


if __name__ == "__main__":
    unittest.main()
