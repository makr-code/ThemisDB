#!/usr/bin/env python3

import shutil
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent


@unittest.skipIf(sys.platform == "win32", "requires OpenLDAP, not WinLDAP")
class LDAPAuthenticatorCompileRegressionTests(unittest.TestCase):
    def test_clang_compiles_with_deprecated_openldap_apis_disabled(self) -> None:
        compiler = shutil.which("clang++")
        if compiler is None:
            self.skipTest("Requires clang++")

        flags = [
            "-std=c++20", "-fsyntax-only", "-DLDAP_DEPRECATED=0",
            "-DSPDLOG_COMPILED_LIB", "-DSPDLOG_FMT_EXTERNAL",
            "-DSPDLOG_DISABLE_DEFAULT_LOGGER", "-DSPDLOG_NO_EXCEPTIONS",
        ]
        # Probe external headers separately so broken repository code fails
        # rather than being mistaken for unavailable development packages.
        for header in ("ldap.h", "lber.h", "spdlog/spdlog.h", "fmt/format.h",
                       "openssl/evp.h"):
            probe = subprocess.run(
                [compiler, *flags, "-x", "c++", "-"],
                input=f"#include <{header}>\n",
                capture_output=True, text=True, timeout=30,
            )
            if probe.returncode != 0:
                self.skipTest(
                    f"Requires usable dependency header <{header}> "
                    "(libldap2-dev, libspdlog-dev, libfmt-dev, libssl-dev): "
                    f"{probe.stderr.strip()}"
                )

        # Compile the complete production LDAP path, including pagination.
        # The default non-LDAP path would not catch obsolete OpenLDAP calls.
        command = [
            compiler, *flags, "-DTHEMIS_HAS_LDAP=1",
            "-I", str(REPO_ROOT / "include"),
            "-I", str(REPO_ROOT / "src"),
            str(REPO_ROOT / "src/auth/ldap_authenticator.cpp"),
        ]
        result = subprocess.run(
            command, capture_output=True, text=True, timeout=120,
        )
        self.assertEqual(
            result.returncode, 0,
            f"LDAP-enabled Clang compilation failed:\n{result.stdout}{result.stderr}",
        )


if __name__ == "__main__":
    unittest.main()
