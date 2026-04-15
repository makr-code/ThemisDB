"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_validator.py                              ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     269                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Example Python Script Template for Feedback Validation

This script demonstrates how to create a custom feedback validation plugin
that can be called from ThemisDB's feedback system via subprocess or API.

Usage:
    python3 feedback_validator.py validate <feedback_json>
    python3 feedback_validator.py stats

The script reads feedback data from stdin (JSON) and outputs validation result.
"""

import json
import sys
import re
from typing import Dict, Any, List
from enum import Enum

class ValidationResult(Enum):
    """Validation result types"""
    ACCEPT = "accept"
    REJECT = "reject"
    FLAG = "flag"
    MODIFY = "modify"

class FeedbackValidator:
    """
    Custom feedback validator
    
    Implement your validation logic here. This example includes:
    - Spam keyword detection
    - PII detection (email, phone)
    - Quality scoring
    - Custom business rules
    """
    
    def __init__(self, config: Dict[str, Any] = None):
        self.config = config or {}
        self.spam_keywords = self.config.get('spam_keywords', [
            'buy now', 'click here', 'casino', 'lottery',
            'free money', 'work from home'
        ])
        self.validation_count = 0
        self.rejected_count = 0
        
    def validate(self, feedback: Dict[str, Any]) -> Dict[str, Any]:
        """
        Validate feedback and return result
        
        Args:
            feedback: Feedback data with fields:
                - question: str
                - answer: str
                - correction: str (optional)
                - comment: str (optional)
                - user_id: str
                - adapter_id: str
                - is_positive: bool
                
        Returns:
            Validation response with:
                - result: str (accept/reject/flag/modify)
                - reason: str (optional)
                - confidence: float (0-1)
                - plugin_data: dict (optional)
        """
        self.validation_count += 1
        
        result = {
            'result': ValidationResult.ACCEPT.value,
            'confidence': 1.0,
            'plugin_data': {}
        }
        
        # Check 1: Spam keywords
        if self._contains_spam(feedback):
            result['result'] = ValidationResult.REJECT.value
            result['reason'] = 'Contains spam keywords'
            result['confidence'] = 0.9
            self.rejected_count += 1
            return result
        
        # Check 2: PII detection
        pii_found = self._detect_pii(feedback)
        if pii_found:
            result['result'] = ValidationResult.FLAG.value
            result['reason'] = f'Contains PII: {", ".join(pii_found)}'
            result['confidence'] = 0.95
            result['plugin_data']['pii_types'] = pii_found
            return result
        
        # Check 3: Quality score
        quality_score = self._calculate_quality(feedback)
        result['plugin_data']['quality_score'] = quality_score
        
        if quality_score < 0.3:
            result['result'] = ValidationResult.REJECT.value
            result['reason'] = 'Low quality feedback'
            result['confidence'] = quality_score
            self.rejected_count += 1
        elif quality_score < 0.6:
            result['result'] = ValidationResult.FLAG.value
            result['reason'] = 'Medium quality - needs review'
            result['confidence'] = quality_score
        else:
            result['confidence'] = quality_score
        
        return result
    
    def _contains_spam(self, feedback: Dict[str, Any]) -> bool:
        """Check if feedback contains spam keywords"""
        text = ' '.join([
            feedback.get('question', ''),
            feedback.get('answer', ''),
            feedback.get('comment', '')
        ]).lower()
        
        return any(keyword in text for keyword in self.spam_keywords)
    
    def _detect_pii(self, feedback: Dict[str, Any]) -> List[str]:
        """Detect PII in feedback"""
        pii_found = []
        text = ' '.join([
            feedback.get('question', ''),
            feedback.get('answer', ''),
            feedback.get('comment', '')
        ])
        
        # Email pattern
        if re.search(r'[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}', text):
            pii_found.append('email')
        
        # Phone pattern (various formats)
        if re.search(r'\d{3}[-.\s]?\d{3}[-.\s]?\d{4}', text):
            pii_found.append('phone')
        
        # SSN pattern (XXX-XX-XXXX)
        if re.search(r'\d{3}-\d{2}-\d{4}', text):
            pii_found.append('ssn')
        
        # Credit card pattern
        if re.search(r'\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}', text):
            pii_found.append('credit_card')
        
        return pii_found
    
    def _calculate_quality(self, feedback: Dict[str, Any]) -> float:
        """Calculate quality score (0-1)"""
        # Configuration constants
        MIN_LENGTH_SHORT = 5
        MIN_LENGTH_NORMAL = 10
        MAX_LENGTH = 5000
        MIN_REPETITION = 10
        
        score = 1.0
        
        question = feedback.get('question', '')
        answer = feedback.get('answer', '')
        correction = feedback.get('correction', '')
        is_positive = feedback.get('is_positive', True)
        
        # Length checks
        if len(question) < MIN_LENGTH_NORMAL:
            score -= 0.2
        if len(answer) < MIN_LENGTH_NORMAL:
            score -= 0.2
        
        # Too short overall
        if len(question) < MIN_LENGTH_SHORT or len(answer) < MIN_LENGTH_SHORT:
            score -= 0.3
        
        # Negative feedback without correction
        if not is_positive and len(correction) < MIN_LENGTH_SHORT:
            score -= 0.3
        
        # Excessive repetition
        if re.search(rf'(.)\1{{{MIN_REPETITION},}}', question + answer):
            score -= 0.4
        
        # Too long (copy-paste spam)
        if len(question) > MAX_LENGTH or len(answer) > MAX_LENGTH:
            score -= 0.3
        
        return max(0.0, min(1.0, score))
    
    def get_statistics(self) -> Dict[str, Any]:
        """Get plugin statistics"""
        return {
            'validation_count': self.validation_count,
            'rejected_count': self.rejected_count,
            'rejection_rate': self.rejected_count / max(1, self.validation_count),
            'spam_keywords_count': len(self.spam_keywords)
        }

def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print(json.dumps({
            'error': 'Missing command',
            'usage': 'feedback_validator.py validate|stats'
        }), file=sys.stderr)
        sys.exit(1)
    
    command = sys.argv[1]
    
    # Load config if available
    config = {}
    try:
        with open('feedback_validator_config.json', 'r') as f:
            config = json.load(f)
    except FileNotFoundError:
        pass
    
    validator = FeedbackValidator(config)
    
    if command == 'validate':
        # Read feedback from stdin
        try:
            feedback_data = json.load(sys.stdin)
        except json.JSONDecodeError as e:
            print(json.dumps({
                'error': f'Invalid JSON input: {str(e)}'
            }), file=sys.stderr)
            sys.exit(1)
        
        # Validate
        result = validator.validate(feedback_data)
        
        # Output result
        print(json.dumps(result, indent=2))
        
    elif command == 'stats':
        # Get statistics
        stats = validator.get_statistics()
        print(json.dumps(stats, indent=2))
        
    else:
        print(json.dumps({
            'error': f'Unknown command: {command}',
            'valid_commands': ['validate', 'stats']
        }), file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
