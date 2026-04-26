# Bash completion script for themisctl
# Source this file or place it in /etc/bash_completion.d/themisctl
# or ~/.local/share/bash-completion/completions/themisctl
#
# Installation:
#   # System-wide (requires root)
#   sudo cp tools/completion/themisctl.bash /etc/bash_completion.d/themisctl
#
#   # Per-user
#   mkdir -p ~/.local/share/bash-completion/completions
#   cp tools/completion/themisctl.bash \
#       ~/.local/share/bash-completion/completions/themisctl
#
#   # One-time activation in current shell
#   source tools/completion/themisctl.bash

_themisctl_complete() {
    local cur prev words cword
    _init_completion || return

    # ── Global flags (valid before any command) ──────────────────────────────
    local global_flags="--host --port --token --timeout --json --no-color --help -h"

    # ── All top-level commands ────────────────────────────────────────────────
    local commands="health version query get put delete schema config branch snapshot admin index repl"

    # If we're completing the very first token after the binary, or after a
    # global flag that takes a value, offer commands + flags.
    case "${prev}" in
        --host|--port|--token|--timeout)
            # These flags take a value; no further completions here
            return 0
            ;;
        themisctl)
            COMPREPLY=( $(compgen -W "${global_flags} ${commands}" -- "${cur}") )
            return 0
            ;;
    esac

    # Find the command token (first non-flag word after 'themisctl')
    local cmd=""
    local i
    for (( i=1; i < ${#words[@]}; i++ )); do
        local w="${words[i]}"
        case "${w}" in
            --host|--port|--token|--timeout)
                (( i++ ))  # skip the value token
                ;;
            --json|--no-color|--help|-h)
                ;;
            -*)
                ;;
            *)
                cmd="${w}"
                break
                ;;
        esac
    done

    case "${cmd}" in
        "")
            COMPREPLY=( $(compgen -W "${global_flags} ${commands}" -- "${cur}") )
            ;;
        config)
            local config_subs="get set"
            COMPREPLY=( $(compgen -W "${config_subs}" -- "${cur}") )
            if [[ "${prev}" == "set" ]]; then
                local config_keys="logging.level= logging.format= request_timeout_ms=
                                   features.semantic_cache= features.llm_store=
                                   features.cdc= features.timeseries=
                                   cdc_retention_hours="
                COMPREPLY=( $(compgen -W "${config_keys}" -- "${cur}") )
            fi
            ;;
        branch)
            local branch_subs="list create switch delete"
            COMPREPLY=( $(compgen -W "${branch_subs}" -- "${cur}") )
            ;;
        snapshot)
            local snapshot_subs="list create"
            COMPREPLY=( $(compgen -W "${snapshot_subs}" -- "${cur}") )
            ;;
        admin)
            local admin_subs="stats cache"
            COMPREPLY=( $(compgen -W "${admin_subs}" -- "${cur}") )
            ;;
        index)
            local index_subs="recommend"
            COMPREPLY=( $(compgen -W "${index_subs}" -- "${cur}") )
            ;;
        schema|get|delete)
            # No meaningful static completions for entity IDs / table names
            return 0
            ;;
        *)
            COMPREPLY=( $(compgen -W "${global_flags} ${commands}" -- "${cur}") )
            ;;
    esac
}

complete -F _themisctl_complete themisctl
