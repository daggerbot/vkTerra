# Environment Variables

## Logging

| Variable | Values | Default | Description |
|---|---|---|---|
| `VKTERRA_LOCAL_TIME` | `0` \| `1` | `0` | Report timestamps in local time instead of UTC. Only has an effect when `VKTERRA_TIMESTAMPS` is enabled. |
| `VKTERRA_LOG_LEVEL` | `none` \| `fatal` \| `error` \| `warn` \| `info` \| `debug` \| `trace` | `debug` (debug builds)<br>`info` (dedicated server release)<br>`warn` (client release) | Minimum severity of log messages to output. |
| `VKTERRA_SOURCE_LOCATION` | `0` \| `1` | `1` (debug builds)<br>`0` (release builds) | Include the source location (file and line) in log messages. |
| `VKTERRA_TIMESTAMPS` | `0` \| `1` | `1` (dedicated server)<br>`0` (client) | Include timestamps in log messages. |

## Vulkan

| Variable | Values | Default | Description |
|---|---|---|---|
| `VULKAN_DEBUG_UTILS` | `0` \| `1` | `1` (debug builds)<br>`0` (release builds) | Enable the Vulkan debug utils extension. |
| `VULKAN_LOG_LEVEL` | `none` \| `error` \| `warn` \| `info` \| `trace` | `warn` | Minimum severity of messages from Vulkan to output. `VKTERRA_LOG_LEVEL` must be at least as verbose as this setting. |
| `VULKAN_VALIDATION` | `0` \| `1` | `1` (debug builds)<br>`0` (release builds) | Enable the Vulkan validation layers. |

## Terminal Colors

These variables control whether log messages include ANSI color codes. Precedence, from highest to lowest: `NO_COLOR` > `FORCE_COLOR` > `TERM`.

| Variable | Values | Default | Description |
|---|---|---|---|
| `FORCE_COLOR` | any value | unset | Always enable ANSI colors. Overrides `TERM`. |
| `NO_COLOR` | any value | unset | Always disable ANSI colors. Overrides `FORCE_COLOR` and `TERM`. |
| `TERM` | terminal type (e.g. `xterm-256color`) | set by the terminal | *(POSIX only)* Used to detect whether the terminal supports ANSI colors. |
