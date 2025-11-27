"""
Educational Source Scrapers

Scrapers for structured learning resources, tutorials, algorithms, and best practices.
These sources are essential for building deep programming understanding in LLMs.
"""

import asyncio
import html
import re
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class LearnXInYMinutesScraper(CodeSource):
    """
    Learn X in Y Minutes scraper.

    Provides concise, example-heavy introductions to programming languages.
    Perfect for teaching language basics to an LLM.
    
    Source: https://learnxinyminutes.com/
    GitHub: https://github.com/adambard/learnxinyminutes-docs
    """

    source_type = SourceType.GITHUB

    # Available languages
    LANGUAGES = [
        "python", "python3", "javascript", "typescript", "java", "c", "c++",
        "csharp", "go", "rust", "ruby", "php", "swift", "kotlin", "scala",
        "haskell", "elixir", "clojure", "lua", "perl", "r", "julia",
        "bash", "powershell", "sql", "html", "css", "json", "yaml", "toml",
        "markdown", "latex", "git", "make", "cmake", "docker", "kubernetes",
        "terraform", "ansible", "vim", "emacs", "regex",
    ]

    def __init__(self, config: Optional[SourceConfig] = None):
        super().__init__(config)
        self.raw_url = "https://raw.githubusercontent.com/adambard/learnxinyminutes-docs/master"
        self._session: Optional[aiohttp.ClientSession] = None

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _fetch(self, filename: str) -> Optional[str]:
        session = await self._get_session()
        url = f"{self.raw_url}/{filename}"
        try:
            async with session.get(url) as resp:
                if resp.status == 200:
                    return await resp.text()
                return None
        except Exception:
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape Learn X in Y Minutes tutorials.

        Args:
            identifier: "all" or specific language name
        """
        logger.info("Scraping Learn X in Y Minutes", identifier=identifier)

        languages = self.LANGUAGES if identifier == "all" else [identifier]

        for lang in languages:
            # Try different filename patterns
            filenames = [f"{lang}.html.markdown", f"{lang}.md", f"{lang}.html"]
            
            content = None
            for filename in filenames:
                content = await self._fetch(filename)
                if content:
                    break

            if not content:
                continue

            yield SourceFile(
                id=SourceFile.generate_id(self.source_type, "learnxinyminutes", lang),
                path=f"tutorials/{lang}.md",
                source_type=self.source_type,
                content=content,
                size_bytes=len(content.encode("utf-8")),
                language=lang,
                repo="learnxinyminutes",
                metadata={
                    "source": "learnxinyminutes",
                    "type": "tutorial",
                    "url": f"https://learnxinyminutes.com/docs/{lang}/",
                },
            )

        logger.info("Scraping complete")

    async def search(self, query: str, language: Optional[str] = None, limit: int = 20) -> list[dict]:
        results = []
        for lang in self.LANGUAGES:
            if query.lower() in lang:
                results.append({"language": lang, "url": f"https://learnxinyminutes.com/docs/{lang}/"})
        return results[:limit]

    async def get_metadata(self, identifier: str) -> dict:
        return {"language": identifier, "source": "learnxinyminutes"}

    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()


class RosettaCodeScraper(CodeSource):
    """
    Rosetta Code scraper.

    Provides the same algorithms/tasks implemented in 700+ programming languages.
    Excellent for teaching cross-language patterns and idiomatic code.
    
    Source: https://rosettacode.org/
    """

    source_type = SourceType.LOCAL

    def __init__(self, config: Optional[SourceConfig] = None):
        super().__init__(config)
        self.api_url = "https://rosettacode.org/w/api.php"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(2)

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _api_request(self, params: dict) -> Optional[dict]:
        async with self._rate_limiter:
            session = await self._get_session()
            params["format"] = "json"
            try:
                async with session.get(self.api_url, params=params) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception as e:
                logger.warning("API request failed", error=str(e))
                return None

    async def _get_page_content(self, title: str) -> Optional[str]:
        """Get wikitext content of a page."""
        data = await self._api_request({
            "action": "query",
            "titles": title,
            "prop": "revisions",
            "rvprop": "content",
            "rvslots": "main",
        })
        
        if not data:
            return None
            
        pages = data.get("query", {}).get("pages", {})
        for page in pages.values():
            revisions = page.get("revisions", [])
            if revisions:
                return revisions[0].get("slots", {}).get("main", {}).get("*")
        return None

    def _extract_code_blocks(self, wikitext: str, language: Optional[str] = None) -> list[tuple[str, str, str]]:
        """Extract code blocks from wikitext. Returns [(language, code, header)]."""
        blocks = []
        
        # Pattern for language headers like ==Python== or ===Python===
        # followed by code in <lang python>...</lang> or <syntaxhighlight>...</syntaxhighlight>
        current_lang = None
        
        # Find language sections
        section_pattern = r'==+\s*\{\{header\|([^}]+)\}\}|==+\s*([A-Za-z0-9#+]+)\s*==+'
        code_pattern = r'<(?:lang|syntaxhighlight)[^>]*>(.*?)</(?:lang|syntaxhighlight)>'
        
        sections = re.split(r'(==+[^=]+==+)', wikitext)
        
        for i, section in enumerate(sections):
            # Check if this is a header
            header_match = re.search(section_pattern, section)
            if header_match:
                current_lang = header_match.group(1) or header_match.group(2)
                current_lang = current_lang.strip().lower()
                continue
            
            # Extract code from section
            if current_lang and (not language or language.lower() in current_lang):
                for match in re.finditer(code_pattern, section, re.DOTALL):
                    code = html.unescape(match.group(1).strip())
                    if code:
                        blocks.append((current_lang, code, current_lang))
        
        return blocks

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape Rosetta Code tasks.

        Args:
            identifier: Task name or "Category:Programming_Tasks" for all
            version: Optional language filter
        """
        logger.info("Scraping Rosetta Code", task=identifier, language=version)

        if identifier.startswith("Category:"):
            # Get all tasks in category
            tasks = []
            cmcontinue = None
            
            while True:
                params = {
                    "action": "query",
                    "list": "categorymembers",
                    "cmtitle": identifier,
                    "cmlimit": 500,
                }
                if cmcontinue:
                    params["cmcontinue"] = cmcontinue
                    
                data = await self._api_request(params)
                if not data:
                    break
                    
                for member in data.get("query", {}).get("categorymembers", []):
                    tasks.append(member["title"])
                
                cmcontinue = data.get("continue", {}).get("cmcontinue")
                if not cmcontinue:
                    break
        else:
            tasks = [identifier]

        file_count = 0
        for task in tasks:
            if file_count >= self.config.max_files_per_repo:
                break
                
            content = await self._get_page_content(task)
            if not content:
                continue
                
            code_blocks = self._extract_code_blocks(content, version)
            
            for lang, code, header in code_blocks:
                if file_count >= self.config.max_files_per_repo:
                    break
                    
                file_count += 1
                task_slug = task.replace(" ", "_").replace("/", "_")
                
                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, "rosettacode", f"{task_slug}/{lang}"),
                    path=f"tasks/{task_slug}/{lang}.txt",
                    source_type=self.source_type,
                    content=f"# Task: {task}\n# Language: {lang}\n\n{code}",
                    size_bytes=len(code.encode("utf-8")),
                    language=lang,
                    repo="rosettacode",
                    metadata={
                        "source": "rosettacode",
                        "task": task,
                        "url": f"https://rosettacode.org/wiki/{task.replace(' ', '_')}#{header}",
                    },
                )

        logger.info("Scraping complete", files=file_count)

    async def search(self, query: str, language: Optional[str] = None, limit: int = 20) -> list[dict]:
        data = await self._api_request({
            "action": "query",
            "list": "search",
            "srsearch": query,
            "srlimit": limit,
        })
        
        if not data:
            return []
            
        return [
            {"title": r["title"], "url": f"https://rosettacode.org/wiki/{r['title'].replace(' ', '_')}"}
            for r in data.get("query", {}).get("search", [])
        ]

    async def get_metadata(self, identifier: str) -> dict:
        return {"task": identifier, "source": "rosettacode"}

    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()


class LeetCodeScraper(CodeSource):
    """
    LeetCode solutions scraper.

    Scrapes algorithm solutions from community repositories.
    Essential for teaching problem-solving patterns.
    """

    source_type = SourceType.GITHUB

    def __init__(self, config: Optional[SourceConfig] = None):
        super().__init__(config)
        self.api_url = "https://api.github.com"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(10)

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            headers = {"Accept": "application/vnd.github+json"}
            if self.config.token:
                headers["Authorization"] = f"Bearer {self.config.token}"
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session

    async def _fetch_json(self, url: str) -> Optional[Any]:
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception:
                return None

    async def _fetch_raw(self, url: str) -> Optional[str]:
        session = await self._get_session()
        try:
            async with session.get(url) as resp:
                if resp.status == 200:
                    return await resp.text()
                return None
        except Exception:
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape LeetCode solutions from popular repos.

        Args:
            identifier: Language filter (python, java, cpp, etc.) or "all"
        """
        logger.info("Scraping LeetCode solutions", language=identifier)

        # Popular LeetCode solution repos
        repos = [
            "neetcode-gh/leetcode",
            "doocs/leetcode",
        ]

        file_count = 0
        for repo in repos:
            if file_count >= self.config.max_files_per_repo:
                break

            # Get repo tree
            tree_url = f"{self.api_url}/repos/{repo}/git/trees/main?recursive=1"
            data = await self._fetch_json(tree_url)
            
            if not data:
                continue

            for item in data.get("tree", []):
                if item["type"] != "blob":
                    continue
                    
                path = item["path"]
                
                # Filter by language if specified
                if identifier != "all":
                    lang_filter = identifier.lower()
                    if not any(path.lower().endswith(ext) for ext in self._get_extensions(lang_filter)):
                        continue
                
                if not self._should_include_file(path, item.get("size", 0)):
                    continue

                # Fetch content
                raw_url = f"https://raw.githubusercontent.com/{repo}/main/{path}"
                content = await self._fetch_raw(raw_url)
                
                if not content:
                    continue
                    
                file_count += 1
                
                # Extract problem number/name from path
                problem_match = re.search(r'(\d+)', path)
                problem_num = problem_match.group(1) if problem_match else "unknown"
                
                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, f"leetcode:{repo}", path),
                    path=path,
                    source_type=self.source_type,
                    content=content,
                    size_bytes=len(content.encode("utf-8")),
                    language=self._detect_language(path),
                    repo=f"leetcode:{repo}",
                    metadata={
                        "source": "leetcode",
                        "problem_number": problem_num,
                        "github_repo": repo,
                        "url": f"https://github.com/{repo}/blob/main/{path}",
                    },
                )
                
                if file_count >= self.config.max_files_per_repo:
                    break

        logger.info("Scraping complete", files=file_count)

    def _get_extensions(self, language: str) -> list[str]:
        ext_map = {
            "python": [".py"],
            "java": [".java"],
            "cpp": [".cpp", ".cc", ".cxx"],
            "c": [".c"],
            "javascript": [".js"],
            "typescript": [".ts"],
            "go": [".go"],
            "rust": [".rs"],
            "ruby": [".rb"],
            "swift": [".swift"],
            "kotlin": [".kt"],
        }
        return ext_map.get(language, [f".{language}"])

    async def search(self, query: str, language: Optional[str] = None, limit: int = 20) -> list[dict]:
        return []  # No search for LeetCode

    async def get_metadata(self, identifier: str) -> dict:
        return {"language": identifier, "source": "leetcode"}

    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()


class DesignPatternsScraper(CodeSource):
    """
    Design Patterns scraper.

    Scrapes design pattern examples from refactoring.guru and other sources.
    Essential for teaching software architecture concepts.
    """

    source_type = SourceType.GITHUB

    def __init__(self, config: Optional[SourceConfig] = None):
        super().__init__(config)
        self.api_url = "https://api.github.com"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(10)

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            headers = {"Accept": "application/vnd.github+json"}
            if self.config.token:
                headers["Authorization"] = f"Bearer {self.config.token}"
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session

    async def _fetch_json(self, url: str) -> Optional[Any]:
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception:
                return None

    async def _fetch_raw(self, url: str) -> Optional[str]:
        session = await self._get_session()
        try:
            async with session.get(url) as resp:
                if resp.status == 200:
                    return await resp.text()
                return None
        except Exception:
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape design pattern examples.

        Args:
            identifier: Language (python, java, typescript, etc.)
        """
        logger.info("Scraping Design Patterns", language=identifier)

        # Design pattern repos by language
        repos = {
            "python": ["faif/python-patterns", "RefactoringGuru/design-patterns-python"],
            "java": ["iluwatar/java-design-patterns", "RefactoringGuru/design-patterns-java"],
            "typescript": ["torokmark/design_patterns_in_typescript"],
            "go": ["tmrts/go-patterns"],
            "rust": ["lpxxn/rust-design-pattern"],
            "cpp": ["JakubVojvworlds/design-patterns-cpp"],
        }

        target_repos = repos.get(identifier.lower(), [])
        if not target_repos and identifier == "all":
            target_repos = [repo for lang_repos in repos.values() for repo in lang_repos]

        file_count = 0
        for repo in target_repos:
            if file_count >= self.config.max_files_per_repo:
                break

            # Get repo tree
            for branch in ["main", "master"]:
                tree_url = f"{self.api_url}/repos/{repo}/git/trees/{branch}?recursive=1"
                data = await self._fetch_json(tree_url)
                if data:
                    break
            
            if not data:
                continue

            for item in data.get("tree", []):
                if item["type"] != "blob":
                    continue
                    
                path = item["path"]
                
                if not self._should_include_file(path, item.get("size", 0)):
                    continue

                # Fetch content
                raw_url = f"https://raw.githubusercontent.com/{repo}/{branch}/{path}"
                content = await self._fetch_raw(raw_url)
                
                if not content:
                    continue
                    
                file_count += 1
                
                # Try to extract pattern name from path
                pattern_match = re.search(r'(singleton|factory|abstract.?factory|builder|prototype|adapter|bridge|composite|decorator|facade|flyweight|proxy|chain.?of.?responsibility|command|interpreter|iterator|mediator|memento|observer|state|strategy|template.?method|visitor)', path.lower())
                pattern_name = pattern_match.group(1).replace("_", " ").title() if pattern_match else "unknown"
                
                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, f"patterns:{repo}", path),
                    path=path,
                    source_type=self.source_type,
                    content=content,
                    size_bytes=len(content.encode("utf-8")),
                    language=self._detect_language(path),
                    repo=f"patterns:{repo}",
                    metadata={
                        "source": "design_patterns",
                        "pattern": pattern_name,
                        "github_repo": repo,
                        "url": f"https://github.com/{repo}/blob/{branch}/{path}",
                    },
                )
                
                if file_count >= self.config.max_files_per_repo:
                    break

        logger.info("Scraping complete", files=file_count)

    async def search(self, query: str, language: Optional[str] = None, limit: int = 20) -> list[dict]:
        return []

    async def get_metadata(self, identifier: str) -> dict:
        return {"language": identifier, "source": "design_patterns"}

    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()


class PythonPEPsScraper(CodeSource):
    """
    Python Enhancement Proposals (PEPs) scraper.

    PEPs define Python's design decisions and best practices.
    Essential for understanding Python idioms.
    """

    source_type = SourceType.GITHUB

    def __init__(self, config: Optional[SourceConfig] = None):
        super().__init__(config)
        self.raw_url = "https://raw.githubusercontent.com/python/peps/main/peps"
        self._session: Optional[aiohttp.ClientSession] = None

    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _fetch(self, url: str) -> Optional[str]:
        session = await self._get_session()
        try:
            async with session.get(url) as resp:
                if resp.status == 200:
                    return await resp.text()
                return None
        except Exception:
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape Python PEPs.

        Args:
            identifier: "all" or specific PEP number (e.g., "8", "484")
        """
        logger.info("Scraping Python PEPs", identifier=identifier)

        # Important PEPs for coding style and best practices
        important_peps = [
            "0008",  # Style Guide
            "0020",  # The Zen of Python
            "0257",  # Docstring Conventions
            "0484",  # Type Hints
            "0526",  # Variable Annotations
            "0544",  # Protocols
            "0585",  # Type Hinting Generics
            "0604",  # Union Types
            "0612",  # ParamSpec
            "0617",  # New Parser
            "0634",  # Structural Pattern Matching
            "0636",  # Pattern Matching Tutorial
            "0673",  # Self Type
            "0681",  # Data Class Transforms
            "0695",  # Type Parameter Syntax
        ]

        if identifier == "all":
            peps = important_peps
        else:
            peps = [identifier.zfill(4)]

        for pep_num in peps:
            # Try both .rst and .txt formats
            for ext in [".rst", ".txt"]:
                url = f"{self.raw_url}/pep-{pep_num}{ext}"
                content = await self._fetch(url)
                if content:
                    break

            if not content:
                continue

            yield SourceFile(
                id=SourceFile.generate_id(self.source_type, "peps", f"pep-{pep_num}"),
                path=f"peps/pep-{pep_num}.rst",
                source_type=self.source_type,
                content=content,
                size_bytes=len(content.encode("utf-8")),
                language="python",
                repo="python/peps",
                metadata={
                    "source": "peps",
                    "pep_number": int(pep_num),
                    "url": f"https://peps.python.org/pep-{pep_num}/",
                },
            )

        logger.info("Scraping complete")

    async def search(self, query: str, language: Optional[str] = None, limit: int = 20) -> list[dict]:
        return []

    async def get_metadata(self, identifier: str) -> dict:
        return {"pep": identifier, "source": "peps"}

    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()
