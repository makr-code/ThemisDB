"""
Domain models for the news-driven infobroker trading example.
"""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
from typing import List


class StrategyProfile(str, Enum):
    """Supported strategy profiles."""

    HFT = "hft"
    LONG_TERM = "long_term"


class TradeAction(str, Enum):
    """Action recommendation."""

    BUY = "buy"
    SELL = "sell"
    HOLD = "hold"


class ForecastDirection(str, Enum):
    """Directional forecast label."""

    BULLISH = "bullish"
    BEARISH = "bearish"
    NEUTRAL = "neutral"


@dataclass(frozen=True)
class NewsItem:
    """Incoming normalized news event."""

    id: str
    timestamp: datetime
    source: str
    symbol: str
    exchange: str
    headline: str
    body: str
    source_reliability: float


@dataclass(frozen=True)
class MarketBar:
    """Historical price point."""

    symbol: str
    exchange: str
    timestamp: datetime
    close: float


@dataclass(frozen=True)
class TradeSignal:
    """LLM-style interpreted signal."""

    news_id: str
    symbol: str
    exchange: str
    generated_at: datetime
    action: TradeAction
    score: float
    confidence: float
    rationale: str
    holding_period: timedelta


@dataclass(frozen=True)
class ForecastPrediction:
    """Hybrid forecast combining price trend and event context."""

    symbol: str
    exchange: str
    generated_at: datetime
    horizon_days: int
    expected_return_pct: float
    confidence: float
    direction: ForecastDirection
    event_sentiment: float
    recent_return_pct: float
    rationale: str


@dataclass(frozen=True)
class EventLabel:
    """Training target for event-to-future-return classification."""

    news_id: str
    symbol: str
    exchange: str
    event_timestamp: datetime
    horizon_days: int
    future_return_pct: float
    label: str
    confidence: float
    rationale: str


@dataclass(frozen=True)
class ForecastEvaluationReport:
    """Forecast quality report against labeled outcomes."""

    compared: int
    directional_accuracy: float
    mean_abs_error_pct: float
    coverage: float


@dataclass(frozen=True)
class ExecutedTrade:
    """Trade execution derived from a signal and market data."""

    signal: TradeSignal
    entry_price: float
    exit_price: float
    return_pct: float
    entry_time: datetime
    exit_time: datetime


@dataclass(frozen=True)
class BacktestReport:
    """Summary of historical validation."""

    profile: StrategyProfile
    trades: List[ExecutedTrade]
    skipped_signals: int
    benchmark_return_pct: float
    strategy_return_pct: float
    win_rate: float
    avg_return_pct: float
    max_drawdown_pct: float
    sharpe_like: float
