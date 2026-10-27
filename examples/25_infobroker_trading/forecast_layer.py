"""Hybrid forecast layer for the infobroker prototype.

This layer demonstrates the practical ThemisDB pattern: event semantics are
combined with a numeric time-series forecast before the final action is decided.
"""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from typing import Iterable, List

from models import ForecastDirection, ForecastPrediction, MarketBar, NewsItem


@dataclass(frozen=True)
class _SymbolSeries:
    timestamps: List[datetime]
    closes: List[float]


def _group_bars(bars: Iterable[MarketBar]) -> dict[str, _SymbolSeries]:
    grouped: dict[str, list[MarketBar]] = {}
    for bar in bars:
        grouped.setdefault(bar.symbol, []).append(bar)
    result: dict[str, _SymbolSeries] = {}
    for symbol, values in grouped.items():
        ordered = sorted(values, key=lambda item: item.timestamp)
        result[symbol] = _SymbolSeries(
            timestamps=[item.timestamp for item in ordered],
            closes=[float(item.close) for item in ordered],
        )
    return result


def _recent_return(series: _SymbolSeries, horizon_days: int) -> float:
    if len(series.closes) < 2:
        return 0.0
    latest = series.closes[-1]
    anchor = series.closes[-max(2, min(len(series.closes), horizon_days + 1))]
    return ((latest - anchor) / anchor) * 100.0


def _event_sentiment(news: NewsItem) -> float:
    text = f"{news.headline} {news.body}".lower()
    positive = sum(text.count(word) for word in ("growth", "upgrade", "beat", "strong", "expansion", "demand"))
    negative = sum(text.count(word) for word in ("miss", "decline", "warning", "drop", "loss", "bottleneck"))
    return (positive - negative) / max(1, positive + negative)


def build_forecasts(
    news_items: Iterable[NewsItem],
    market_bars: Iterable[MarketBar],
    horizons: Iterable[int] = (1, 3, 7),
) -> List[ForecastPrediction]:
    """Create a hybrid forecast object per news item from time-series and event context."""

    series_by_symbol = _group_bars(market_bars)
    predictions: List[ForecastPrediction] = []

    for item in news_items:
        series = series_by_symbol.get(item.symbol)
        if not series:
            continue

        sentiment = _event_sentiment(item)
        recent_return = _recent_return(series, 7)
        for horizon in horizons:
            expected_return = recent_return * 0.55 + sentiment * 100.0
            if expected_return > 1.5:
                direction = ForecastDirection.BULLISH
            elif expected_return < -1.5:
                direction = ForecastDirection.BEARISH
            else:
                direction = ForecastDirection.NEUTRAL

            confidence = min(0.99, max(0.15, abs(expected_return) / 12.0 + item.source_reliability * 0.35))
            predictions.append(
                ForecastPrediction(
                    symbol=item.symbol,
                    exchange=item.exchange,
                    generated_at=item.timestamp,
                    horizon_days=horizon,
                    expected_return_pct=expected_return,
                    confidence=confidence,
                    direction=direction,
                    event_sentiment=sentiment,
                    recent_return_pct=recent_return,
                    rationale=(
                        f"event_sentiment={sentiment:.2f}, recent_return={recent_return:.2f}% "
                        f"source_reliability={item.source_reliability:.2f}"
                    ),
                )
            )

    return predictions
