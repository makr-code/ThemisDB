"""Forecast evaluation against labeled future-return outcomes."""

from __future__ import annotations

import csv
from datetime import datetime
from statistics import mean
from typing import Iterable, List

from models import EventLabel, ForecastDirection, ForecastEvaluationReport, ForecastPrediction


def _parse_ts(value: str) -> datetime:
    return datetime.fromisoformat(value.replace("Z", "+00:00"))


def load_labels(path: str) -> List[EventLabel]:
    labels: List[EventLabel] = []
    with open(path, "r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            labels.append(
                EventLabel(
                    news_id=row["news_id"],
                    symbol=row["symbol"],
                    exchange=row["exchange"],
                    event_timestamp=_parse_ts(row["event_timestamp"]),
                    horizon_days=int(row["horizon_days"]),
                    future_return_pct=float(row["future_return_pct"]),
                    label=row["label"],
                    confidence=float(row["confidence"]),
                    rationale=row["rationale"],
                )
            )
    return labels


def _direction_from_return(value: float) -> ForecastDirection:
    if value > 0.5:
        return ForecastDirection.BULLISH
    if value < -0.5:
        return ForecastDirection.BEARISH
    return ForecastDirection.NEUTRAL


def evaluate_forecasts(
    forecasts: Iterable[ForecastPrediction],
    labels: Iterable[EventLabel],
) -> ForecastEvaluationReport:
    """Compare forecasts with labels using directional accuracy and absolute error."""

    by_key = {(item.symbol, item.exchange, item.generated_at, item.horizon_days): item for item in forecasts}
    compared = 0
    correct = 0
    abs_errors: List[float] = []

    label_list = list(labels)
    for label in label_list:
        key = (label.symbol, label.exchange, label.event_timestamp, label.horizon_days)
        forecast = by_key.get(key)
        if forecast is None:
            continue
        compared += 1
        if forecast.direction == _direction_from_return(label.future_return_pct):
            correct += 1
        abs_errors.append(abs(forecast.expected_return_pct - label.future_return_pct))

    total = len(label_list)
    return ForecastEvaluationReport(
        compared=compared,
        directional_accuracy=(correct / compared) if compared else 0.0,
        mean_abs_error_pct=mean(abs_errors) if abs_errors else 0.0,
        coverage=(compared / total) if total else 0.0,
    )
