SELECT product_id, name, description, price, similarity_score
FROM products_vector_search(
  query='renewable energy battery storage',
  top_k=5,
  min_similarity=0.6
)
ORDER BY similarity_score DESC;