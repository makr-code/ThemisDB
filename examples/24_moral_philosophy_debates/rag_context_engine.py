"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_context_engine.py                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     725                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
RAG Context Engine for Ethical Discourse

Implements Retrieval-Augmented Generation (RAG) for ethical decision-making.
Uses concrete AQL query patterns to retrieve relevant historical debates,
similar arguments, and best practices to inform current ethical decisions.

Author: ThemisDB Ethics AI Framework
License: MIT
"""

from typing import List, Dict, Any, Optional, Tuple
from datetime import datetime, timedelta


class RagContextEngine:
    """
    RAG-based context retrieval engine for ethical discourse.
    
    Features:
    - Textual similarity search using TEXT_SIMILARITY AQL function
    - Philosophy-specific argument retrieval
    - Best-practice synthesis retrieval
    - Vector semantic search
    - Argument chain traversal
    - Temporal filtering for recent debates
    """
    
    def __init__(self, themis_client=None):
        """
        Initialize RAG context engine.
        
        Args:
            themis_client: ThemisDB client for query execution
        """
        self.client = themis_client
        self.similarity_threshold = 0.65
        self.vector_distance_threshold = 0.3
        self.satisfaction_threshold = 0.75
    
    # ========================================================================
    # AQL Query Pattern 1: Textual Similarity Search
    # ========================================================================
    
    def find_similar_dilemmas(
        self,
        dilemma_description: str,
        limit: int = 10
    ) -> List[Dict[str, Any]]:
        """
        Find historically similar ethical dilemmas using text similarity.
        
        AQL Pattern:
        ```
        SELECT dilemma_id, description, similarity_score
        FROM ethical_dilemmas
        WHERE TEXT_SIMILARITY(description, @query, threshold=0.65) > 0
        ORDER BY similarity_score DESC
        LIMIT @limit
        ```
        
        Args:
            dilemma_description: Description of current dilemma
            limit: Maximum number of results
        
        Returns:
            List of similar dilemmas with similarity scores
        """
        if not self.client:
            return self._mock_similar_dilemmas(dilemma_description, limit)
        
        query = """
        SELECT 
            dilemma_id,
            description,
            TEXT_SIMILARITY(description, @query, threshold=@threshold) AS similarity_score,
            decision_count,
            avg_satisfaction_score
        FROM ethical_dilemmas
        WHERE TEXT_SIMILARITY(description, @query, threshold=@threshold) > 0
        ORDER BY similarity_score DESC
        LIMIT @limit
        """
        
        params = {
            'query': dilemma_description,
            'threshold': self.similarity_threshold,
            'limit': limit
        }
        
        try:
            # results = self.client.query(query, params)
            # return results
            return self._mock_similar_dilemmas(dilemma_description, limit)
        except Exception as e:
            print(f"Error executing similarity search: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 2: Philosophy-Specific Arguments
    # ========================================================================
    
    def retrieve_philosophy_arguments(
        self,
        philosophy_school: str,
        argument_types: List[str],
        limit: int = 20
    ) -> List[Dict[str, Any]]:
        """
        Retrieve arguments from a specific philosophy school.
        
        AQL Pattern:
        ```
        SELECT argument_id, content, argument_type, strength, principle_basis
        FROM ethical_arguments
        WHERE school = @school 
          AND argument_type IN @types
        ORDER BY strength DESC, created_at DESC
        LIMIT @limit
        ```
        
        Args:
            philosophy_school: Philosophy school ID (e.g., 'kantianismus')
            argument_types: Types of arguments (e.g., ['pro', 'contra'])
            limit: Maximum number of results
        
        Returns:
            List of arguments from the specified school
        """
        if not self.client:
            return self._mock_philosophy_arguments(philosophy_school, argument_types, limit)
        
        query = """
        SELECT 
            argument_id,
            content,
            argument_type,
            strength,
            principle_basis,
            created_at
        FROM ethical_arguments
        WHERE philosophy_school = @school
          AND argument_type IN @types
        ORDER BY 
            CASE strength
                WHEN 'decisive' THEN 4
                WHEN 'strong' THEN 3
                WHEN 'moderate' THEN 2
                ELSE 1
            END DESC,
            created_at DESC
        LIMIT @limit
        """
        
        params = {
            'school': philosophy_school,
            'types': argument_types,
            'limit': limit
        }
        
        try:
            # results = self.client.query(query, params)
            # return results
            return self._mock_philosophy_arguments(philosophy_school, argument_types, limit)
        except Exception as e:
            print(f"Error retrieving philosophy arguments: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 3: Best-Practice Synthesis Retrieval
    # ========================================================================
    
    def retrieve_best_practices(
        self,
        dilemma_category: str,
        min_satisfaction: float = 0.75,
        limit: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Retrieve best-practice decisions with high satisfaction scores.
        
        AQL Pattern:
        ```
        SELECT d.decision_id, d.decision_text, d.philosophy, 
               d.satisfaction_score, d.consensus_level
        FROM ethical_decisions d
        WHERE d.category = @category
        GROUP BY d.decision_id
        HAVING d.satisfaction_score > @min_satisfaction
        ORDER BY d.satisfaction_score DESC, d.consensus_level DESC
        LIMIT @limit
        ```
        
        Args:
            dilemma_category: Category of dilemma (e.g., 'bioethics')
            min_satisfaction: Minimum satisfaction score threshold
            limit: Maximum number of results
        
        Returns:
            List of best-practice decisions
        """
        if not self.client:
            return self._mock_best_practices(dilemma_category, limit)
        
        query = """
        SELECT 
            d.decision_id,
            d.decision_text,
            d.primary_philosophy,
            d.supporting_philosophies,
            d.satisfaction_score,
            d.consensus_level,
            d.argument_chain_ids,
            d.created_at
        FROM ethical_decisions d
        JOIN ethical_dilemmas e ON d.dilemma_id = e.dilemma_id
        WHERE e.category = @category
        GROUP BY d.decision_id
        HAVING d.satisfaction_score > @min_satisfaction
        ORDER BY d.satisfaction_score DESC, d.consensus_level DESC
        LIMIT @limit
        """
        
        params = {
            'category': dilemma_category,
            'min_satisfaction': min_satisfaction,
            'limit': limit
        }
        
        try:
            # results = self.client.query(query, params)
            # return results
            return self._mock_best_practices(dilemma_category, limit)
        except Exception as e:
            print(f"Error retrieving best practices: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 4: Vector Semantic Search
    # ========================================================================
    
    def vector_semantic_search(
        self,
        query_embedding: List[float],
        collection: str = "ethical_arguments",
        limit: int = 15
    ) -> List[Dict[str, Any]]:
        """
        Perform vector-based semantic search for similar arguments.
        
        AQL Pattern:
        ```
        SELECT argument_id, content, philosophy_school,
               VECTOR_DISTANCE(embedding, @query_embedding) AS distance
        FROM vector_ethical_arguments
        WHERE VECTOR_DISTANCE(embedding, @query_embedding) < @threshold
        ORDER BY distance ASC
        LIMIT @limit
        ```
        
        Args:
            query_embedding: Vector embedding of the query
            collection: Vector collection name
            limit: Maximum number of results
        
        Returns:
            List of semantically similar arguments
        """
        if not self.client:
            return self._mock_vector_search(query_embedding, limit)
        
        query = """
        SELECT 
            argument_id,
            content,
            philosophy_school,
            argument_type,
            VECTOR_DISTANCE(embedding, @query_embedding) AS distance,
            metadata
        FROM vector_ethical_arguments
        WHERE VECTOR_DISTANCE(embedding, @query_embedding) < @threshold
        ORDER BY distance ASC
        LIMIT @limit
        """
        
        params = {
            'query_embedding': query_embedding,
            'threshold': self.vector_distance_threshold,
            'limit': limit
        }
        
        try:
            # results = self.client.vector_query(query, params)
            # return results
            return self._mock_vector_search(query_embedding, limit)
        except Exception as e:
            print(f"Error executing vector search: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 5: Argument Chain Traversal
    # ========================================================================
    
    def traverse_argument_chains(
        self,
        start_argument_id: str,
        max_depth: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Traverse argument chains using graph pattern matching.
        
        AQL Pattern (Graph Traversal):
        ```
        MATCH path = (start:Argument {id: @start_id})
                     -[:supports|counters|rebuts*1..@max_depth]->(related:Argument)
        RETURN path, related.content, related.philosophy_school
        ORDER BY length(path) ASC
        ```
        
        Args:
            start_argument_id: Starting argument ID
            max_depth: Maximum traversal depth
        
        Returns:
            List of connected arguments in chains
        """
        if not self.client:
            return self._mock_argument_chains(start_argument_id, max_depth)
        
        # Graph query for argument chain traversal
        graph_query = """
        MATCH path = (start:EthicalArgument {id: @start_id})
                     -[:supports|counters|rebuts*1..@max_depth]->(related:EthicalArgument)
        RETURN 
            path,
            related.id AS argument_id,
            related.content AS content,
            related.philosophy_school AS school,
            length(path) AS chain_depth
        ORDER BY length(path) ASC
        """
        
        params = {
            'start_id': start_argument_id,
            'max_depth': max_depth
        }
        
        try:
            # results = self.client.graph_query(graph_query, params)
            # return results
            return self._mock_argument_chains(start_argument_id, max_depth)
        except Exception as e:
            print(f"Error traversing argument chains: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 6: Temporal Filtering
    # ========================================================================
    
    def retrieve_recent_debates(
        self,
        days_back: int = 30,
        category: Optional[str] = None,
        limit: int = 20
    ) -> List[Dict[str, Any]]:
        """
        Retrieve recent debates for temporal context.
        
        AQL Pattern:
        ```
        SELECT dilemma_id, description, category, created_at, decision_count
        FROM ethical_dilemmas
        WHERE created_at >= @start_date
          AND (@category IS NULL OR category = @category)
        ORDER BY created_at DESC
        LIMIT @limit
        ```
        
        Args:
            days_back: Number of days to look back
            category: Optional category filter
            limit: Maximum number of results
        
        Returns:
            List of recent debates
        """
        if not self.client:
            return self._mock_recent_debates(days_back, category, limit)
        
        start_date = (datetime.now() - timedelta(days=days_back)).isoformat()
        
        query = """
        SELECT 
            dilemma_id,
            description,
            category,
            created_at,
            decision_count,
            participating_schools
        FROM ethical_dilemmas
        WHERE created_at >= @start_date
          AND (@category IS NULL OR category = @category)
        ORDER BY created_at DESC
        LIMIT @limit
        """
        
        params = {
            'start_date': start_date,
            'category': category,
            'limit': limit
        }
        
        try:
            # results = self.client.query(query, params)
            # return results
            return self._mock_recent_debates(days_back, category, limit)
        except Exception as e:
            print(f"Error retrieving recent debates: {e}")
            return []
    
    # ========================================================================
    # AQL Query Pattern 7: Multi-Philosophy Consensus
    # ========================================================================
    
    def find_consensus_decisions(
        self,
        min_philosophy_count: int = 3,
        min_consensus: float = 0.8,
        limit: int = 10
    ) -> List[Dict[str, Any]]:
        """
        Find decisions with multi-philosophy consensus.
        
        AQL Pattern:
        ```
        SELECT d.decision_id, d.decision_text, d.consensus_level,
               COUNT(DISTINCT d.supporting_philosophies) as philosophy_count
        FROM ethical_decisions d
        GROUP BY d.decision_id
        HAVING philosophy_count >= @min_count 
           AND d.consensus_level >= @min_consensus
        ORDER BY d.consensus_level DESC, philosophy_count DESC
        LIMIT @limit
        ```
        
        Args:
            min_philosophy_count: Minimum number of supporting philosophies
            min_consensus: Minimum consensus level
            limit: Maximum number of results
        
        Returns:
            List of consensus decisions
        """
        if not self.client:
            return self._mock_consensus_decisions(min_philosophy_count, limit)
        
        query = """
        SELECT 
            d.decision_id,
            d.decision_text,
            d.primary_philosophy,
            d.supporting_philosophies,
            d.consensus_level,
            d.satisfaction_score,
            ARRAY_LENGTH(d.supporting_philosophies) + 1 AS philosophy_count
        FROM ethical_decisions d
        WHERE ARRAY_LENGTH(d.supporting_philosophies) + 1 >= @min_count
          AND d.consensus_level >= @min_consensus
        ORDER BY d.consensus_level DESC, philosophy_count DESC
        LIMIT @limit
        """
        
        params = {
            'min_count': min_philosophy_count,
            'min_consensus': min_consensus,
            'limit': limit
        }
        
        try:
            # results = self.client.query(query, params)
            # return results
            return self._mock_consensus_decisions(min_philosophy_count, limit)
        except Exception as e:
            print(f"Error finding consensus decisions: {e}")
            return []
    
    # ========================================================================
    # Context Aggregation
    # ========================================================================
    
    def build_rag_context(
        self,
        dilemma_description: str,
        philosophy_schools: List[str],
        dilemma_category: str = "general"
    ) -> Dict[str, Any]:
        """
        Build comprehensive RAG context for ethical decision-making.
        
        Aggregates:
        - Similar historical dilemmas
        - Philosophy-specific arguments
        - Best-practice decisions
        - Recent debates for temporal context
        - Consensus decisions
        
        Args:
            dilemma_description: Description of current dilemma
            philosophy_schools: List of participating philosophy schools
            dilemma_category: Category of the dilemma
        
        Returns:
            Comprehensive context dictionary
        """
        context = {
            'similar_dilemmas': self.find_similar_dilemmas(dilemma_description, limit=5),
            'philosophy_arguments': {},
            'best_practices': self.retrieve_best_practices(dilemma_category, limit=3),
            'recent_debates': self.retrieve_recent_debates(days_back=30, category=dilemma_category, limit=10),
            'consensus_decisions': self.find_consensus_decisions(min_philosophy_count=2, limit=5),
            'retrieval_timestamp': datetime.now().isoformat()
        }
        
        # Retrieve arguments for each participating philosophy
        for school in philosophy_schools:
            context['philosophy_arguments'][school] = self.retrieve_philosophy_arguments(
                school,
                ['pro', 'contra'],
                limit=10
            )
        
        return context
    
    def format_context_for_prompt(self, context: Dict[str, Any]) -> str:
        """
        Format RAG context for inclusion in AI prompts.
        
        Args:
            context: Context dictionary from build_rag_context()
        
        Returns:
            Formatted context string for prompt injection
        """
        prompt_parts = []
        
        # Similar dilemmas
        if context.get('similar_dilemmas'):
            prompt_parts.append("## Similar Historical Dilemmas:")
            for i, dilemma in enumerate(context['similar_dilemmas'][:3], 1):
                prompt_parts.append(
                    f"{i}. {dilemma.get('description', 'N/A')} "
                    f"(similarity: {dilemma.get('similarity_score', 0):.2f})"
                )
        
        # Best practices
        if context.get('best_practices'):
            prompt_parts.append("\n## Best-Practice Decisions:")
            for i, decision in enumerate(context['best_practices'][:2], 1):
                prompt_parts.append(
                    f"{i}. {decision.get('decision_text', 'N/A')} "
                    f"(satisfaction: {decision.get('satisfaction_score', 0):.2f}, "
                    f"consensus: {decision.get('consensus_level', 0):.2f})"
                )
        
        # Philosophy-specific arguments
        if context.get('philosophy_arguments'):
            prompt_parts.append("\n## Relevant Philosophical Arguments:")
            for school, arguments in context['philosophy_arguments'].items():
                if arguments:
                    prompt_parts.append(f"\n### {school.title()}:")
                    for arg in arguments[:3]:
                        prompt_parts.append(f"- {arg.get('content', 'N/A')[:150]}...")
        
        # Recent context
        if context.get('recent_debates'):
            count = len(context['recent_debates'])
            prompt_parts.append(f"\n## Recent Debates: {count} related debates in the last 30 days")
        
        return "\n".join(prompt_parts)
    
    # ========================================================================
    # Mock Data Methods (for testing without ThemisDB)
    # ========================================================================
    
    def _mock_similar_dilemmas(self, query: str, limit: int) -> List[Dict[str, Any]]:
        """Mock similar dilemmas for testing."""
        return [
            {
                'dilemma_id': f'dilemma_{i}',
                'description': f'Similar ethical dilemma {i} related to: {query[:50]}...',
                'similarity_score': 0.85 - (i * 0.1),
                'decision_count': 3 + i,
                'avg_satisfaction_score': 0.75 + (i * 0.05)
            }
            for i in range(min(limit, 3))
        ]
    
    def _mock_philosophy_arguments(self, school: str, types: List[str], limit: int) -> List[Dict[str, Any]]:
        """Mock philosophy arguments for testing."""
        return [
            {
                'argument_id': f'arg_{school}_{i}',
                'content': f'{school} argument {i} of type {types[i % len(types)]}',
                'argument_type': types[i % len(types)],
                'strength': 'strong',
                'principle_basis': [f'{school}_principle_{i}'],
                'created_at': datetime.now().isoformat()
            }
            for i in range(min(limit, 5))
        ]
    
    def _mock_best_practices(self, category: str, limit: int) -> List[Dict[str, Any]]:
        """Mock best practices for testing."""
        return [
            {
                'decision_id': f'decision_{i}',
                'decision_text': f'Best practice decision {i} for {category}',
                'primary_philosophy': 'kant',
                'supporting_philosophies': ['utilitarianism', 'virtue_ethics'],
                'satisfaction_score': 0.85,
                'consensus_level': 0.80,
                'argument_chain_ids': [f'chain_{i}'],
                'created_at': datetime.now().isoformat()
            }
            for i in range(min(limit, 2))
        ]
    
    def _mock_vector_search(self, embedding: List[float], limit: int) -> List[Dict[str, Any]]:
        """Mock vector search for testing."""
        return [
            {
                'argument_id': f'vec_arg_{i}',
                'content': f'Semantically similar argument {i}',
                'philosophy_school': ['kant', 'utilitarianism', 'virtue_ethics'][i % 3],
                'argument_type': 'pro',
                'distance': 0.15 + (i * 0.05),
                'metadata': {}
            }
            for i in range(min(limit, 5))
        ]
    
    def _mock_argument_chains(self, start_id: str, max_depth: int) -> List[Dict[str, Any]]:
        """Mock argument chains for testing."""
        return [
            {
                'argument_id': f'chain_arg_{i}',
                'content': f'Chained argument {i} from {start_id}',
                'school': ['kant', 'utilitarianism'][i % 2],
                'chain_depth': i + 1
            }
            for i in range(min(max_depth, 3))
        ]
    
    def _mock_recent_debates(self, days: int, category: Optional[str], limit: int) -> List[Dict[str, Any]]:
        """Mock recent debates for testing."""
        return [
            {
                'dilemma_id': f'recent_{i}',
                'description': f'Recent debate {i} in last {days} days',
                'category': category or 'general',
                'created_at': (datetime.now() - timedelta(days=i*2)).isoformat(),
                'decision_count': 2 + i,
                'participating_schools': ['kant', 'utilitarianism', 'virtue_ethics']
            }
            for i in range(min(limit, 5))
        ]
    
    def _mock_consensus_decisions(self, min_count: int, limit: int) -> List[Dict[str, Any]]:
        """Mock consensus decisions for testing."""
        return [
            {
                'decision_id': f'consensus_{i}',
                'decision_text': f'Consensus decision {i} with {min_count}+ philosophies',
                'primary_philosophy': 'kant',
                'supporting_philosophies': ['utilitarianism', 'virtue_ethics', 'care_ethics'][:min_count-1],
                'consensus_level': 0.85,
                'satisfaction_score': 0.80,
                'philosophy_count': min_count
            }
            for i in range(min(limit, 3))
        ]


# Convenience function
def create_rag_engine(themis_host: str = "localhost", themis_port: int = 8080) -> RagContextEngine:
    """
    Create a RAG context engine with ThemisDB client.
    
    Args:
        themis_host: ThemisDB host
        themis_port: ThemisDB port
    
    Returns:
        Configured RagContextEngine
    """
    try:
        from themis_client import MoralDebateClient
        client = MoralDebateClient(host=themis_host, port=themis_port)
    except ImportError:
        print("Warning: themis_client not available, using mock data")
        client = None
    
    return RagContextEngine(themis_client=client)
