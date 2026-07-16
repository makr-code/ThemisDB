"""Impact Classification System for ThemisDB AI-Vibe Findings

Maps file paths to impact levels based on module criticality:
- CRITICAL: Core database, auth, crypto, distributed consensus
- HIGH: LLM integration, networking, graph operations
- MEDIUM: Monitoring, multi-GPU, MQTT, model serving
- LOW: Utilities, helpers, formatting, logging
"""

import re
from pathlib import Path
from typing import Tuple

class ImpactClassifier:
    """Classify findings by ThemisDB module criticality."""
    
    # Priority 1: CRITICAL impact modules
    CRITICAL_PATTERNS = [
        r'src/core/.*\.(cpp|h|hpp)',           # Core database engine
        r'src/distributed/.*consensus.*',      # Distributed consensus
        r'src/auth/.*\.(cpp|h|hpp)',           # Authentication
        r'src/security/.*\.(cpp|h|hpp)',       # Security/crypto
        r'src/storage/.*\.(cpp|h|hpp)',        # Storage engine
        r'include/core/.*\.(h|hpp)',           # Core public APIs
        r'include/auth/.*\.(h|hpp)',           # Auth APIs
    ]
    
    # Priority 2: HIGH impact modules
    HIGH_PATTERNS = [
        r'src/llm/.*\.(cpp|h|hpp)',            # LLM integration
        r'src/ai_.*\.(cpp|h|hpp)',             # AI modules
        r'src/network/.*\.(cpp|h|hpp)',        # Networking
        r'src/graph/.*\.(cpp|h|hpp)',          # Graph operations
        r'src/model/.*\.(cpp|h|hpp)',          # Model serving
        r'include/llm/.*\.(h|hpp)',            # LLM APIs
        r'include/network/.*\.(h|hpp)',        # Network APIs
    ]
    
    # Priority 3: MEDIUM impact modules
    MEDIUM_PATTERNS = [
        r'src/monitoring/.*\.(cpp|h|hpp)',     # Monitoring/observability
        r'src/multi/.*\.(cpp|h|hpp)',          # Multi-GPU coordination
        r'src/mqtt/.*\.(cpp|h|hpp)',           # MQTT protocol
        r'src/mongo/.*\.(cpp|h|hpp)',          # MongoDB integration
        r'src/kafka/.*\.(cpp|h|hpp)',          # Kafka integration
        r'src/module/.*\.(cpp|h|hpp)',         # Module system
        r'include/monitoring/.*\.(h|hpp)',     # Monitoring APIs
    ]
    
    # Priority 4: LOW impact (helpers, utilities)
    LOW_PATTERNS = [
        r'src/utils/.*\.(cpp|h|hpp)',          # Utility functions
        r'src/helper.*\.(cpp|h|hpp)',          # Helper modules
        r'src/format/.*\.(cpp|h|hpp)',         # Formatting
        r'src/logging/.*\.(cpp|h|hpp)',        # Logging utilities
        r'src/pool/.*\.(cpp|h|hpp)',           # Memory pools
        r'benchmarks/.*\.(cpp|h|hpp)',         # Benchmarks
        r'include/utils/.*\.(h|hpp)',          # Utility APIs
    ]
    
    # Lowest priority: Third-party / external
    THIRD_PARTY_PATTERNS = [
        r'third_party/.*',
        r'external/.*',
        r'vendor/.*',
    ]
    
    @staticmethod
    def classify(filepath: str) -> Tuple[str, str]:
        """
        Classify file by impact level.
        
        Returns:
            (impact_level, subsystem) where:
            - impact_level: "CRITICAL", "HIGH", "MEDIUM", "LOW", "THIRD_PARTY"
            - subsystem: module name (e.g., "llm", "graph", "core", "utils")
        """
        filepath = filepath.replace('\\', '/')
        
        # Check critical first (highest priority)
        for pattern in ImpactClassifier.CRITICAL_PATTERNS:
            if re.search(pattern, filepath):
                subsys = ImpactClassifier._extract_subsystem(filepath)
                return ("CRITICAL", subsys)
        
        # Then HIGH
        for pattern in ImpactClassifier.HIGH_PATTERNS:
            if re.search(pattern, filepath):
                subsys = ImpactClassifier._extract_subsystem(filepath)
                return ("HIGH", subsys)
        
        # Then MEDIUM
        for pattern in ImpactClassifier.MEDIUM_PATTERNS:
            if re.search(pattern, filepath):
                subsys = ImpactClassifier._extract_subsystem(filepath)
                return ("MEDIUM", subsys)
        
        # Then LOW (utilities)
        for pattern in ImpactClassifier.LOW_PATTERNS:
            if re.search(pattern, filepath):
                subsys = ImpactClassifier._extract_subsystem(filepath)
                return ("LOW", subsys)
        
        # Third-party
        for pattern in ImpactClassifier.THIRD_PARTY_PATTERNS:
            if re.search(pattern, filepath):
                return ("THIRD_PARTY", "external")
        
        # Default: LOW (unclassified)
        return ("LOW", "misc")
    
    @staticmethod
    def _extract_subsystem(filepath: str) -> str:
        """Extract subsystem name from filepath."""
        # src/llm/module.cpp → llm
        # src/core/database/engine.cpp → core
        # include/graph/visitor.hpp → graph
        
        parts = filepath.replace('\\', '/').split('/')
        
        # Look for subsystem identifier in path
        for part in parts:
            if part in ['core', 'llm', 'ai', 'network', 'graph', 'model', 'auth',
                       'security', 'storage', 'monitoring', 'multi', 'mqtt', 'mongo',
                       'kafka', 'module', 'distributed', 'utils', 'helper', 'format', 'logging']:
                return part
        
        # Fallback: use directory after src/
        if 'src/' in filepath:
            after_src = filepath.split('src/')[1].split('/')[0]
            return after_src if after_src else 'misc'
        
        return 'misc'


# Test
if __name__ == '__main__':
    test_files = [
        'src/core/database/engine.cpp',
        'src/llm/plugin_manager.cpp',
        'src/utils/string_helpers.cpp',
        'src/graph/visitor.hpp',
        'third_party/json/nlohmann_json.hpp',
        'benchmarks/graph_traversal.cpp',
        'include/auth/credentials.hpp',
    ]
    
    for f in test_files:
        impact, subsys = ImpactClassifier.classify(f)
        print(f"{f:50s} → {impact:15s} ({subsys})")
