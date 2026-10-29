#!/usr/bin/env python3

import os
import re
import subprocess
import unittest
from pathlib import Path


DOCKERFILE = Path(__file__).resolve().parent.parent / "docker" / "Dockerfile.unified"


def model_provisioning_command() -> str:
    text = DOCKERFILE.read_text(encoding="utf-8")
    match = re.search(
        r"^RUN --mount=type=cache,id=llm-model-cache-\$\{TARGETARCH\},target=/opt/themis-models \\\n"
        r"(?P<body>(?:[^\n]*\\\n)*[^\n]*)",
        text,
        re.MULTILINE,
    )
    if match is None:
        raise AssertionError("Could not find the model-provisioning RUN instruction")
    return match.group("body").replace("\\\n", "")


@unittest.skipUnless(Path("/bin/sh").is_file(), "Requires the Docker build's POSIX shell")
class DockerModelShellRegressionTests(unittest.TestCase):
    def test_model_provisioning_has_valid_shell_syntax(self) -> None:
        result = subprocess.run(
            ["/bin/sh", "-n", "-c", model_provisioning_command()],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_syntax_check_rejects_original_single_semicolon_regression(self) -> None:
        command = model_provisioning_command()
        self.assertIn(";;", command)
        result = subprocess.run(
            ["/bin/sh", "-n", "-c", command.replace(";;", ";", 1)],
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Syntax error", result.stderr)

    def test_non_download_branches_execute_without_syntax_errors(self) -> None:
        for enabled, model, expected_status, expected_message in (
            ("OFF", "tinyllama", 0, "INCLUDE_TINYLLAMA is disabled"),
            ("ON", "custom", 0, "skipping model download"),
            ("ON", "none", 0, "skipping model download"),
            ("ON", "disabled", 0, "skipping model download"),
            ("ON", "unknown-model", 1, "No built-in URL exists"),
        ):
            with self.subTest(enabled=enabled, model=model):
                result = subprocess.run(
                    ["/bin/sh", "-c", model_provisioning_command()],
                    env={
                        **os.environ,
                        "INCLUDE_TINYLLAMA": enabled,
                        "LLM_MODEL": model,
                        "LLM_MODEL_URL": "",
                        "TINYLLAMA_HF_URL": "",
                    },
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(result.returncode, expected_status, result.stderr)
                self.assertIn(expected_message, result.stdout)


if __name__ == "__main__":
    unittest.main()
