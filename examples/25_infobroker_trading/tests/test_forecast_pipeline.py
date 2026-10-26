from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from forecast_eval import evaluate_forecasts, load_labels
from forecast_layer import build_forecasts
from main import load_news, load_prices


class ForecastPipelineTests(unittest.TestCase):
    def test_build_forecasts_generates_records_per_horizon(self) -> None:
        news = load_news(ROOT / "data" / "news_realistic.csv")
        prices = load_prices(ROOT / "data" / "prices_realistic.csv")
        forecasts = build_forecasts(news, prices, horizons=(1, 3))
        self.assertEqual(len(forecasts), len(news) * 2)

    def test_forecast_evaluation_compares_against_labels(self) -> None:
        news = load_news(ROOT / "data" / "news_realistic.csv")
        prices = load_prices(ROOT / "data" / "prices_realistic.csv")
        labels = load_labels(str(ROOT / "labels_training.csv"))
        forecasts = build_forecasts(news, prices, horizons=(3,))
        report = evaluate_forecasts(forecasts, labels)
        self.assertGreater(report.compared, 0)
        self.assertGreater(report.coverage, 0.5)
        self.assertGreaterEqual(report.directional_accuracy, 0.0)
        self.assertGreaterEqual(report.mean_abs_error_pct, 0.0)

    def test_cli_writes_forecast_report_json(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            output = Path(tmpdir) / "forecast_report.json"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "main.py"),
                    "--profile",
                    "long_term",
                    "--news-csv",
                    str(ROOT / "data" / "news_realistic.csv"),
                    "--price-csv",
                    str(ROOT / "data" / "prices_realistic.csv"),
                    "--labels-csv",
                    str(ROOT / "labels_training.csv"),
                    "--forecast-report-json",
                    str(output),
                ],
                check=True,
                cwd=str(ROOT),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            self.assertTrue(output.exists())
            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertIn("directional_accuracy", payload)
            self.assertIn("mean_abs_error_pct", payload)


if __name__ == "__main__":
    unittest.main()
