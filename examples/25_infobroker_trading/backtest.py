"""
Historical validation pipeline for signal quality.
"""

from __future__ import annotations

from bisect import bisect_left
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime
from statistics import mean, pstdev
from typing import Dict, Iterable, List, Sequence, Tuple

from models import (
    BacktestReport,
    ExecutedTrade,
    MarketBar,
    StrategyProfile,
    TradeAction,
    TradeSignal,
)


@dataclass(frozen=True)
class _SymbolSeries:
    times: Sequence[datetime]
    prices: Sequence[float]


def _index_bars(market_bars: Iterable[MarketBar]) -> Dict[Tuple[str, str], _SymbolSeries]:
    grouped: Dict[Tuple[str, str], List[MarketBar]] = defaultdict(list)
    for bar in market_bars:
        grouped[(bar.symbol, bar.exchange)].append(bar)

    indexed: Dict[Tuple[str, str], _SymbolSeries] = {}
    for key, bars in grouped.items():
        ordered = sorted(bars, key=lambda bar: bar.timestamp)
        indexed[key] = _SymbolSeries(
            times=[bar.timestamp for bar in ordered],
            prices=[bar.close for bar in ordered],
        )
    return indexed


def _find_price(series: _SymbolSeries, ts: datetime) -> tuple[datetime, float] | None:
    idx = bisect_left(series.times, ts)
    if idx >= len(series.times):
        return None
    return series.times[idx], series.prices[idx]


def _return_pct(action: TradeAction, entry: float, exit_price: float) -> float:
    raw = (exit_price - entry) / entry
    if action == TradeAction.SELL:
        return -raw
    return raw


def _max_drawdown(cumulative_returns: Sequence[float]) -> float:
    peak = float("-inf")
    max_drawdown = 0.0
    for value in cumulative_returns:
        peak = max(peak, value)
        drawdown = peak - value
        max_drawdown = max(max_drawdown, drawdown)
    return max_drawdown


def _benchmark_return(market_bars: Iterable[MarketBar]) -> float:
    grouped: Dict[Tuple[str, str], List[MarketBar]] = defaultdict(list)
    for bar in market_bars:
        grouped[(bar.symbol, bar.exchange)].append(bar)
    if not grouped:
        return 0.0

    symbol_returns: List[float] = []
    for bars in grouped.values():
        ordered = sorted(bars, key=lambda bar: bar.timestamp)
        if len(ordered) < 2:
            continue
        symbol_returns.append((ordered[-1].close - ordered[0].close) / ordered[0].close)

    if not symbol_returns:
        return 0.0
    return mean(symbol_returns)


def run_backtest(
    profile: StrategyProfile,
    signals: Iterable[TradeSignal],
    market_bars: Iterable[MarketBar],
) -> BacktestReport:
    """
    Executes long/short signals over historical bars.
    """

    bars = list(market_bars)
    signal_list = list(signals)
    indexed = _index_bars(bars)

    trades: List[ExecutedTrade] = []
    skipped = 0
    for signal in signal_list:
        if signal.action == TradeAction.HOLD:
            skipped += 1
            continue

        series = indexed.get((signal.symbol, signal.exchange))
        if series is None:
            skipped += 1
            continue

        entry = _find_price(series, signal.generated_at)
        exit_point = _find_price(series, signal.generated_at + signal.holding_period)
        if entry is None or exit_point is None:
            skipped += 1
            continue

        entry_ts, entry_price = entry
        exit_ts, exit_price = exit_point
        trades.append(
            ExecutedTrade(
                signal=signal,
                entry_price=entry_price,
                exit_price=exit_price,
                return_pct=_return_pct(signal.action, entry_price, exit_price),
                entry_time=entry_ts,
                exit_time=exit_ts,
            )
        )

    strategy_returns = [trade.return_pct for trade in trades]
    cumulative = []
    running = 0.0
    for result in strategy_returns:
        running += result
        cumulative.append(running)

    strategy_return = running
    wins = sum(1 for result in strategy_returns if result > 0.0)
    win_rate = wins / len(strategy_returns) if strategy_returns else 0.0
    avg_return = mean(strategy_returns) if strategy_returns else 0.0
    std = pstdev(strategy_returns) if len(strategy_returns) > 1 else 0.0
    sharpe_like = avg_return / std if std > 0 else 0.0
    drawdown = _max_drawdown(cumulative) if cumulative else 0.0

    return BacktestReport(
        profile=profile,
        trades=trades,
        skipped_signals=skipped,
        benchmark_return_pct=_benchmark_return(bars),
        strategy_return_pct=strategy_return,
        win_rate=win_rate,
        avg_return_pct=avg_return,
        max_drawdown_pct=drawdown,
        sharpe_like=sharpe_like,
    )
