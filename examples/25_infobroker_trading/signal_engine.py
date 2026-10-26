"""
Signal generation for the news-driven infobroker.
"""

from __future__ import annotations

import math
import re
from datetime import timedelta
from typing import Iterable, List

from models import NewsItem, StrategyProfile, TradeAction, TradeSignal

TOKEN_RE = re.compile(r"[a-zA-Z][a-zA-Z\-]+")

POSITIVE_LEXICON = {
    "beat": 1.4,
    "growth": 1.0,
    "upgrade": 1.3,
    "record": 0.8,
    "surge": 1.2,
    "profit": 1.0,
    "launch": 0.7,
    "contract": 0.6,
}

NEGATIVE_LEXICON = {
    "miss": 1.4,
    "downgrade": 1.3,
    "lawsuit": 1.0,
    "fraud": 1.6,
    "decline": 0.9,
    "drop": 1.0,
    "loss": 1.1,
    "probe": 1.2,
}

EVENT_MULTIPLIER = {
    "earnings": 1.3,
    "guidance": 1.2,
    "rate": 1.1,
    "merger": 1.2,
    "buyback": 1.1,
}


def _tokenize(text: str) -> List[str]:
    return [token.lower() for token in TOKEN_RE.findall(text)]


def _score_news(news: NewsItem) -> tuple[float, str]:
    tokens = _tokenize(f"{news.headline} {news.body}")
    if not tokens:
        return 0.0, "no lexical signal"

    positive = sum(POSITIVE_LEXICON.get(token, 0.0) for token in tokens)
    negative = sum(NEGATIVE_LEXICON.get(token, 0.0) for token in tokens)
    base = positive - negative

    event_weight = 1.0
    events = []
    for token in tokens:
        multiplier = EVENT_MULTIPLIER.get(token)
        if multiplier:
            event_weight = max(event_weight, multiplier)
            events.append(token)

    weighted = base * event_weight
    normalized = math.tanh(weighted / 4.0)
    rationale = (
        f"base={base:.2f}, event_weight={event_weight:.2f}, "
        f"events={','.join(sorted(set(events))) or 'none'}"
    )
    return normalized, rationale


def _holding_period(profile: StrategyProfile) -> timedelta:
    if profile == StrategyProfile.HFT:
        return timedelta(minutes=30)
    return timedelta(days=20)


def generate_signals(
    news_items: Iterable[NewsItem],
    profile: StrategyProfile,
    min_confidence: float,
) -> List[TradeSignal]:
    """
    Generates trade signals from normalized news.
    """

    signals: List[TradeSignal] = []
    holding_period = _holding_period(profile)
    threshold = 0.22 if profile == StrategyProfile.HFT else 0.18

    for item in news_items:
        score, rationale = _score_news(item)
        confidence = min(1.0, abs(score) * 0.7 + item.source_reliability * 0.3)
        if confidence < min_confidence:
            action = TradeAction.HOLD
        elif score >= threshold:
            action = TradeAction.BUY
        elif score <= -threshold:
            action = TradeAction.SELL
        else:
            action = TradeAction.HOLD

        signals.append(
            TradeSignal(
                news_id=item.id,
                symbol=item.symbol,
                exchange=item.exchange,
                generated_at=item.timestamp,
                action=action,
                score=score,
                confidence=confidence,
                rationale=rationale,
                holding_period=holding_period,
            )
        )

    return signals
