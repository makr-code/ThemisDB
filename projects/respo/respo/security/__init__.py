"""
RESPO Security Module

Rate limiting and input validation for API security.
"""

import time
import re
import hashlib
from dataclasses import dataclass
from typing import Dict, Optional, List
from collections import defaultdict


@dataclass
class RateLimitConfig:
    """Configuration for rate limiting."""
    
    requests_per_minute: int = 60
    requests_per_hour: int = 1000
    burst_limit: int = 10


class RateLimiter:
    """
    Rate limiter using sliding window algorithm.
    
    Usage:
        limiter = RateLimiter(RateLimitConfig(requests_per_minute=60))
        
        if await limiter.check(client_ip):
            # Process request
        else:
            # Return 429 Too Many Requests
    """
    
    def __init__(self, config: Optional[RateLimitConfig] = None):
        self.config = config or RateLimitConfig()
        self._requests: Dict[str, List[float]] = defaultdict(list)
    
    async def check(self, client_id: str) -> bool:
        """
        Check if request is allowed for client.
        
        Returns True if allowed, False if rate limited.
        """
        now = time.time()
        minute_ago = now - 60
        hour_ago = now - 3600
        
        # Clean old entries
        self._requests[client_id] = [
            t for t in self._requests[client_id]
            if t > hour_ago
        ]
        
        requests = self._requests[client_id]
        
        # Check per-minute limit
        minute_requests = sum(1 for t in requests if t > minute_ago)
        if minute_requests >= self.config.requests_per_minute:
            return False
        
        # Check per-hour limit
        if len(requests) >= self.config.requests_per_hour:
            return False
        
        # Check burst limit (last 5 seconds)
        burst_requests = sum(1 for t in requests if t > now - 5)
        if burst_requests >= self.config.burst_limit:
            return False
        
        # Record request
        requests.append(now)
        return True
    
    def get_remaining(self, client_id: str) -> Dict[str, int]:
        """Get remaining requests for client."""
        now = time.time()
        requests = self._requests.get(client_id, [])
        
        minute_requests = sum(1 for t in requests if t > now - 60)
        hour_requests = sum(1 for t in requests if t > now - 3600)
        
        return {
            "remaining_per_minute": max(0, self.config.requests_per_minute - minute_requests),
            "remaining_per_hour": max(0, self.config.requests_per_hour - hour_requests),
        }
    
    def reset(self, client_id: str):
        """Reset rate limit for client."""
        if client_id in self._requests:
            del self._requests[client_id]


class InputValidator:
    """
    Input validation for code injection prevention.
    
    Usage:
        validator = InputValidator()
        
        if validator.validate_code(user_input):
            # Process code
        else:
            # Reject input
    """
    
    # Patterns that might indicate malicious input
    DANGEROUS_PATTERNS = [
        r"__import__\s*\(",
        r"eval\s*\(",
        r"exec\s*\(",
        r"compile\s*\(",
        r"subprocess\.",
        r"os\.system\s*\(",
        r"os\.popen\s*\(",
        r"shutil\.rmtree\s*\(",
        r"rm\s+-rf",
        r"DROP\s+TABLE",
        r"DELETE\s+FROM",
        r"<script>",
        r"javascript:",
    ]
    
    # Maximum lengths
    MAX_CODE_LENGTH = 100000  # 100KB
    MAX_MESSAGE_LENGTH = 10000  # 10KB
    MAX_QUERY_LENGTH = 1000
    
    def __init__(self, strict_mode: bool = False):
        self.strict_mode = strict_mode
        self._compiled_patterns = [
            re.compile(p, re.IGNORECASE) for p in self.DANGEROUS_PATTERNS
        ]
    
    def validate_code(self, code: str) -> bool:
        """Validate code input."""
        if not code or not isinstance(code, str):
            return False
        
        if len(code) > self.MAX_CODE_LENGTH:
            return False
        
        if self.strict_mode:
            for pattern in self._compiled_patterns:
                if pattern.search(code):
                    return False
        
        return True
    
    def validate_message(self, message: str) -> bool:
        """Validate chat message input."""
        if not message or not isinstance(message, str):
            return False
        
        if len(message) > self.MAX_MESSAGE_LENGTH:
            return False
        
        return True
    
    def validate_query(self, query: str) -> bool:
        """Validate search query input."""
        if not query or not isinstance(query, str):
            return False
        
        if len(query) > self.MAX_QUERY_LENGTH:
            return False
        
        return True
    
    def sanitize_filename(self, filename: str) -> str:
        """Sanitize a filename to prevent path traversal."""
        # Remove path separators and null bytes
        sanitized = re.sub(r'[/\\:\0]', '', filename)
        # Remove leading dots (hidden files/parent dir)
        sanitized = sanitized.lstrip('.')
        return sanitized or "unnamed"
    
    def hash_input(self, input_str: str) -> str:
        """Create a hash of input for caching/logging."""
        return hashlib.sha256(input_str.encode()).hexdigest()[:16]


# Convenience functions
_default_limiter: Optional[RateLimiter] = None
_default_validator: Optional[InputValidator] = None


def get_rate_limiter() -> RateLimiter:
    """Get the default rate limiter instance."""
    global _default_limiter
    if _default_limiter is None:
        _default_limiter = RateLimiter()
    return _default_limiter


def get_input_validator() -> InputValidator:
    """Get the default input validator instance."""
    global _default_validator
    if _default_validator is None:
        _default_validator = InputValidator()
    return _default_validator
