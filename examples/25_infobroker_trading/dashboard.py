#!/usr/bin/env python3
"""Dashboard and visual export for the infobroker trading demo."""

from __future__ import annotations

import argparse
import csv
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Iterable, List

import matplotlib

matplotlib.use("Agg")
from matplotlib import pyplot as plt

from backtest import run_backtest
from models import MarketBar, NewsItem, StrategyProfile, TradeAction
from signal_engine import generate_signals


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


def _price_by_symbol(prices: Iterable[MarketBar]) -> dict[str, List[MarketBar]]:
    grouped: dict[str, List[MarketBar]] = {}
    for bar in prices:
        grouped.setdefault(bar.symbol, []).append(bar)
    for symbol in grouped:
        grouped[symbol].sort(key=lambda item: item.timestamp)
    return grouped


def _nearest_price(series: List[MarketBar], ts: datetime) -> float:
    if not series:
        return float("nan")
    return min(series, key=lambda bar: abs((bar.timestamp - ts).total_seconds())).close


def build_dashboard(
    news_items: Iterable[NewsItem],
    market_bars: Iterable[MarketBar],
    signals: Iterable,
    output_path: Path,
    profile: StrategyProfile,
    report=None,
) -> Path:
    """Render a richer PNG dashboard with pricing, event timeline, and pipeline metrics."""
    prices = list(market_bars)
    series_by_symbol = _price_by_symbol(prices)
    signal_list = list(signals)
    all_news = list(news_items)

    if report is None:
        report = run_backtest(profile=profile, signals=signal_list, market_bars=prices)

    fig, axes = plt.subplots(3, 1, figsize=(14, 11), gridspec_kw={"height_ratios": [3, 1, 1.05]})
    ax_main, ax_events, ax_summary = axes

    for symbol, bars in sorted(series_by_symbol.items()):
        timestamps = [bar.timestamp for bar in bars]
        closes = [bar.close for bar in bars]
        ax_main.plot(timestamps, closes, label=symbol, linewidth=2)

    for signal in signal_list:
        if signal.action == TradeAction.HOLD:
            continue
        series = series_by_symbol.get(signal.symbol, [])
        if not series:
            continue
        price = _nearest_price(series, signal.generated_at)
        color = "tab:green" if signal.action == TradeAction.BUY else "tab:red"
        ax_main.scatter(signal.generated_at, price, s=80, color=color, zorder=5)
        ax_main.annotate(
            f"{signal.action.value.upper()}\n{signal.confidence:.2f}",
            (signal.generated_at, price),
            textcoords="offset points",
            xytext=(8, 8),
            fontsize=8,
            color=color,
            bbox={"boxstyle": "round,pad=0.2", "fc": "white", "alpha": 0.75},
        )
        ax_events.axvline(signal.generated_at, color=color, linestyle="--", alpha=0.5)

    for item in all_news:
        ax_events.axvline(item.timestamp, color="gray", linestyle="-", linewidth=0.8, alpha=0.4)
        ax_events.text(item.timestamp, 0.55, item.symbol, rotation=90, va="center", ha="center", fontsize=7)

    ax_main.set_title(f"ThemisDB Infobroker Dashboard [{profile.value}]")
    ax_main.set_ylabel("Price (local close)")
    ax_main.grid(True, alpha=0.25)
    ax_main.legend(loc="upper left", fontsize=9)

    ax_events.set_ylim(0, 1)
    ax_events.set_yticks([])
    ax_events.set_ylabel("News / Signals")
    ax_events.set_xlabel("Time")
    ax_events.grid(False)

    summary_lines = [
        "Pipeline overview",
        f"News events: {len(all_news)}",
        f"Signals: {len(signal_list)} | Executed trades: {len(report.trades)}",
        f"Strategy return: {report.strategy_return_pct:.2%}",
        f"Benchmark return: {report.benchmark_return_pct:.2%}",
        f"Win rate: {report.win_rate:.2%}",
        f"Max drawdown: {report.max_drawdown_pct:.2%}",
        f"Sharpe-like: {report.sharpe_like:.3f}",
        f"Skipped signals: {report.skipped_signals}",
    ]
    ax_summary.axis("off")
    ax_summary.text(
        0.02,
        0.98,
        "\n".join(summary_lines),
        va="top",
        ha="left",
        fontsize=9,
        family="monospace",
        transform=ax_summary.transAxes,
    )
    ax_summary.set_title("Evaluation summary", fontsize=10)

    fig.autofmt_xdate()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(output_path, dpi=180)
    plt.close(fig)
    return output_path


def launch_window(news_items: Iterable[NewsItem], market_bars: Iterable[MarketBar], signals: Iterable, profile: StrategyProfile) -> None:
    """Open a Tkinter window if a display is available."""
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        raise RuntimeError("No DISPLAY available for windowed mode; use --dashboard-output to save PNG instead.")

    try:
        import tkinter as tk
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    except Exception as exc:  # pragma: no cover - GUI dependency may be absent
        raise RuntimeError(f"Tkinter/Matplotlib GUI support is unavailable: {exc}") from exc

    prices = list(market_bars)
    signal_list = list(signals)
    figure = plt.figure(figsize=(13, 8))
    ax_main = figure.add_subplot(211)
    ax_events = figure.add_subplot(212)

    series_by_symbol = _price_by_symbol(prices)
    for symbol, bars in sorted(series_by_symbol.items()):
        ax_main.plot([bar.timestamp for bar in bars], [bar.close for bar in bars], label=symbol)
    for signal in signal_list:
        if signal.action == TradeAction.HOLD:
            continue
        series = series_by_symbol.get(signal.symbol, [])
        if not series:
            continue
        price = _nearest_price(series, signal.generated_at)
        color = "tab:green" if signal.action == TradeAction.BUY else "tab:red"
        ax_main.scatter(signal.generated_at, price, s=80, color=color)
    for item in news_items:
        ax_events.axvline(item.timestamp, color="gray", alpha=0.5)

    ax_main.set_title(f"ThemisDB Infobroker Dashboard [{profile.value}]")
    ax_main.legend(loc="upper left")
    ax_events.set_ylim(0, 1)
    ax_events.set_yticks([])
    ax_events.set_xlabel("Time")

    root = tk.Tk()
    root.title("ThemisDB Infobroker Dashboard")
    canvas = FigureCanvasTkAgg(figure, master=root)
    canvas.draw()
    canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
    root.mainloop()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Visual dashboard generator for infobroker trading demo")
    parser.add_argument("--news-csv", default="sample_data/news_realistic.csv", help="Path to the news CSV.")
    parser.add_argument("--price-csv", default="sample_data/prices_realistic.csv", help="Path to the price CSV.")
    parser.add_argument("--profile", choices=[profile.value for profile in StrategyProfile], default=StrategyProfile.LONG_TERM.value)
    parser.add_argument("--min-confidence", type=float, default=0.45, help="Minimum confidence threshold for signal generation.")
    parser.add_argument("--dashboard-output", default="out/infobroker_dashboard.png", help="PNG output path for the dashboard.")
    parser.add_argument("--window", action="store_true", help="Open the dashboard in a Tkinter window instead of saving a PNG.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    news = load_news(Path(args.news_csv))
    prices = load_prices(Path(args.price_csv))
    profile = StrategyProfile(args.profile)
    signals = generate_signals(news, profile=profile, min_confidence=args.min_confidence)
    report = run_backtest(profile=profile, signals=signals, market_bars=prices)

    if args.window:
        launch_window(news, prices, signals, profile)
    else:
        output_path = Path(args.dashboard_output)
        build_dashboard(news, prices, signals, output_path, profile, report=report)
        print(f"Dashboard saved to {output_path}")
        print(f"Signals: {len(signals)} | Executed trades: {len(report.trades)} | Strategy return: {report.strategy_return_pct:.2%}")


if __name__ == "__main__":
    main()
