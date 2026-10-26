"""
ThemisDB multi-model export helpers.
"""

from __future__ import annotations

import json
import math
import re
from pathlib import Path
from typing import Iterable, List

from models import MarketBar, NewsItem, TradeSignal

TOKEN_RE = re.compile(r"[a-zA-Z][a-zA-Z\-]+")


def _feature_vector(text: str, dimensions: int = 12) -> List[float]:
    """
    Generates deterministic low-cost embeddings for demo ingestion.
    """

    vector = [0.0] * dimensions
    tokens = [token.lower() for token in TOKEN_RE.findall(text)]
    if not tokens:
        return vector

    for token in tokens:
        idx = hash(token) % dimensions
        vector[idx] += 1.0

    norm = math.sqrt(sum(value * value for value in vector)) or 1.0
    return [value / norm for value in vector]


def export_jsonl(
    output_dir: Path,
    news_items: Iterable[NewsItem],
    market_bars: Iterable[MarketBar],
    signals: Iterable[TradeSignal],
) -> None:
    """
    Writes import packets aligned with document/vector/graph/time-series usage.
    """

    output_dir.mkdir(parents=True, exist_ok=True)

    news_path = output_dir / "news_documents.jsonl"
    price_path = output_dir / "price_series.jsonl"
    signal_path = output_dir / "signals_graph_edges.jsonl"

    with news_path.open("w", encoding="utf-8") as handle:
        for item in news_items:
            text = f"{item.headline}\n{item.body}"
            payload = {
                "id": item.id,
                "symbol": item.symbol,
                "exchange": item.exchange,
                "source": item.source,
                "timestamp": item.timestamp.isoformat(),
                "document": {
                    "headline": item.headline,
                    "body": item.body,
                },
                "vector": _feature_vector(text),
            }
            handle.write(json.dumps(payload) + "\n")

    with price_path.open("w", encoding="utf-8") as handle:
        for bar in market_bars:
            payload = {
                "symbol": bar.symbol,
                "exchange": bar.exchange,
                "timestamp": bar.timestamp.isoformat(),
                "close": bar.close,
            }
            handle.write(json.dumps(payload) + "\n")

    with signal_path.open("w", encoding="utf-8") as handle:
        for signal in signals:
            payload = {
                "from_news": signal.news_id,
                "to_symbol": signal.symbol,
                "exchange": signal.exchange,
                "action": signal.action.value,
                "confidence": signal.confidence,
                "score": signal.score,
                "generated_at": signal.generated_at.isoformat(),
            }
            handle.write(json.dumps(payload) + "\n")
