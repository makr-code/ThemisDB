"""
Tests for GitHub Scraper
"""

import pytest

from respo.ingestion import (
    CodeFile,
    GitHubScraper,
    ScraperConfig,
    parse_github_url,
)


class TestParseGitHubUrl:
    """Tests for parse_github_url function."""

    def test_https_url(self) -> None:
        """Test parsing HTTPS GitHub URL."""
        owner, repo = parse_github_url("https://github.com/owner/repo")
        assert owner == "owner"
        assert repo == "repo"

    def test_https_url_with_git_suffix(self) -> None:
        """Test parsing HTTPS URL with .git suffix."""
        owner, repo = parse_github_url("https://github.com/owner/repo.git")
        assert owner == "owner"
        assert repo == "repo"

    def test_ssh_url(self) -> None:
        """Test parsing SSH GitHub URL."""
        owner, repo = parse_github_url("git@github.com:owner/repo.git")
        assert owner == "owner"
        assert repo == "repo"

    def test_owner_repo_format(self) -> None:
        """Test parsing owner/repo format."""
        owner, repo = parse_github_url("owner/repo")
        assert owner == "owner"
        assert repo == "repo"

    def test_invalid_url(self) -> None:
        """Test invalid URL raises ValueError."""
        with pytest.raises(ValueError):
            parse_github_url("not-a-valid-url")

    def test_complex_repo_name(self) -> None:
        """Test parsing URL with complex repo name."""
        owner, repo = parse_github_url("https://github.com/my-org/my-awesome-repo")
        assert owner == "my-org"
        assert repo == "my-awesome-repo"

    def test_repo_name_with_dots(self) -> None:
        """Test parsing URL with dots in repo name."""
        owner, repo = parse_github_url("https://github.com/owner/repo.js")
        assert owner == "owner"
        assert repo == "repo.js"

    def test_repo_name_with_dots_and_git_suffix(self) -> None:
        """Test parsing URL with dots in repo name and .git suffix."""
        owner, repo = parse_github_url("https://github.com/owner/my.dotted.repo.git")
        assert owner == "owner"
        assert repo == "my.dotted.repo"


class TestScraperConfig:
    """Tests for ScraperConfig."""

    def test_default_config(self) -> None:
        """Test default configuration values."""
        config = ScraperConfig()
        assert config.clone_depth == 1
        assert config.max_file_size_kb == 500
        assert ".py" in config.include_extensions
        assert "**/node_modules/**" in config.exclude_patterns

    def test_custom_config(self) -> None:
        """Test custom configuration."""
        config = ScraperConfig(
            github_token="test_token",
            clone_depth=10,
            max_file_size_kb=1000,
        )
        assert config.github_token == "test_token"
        assert config.clone_depth == 10
        assert config.max_file_size_kb == 1000


class TestGitHubScraper:
    """Tests for GitHubScraper class."""

    def test_detect_language_python(self) -> None:
        """Test Python language detection."""
        scraper = GitHubScraper()
        assert scraper._detect_language("test.py") == "python"
        assert scraper._detect_language("test.pyi") == "python"

    def test_detect_language_javascript(self) -> None:
        """Test JavaScript language detection."""
        scraper = GitHubScraper()
        assert scraper._detect_language("test.js") == "javascript"
        assert scraper._detect_language("test.jsx") == "javascript"

    def test_detect_language_typescript(self) -> None:
        """Test TypeScript language detection."""
        scraper = GitHubScraper()
        assert scraper._detect_language("test.ts") == "typescript"
        assert scraper._detect_language("test.tsx") == "typescript"

    def test_detect_language_unknown(self) -> None:
        """Test unknown language detection."""
        scraper = GitHubScraper()
        assert scraper._detect_language("test.xyz") == "unknown"

    def test_should_include_file_python(self) -> None:
        """Test Python file should be included."""
        scraper = GitHubScraper()
        assert scraper._should_include_file("src/main.py") is True

    def test_should_include_file_node_modules(self) -> None:
        """Test node_modules should be excluded."""
        scraper = GitHubScraper()
        assert scraper._should_include_file("node_modules/test.js") is False

    def test_should_include_file_wrong_extension(self) -> None:
        """Test wrong extension should be excluded."""
        scraper = GitHubScraper()
        assert scraper._should_include_file("test.exe") is False

    def test_get_headers_without_token(self) -> None:
        """Test headers without GitHub token."""
        scraper = GitHubScraper(ScraperConfig(github_token=None))
        headers = scraper._get_headers()
        assert "Authorization" not in headers
        assert "User-Agent" in headers

    def test_get_headers_with_token(self) -> None:
        """Test headers with GitHub token."""
        scraper = GitHubScraper(ScraperConfig(github_token="test_token"))
        headers = scraper._get_headers()
        assert headers["Authorization"] == "token test_token"


class TestCodeFile:
    """Tests for CodeFile dataclass."""

    def test_code_file_creation(self) -> None:
        """Test CodeFile creation."""
        code_file = CodeFile(
            path="src/main.py",
            content="print('hello')",
            language="python",
            size_bytes=14,
            repo="owner/repo",
            branch="main",
        )
        assert code_file.path == "src/main.py"
        assert code_file.language == "python"
        assert code_file.repo == "owner/repo"

    def test_code_file_with_metadata(self) -> None:
        """Test CodeFile with metadata."""
        code_file = CodeFile(
            path="src/main.py",
            content="print('hello')",
            language="python",
            size_bytes=14,
            repo="owner/repo",
            branch="main",
            commit_sha="abc123",
            license="MIT",
            metadata={"stars": 100},
        )
        assert code_file.commit_sha == "abc123"
        assert code_file.license == "MIT"
        assert code_file.metadata["stars"] == 100
