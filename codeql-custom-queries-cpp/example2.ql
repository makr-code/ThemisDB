/**
 * @name Manual delete usage
 * @description Flags explicit delete/delete[] usage as a code-quality and lifetime-safety smell.
 * @kind problem
 * @problem.severity warning
 * @id cpp/themisdb/manual-delete-usage
 */

import cpp

from DeleteExpr del
select del,
    "Manual delete/delete[] usage detected. Prefer RAII, std::unique_ptr, or std::shared_ptr to keep ownership explicit and safe."
