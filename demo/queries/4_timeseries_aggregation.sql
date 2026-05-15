SELECT DATE_TRUNC('week', order_date) AS week_start,
       COUNT(*) AS order_count,
       ROUND(AVG(total_amount), 2) AS avg_order_value,
       SUM(total_amount) AS total_revenue
FROM orders
GROUP BY DATE_TRUNC('week', order_date)
ORDER BY week_start DESC;