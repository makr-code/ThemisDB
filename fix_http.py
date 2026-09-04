import re

with open('src/server/http_server.cpp', 'r') as f:
    content = f.read()

# Fix line 667
content = content.replace(
    'if (sharding_enabled && (std::string(sharding_enabled) == "true" || std::string(sharding_enabled) == "1"))',
    'if ((sharding_enabled) && (std::string(sharding_enabled) == "true" || std::string(sharding_enabled) == "1"))'
)

# Fix lines with pattern: if (target == "..." && (method == ... || method == ...))
content = re.sub(
    r'if \(target == "([^"]+)" && \(method == http::verb::(\w+) \|\| method == http::verb::(\w+)\)\)',
    lambda m: f'if ((target == "{m.group(1)}") && (method == http::verb::{m.group(2)} || method == http::verb::{m.group(3)}))',
    content
)

# Fix multi-line patterns with && and ||
content = re.sub(
    r'if \(\(path_only == "([^"]+)" \|\| path_only == "([^"]+)"\) &&\n\s+method == http::verb::(\w+)\)',
    lambda m: f'if (((path_only == "{m.group(1)}" || path_only == "{m.group(2)}")) &&\n        method == http::verb::{m.group(3)})',
    content
)

with open('src/server/http_server.cpp', 'w') as f:
    f.write(content)

print("Fixed http_server.cpp")

