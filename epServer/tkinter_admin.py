"""Tkinter Admin UI for ThemisDB Enterprise Pricing Server."""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import asyncio
import httpx
from datetime import datetime
from typing import Optional


class PricingServerAdmin:
    """Admin UI for managing customers, subscriptions, and payments."""
    
    def __init__(self, root: tk.Tk, api_url: str = "http://localhost:8000"):
        """Initialize the admin UI."""
        self.root = root
        self.api_url = api_url
        self.token: Optional[str] = None
        self.client = httpx.AsyncClient(base_url=api_url)
        
        # Setup UI
        self.root.title("ThemisDB Enterprise Pricing Server - Admin")
        self.root.geometry("1000x700")
        
        self.create_widgets()
    
    def create_widgets(self):
        """Create UI widgets."""
        # Create notebook for tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Login tab
        self.login_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.login_frame, text="Login")
        self.create_login_tab()
        
        # Customers tab
        self.customers_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.customers_frame, text="Customers", state="disabled")
        self.create_customers_tab()
        
        # Subscriptions tab
        self.subscriptions_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.subscriptions_frame, text="Subscriptions", state="disabled")
        self.create_subscriptions_tab()
        
        # Payments tab
        self.payments_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.payments_frame, text="Payments", state="disabled")
        self.create_payments_tab()
        
        # Pricing tab
        self.pricing_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.pricing_frame, text="Pricing Tiers")
        self.create_pricing_tab()
        
        # Status bar
        self.status_bar = ttk.Label(self.root, text="Not logged in", relief=tk.SUNKEN)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def create_login_tab(self):
        """Create login tab."""
        frame = ttk.Frame(self.login_frame, padding=20)
        frame.pack(expand=True)
        
        ttk.Label(frame, text="ThemisDB Pricing Server", font=("Arial", 16, "bold")).grid(row=0, column=0, columnspan=2, pady=20)
        
        # Email
        ttk.Label(frame, text="Email:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.email_entry = ttk.Entry(frame, width=30)
        self.email_entry.grid(row=1, column=1, pady=5)
        
        # Password
        ttk.Label(frame, text="Password:").grid(row=2, column=0, sticky=tk.W, pady=5)
        self.password_entry = ttk.Entry(frame, width=30, show="*")
        self.password_entry.grid(row=2, column=1, pady=5)
        
        # Login button
        ttk.Button(frame, text="Login", command=self.login).grid(row=3, column=0, columnspan=2, pady=20)
        
        # Register section
        ttk.Separator(frame, orient=tk.HORIZONTAL).grid(row=4, column=0, columnspan=2, sticky="ew", pady=10)
        ttk.Label(frame, text="New Customer Registration", font=("Arial", 12, "bold")).grid(row=5, column=0, columnspan=2, pady=10)
        
        # Organization
        ttk.Label(frame, text="Organization:").grid(row=6, column=0, sticky=tk.W, pady=5)
        self.org_entry = ttk.Entry(frame, width=30)
        self.org_entry.grid(row=6, column=1, pady=5)
        
        # Contact Name
        ttk.Label(frame, text="Contact Name:").grid(row=7, column=0, sticky=tk.W, pady=5)
        self.contact_entry = ttk.Entry(frame, width=30)
        self.contact_entry.grid(row=7, column=1, pady=5)
        
        # Register button
        ttk.Button(frame, text="Register", command=self.register).grid(row=8, column=0, columnspan=2, pady=10)
    
    def create_customers_tab(self):
        """Create customers management tab."""
        # Info display
        info_frame = ttk.LabelFrame(self.customers_frame, text="Customer Information", padding=10)
        info_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.customer_text = scrolledtext.ScrolledText(info_frame, height=20)
        self.customer_text.pack(fill=tk.BOTH, expand=True)
        
        # Buttons
        button_frame = ttk.Frame(self.customers_frame)
        button_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(button_frame, text="Load My Profile", command=self.load_customer_profile).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Refresh", command=self.load_customer_profile).pack(side=tk.LEFT, padx=5)
    
    def create_subscriptions_tab(self):
        """Create subscriptions management tab."""
        # Subscription list
        list_frame = ttk.LabelFrame(self.subscriptions_frame, text="My Subscriptions", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Treeview for subscriptions
        columns = ("ID", "Tier", "Status", "License Key", "Created")
        self.subscription_tree = ttk.Treeview(list_frame, columns=columns, show="headings", height=10)
        
        for col in columns:
            self.subscription_tree.heading(col, text=col)
            self.subscription_tree.column(col, width=150)
        
        self.subscription_tree.pack(fill=tk.BOTH, expand=True)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.subscription_tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.subscription_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        button_frame = ttk.Frame(self.subscriptions_frame)
        button_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(button_frame, text="Create Subscription", command=self.create_subscription_dialog).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Refresh", command=self.load_subscriptions).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Cancel Selected", command=self.cancel_selected_subscription).pack(side=tk.LEFT, padx=5)
    
    def create_payments_tab(self):
        """Create payments management tab."""
        # Payment list
        list_frame = ttk.LabelFrame(self.payments_frame, text="My Payments", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Treeview for payments
        columns = ("ID", "Amount", "Status", "Transaction ID", "Created")
        self.payment_tree = ttk.Treeview(list_frame, columns=columns, show="headings", height=10)
        
        for col in columns:
            self.payment_tree.heading(col, text=col)
            self.payment_tree.column(col, width=150)
        
        self.payment_tree.pack(fill=tk.BOTH, expand=True)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.payment_tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.payment_tree.configure(yscrollcommand=scrollbar.set)
        
        # Buttons
        button_frame = ttk.Frame(self.payments_frame)
        button_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(button_frame, text="Create Payment", command=self.create_payment_dialog).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Refresh", command=self.load_payments).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Verify Selected", command=self.verify_selected_payment).pack(side=tk.LEFT, padx=5)
    
    def create_pricing_tab(self):
        """Create pricing tiers display tab."""
        info_frame = ttk.LabelFrame(self.pricing_frame, text="Available Pricing Tiers", padding=10)
        info_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.pricing_text = scrolledtext.ScrolledText(info_frame, height=25)
        self.pricing_text.pack(fill=tk.BOTH, expand=True)
        
        # Load pricing button
        button_frame = ttk.Frame(self.pricing_frame)
        button_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(button_frame, text="Load Pricing", command=self.load_pricing).pack(side=tk.LEFT, padx=5)
        
        # Auto-load pricing
        self.load_pricing()
    
    # Async wrappers
    def login(self):
        """Login to the pricing server."""
        asyncio.run(self._login())
    
    async def _login(self):
        """Async login implementation."""
        email = self.email_entry.get()
        password = self.password_entry.get()
        
        if not email or not password:
            messagebox.showerror("Error", "Please enter email and password")
            return
        
        try:
            response = await self.client.post(
                "/auth/login-json",
                json={"email": email, "password": password}
            )
            
            if response.status_code == 200:
                data = response.json()
                self.token = data["access_token"]
                self.client.headers["Authorization"] = f"Bearer {self.token}"
                
                # Enable tabs
                self.notebook.tab(1, state="normal")  # Customers
                self.notebook.tab(2, state="normal")  # Subscriptions
                self.notebook.tab(3, state="normal")  # Payments
                
                self.status_bar.config(text=f"Logged in as {email}")
                messagebox.showinfo("Success", "Login successful!")
                
                # Switch to customers tab
                self.notebook.select(1)
            else:
                messagebox.showerror("Error", f"Login failed: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def register(self):
        """Register a new customer."""
        asyncio.run(self._register())
    
    async def _register(self):
        """Async register implementation."""
        email = self.email_entry.get()
        password = self.password_entry.get()
        org = self.org_entry.get()
        contact = self.contact_entry.get()
        
        if not all([email, password, org, contact]):
            messagebox.showerror("Error", "Please fill all registration fields")
            return
        
        try:
            response = await self.client.post(
                "/auth/register",
                json={
                    "email": email,
                    "password": password,
                    "organization_name": org,
                    "contact_name": contact
                }
            )
            
            if response.status_code == 201:
                messagebox.showinfo("Success", "Registration successful! You can now login.")
            else:
                messagebox.showerror("Error", f"Registration failed: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def load_customer_profile(self):
        """Load customer profile."""
        asyncio.run(self._load_customer_profile())
    
    async def _load_customer_profile(self):
        """Async load customer profile."""
        try:
            response = await self.client.get("/auth/me")
            
            if response.status_code == 200:
                data = response.json()
                
                self.customer_text.delete(1.0, tk.END)
                self.customer_text.insert(tk.END, "=== Customer Profile ===\n\n")
                self.customer_text.insert(tk.END, f"ID: {data['id']}\n")
                self.customer_text.insert(tk.END, f"Email: {data['email']}\n")
                self.customer_text.insert(tk.END, f"Organization: {data['organization_name']}\n")
                self.customer_text.insert(tk.END, f"Contact: {data['contact_name']}\n")
                self.customer_text.insert(tk.END, f"Phone: {data.get('phone', 'N/A')}\n")
                self.customer_text.insert(tk.END, f"Country: {data.get('country', 'N/A')}\n")
                self.customer_text.insert(tk.END, f"Status: {'Active' if data['is_active'] else 'Inactive'}\n")
                self.customer_text.insert(tk.END, f"Created: {data['created_at']}\n")
            else:
                messagebox.showerror("Error", f"Failed to load profile: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def load_subscriptions(self):
        """Load subscriptions."""
        asyncio.run(self._load_subscriptions())
    
    async def _load_subscriptions(self):
        """Async load subscriptions."""
        try:
            response = await self.client.get("/subscriptions")
            
            if response.status_code == 200:
                subscriptions = response.json()
                
                # Clear tree
                for item in self.subscription_tree.get_children():
                    self.subscription_tree.delete(item)
                
                # Add subscriptions
                for sub in subscriptions:
                    self.subscription_tree.insert("", tk.END, values=(
                        sub['id'],
                        sub['tier'],
                        sub['status'],
                        sub['license_key'][:20] + "...",
                        sub['created_at'][:10]
                    ))
            else:
                messagebox.showerror("Error", f"Failed to load subscriptions: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def load_payments(self):
        """Load payments."""
        asyncio.run(self._load_payments())
    
    async def _load_payments(self):
        """Async load payments."""
        try:
            response = await self.client.get("/payments")
            
            if response.status_code == 200:
                payments = response.json()
                
                # Clear tree
                for item in self.payment_tree.get_children():
                    self.payment_tree.delete(item)
                
                # Add payments
                for payment in payments:
                    self.payment_tree.insert("", tk.END, values=(
                        payment['id'],
                        f"{payment['amount']} {payment['currency']}",
                        payment['status'],
                        payment.get('transaction_id', 'N/A')[:20],
                        payment['created_at'][:10]
                    ))
            else:
                messagebox.showerror("Error", f"Failed to load payments: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def load_pricing(self):
        """Load pricing tiers."""
        asyncio.run(self._load_pricing())
    
    async def _load_pricing(self):
        """Async load pricing."""
        try:
            response = await self.client.get("/pricing")
            
            if response.status_code == 200:
                data = response.json()
                
                self.pricing_text.delete(1.0, tk.END)
                self.pricing_text.insert(tk.END, "=== ThemisDB Pricing Tiers ===\n\n")
                
                for tier in data['tiers']:
                    self.pricing_text.insert(tk.END, f"--- {tier['name']} ---\n")
                    self.pricing_text.insert(tk.END, f"Price: €{tier['price_per_month']}/month\n")
                    self.pricing_text.insert(tk.END, f"Features:\n")
                    for feature in tier['features']:
                        self.pricing_text.insert(tk.END, f"  • {feature}\n")
                    self.pricing_text.insert(tk.END, f"\n")
            else:
                messagebox.showerror("Error", f"Failed to load pricing: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def create_subscription_dialog(self):
        """Show dialog to create subscription."""
        dialog = tk.Toplevel(self.root)
        dialog.title("Create Subscription")
        dialog.geometry("400x300")
        
        ttk.Label(dialog, text="Select Tier:").pack(pady=10)
        tier_var = tk.StringVar(value="enterprise")
        ttk.Radiobutton(dialog, text="Enterprise (€5,000/month)", variable=tier_var, value="enterprise").pack()
        ttk.Radiobutton(dialog, text="Hyperscaler (€25,000/month)", variable=tier_var, value="hyperscaler").pack()
        ttk.Radiobutton(dialog, text="Reseller (€15,000/month)", variable=tier_var, value="reseller").pack()
        
        ttk.Label(dialog, text="Max Nodes:").pack(pady=10)
        nodes_entry = ttk.Entry(dialog)
        nodes_entry.insert(0, "10")
        nodes_entry.pack()
        
        def create():
            tier = tier_var.get()
            max_nodes = int(nodes_entry.get())
            asyncio.run(self._create_subscription(tier, max_nodes))
            dialog.destroy()
            self.load_subscriptions()
        
        ttk.Button(dialog, text="Create", command=create).pack(pady=20)
    
    async def _create_subscription(self, tier: str, max_nodes: int):
        """Async create subscription."""
        try:
            response = await self.client.post(
                "/subscriptions",
                json={
                    "tier": tier,
                    "max_nodes": max_nodes,
                    "billing_period_months": 12
                }
            )
            
            if response.status_code == 201:
                messagebox.showinfo("Success", "Subscription created successfully!")
            else:
                messagebox.showerror("Error", f"Failed to create subscription: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def create_payment_dialog(self):
        """Show dialog to create payment."""
        # First, need to select subscription
        messagebox.showinfo("Info", "Please select a subscription from the Subscriptions tab first, then create a payment for it.")
    
    def cancel_selected_subscription(self):
        """Cancel selected subscription."""
        selection = self.subscription_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a subscription")
            return
        
        item = self.subscription_tree.item(selection[0])
        sub_id = item['values'][0]
        
        if messagebox.askyesno("Confirm", f"Cancel subscription {sub_id}?"):
            asyncio.run(self._cancel_subscription(sub_id))
            self.load_subscriptions()
    
    async def _cancel_subscription(self, sub_id: int):
        """Async cancel subscription."""
        try:
            response = await self.client.post(f"/subscriptions/{sub_id}/cancel")
            
            if response.status_code == 200:
                messagebox.showinfo("Success", "Subscription cancelled")
            else:
                messagebox.showerror("Error", f"Failed to cancel: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
    
    def verify_selected_payment(self):
        """Verify selected payment."""
        selection = self.payment_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a payment")
            return
        
        item = self.payment_tree.item(selection[0])
        payment_id = item['values'][0]
        
        asyncio.run(self._verify_payment(payment_id))
        self.load_payments()
    
    async def _verify_payment(self, payment_id: int):
        """Async verify payment."""
        try:
            response = await self.client.post(f"/payments/{payment_id}/verify")
            
            if response.status_code == 200:
                data = response.json()
                messagebox.showinfo("Success", f"Payment verified! Status: {data['status']}")
            else:
                messagebox.showerror("Error", f"Failed to verify: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")


def main():
    """Main entry point."""
    root = tk.Tk()
    app = PricingServerAdmin(root)
    root.mainloop()


if __name__ == "__main__":
    main()
