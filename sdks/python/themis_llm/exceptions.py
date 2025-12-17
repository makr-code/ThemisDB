"""Exceptions for ThemisDB LLM SDK."""


class ThemisError(Exception):
    """Base exception for ThemisDB LLM SDK."""
    pass


class ThemisAuthError(ThemisError):
    """Authentication error (invalid or expired bearer token)."""
    pass


class ThemisAPIError(ThemisError):
    """API error from ThemisDB server."""
    
    def __init__(self, status_code: int, message: str):
        self.status_code = status_code
        self.message = message
        super().__init__(f"API Error {status_code}: {message}")


class ThemisConnectionError(ThemisError):
    """Connection error to ThemisDB server."""
    pass
