"""The retired comparative entry point must not produce unsupported results."""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from benchmarks.specialized_comparative_benchmarks import RETIREMENT_MESSAGE, main


class SpecializedComparativeBenchmarkTests(unittest.TestCase):
    def test_runner_fails_explicitly_without_network_or_report_side_effects(self):
        with tempfile.TemporaryDirectory() as directory:
            stdout = io.StringIO()
            stderr = io.StringIO()
            with (
                patch(
                    "benchmarks.specialized_comparative_benchmarks.requests",
                    create=True,
                ) as requests,
                patch(
                    "benchmarks.specialized_comparative_benchmarks.os",
                    create=True,
                ) as os_module,
                contextlib.redirect_stdout(stdout),
                contextlib.redirect_stderr(stderr),
            ):
                exit_code = main([])

            self.assertEqual(exit_code, 2)
            self.assertEqual(stdout.getvalue(), "")
            self.assertIn(RETIREMENT_MESSAGE, stderr.getvalue())
            requests.get.assert_not_called()
            requests.post.assert_not_called()
            os_module.makedirs.assert_not_called()
            self.assertEqual(list(Path(directory).iterdir()), [])

    def test_help_remains_available(self):
        with contextlib.redirect_stdout(io.StringIO()) as stdout:
            with self.assertRaises(SystemExit) as exit_info:
                main(["--help"])

        self.assertEqual(exit_info.exception.code, 0)
        self.assertIn("Retired", stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
