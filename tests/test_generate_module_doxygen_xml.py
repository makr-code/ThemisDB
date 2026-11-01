import tempfile
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from scripts.generate_module_doxygen_xml import write_module_markdown


class WriteModuleMarkdownTests(unittest.TestCase):
    def test_artifact_paths_are_relative_to_output_root(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir) / "repo"
            output_root = Path(temp_dir) / "doxygen-output"
            result = {
                "xml_index": "aql/xml/index.xml",
                "warnings_log": "aql/doxygen-warnings.log",
                "cpp_file_count": 1,
                "compound_total": 1,
                "class_struct_total": 0,
                "namespace_total": 0,
                "file_total": 1,
            }

            write_module_markdown(
                repo_root=repo_root,
                module="aql",
                result=result,
                details={"namespaces": [], "classes": [], "structs": [], "files": []},
                function_docs=[],
                generated_at="2026-10-31T00:00:00Z",
                output_root=output_root,
            )

            markdown = (repo_root / "src/aql/DOXYGEN.md").read_text(encoding="utf-8")
            self.assertIn("`aql/xml/index.xml`", markdown)
            self.assertIn("`aql/doxygen-warnings.log`", markdown)
            self.assertIn(
                "`python3 scripts/generate_module_doxygen_xml.py --repo-root . "
                "--modules aql --output-root build/module-doxygen "
                "--manifest build/module-doxygen/manifest.json "
                "--summary-md build/module-doxygen/summary.md --write-module-markdown`",
                markdown,
            )
            self.assertNotIn(str(output_root), markdown)
            self.assertTrue(markdown.endswith("\n"))
            self.assertFalse(markdown.endswith("\n\n"))


if __name__ == "__main__":
    unittest.main()
