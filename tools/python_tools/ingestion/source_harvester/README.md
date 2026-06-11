# Source Harvester (Python)

Purpose: build a reusable source-code/documentation corpus for Graph-RAG and retrieval.

This MVP focuses on:
- GitHub repository content via GitHub REST API (preferred over HTML scraping)
- HTML docs crawl for allow-listed domains (for example Microsoft Learn)
- Azure DevOps wiki ingestion via REST API
- Hugging Face dataset metadata search with size/split introspection where available
- Incremental state tracking (SQLite) and append-only JSONL output

## Why this design

- API-first collection where possible (GitHub, Azure DevOps)
- API-first dataset discovery for training/eval corpora (Hugging Face)
- Controlled HTML crawl only for docs domains with explicit allow-lists
- Incremental fetch with content hash to avoid re-indexing unchanged pages
- One normalized output format for downstream chunking/embedding

## Install

```powershell
cd tools/python_tools/ingestion/source_harvester
python -m pip install -r requirements.txt
```

## Run

```powershell
python tools/python_tools/ingestion/run_source_harvester.py --config tools/python_tools/ingestion/source_harvester/config.example.yaml
```

## Run End-to-End Into Existing Ingestion

This uses the new bridge script to:
- run the source harvester
- materialize harvested documents as staged JSON files
- feed those staged files into the existing [tools/python_tools/ingestion/ingest.py](tools/python_tools/ingestion/ingest.py) workflow

```powershell
python tools/python_tools/ingestion/run_harvest_ingest_pipeline.py \
	--harvester-config tools/python_tools/ingestion/source_harvester/hf_shortlist.example.yaml \
	--stage-dir tools/python_tools/ingestion/.stage_hf \
	--output tools/python_tools/ingestion/hf_ingestion_output.json \
	--db tools/python_tools/ingestion/hf_ingestion_tracker.db \
	--clean-stage
```

The resulting output stays compatible with the existing ingestion engine output structure.

## Output artifacts

- `storage.sqlite_path`: fetch state with last content hash
- `storage.output_jsonl`: normalized documents, one JSON record per line

Document fields:
- source
- url
- title
- content_raw
- content_clean
- metadata
- fetched_at

## Source kinds

1. `github_repo`
- owner, repo, branch, include_paths

2. `html_docs`
- seeds, allow_domains, max_pages
- `respect_robots` defaults to true
- canonical URLs are normalized before persistence
- common tracking query parameters are removed for deduplication

3. `azure_devops_wiki`
- organization_url, project, wiki_identifier

4. `huggingface_dataset_metadata`
- dataset_ids, search_queries, search_limit
- pulls dataset metadata from the Hugging Face API
- enriches with sizes/splits via the datasets-server API where available
- preserves gated/viewer-limit errors in metadata so planning stays explicit

## Practical shortlist for coding assistants

The file [tools/python_tools/ingestion/source_harvester/hf_shortlist.example.yaml](tools/python_tools/ingestion/source_harvester/hf_shortlist.example.yaml) contains a useful starter set:
- CodeSearchNet for code retrieval
- MBPP / HumanEval / DS-1000 for coding eval and small SFT-style tasks
- MS MARCO and BeIR subsets for retrieval/reranking
- The Stack variants and APPS as larger follow-up candidates

## Training Matrix Generator

Use the generator to turn the shortlist into a reproducible prioritization matrix:

```powershell
python tools/python_tools/ingestion/source_harvester/generate_hf_training_matrix.py \
	--config tools/python_tools/ingestion/source_harvester/hf_shortlist.example.yaml \
	--output-md tools/python_tools/ingestion/source_harvester/hf_training_matrix.generated.md \
	--output-json tools/python_tools/ingestion/source_harvester/hf_training_matrix.generated.json
```

The example output in [tools/python_tools/ingestion/source_harvester/hf_training_matrix.example.md](tools/python_tools/ingestion/source_harvester/hf_training_matrix.example.md) shows the expected prioritization format.

## Operational guidance

- Keep domain allow-lists strict.
- Respect robots.txt and terms of service for HTML sources.
- Prefer official APIs whenever available.
- Start with small max_pages and expand after quality checks.
- Treat gated datasets and viewer/API limitations as governance signals, not as scrape failures.
- For doc sites, keep canonicalization and query-param stripping enabled unless a site depends on query-state navigation.
