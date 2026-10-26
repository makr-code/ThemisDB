from datetime import datetime, timedelta
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from backtest import run_backtest
from models import MarketBar, StrategyProfile, TradeAction, TradeSignal


class BacktestTests(unittest.TestCase):
    def test_long_and_short_returns(self) -> None:
        bars = [
            MarketBar("AAA", "XETRA", datetime.fromisoformat("2026-01-01T10:00:00+00:00"), 100.0),
            MarketBar("AAA", "XETRA", datetime.fromisoformat("2026-01-21T10:30:00+00:00"), 110.0),
            MarketBar("BBB", "XETRA", datetime.fromisoformat("2026-01-01T10:00:00+00:00"), 100.0),
            MarketBar("BBB", "XETRA", datetime.fromisoformat("2026-01-21T10:30:00+00:00"), 90.0),
        ]
        signals = [
            TradeSignal(
                news_id="a",
                symbol="AAA",
                exchange="XETRA",
                generated_at=datetime.fromisoformat("2026-01-01T10:00:00+00:00"),
                action=TradeAction.BUY,
                score=0.8,
                confidence=0.9,
                rationale="positive",
                holding_period=timedelta(days=20),
            ),
            TradeSignal(
                news_id="b",
                symbol="BBB",
                exchange="XETRA",
                generated_at=datetime.fromisoformat("2026-01-01T10:00:00+00:00"),
                action=TradeAction.SELL,
                score=-0.7,
                confidence=0.9,
                rationale="negative",
                holding_period=timedelta(days=20),
            ),
        ]
        report = run_backtest(StrategyProfile.LONG_TERM, signals, bars)
        self.assertEqual(len(report.trades), 2)
        self.assertGreater(report.strategy_return_pct, 0.0)
        self.assertGreaterEqual(report.win_rate, 1.0)


if __name__ == "__main__":
    unittest.main()
