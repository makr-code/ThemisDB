from datetime import datetime
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from models import NewsItem, StrategyProfile, TradeAction
from signal_engine import generate_signals


class SignalEngineTests(unittest.TestCase):
    def test_positive_news_creates_buy_signal(self) -> None:
        news = NewsItem(
            id="p1",
            timestamp=datetime.fromisoformat("2026-01-01T10:00:00+00:00"),
            source="Reuters",
            symbol="ABC",
            exchange="XETRA",
            headline="Earnings beat and guidance upgrade",
            body="Strong growth and profit outlook",
            source_reliability=0.9,
        )
        signal = generate_signals([news], StrategyProfile.LONG_TERM, min_confidence=0.4)[0]
        self.assertEqual(signal.action, TradeAction.BUY)
        self.assertGreater(signal.score, 0.0)

    def test_negative_news_creates_sell_signal(self) -> None:
        news = NewsItem(
            id="n1",
            timestamp=datetime.fromisoformat("2026-01-01T10:00:00+00:00"),
            source="Bloomberg",
            symbol="XYZ",
            exchange="NASDAQ",
            headline="Guidance miss and lawsuit risk",
            body="Analysts downgrade after decline",
            source_reliability=0.9,
        )
        signal = generate_signals([news], StrategyProfile.HFT, min_confidence=0.3)[0]
        self.assertEqual(signal.action, TradeAction.SELL)
        self.assertLess(signal.score, 0.0)


if __name__ == "__main__":
    unittest.main()
