FOR o IN orders
  FOR c IN customers
    FILTER c._key == o.customer_id
    FILTER c.country == 'DE'
    FOR p IN products
      FILTER p._key == o.product_id
      SORT o.order_date DESC
      RETURN {
        order_id:     o.order_id,
        customer:     c.name,
        product:      p.name,
        quantity:     o.quantity,
        total_amount: o.total_amount,
        status:       o.status
      }