/**
 * @name Unsafe memory and shell APIs
 * @description Flags legacy memory and process APIs that are prone to overflow, unsafe copies, or execution of external commands.
 * @kind problem
 * @problem.severity warning
 * @id cpp/themisdb/unsafe-memory-and-shell-apis
 */

import cpp

from FunctionCall call, Function target
where target = call.getTarget() and (
    target.getName() = "memcpy" or
    target.getName() = "memmove" or
    target.getName() = "strncpy" or
    target.getName() = "strncat" or
    target.getName() = "system" or
    target.getName() = "popen" or
    target.getName() = "execvp" or
    target.getName() = "execl"
)
select call,
    "Unsafe or legacy API '" + target.getName() + "' used. Prefer bounds-checked containers, std::string/std::vector APIs, and explicit process invocation with validated arguments."
