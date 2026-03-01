# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-08

### Added
- Initial release of ThemisDB Order Request & Contract Management Plugin
- Dialog-based order flow with 5 steps (Product → Modules → Training → Customer Data → Summary)
- Complete CRUD operations for orders and contracts
- Automatic contract revision tracking for legal compliance
- PDF generation for contracts and orders
- Email system with automated notifications and logging
- epServer API integration for master data synchronization
- Admin interface for managing orders, contracts, products, and email logs
- Frontend shortcodes: `[themisdb_order_flow]`, `[themisdb_my_orders]`, `[themisdb_my_contracts]`
- Responsive design for mobile and desktop
- Security features: CSRF protection, SQL injection prevention, XSS protection
- GDPR compliance features
- Multi-language support (German primary, extensible)
- Comprehensive documentation and README

### Features
- **Order Management**: Full order lifecycle from creation to completion
- **Contract Management**: Legal-compliant contract handling with revisions
- **PDF Generation**: Professional PDF templates for contracts and orders
- **Email System**: Automated emails with PDF attachments and comprehensive logging
- **epServer Integration**: Real-time product synchronization and customer management
- **Admin Dashboard**: Complete management interface with statistics and reports
- **Frontend Experience**: User-friendly dialog-based ordering process
- **Legal Compliance**: Full audit trail and revision history for all contracts

### Database Schema
- Orders table with customer and product information
- Contracts table with legal data and PDF storage
- Contract revisions table for change tracking
- Products, modules, and training master data tables
- Email log table for delivery tracking

### Security
- WordPress nonce verification for all AJAX requests
- SQL prepared statements to prevent injection attacks
- Input sanitization and output escaping
- User capability checks for admin functions
- Secure PDF storage options (database or filesystem)

## [Unreleased]

### Planned Features
- Multi-currency support
- Automated invoice generation
- Payment gateway integration (Stripe, PayPal)
- Electronic signature support
- Multi-language support (English, Spanish, French)
- Advanced reporting and analytics
- Workflow automation rules
- Mobile app integration
- REST API for third-party integrations
- Bulk operations for orders and contracts
- Export functionality (CSV, Excel)
- Custom contract templates
- Advanced email templates with drag-and-drop editor
