from __future__ import annotations

import hashlib
import json
import sqlite3
from dataclasses import asdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Optional

from .models import FetchState, HarvestDocument


class HarvestStateStore:
    def __init__(self, db_path: str) -> None:
        self.db_path = db_path
        self._conn = sqlite3.connect(db_path)
        self._init_schema()

    def _init_schema(self) -> None:
        cursor = self._conn.cursor()
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS fetch_state (
                url TEXT PRIMARY KEY,
                etag TEXT,
                last_modified TEXT,
                content_sha256 TEXT,
                last_status_code INTEGER,
                last_fetched_at TEXT
            )
            """
        )
        self._conn.commit()

    def get(self, url: str) -> Optional[FetchState]:
        cursor = self._conn.cursor()
        cursor.execute(
            "SELECT url, etag, last_modified, content_sha256, last_status_code, last_fetched_at FROM fetch_state WHERE url = ?",
            (url,),
        )
        row = cursor.fetchone()
        if not row:
            return None
        return FetchState(
            url=row[0],
            etag=row[1],
            last_modified=row[2],
            content_sha256=row[3],
            last_status_code=row[4],
            last_fetched_at=row[5],
        )

    def upsert(self, state: FetchState) -> None:
        cursor = self._conn.cursor()
        cursor.execute(
            """
            INSERT INTO fetch_state (url, etag, last_modified, content_sha256, last_status_code, last_fetched_at)
            VALUES (?, ?, ?, ?, ?, ?)
            ON CONFLICT(url) DO UPDATE SET
                etag = excluded.etag,
                last_modified = excluded.last_modified,
                content_sha256 = excluded.content_sha256,
                last_status_code = excluded.last_status_code,
                last_fetched_at = excluded.last_fetched_at
            """,
            (
                state.url,
                state.etag,
                state.last_modified,
                state.content_sha256,
                state.last_status_code,
                state.last_fetched_at,
            ),
        )
        self._conn.commit()

    def close(self) -> None:
        self._conn.close()


class JsonlSink:
    def __init__(self, output_path: str) -> None:
        self.path = Path(output_path)
        self.path.parent.mkdir(parents=True, exist_ok=True)

    @staticmethod
    def compute_sha256(text: str) -> str:
        return hashlib.sha256(text.encode("utf-8", errors="ignore")).hexdigest()

    def append_documents(self, documents: Iterable[HarvestDocument]) -> int:
        count = 0
        now = datetime.now(timezone.utc).isoformat()
        with self.path.open("a", encoding="utf-8") as handle:
            for doc in documents:
                payload = doc.to_json()
                payload["ingested_at"] = now
                payload["content_sha256"] = self.compute_sha256(doc.content_clean)
                handle.write(json.dumps(payload, ensure_ascii=False) + "\n")
                count += 1
        return count
