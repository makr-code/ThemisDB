"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     319                                            ║
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
ThemisDB Client für DMS/ERP-System
Demonstriert komplexe Multi-Model-Integration
"""

import requests
from typing import List, Optional, Dict
from datetime import datetime
import uuid
from models import Document, Customer, Order, AuditLog


class DMSERPClient:
    """Client für DMS/ERP-Operationen mit ThemisDB"""
    
    def __init__(self, host: str = "localhost", port: int = 8080, timeout: int = 10):
        """
        Initialisiert den Client
        
        Args:
            host: ThemisDB Host
            port: ThemisDB Port
            timeout: Request Timeout in Sekunden
        """
        self.base_url = f"http://{host}:{port}/api/v1"
        self.timeout = timeout
        self.current_user = "admin"  # Für Audit-Logging
    
    # ==================== Dokument-Operationen ====================
    
    def create_document(self, doc: Document) -> bool:
        """Erstellt ein neues Dokument"""
        try:
            response = requests.post(
                f"{self.base_url}/documents",
                json=doc.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 201:
                # Audit-Log erstellen
                self._create_audit_log("document", doc.id, "create", {})
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    def get_document(self, doc_id: str) -> Optional[Document]:
        """Holt ein Dokument"""
        try:
            response = requests.get(
                f"{self.base_url}/documents/{doc_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return Document.from_dict(response.json())
            return None
        except requests.exceptions.RequestException:
            return None
    
    def list_documents(self, filters: Optional[Dict] = None) -> List[Document]:
        """Listet alle Dokumente"""
        try:
            params = filters or {}
            response = requests.get(
                f"{self.base_url}/documents",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Document.from_dict(d) for d in response.json()]
            return []
        except requests.exceptions.RequestException:
            return []
    
    def update_document(self, doc: Document, changes: Dict) -> bool:
        """Aktualisiert ein Dokument"""
        try:
            response = requests.put(
                f"{self.base_url}/documents/{doc.id}",
                json=doc.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 200:
                # Audit-Log mit Änderungen
                self._create_audit_log("document", doc.id, "update", changes)
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    def delete_document(self, doc_id: str) -> bool:
        """Löscht ein Dokument"""
        try:
            response = requests.delete(
                f"{self.base_url}/documents/{doc_id}",
                timeout=self.timeout
            )
            if response.status_code == 204:
                # Audit-Log
                self._create_audit_log("document", doc_id, "delete", {})
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    def search_documents(self, query: str, filters: Optional[Dict] = None) -> List[Document]:
        """Sucht Dokumente"""
        try:
            params = {"q": query}
            if filters:
                params.update(filters)
            
            response = requests.get(
                f"{self.base_url}/documents/search",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Document.from_dict(d) for d in response.json()]
            return []
        except requests.exceptions.RequestException:
            return []
    
    # ==================== Kunden-Operationen ====================
    
    def create_customer(self, customer: Customer) -> bool:
        """Erstellt einen neuen Kunden"""
        try:
            response = requests.post(
                f"{self.base_url}/customers",
                json=customer.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 201:
                self._create_audit_log("customer", customer.id, "create", {})
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    def get_customer(self, customer_id: str) -> Optional[Customer]:
        """Holt einen Kunden"""
        try:
            response = requests.get(
                f"{self.base_url}/customers/{customer_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return Customer.from_dict(response.json())
            return None
        except requests.exceptions.RequestException:
            return None
    
    def list_customers(self) -> List[Customer]:
        """Listet alle Kunden"""
        try:
            response = requests.get(
                f"{self.base_url}/customers",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Customer.from_dict(c) for c in response.json()]
            return []
        except requests.exceptions.RequestException:
            return []
    
    def update_customer(self, customer: Customer, changes: Dict) -> bool:
        """Aktualisiert einen Kunden"""
        try:
            response = requests.put(
                f"{self.base_url}/customers/{customer.id}",
                json=customer.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 200:
                self._create_audit_log("customer", customer.id, "update", changes)
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    # ==================== Bestellungs-Operationen ====================
    
    def create_order(self, order: Order) -> bool:
        """Erstellt eine neue Bestellung"""
        try:
            response = requests.post(
                f"{self.base_url}/orders",
                json=order.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 201:
                self._create_audit_log("order", order.id, "create", {})
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    def get_order(self, order_id: str) -> Optional[Order]:
        """Holt eine Bestellung"""
        try:
            response = requests.get(
                f"{self.base_url}/orders/{order_id}",
                timeout=self.timeout
            )
            if response.status_code == 200:
                return Order.from_dict(response.json())
            return None
        except requests.exceptions.RequestException:
            return None
    
    def list_orders(self, customer_id: Optional[str] = None) -> List[Order]:
        """Listet Bestellungen"""
        try:
            params = {"customer_id": customer_id} if customer_id else {}
            response = requests.get(
                f"{self.base_url}/orders",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [Order.from_dict(o) for o in response.json()]
            return []
        except requests.exceptions.RequestException:
            return []
    
    def update_order(self, order: Order, changes: Dict) -> bool:
        """Aktualisiert eine Bestellung"""
        try:
            response = requests.put(
                f"{self.base_url}/orders/{order.id}",
                json=order.to_dict(),
                timeout=self.timeout
            )
            if response.status_code == 200:
                self._create_audit_log("order", order.id, "update", changes)
                return True
            return False
        except requests.exceptions.RequestException:
            return False
    
    # ==================== Audit-Log-Operationen ====================
    
    def _create_audit_log(self, entity_type: str, entity_id: str, action: str, changes: Dict) -> bool:
        """Erstellt einen Audit-Log-Eintrag (intern)"""
        try:
            log = AuditLog(
                id=str(uuid.uuid4()),
                entity_type=entity_type,
                entity_id=entity_id,
                action=action,
                user=self.current_user,
                timestamp=datetime.now().isoformat(),
                changes=changes,
                ip_address="127.0.0.1"
            )
            
            response = requests.post(
                f"{self.base_url}/audit_logs",
                json=log.to_dict(),
                timeout=self.timeout
            )
            return response.status_code == 201
        except requests.exceptions.RequestException:
            return False
    
    def get_audit_logs(self, entity_type: Optional[str] = None, 
                       entity_id: Optional[str] = None) -> List[AuditLog]:
        """Holt Audit-Logs"""
        try:
            params = {}
            if entity_type:
                params['entity_type'] = entity_type
            if entity_id:
                params['entity_id'] = entity_id
            
            response = requests.get(
                f"{self.base_url}/audit_logs",
                params=params,
                timeout=self.timeout
            )
            if response.status_code == 200:
                return [AuditLog.from_dict(log) for log in response.json()]
            return []
        except requests.exceptions.RequestException:
            return []
    
    # ==================== Hilfsfunktionen ====================
    
    def check_connection(self) -> bool:
        """Prüft Verbindung zu ThemisDB"""
        try:
            response = requests.get(f"{self.base_url}/health", timeout=self.timeout)
            return response.status_code == 200
        except requests.exceptions.RequestException:
            return False
