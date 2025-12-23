#!/usr/bin/env python3
"""
TPC-C Schema Definition for ThemisDB

Based on TPC-C Specification 5.11
http://www.tpc.org/tpcc/spec/tpcc_current.pdf

This module defines the database schema for the TPC-C benchmark,
including all tables, indexes, and constraints.
"""

from dataclasses import dataclass
from typing import List, Dict
from datetime import datetime
from decimal import Decimal


@dataclass
class WarehouseSchema:
    """
    WAREHOUSE table (W rows)
    
    Primary Key: W_ID
    """
    W_ID: int  # 2*W unique IDs
    W_NAME: str  # variable text, size 10
    W_STREET_1: str  # variable text, size 20
    W_STREET_2: str  # variable text, size 20
    W_CITY: str  # variable text, size 20
    W_STATE: str  # fixed text, size 2
    W_ZIP: str  # fixed text, size 9
    W_TAX: Decimal  # signed numeric(4,4)
    W_YTD: Decimal  # signed numeric(12,2)


@dataclass
class DistrictSchema:
    """
    DISTRICT table (10*W rows)
    
    Primary Key: (D_W_ID, D_ID)
    Foreign Key: D_W_ID references WAREHOUSE(W_ID)
    """
    D_ID: int  # 20 unique IDs
    D_W_ID: int  # 2*W unique IDs
    D_NAME: str  # variable text, size 10
    D_STREET_1: str  # variable text, size 20
    D_STREET_2: str  # variable text, size 20
    D_CITY: str  # variable text, size 20
    D_STATE: str  # fixed text, size 2
    D_ZIP: str  # fixed text, size 9
    D_TAX: Decimal  # signed numeric(4,4)
    D_YTD: Decimal  # signed numeric(12,2)
    D_NEXT_O_ID: int  # 10,000,000 unique IDs


@dataclass
class CustomerSchema:
    """
    CUSTOMER table (30,000*W rows)
    
    Primary Key: (C_W_ID, C_D_ID, C_ID)
    Foreign Key: (C_W_ID, C_D_ID) references DISTRICT(D_W_ID, D_ID)
    """
    C_ID: int  # 96,000 unique IDs
    C_D_ID: int  # 20 unique IDs
    C_W_ID: int  # 2*W unique IDs
    C_FIRST: str  # variable text, size 16
    C_MIDDLE: str  # fixed text, size 2
    C_LAST: str  # variable text, size 16
    C_STREET_1: str  # variable text, size 20
    C_STREET_2: str  # variable text, size 20
    C_CITY: str  # variable text, size 20
    C_STATE: str  # fixed text, size 2
    C_ZIP: str  # fixed text, size 9
    C_PHONE: str  # fixed text, size 16
    C_SINCE: datetime  # date and time
    C_CREDIT: str  # fixed text, size 2 ("GC"=good, "BC"=bad)
    C_CREDIT_LIM: Decimal  # signed numeric(12,2)
    C_DISCOUNT: Decimal  # signed numeric(4,4)
    C_BALANCE: Decimal  # signed numeric(12,2)
    C_YTD_PAYMENT: Decimal  # signed numeric(12,2)
    C_PAYMENT_CNT: int  # numeric(4)
    C_DELIVERY_CNT: int  # numeric(4)
    C_DATA: str  # variable text, size 500


@dataclass
class HistorySchema:
    """
    HISTORY table (30,000*W rows initially)
    
    Primary Key: None
    Foreign Key: (H_C_W_ID, H_C_D_ID, H_C_ID) references CUSTOMER
    Foreign Key: (H_W_ID, H_D_ID) references DISTRICT
    """
    H_C_ID: int  # 96,000 unique IDs
    H_C_D_ID: int  # 20 unique IDs
    H_C_W_ID: int  # 2*W unique IDs
    H_D_ID: int  # 20 unique IDs
    H_W_ID: int  # 2*W unique IDs
    H_DATE: datetime  # date and time
    H_AMOUNT: Decimal  # signed numeric(6,2)
    H_DATA: str  # variable text, size 24


@dataclass
class NewOrderSchema:
    """
    NEW-ORDER table (9,000*W rows initially)
    
    Primary Key: (NO_W_ID, NO_D_ID, NO_O_ID)
    Foreign Key: (NO_W_ID, NO_D_ID, NO_O_ID) references ORDERS
    """
    NO_O_ID: int  # 10,000,000 unique IDs
    NO_D_ID: int  # 20 unique IDs
    NO_W_ID: int  # 2*W unique IDs


@dataclass
class OrderSchema:
    """
    ORDERS table (30,000*W rows initially)
    
    Primary Key: (O_W_ID, O_D_ID, O_ID)
    Foreign Key: (O_W_ID, O_D_ID, O_C_ID) references CUSTOMER
    """
    O_ID: int  # 10,000,000 unique IDs
    O_D_ID: int  # 20 unique IDs
    O_W_ID: int  # 2*W unique IDs
    O_C_ID: int  # 96,000 unique IDs
    O_ENTRY_D: datetime  # date and time
    O_CARRIER_ID: int  # 10 unique IDs, or null
    O_OL_CNT: int  # numeric(2)
    O_ALL_LOCAL: int  # numeric(1)


@dataclass
class OrderLineSchema:
    """
    ORDER-LINE table (300,000*W rows initially)
    
    Primary Key: (OL_W_ID, OL_D_ID, OL_O_ID, OL_NUMBER)
    Foreign Key: (OL_W_ID, OL_D_ID, OL_O_ID) references ORDERS
    Foreign Key: (OL_SUPPLY_W_ID, OL_I_ID) references STOCK
    """
    OL_O_ID: int  # 10,000,000 unique IDs
    OL_D_ID: int  # 20 unique IDs
    OL_W_ID: int  # 2*W unique IDs
    OL_NUMBER: int  # 15 unique IDs
    OL_I_ID: int  # 200,000 unique IDs
    OL_SUPPLY_W_ID: int  # 2*W unique IDs
    OL_DELIVERY_D: datetime  # date and time, or null
    OL_QUANTITY: int  # numeric(2)
    OL_AMOUNT: Decimal  # signed numeric(6,2)
    OL_DIST_INFO: str  # fixed text, size 24


@dataclass
class ItemSchema:
    """
    ITEM table (100,000 rows)
    
    Primary Key: I_ID
    """
    I_ID: int  # 200,000 unique IDs
    I_IM_ID: int  # 200,000 unique IDs
    I_NAME: str  # variable text, size 24
    I_PRICE: Decimal  # numeric(5,2)
    I_DATA: str  # variable text, size 50


@dataclass
class StockSchema:
    """
    STOCK table (100,000*W rows)
    
    Primary Key: (S_W_ID, S_I_ID)
    Foreign Key: S_W_ID references WAREHOUSE(W_ID)
    Foreign Key: S_I_ID references ITEM(I_ID)
    """
    S_I_ID: int  # 200,000 unique IDs
    S_W_ID: int  # 2*W unique IDs
    S_QUANTITY: int  # signed numeric(4)
    S_DIST_01: str  # fixed text, size 24
    S_DIST_02: str  # fixed text, size 24
    S_DIST_03: str  # fixed text, size 24
    S_DIST_04: str  # fixed text, size 24
    S_DIST_05: str  # fixed text, size 24
    S_DIST_06: str  # fixed text, size 24
    S_DIST_07: str  # fixed text, size 24
    S_DIST_08: str  # fixed text, size 24
    S_DIST_09: str  # fixed text, size 24
    S_DIST_10: str  # fixed text, size 24
    S_YTD: int  # numeric(8)
    S_ORDER_CNT: int  # numeric(4)
    S_REMOTE_CNT: int  # numeric(4)
    S_DATA: str  # variable text, size 50


class TPCCSchema:
    """
    TPC-C Database Schema Manager
    
    Provides schema definitions and DDL generation for ThemisDB.
    """
    
    def __init__(self, num_warehouses: int = 1):
        self.num_warehouses = num_warehouses
        
    def get_table_definitions(self) -> Dict[str, str]:
        """
        Get SQL-like DDL statements for all TPC-C tables.
        
        Returns:
            Dictionary mapping table names to DDL statements
        """
        return {
            "WAREHOUSE": self._get_warehouse_ddl(),
            "DISTRICT": self._get_district_ddl(),
            "CUSTOMER": self._get_customer_ddl(),
            "HISTORY": self._get_history_ddl(),
            "NEW_ORDER": self._get_new_order_ddl(),
            "ORDERS": self._get_orders_ddl(),
            "ORDER_LINE": self._get_order_line_ddl(),
            "ITEM": self._get_item_ddl(),
            "STOCK": self._get_stock_ddl(),
        }
    
    def _get_warehouse_ddl(self) -> str:
        return """
        CREATE TABLE WAREHOUSE (
            W_ID INTEGER PRIMARY KEY,
            W_NAME VARCHAR(10),
            W_STREET_1 VARCHAR(20),
            W_STREET_2 VARCHAR(20),
            W_CITY VARCHAR(20),
            W_STATE CHAR(2),
            W_ZIP CHAR(9),
            W_TAX DECIMAL(4,4),
            W_YTD DECIMAL(12,2)
        );
        """
    
    def _get_district_ddl(self) -> str:
        return """
        CREATE TABLE DISTRICT (
            D_ID INTEGER,
            D_W_ID INTEGER,
            D_NAME VARCHAR(10),
            D_STREET_1 VARCHAR(20),
            D_STREET_2 VARCHAR(20),
            D_CITY VARCHAR(20),
            D_STATE CHAR(2),
            D_ZIP CHAR(9),
            D_TAX DECIMAL(4,4),
            D_YTD DECIMAL(12,2),
            D_NEXT_O_ID INTEGER,
            PRIMARY KEY (D_W_ID, D_ID),
            FOREIGN KEY (D_W_ID) REFERENCES WAREHOUSE(W_ID)
        );
        CREATE INDEX IDX_DISTRICT_W_ID ON DISTRICT(D_W_ID);
        """
    
    def _get_customer_ddl(self) -> str:
        return """
        CREATE TABLE CUSTOMER (
            C_ID INTEGER,
            C_D_ID INTEGER,
            C_W_ID INTEGER,
            C_FIRST VARCHAR(16),
            C_MIDDLE CHAR(2),
            C_LAST VARCHAR(16),
            C_STREET_1 VARCHAR(20),
            C_STREET_2 VARCHAR(20),
            C_CITY VARCHAR(20),
            C_STATE CHAR(2),
            C_ZIP CHAR(9),
            C_PHONE CHAR(16),
            C_SINCE TIMESTAMP,
            C_CREDIT CHAR(2),
            C_CREDIT_LIM DECIMAL(12,2),
            C_DISCOUNT DECIMAL(4,4),
            C_BALANCE DECIMAL(12,2),
            C_YTD_PAYMENT DECIMAL(12,2),
            C_PAYMENT_CNT INTEGER,
            C_DELIVERY_CNT INTEGER,
            C_DATA VARCHAR(500),
            PRIMARY KEY (C_W_ID, C_D_ID, C_ID),
            FOREIGN KEY (C_W_ID, C_D_ID) REFERENCES DISTRICT(D_W_ID, D_ID)
        );
        CREATE INDEX IDX_CUSTOMER_W_ID ON CUSTOMER(C_W_ID, C_D_ID);
        CREATE INDEX IDX_CUSTOMER_LAST ON CUSTOMER(C_W_ID, C_D_ID, C_LAST);
        """
    
    def _get_history_ddl(self) -> str:
        return """
        CREATE TABLE HISTORY (
            H_C_ID INTEGER,
            H_C_D_ID INTEGER,
            H_C_W_ID INTEGER,
            H_D_ID INTEGER,
            H_W_ID INTEGER,
            H_DATE TIMESTAMP,
            H_AMOUNT DECIMAL(6,2),
            H_DATA VARCHAR(24),
            FOREIGN KEY (H_C_W_ID, H_C_D_ID, H_C_ID) REFERENCES CUSTOMER(C_W_ID, C_D_ID, C_ID),
            FOREIGN KEY (H_W_ID, H_D_ID) REFERENCES DISTRICT(D_W_ID, D_ID)
        );
        CREATE INDEX IDX_HISTORY_C_ID ON HISTORY(H_C_W_ID, H_C_D_ID, H_C_ID);
        CREATE INDEX IDX_HISTORY_W_ID ON HISTORY(H_W_ID, H_D_ID);
        """
    
    def _get_new_order_ddl(self) -> str:
        return """
        CREATE TABLE NEW_ORDER (
            NO_O_ID INTEGER,
            NO_D_ID INTEGER,
            NO_W_ID INTEGER,
            PRIMARY KEY (NO_W_ID, NO_D_ID, NO_O_ID),
            FOREIGN KEY (NO_W_ID, NO_D_ID, NO_O_ID) REFERENCES ORDERS(O_W_ID, O_D_ID, O_ID)
        );
        CREATE INDEX IDX_NEW_ORDER_W_ID ON NEW_ORDER(NO_W_ID, NO_D_ID);
        """
    
    def _get_orders_ddl(self) -> str:
        return """
        CREATE TABLE ORDERS (
            O_ID INTEGER,
            O_D_ID INTEGER,
            O_W_ID INTEGER,
            O_C_ID INTEGER,
            O_ENTRY_D TIMESTAMP,
            O_CARRIER_ID INTEGER,
            O_OL_CNT INTEGER,
            O_ALL_LOCAL INTEGER,
            PRIMARY KEY (O_W_ID, O_D_ID, O_ID),
            FOREIGN KEY (O_W_ID, O_D_ID, O_C_ID) REFERENCES CUSTOMER(C_W_ID, C_D_ID, C_ID)
        );
        CREATE INDEX IDX_ORDERS_C_ID ON ORDERS(O_W_ID, O_D_ID, O_C_ID);
        """
    
    def _get_order_line_ddl(self) -> str:
        return """
        CREATE TABLE ORDER_LINE (
            OL_O_ID INTEGER,
            OL_D_ID INTEGER,
            OL_W_ID INTEGER,
            OL_NUMBER INTEGER,
            OL_I_ID INTEGER,
            OL_SUPPLY_W_ID INTEGER,
            OL_DELIVERY_D TIMESTAMP,
            OL_QUANTITY INTEGER,
            OL_AMOUNT DECIMAL(6,2),
            OL_DIST_INFO CHAR(24),
            PRIMARY KEY (OL_W_ID, OL_D_ID, OL_O_ID, OL_NUMBER),
            FOREIGN KEY (OL_W_ID, OL_D_ID, OL_O_ID) REFERENCES ORDERS(O_W_ID, O_D_ID, O_ID),
            FOREIGN KEY (OL_SUPPLY_W_ID, OL_I_ID) REFERENCES STOCK(S_W_ID, S_I_ID)
        );
        CREATE INDEX IDX_ORDER_LINE_O_ID ON ORDER_LINE(OL_W_ID, OL_D_ID, OL_O_ID);
        """
    
    def _get_item_ddl(self) -> str:
        return """
        CREATE TABLE ITEM (
            I_ID INTEGER PRIMARY KEY,
            I_IM_ID INTEGER,
            I_NAME VARCHAR(24),
            I_PRICE DECIMAL(5,2),
            I_DATA VARCHAR(50)
        );
        """
    
    def _get_stock_ddl(self) -> str:
        return """
        CREATE TABLE STOCK (
            S_I_ID INTEGER,
            S_W_ID INTEGER,
            S_QUANTITY INTEGER,
            S_DIST_01 CHAR(24),
            S_DIST_02 CHAR(24),
            S_DIST_03 CHAR(24),
            S_DIST_04 CHAR(24),
            S_DIST_05 CHAR(24),
            S_DIST_06 CHAR(24),
            S_DIST_07 CHAR(24),
            S_DIST_08 CHAR(24),
            S_DIST_09 CHAR(24),
            S_DIST_10 CHAR(24),
            S_YTD INTEGER,
            S_ORDER_CNT INTEGER,
            S_REMOTE_CNT INTEGER,
            S_DATA VARCHAR(50),
            PRIMARY KEY (S_W_ID, S_I_ID),
            FOREIGN KEY (S_W_ID) REFERENCES WAREHOUSE(W_ID),
            FOREIGN KEY (S_I_ID) REFERENCES ITEM(I_ID)
        );
        CREATE INDEX IDX_STOCK_W_ID ON STOCK(S_W_ID);
        """
    
    def get_row_counts(self) -> Dict[str, int]:
        """
        Calculate expected row counts for given number of warehouses.
        
        Returns:
            Dictionary mapping table names to expected row counts
        """
        W = self.num_warehouses
        return {
            "WAREHOUSE": W,
            "DISTRICT": 10 * W,
            "CUSTOMER": 30000 * W,
            "HISTORY": 30000 * W,  # Initially
            "NEW_ORDER": 9000 * W,  # Initially (30% of orders)
            "ORDERS": 30000 * W,  # Initially
            "ORDER_LINE": 300000 * W,  # Initially (~10 lines per order)
            "ITEM": 100000,  # Fixed size
            "STOCK": 100000 * W,
        }
    
    def get_total_size_estimate_mb(self) -> float:
        """
        Estimate total database size in MB.
        
        Based on TPC-C specification guidelines:
        - 1 warehouse ≈ 100 MB
        
        Returns:
            Estimated size in megabytes
        """
        return self.num_warehouses * 100.0


if __name__ == "__main__":
    # Example usage
    schema = TPCCSchema(num_warehouses=10)
    
    print("TPC-C Schema for 10 Warehouses")
    print("=" * 60)
    print("\nExpected Row Counts:")
    for table, count in schema.get_row_counts().items():
        print(f"  {table:15s}: {count:>12,d} rows")
    
    print(f"\nEstimated Database Size: {schema.get_total_size_estimate_mb():.1f} MB")
    
    print("\nDDL Statements:")
    for table, ddl in schema.get_table_definitions().items():
        print(f"\n{table}:")
        print(ddl.strip())
