"""
RESPO CLI

Command-line interface for the RAG-Enhanced Software Programming Optimizer.
"""

import argparse
import asyncio
import json
import sys
from pathlib import Path

import structlog

from respo.ingestion import GitHubScraper, ScraperConfig, parse_github_url

logger = structlog.get_logger(__name__)


def setup_logging(verbose: bool = False) -> None:
    """Configure logging based on verbosity."""
    import logging

    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format="%(message)s",
    )

    if verbose:
        structlog.configure(
            processors=[
                structlog.stdlib.filter_by_level,
                structlog.dev.ConsoleRenderer(),
            ],
            wrapper_class=structlog.stdlib.BoundLogger,
            context_class=dict,
            logger_factory=structlog.stdlib.LoggerFactory(),
        )


async def cmd_scrape(args: argparse.Namespace) -> int:
    """Scrape a GitHub repository."""
    setup_logging(args.verbose)

    # Parse repository URL or owner/repo
    try:
        owner, repo = parse_github_url(args.repository)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Scraping repository: {owner}/{repo}")

    # Configure scraper
    config = ScraperConfig(
        github_token=args.token,
        max_file_size_kb=args.max_file_size,
        clone_depth=args.depth,
    )

    if args.extensions:
        config.include_extensions = [
            ext if ext.startswith(".") else f".{ext}"
            for ext in args.extensions.split(",")
        ]

    # Output setup
    output_path = Path(args.output) if args.output else None
    if output_path:
        output_path.mkdir(parents=True, exist_ok=True)

    # Scrape repository
    file_count = 0
    total_size = 0
    files_data = []

    async with GitHubScraper(config) as scraper:
        async for code_file in scraper.scrape_repository(
            owner=owner,
            repo=repo,
            branch=args.branch,
        ):
            file_count += 1
            total_size += code_file.size_bytes

            if args.verbose:
                print(f"  [{code_file.language}] {code_file.path} ({code_file.size_bytes} bytes)")

            if output_path:
                # Save file content
                file_output = output_path / code_file.path
                file_output.parent.mkdir(parents=True, exist_ok=True)
                file_output.write_text(code_file.content)

            if args.json_output:
                files_data.append({
                    "path": code_file.path,
                    "language": code_file.language,
                    "size_bytes": code_file.size_bytes,
                    "repo": code_file.repo,
                    "branch": code_file.branch,
                    "commit_sha": code_file.commit_sha,
                    "license": code_file.license,
                    "metadata": code_file.metadata,
                })

    # Summary
    print(f"\nScraped {file_count} files ({total_size / 1024:.1f} KB)")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(files_data, indent=2))
        print(f"Metadata saved to: {json_path}")

    if output_path:
        print(f"Files saved to: {output_path}")

    return 0


async def cmd_search(args: argparse.Namespace) -> int:
    """Search for GitHub repositories."""
    setup_logging(args.verbose)

    config = ScraperConfig(github_token=args.token)

    print(f"Searching for: {args.query}")
    if args.language:
        print(f"Language filter: {args.language}")
    if args.min_stars:
        print(f"Minimum stars: {args.min_stars}")

    async with GitHubScraper(config) as scraper:
        results = await scraper.search_repositories(
            query=args.query,
            language=args.language,
            min_stars=args.min_stars,
            max_results=args.limit,
        )

    print(f"\nFound {len(results)} repositories:\n")

    for repo in results:
        stars = repo["stars"]
        forks = repo["forks"]
        lang = repo.get("language", "Unknown")
        desc = repo.get("description", "No description")[:60]
        if len(repo.get("description", "")) > 60:
            desc += "..."

        print(f"  ⭐ {stars:>6} | 🍴 {forks:>5} | [{lang:>12}] {repo['full_name']}")
        print(f"           {desc}")
        print()

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(results, indent=2))
        print(f"Results saved to: {json_path}")

    return 0


async def cmd_batch_scrape(args: argparse.Namespace) -> int:
    """Batch scrape multiple repositories from a file."""
    setup_logging(args.verbose)

    # Read repository list
    repos_file = Path(args.repos_file)
    if not repos_file.exists():
        print(f"Error: File not found: {repos_file}", file=sys.stderr)
        return 1

    repos = []
    for line in repos_file.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            owner, repo = parse_github_url(line)
            repos.append((owner, repo))
        except ValueError as e:
            print(f"Warning: Skipping invalid entry: {line} ({e})")

    print(f"Batch scraping {len(repos)} repositories")

    config = ScraperConfig(
        github_token=args.token,
        max_file_size_kb=args.max_file_size,
    )

    output_path = Path(args.output) if args.output else None
    if output_path:
        output_path.mkdir(parents=True, exist_ok=True)

    total_files = 0
    total_size = 0
    all_files_data = []

    async with GitHubScraper(config) as scraper:
        for owner, repo in repos:
            print(f"\nScraping: {owner}/{repo}")
            file_count = 0

            try:
                async for code_file in scraper.scrape_repository(owner, repo):
                    file_count += 1
                    total_files += 1
                    total_size += code_file.size_bytes

                    if output_path:
                        file_output = output_path / code_file.repo / code_file.path
                        file_output.parent.mkdir(parents=True, exist_ok=True)
                        file_output.write_text(code_file.content)

                    if args.json_output:
                        all_files_data.append({
                            "path": code_file.path,
                            "language": code_file.language,
                            "size_bytes": code_file.size_bytes,
                            "repo": code_file.repo,
                            "branch": code_file.branch,
                            "commit_sha": code_file.commit_sha,
                            "license": code_file.license,
                        })

                print(f"  → {file_count} files")

            except Exception as e:
                print(f"  → Error: {e}")

    print(f"\n{'='*50}")
    print(f"Total: {total_files} files ({total_size / 1024 / 1024:.1f} MB)")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(all_files_data, indent=2))
        print(f"Metadata saved to: {json_path}")

    return 0


def main() -> None:
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        prog="respo",
        description="RESPO - RAG-Enhanced Software Programming Optimizer",
    )
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s 0.1.0",
    )

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Scrape command
    scrape_parser = subparsers.add_parser(
        "scrape",
        help="Scrape a GitHub repository",
    )
    scrape_parser.add_argument(
        "repository",
        help="GitHub repository (URL or owner/repo format)",
    )
    scrape_parser.add_argument(
        "-b", "--branch",
        help="Branch to scrape (default: default branch)",
    )
    scrape_parser.add_argument(
        "-o", "--output",
        help="Output directory for scraped files",
    )
    scrape_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file for metadata",
    )
    scrape_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    scrape_parser.add_argument(
        "-e", "--extensions",
        help="File extensions to include (comma-separated, e.g., py,js,ts)",
    )
    scrape_parser.add_argument(
        "--max-file-size",
        type=int,
        default=500,
        help="Maximum file size in KB (default: 500)",
    )
    scrape_parser.add_argument(
        "--depth",
        type=int,
        default=1,
        help="Git clone depth (default: 1 for shallow)",
    )
    scrape_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Search command
    search_parser = subparsers.add_parser(
        "search",
        help="Search for GitHub repositories",
    )
    search_parser.add_argument(
        "query",
        help="Search query",
    )
    search_parser.add_argument(
        "-l", "--language",
        help="Filter by language",
    )
    search_parser.add_argument(
        "-s", "--min-stars",
        type=int,
        default=0,
        help="Minimum stars",
    )
    search_parser.add_argument(
        "-n", "--limit",
        type=int,
        default=20,
        help="Maximum results (default: 20)",
    )
    search_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file",
    )
    search_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    search_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Batch scrape command
    batch_parser = subparsers.add_parser(
        "batch-scrape",
        help="Batch scrape repositories from a file",
    )
    batch_parser.add_argument(
        "repos_file",
        help="File with repository URLs (one per line)",
    )
    batch_parser.add_argument(
        "-o", "--output",
        help="Output directory for scraped files",
    )
    batch_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file for metadata",
    )
    batch_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    batch_parser.add_argument(
        "--max-file-size",
        type=int,
        default=500,
        help="Maximum file size in KB (default: 500)",
    )
    batch_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Server command
    server_parser = subparsers.add_parser(
        "server",
        help="Start the RESPO API server",
    )
    server_parser.add_argument(
        "-p", "--port",
        type=int,
        default=8080,
        help="Port to listen on (default: 8080)",
    )
    server_parser.add_argument(
        "--host",
        default="0.0.0.0",
        help="Host to bind to (default: 0.0.0.0)",
    )

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(0)

    if args.command == "scrape":
        exit_code = asyncio.run(cmd_scrape(args))
    elif args.command == "search":
        exit_code = asyncio.run(cmd_search(args))
    elif args.command == "batch-scrape":
        exit_code = asyncio.run(cmd_batch_scrape(args))
    elif args.command == "server":
        from respo.api.app import run_server
        run_server()
        exit_code = 0
    else:
        parser.print_help()
        exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
