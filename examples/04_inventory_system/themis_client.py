"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
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
ThemisDB Client für Inventarsystem
Client-Operationen für Produkte, Bestände und Lieferanten
"""

import json
from typing import List, Optional
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from models import Product, StockMovement, Supplier, ProductSupplier, MovementType


# Configuration constants
DEFAULT_TIMEOUT = 10
DEFAULT_RETRY_COUNT = 3


class InventoryClient:
    """
    Client für Inventar-Operationen in ThemisDB.
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = DEFAULT_TIMEOUT
    ):
        """Initialisiert den Inventory Client."""
        self.base_url = f"{protocol}://{host}:{port}"
        self.timeout = timeout
        self.session = self._create_session()
    
    def _create_session(self) -> requests.Session:
        """Erstellt HTTP-Session mit Retry-Logik."""
        session = requests.Session()
        retry_strategy = Retry(
            total=DEFAULT_RETRY_COUNT,
            backoff_factor=0.5,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        return session
    
    def health_check(self) -> bool:
        """Prüft Verbindung zum Server."""
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5
            )
            return response.status_code == 200
        except Exception:
            return False
    
    # Product operations
    def create_product(self, product: Product) -> Product:
        """Erstellt ein neues Produkt."""
        entity_key = f"products:{product.id}"
        product_data = product.to_dict()
        product_data_clean = {k: v for k, v in product_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(product_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return product
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create product: {str(e)}")
    
    def get_product(self, product_id: str) -> Optional[Product]:
        """Ruft ein Produkt ab."""
        entity_key = f"products:{product_id}"
        
        try:
            response = self.session.get(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code == 404:
                return None
            
            response.raise_for_status()
            
            data = response.json()
            if "blob" in data:
                product_data = json.loads(data["blob"])
                product_data["id"] = product_id
                return Product.from_dict(product_data)
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get product: {str(e)}")
    
    def update_product(self, product: Product) -> Product:
        """Aktualisiert ein Produkt."""
        return self.create_product(product)
    
    def delete_product(self, product_id: str) -> bool:
        """Löscht ein Produkt."""
        entity_key = f"products:{product_id}"
        
        try:
            response = self.session.delete(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code in [200, 204, 404]:
                return True
            
            response.raise_for_status()
            return True
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to delete product: {str(e)}")
    
    # Stock movement operations
    def record_movement(self, movement: StockMovement) -> StockMovement:
        """Erfasst eine Bestandsbewegung."""
        entity_key = f"movements:{movement.id}"
        movement_data = movement.to_dict()
        movement_data_clean = {k: v for k, v in movement_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(movement_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return movement
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to record movement: {str(e)}")
    
    # Supplier operations
    def create_supplier(self, supplier: Supplier) -> Supplier:
        """Erstellt einen neuen Lieferanten."""
        entity_key = f"suppliers:{supplier.id}"
        supplier_data = supplier.to_dict()
        supplier_data_clean = {k: v for k, v in supplier_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(supplier_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return supplier
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create supplier: {str(e)}")
    
    def get_supplier(self, supplier_id: str) -> Optional[Supplier]:
        """Ruft einen Lieferanten ab."""
        entity_key = f"suppliers:{supplier_id}"
        
        try:
            response = self.session.get(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code == 404:
                return None
            
            response.raise_for_status()
            
            data = response.json()
            if "blob" in data:
                supplier_data = json.loads(data["blob"])
                supplier_data["id"] = supplier_id
                return Supplier.from_dict(supplier_data)
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get supplier: {str(e)}")
    
    # Product-Supplier relationship
    def link_product_supplier(self, link: ProductSupplier) -> ProductSupplier:
        """Verknüpft Produkt mit Lieferant."""
        entity_key = f"product_suppliers:{link.product_id}:{link.supplier_id}"
        link_data = link.to_dict()
        
        payload = {"blob": json.dumps(link_data)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return link
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to link product-supplier: {str(e)}")
