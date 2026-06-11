from __future__ import annotations

import json
from typing import Iterable, List

import httpx

from ..models import HarvestDocument
from .base import BaseFetcher


class HuggingFaceDatasetFetcher(BaseFetcher):
    def __init__(
        self,
        source_name: str,
        client: httpx.Client,
        dataset_ids: List[str] | None = None,
        search_queries: List[str] | None = None,
        search_limit: int = 10,
    ) -> None:
        super().__init__(source_name, client)
        self.dataset_ids = dataset_ids or []
        self.search_queries = search_queries or []
        self.search_limit = search_limit

    def fetch(self) -> Iterable[HarvestDocument]:
        seen: set[str] = set()

        for dataset_id in self.dataset_ids:
            if dataset_id in seen:
                continue
            seen.add(dataset_id)
            doc = self._fetch_dataset(dataset_id)
            if doc is not None:
                yield doc

        for query in self.search_queries:
            for dataset_id in self._search(query):
                if dataset_id in seen:
                    continue
                seen.add(dataset_id)
                doc = self._fetch_dataset(dataset_id, search_query=query)
                if doc is not None:
                    yield doc

    def _search(self, query: str) -> Iterable[str]:
        response = self.client.get(
            "https://huggingface.co/api/datasets",
            params={"search": query, "limit": self.search_limit, "full": "true"},
        )
        response.raise_for_status()
        for item in response.json():
            dataset_id = item.get("id") or item.get("_id")
            if dataset_id:
                yield dataset_id

    def _fetch_dataset(self, dataset_id: str, search_query: str | None = None) -> HarvestDocument | None:
        meta_response = self.client.get(f"https://huggingface.co/api/datasets/{dataset_id}")
        if meta_response.status_code >= 400:
            return None
        meta_response.raise_for_status()
        metadata = meta_response.json()

        size_response = self.client.get(
            "https://datasets-server.huggingface.co/size",
            params={"dataset": dataset_id},
        )
        size_payload: dict = {}
        if size_response.status_code == 200:
            size_payload = size_response.json()
        elif size_response.status_code in {401, 404, 500, 501}:
            size_payload = {"error": size_response.text[:500], "status_code": size_response.status_code}
        else:
            size_response.raise_for_status()

        card_data = metadata.get("cardData") or {}
        summary = {
            "id": dataset_id,
            "gated": metadata.get("gated"),
            "private": metadata.get("private"),
            "downloads": metadata.get("downloads"),
            "likes": metadata.get("likes"),
            "tags": metadata.get("tags", []),
            "license": card_data.get("license"),
            "size_categories": card_data.get("size_categories"),
            "dataset_info": card_data.get("dataset_info"),
            "size": size_payload.get("size", {}),
            "search_query": search_query,
        }
        content = json.dumps(summary, ensure_ascii=False, indent=2)
        return HarvestDocument(
            source=self.source_name,
            url=f"https://huggingface.co/datasets/{dataset_id}",
            title=dataset_id,
            content_raw=content,
            content_clean=content,
            metadata={
                "kind": "huggingface_dataset_metadata",
                "dataset_id": dataset_id,
                "downloads": metadata.get("downloads"),
                "likes": metadata.get("likes"),
                "gated": metadata.get("gated"),
                "private": metadata.get("private"),
                "search_query": search_query,
                "size_status": size_payload.get("status_code", 200),
            },
        )
