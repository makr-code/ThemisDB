#!/usr/bin/env python3
"""
TPC-C Data Generator for ThemisDB

Generates realistic test data according to TPC-C Specification 5.11.
Includes all required data distributions and constraints.

Usage:
    python3 tpc_c_data_generator.py --warehouses 10 --output /tmp/tpc_c_data
"""

import random
import string
import argparse
import json
from datetime import datetime, timedelta
from decimal import Decimal
from pathlib import Path
from typing import List, Dict, Tuple
import sys


class TPCCDataGenerator:
    """
    TPC-C compliant data generator.
    
    Generates data for all 9 TPC-C tables with proper distributions
    and relationships according to the specification.
    """
    
    # TPC-C Specification Constants
    ITEMS_COUNT = 100000  # Fixed at 100,000 items
    DISTRICTS_PER_WAREHOUSE = 10
    CUSTOMERS_PER_DISTRICT = 3000
    ORDERS_PER_DISTRICT = 3000
    NEW_ORDERS_PER_DISTRICT = 900  # Last 900 orders are new orders
    
    # String generation constants
    SYLLABLES = [
        'BAR', 'OUGHT', 'ABLE', 'PRI', 'PRES', 'ESE', 'ANTI', 'CALLY', 'ATION', 'EING'
    ]
    
    def __init__(self, num_warehouses: int, random_seed: int = 42):
        """
        Initialize data generator.
        
        Args:
            num_warehouses: Number of warehouses to generate
            random_seed: Random seed for reproducibility
        """
        self.num_warehouses = num_warehouses
        random.seed(random_seed)
        
        # Pre-generate last names for customers (TPC-C requirement)
        self.last_names = self._generate_last_names()
        
    def _generate_last_names(self) -> List[str]:
        """Generate 1000 unique last names using syllables."""
        names = []
        for i in range(1000):
            # TPC-C 4.3.2.3: Generate name from 3 syllables based on number
            n1 = (i // 100) % 10
            n2 = (i // 10) % 10
            n3 = i % 10
            name = self.SYLLABLES[n1] + self.SYLLABLES[n2] + self.SYLLABLES[n3]
            names.append(name)
        return names
    
    def _random_string(self, min_len: int, max_len: int) -> str:
        """Generate random alphanumeric string."""
        length = random.randint(min_len, max_len)
        return ''.join(random.choices(string.ascii_letters + string.digits, k=length))
    
    def _random_number_string(self, length: int) -> str:
        """Generate random numeric string."""
        return ''.join(random.choices(string.digits, k=length))
    
    def _random_zip(self) -> str:
        """Generate random ZIP code (format: ####11111)."""
        return self._random_number_string(4) + "11111"
    
    def _random_phone(self) -> str:
        """Generate random phone number."""
        return self._random_number_string(16)
    
    def _nu_rand(self, a: int, x: int, y: int) -> int:
        """
        TPC-C NURand function for non-uniform random distribution.
        
        Args:
            a: Constant value
            x: Minimum value
            y: Maximum value
        """
        c_load = 0  # For data generation
        return (((random.randint(0, a) | random.randint(x, y)) + c_load) % (y - x + 1)) + x
    
    def _generate_original_string(self) -> str:
        """
        Generate string with 'ORIGINAL' embedded (10% probability).
        TPC-C requirement for Item and Stock tables.
        """
        data = self._random_string(26, 50)
        if random.random() < 0.1:
            # Insert 'ORIGINAL' at random position
            pos = random.randint(0, len(data) - 8)
            data = data[:pos] + 'ORIGINAL' + data[pos + 8:]
        return data
    
    def generate_warehouses(self) -> List[Dict]:
        """Generate WAREHOUSE table data."""
        warehouses = []
        for w_id in range(1, self.num_warehouses + 1):
            warehouse = {
                'W_ID': w_id,
                'W_NAME': self._random_string(6, 10),
                'W_STREET_1': self._random_string(10, 20),
                'W_STREET_2': self._random_string(10, 20),
                'W_CITY': self._random_string(10, 20),
                'W_STATE': ''.join(random.choices(string.ascii_uppercase, k=2)),
                'W_ZIP': self._random_zip(),
                'W_TAX': Decimal(random.randint(0, 2000)) / Decimal(10000),  # 0.0000 to 0.2000
                'W_YTD': Decimal('300000.00')  # Initial value
            }
            warehouses.append(warehouse)
        return warehouses
    
    def generate_districts(self) -> List[Dict]:
        """Generate DISTRICT table data."""
        districts = []
        for w_id in range(1, self.num_warehouses + 1):
            for d_id in range(1, self.DISTRICTS_PER_WAREHOUSE + 1):
                district = {
                    'D_ID': d_id,
                    'D_W_ID': w_id,
                    'D_NAME': self._random_string(6, 10),
                    'D_STREET_1': self._random_string(10, 20),
                    'D_STREET_2': self._random_string(10, 20),
                    'D_CITY': self._random_string(10, 20),
                    'D_STATE': ''.join(random.choices(string.ascii_uppercase, k=2)),
                    'D_ZIP': self._random_zip(),
                    'D_TAX': Decimal(random.randint(0, 2000)) / Decimal(10000),
                    'D_YTD': Decimal('30000.00'),  # Initial value
                    'D_NEXT_O_ID': 3001  # Next order ID
                }
                districts.append(district)
        return districts
    
    def generate_customers(self) -> List[Dict]:
        """Generate CUSTOMER table data."""
        customers = []
        for w_id in range(1, self.num_warehouses + 1):
            for d_id in range(1, self.DISTRICTS_PER_WAREHOUSE + 1):
                for c_id in range(1, self.CUSTOMERS_PER_DISTRICT + 1):
                    # TPC-C 4.3.2.3: Last name is non-uniform random
                    if c_id <= 1000:
                        c_last = self.last_names[c_id - 1]
                    else:
                        c_last = self.last_names[self._nu_rand(255, 0, 999)]
                    
                    # 10% bad credit
                    c_credit = 'BC' if random.random() < 0.1 else 'GC'
                    
                    customer = {
                        'C_ID': c_id,
                        'C_D_ID': d_id,
                        'C_W_ID': w_id,
                        'C_FIRST': self._random_string(8, 16),
                        'C_MIDDLE': 'OE',
                        'C_LAST': c_last,
                        'C_STREET_1': self._random_string(10, 20),
                        'C_STREET_2': self._random_string(10, 20),
                        'C_CITY': self._random_string(10, 20),
                        'C_STATE': ''.join(random.choices(string.ascii_uppercase, k=2)),
                        'C_ZIP': self._random_zip(),
                        'C_PHONE': self._random_phone(),
                        'C_SINCE': datetime.now().isoformat(),
                        'C_CREDIT': c_credit,
                        'C_CREDIT_LIM': Decimal('50000.00'),
                        'C_DISCOUNT': Decimal(random.randint(0, 5000)) / Decimal(10000),
                        'C_BALANCE': Decimal('-10.00'),
                        'C_YTD_PAYMENT': Decimal('10.00'),
                        'C_PAYMENT_CNT': 1,
                        'C_DELIVERY_CNT': 0,
                        'C_DATA': self._random_string(300, 500)
                    }
                    customers.append(customer)
        return customers
    
    def generate_items(self) -> List[Dict]:
        """Generate ITEM table data (100,000 items)."""
        items = []
        for i_id in range(1, self.ITEMS_COUNT + 1):
            item = {
                'I_ID': i_id,
                'I_IM_ID': random.randint(1, 10000),
                'I_NAME': self._random_string(14, 24),
                'I_PRICE': Decimal(random.randint(100, 10000)) / Decimal(100),  # $1.00 to $100.00
                'I_DATA': self._generate_original_string()
            }
            items.append(item)
        return items
    
    def generate_stock(self) -> List[Dict]:
        """Generate STOCK table data."""
        stock = []
        for w_id in range(1, self.num_warehouses + 1):
            for i_id in range(1, self.ITEMS_COUNT + 1):
                stock_item = {
                    'S_I_ID': i_id,
                    'S_W_ID': w_id,
                    'S_QUANTITY': random.randint(10, 100),
                    'S_DIST_01': self._random_string(24, 24),
                    'S_DIST_02': self._random_string(24, 24),
                    'S_DIST_03': self._random_string(24, 24),
                    'S_DIST_04': self._random_string(24, 24),
                    'S_DIST_05': self._random_string(24, 24),
                    'S_DIST_06': self._random_string(24, 24),
                    'S_DIST_07': self._random_string(24, 24),
                    'S_DIST_08': self._random_string(24, 24),
                    'S_DIST_09': self._random_string(24, 24),
                    'S_DIST_10': self._random_string(24, 24),
                    'S_YTD': 0,
                    'S_ORDER_CNT': 0,
                    'S_REMOTE_CNT': 0,
                    'S_DATA': self._generate_original_string()
                }
                stock.append(stock_item)
        return stock
    
    def generate_orders_and_related(self) -> Tuple[List[Dict], List[Dict], List[Dict], List[Dict]]:
        """
        Generate ORDERS, NEW_ORDER, ORDER_LINE, and HISTORY tables.
        These must be generated together to maintain referential integrity.
        
        Returns:
            Tuple of (orders, new_orders, order_lines, history)
        """
        orders = []
        new_orders = []
        order_lines = []
        history = []
        
        for w_id in range(1, self.num_warehouses + 1):
            for d_id in range(1, self.DISTRICTS_PER_WAREHOUSE + 1):
                # Generate 3000 orders per district
                for o_id in range(1, self.ORDERS_PER_DISTRICT + 1):
                    # Customer ID follows non-uniform distribution
                    c_id = self._nu_rand(1023, 1, self.CUSTOMERS_PER_DISTRICT)
                    
                    # Entry date (random within last year)
                    days_ago = random.randint(0, 365)
                    entry_date = (datetime.now() - timedelta(days=days_ago)).isoformat()
                    
                    # Last 900 orders have no carrier (are new orders)
                    is_new_order = (o_id > self.ORDERS_PER_DISTRICT - self.NEW_ORDERS_PER_DISTRICT)
                    carrier_id = None if is_new_order else random.randint(1, 10)
                    
                    # Number of order lines (5-15)
                    ol_cnt = random.randint(5, 15)
                    
                    # All local (1) or some remote (0) - 1% remote
                    all_local = 0 if random.random() < 0.01 else 1
                    
                    order = {
                        'O_ID': o_id,
                        'O_D_ID': d_id,
                        'O_W_ID': w_id,
                        'O_C_ID': c_id,
                        'O_ENTRY_D': entry_date,
                        'O_CARRIER_ID': carrier_id,
                        'O_OL_CNT': ol_cnt,
                        'O_ALL_LOCAL': all_local
                    }
                    orders.append(order)
                    
                    # Add to NEW_ORDER if applicable
                    if is_new_order:
                        new_order = {
                            'NO_O_ID': o_id,
                            'NO_D_ID': d_id,
                            'NO_W_ID': w_id
                        }
                        new_orders.append(new_order)
                    
                    # Generate order lines
                    for ol_number in range(1, ol_cnt + 1):
                        # Item ID follows non-uniform distribution
                        i_id = self._nu_rand(8191, 1, self.ITEMS_COUNT)
                        
                        # Supply warehouse (1% remote)
                        if all_local == 0 and random.random() < 0.01 and self.num_warehouses > 1:
                            supply_w_id = random.choice([w for w in range(1, self.num_warehouses + 1) if w != w_id])
                        else:
                            supply_w_id = w_id
                        
                        # Delivery date (null for new orders)
                        delivery_date = None if is_new_order else entry_date
                        
                        # Quantity and amount
                        quantity = 5
                        amount = Decimal(random.randint(10, 10000)) / Decimal(100) if not is_new_order else Decimal('0.00')
                        
                        order_line = {
                            'OL_O_ID': o_id,
                            'OL_D_ID': d_id,
                            'OL_W_ID': w_id,
                            'OL_NUMBER': ol_number,
                            'OL_I_ID': i_id,
                            'OL_SUPPLY_W_ID': supply_w_id,
                            'OL_DELIVERY_D': delivery_date,
                            'OL_QUANTITY': quantity,
                            'OL_AMOUNT': str(amount),
                            'OL_DIST_INFO': self._random_string(24, 24)
                        }
                        order_lines.append(order_line)
                    
                    # Generate history record for this customer
                    history_record = {
                        'H_C_ID': c_id,
                        'H_C_D_ID': d_id,
                        'H_C_W_ID': w_id,
                        'H_D_ID': d_id,
                        'H_W_ID': w_id,
                        'H_DATE': entry_date,
                        'H_AMOUNT': Decimal('10.00'),
                        'H_DATA': self._random_string(12, 24)
                    }
                    history.append(history_record)
        
        return orders, new_orders, order_lines, history
    
    def generate_all(self, output_dir: str, output_format: str = 'json'):
        """
        Generate all TPC-C data and save to files.
        
        Args:
            output_dir: Directory to save data files
            output_format: Output format ('json' or 'csv')
        """
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        print(f"Generating TPC-C data for {self.num_warehouses} warehouse(s)...")
        print("=" * 60)
        
        # Generate all tables
        print("Generating WAREHOUSE table...")
        warehouses = self.generate_warehouses()
        self._save_data(warehouses, output_path / f'warehouse.{output_format}', output_format)
        
        print("Generating DISTRICT table...")
        districts = self.generate_districts()
        self._save_data(districts, output_path / f'district.{output_format}', output_format)
        
        print("Generating CUSTOMER table...")
        customers = self.generate_customers()
        self._save_data(customers, output_path / f'customer.{output_format}', output_format)
        
        print("Generating ITEM table...")
        items = self.generate_items()
        self._save_data(items, output_path / f'item.{output_format}', output_format)
        
        print("Generating STOCK table...")
        stock = self.generate_stock()
        self._save_data(stock, output_path / f'stock.{output_format}', output_format)
        
        print("Generating ORDERS, NEW_ORDER, ORDER_LINE, and HISTORY tables...")
        orders, new_orders, order_lines, history = self.generate_orders_and_related()
        self._save_data(orders, output_path / f'orders.{output_format}', output_format)
        self._save_data(new_orders, output_path / f'new_order.{output_format}', output_format)
        self._save_data(order_lines, output_path / f'order_line.{output_format}', output_format)
        self._save_data(history, output_path / f'history.{output_format}', output_format)
        
        print("\n" + "=" * 60)
        print("Data generation complete!")
        self._print_summary()
    
    def _save_data(self, data: List[Dict], filepath: Path, output_format: str):
        """Save data to file in specified format."""
        if output_format == 'json':
            with open(filepath, 'w') as f:
                # Convert Decimal to string for JSON serialization
                data_json = []
                for row in data:
                    row_json = {}
                    for key, value in row.items():
                        if isinstance(value, Decimal):
                            row_json[key] = str(value)
                        else:
                            row_json[key] = value
                    data_json.append(row_json)
                json.dump(data_json, f, indent=2)
        elif output_format == 'csv':
            import csv
            if data:
                with open(filepath, 'w', newline='') as f:
                    writer = csv.DictWriter(f, fieldnames=data[0].keys())
                    writer.writeheader()
                    writer.writerows(data)
        
        print(f"  Saved {len(data):,} rows to {filepath.name}")
    
    def _print_summary(self):
        """Print summary of generated data."""
        W = self.num_warehouses
        print(f"\nData Summary for {W} warehouse(s):")
        print(f"  WAREHOUSE:   {W:>12,} rows")
        print(f"  DISTRICT:    {10 * W:>12,} rows")
        print(f"  CUSTOMER:    {30000 * W:>12,} rows")
        print(f"  HISTORY:     {30000 * W:>12,} rows")
        print(f"  ORDERS:      {30000 * W:>12,} rows")
        print(f"  NEW_ORDER:   {9000 * W:>12,} rows")
        print(f"  ORDER_LINE:  {~300000 * W:>12,} rows (approx)")
        print(f"  ITEM:        {self.ITEMS_COUNT:>12,} rows")
        print(f"  STOCK:       {self.ITEMS_COUNT * W:>12,} rows")
        print(f"\nEstimated total size: ~{W * 100} MB")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description='TPC-C Data Generator for ThemisDB')
    parser.add_argument('--warehouses', type=int, default=1,
                       help='Number of warehouses to generate (default: 1)')
    parser.add_argument('--output', type=str, required=True,
                       help='Output directory for data files')
    parser.add_argument('--format', choices=['json', 'csv'], default='json',
                       help='Output format (default: json)')
    parser.add_argument('--seed', type=int, default=42,
                       help='Random seed for reproducibility (default: 42)')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.warehouses < 1:
        print("Error: Number of warehouses must be at least 1", file=sys.stderr)
        sys.exit(1)
    
    # Create generator and generate data
    generator = TPCCDataGenerator(
        num_warehouses=args.warehouses,
        random_seed=args.seed
    )
    
    try:
        generator.generate_all(
            output_dir=args.output,
            output_format=args.format
        )
        print(f"\nSuccess! Data saved to: {args.output}")
    except Exception as e:
        print(f"Error generating data: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
