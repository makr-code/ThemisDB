"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_utils.py                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:51:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     255                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Unit tests for RoPE visualization utilities
"""

import unittest
import numpy as np
import sys
from pathlib import Path

# Add rope_visualizer to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / 'tools'))

from rope_visualizer.utils import (
    compute_similarity_matrix,
    project_embeddings,
    rotate_embedding_2d,
    compute_rotation_angles,
    apply_rope_rotation
)


class TestRopeVisualizerUtils(unittest.TestCase):
    """Test utility functions for RoPE visualization"""
    
    def setUp(self):
        """Set up test fixtures"""
        np.random.seed(42)
        self.hidden_dim = 128
        self.num_samples = 50
        self.embeddings = np.random.randn(self.num_samples, self.hidden_dim).astype(np.float32)
        # Normalize embeddings
        norms = np.linalg.norm(self.embeddings, axis=1, keepdims=True)
        self.embeddings = self.embeddings / norms
    
    def test_compute_similarity_matrix_cosine(self):
        """Test cosine similarity matrix computation"""
        similarity = compute_similarity_matrix(self.embeddings, metric='cosine')
        
        # Check shape
        self.assertEqual(similarity.shape, (self.num_samples, self.num_samples))
        
        # Check diagonal (self-similarity should be ~1.0)
        diagonal = np.diag(similarity)
        np.testing.assert_array_almost_equal(diagonal, np.ones(self.num_samples), decimal=5)
        
        # Check symmetry
        np.testing.assert_array_almost_equal(similarity, similarity.T, decimal=5)
        
        # Check range [-1, 1] (with small tolerance for numerical precision)
        self.assertTrue(np.all(similarity >= -1.0 - 1e-6))
        self.assertTrue(np.all(similarity <= 1.0 + 1e-6))
    
    def test_compute_similarity_matrix_euclidean(self):
        """Test Euclidean similarity matrix computation"""
        similarity = compute_similarity_matrix(self.embeddings, metric='euclidean')
        
        # Check shape
        self.assertEqual(similarity.shape, (self.num_samples, self.num_samples))
        
        # Check symmetry
        np.testing.assert_array_almost_equal(similarity, similarity.T, decimal=5)
        
        # All values should be positive
        self.assertTrue(np.all(similarity > 0))
    
    def test_compute_similarity_matrix_dot(self):
        """Test dot product similarity matrix computation"""
        similarity = compute_similarity_matrix(self.embeddings, metric='dot')
        
        # Check shape
        self.assertEqual(similarity.shape, (self.num_samples, self.num_samples))
        
        # Check symmetry
        np.testing.assert_array_almost_equal(similarity, similarity.T, decimal=5)
    
    def test_project_embeddings_pca(self):
        """Test PCA projection"""
        projected = project_embeddings(self.embeddings, method='pca', n_components=2)
        
        # Check shape
        self.assertEqual(projected.shape, (self.num_samples, 2))
        
        # Check that projection is real
        self.assertFalse(np.any(np.isnan(projected)))
        self.assertFalse(np.any(np.isinf(projected)))
    
    def test_project_embeddings_tsne(self):
        """Test t-SNE projection"""
        projected = project_embeddings(self.embeddings, method='tsne', n_components=2)
        
        # Check shape
        self.assertEqual(projected.shape, (self.num_samples, 2))
        
        # Check that projection is real
        self.assertFalse(np.any(np.isnan(projected)))
        self.assertFalse(np.any(np.isinf(projected)))
    
    def test_project_embeddings_3d(self):
        """Test 3D projection"""
        projected = project_embeddings(self.embeddings, method='pca', n_components=3)
        
        # Check shape
        self.assertEqual(projected.shape, (self.num_samples, 3))
    
    def test_rotate_embedding_2d(self):
        """Test 2D coordinate rotation"""
        x, y = 1.0, 0.0
        theta = np.pi / 2  # 90 degrees
        
        x_rot, y_rot = rotate_embedding_2d(x, y, theta)
        
        # After 90 degree rotation, (1, 0) should become ~(0, 1)
        np.testing.assert_almost_equal(x_rot, 0.0, decimal=5)
        np.testing.assert_almost_equal(y_rot, 1.0, decimal=5)
    
    def test_compute_rotation_angles(self):
        """Test rotation angle computation"""
        position = 10
        angles = compute_rotation_angles(position, self.hidden_dim, base_theta=10000.0)
        
        # Check shape
        expected_num_pairs = self.hidden_dim // 2
        self.assertEqual(len(angles), expected_num_pairs)
        
        # Check that angles are positive
        self.assertTrue(np.all(angles >= 0))
        
        # Check that angles increase with position
        angles_pos_20 = compute_rotation_angles(20, self.hidden_dim, base_theta=10000.0)
        # All angles should be larger for position 20 vs position 10
        self.assertTrue(np.all(angles_pos_20 >= angles))
    
    def test_apply_rope_rotation(self):
        """Test RoPE rotation application"""
        embedding = self.embeddings[0]
        position = 5
        
        rotated = apply_rope_rotation(embedding, position, base_theta=10000.0)
        
        # Check shape
        self.assertEqual(rotated.shape, embedding.shape)
        
        # Check that rotation is non-trivial (not identity)
        self.assertFalse(np.allclose(rotated, embedding))
        
        # Check that magnitude is preserved (rotation is unitary)
        original_norm = np.linalg.norm(embedding)
        rotated_norm = np.linalg.norm(rotated)
        np.testing.assert_almost_equal(original_norm, rotated_norm, decimal=5)
    
    def test_apply_rope_rotation_position_zero(self):
        """Test RoPE rotation at position 0 (should be identity)"""
        embedding = self.embeddings[0]
        
        rotated = apply_rope_rotation(embedding, 0, base_theta=10000.0)
        
        # At position 0, rotation should be identity
        np.testing.assert_array_almost_equal(rotated, embedding, decimal=5)
    
    def test_apply_rope_rotation_odd_dimension(self):
        """Test that odd dimensions raise error"""
        embedding = np.random.randn(127)  # Odd dimension
        
        with self.assertRaises(ValueError):
            apply_rope_rotation(embedding, 1, base_theta=10000.0)
    
    def test_invalid_projection_method(self):
        """Test invalid projection method"""
        with self.assertRaises(ValueError):
            project_embeddings(self.embeddings, method='invalid_method')
    
    def test_invalid_similarity_metric(self):
        """Test invalid similarity metric"""
        with self.assertRaises(ValueError):
            compute_similarity_matrix(self.embeddings, metric='invalid_metric')


class TestRopeRotationProperties(unittest.TestCase):
    """Test mathematical properties of RoPE rotation"""
    
    def setUp(self):
        """Set up test fixtures"""
        np.random.seed(42)
        self.hidden_dim = 64
        self.embedding = np.random.randn(self.hidden_dim).astype(np.float32)
        self.base_theta = 10000.0
    
    def test_rotation_invertibility(self):
        """Test that rotation can be inverted"""
        position = 10
        
        # Rotate forward
        rotated = apply_rope_rotation(self.embedding, position, self.base_theta)
        
        # Rotate backward (negative position should approximately invert)
        # Note: This is not exactly invertible due to the nature of RoPE
        # but rotating by -position should give similar results
        back_rotated = apply_rope_rotation(rotated, -position, self.base_theta)
        
        # Should be close to original (within numerical precision)
        np.testing.assert_array_almost_equal(back_rotated, self.embedding, decimal=4)
    
    def test_rotation_composability(self):
        """Test that rotations compose correctly"""
        pos1 = 5
        pos2 = 10
        
        # Rotate by pos1 + pos2
        direct = apply_rope_rotation(self.embedding, pos1 + pos2, self.base_theta)
        
        # Rotate by pos1, then by pos2
        intermediate = apply_rope_rotation(self.embedding, pos1, self.base_theta)
        composed = apply_rope_rotation(intermediate, pos2, self.base_theta)
        
        # Should be the same (within numerical precision)
        np.testing.assert_array_almost_equal(composed, direct, decimal=4)
    
    def test_rotation_norm_preservation(self):
        """Test that rotation preserves vector norm"""
        positions = [0, 1, 10, 100, 1000]
        original_norm = np.linalg.norm(self.embedding)
        
        for pos in positions:
            rotated = apply_rope_rotation(self.embedding, pos, self.base_theta)
            rotated_norm = np.linalg.norm(rotated)
            np.testing.assert_almost_equal(
                rotated_norm, original_norm, decimal=5,
                err_msg=f"Norm not preserved at position {pos}"
            )


if __name__ == '__main__':
    unittest.main()
