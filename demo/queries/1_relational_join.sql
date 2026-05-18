SELECT o.order_id, c.name, p.name, o.quantity, o.total_amount, o.status
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN products p ON o.product_id = p.product_id
WHERE c.country = 'DE'
ORDER BY o.order_date DESC;