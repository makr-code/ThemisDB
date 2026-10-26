from datetime import datetime
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from dashboard import build_dashboard, load_news, load_prices
from models import StrategyProfile


class DashboardTests(unittest.TestCase):
    def test_dashboard_png_is_generated(self) -> None:
        news = load_news(ROOT / "data" / "news_realistic.csv")
        prices = load_prices(ROOT / "data" / "prices_realistic.csv")
        with tempfile.TemporaryDirectory() as tmpdir:
            output = Path(tmpdir) / "dashboard.png"
            build_dashboard(news, prices, [], output, StrategyProfile.LONG_TERM)
            self.assertTrue(output.exists())
            self.assertGreater(output.stat().st_size, 1024)


if __name__ == "__main__":
    unittest.main()
