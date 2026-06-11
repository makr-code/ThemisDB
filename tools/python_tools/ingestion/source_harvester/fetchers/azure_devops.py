from __future__ import annotations

import base64
from typing import Iterable

import httpx

from ..models import HarvestDocument
from .base import BaseFetcher


class AzureDevOpsWikiFetcher(BaseFetcher):
    def __init__(
        self,
        source_name: str,
        client: httpx.Client,
        organization_url: str,
        project: str,
        wiki_identifier: str,
        pat: str | None = None,
    ) -> None:
        super().__init__(source_name, client)
        self.organization_url = organization_url.rstrip("/")
        self.project = project
        self.wiki_identifier = wiki_identifier
        self.pat = pat

    def fetch(self) -> Iterable[HarvestDocument]:
        pages_api = (
            f"{self.organization_url}/{self.project}/_apis/wiki/wikis/{self.wiki_identifier}/pages"
            "?path=/&recursionLevel=full&includeContent=true&api-version=7.1-preview.1"
        )
        headers = None
        if self.pat:
            token = base64.b64encode(f":{self.pat}".encode("utf-8")).decode("ascii")
            headers = {"Authorization": f"Basic {token}"}

        response = self.client.get(pages_api, headers=headers)
        response.raise_for_status()
        payload = response.json()

        for page in self._walk_pages(payload):
            path = page.get("path", "/")
            content = page.get("content", "")
            if not content:
                continue
            page_url = page.get("url", pages_api)
            yield HarvestDocument(
                source=self.source_name,
                url=page_url,
                title=path,
                content_raw=content,
                content_clean=content,
                metadata={
                    "kind": "azure_devops_wiki",
                    "project": self.project,
                    "wiki_identifier": self.wiki_identifier,
                    "path": path,
                },
            )

    def _walk_pages(self, node: dict) -> Iterable[dict]:
        if "path" in node:
            yield node
        for child in node.get("subPages", []) or []:
            yield from self._walk_pages(child)
