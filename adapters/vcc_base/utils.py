"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            utils.py                                           ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:34:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Utility functions for VCC adapters.
"""

import logging
import sys
from typing import Optional
import httpx


def setup_logging(level: str = "INFO", adapter_name: str = "vcc_adapter") -> logging.Logger:
    """
    Configure logging for VCC adapters.
    
    Args:
        level: Log level (DEBUG, INFO, WARNING, ERROR)
        adapter_name: Name of the adapter for log messages
        
    Returns:
        Configured logger instance
    """
    log_level = getattr(logging, level.upper(), logging.INFO)
    
    logging.basicConfig(
        level=log_level,
        format=f"%(asctime)s - {adapter_name} - %(name)s - %(levelname)s - %(message)s",
        handlers=[logging.StreamHandler(sys.stdout)]
    )
    
    logger = logging.getLogger(adapter_name)
    logger.info(f"Logging configured at {level} level for {adapter_name}")
    
    return logger


async def validate_themis_connection(base_url: str, timeout: float = 10.0) -> bool:
    """
    Validate connection to ThemisDB.
    
    Args:
        base_url: ThemisDB base URL
        timeout: Connection timeout in seconds
        
    Returns:
        True if connection successful, False otherwise
    """
    try:
        async with httpx.AsyncClient(timeout=timeout) as client:
            response = await client.get(f"{base_url}/health")
            response.raise_for_status()
            data = response.json()
            
            logging.info(f"ThemisDB connection validated: {data}")
            return True
            
    except Exception as e:
        logging.error(f"Failed to connect to ThemisDB at {base_url}: {e}")
        return False
