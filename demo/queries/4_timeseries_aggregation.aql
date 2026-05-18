FOR o IN orders
  COLLECT week_start = DATE_TRUNC('week', o.order_date)
  AGGREGATE
    order_count     = COUNT(1),
    avg_order_value = ROUND(AVG(o.total_amount), 2),
    total_revenue   = SUM(o.total_amount)
  SORT week_start DESC
  RETURN { week_start, order_count, avg_order_value, total_revenue }