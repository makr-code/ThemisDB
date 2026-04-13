# Fish completion script for themisctl
# Installation:
#   # Per-user
#   cp tools/completion/themisctl.fish ~/.config/fish/completions/themisctl.fish

# ── Global flags ──────────────────────────────────────────────────────────────
complete -c themisctl -f -l host    -d 'ThemisDB host'          -r
complete -c themisctl -f -l port    -d 'ThemisDB port'          -r
complete -c themisctl -f -l token   -d 'Bearer auth token'      -r
complete -c themisctl -f -l timeout -d 'Request timeout (seconds)' -r
complete -c themisctl -f -l json    -d 'Print raw JSON responses'
complete -c themisctl -f -l no-color -d 'Disable ANSI colour output'
complete -c themisctl -f -s h -l help -d 'Print help'

# ── Helper: "no sub-command provided yet" ─────────────────────────────────────
function __themisctl_no_sub
    not __fish_seen_subcommand_from \
        health version query get put delete schema config branch snapshot admin index repl
end

# ── Top-level commands ────────────────────────────────────────────────────────
complete -c themisctl -f -n '__themisctl_no_sub' -a health   -d 'Check server liveness/readiness'
complete -c themisctl -f -n '__themisctl_no_sub' -a version  -d 'Print server version'
complete -c themisctl -f -n '__themisctl_no_sub' -a query    -d 'Execute an AQL query'
complete -c themisctl -f -n '__themisctl_no_sub' -a get      -d 'Retrieve an entity by key'
complete -c themisctl -f -n '__themisctl_no_sub' -a put      -d 'Create or update an entity'
complete -c themisctl -f -n '__themisctl_no_sub' -a delete   -d 'Delete an entity'
complete -c themisctl -f -n '__themisctl_no_sub' -a schema   -d 'Show schema'
complete -c themisctl -f -n '__themisctl_no_sub' -a config   -d 'Read or hot-reload server config'
complete -c themisctl -f -n '__themisctl_no_sub' -a branch   -d 'Branch management'
complete -c themisctl -f -n '__themisctl_no_sub' -a snapshot -d 'Snapshot management'
complete -c themisctl -f -n '__themisctl_no_sub' -a admin    -d 'Observability/cache statistics'
complete -c themisctl -f -n '__themisctl_no_sub' -a index    -d 'Automatic index recommendations'
complete -c themisctl -f -n '__themisctl_no_sub' -a repl     -d 'Start interactive REPL'

# ── config sub-commands ───────────────────────────────────────────────────────
complete -c themisctl -f -n '__fish_seen_subcommand_from config' -a get \
    -d 'Print current server configuration'
complete -c themisctl -f -n '__fish_seen_subcommand_from config' -a set \
    -d 'Hot-reload one or more config keys'

# config set completions for known keys
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'logging.level='         -d 'Log level (trace|debug|info|warn|error)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'logging.format='        -d 'Log format (text|json)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'request_timeout_ms='    -d 'Request timeout in ms (1000-300000)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'features.semantic_cache=' -d 'Enable/disable semantic cache (true|false)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'features.llm_store='    -d 'Enable/disable LLM store (true|false)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'features.cdc='          -d 'Enable/disable CDC (true|false)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'features.timeseries='   -d 'Enable/disable timeseries (true|false)'
complete -c themisctl -f \
    -n '__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set' \
    -a 'cdc_retention_hours='   -d 'CDC retention in hours (1-8760)'

# ── branch sub-commands ───────────────────────────────────────────────────────
complete -c themisctl -f -n '__fish_seen_subcommand_from branch' -a list   -d 'List all branches'
complete -c themisctl -f -n '__fish_seen_subcommand_from branch' -a create -d 'Create a branch'
complete -c themisctl -f -n '__fish_seen_subcommand_from branch' -a switch -d 'Switch active branch'
complete -c themisctl -f -n '__fish_seen_subcommand_from branch' -a delete -d 'Delete a branch'

# ── snapshot sub-commands ─────────────────────────────────────────────────────
complete -c themisctl -f -n '__fish_seen_subcommand_from snapshot' -a list   -d 'List snapshot tags'
complete -c themisctl -f -n '__fish_seen_subcommand_from snapshot' -a create -d 'Create a snapshot tag'

# ── admin sub-commands ────────────────────────────────────────────────────────
complete -c themisctl -f -n '__fish_seen_subcommand_from admin' -a stats -d 'Show observability health'
complete -c themisctl -f -n '__fish_seen_subcommand_from admin' -a cache -d 'Show cache statistics'

# ── index sub-commands ────────────────────────────────────────────────────────
complete -c themisctl -f -n '__fish_seen_subcommand_from index' -a recommend \
    -d 'Show automatic index recommendations'
