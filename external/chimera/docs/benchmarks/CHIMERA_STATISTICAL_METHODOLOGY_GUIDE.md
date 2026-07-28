# CHIMERA Suite: Statistical Methodology Guide

**Version:** 1.0.0  
**Standards Compliance:** IEEE Std 730-2014, IEEE Std 1012-2016  
**Last Updated:** April 2026

---

## Table of Contents

1. [Introduction](#introduction)
2. [Statistical Testing Framework](#statistical-testing-framework)
3. [Hypothesis Testing](#hypothesis-testing)
4. [Effect Size Measures](#effect-size-measures)
5. [Outlier Detection](#outlier-detection)
6. [Confidence Intervals](#confidence-intervals)
7. [Measurement Protocol](#measurement-protocol)
8. [Reporting Standards](#reporting-standards)
9. [Practical Examples](#practical-examples)

---

## 1. Introduction

### 1.1 Purpose

This guide explains the statistical methodologies used in CHIMERA Suite benchmarks to ensure:
- Scientific rigor
- Reproducibility
- Fair comparisons
- Proper interpretation of results

### 1.2 Key Principles

**Statistical Significance ≠ Practical Significance**

A statistically significant difference (p < 0.05) does not automatically mean the difference is meaningful in practice. Always consider:
- Effect size magnitude
- Confidence intervals
- Practical implications
- Cost-benefit analysis

### 1.3 Standards Compliance

**IEEE Std 730-2014:** Software Quality Assurance Processes  
**IEEE Std 1012-2016:** System, Software, and Hardware Verification and Validation

---

## 2. Statistical Testing Framework

### 2.1 Configuration

```yaml
methodology:
  statistical_testing:
    # Significance level (Type I error rate)
    alpha: 0.05                         # 5% significance level
    
    # Confidence level for intervals
    confidence_level: 0.95              # 95% confidence intervals
    
    # Sample size requirements
    min_sample_size: 30                 # Minimum for CLT approximation
    recommended_sample_size: 100        # Recommended for robust results
    
    # Multiple testing correction
    multiple_testing_correction: "bonferroni"  # bonferroni, holm, fdr_bh
```

### 2.2 Significance Level (α)

**Definition:** Probability of rejecting the null hypothesis when it's true (Type I error).

**Common Values:**
- α = 0.05 (5%): Standard in most fields
- α = 0.01 (1%): More conservative, reduces false positives
- α = 0.10 (10%): More lenient, exploratory analysis

**Configuration:**
```yaml
alpha: 0.05
```

**Interpretation:**
- p < 0.05: Reject null hypothesis (statistically significant)
- p ≥ 0.05: Fail to reject null hypothesis (not significant)

### 2.3 Multiple Testing Correction

When performing multiple comparisons, adjust significance level:

**Bonferroni Correction:**
```yaml
multiple_testing_correction: "bonferroni"
```
- Adjusted α = α / n (n = number of tests)
- Conservative but simple
- Example: 10 tests, α = 0.05 → adjusted α = 0.005

**Holm-Bonferroni:**
```yaml
multiple_testing_correction: "holm"
```
- Less conservative than Bonferroni
- Better power
- Recommended for most cases

**False Discovery Rate (FDR):**
```yaml
multiple_testing_correction: "fdr_bh"
```
- Controls proportion of false discoveries
- More lenient
- Good for exploratory analysis

---

## 3. Hypothesis Testing

### 3.1 Parametric Tests

#### Welch's t-test

**When to Use:**
- Comparing means of two systems
- Continuous data
- Normal or near-normal distribution
- Unequal variances allowed

**Citation:** Welch, B. L. (1947). The generalization of 'Student's' problem when several different population variances are involved. Biometrika, 34(1-2), 28-35.

**Configuration:**
```yaml
tests:
  - name: "Welch's t-test"
    reference: "Welch, B. L. (1947). Biometrika, 34(1-2), 28-35"
    use_case: "Comparing means with unequal variances"
    assumptions:
      - "Continuous data"
      - "Independent samples"
      - "Approximately normal distribution"
    paired: false                       # Set true for paired samples
    two_tailed: true                    # Two-tailed test
```

**Null Hypothesis (H₀):** μ₁ = μ₂ (means are equal)  
**Alternative Hypothesis (H₁):** μ₁ ≠ μ₂ (means differ)

**Test Statistic:**
```
t = (x̄₁ - x̄₂) / √(s₁²/n₁ + s₂²/n₂)
```

**Interpretation:**
- p < α: Reject H₀, means are significantly different
- p ≥ α: Fail to reject H₀, no significant difference

**Example Results:**
```yaml
result:
  test_name: "Welch's t-test"
  system_a_mean: 12500
  system_b_mean: 15000
  t_statistic: -3.45
  df: 58                                # Degrees of freedom
  p_value: 0.001
  is_significant: true
  interpretation: "System B significantly faster (p = 0.001)"
```

#### Paired t-test

**When to Use:**
- Before/after comparisons
- Same system, different configurations
- Matched pairs

**Configuration:**
```yaml
tests:
  - name: "Paired t-test"
    reference: "Student (1908). Biometrika, 6(1), 1-25"
    paired: true
    use_case: "Comparing paired measurements"
```

#### ANOVA (Analysis of Variance)

**When to Use:**
- Comparing three or more systems
- Single independent variable

**Configuration:**
```yaml
tests:
  - name: "One-way ANOVA"
    reference: "Fisher, R. A. (1925). Statistical Methods for Research Workers"
    use_case: "Comparing means of multiple groups"
    post_hoc: "tukey_hsd"              # Post-hoc test for pairwise comparisons
```

### 3.2 Non-Parametric Tests

#### Mann-Whitney U Test

**When to Use:**
- Non-normal distributions
- Ordinal data
- Outliers present
- Small sample sizes
- Alternative to t-test

**Citation:** Mann, H. B., & Whitney, D. R. (1947). On a test of whether one of two random variables is stochastically larger than the other. The Annals of Mathematical Statistics, 18(1), 50-60.

**Configuration:**
```yaml
tests:
  - name: "Mann-Whitney U test"
    reference: "Mann & Whitney (1947). Ann. Math. Stat., 18(1), 50-60"
    use_case: "Non-parametric comparison of distributions"
    alternative: "two-sided"           # two-sided, less, greater
```

**Null Hypothesis (H₀):** Distributions are identical  
**Alternative Hypothesis (H₁):** One distribution is stochastically greater

**Test Statistic:**
```
U = n₁n₂ + n₁(n₁+1)/2 - R₁
```
Where R₁ is the sum of ranks for sample 1.

**Example Results:**
```yaml
result:
  test_name: "Mann-Whitney U test"
  u_statistic: 1250
  z_score: -2.34
  p_value: 0.019
  is_significant: true
  rank_biserial: 0.42                  # Effect size
```

#### Wilcoxon Signed-Rank Test

**When to Use:**
- Paired non-parametric test
- Alternative to paired t-test
- Non-normal differences

**Configuration:**
```yaml
tests:
  - name: "Wilcoxon Signed-Rank test"
    reference: "Wilcoxon (1945). Biometrics Bulletin, 1(6), 80-83"
    paired: true
    use_case: "Non-parametric paired comparison"
```

#### Kruskal-Wallis H Test

**When to Use:**
- Non-parametric alternative to ANOVA
- Three or more groups
- Non-normal distributions

**Configuration:**
```yaml
tests:
  - name: "Kruskal-Wallis H test"
    reference: "Kruskal & Wallis (1952). J. Am. Stat. Assoc., 47(260), 583-621"
    use_case: "Non-parametric comparison of multiple groups"
    post_hoc: "dunn"                   # Post-hoc test
```

### 3.3 Distribution Tests

#### Kolmogorov-Smirnov Test

**When to Use:**
- Test if data follows specific distribution
- Compare two distributions
- Assess normality

**Citation:** Massey, F. J. (1951). The Kolmogorov-Smirnov Test for Goodness of Fit. Journal of the American Statistical Association, 46(253), 68-78.

**Configuration:**
```yaml
tests:
  - name: "Kolmogorov-Smirnov test"
    reference: "Massey (1951). J. Am. Stat. Assoc., 46(253), 68-78"
    use_case: "Testing distribution equality"
```

#### Shapiro-Wilk Test

**When to Use:**
- Test for normality
- Small to medium sample sizes (n < 5000)

**Configuration:**
```yaml
tests:
  - name: "Shapiro-Wilk test"
    reference: "Shapiro & Wilk (1965). Biometrika, 52(3-4), 591-611"
    use_case: "Testing normality assumption"
```

---

## 4. Effect Size Measures

### 4.1 Why Effect Sizes Matter

**Problem with p-values alone:**
- Depend on sample size
- Don't indicate magnitude
- Can be "significant" but trivial

**Effect sizes provide:**
- Magnitude of difference
- Practical significance
- Independent of sample size
- Comparable across studies

### 4.2 Cohen's d

**When to Use:**
- Standardized mean difference
- Parametric comparisons
- Most common effect size

**Citation:** Cohen, J. (1988). Statistical Power Analysis for the Behavioral Sciences. 2nd Edition, Lawrence Erlbaum Associates.

**Configuration:**
```yaml
effect_sizes:
  - name: "Cohen's d"
    reference: "Cohen, J. (1988). Statistical Power Analysis"
    thresholds:
      negligible: 0.0
      small: 0.2
      medium: 0.5
      large: 0.8
      very_large: 1.2
```

**Formula:**
```
d = (μ₁ - μ₂) / σ_pooled
```

Where σ_pooled is the pooled standard deviation:
```
σ_pooled = √((n₁-1)s₁² + (n₂-1)s₂²) / (n₁ + n₂ - 2))
```

**Interpretation:**

| Cohen's d | Interpretation | Overlap |
|-----------|----------------|---------|
| 0.0 - 0.2 | Negligible | 92% |
| 0.2 - 0.5 | Small | 85% |
| 0.5 - 0.8 | Medium | 73% |
| 0.8 - 1.2 | Large | 63% |
| > 1.2 | Very Large | < 60% |

**Example:**
```yaml
result:
  cohens_d: 0.65
  interpretation: "Medium effect size"
  practical_significance: "Moderate performance difference"
```

### 4.3 Hedges' g

**When to Use:**
- Small sample sizes (n < 20)
- Corrects Cohen's d bias

**Configuration:**
```yaml
effect_sizes:
  - name: "Hedges' g"
    reference: "Hedges (1981). J. Ed. Stat., 6(2), 107-128"
    use_case: "Bias-corrected effect size for small samples"
```

### 4.4 Rank-Biserial Correlation

**When to Use:**
- Non-parametric effect size
- Mann-Whitney U test
- Ordinal data

**Citation:** Kerby, D. S. (2014). The Simple Difference Formula: An Approach to Teaching Nonparametric Correlation. Journal of Statistics Education, 22(3).

**Configuration:**
```yaml
effect_sizes:
  - name: "Rank-biserial correlation"
    reference: "Kerby (2014). J. Stat. Ed., 22(3)"
    use_case: "Effect size for Mann-Whitney U test"
    range: [-1, 1]
```

**Formula:**
```
r_rb = 1 - (2U) / (n₁ × n₂)
```

**Interpretation:**

| r_rb | Interpretation |
|------|----------------|
| 0.0 - 0.1 | Negligible |
| 0.1 - 0.3 | Small |
| 0.3 - 0.5 | Medium |
| > 0.5 | Large |

### 4.5 Cliff's Delta

**When to Use:**
- Non-parametric effect size
- Robust to outliers
- Alternative to rank-biserial

**Configuration:**
```yaml
effect_sizes:
  - name: "Cliff's delta"
    reference: "Cliff (1993). Psych. Bull., 114(3), 494-509"
    use_case: "Non-parametric effect size"
```

---

## 5. Outlier Detection

### 5.1 Interquartile Range (IQR) Method

**Citation:** Tukey, J. W. (1977). Exploratory Data Analysis. Addison-Wesley.

**Configuration:**
```yaml
outlier_detection:
  method: "IQR"
  reference: "Tukey, J. W. (1977). Exploratory Data Analysis"
  threshold: 1.5                        # IQR multiplier
  action: "remove_and_report"           # remove, keep, flag
```

**Algorithm:**
1. Calculate Q1 (25th percentile) and Q3 (75th percentile)
2. IQR = Q3 - Q1
3. Lower bound = Q1 - 1.5 × IQR
4. Upper bound = Q3 + 1.5 × IQR
5. Values outside bounds are outliers

**Threshold Values:**
- 1.5 × IQR: Standard (identifies ~0.7% of normal data)
- 3.0 × IQR: Far outliers (more conservative)

**Example:**
```yaml
outlier_results:
  total_samples: 100
  outliers_detected: 3
  outliers_removed: 3
  lower_bound: 9500
  upper_bound: 16500
  outlier_values: [7200, 18500, 19000]
```

### 5.2 Z-Score Method

**When to Use:**
- Normally distributed data
- Known mean and standard deviation

**Configuration:**
```yaml
outlier_detection:
  method: "z_score"
  threshold: 3.0                        # Standard deviations
  action: "remove_and_report"
```

**Algorithm:**
```
z = (x - μ) / σ
```
Remove values where |z| > threshold.

### 5.3 Modified Z-Score (MAD)

**When to Use:**
- Robust to outliers in outlier detection
- Asymmetric distributions

**Configuration:**
```yaml
outlier_detection:
  method: "modified_z_score"
  threshold: 3.5
  action: "remove_and_report"
```

**Algorithm:**
```
M_i = 0.6745 × (x_i - median) / MAD
```
Where MAD = median(|x_i - median(x)|)

---

## 6. Confidence Intervals

### 6.1 Purpose

Confidence intervals provide:
- Range of plausible values
- Uncertainty quantification
- More information than point estimates

### 6.2 Configuration

```yaml
methodology:
  confidence_intervals:
    confidence_level: 0.95              # 95% CI
    method: "t_distribution"            # t_distribution, bootstrap
    bootstrap_iterations: 10000         # For bootstrap method
```

### 6.3 t-Distribution Method

**When to Use:**
- Normal or near-normal data
- Unknown population variance
- Standard approach

**Formula:**
```
CI = x̄ ± t_(α/2, n-1) × (s / √n)
```

**Example:**
```yaml
confidence_interval:
  mean: 12500
  confidence_level: 0.95
  lower_bound: 12200
  upper_bound: 12800
  margin_of_error: 300
  interpretation: "We are 95% confident the true mean is between 12200 and 12800"
```

### 6.4 Bootstrap Method

**When to Use:**
- Non-normal distributions
- Complex statistics
- Small sample sizes

**Configuration:**
```yaml
confidence_intervals:
  method: "bootstrap"
  bootstrap_iterations: 10000
  bootstrap_method: "percentile"        # percentile, bca
```

**Algorithm:**
1. Resample data with replacement
2. Calculate statistic for each resample
3. Use percentiles as CI bounds

---

## 7. Measurement Protocol

### 7.1 Complete Protocol

```yaml
methodology:
  measurement:
    # System warm-up
    warm_up_seconds: 60                 # Allow caches to stabilize
    warm_up_iterations: null            # Or null for time-based
    
    # Measurement period
    measurement_duration_seconds: 300   # 5 minutes
    measurement_iterations: null        # Or number of iterations
    
    # Cool down
    cool_down_seconds: 30               # Clean shutdown
    
    # Multiple runs
    num_runs: 5                         # Independent runs
    inter_run_delay_seconds: 60         # Delay between runs
    
    # Data collection
    sampling_interval_ms: 100           # Metrics collection interval
    
    # Quality control
    min_successful_runs: 3              # Minimum successful runs
    max_cv_percent: 10                  # Maximum coefficient of variation
```

### 7.2 Warm-Up Period

**Purpose:**
- Stabilize caches
- Reach thermal equilibrium
- Initialize JIT compilation
- Load working set into memory

**Guidelines:**
- Minimum: 30-60 seconds
- OLTP workloads: 60-120 seconds
- Analytical workloads: 120-300 seconds
- Cold start benchmarks: 0 seconds (document rationale)

### 7.3 Measurement Duration

**Guidelines:**
- Minimum: 300 seconds (5 minutes)
- OLTP: 300-600 seconds
- Analytical: 600-1800 seconds
- Must be longer than any periodic effects

**Considerations:**
- Longer duration → more stable results
- Must exceed system's periodic behaviors
- Balance precision with time constraints

### 7.4 Multiple Independent Runs

**Why Multiple Runs:**
- Quantify variability
- Enable statistical testing
- Detect anomalies
- Increase confidence

**Guidelines:**
- Minimum: 3 runs
- Recommended: 5-10 runs
- High variability: 10-30 runs

**Inter-Run Delay:**
- Allow system to return to baseline
- Clear caches if needed
- Prevent thermal effects

### 7.5 Data Quality Control

**Coefficient of Variation (CV):**
```
CV = (σ / μ) × 100%
```

**Quality Thresholds:**
- CV < 5%: Excellent
- CV 5-10%: Good
- CV 10-20%: Fair
- CV > 20%: Poor (investigate)

**Actions if CV too high:**
1. Increase number of runs
2. Check for interference
3. Verify system stability
4. Review isolation settings

---

## 8. Reporting Standards

### 8.1 Required Reports

```yaml
methodology:
  reporting:
    # Descriptive statistics
    metrics:
      - "mean"
      - "median"
      - "std_dev"
      - "min"
      - "max"
      - "p25"                           # 25th percentile
      - "p50"                           # Median
      - "p75"                           # 75th percentile
      - "p95"                           # 95th percentile
      - "p99"                           # 99th percentile
      - "p99.9"                         # 99.9th percentile
    
    # Statistical analysis
    include:
      - "confidence_intervals"
      - "effect_sizes"
      - "hypothesis_test_results"
      - "outlier_summary"
      - "raw_data"                      # For verification
      - "system_metrics"                # CPU, memory, etc.
    
    # Formats
    formats:
      - "html"                          # Interactive reports
      - "json"                          # Machine-readable
      - "csv"                           # Raw statistics
      - "latex"                         # For papers
```

### 8.2 Descriptive Statistics Table

**Example:**

| Metric | System A | System B | Difference |
|--------|----------|----------|------------|
| Mean | 12,500 | 15,000 | +20.0% |
| Median | 12,400 | 14,900 | +20.2% |
| Std Dev | 500 | 600 | +20.0% |
| Min | 11,200 | 13,500 | +20.5% |
| Max | 13,800 | 16,500 | +19.6% |
| P95 | 13,200 | 15,800 | +19.7% |
| P99 | 13,500 | 16,200 | +20.0% |
| CV | 4.0% | 4.0% | — |

### 8.3 Statistical Test Results

**Example:**

```yaml
statistical_comparison:
  systems: ["System A", "System B"]
  metric: "throughput_ops_per_sec"
  
  welch_t_test:
    t_statistic: -8.45
    df: 8
    p_value: 0.000023
    is_significant: true
    significance_level: 0.05
    
  mann_whitney_u:
    u_statistic: 0
    z_score: -2.61
    p_value: 0.009
    is_significant: true
    
  effect_size:
    cohens_d: 3.77
    interpretation: "Very large effect"
    
  confidence_interval:
    difference_mean: 2500
    ci_95_lower: 1800
    ci_95_upper: 3200
    
  interpretation: >
    System B significantly outperforms System A (p < 0.001).
    The difference is very large (d = 3.77) and practically significant.
    We estimate System B is 1800-3200 ops/sec faster (95% CI).
```

---

## 9. Practical Examples

### 9.1 Complete Analysis Example

**Scenario:** Comparing throughput of two database systems.

**Data:**
- System A: [12500, 12300, 12700, 12400, 12600]
- System B: [15000, 14800, 15200, 14900, 15100]

**Step 1: Descriptive Statistics**
```yaml
system_a:
  mean: 12500
  median: 12500
  std_dev: 158
  min: 12300
  max: 12700
  cv: 1.26%

system_b:
  mean: 15000
  median: 15000
  std_dev: 158
  min: 14800
  max: 15200
  cv: 1.05%
```

**Step 2: Check Assumptions**
- Normality: Shapiro-Wilk test (p > 0.05)
- Independence: Runs collected independently
- Outliers: No outliers detected (IQR method)

**Step 3: Hypothesis Test**
```yaml
welch_t_test:
  null_hypothesis: "System A and B have equal mean throughput"
  alternative: "Means differ"
  t_statistic: -22.36
  df: 8
  p_value: 1.2e-8
  conclusion: "Reject null hypothesis (p < 0.001)"
```

**Step 4: Effect Size**
```yaml
cohens_d: 15.81
interpretation: "Extremely large effect size"
practical_significance: "System B is substantially faster"
```

**Step 5: Confidence Interval**
```yaml
difference_ci_95:
  mean_difference: 2500
  lower_bound: 2276
  upper_bound: 2724
  interpretation: "System B is 2276-2724 ops/sec faster (95% CI)"
```

**Step 6: Report**
```markdown
## Results

System B significantly outperformed System A by 20% 
(15,000 vs 12,500 ops/sec, p < 0.001, d = 15.81).

The 95% confidence interval for the difference is 
[2276, 2724] ops/sec, indicating high precision in our estimate.

The extremely large effect size (d = 15.81) confirms this is 
not only statistically significant but also practically meaningful.
```

### 9.2 Non-Normal Data Example

**Scenario:** Latency measurements with outliers.

**Data:**
- System A: [50, 52, 51, 50, 53, 51, 52, 150, 50, 51] ms

**Step 1: Check Normality**
```yaml
shapiro_wilk:
  statistic: 0.612
  p_value: 0.0001
  conclusion: "Data not normally distributed"
```

**Step 2: Outlier Detection**
```yaml
iqr_method:
  q1: 50
  q3: 52
  iqr: 2
  lower_bound: 47
  upper_bound: 55
  outliers_detected: [150]
  outliers_removed: 1
```

**Step 3: Use Non-Parametric Test**
```yaml
mann_whitney_u:
  use_reason: "Non-normal distribution, outliers present"
  u_statistic: 15
  z_score: -2.13
  p_value: 0.033
  rank_biserial: 0.4
  interpretation: "Medium effect size"
```

---

## Appendix A: Decision Tree

### Choosing the Right Statistical Test

```
Start
  │
  ├─ Comparing 2 groups?
  │   │
  │   ├─ Yes: Independent or paired?
  │   │   │
  │   │   ├─ Independent: Normal distribution?
  │   │   │   │
  │   │   │   ├─ Yes: Welch's t-test
  │   │   │   └─ No: Mann-Whitney U
  │   │   │
  │   │   └─ Paired: Normal differences?
  │   │       │
  │   │       ├─ Yes: Paired t-test
  │   │       └─ No: Wilcoxon signed-rank
  │   │
  │   └─ No: Comparing 3+ groups?
  │       │
  │       ├─ Yes: Normal distribution?
  │       │   │
  │       │   ├─ Yes: ANOVA
  │       │   └─ No: Kruskal-Wallis
  │       │
  │       └─ No: Testing distribution?
  │           │
  │           └─ Kolmogorov-Smirnov
```

---

**Document Version:** 1.0.0  
**Last Updated:** April 2026  
**License:** MIT  
**Maintainer:** CHIMERA Development Team
