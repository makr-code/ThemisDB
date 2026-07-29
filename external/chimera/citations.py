"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            citations.py                                       ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     356                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
IEEE Citation Manager for Benchmark Reporting

Manages IEEE-style citations for methods, benchmarks, and tools used in reporting.
"""

from typing import Dict, List
from dataclasses import dataclass
from enum import Enum


class CitationType(Enum):
    """Types of citations"""
    STANDARD = "standard"
    JOURNAL = "journal"
    CONFERENCE = "conference"
    BOOK = "book"
    SOFTWARE = "software"
    WEBPAGE = "webpage"


@dataclass
class Citation:
    """IEEE-style citation"""
    id: str
    authors: List[str]
    title: str
    publication: str
    year: int
    citation_type: CitationType
    doi: str = ""
    url: str = ""
    pages: str = ""
    volume: str = ""
    issue: str = ""
    
    def format_ieee(self) -> str:
        """Format citation in IEEE style"""
        # Format authors
        if len(self.authors) == 0:
            author_str = ""
        elif len(self.authors) == 1:
            author_str = self.authors[0]
        elif len(self.authors) == 2:
            author_str = f"{self.authors[0]} and {self.authors[1]}"
        elif len(self.authors) <= 6:
            author_str = ", ".join(self.authors[:-1]) + f", and {self.authors[-1]}"
        else:
            author_str = ", ".join(self.authors[:6]) + " et al."
        
        # Format based on type
        if self.citation_type == CitationType.JOURNAL:
            citation = f'{author_str}, "{self.title}," {self.publication}'
            if self.volume:
                citation += f", vol. {self.volume}"
            if self.issue:
                citation += f", no. {self.issue}"
            if self.pages:
                citation += f", pp. {self.pages}"
            citation += f", {self.year}."
        
        elif self.citation_type == CitationType.CONFERENCE:
            citation = f'{author_str}, "{self.title}," in {self.publication}, {self.year}'
            if self.pages:
                citation += f", pp. {self.pages}"
            citation += "."
        
        elif self.citation_type == CitationType.BOOK:
            citation = f'{author_str}, {self.title}. {self.publication}, {self.year}.'
        
        elif self.citation_type == CitationType.STANDARD:
            if author_str:
                citation = f'{author_str}, {self.title}, {self.publication}, {self.year}.'
            else:
                citation = f'{self.title}, {self.publication}, {self.year}.'
        
        elif self.citation_type == CitationType.SOFTWARE:
            if author_str:
                citation = f'{author_str}, {self.title}, {self.year}.'
            else:
                citation = f'{self.title}, {self.year}.'
            if self.url:
                citation += f' [Online]. Available: {self.url}'
        
        else:  # WEBPAGE
            if author_str:
                citation = f'{author_str}, "{self.title}," {self.publication}, {self.year}.'
            else:
                citation = f'"{self.title}," {self.publication}, {self.year}.'
            if self.url:
                citation += f' [Online]. Available: {self.url}'
        
        if self.doi:
            citation += f' doi: {self.doi}'
        
        return citation


class CitationManager:
    """
    Manages citations for benchmark reports.
    
    Pre-loaded with standard citations for statistical methods,
    color schemes, and benchmarking practices.
    """
    
    def __init__(self):
        """Initialize citation manager with standard citations"""
        self.citations: Dict[str, Citation] = {}
        self._load_standard_citations()
    
    def _load_standard_citations(self):
        """Load standard citations for common methods and tools"""
        
        # Statistical methods
        self.add_citation(Citation(
            id="cohen1988",
            authors=["J. Cohen"],
            title="Statistical Power Analysis for the Behavioral Sciences",
            publication="Lawrence Erlbaum Associates",
            year=1988,
            citation_type=CitationType.BOOK
        ))
        
        self.add_citation(Citation(
            id="mann1947",
            authors=["H. B. Mann", "D. R. Whitney"],
            title="On a test of whether one of two random variables is stochastically larger than the other",
            publication="The Annals of Mathematical Statistics",
            year=1947,
            citation_type=CitationType.JOURNAL,
            volume="18",
            issue="1",
            pages="50-60"
        ))
        
        self.add_citation(Citation(
            id="welch1947",
            authors=["B. L. Welch"],
            title="The generalization of 'Student's' problem when several different population variances are involved",
            publication="Biometrika",
            year=1947,
            citation_type=CitationType.JOURNAL,
            volume="34",
            issue="1-2",
            pages="28-35"
        ))
        
        # Color schemes
        self.add_citation(Citation(
            id="okabe2008",
            authors=["M. Okabe", "K. Ito"],
            title="Color Universal Design (CUD): How to make figures and presentations that are friendly to colorblind people",
            publication="J*FLY",
            year=2008,
            citation_type=CitationType.WEBPAGE,
            url="https://jfly.uni-koeln.de/color/"
        ))
        
        self.add_citation(Citation(
            id="tol2021",
            authors=["P. Tol"],
            title="Colour Schemes",
            publication="Personal webpage",
            year=2021,
            citation_type=CitationType.WEBPAGE,
            url="https://personal.sron.nl/~pault/"
        ))
        
        # Benchmarking standards
        self.add_citation(Citation(
            id="ieee_std_730",
            authors=[],
            title="IEEE Standard for Software Quality Assurance Processes",
            publication="IEEE Std 730-2014",
            year=2014,
            citation_type=CitationType.STANDARD,
            doi="10.1109/IEEESTD.2014.6835311"
        ))
        
        self.add_citation(Citation(
            id="ieee_std_1012",
            authors=[],
            title="IEEE Standard for System, Software, and Hardware Verification and Validation",
            publication="IEEE Std 1012-2016",
            year=2016,
            citation_type=CitationType.STANDARD,
            doi="10.1109/IEEESTD.2017.7961505"
        ))
        
        # Outlier detection
        self.add_citation(Citation(
            id="tukey1977",
            authors=["J. W. Tukey"],
            title="Exploratory Data Analysis",
            publication="Addison-Wesley",
            year=1977,
            citation_type=CitationType.BOOK
        ))
        
        # Database benchmarking
        self.add_citation(Citation(
            id="tpc_c",
            authors=[],
            title="TPC Benchmark C Standard Specification",
            publication="Transaction Processing Performance Council",
            year=2010,
            citation_type=CitationType.STANDARD,
            url="http://www.tpc.org/tpcc/"
        ))
        
        self.add_citation(Citation(
            id="tpc_h",
            authors=[],
            title="TPC Benchmark H Standard Specification",
            publication="Transaction Processing Performance Council",
            year=2018,
            citation_type=CitationType.STANDARD,
            url="http://www.tpc.org/tpch/"
        ))
        
        self.add_citation(Citation(
            id="ycsb2010",
            authors=["B. F. Cooper", "A. Silberstein", "E. Tam", "R. Ramakrishnan", "R. Sears"],
            title="Benchmarking cloud serving systems with YCSB",
            publication="Proceedings of the 1st ACM symposium on Cloud computing",
            year=2010,
            citation_type=CitationType.CONFERENCE,
            pages="143-154"
        ))
    
    def add_citation(self, citation: Citation):
        """Add a citation to the manager"""
        self.citations[citation.id] = citation
    
    def get_citation(self, citation_id: str) -> Citation:
        """Get a citation by ID"""
        if citation_id not in self.citations:
            raise ValueError(f"Citation not found: {citation_id}")
        return self.citations[citation_id]
    
    def format_bibliography(self, citation_ids: List[str] = None) -> str:
        """
        Format a bibliography in IEEE style.
        
        Args:
            citation_ids: List of citation IDs to include. If None, includes all.
            
        Returns:
            Formatted bibliography string
        """
        if citation_ids is None:
            citation_ids = sorted(self.citations.keys())
        
        bibliography = ["## References\n"]
        
        for i, cid in enumerate(citation_ids, 1):
            if cid in self.citations:
                citation = self.citations[cid]
                bibliography.append(f"[{i}] {citation.format_ieee()}\n")
        
        return "\n".join(bibliography)
    
    def get_neutrality_statement(self) -> str:
        """
        Get the neutrality and methodology disclosure statement.
        
        Returns:
            Formatted neutrality statement
        """
        return """
## Neutrality Seal and Methodology Disclosure

### Vendor Neutrality
This report has been generated using the CHIMERA (Comprehensive, Honest, Impartial 
Metrics for Empirical Reporting and Analysis) framework. All results are presented 
in a vendor-neutral manner with the following guarantees:

- **No Vendor Bias**: Systems are sorted alphabetically or by metric value only, 
  never by brand preference
- **Color-Blind Friendly**: All visualizations use scientifically validated 
  color-blind accessible palettes [1], [2]
- **No Commercial Logos**: Reports contain no vendor logos, brand colors, or 
  marketing materials
- **Transparent Methodology**: All statistical methods are fully disclosed and 
  referenced

### Statistical Methodology
All statistical analyses follow IEEE and ACM standards for scientific computing:

- **Outlier Removal**: Interquartile Range (IQR) method [3]
- **Hypothesis Testing**: Welch's t-test [4] and Mann-Whitney U test [5]
- **Effect Size**: Cohen's d [6] for parametric tests
- **Confidence Intervals**: 95% confidence intervals using t-distribution
- **Significance Level**: α = 0.05 (adjustable)

### Data Quality
- Outliers are identified and removed using the IQR method (1.5 × IQR)
- Minimum sample size: n ≥ 3 for statistical validity
- All removed outliers are reported transparently

### Interpretation Guidelines
- Statistical significance does not imply practical significance
- Effect sizes must be considered alongside p-values
- Multiple comparison corrections are applied when testing multiple hypotheses
- "No default winner" - significance indicates difference, not superiority

---

*This report was generated using CHIMERA v1.0.0*  
*For questions or concerns about methodology, please consult the documentation.*
"""
    
    def get_method_citations(self) -> Dict[str, str]:
        """
        Get citations for all methods used in the framework.
        
        Returns:
            Dictionary mapping method names to citation IDs
        """
        return {
            "t_test": "welch1947",
            "mann_whitney_u": "mann1947",
            "cohens_d": "cohen1988",
            "outlier_detection": "tukey1977",
            "color_palette_okabe_ito": "okabe2008",
            "color_palette_tol": "tol2021",
            "ieee_quality_standards": "ieee_std_730",
            "ieee_validation_standards": "ieee_std_1012",
            "tpc_c_benchmark": "tpc_c",
            "tpc_h_benchmark": "tpc_h",
            "ycsb_benchmark": "ycsb2010"
        }
