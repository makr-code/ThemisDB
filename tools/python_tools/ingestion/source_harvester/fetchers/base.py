from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Iterable, List

import httpx

from ..models import HarvestDocument


class BaseFetcher(ABC):
    def __init__(self, source_name: str, client: httpx.Client) -> None:
        self.source_name = source_name
        self.client = client

    @abstractmethod
    def fetch(self) -> Iterable[HarvestDocument]:
        raise NotImplementedError

    @staticmethod
    def _safe_title_from_url(url: str) -> str:
        parts: List[str] = [p for p in url.rstrip("/").split("/") if p]
        return parts[-1] if parts else "untitled"
