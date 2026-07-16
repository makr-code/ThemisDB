FOR doc IN products
  FILTER SIMILARITY(doc.embedding, 'renewable energy battery storage', 5)
  FILTER doc._similarity >= 0.6
  SORT doc._similarity DESC
  RETURN {
    product_id:       doc.product_id,
    name:             doc.name,
    description:      doc.description,
    price:            doc.price,
    similarity_score: doc._similarity
  }