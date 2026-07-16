from __future__ import annotations

import base64
from typing import Iterable, List

import httpx

from ..models import HarvestDocument
from .base import BaseFetcher


class GitHubRepoFetcher(BaseFetcher):
    def __init__(
        self,
        source_name: str,
        client: httpx.Client,
        owner: str,
        repo: str,
        branch: str,
        include_paths: List[str],
        token: str | None = None,
    ) -> None:
        super().__init__(source_name, client)
        self.owner = owner
        self.repo = repo
        self.branch = branch
        self.include_paths = include_paths
        self.token = token

    def fetch(self) -> Iterable[HarvestDocument]:
        for path in self.include_paths:
            if path.endswith("/"):
                yield from self._fetch_directory(path)
            else:
                doc = self._fetch_file(path)
                if doc is not None:
                    yield doc

    def _repo_api_url(self, path: str) -> str:
        return (
            f"https://api.github.com/repos/{self.owner}/{self.repo}/contents/{path}"
            f"?ref={self.branch}"
        )

    def _fetch_directory(self, directory_path: str) -> Iterable[HarvestDocument]:
        response = self.client.get(
            self._repo_api_url(directory_path.rstrip("/")),
            headers=self._headers(),
        )
        response.raise_for_status()
        for entry in response.json():
            if entry.get("type") == "file":
                item_path = entry.get("path", "")
                if item_path:
                    doc = self._fetch_file(item_path)
                    if doc is not None:
                        yield doc

    def _fetch_file(self, path: str) -> HarvestDocument | None:
        response = self.client.get(self._repo_api_url(path), headers=self._headers())
        if response.status_code == 404:
            return None
        response.raise_for_status()

        payload = response.json()
        encoded = payload.get("content", "")
        if not encoded:
            return None

        raw_bytes = base64.b64decode(encoded)
        text = raw_bytes.decode("utf-8", errors="ignore")
        html_url = payload.get("html_url") or (
            f"https://github.com/{self.owner}/{self.repo}/blob/{self.branch}/{path}"
        )
        return HarvestDocument(
            source=self.source_name,
            url=html_url,
            title=path,
            content_raw=text,
            content_clean=text,
            metadata={
                "kind": "github_repo",
                "owner": self.owner,
                "repo": self.repo,
                "branch": self.branch,
                "path": path,
                "sha": payload.get("sha"),
            },
        )

    def _headers(self) -> dict[str, str] | None:
        if not self.token:
            return None
        return {"Authorization": f"Bearer {self.token}"}
