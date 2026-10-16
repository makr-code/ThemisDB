/**
 * @name Unsafe C string APIs
 * @description Flags risky C string functions that can overflow or accept unsafe input.
 * @kind problem
 * @problem.severity warning
 * @id cpp/themisdb/unsafe-cstring-apis
 */

import cpp

from FunctionCall call, Function target
where target = call.getTarget() and (
    target.getName() = "strcpy" or
    target.getName() = "strcat" or
    target.getName() = "sprintf" or
    target.getName() = "vsprintf" or
    target.getName() = "gets"
)
select call,
    "Unsafe C string API '" + target.getName() + "' used. Prefer std::string, std::string_view, std::snprintf, or bounds-checked APIs."
