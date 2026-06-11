from __future__ import annotations

import hashlib
import logging
from datetime import datetime, timezone
import os
from typing import Dict, Iterable, List

import httpx

from .fetchers import AzureDevOpsWikiFetcher, GitHubRepoFetcher, HtmlDocsFetcher, HuggingFaceDatasetFetcher
from .models import FetchState, HarvestDocument, SourceConfig
from .storage import HarvestStateStore, JsonlSink

logger = logging.getLogger("source-harvester")


class HarvesterPipeline:
    def __init__(
        self,
        state_store: HarvestStateStore,
        sink: JsonlSink,
        timeout_seconds: int = 20,
        user_agent: str = "ThemisDB-SourceHarvester/0.1",
        max_retries: int = 3,
        retry_backoff_seconds: float = 1.5,
    ) -> None:
        self.state_store = state_store
        self.sink = sink
        transport = httpx.HTTPTransport(retries=max_retries)
        self.client = httpx.Client(
            timeout=timeout_seconds,
            transport=transport,
            headers={"User-Agent": user_agent},
            follow_redirects=True,
        )
        self.retry_backoff_seconds = retry_backoff_seconds

    def close(self) -> None:
        self.client.close()
        self.state_store.close()

    def run(self, source_configs: List[SourceConfig]) -> Dict[str, int]:
        fetched = 0
        written = 0

        for source_cfg in source_configs:
            if not source_cfg.enabled:
                logger.info("Source disabled: %s", source_cfg.name)
                continue

            fetcher = self._build_fetcher(source_cfg)
            if fetcher is None:
                logger.warning("Unsupported source kind: %s", source_cfg.kind)
                continue

            documents_to_write: List[HarvestDocument] = []
            for doc in fetcher.fetch():
                fetched += 1
                sha = hashlib.sha256(doc.content_clean.encode("utf-8", errors="ignore")).hexdigest()
                prev = self.state_store.get(doc.url)
                if prev and prev.content_sha256 == sha:
                    continue

                documents_to_write.append(doc)
                self.state_store.upsert(
                    FetchState(
                        url=doc.url,
                        content_sha256=sha,
                        etag=doc.metadata.get("etag"),
                        last_modified=doc.metadata.get("last_modified"),
                        last_status_code=200,
                        last_fetched_at=datetime.now(timezone.utc).isoformat(),
                    )
                )

            written += self.sink.append_documents(documents_to_write)
            logger.info(
                "Source complete: %s fetched=%d new=%d",
                source_cfg.name,
                fetched,
                len(documents_to_write),
            )

        return {"fetched": fetched, "written": written}

    def _build_fetcher(self, cfg: SourceConfig):
        opts = cfg.options
        if cfg.kind == "github_repo":
            return GitHubRepoFetcher(
                source_name=cfg.name,
                client=self.client,
                owner=str(opts["owner"]),
                repo=str(opts["repo"]),
                branch=str(opts.get("branch", "main")),
                include_paths=list(opts.get("include_paths", [])),
                token=self._resolve_secret(opts.get("token"), opts.get("token_env")),
            )

        if cfg.kind == "html_docs":
            return HtmlDocsFetcher(
                source_name=cfg.name,
                client=self.client,
                seeds=list(opts.get("seeds", [])),
                allow_domains=list(opts.get("allow_domains", [])),
                max_pages=int(opts.get("max_pages", 50)),
                respect_robots=bool(opts.get("respect_robots", True)),
                dedupe_ignore_query_params=list(opts.get("dedupe_ignore_query_params", [])) or None,
            )

        if cfg.kind == "azure_devops_wiki":
            return AzureDevOpsWikiFetcher(
                source_name=cfg.name,
                client=self.client,
                organization_url=str(opts["organization_url"]),
                project=str(opts["project"]),
                wiki_identifier=str(opts["wiki_identifier"]),
                pat=self._resolve_secret(opts.get("pat"), opts.get("pat_env")),
            )

        if cfg.kind == "huggingface_dataset_metadata":
            return HuggingFaceDatasetFetcher(
                source_name=cfg.name,
                client=self.client,
                dataset_ids=list(opts.get("dataset_ids", [])),
                search_queries=list(opts.get("search_queries", [])),
                search_limit=int(opts.get("search_limit", 10)),
            )

        return None

    @staticmethod
    def _resolve_secret(value: str | None, env_name: str | None) -> str | None:
        if value:
            return str(value)
        if env_name:
            return os.getenv(str(env_name))
        return None
