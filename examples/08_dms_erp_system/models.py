"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     342                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Datenmodelle für DMS/ERP-System
Demonstriert komplexe Multi-Model-Architektur mit ThemisDB
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional, List, Dict
from enum import Enum


class DocumentType(Enum):
    """Dokumenttypen"""
    INVOICE = "invoice"
    ORDER = "order"
    CONTRACT = "contract"
    DELIVERY_NOTE = "delivery_note"
    OFFER = "offer"
    REPORT = "report"
    OTHER = "other"


class DocumentStatus(Enum):
    """Dokumentstatus"""
    DRAFT = "draft"
    PENDING = "pending"
    APPROVED = "approved"
    REJECTED = "rejected"
    ARCHIVED = "archived"


class Priority(Enum):
    """Priorität"""
    LOW = "low"
    NORMAL = "normal"
    HIGH = "high"
    URGENT = "urgent"


class OrderStatus(Enum):
    """Bestellstatus"""
    NEW = "new"
    CONFIRMED = "confirmed"
    IN_PRODUCTION = "in_production"
    SHIPPED = "shipped"
    DELIVERED = "delivered"
    CANCELLED = "cancelled"


@dataclass
class Document:
    """Dokument im DMS"""
    id: str
    title: str
    content: str
    doc_type: str
    status: str
    priority: str
    tags: List[str]
    created_at: str
    updated_at: str
    created_by: str
    file_path: Optional[str] = None
    file_size: Optional[int] = None
    metadata: Dict = field(default_factory=dict)
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary"""
        return {
            'id': self.id,
            'title': self.title,
            'content': self.content,
            'doc_type': self.doc_type,
            'status': self.status,
            'priority': self.priority,
            'tags': self.tags,
            'created_at': self.created_at,
            'updated_at': self.updated_at,
            'created_by': self.created_by,
            'file_path': self.file_path,
            'file_size': self.file_size,
            'metadata': self.metadata
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Document':
        """Erstellt aus Dictionary"""
        return Document(**data)


@dataclass
class Customer:
    """Kunde im ERP"""
    id: str
    name: str
    company: str
    email: str
    phone: str
    address: str
    city: str
    postal_code: str
    country: str
    customer_number: str
    credit_limit: float
    balance: float
    created_at: str
    notes: str = ""
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary"""
        return {
            'id': self.id,
            'name': self.name,
            'company': self.company,
            'email': self.email,
            'phone': self.phone,
            'address': self.address,
            'city': self.city,
            'postal_code': self.postal_code,
            'country': self.country,
            'customer_number': self.customer_number,
            'credit_limit': self.credit_limit,
            'balance': self.balance,
            'created_at': self.created_at,
            'notes': self.notes
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Customer':
        """Erstellt aus Dictionary"""
        return Customer(**data)


@dataclass
class OrderItem:
    """Bestellposition"""
    product_id: str
    product_name: str
    quantity: int
    unit_price: float
    discount: float
    total: float
    
    def to_dict(self) -> dict:
        return {
            'product_id': self.product_id,
            'product_name': self.product_name,
            'quantity': self.quantity,
            'unit_price': self.unit_price,
            'discount': self.discount,
            'total': self.total
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'OrderItem':
        return OrderItem(**data)


@dataclass
class Order:
    """Bestellung im ERP"""
    id: str
    order_number: str
    customer_id: str
    customer_name: str
    status: str
    items: List[Dict]
    subtotal: float
    tax: float
    total: float
    created_at: str
    updated_at: str
    delivery_date: Optional[str] = None
    notes: str = ""
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary"""
        return {
            'id': self.id,
            'order_number': self.order_number,
            'customer_id': self.customer_id,
            'customer_name': self.customer_name,
            'status': self.status,
            'items': self.items,
            'subtotal': self.subtotal,
            'tax': self.tax,
            'total': self.total,
            'created_at': self.created_at,
            'updated_at': self.updated_at,
            'delivery_date': self.delivery_date,
            'notes': self.notes
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Order':
        """Erstellt aus Dictionary"""
        return Order(**data)


@dataclass
class AuditLog:
    """Audit-Log für Compliance"""
    id: str
    entity_type: str
    entity_id: str
    action: str
    user: str
    timestamp: str
    changes: Dict
    ip_address: str = ""
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary"""
        return {
            'id': self.id,
            'entity_type': self.entity_type,
            'entity_id': self.entity_id,
            'action': self.action,
            'user': self.user,
            'timestamp': self.timestamp,
            'changes': self.changes,
            'ip_address': self.ip_address
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'AuditLog':
        """Erstellt aus Dictionary"""
        return AuditLog(**data)


class DMSStats:
    """Statistiken für DMS/ERP Dashboard"""
    
    @staticmethod
    def calculate_document_stats(documents: List[Document]) -> Dict:
        """Berechnet Dokumentstatistiken"""
        if not documents:
            return {
                'total': 0,
                'by_type': {},
                'by_status': {},
                'by_priority': {},
                'total_size': 0
            }
        
        stats = {
            'total': len(documents),
            'by_type': {},
            'by_status': {},
            'by_priority': {},
            'total_size': 0
        }
        
        for doc in documents:
            # Nach Typ
            stats['by_type'][doc.doc_type] = stats['by_type'].get(doc.doc_type, 0) + 1
            
            # Nach Status
            stats['by_status'][doc.status] = stats['by_status'].get(doc.status, 0) + 1
            
            # Nach Priorität
            stats['by_priority'][doc.priority] = stats['by_priority'].get(doc.priority, 0) + 1
            
            # Gesamtgröße
            if doc.file_size:
                stats['total_size'] += doc.file_size
        
        return stats
    
    @staticmethod
    def calculate_order_stats(orders: List[Order]) -> Dict:
        """Berechnet Bestellstatistiken"""
        if not orders:
            return {
                'total': 0,
                'by_status': {},
                'total_revenue': 0.0,
                'avg_order_value': 0.0
            }
        
        stats = {
            'total': len(orders),
            'by_status': {},
            'total_revenue': 0.0,
            'avg_order_value': 0.0
        }
        
        total_value = 0.0
        for order in orders:
            # Nach Status
            stats['by_status'][order.status] = stats['by_status'].get(order.status, 0) + 1
            
            # Umsatz
            total_value += order.total
        
        stats['total_revenue'] = total_value
        stats['avg_order_value'] = total_value / len(orders) if orders else 0.0
        
        return stats
    
    @staticmethod
    def calculate_customer_stats(customers: List[Customer]) -> Dict:
        """Berechnet Kundenstatistiken"""
        if not customers:
            return {
                'total': 0,
                'total_credit_limit': 0.0,
                'total_balance': 0.0,
                'avg_balance': 0.0
            }
        
        total_credit = sum(c.credit_limit for c in customers)
        total_balance = sum(c.balance for c in customers)
        
        return {
            'total': len(customers),
            'total_credit_limit': total_credit,
            'total_balance': total_balance,
            'avg_balance': total_balance / len(customers) if customers else 0.0
        }
