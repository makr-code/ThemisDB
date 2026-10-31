import importlib.util
import sys
import unittest
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "ai-dev-llm-wiki-sync.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("ai_dev_llm_wiki_sync", _SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class CuratedModuleIndexTests(unittest.TestCase):
    def test_importers_context_is_kept_in_generated_indexes(self):
        wiki_sync = _load_module()

        index = wiki_sync.render_index([], "2026-10-31T00:00:00+00:00", False, "digest")
        modules = wiki_sync.render_modules_and_apis(Path(__file__).parent.parent, [])

        self.assertIn("[IMPORTERS.md](IMPORTERS.md)", index)
        self.assertIn("[Importer module context](IMPORTERS.md)", modules)


if __name__ == "__main__":
    unittest.main()
