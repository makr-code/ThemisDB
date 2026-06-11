from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional


@dataclass
class SourceConfig:
    name: str
    kind: str
    enabled: bool = True
    options: Dict[str, Any] = field(default_factory=dict)


@dataclass
class HarvestDocument:
    source: str
    url: str
    title: str
    content_raw: str
    content_clean: str
    metadata: Dict[str, Any] = field(default_factory=dict)
    fetched_at: str = field(default_factory=lambda: datetime.now(timezone.utc).isoformat())

    def to_json(self) -> Dict[str, Any]:
        return {
            "source": self.source,
            "url": self.url,
            "title": self.title,
            "content_raw": self.content_raw,
            "content_clean": self.content_clean,
            "metadata": self.metadata,
            "fetched_at": self.fetched_at,
        }


@dataclass
class FetchState:
    url: str
    etag: Optional[str] = None
    last_modified: Optional[str] = None
    content_sha256: Optional[str] = None
    last_status_code: Optional[int] = None
    last_fetched_at: Optional[str] = None
