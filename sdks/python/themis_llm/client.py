"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            client.py                                          ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     237                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""ThemisDB LLM Client - Python SDK for LLM operations with Bearer Token authentication."""

import json
import requests
from typing import Dict, List, Optional, Iterator, Any
from .models import InferenceRequest, InferenceResponse, RAGRequest, RAGResponse, ModelInfo
from .exceptions import ThemisAuthError, ThemisAPIError, ThemisConnectionError


class ThemisLLMClient:
    """Client for ThemisDB LLM API with Bearer Token authentication."""
    
    def __init__(self, base_url: str, bearer_token: str, timeout: int = 30):
        """
        Initialize ThemisDB LLM client.
        
        Args:
            base_url: Base URL of ThemisDB server (e.g., 'http://localhost:8080')
            bearer_token: JWT bearer token for authentication
            timeout: Request timeout in seconds
        """
        self.base_url = base_url.rstrip('/')
        self.bearer_token = bearer_token
        self.timeout = timeout
        self.session = requests.Session()
        self.session.headers.update({
            'Authorization': f'Bearer {bearer_token}',
            'Content-Type': 'application/json'
        })
    
    def _make_request(self, method: str, endpoint: str, **kwargs) -> Dict[str, Any]:
        """Make HTTP request with error handling."""
        url = f'{self.base_url}/api/v1/llm{endpoint}'
        try:
            response = self.session.request(method, url, timeout=self.timeout, **kwargs)
            
            if response.status_code == 401:
                raise ThemisAuthError("Invalid or expired bearer token")
            elif response.status_code >= 400:
                raise ThemisAPIError(response.status_code, response.text)
            
            return response.json() if response.content else {}
        except requests.exceptions.RequestException as e:
            raise ThemisConnectionError(f"Connection error: {str(e)}")
    
    def infer(self, prompt: str, model: Optional[str] = None, 
              lora: Optional[str] = None, **options) -> InferenceResponse:
        """
        Perform text inference.
        
        Args:
            prompt: Input text prompt
            model: Model ID (optional)
            lora: LoRA adapter ID (optional)
            **options: Additional options (max_tokens, temperature, etc.)
        
        Returns:
            InferenceResponse with generated text
        """
        data = {'prompt': prompt, **options}
        if model:
            data['model'] = model
        if lora:
            data['lora'] = lora
        
        result = self._make_request('POST', '/inference', json=data)
        return InferenceResponse(**result)
    
    def rag(self, query: str, collection: str, top_k: int = 5,
            lora: Optional[str] = None, **options) -> RAGResponse:
        """
        Perform RAG (Retrieval-Augmented Generation) inference.
        
        Args:
            query: Search query
            collection: Document collection name
            top_k: Number of documents to retrieve
            lora: LoRA adapter ID (optional)
            **options: Additional options
        
        Returns:
            RAGResponse with generated text and source documents
        """
        data = {
            'query': query,
            'collection': collection,
            'top_k': top_k,
            **options
        }
        if lora:
            data['lora'] = lora
        
        result = self._make_request('POST', '/rag', json=data)
        return RAGResponse(**result)
    
    def embed(self, text: str, model: Optional[str] = None) -> List[float]:
        """
        Generate embedding for text.
        
        Args:
            text: Input text
            model: Model ID (optional)
        
        Returns:
            Embedding vector
        """
        data = {'text': text}
        if model:
            data['model'] = model
        
        result = self._make_request('POST', '/embed', json=data)
        return result['embedding']
    
    def stream_infer(self, prompt: str, model: Optional[str] = None,
                     lora: Optional[str] = None, **options) -> Iterator[str]:
        """
        Stream inference tokens in real-time using Server-Sent Events.
        
        Args:
            prompt: Input text prompt
            model: Model ID (optional)
            lora: LoRA adapter ID (optional)
            **options: Additional options
        
        Yields:
            Generated tokens as they become available
        """
        params = {'prompt': prompt, **options}
        if model:
            params['model'] = model
        if lora:
            params['lora'] = lora
        
        url = f'{self.base_url}/api/v1/llm/stream'
        headers = {'Authorization': f'Bearer {self.bearer_token}'}
        
        with requests.get(url, params=params, headers=headers, stream=True) as response:
            if response.status_code != 200:
                raise ThemisAPIError(response.status_code, response.text)
            
            for line in response.iter_lines():
                if line:
                    line_str = line.decode('utf-8')
                    if line_str.startswith('data: '):
                        data = json.loads(line_str[6:])
                        if 'token' in data:
                            yield data['token']
                        if data.get('done', False):
                            break
    
    # Model Management
    def list_models(self) -> List[ModelInfo]:
        """List all available models."""
        result = self._make_request('GET', '/models')
        return [ModelInfo(**m) for m in result.get('models', [])]
    
    def load_model(self, model_id: str, path: str) -> Dict[str, Any]:
        """Load a model from disk."""
        data = {'model_id': model_id, 'path': path}
        return self._make_request('POST', '/models/load', json=data)
    
    def unload_model(self, model_id: str) -> Dict[str, Any]:
        """Unload a model from memory."""
        data = {'model_id': model_id}
        return self._make_request('POST', '/models/unload', json=data)
    
    def get_model_info(self, model_id: str) -> ModelInfo:
        """Get information about a specific model."""
        result = self._make_request('GET', f'/models/{model_id}')
        return ModelInfo(**result)
    
    # LoRA Management
    def list_loras(self) -> List[Dict[str, Any]]:
        """List all available LoRA adapters."""
        result = self._make_request('GET', '/loras')
        return result.get('loras', [])
    
    def load_lora(self, lora_id: str, path: str, model_id: str) -> Dict[str, Any]:
        """Load a LoRA adapter."""
        data = {'lora_id': lora_id, 'path': path, 'model_id': model_id}
        return self._make_request('POST', '/loras/load', json=data)
    
    def unload_lora(self, lora_id: str) -> Dict[str, Any]:
        """Unload a LoRA adapter."""
        data = {'lora_id': lora_id}
        return self._make_request('POST', '/loras/unload', json=data)
    
    # Statistics
    def get_stats(self) -> Dict[str, Any]:
        """Get performance statistics."""
        return self._make_request('GET', '/stats')
    
    def get_cache_stats(self) -> Dict[str, Any]:
        """Get cache statistics."""
        return self._make_request('GET', '/cache/stats')
    
    def clear_cache(self) -> Dict[str, Any]:
        """Clear all caches."""
        return self._make_request('DELETE', '/cache')
    
    def health_check(self) -> Dict[str, Any]:
        """Check health status."""
        return self._make_request('GET', '/health')
    
    def close(self):
        """Close the session."""
        self.session.close()
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.close()
