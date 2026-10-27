#!/usr/bin/env python3
"""
News infobroker strategy runner with historical validation.
"""

from __future__ import annotations

import argparse
import csv
import json
from datetime import datetime
from pathlib import Path
from typing import List

from backtest import run_backtest
from forecast_eval import evaluate_forecasts, load_labels
from forecast_layer import build_forecasts
from models import MarketBar, NewsItem, StrategyProfile
from news_ingestion import default_global_sources, fetch_global_news, filter_sources_by_region, write_news_csv
from signal_engine import generate_signals
from themis_export import export_jsonl


def _parse_ts(value: str) -> datetime:
    return datetime.fromisoformat(value.replace("Z", "+00:00"))


def load_news(path: Path) -> List[NewsItem]:
    items: List[NewsItem] = []
    with path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            items.append(
                NewsItem(
                    id=row["id"],
                    timestamp=_parse_ts(row["timestamp"]),
                    source=row["source"],
                    symbol=row["symbol"],
                    exchange=row["exchange"],
                    headline=row["headline"],
                    body=row["body"],
                    source_reliability=float(row["source_reliability"]),
                )
            )
    return items


def load_prices(path: Path) -> List[MarketBar]:
    bars: List[MarketBar] = []
    with path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            bars.append(
                MarketBar(
                    symbol=row["symbol"],
                    exchange=row["exchange"],
                    timestamp=_parse_ts(row["timestamp"]),
                    close=float(row["close"]),
                )
            )
    return bars


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ThemisDB Infobroker Trading Example")
    parser.add_argument(
        "--profile",
        choices=[profile.value for profile in StrategyProfile],
        default=StrategyProfile.LONG_TERM.value,
        help="Signal profile: high-frequency or long-term horizon.",
    )
    parser.add_argument(
        "--news-csv",
        default="sample_data/news_realistic.csv",
        help="Path to historical news feed.",
    )
    parser.add_argument(
        "--price-csv",
        default="sample_data/prices_realistic.csv",
        help="Path to historical prices feed.",
    )
    parser.add_argument(
        "--min-confidence",
        type=float,
        default=0.45,
        help="Minimum confidence for trade execution.",
    )
    parser.add_argument(
        "--export-themis-jsonl",
        default="",
        help="Optional output directory for ThemisDB import packets.",
    )
    parser.add_argument(
        "--labels-csv",
        default="",
        help="Optional labeled event dataset for forecast quality evaluation.",
    )
    parser.add_argument(
        "--forecast-report-json",
        default="",
        help="Optional output file path for forecast evaluation metrics as JSON.",
    )
    parser.add_argument(
        "--fetch-global-news",
        action="store_true",
        help="Fetch global RSS/Atom news feeds and use them as input events.",
    )
    parser.add_argument(
        "--global-news-regions",
        default="",
        help="Comma-separated source regions (global,europe,apac,americas,mena). Empty means all.",
    )
    parser.add_argument(
        "--global-news-max-items",
        type=int,
        default=20,
        help="Maximum feed items to ingest per source.",
    )
    parser.add_argument(
        "--global-news-output-csv",
        default="",
        help="Optional CSV output path for fetched global news events.",
    )
    parser.add_argument(
        "--fetch-only",
        action="store_true",
        help="Only fetch and persist global news, then exit.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    profile = StrategyProfile(args.profile)

    if args.fetch_global_news:
        sources = default_global_sources()
        regions = [value.strip() for value in args.global_news_regions.split(",")] if args.global_news_regions else []
        sources = filter_sources_by_region(sources, regions)
        if not sources:
            raise ValueError("no global news sources selected for the requested region filter")
        news = fetch_global_news(sources, max_items_per_source=args.global_news_max_items)
        print(f"Fetched global news   : {len(news)}")
        if args.global_news_output_csv:
            output_path = Path(args.global_news_output_csv)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            write_news_csv(str(output_path), news)
            print(f"Global news CSV       : {output_path}")
        if args.fetch_only:
            return
    else:
        news = load_news(Path(args.news_csv))
    prices = load_prices(Path(args.price_csv))
    signals = generate_signals(news, profile=profile, min_confidence=args.min_confidence)
    forecasts = build_forecasts(news, prices)
    report = run_backtest(profile=profile, signals=signals, market_bars=prices)

    print("=== ThemisDB Infobroker Trading Validation ===")
    print(f"Profile               : {profile.value}")
    print(f"News events           : {len(news)}")
    print(f"Generated signals     : {len(signals)}")
    print(f"Forecast records      : {len(forecasts)}")
    print(f"Executed trades       : {len(report.trades)}")
    print(f"Skipped signals       : {report.skipped_signals}")
    print(f"Strategy return       : {report.strategy_return_pct:.2%}")
    print(f"Benchmark return      : {report.benchmark_return_pct:.2%}")
    print(f"Win rate              : {report.win_rate:.2%}")
    print(f"Average trade return  : {report.avg_return_pct:.2%}")
    print(f"Max drawdown          : {report.max_drawdown_pct:.2%}")
    print(f"Sharpe-like ratio     : {report.sharpe_like:.3f}")
    if forecasts:
        top = max(forecasts, key=lambda item: abs(item.expected_return_pct))
        print(f"Strongest forecast    : {top.symbol} {top.direction.value} {top.expected_return_pct:.2f}% / {top.horizon_days}d")
    if args.labels_csv:
        labels = load_labels(args.labels_csv)
        eval_report = evaluate_forecasts(forecasts, labels)
        print(f"Forecast compared     : {eval_report.compared}")
        print(f"Forecast coverage     : {eval_report.coverage:.2%}")
        print(f"Directional accuracy  : {eval_report.directional_accuracy:.2%}")
        print(f"Forecast MAE (pct)    : {eval_report.mean_abs_error_pct:.2f}")
        if args.forecast_report_json:
            report_path = Path(args.forecast_report_json)
            report_path.parent.mkdir(parents=True, exist_ok=True)
            payload = {
                "profile": profile.value,
                "compared": eval_report.compared,
                "coverage": eval_report.coverage,
                "directional_accuracy": eval_report.directional_accuracy,
                "mean_abs_error_pct": eval_report.mean_abs_error_pct,
            }
            with report_path.open("w", encoding="utf-8") as handle:
                json.dump(payload, handle, indent=2)
            print(f"Forecast report JSON  : {report_path}")

    if args.export_themis_jsonl:
        export_dir = Path(args.export_themis_jsonl)
        export_jsonl(export_dir, news, prices, signals)
        print(f"Exported ThemisDB packets to: {export_dir}")


if __name__ == "__main__":
    main()
